"""Validate bounded-memory soak records; do not equate wall-clock time with simulated time."""
import argparse
import hashlib
import json
import math
from pathlib import Path


def validate(result, intervals, log):
    requested = result['requested_seconds']
    if not result['final'] or result['outcome'] != 'smoke_soak_completed':
        raise ValueError('No successful final result')
    if not 180 <= requested <= 7200 or not requested <= result['total']['wall_seconds'] <= requested + 600:
        raise ValueError('Insufficient or excessive wall time')
    if not requested <= result['measured_simulation_seconds'] <= requested + 30:
        raise ValueError('Insufficient or excessive simulation time')
    if (result['width'], result['height']) != (1920, 1080):
        raise ValueError('Unexpected viewport resolution')
    for key, expected in [('r.ScreenPercentage', 100), ('r.SecondaryScreenPercentage.GameViewport', 100),
                          ('r.DynamicRes.OperationMode', 0), ('r.VSync', 0), ('t.MaxFPS', 0)]:
        if result[key] != expected: raise ValueError('Unexpected render setting: ' + key)
    for name in ['ViewDistance', 'AntiAliasing', 'Shadow', 'GlobalIllumination', 'Reflection', 'PostProcess', 'Texture', 'Effects', 'Foliage']:
        if result[f'sg.{name}Quality'] != result['quality']: raise ValueError('Quality mismatch')
    if not intervals or len(intervals) < requested // 30 - 1: raise ValueError('Missing interval records')
    last_wall = last_frames = 0
    frames = seconds = 0
    hitch_totals = {key: 0 for key in ['over_16_67_ms', 'over_33_33_ms', 'over_100_ms']}
    maximum = 0
    for row in intervals + [result]:
        total, window = row['total'], row['interval']
        for metrics in [total, window]:
            if not all(isinstance(v, (float, int)) and math.isfinite(v) and v >= 0 for v in metrics.values()):
                raise ValueError('Invalid metrics')
            if metrics['wall_seconds'] and abs(metrics['average_fps'] - metrics['frames'] / metrics['wall_seconds']) > .001:
                raise ValueError('FPS inconsistent with measured time')
            if not metrics['frames'] >= metrics['over_16_67_ms'] >= metrics['over_33_33_ms'] >= metrics['over_100_ms']:
                raise ValueError('Invalid hitch counts')
            if not metrics['p95_upper_ms'] <= metrics['p99_upper_ms'] <= metrics['maximum_ms'] + .251:
                raise ValueError('Invalid histogram bounds')
        if total['wall_seconds'] < last_wall or total['frames'] < last_frames:
            raise ValueError('Non-monotonic records')
        frames += window['frames']; seconds += window['wall_seconds']
        maximum = max(maximum, window['maximum_ms'])
        for key in hitch_totals:
            hitch_totals[key] += window[key]
            if hitch_totals[key] != total[key]: raise ValueError('Intervals omit hitch counts')
        if abs(maximum - total['maximum_ms']) > .001: raise ValueError('Intervals omit longest frame')
        if frames != total['frames'] or abs(seconds - total['wall_seconds']) > .001:
            raise ValueError('Intervals omit frames or measured time')
        if not 1 <= row['terrain_chunks'] <= 9 or row['failed_autosaves'] or row['pending_autosaves'] > 1:
            raise ValueError('Terrain or writer invariants failed')
        last_wall, last_frames = total['wall_seconds'], total['frames']
    if result['turns'] < max(1, requested // 500) or result['visited_chunk_centers'] < 3 or result['distance_m'] < requested * .3:
        raise ValueError('Insufficient course traversal or streaming')
    if result['successful_autosaves'] < requested // 30:
        raise ValueError('Insufficient completed autosaves')
    if requested >= 300 and (not result['surface_reached'] or result['minimum_depth_m'] >= .2 or result['maximum_depth_m'] <= 5):
        raise ValueError('Underwater/surface cycle incomplete')
    if not log.get('is_test') or log['status'] != result['outcome'] or len(log['samples']) != result['samples']:
        raise ValueError('Final saved log differs from runtime')
    if abs(log['duration_seconds'] - result['simulation_seconds']) > .001 or len(log['samples']) < int(result['simulation_seconds']):
        raise ValueError('Final recording incomplete')
    times = [sample['seconds'] for sample in log['samples']]
    if not times or any(not math.isfinite(t) for t in times) or any(b < a for a, b in zip(times, times[1:])):
        raise ValueError('Invalid recording times')
    if abs(times[-1] - log['duration_seconds']) > .001: raise ValueError('Missing final sample')


def verify_package(host):
    """Bind a long run to the same binary and cooked content at its end."""
    def digest(path):
        with path.open('rb') as stream:
            return hashlib.file_digest(stream, 'sha256').hexdigest()
    executable = Path(host['executable'])
    if digest(executable) != host['executable_sha256'].lower():
        raise ValueError('Executable changed during soak')
    if 'package_containers' not in host:
        return 'legacy_executable_only'
    root = executable.parents[2] / 'Content/Paks'
    actual = [dict(file=p.name, bytes=p.stat().st_size, sha256=digest(p))
              for p in sorted(root.iterdir()) if p.is_file()]
    expected = sorted(host['package_containers'], key=lambda p: p['file'])
    if not expected or actual != expected:
        raise ValueError('Cooked package changed during soak')
    return 'executable_and_cooked_containers'


def main():
    parser = argparse.ArgumentParser(); parser.add_argument('directory', type=Path)
    folder = parser.parse_args().directory
    result = json.loads((folder / 'Result.json').read_text(encoding='utf-8-sig'))
    intervals = [json.loads(line) for line in (folder / 'Intervals.jsonl').read_text(encoding='utf-8-sig').splitlines()]
    log = json.loads((folder / 'DiveLog.json').read_text(encoding='utf-8-sig'))
    validate(result, intervals, log)
    for name in ['Start.png', 'Final.png']:
        data = (folder / name).read_bytes()
        if data[:8] != b'\x89PNG\r\n\x1a\n' or tuple(int.from_bytes(data[i:i+4], 'big') for i in [16, 20]) != (1920, 1080):
            raise ValueError('Missing actual-resolution screenshot')
    evidence = {name: hashlib.sha256((folder / name).read_bytes()).hexdigest()
                for name in ['Result.json', 'Intervals.jsonl', 'DiveLog.json', 'Runtime.log', 'Start.png', 'Final.png']}
    host = json.loads((folder / 'Host.json').read_text(encoding='utf-8-sig'))
    package_integrity = verify_package(host)
    summary = {'result': result, 'interval_count': len(intervals), 'host': host, 'package_integrity': package_integrity,
               'sha256': evidence, 'two_real_hours': result['total']['wall_seconds'] >= 7200 and result['measured_simulation_seconds'] >= 7200}
    (folder / 'Summary.json').write_text(json.dumps(summary, indent=2) + '\n', encoding='utf-8')
    print(f"SOAK_VALIDATED wall={result['total']['wall_seconds']:.2f}s sim={result['measured_simulation_seconds']:.2f}s "
          f"fps={result['total']['average_fps']:.2f} saves={result['successful_autosaves']} samples={result['samples']}")


if __name__ == '__main__': main()
