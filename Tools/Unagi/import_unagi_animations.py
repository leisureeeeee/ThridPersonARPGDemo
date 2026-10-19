"""Import FBX clips using the current Unagi mesh's Interchange settings.

Run through Import-UnagiAnimations.ps1 (UE editor Python, not system Python).
Existing assets are never replaced; only newly created animations are saved.
"""

import hashlib
import json
import os
from pathlib import Path
import re

import unreal


def file_digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    config = json.loads(Path(os.environ["UNAGI_IMPORT_CONFIG"]).read_text(encoding="utf-8-sig"))
    report_path = Path(config["report_path"])
    report = {"status": "running", "clips": [], "errors": []}
    report_path.parent.mkdir(parents=True, exist_ok=True)

    def write_report():
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    try:
        mesh = unreal.load_asset(config["mesh"])
        if not isinstance(mesh, unreal.SkeletalMesh):
            raise RuntimeError("The configured mesh is not a SkeletalMesh: " + config["mesh"])
        skeleton = mesh.get_editor_property("skeleton")
        if not skeleton:
            raise RuntimeError("The configured mesh has no skeleton")
        import_data = mesh.get_editor_property("asset_import_data")
        templates = [p for p in import_data.get_pipelines()
                     if isinstance(p, unreal.InterchangeGenericAssetsPipeline)]
        if len(templates) != 1:
            raise RuntimeError("Expected one saved InterchangeGenericAssetsPipeline on the mesh")
        template = templates[0]
        report["mesh"] = mesh.get_path_name()
        report["skeleton"] = skeleton.get_path_name()
        report["import_uniform_scale"] = template.get_editor_property("import_offset_uniform_scale")
        destination = config["destination"].rstrip("/")
        if not destination.startswith("/Game/") or ".." in destination:
            raise ValueError("Destination must be a /Game/ content folder")
        # Importing must not save an unrelated package or change its disk contents.
        content = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()))
        protected = [content / (obj.get_path_name().split(".")[0][6:] + ".uasset")
                     for obj in (mesh, skeleton)]
        before = {str(p): file_digest(p) for p in protected}
        report["protected_assets_sha256_before"] = before
        files = sorted(Path(config["source_dir"]).rglob("*"))
        files = [p for p in files if p.is_file() and p.suffix.lower() == ".fbx"]
        if not files:
            raise RuntimeError("No FBX files found under " + config["source_dir"])
        source_root = Path(config["source_dir"])
        planned = set()
        jobs = []
        for source in files:
            relative = source.relative_to(source_root)
            folders = [re.sub(r"[^A-Za-z0-9_]", "_", part) for part in relative.parts[:-1]]
            folder = "/".join([destination] + folders)
            suffix = source.stem.zfill(2) if source.stem.isdigit() else source.stem.removeprefix("Unagi_")
            name = "AN_Unagi_" + re.sub(r"[^A-Za-z0-9_]", "_", suffix)
            target = folder + "/" + name
            if target.casefold() in planned:
                raise RuntimeError("Ambiguous output name: " + target)
            planned.add(target.casefold())
            jobs.append((source, folder, name, target))
        registry = unreal.AssetRegistryHelpers.get_asset_registry()
        registry.search_all_assets(True)
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        for source, folder, name, target in jobs:
            entry = {"source": str(source), "asset": target}
            report["clips"].append(entry)
            if unreal.EditorAssetLibrary.does_asset_exist(target):
                entry["status"] = "skipped_existing"
                write_report()
                continue
            try:
                pipeline = unreal.InterchangeGenericAssetsPipeline()
                for transform_property in ("import_offset_uniform_scale", "import_offset_translation", "import_offset_rotation"):
                    pipeline.set_editor_property(transform_property, template.get_editor_property(transform_property))
                pipeline.set_editor_property("asset_name", name)
                pipeline.set_editor_property("use_source_name_for_asset", False)
                pipeline.set_editor_property("asset_type_sub_folders", False)
                common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
                common.set_editor_property("import_only_animations", True)
                common.set_editor_property("try_auto_select_skeleton", False)
                common.set_editor_property("skeleton", skeleton)
                common.set_editor_property("add_curve_metadata_to_skeleton", False)
                common_meshes = pipeline.get_editor_property("common_meshes_properties")
                original_meshes = template.get_editor_property("common_meshes_properties")
                for conversion_property in ("convert_statics_in_bone_hierarchy_to_skeletals", "convert_statics_with_animated_transform_to_skeletals", "convert_statics_with_morph_targets_to_skeletals"):
                    common_meshes.set_editor_property(conversion_property, original_meshes.get_editor_property(conversion_property))
                anim_options = pipeline.get_editor_property("animation_pipeline")
                anim_options.set_editor_property("import_animations", True)
                anim_options.set_editor_property("import_bone_tracks", True)
                anim_options.set_editor_property("import_custom_attribute", False)
                anim_options.set_editor_property("animation_range", unreal.InterchangeAnimationRange.TIMELINE)
                # The supplied clips have end times on a 1/60 s grid. 30 Hz
                # leaves half-frame endpoints and rejects eight of the files.
                anim_options.set_editor_property("use30_hz_to_bake_bone_animation", False)
                anim_options.set_editor_property("custom_bone_animation_sample_rate", 60)
                mesh_options = pipeline.get_editor_property("mesh_pipeline")
                mesh_options.set_editor_property("import_static_meshes", False)
                mesh_options.set_editor_property("import_skeletal_meshes", False)
                mesh_options.set_editor_property("update_skeleton_reference_pose", False)
                material_options = pipeline.get_editor_property("material_pipeline")
                material_options.set_editor_property("import_materials", False)
                material_options.get_editor_property("texture_pipeline").set_editor_property("import_textures", False)
                overrides = unreal.InterchangePipelineStackOverride()
                overrides.set_editor_property("override_pipelines", [unreal.SoftObjectPath(pipeline.get_path_name())])
                task = unreal.AssetImportTask()
                task.set_editor_property("filename", str(source))
                task.set_editor_property("destination_path", folder)
                task.set_editor_property("destination_name", name)
                task.set_editor_property("automated", True)
                task.set_editor_property("replace_existing", False)
                task.set_editor_property("save", False)
                task.set_editor_property("async_", False)
                task.set_editor_property("options", overrides)
                tools.import_asset_tasks([task])
                imported = list(task.get_objects())
                sequences = [obj for obj in imported if isinstance(obj, unreal.AnimSequence)]
                if len(sequences) != 1 or len(imported) != 1:
                    raise RuntimeError("Expected exactly one AnimSequence; got " + str(
                        [(obj.get_path_name(), obj.get_class().get_name()) for obj in imported]))
                sequence = sequences[0]
                if sequence.get_path_name().split(".")[0] != target:
                    raise RuntimeError("Importer returned an unexpected asset path: " + sequence.get_path_name())
                if sequence.get_editor_property("skeleton") != skeleton:
                    raise RuntimeError("Imported animation uses a different skeleton")
                frames = unreal.AnimationLibrary.get_num_frames(sequence)
                seconds = unreal.AnimationLibrary.get_sequence_length(sequence)
                tracks = list(unreal.AnimationLibrary.get_animation_track_names(sequence))
                if frames < 2 or seconds <= 0 or not tracks:
                    raise RuntimeError("The imported animation has no usable bone tracks/time range")
                sequence.set_preview_skeletal_mesh(mesh)
                if not unreal.EditorAssetLibrary.save_loaded_asset(sequence, only_if_is_dirty=False):
                    raise RuntimeError("Could not save " + sequence.get_path_name())
                entry.update(status="imported", frames=frames, seconds=seconds, bone_tracks=len(tracks))
            except Exception as error:
                entry.update(status="failed", error=str(error))
                report["errors"].append(str(source) + ": " + str(error))
                unreal.log_error(entry["error"])
            write_report()
        after = {str(p): file_digest(p) for p in protected}
        report["protected_assets_sha256_after"] = after
        if before != after:
            raise RuntimeError("An existing mesh/skeleton changed on disk; inspect before continuing")
        report["status"] = "failed" if report["errors"] else "complete"
        write_report()
        if report["errors"]:
            raise RuntimeError("Some imports failed; see " + str(report_path))
    except Exception as error:
        report["status"] = "failed"
        report["errors"].append(str(error))
        write_report()
        raise


if __name__ == "__main__":
    main()
