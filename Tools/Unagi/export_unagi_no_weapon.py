"""Remove only weapon geometry/skin from the source FBX, preserving the rig.

Run with system Python. Existing output files are never overwritten.
The five weapon models remain as Null transform nodes to keep all 234 rig
nodes and their animation/parent connections, including custom weapon bones.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

from fbx_binary import array, fingerprint, read, scalar, value, write


def sha(data):
    return hashlib.sha256(data).hexdigest()


def label(node):
    return value(node.properties[1]).split(b"\x00\x01")[0].decode("utf-8")


def export(source, output, report_path):
    if output.exists() or report_path.exists():
        raise FileExistsError("Refusing to replace an existing output or report")
    original = source.read_bytes()
    version, roots, footer = read(original)
    objects = next(n for n in roots if n.name == b"Objects")
    connections = next(n for n in roots if n.name == b"Connections")
    definitions = next(n for n in roots if n.name == b"Definitions")
    by_id = {value(n.properties[0]): n for n in objects.children}
    edges = [(value(n.properties[1]), value(n.properties[2])) for n in connections.children]
    weapon_names = {"Unagi_Weapon{:02d}".format(i) for i in range(1, 6)}
    weapons = {uid: n for uid, n in by_id.items() if n.name == b"Model" and label(n) in weapon_names}
    if {label(n) for n in weapons.values()} != weapon_names:
        raise ValueError("Source must contain exactly the five expected weapon models")
    geometries = {a for a, b in edges if b in weapons and by_id[a].name == b"Geometry"}
    if len(geometries) != 5:
        raise ValueError("Expected exactly five weapon geometry objects")
    # Follow only Deformer -> Geometry/Skin connections upstream. Bones are
    # also inputs to clusters, but must remain in the scene.
    removed = set(geometries)
    while True:
        more = {a for a, b in edges if b in removed and a in by_id and by_id[a].name == b"Deformer"}
        if more <= removed:
            break
        removed |= more
    if any(a in removed and b not in removed and b not in weapons for a, b in edges):
        raise ValueError("A weapon object is shared with a retained object")
    retained = {uid: fingerprint(n) for uid, n in by_id.items() if uid not in removed and uid not in weapons}
    retained_models = {uid: fingerprint(n) for uid, n in by_id.items() if n.name == b"Model"}
    removed_info = [{"id": uid, "type": by_id[uid].name.decode(),
                     "subtype": value(by_id[uid].properties[2]).decode()} for uid in sorted(removed)]
    for node in weapons.values():
        if value(node.properties[2]) != b"Mesh":
            raise ValueError("Weapon model is not a Mesh")
        node.properties[2] = scalar(b"Null", "S")
    objects.children = [n for n in objects.children if value(n.properties[0]) not in removed]
    # Remove material bindings on the now geometry-free transform nodes.
    connections.children = [n for n in connections.children
                            if not ({value(n.properties[1]), value(n.properties[2])} & removed)
                            and not (value(n.properties[2]) in weapons
                                     and by_id[value(n.properties[1])].name == b"Material")]
    counts = Counter(n.name for n in objects.children)
    for node in definitions.children:
        if node.name == b"ObjectType":
            kind = value(node.properties[0])
            count = 1 if kind == b"GlobalSettings" else counts[kind]
            node.child(b"Count").properties[0] = scalar(count, "I")
    definitions.child(b"Count").properties[0] = scalar(sum(counts.values()) + 1, "I")
    encoded = write(version, roots, footer)
    # Re-read the actual serialized bytes before publishing the new file.
    _, saved_roots, _ = read(encoded)
    saved_objects = next(n for n in saved_roots if n.name == b"Objects")
    saved_by_id = {value(n.properties[0]): n for n in saved_objects.children}
    if any(fingerprint(saved_by_id[uid]) != digest for uid, digest in retained.items()):
        raise RuntimeError("Retained object data changed")
    model_report = []
    for uid, node in saved_by_id.items():
        if node.name != b"Model":
            continue
        if uid in weapons:
            original_subtype = node.properties[2]
            node.properties[2] = scalar(b"Mesh", "S")
            matches = fingerprint(node) == retained_models[uid]
            node.properties[2] = original_subtype
        else:
            matches = fingerprint(node) == retained_models[uid]
        if not matches:
            raise RuntimeError("A model transform or hierarchy property changed")
        model_report.append(label(node))
    if len(model_report) != 234:
        raise RuntimeError("Expected all 234 source model nodes")
    mesh_info = []
    saved_edges = [(value(n.properties[1]), value(n.properties[2]))
                   for n in next(n for n in saved_roots if n.name == b"Connections").children]
    for uid, node in saved_by_id.items():
        if node.name == b"Model" and value(node.properties[2]) == b"Mesh":
            geometry = [saved_by_id[a] for a, b in saved_edges if b == uid and saved_by_id[a].name == b"Geometry"]
            if len(geometry) != 1:
                raise RuntimeError("Expected a single geometry per body part")
            geo = geometry[0]
            indices = array(geo.child(b"PolygonVertexIndex").properties[0])
            mesh_info.append({"name": label(node), "vertices": len(array(geo.child(b"Vertices").properties[0])) // 3,
                              "polygons": sum(i < 0 for i in indices), "geometry_fingerprint": fingerprint(geo)})
    if {n["name"] for n in mesh_info} != {"Unagi_Body", "Unagi_Face", "Unagi_Hair"}:
        raise RuntimeError("Unexpected remaining mesh objects")
    if source.read_bytes() != original:
        raise RuntimeError("Source changed during export")
    report = {"source": source.as_posix(), "output": output.as_posix(), "fbx_version": version,
              "source_sha256": sha(original), "output_sha256": sha(encoded),
              "remaining_meshes": mesh_info, "removed_objects": removed_info,
              "weapon_transform_nodes_kept": sorted(weapon_names), "retained_model_nodes": len(model_report),
              "unchanged_retained_objects": len(retained), "retained_object_properties_exact": True,
              "source_units_unchanged": True, "import_uniform_scale_for_this_project": 100,
              "animation_objects_unchanged": True,
              "notes": "Only weapon mesh/skin removed; all model transforms, body skins, morphs and source animation retained. Textures are not embedded."}
    output.parent.mkdir(parents=True, exist_ok=True)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive creation guards against a concurrent user export.
    with output.open("xb") as stream:
        stream.write(encoded)
    with report_path.open("x", encoding="utf-8") as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
    print(json.dumps({"output": str(output), "meshes": mesh_info, "model_nodes": len(model_report)}, ensure_ascii=False))


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--output", type=Path, default=root / "SourceAssets/Unagi/Character/Unagi_NoWeapon.fbx")
    parser.add_argument("--report", type=Path, default=root / "Docs/UnagiNoWeaponExtraction.json")
    args = parser.parse_args()
    sources = [args.source.resolve()] if args.source else list((root / "Content").glob("039-*/fbx/*/1.fbx"))
    if len(sources) != 1:
        raise ValueError("Specify a unique source using --source")
    # All command-line paths must belong to this project; run from its root so
    # the report contains portable relative paths.
    import os
    output = args.output.resolve().relative_to(root)
    report = args.report.resolve().relative_to(root)
    source = sources[0].relative_to(root)
    os.chdir(root)
    export(source, output, report)
