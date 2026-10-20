"""Import the exported character as a new mesh sharing the existing skeleton.

Run with UnrealEditor-Cmd's Python commandlet. Only the new mesh is saved.
"""

import hashlib
import json
from pathlib import Path
import uuid

import unreal


def main():
    root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    work = root / "Saved/UnagiNoWeaponWork" / uuid.uuid4().hex
    work.mkdir(parents=True)
    report_path = work / "import-report.json"
    report = {"status": "running"}

    def persist():
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    try:
        destination = "/Game/Asset/Unagi"
        name = "SK_Unagi_NoWeapon"
        target = destination + "/" + name
        unreal.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            raise FileExistsError("Refusing to overwrite " + target)
        original = unreal.load_asset(destination + "/SK_Unagi")
        skeleton = original.get_editor_property("skeleton")
        protected = [root / "Content/Asset/Unagi" / (item + ".uasset") for item in
                     ("SK_Unagi", "SKEL_Unagi", "1_PhysicsAsset", "Material__25", "Material__26", "Material__27", "Material__28")]
        protected += list((root / "Content").glob("039-*/fbx/*/1.fbx"))
        before = {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
        report["protected_sha256_before"] = before
        templates = [p for p in original.get_editor_property("asset_import_data").get_pipelines()
                     if isinstance(p, unreal.InterchangeGenericAssetsPipeline)]
        if len(templates) != 1:
            raise RuntimeError("Expected the original mesh's import pipeline")
        # Copy the relevant settings into a fresh pipeline; do not mutate the
        # original mesh's saved pipeline/import metadata.
        pipeline = unreal.InterchangeGenericAssetsPipeline()
        template = templates[0]
        for prop in ("import_offset_uniform_scale", "import_offset_translation", "import_offset_rotation"):
            pipeline.set_editor_property(prop, template.get_editor_property(prop))
        mesh_settings = pipeline.get_editor_property("common_meshes_properties")
        source_settings = template.get_editor_property("common_meshes_properties")
        for prop in ("force_all_mesh_as_type", "convert_statics_with_animated_transform_to_skeletals",
                     "convert_statics_with_morph_targets_to_skeletals", "convert_statics_in_bone_hierarchy_to_skeletals",
                     "bake_meshes", "bake_pivot_meshes", "recompute_normals", "recompute_tangents", "use_mikk_t_space",
                     "compute_weighted_normals", "use_high_precision_tangent_basis", "use_full_precision_u_vs",
                     "remove_degenerates"):
            mesh_settings.set_editor_property(prop, source_settings.get_editor_property(prop))
        pipeline.set_editor_properties({"asset_name": name, "use_source_name_for_asset": False,
                                        "asset_type_sub_folders": False})
        common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
        common.set_editor_properties({"import_only_animations": False, "try_auto_select_skeleton": False,
                                      "skeleton": skeleton, "add_curve_metadata_to_skeleton": False,
                                      "use_t0_as_ref_pose": False})
        options = pipeline.get_editor_property("mesh_pipeline")
        options.set_editor_properties({"import_skeletal_meshes": True, "import_static_meshes": False,
                                       "combine_skeletal_meshes_behavior": unreal.InterchangeCombineSkeletalMeshesBehavior.BY_SKELETON,
                                       "import_morph_targets": True, "update_skeleton_reference_pose": False,
                                       "create_physics_asset": False, "physics_asset": None})
        pipeline.get_editor_property("animation_pipeline").set_editor_property("import_animations", False)
        materials = pipeline.get_editor_property("material_pipeline")
        materials.set_editor_property("import_materials", False)
        materials.get_editor_property("texture_pipeline").set_editor_property("import_textures", False)
        override = unreal.InterchangePipelineStackOverride()
        override.set_editor_property("override_pipelines", [unreal.SoftObjectPath(pipeline.get_path_name())])
        task = unreal.AssetImportTask()
        task.set_editor_properties({"filename": str(root / "SourceAssets/Unagi/Character/Unagi_NoWeapon.fbx"),
                                    "destination_path": destination, "destination_name": name,
                                    "automated": True, "save": False, "replace_existing": False,
                                    "async_": False, "options": override})
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        imported = list(task.get_objects())
        report["imported"] = [{"path": x.get_path_name(), "class": x.get_class().get_name()} for x in imported]
        persist()
        if len(imported) != 1 or not isinstance(imported[0], unreal.SkeletalMesh):
            raise RuntimeError("Expected exactly one skeletal mesh, without new skeleton/material/animation")
        mesh = imported[0]
        if mesh.get_path_name().split(".")[0] != target or mesh.get_editor_property("skeleton") != skeleton:
            raise RuntimeError("Unexpected mesh path or skeleton")
        # Map slots by source names, rather than assuming a material order.
        material_map = {}
        for slot in original.get_editor_property("materials"):
            for prop in ("material_slot_name", "imported_material_slot_name"):
                material_map[str(slot.get_editor_property(prop))] = slot.get_editor_property("material_interface")
        slots = list(mesh.get_editor_property("materials"))
        for slot in slots:
            key = str(slot.get_editor_property("imported_material_slot_name"))
            if key not in material_map or not material_map[key]:
                raise RuntimeError("No original material for " + key)
            slot.set_editor_property("material_interface", material_map[key])
        mesh.set_editor_property("materials", slots)
        if len(slots) != 3:
            raise RuntimeError("Expected hair/body/face materials only")
        original_component = unreal.SkeletalMeshComponent()
        original_component.set_skeletal_mesh_asset(original)
        component = unreal.SkeletalMeshComponent()
        component.set_skeletal_mesh_asset(mesh)
        original_names = [str(original_component.get_bone_name(i)) for i in range(original_component.get_num_bones())]
        names = [str(component.get_bone_name(i)) for i in range(component.get_num_bones())]
        report.update(bone_count=len(names), original_bone_count=len(original_names),
                      material_slots=[{"name": str(s.material_slot_name), "material": s.material_interface.get_path_name()} for s in slots],
                      morph_targets=[m.get_name() for m in mesh.get_editor_property("morph_targets")],
                      bounds=str(mesh.get_imported_bounds()), import_scale=pipeline.get_editor_property("import_offset_uniform_scale"))
        if names != original_names:
            report.update(missing_bones=sorted(set(original_names) - set(names)), added_bones=sorted(set(names) - set(original_names)))
            raise RuntimeError("Bone names/order do not exactly match the original")
        if any(mesh.get_bone_parent(n) != original.get_bone_parent(n) for n in names):
            raise RuntimeError("A bone parent differs from the original")
        if mesh.get_imported_bounds().box_extent.z < 75 or mesh.get_imported_bounds().box_extent.z > 130:
            raise RuntimeError("Unexpected model height")
        if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
            raise RuntimeError("Could not save the new mesh")
        after = {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
        report["protected_sha256_after"] = after
        if before != after:
            raise RuntimeError("An original resource changed on disk")
        report.update(status="complete", asset=target, skeleton=skeleton.get_path_name(), bones_and_parents_match=True)
        persist()
        unreal.log("UNAGI_NO_WEAPON_IMPORTED " + str(report_path))
    except Exception as error:
        report.update(status="failed", error=str(error))
        persist()
        raise


if __name__ == "__main__":
    main()
