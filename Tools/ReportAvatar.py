"""Collect native avatar evidence without treating screenshots as visual acceptance."""
import argparse
import hashlib
import json
import re
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def report(directory):
    root = Path(directory).resolve()
    host = json.loads((root/'Host.json').read_text(encoding='utf-8-sig'))
    cases = json.loads((root/'Cases.json').read_text(encoding='utf-8-sig'))
    if len(cases) != 6 or {c['name'] for c in cases} != set(range(6)):
        raise ValueError('All six avatar cases must complete')
    results = []
    stages = ['Front', 'Back', 'Swimming', 'Detail', 'FirstPerson', 'Computer', 'Paused', 'LookDown']
    for case in cases:
        number = case['name']
        folder = root/str(number)
        runtime = folder/'Runtime.log'
        log = runtime.read_text(encoding='utf-8-sig', errors='strict')
        if case['status'] != 'PASS' or f'DIVE_SMOKE_PASS avatar=1 case={number}' not in log:
            raise ValueError(f'Failed avatar case {number}')
        body_pose = 'DIVE_BODY_POSE_VERSION 1' in log
        for phase in range(25 if body_pose else 17):
            if f'DIVE_AVATAR_PHASE {phase} PASS' not in log:
                raise ValueError(f'Missing phase {phase} in case {number}')
        if re.search(r'DIVE_\w+_FAIL|DIVE_AVATAR_PHASE \d+ FAIL|Default Material will be used|Failed to compile Material', log):
            raise ValueError(f'Runtime validation error in case {number}')
        images = []
        for stage in stages + (['LookUp', 'RegulatorOut', 'RegulatorIn'] if body_pose else []):
            path = folder/f'Avatar_{number}_{stage}.png'
            if path.stat().st_size < 10000 or path.read_bytes()[:8] != b'\x89PNG\r\n\x1a\n':
                raise ValueError(f'Missing screenshot {path}')
            images.append({'file': str(path), 'sha256': digest(path)})
        results.append({**case, 'runtime_log': str(runtime), 'runtime_log_sha256': digest(runtime),
                        'cylinder_litres': [12, 15, 10][number % 3],
                        'cylinder_material': 'Steel' if number < 3 else 'Aluminium',
                        'bcd': ['Jacket', 'Travel', 'Wing'][number % 3],
                        'suit': 'Dry' if number % 2 == 0 else 'Wet',
                        'shore_start': number == 5, 'screenshots': images})
    return {'host': host, 'status': 'PASS', 'cases': results,
            'limits': ['Screenshots are evidence for manual review, not automatic photorealistic acceptance.',
                       'This six-case matrix covers every listed variant, not every possible combination.']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = report(args.directory)
    content = json.dumps(result, indent=2)+'\n'
    if args.output:
        args.output.write_text(content, encoding='utf-8')
    print(f"PASS: {len(result['cases'])} avatar cases, {sum(len(c['screenshots']) for c in result['cases'])} screenshots")
