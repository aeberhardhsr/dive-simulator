"""Check packaged contact/head-turn regressions and record rendered evidence."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from PIL import Image


def evidence(path):
    return {'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def report(folder):
    host = json.loads((folder / 'Host.json').read_text(encoding='utf-8-sig'))
    cases = json.loads((folder / 'Cases.json').read_text(encoding='utf-8-sig'))
    expected = {'sediment0', 'sediment1', 'sediment2', 'shore0', 'shore1', 'shore2',
                'avatar0', 'avatar5', 'breathing', 'wrist', 'stream1'}
    if len(cases) != len(expected) or {c['name'] for c in cases} != expected:
        raise ValueError('Incomplete head-turn regression matrix')
    head_turns = 0
    for case in cases:
        directory = folder / case['name']
        log = directory / 'Runtime.log'
        text = log.read_text(encoding='utf-8-sig')
        if case['status'] != 'PASS' or 'DIVE_SMOKE_PASS' not in text or re.search(
                r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used', text):
            raise ValueError('Failed scenario: ' + case['name'])
        images = sorted(directory.glob('*.png'))
        if len(images) != case['screenshots'] or len(images) < 2:
            raise ValueError('Missing rendered states')
        for path in images:
            with Image.open(path) as im:
                display_export = case['name'] == 'wrist' and path.name == 'WristTexture_wrist.png'
                if im.size != ((768, 512) if display_export else (1600, 900)):
                    raise ValueError('Wrong viewport or display texture size: ' + str(path))
        case['display_texture_exports'] = sum(path.name == 'WristTexture_wrist.png' for path in images)
        case['viewport_captures'] = len(images) - case['display_texture_exports']
        case['log'] = evidence(log)
        case['images'] = [evidence(path) for path in images]
        events = re.findall(r'DIVE_CONTACT_HEAD yaw=([-\d.]+) body=([-\d.]+) gaze=([-\d.]+) requested=([-\d.]+)', text)
        case['head_events'] = [dict(zip(('head_yaw', 'body_yaw', 'gaze_yaw', 'requested_yaw'), map(float, event))) for event in events]
        for event in case['head_events']:
            if abs(event['head_yaw']) > 65.001:
                raise ValueError('Accepted head escaped authored yaw limit')
            head_turns += abs(event['head_yaw']) > 10
        if case['scenario'] == 'sediment':
            if 'DIVE_SEDIMENT_PASS' not in text or 'DIVE_SEDIMENT_PAUSE_PASS' not in text:
                raise ValueError('Sediment lifecycle/pause failed')
            case['sediment_view'] = re.findall(r'DIVE_SEDIMENT_VIEW (.+)', text)
        if case['scenario'] == 'stream':
            if 'DIVE_POPULATION_PASS' not in text:
                raise ValueError('Population identity fixture failed')
            detail = re.search(r'DIVE_SMOKE_DETAIL entry=1 scenario=stream displacement=([\d.]+)', text)
            if not detail or float(detail[1]) < 80:
                raise ValueError('Stream route did not travel 80 m')
            case['displacement_metres'] = float(detail[1])
    if not head_turns:
        raise ValueError('No accepted independent head turns in packaged gameplay')
    return {'status': 'PASS', 'host': host, 'cases': cases, 'accepted_sideways_head_events': head_turns,
            'limits': ['65 degrees is an animation limit, not an anatomical validation.',
                       'A contact-checked head fallback is implemented; unrestricted free look and full-body turns still require clearance.',
                       'These capped Windows runs are not a performance or native macOS acceptance test.',
                       'Sediment projection alone does not prove visible optical contrast.']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = report(args.directory.resolve())
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('PASS:', len(result['cases']), 'cases;', sum(c['viewport_captures'] for c in result['cases']), 'viewport captures;', result['accepted_sideways_head_events'], 'head turns')
