"""Create new UE4-to-Unagi retarget assets with UE editor Python.

Run via UnrealEditor-Cmd -run=pythonscript. Set UNAGI_RIG_INSPECT=1 for
read-only inspection in a separate process. Existing assets are never edited.
This configures assets; it does not play, bake, or replace any animation.
"""

import hashlib
import json
import os
from pathlib import Path
import traceback

import unreal


DESTINATION = "/Game/Asset/Unagi/Rig"
SOURCE_MESH = "/Game/ARPG_Samurai/Demo/Mannequin/Character/Mesh/SK_Mannequin"
TARGET_MESH = "/Game/Asset/Unagi/SK_Unagi_NoWeapon"
SOURCE_RIG = DESTINATION + "/IK_UE4Mannequin"
TARGET_RIG = DESTINATION + "/IK_Unagi"
RETARGETER = DESTINATION + "/RTG_UE4_To_Unagi"

# Matching chain names describe anatomy, while each skeleton keeps its own names.
SOURCE_CHAINS = {
    "Spine": ("spine_01", "spine_03"),
    "LeftClavicle": ("clavicle_l", "clavicle_l"),
    "RightClavicle": ("clavicle_r", "clavicle_r"),
    "LeftArm": ("upperarm_l", "hand_l"),
    "RightArm": ("upperarm_r", "hand_r"),
    "LeftLeg": ("thigh_l", "foot_l"),
    "RightLeg": ("thigh_r", "foot_r"),
    "Neck": ("neck_01", "neck_01"),
    "Head": ("head", "head"),
}
TARGET_CHAINS = {
    "Spine": ("Bip001-Spine", "Bip001-Spine2"),
    "LeftClavicle": ("Bip001-L-Clavicle", "Bip001-L-Clavicle"),
    "RightClavicle": ("Bip001-R-Clavicle", "Bip001-R-Clavicle"),
    "LeftArm": ("Bip001-L-UpperArm", "Bip001-L-Hand"),
    "RightArm": ("Bip001-R-UpperArm", "Bip001-R-Hand"),
    "LeftLeg": ("Bip001-L-Thigh", "Bip001-L-Foot"),
    "RightLeg": ("Bip001-R-Thigh", "Bip001-R-Foot"),
    "Neck": ("Bip001-Neck", "Bip001-Neck"),
    "Head": ("Bip001-Head", "Bip001-Head"),
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def asset_file(root, asset_path):
    return root / "Content" / (asset_path.removeprefix("/Game/") + ".uasset")


def load_mesh(path, chains, pelvis):
    mesh = unreal.load_asset(path)
    require(isinstance(mesh, unreal.SkeletalMesh), "Missing skeletal mesh: " + path)
    component = unreal.SkeletalMeshComponent()
    component.set_skeletal_mesh_asset(mesh)
    bones = [str(component.get_bone_name(i)) for i in range(component.get_num_bones())]
    require(pelvis in bones, "Missing pelvis: " + pelvis)
    for name, (start, end) in chains.items():
        require(start in bones and end in bones, "Missing chain endpoint: " + name)
        # End must descend from start; auxiliary sibling bones are not included.
        ancestor = end
        visited = set()
        while ancestor != start and ancestor in bones and ancestor not in visited:
            visited.add(ancestor)
            ancestor = str(mesh.get_bone_parent(ancestor))
        require(ancestor == start, "Invalid chain hierarchy: " + name)
    return mesh, bones


def read_rig(rig, mesh, chains, pelvis, root_motion_bone):
    controller = unreal.IKRigController.get_controller(rig)
    require(controller.get_skeletal_mesh() == mesh, "Wrong rig preview mesh")
    require(str(controller.get_retarget_root()) == pelvis, "Wrong pelvis")
    require(str(controller.get_root_motion_bone()) == root_motion_bone, "Wrong root-motion bone")
    names = [str(c.get_editor_property("chain_name")) for c in controller.get_retarget_chains()]
    actual = {name: (str(controller.get_retarget_chain_start_bone(name)),
                     str(controller.get_retarget_chain_end_bone(name))) for name in names}
    require(actual == chains, "Rig chain definitions differ: " + rig.get_path_name()
            + " actual=" + repr(actual) + " expected=" + repr(chains))
    return {"asset": rig.get_path_name(), "mesh": mesh.get_path_name(),
            "skeleton": mesh.get_editor_property("skeleton").get_path_name(),
            "pelvis": pelvis, "root_motion_bone": root_motion_bone,
            "chains": {name: {"start": pair[0], "end": pair[1]} for name, pair in actual.items()},
            "solver_count": controller.get_num_solvers()}


def make_rig(path, mesh, chains, pelvis, root_motion_bone):
    rig = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        path.rsplit("/", 1)[1], DESTINATION, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    require(rig is not None, "Could not create " + path)
    controller = unreal.IKRigController.get_controller(rig)
    require(controller.set_skeletal_mesh(mesh), "Could not assign mesh")
    require(controller.set_retarget_root(pelvis), "Could not assign pelvis")
    require(controller.set_root_motion_bone(root_motion_bone), "Could not assign root-motion bone")
    for name, (start, end) in chains.items():
        require(str(controller.add_retarget_chain(name, start, end, "None")) == name,
                "Could not create chain: " + name)
    read_rig(rig, mesh, chains, pelvis, root_motion_bone)
    return rig


def read_retargeter(asset, source, target, source_mesh, target_mesh):
    controller = unreal.IKRetargeterController.get_controller(asset)
    side = unreal.RetargetSourceOrTarget
    require(controller.get_ik_rig(side.SOURCE) == source, "Wrong source IK Rig")
    require(controller.get_ik_rig(side.TARGET) == target, "Wrong target IK Rig")
    require(controller.get_preview_mesh(side.SOURCE) == source_mesh, "Wrong source mesh")
    require(controller.get_preview_mesh(side.TARGET) == target_mesh, "Wrong target mesh")
    mapping = {name: str(controller.get_source_chain(name)) for name in TARGET_CHAINS}
    require(all(name == source_name for name, source_name in mapping.items()), "Incomplete chain mapping")
    ops = [{"name": str(controller.get_op_name(i)),
            "controller": controller.get_op_controller(i).get_class().get_name(),
            "enabled": controller.get_retarget_op_enabled(i)}
           for i in range(controller.get_num_retarget_ops())]
    require(any(op["enabled"] and "FKChains" in op["controller"] for op in ops), "Missing active FK operation")
    return {"asset": asset.get_path_name(), "source_rig": source.get_path_name(),
            "target_rig": target.get_path_name(), "chain_mapping": mapping, "operations": ops,
            "pose_alignment": "default reference poses; visual adjustment pending",
            "animation_exported": False}


def main():
    root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    work = root / "Saved/UnagiIKRigWork"
    work.mkdir(parents=True, exist_ok=True)
    inspect_only = os.environ.get("UNAGI_RIG_INSPECT") == "1"
    report_file = work / ("inspection-report.json" if inspect_only else "creation-report.json")
    report = {"status": "running", "inspect_only": inspect_only, "saved_assets": []}
    protected = [root / "Config/DefaultEditor.ini", root / "Content/NewMap.umap"]
    protected += [p for p in (root / "Content/Asset/Unagi").glob("*.uasset")]
    protected += [p for p in (root / "Content/ARPG_Samurai/Demo/Mannequin/Character/Mesh").glob("*.uasset")]
    protected += [p for p in (root / "Content/Katana_Animations/Animations/Sequence/01_Idle").glob("*.uasset")]

    def hashes():
        return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in protected if p.is_file()}

    before = hashes()
    report["protected_sha256_before"] = before
    try:
        unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
        source_mesh, source_bones = load_mesh(SOURCE_MESH, SOURCE_CHAINS, "pelvis")
        target_mesh, target_bones = load_mesh(TARGET_MESH, TARGET_CHAINS, "Bip001-Pelvis")
        paths = [SOURCE_RIG, TARGET_RIG, RETARGETER]
        if inspect_only:
            source, target, asset = [unreal.load_asset(path) for path in paths]
            require(all((source, target, asset)), "Missing saved retarget asset")
        else:
            for path in paths:
                require(not unreal.EditorAssetLibrary.does_asset_exist(path) and not asset_file(root, path).exists(),
                        "Refusing to overwrite existing asset: " + path)
            source = make_rig(SOURCE_RIG, source_mesh, SOURCE_CHAINS, "pelvis", source_bones[0])
            target = make_rig(TARGET_RIG, target_mesh, TARGET_CHAINS, "Bip001-Pelvis", target_bones[0])
            asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                "RTG_UE4_To_Unagi", DESTINATION, unreal.IKRetargeter, unreal.IKRetargetFactory())
            require(asset is not None, "Could not create retargeter")
            controller = unreal.IKRetargeterController.get_controller(asset)
            side = unreal.RetargetSourceOrTarget
            controller.set_ik_rig(side.SOURCE, source)
            controller.set_ik_rig(side.TARGET, target)
            controller.set_preview_mesh(side.SOURCE, source_mesh)
            controller.set_preview_mesh(side.TARGET, target_mesh)
            controller.add_default_ops()
            controller.assign_ik_rig_to_all_ops(side.SOURCE, source)
            controller.assign_ik_rig_to_all_ops(side.TARGET, target)
            for i in range(controller.get_num_retarget_ops()):
                controller.run_op_initial_setup(i)
                op_class = controller.get_op_controller(i).get_class().get_name()
                # Basic chain transfer uses pelvis + FK. There are no IK goals
                # or solvers yet; leave the optional IK solving operation off.
                if "RunIKRig" in op_class or "IKChains" in op_class:
                    require(controller.set_retarget_op_enabled(i, False), "Could not disable unused IK op")
            for name in TARGET_CHAINS:
                require(controller.set_source_chain(name, name), "Could not map chain: " + name)

        report["source"] = read_rig(source, source_mesh, SOURCE_CHAINS, "pelvis", source_bones[0])
        report["target"] = read_rig(target, target_mesh, TARGET_CHAINS, "Bip001-Pelvis", target_bones[0])
        report["retargeter"] = read_retargeter(asset, source, target, source_mesh, target_mesh)
        if not inspect_only:
            for obj in (source, target, asset):
                require(unreal.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False),
                        "Could not save " + obj.get_path_name())
                report["saved_assets"].append(obj.get_path_name())
        report["output_sha256"] = {path: hashlib.sha256(asset_file(root, path).read_bytes()).hexdigest()
                                   for path in paths}
        after = hashes()
        report["protected_sha256_after"] = after
        require(before == after, "A protected existing file changed on disk")
        report["status"] = "complete"
        unreal.log("UNAGI_IK_RIG_ASSETS_COMPLETE " + str(report_file))
    except Exception:
        report["status"] = "failed"
        report["error"] = traceback.format_exc()
        raise
    finally:
        report_file.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
