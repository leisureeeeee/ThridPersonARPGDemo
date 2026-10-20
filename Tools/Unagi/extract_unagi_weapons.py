"""Extract the five weapon objects from the existing character FBX using UE.

Run in an unattended UnrealEditor-Cmd Python commandlet. Source assets and
existing content are read-only; outputs must not already exist.
"""

import hashlib
import json
from pathlib import Path
import uuid

import unreal


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def vector_values(vector):
    return [float(vector.x), float(vector.y), float(vector.z)]


def main():
    root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    run_id = uuid.uuid4().hex
    work = root / "Saved/UnagiWeaponWork" / run_id
    work.mkdir(parents=True)
    report_path = work / "extraction-report.json"
    report = {"status": "running", "weapons": []}

    def write_report():
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    try:
        resources = list((root / "Content").glob("039-*"))
        sources = [p for folder in resources for p in folder.rglob("1.fbx") if p.parent.parent.name == "fbx"]
        if len(sources) != 1:
            raise RuntimeError("Cannot uniquely locate the character's numbered 1.fbx")
        source = sources[0]
        destination = "/Game/Asset/Unagi/Weapons"
        source_out = root / "SourceAssets/Unagi/Weapons"
        names = ["Unagi_Weapon{:02d}".format(index) for index in range(1, 6)]
        protected = [source] + [root / "Content/Asset/Unagi" / name for name in
                                ("SK_Unagi.uasset", "SKEL_Unagi.uasset", "Material__28.uasset")]
        before = {p.relative_to(root).as_posix(): digest(p) for p in protected}
        report.update(source=source.relative_to(root).as_posix(), import_uniform_scale=100,
                      pivot="Each mesh bounding-box center; not a verified grip point",
                      protected_sha256_before=before)
        registry = unreal.AssetRegistryHelpers.get_asset_registry()
        registry.search_all_assets(True)
        for name in names:
            asset = destination + "/SM_" + name
            if unreal.EditorAssetLibrary.does_asset_exist(asset) or (source_out / (name + ".fbx")).exists():
                raise RuntimeError("Output already exists; refusing to replace: " + name)
        material = unreal.load_asset("/Game/Asset/Unagi/Material__28")
        if not isinstance(material, unreal.MaterialInterface):
            raise RuntimeError("The current character's weapon material Material__28 is unavailable")

        pipeline = unreal.InterchangeGenericAssetsPipeline()
        pipeline.set_editor_property("import_offset_uniform_scale", 100.0)
        common = pipeline.get_editor_property("common_meshes_properties")
        common.set_editor_property("force_all_mesh_as_type", unreal.InterchangeForceMeshType.IFMT_STATIC_MESH)
        # Bake the scene-node rotation/scale, then remove its world-space
        # translation by centering the vertices below. This also preserves
        # source object names instead of anonymous geometry names.
        common.set_editor_property("bake_meshes", True)
        common.set_editor_property("bake_pivot_meshes", False)
        common.set_editor_property("convert_statics_with_animated_transform_to_skeletals", False)
        common.set_editor_property("convert_statics_with_morph_targets_to_skeletals", False)
        common.set_editor_property("recompute_normals", False)
        mesh_options = pipeline.get_editor_property("mesh_pipeline")
        mesh_options.set_editor_property("import_static_meshes", True)
        mesh_options.set_editor_property("import_skeletal_meshes", False)
        mesh_options.set_editor_property("combine_static_meshes_behavior", unreal.InterchangeCombineStaticMeshesBehavior.DO_NOT_COMBINE)
        mesh_options.set_editor_property("collision", False)
        mesh_options.set_editor_property("build_nanite", False)
        pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", False)
        material_options = pipeline.get_editor_property("material_pipeline")
        material_options.set_editor_property("import_materials", False)
        material_options.get_editor_property("texture_pipeline").set_editor_property("import_textures", False)
        overrides = unreal.InterchangePipelineStackOverride()
        overrides.set_editor_property("override_pipelines", [unreal.SoftObjectPath(pipeline.get_path_name())])
        task = unreal.AssetImportTask()
        task.set_editor_properties({"filename": str(source), "destination_path": "/Game/__UnagiWeaponWork_" + run_id,
                                    "automated": True, "save": False, "replace_existing": False,
                                    "async_": False, "options": overrides})
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        tools.import_asset_tasks([task])
        imported = list(task.get_objects())
        report["scratch_objects"] = [{"name": obj.get_name(), "class": obj.get_class().get_name()} for obj in imported]
        meshes = {}
        for name in names:
            candidates = [obj for obj in imported if isinstance(obj, unreal.StaticMesh) and obj.get_name().endswith(name)]
            if len(candidates) != 1:
                raise RuntimeError("Expected one static mesh for " + name + "; inspect scratch_objects")
            meshes[name] = candidates[0]

        source_out.mkdir(parents=True, exist_ok=True)
        for name in names:
            original = meshes[name]
            description = original.get_static_mesh_description(0)
            count = description.get_vertex_count()
            ids = [unreal.VertexID(id_value=index) for index in range(count)]
            if not all(description.is_vertex_valid(identity) for identity in ids):
                raise RuntimeError("Imported mesh has non-contiguous vertex IDs: " + name)
            points = [description.get_vertex_position(identity) for identity in ids]
            lo = [min(vector_values(point)[axis] for point in points) for axis in range(3)]
            hi = [max(vector_values(point)[axis] for point in points) for axis in range(3)]
            center = unreal.Vector(*[(a + b) / 2 for a, b in zip(lo, hi)])
            for identity, point in zip(ids, points):
                description.set_vertex_position(identity, point - center)
            mesh = tools.duplicate_asset("SM_" + name, destination, original)
            if not mesh:
                raise RuntimeError("Could not create target mesh " + name)
            mesh.set_editor_property("static_materials", original.get_editor_property("static_materials"))
            mesh.build_from_static_mesh_descriptions([description], build_simple_collision=False, fast_build=False)
            for slot in range(len(mesh.get_editor_property("static_materials"))):
                mesh.set_material(slot, material)
            if description.get_triangle_count() <= 0:
                raise RuntimeError("Output is missing triangles: " + name)
            uvs = [description.get_vertex_instance_uv(unreal.VertexInstanceID(id_value=index), 0)
                   for index in range(description.get_vertex_instance_count())]
            uv_bounds = [[min(uv.x for uv in uvs), min(uv.y for uv in uvs)],
                         [max(uv.x for uv in uvs), max(uv.y for uv in uvs)]]
            fbx_path = source_out / (name + ".fbx")
            options = unreal.FbxExportOption()
            options.set_editor_properties({"ascii": False, "collision": False, "level_of_detail": False,
                                           "export_source_mesh": True,
                                           "fbx_export_compatibility": unreal.FbxExportCompatibility.FBX_2020,
                                           "bake_material_inputs": unreal.FbxMaterialBakeMode.DISABLED})
            export = unreal.AssetExportTask()
            export.set_editor_properties({"object": mesh, "filename": str(fbx_path), "automated": True,
                                          "prompt": False, "replace_identical": False,
                                          "exporter": unreal.StaticMeshExporterFBX(), "options": options})
            if not unreal.Exporter.run_asset_export_task(export) or not fbx_path.is_file():
                raise RuntimeError("Could not export standalone FBX: " + name)
            # Record the new FBX for normal reimport, rather than the whole character.
            data = unreal.FbxStaticMeshImportData(outer=mesh)
            data.scripted_add_filename(str(fbx_path), 0, "Extracted weapon")
            mesh.set_editor_property("asset_import_data", data)
            if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
                raise RuntimeError("Could not save " + mesh.get_path_name())
            entry = {"source_object": name, "asset": mesh.get_path_name().split(".")[0],
                     "fbx": fbx_path.relative_to(root).as_posix(),
                     "vertices": count, "triangles": description.get_triangle_count(),
                     "uv0_bounds": uv_bounds,
                     "center_removed_cm": vector_values(center),
                     "dimensions_cm": [b - a for a, b in zip(lo, hi)],
                     "material": material.get_path_name(), "fbx_sha256": digest(fbx_path)}
            report["weapons"].append(entry)
            write_report()
        after = {p.relative_to(root).as_posix(): digest(p) for p in protected}
        report["protected_sha256_after"] = after
        if before != after:
            raise RuntimeError("A protected source/character file changed on disk")
        report["status"] = "complete"
        write_report()
        unreal.log("UNAGI_WEAPONS_EXTRACTED " + str(report_path))
    except Exception as error:
        report.update(status="failed", error=str(error))
        write_report()
        raise


if __name__ == "__main__":
    main()
