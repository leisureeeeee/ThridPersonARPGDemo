"""Bake the three selected clips into native animation defaults. Editor only."""
import math
import unreal


def _travel(sequence_path, sample_rate=60):
    sequence = unreal.EditorAssetLibrary.load_asset(sequence_path)
    if not isinstance(sequence, unreal.AnimSequence) or sequence.get_play_length() <= 0:
        raise ValueError('Expected an animation sequence with positive length.')
    length = sequence.get_play_length()
    count = max(1, round(length * sample_rate))
    samples = []
    previous = None
    distance = 0.0
    for index in range(count + 1):
        time = length * index / count
        pose = unreal.AnimSequenceService.get_pose_at_time(sequence_path, time, False)
        root = next((p for p in pose if str(p.bone_name) == 'root'), None)
        if root is None:
            raise ValueError('Missing root track.')
        location = root.transform.translation
        if previous is not None:
            delta = location - previous
            distance += math.hypot(delta.x, delta.y)
        samples.append(unreal.Vector2D(time, distance))
        previous = location
    if distance <= 0.01:
        raise ValueError('Clip has no usable horizontal root travel.')
    return samples, length, distance


def bake_ground_profiles(blueprint_path, start_path, walk_path, stop_path):
    """Caller must assign the same clips to Start/Walk/Stop graph nodes first."""
    if unreal.WidgetService.is_pie_running():
        raise RuntimeError('Stop PIE before baking profiles.')
    blueprint = unreal.EditorAssetLibrary.load_asset(blueprint_path)
    if not isinstance(blueprint, unreal.AnimBlueprint):
        raise ValueError('Expected an animation blueprint.')
    start, _, start_distance = _travel(start_path)
    _, walk_length, walk_distance = _travel(walk_path)
    stop, _, stop_distance = _travel(stop_path)
    for sample in stop:
        sample.y = max(0.0, stop_distance - sample.y)
    defaults = unreal.get_default_object(blueprint.generated_class())
    defaults.set_editor_property('StartDistanceSamples', start)
    defaults.set_editor_property('StopDistanceSamples', stop)
    defaults.set_editor_property('WalkAuthoredSpeed', walk_distance / walk_length)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
        raise RuntimeError('Failed to save animation blueprint.')
    return {'start_cm': start_distance, 'stop_cm': stop_distance,
            'walk_cm_per_second': walk_distance / walk_length,
            'start_samples': len(start), 'stop_samples': len(stop)}
