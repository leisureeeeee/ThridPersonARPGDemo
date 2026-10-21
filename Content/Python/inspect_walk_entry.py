"""Editor-only, read-only leg-pose comparison for start-to-loop entry.

Uses local rotations so authored body yaw does not bias the leg comparison.
This is an authoring aid, not foot-contact detection or runtime pose search.
"""
import math
import unreal


_LEG_BONES = ('thigh_l', 'calf_l', 'foot_l', 'ball_l',
              'thigh_r', 'calf_r', 'foot_r', 'ball_r')


def _rotations(path, time):
    pose = unreal.AnimSequenceService.get_pose_at_time(path, time, False)
    result = {str(bone.bone_name): bone.transform.rotation for bone in pose
              if str(bone.bone_name) in _LEG_BONES}
    if len(result) != len(_LEG_BONES):
        raise ValueError('Animation is missing required UE4 leg bones: ' + path)
    return result


def _error(a, b):
    squared = 0.0
    for bone in _LEG_BONES:
        q, r = a[bone], b[bone]
        norm = math.sqrt((q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w)
                         * (r.x*r.x + r.y*r.y + r.z*r.z + r.w*r.w))
        if norm <= 1e-8:
            raise ValueError('Invalid quaternion for ' + bone)
        dot = abs(q.x*r.x + q.y*r.y + q.z*r.z + q.w*r.w) / norm
        angle = 2.0 * math.acos(min(1.0, max(0.0, dot)))
        squared += angle * angle
    return math.degrees(math.sqrt(squared / len(_LEG_BONES)))


def inspect_walk_entry(walk_path, start_paths, sample_count=128):
    """Return best loop entry by RMS local leg angle, without editing assets."""
    if sample_count < 2 or not start_paths:
        raise ValueError('At least two samples and one start are required.')
    walk = unreal.EditorAssetLibrary.load_asset(walk_path)
    if not isinstance(walk, unreal.AnimSequence) or walk.get_play_length() <= 0:
        raise ValueError('Expected a positive-length walk animation.')
    length = walk.get_play_length()
    samples = [(length * i / sample_count,
                _rotations(walk_path, length * i / sample_count))
               for i in range(sample_count)]
    result = []
    for path in start_paths:
        start = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(start, unreal.AnimSequence) or start.get_play_length() <= 0:
            raise ValueError('Expected a positive-length start animation: ' + path)
        if start.get_skeleton() != walk.get_skeleton():
            raise ValueError('Start and walk must share a skeleton.')
        end = _rotations(path, start.get_play_length())
        error, time = min((_error(end, pose), time) for time, pose in samples)
        result.append({'start': path, 'best_loop_time': time,
                       'best_leg_rms_degrees': error,
                       'zero_leg_rms_degrees': _error(end, samples[0][1])})
    return {'walk': walk_path, 'loop_length': length,
            'sample_count': sample_count, 'starts': result}
