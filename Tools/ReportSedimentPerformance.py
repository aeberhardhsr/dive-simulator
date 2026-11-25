"""Attach live sediment counts and artifact hashes to a complete UE capture."""
import argparse
import csv
import hashlib
import json
import statistics
from pathlib import Path

from analyze_performance import analyze


def report(folder, executable, allow_background=False):
    data = analyze(folder)
    with (folder / 'Capture.csv').open(encoding='utf-8-sig', newline='') as stream:
        rows = list(csv.reader(stream))
    index = rows[-2].index('DivePerf/SedimentLive')
    values = [float(row[index]) if index < len(row) else 0 for row in rows[2:-3]]
    if len(values) != data['frames'] or any(not 0 <= v <= 24 or v != int(v) for v in values):
        raise ValueError('Invalid live sediment counts')
    data['sediment'] = {'min_live': min(values), 'max_live': max(values),
                        'mean_live': statistics.mean(values),
                        'frames_with_clouds': sum(v > 0 for v in values)}
    if data['metadata']['scenario'] == 'perf-sediment' and not data['sediment']['frames_with_clouds']:
        raise ValueError('Sediment benchmark contained no active clouds')
    data['uncontended_unreal_capture'] = not bool(data['host']['background_processes'])
    data['limits'] = [
        'Live cloud counts describe simulated state, not guaranteed on-screen coverage or worst-case translucent overdraw.',
        'Different benchmark routes do not isolate the incremental cost of sediment.',
        'This short capture does not establish minimum-hardware, native macOS or long-session performance.']
    if not data['uncontended_unreal_capture'] and not allow_background:
        raise ValueError('Another Unreal instance was present at capture start')
    data['executable_sha256'] = hashlib.sha256(executable.read_bytes()).hexdigest()
    data['evidence'] = {name: {'path': str(folder / name),
        'sha256': hashlib.sha256((folder / name).read_bytes()).hexdigest()}
        for name in ['Capture.csv', 'Metadata.json', 'Frame.png', 'Runtime.log', 'Host.json']}
    return data


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--allow-background', action='store_true',
                        help='Report a non-isolated capture with its recorded background processes.')
    args = parser.parse_args()
    result = report(args.directory.resolve(), args.executable.resolve(), args.allow_background)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(result['metadata']['scenario'], result['average_fps'], result['one_percent_low_fps'], result['sediment'])
