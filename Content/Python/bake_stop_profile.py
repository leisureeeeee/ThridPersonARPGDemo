"""Editor-only regeneration of the C++ stop profile after replacing the stop clip.

Call bake_stop_profile(anim_blueprint_path, stop_sequence_path) in UE Python.
The Stop Sequence Evaluator must reference the same sequence. Does not start PIE.
"""
import math
import unreal


def bake_stop_profile(anim_blueprint_path, stop_sequence_path, sample_rate=60):
    if unreal.WidgetService.is_pie_running():
        raise RuntimeError('Stop PIE before changing animation defaults.')
    blueprint = unreal.EditorAssetLibrary.load_asset(anim_blueprint_path)
    sequence = unreal.EditorAssetLibrary.load_asset(stop_sequence_path)
    if not isinstance(blueprint, unreal.AnimBlueprint) or not isinstance(sequence, unreal.AnimSequence):
        raise ValueError('Expected an AnimBlueprint and an AnimSequence.')
    if not math.isfinite(sample_rate) or sample_rate <= 0:
        raise ValueError('Sample rate must be positive and finite.')
    players = [n for n in unreal.BlueprintService.get_nodes_in_graph(anim_blueprint_path, 'Stop')
               if str(n.node_type) == 'AnimGraphNode_SequenceEvaluator']
    if len(players) != 1:
        raise ValueError('Stop must contain exactly one Sequence Evaluator.')
    assigned = unreal.AnimGraphService.get_node_animation_asset(
        anim_blueprint_path, 'Stop', players[0].node_id)
    # Some VibeUE versions only return assets for Sequence Players, not Evaluators.
    # In that case the caller must verify the evaluator's sequence in the editor.
    if assigned and assigned.split('.')[0] != stop_sequence_path.split('.')[0]:
        raise ValueError('Profile sequence must match the Stop evaluator animation.')
    length = sequence.get_play_length()
    if length <= 0:
        raise ValueError('Animation must have a positive length.')
    count = max(1, round(length * sample_rate))
    positions = []
    for index in range(count + 1):
        time = length * index / count
        pose = unreal.AnimSequenceService.get_pose_at_time(stop_sequence_path, time, False)
        root = next((p for p in pose if str(p.bone_name) == 'root'), None)
        if root is None:
            raise ValueError('Animation must contain the root bone.')
        positions.append((time, root.transform.translation))
    remaining = 0.0
    samples = []
    for index in range(len(positions) - 1, -1, -1):
        if index < len(positions) - 1:
            delta = positions[index + 1][1] - positions[index][1]
            remaining += math.hypot(delta.x, delta.y)
        samples.append(unreal.Vector2D(positions[index][0], remaining))
    samples.reverse()
    if samples[0].y <= 0.01:
        raise ValueError('No usable horizontal root travel in this clip.')
    # Only write after all source samples have been collected successfully.
    defaults = unreal.get_default_object(blueprint.generated_class())
    defaults.set_editor_property('StopDistanceSamples', samples)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False):
        raise RuntimeError('Failed to save the animation blueprint.')
    return len(samples), remaining
