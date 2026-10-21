"""Editor-only batch skeleton assignment. Back up assets before calling apply_batch.

Use with the compiled ARPGAnimationAssetTools bridge. This does not retarget poses.
"""
import unreal


def load_sequence(path):
    try:
        asset = unreal.EditorAssetLibrary.load_asset(path)
    except Exception:
        # A missing skeleton can make FKControlRig post-load report an error even
        # though the sequence and its source data model have loaded successfully.
        asset = unreal.find_object(None, path + '.' + path.rsplit('/', 1)[-1])
    if not isinstance(asset, unreal.AnimSequence):
        raise ValueError('Could not load animation sequence: ' + path)
    return asset


def apply_batch(paths, target_path, allowed_unmapped_tracks, required_folder):
    if unreal.WidgetService.is_pie_running():
        raise RuntimeError('Stop PIE before assigning animation skeletons.')
    target = unreal.EditorAssetLibrary.load_asset(target_path)
    if not isinstance(target, unreal.Skeleton):
        raise ValueError('Target is not a valid skeleton.')
    tool = unreal.get_default_object(unreal.load_class(None, '/Script/ThirdPersonARPG.ARPGAnimationAssetTools'))
    allowed = [unreal.Name(name) for name in allowed_unmapped_tracks]
    sequences = []
    for path in paths:
        if not path.startswith(required_folder.rstrip('/') + '/'):
            raise ValueError('Asset is outside the requested folder: ' + path)
        asset = load_sequence(path)
        error = tool.call_method('ValidateSkeletonAssignment', (asset, target, allowed))
        if error:
            raise RuntimeError(path + ': ' + error)
        sequences.append((path, asset))
    results = []
    for path, asset in sequences:
        changed = asset.get_skeleton() != target
        before = (asset.get_play_length(), asset.get_editor_property('enable_root_motion'),
                  asset.get_editor_property('force_root_lock'))
        if changed:
            error = tool.call_method('AssignCompatibleSkeleton', (asset, target, allowed))
            if error:
                raise RuntimeError(path + ': ' + error)
            after = (asset.get_play_length(), asset.get_editor_property('enable_root_motion'),
                     asset.get_editor_property('force_root_lock'))
            if before != after:
                raise RuntimeError('Duration or root-motion settings changed; do not save: ' + path)
            if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
                raise RuntimeError('Failed to save: ' + path)
        if asset.get_skeleton() != target:
            raise RuntimeError('Skeleton did not match after saving: ' + path)
        results.append({'path': path, 'changed': changed, 'skeleton': target.get_path_name(),
                        'length': before[0], 'enable_root_motion': before[1], 'force_root_lock': before[2]})
    return results
