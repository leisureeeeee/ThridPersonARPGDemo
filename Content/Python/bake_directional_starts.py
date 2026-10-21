"""Bake body yaw relative to a normalized-time forward start, not Euler pelvis yaw.

The UE4 pelvis reference orientation is nearly vertical, so direct Euler yaw is
unstable. Compare quaternion-rotated horizontal axes instead.
"""
import math
import unreal


def _pose(path, time):
    pose = unreal.AnimSequenceService.get_pose_at_time(path, time, False)
    return {str(p.bone_name): p.transform for p in pose}


def _heading(rotation, reference):
    inverse = unreal.Quat(-reference.x, -reference.y, -reference.z, reference.w)
    direction = (rotation * inverse).rotate_vector(unreal.Vector(0, 1, 0))
    angle = math.degrees(math.atan2(direction.y, direction.x)) - 90
    return (angle + 180) % 360 - 180


def bake_profile(forward_path, directional_path):
    forward = unreal.EditorAssetLibrary.load_asset(forward_path)
    animation = unreal.EditorAssetLibrary.load_asset(directional_path)
    if not isinstance(forward, unreal.AnimSequence) or not isinstance(animation, unreal.AnimSequence):
        raise ValueError('Expected animation sequences.')
    if forward.get_skeleton() != animation.get_skeleton():
        raise ValueError('Animations must use the same skeleton.')
    reference = _pose(forward_path, 0)['pelvis'].rotation
    length = animation.get_play_length()
    count = max(1, round(length * 60))
    samples = []
    distance = 0.0
    previous_location = None
    previous_yaw = 0.0
    unwrapped_yaw = 0.0
    for index in range(count + 1):
        fraction = index / count
        pose = _pose(directional_path, length * fraction)
        baseline = _pose(forward_path, forward.get_play_length() * fraction)
        location = pose['root'].translation
        if previous_location is not None:
            delta = location - previous_location
            distance += math.hypot(delta.x, delta.y)
        yaw = _heading(pose['pelvis'].rotation, reference) - _heading(baseline['pelvis'].rotation, reference)
        yaw = (yaw + 180) % 360 - 180
        unwrapped_yaw += (yaw - previous_yaw + 180) % 360 - 180
        samples.append(unreal.Vector(length * fraction, distance, unwrapped_yaw))
        previous_location = location
        previous_yaw = yaw
    if abs(samples[-1].z) < 45:
        raise ValueError('Clip does not contain a usable directional body turn.')
    profile = unreal.ARPGDirectionalStartProfile()
    profile.set_editor_property('Animation', animation)
    profile.set_editor_property('AuthoredYaw', samples[-1].z)
    profile.set_editor_property('MotionSamples', samples)
    return profile
