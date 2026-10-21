"""Derive head-aligned directional starts; never edit source animations.

Only head local rotation is changed. Horizontal heading is matched to the
pelvis relative to the normalized-time forward-start baseline. Raw root and
pelvis motion therefore remain compatible with the existing C++ profiles.
"""
import math
import unreal


def _pose(path, time):
    return {str(p.bone_name): p.transform for p in
            unreal.AnimSequenceService.get_pose_at_time(path, time, True)}


def _inverse(q):
    n = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w
    if n < 1e-8:
        raise ValueError('Invalid rotation quaternion.')
    return unreal.Quat(-q.x/n, -q.y/n, -q.z/n, q.w/n)


def _heading(q, reference):
    d = (q * _inverse(reference)).rotate_vector(unreal.Vector(0, 1, 0))
    if math.hypot(d.x, d.y) < 1e-4:
        raise ValueError('Heading axis has no stable horizontal projection.')
    return math.degrees(math.atan2(d.y, d.x)) - 90.0


def _wrap(angle):
    return (angle + 180.0) % 360.0 - 180.0


def bake_head_alignment(forward_path, source_path, destination_path):
    if unreal.WidgetService.is_pie_running():
        raise RuntimeError('Stop PIE before authoring animation assets.')
    if unreal.EditorAssetLibrary.does_asset_exist(destination_path):
        raise RuntimeError('Refusing to overwrite existing asset: ' + destination_path)
    source = unreal.load_asset(source_path)
    forward = unreal.load_asset(forward_path)
    if not isinstance(source, unreal.AnimSequence) or not isinstance(forward, unreal.AnimSequence):
        raise ValueError('Expected animation sequences.')
    if source.get_skeleton() != forward.get_skeleton():
        raise ValueError('Skeletons must match.')
    length = source.get_play_length()
    rate = unreal.AnimSequenceService.get_animation_frame_rate(source_path)
    if rate <= 0 or length <= 0:
        raise ValueError('Invalid sequence duration/frame rate.')
    count = round(length * rate)
    reference = _pose(forward_path, 0)
    rotations = []
    max_correction = 0.0
    for i in range(count + 1):
        fraction = i / count
        pose = _pose(source_path, length * fraction)
        baseline = _pose(forward_path, forward.get_play_length() * fraction)
        deltas = {}
        for bone in ('pelvis', 'head'):
            deltas[bone] = _wrap(_heading(pose[bone].rotation, reference[bone].rotation)
                                 - _heading(baseline[bone].rotation, reference[bone].rotation))
        correction = _wrap(deltas['pelvis'] - deltas['head'])
        yaw = unreal.Rotator(pitch=0, yaw=correction, roll=0).quaternion()
        # UE4 head parent is neck_01; convert the desired component rotation to local.
        rotation = _inverse(pose['neck_01'].rotation) * yaw * pose['head'].rotation
        rotations.append(rotation.rotator())
        max_correction = max(max_correction, abs(correction))
    destination = unreal.EditorAssetLibrary.duplicate_asset(source_path, destination_path)
    if not destination:
        raise RuntimeError('Failed to duplicate animation.')
    for i, rotation in enumerate(rotations):
        result = unreal.AnimSequenceService.apply_bone_rotation(
            destination_path, 'head', rotation, 'local', i, i, False)
        if not result or not result.success:
            raise RuntimeError('Failed to author head frame ' + str(i))
    if not unreal.EditorAssetLibrary.save_loaded_asset(destination, False):
        raise RuntimeError('Failed to save corrected animation.')
    return {'asset': destination_path, 'frames': count + 1,
            'max_correction_degrees': max_correction}
