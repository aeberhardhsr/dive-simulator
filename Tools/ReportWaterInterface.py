"""Bind water-interface captures to their source and record regression checks."""
import argparse
import json
import re
from pathlib import Path

import numpy as np
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from validate_lighting_frames import scene


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/WATER_INTERFACE_RESULTS.json')
    args = parser.parse_args()
    run = args.run.resolve()
    host = json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    assert host['native_editor'] and not host['packaged']
    for item in host['files']:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    cases = json.loads((run/'Cases.json').read_text(encoding='utf-8-sig'))
    expected = {'surface-legacy', 'surface-software', 'surface-hardware', 'surface-air-fog-control',
                'bed-legacy', 'bed-interface', 'storm-interface', 'dark-interface-on', 'dark-interface-off'}
    assert len(cases) == 9 and {c['name'] for c in cases} == expected
    evidence, poses, actual = [], {}, {}
    for case in cases:
        name = case['name']
        folder = run/name
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert 'DIVE_SMOKE_PASS' in log and 'DIVE_MEDIUM_PASS' in log and 'DIVE_OPTICS_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
        assert ('DIVE_WATER_INTERFACE_PASS layers=2' in log) == case['interface']
        assert 't.MaxFPS = "30"' in log
        if case['hardware']:
            assert 'r.Lumen.HardwareRayTracing = "1"' in log
        if name.startswith('dark-'):
            assert float(re.search(r'DIVE_LIGHT_PASS elevation=([-\d.]+)', log)[1]) < -18
        if name == 'storm-interface':
            assert 'DIVE_WEATHER_PASS kind=3 wind=22.0' in log
            for phase in ['air', 'transition', 'water']:
                assert f'DIVE_MEDIUM_PASS phase={phase}' in log
        images = sorted(folder.glob('*.png'))
        assert len(images) == case['screenshots'] == (2 if name.startswith('dark-') else 3)
        for path in images:
            pixels(path)
        poses[name] = re.findall(r'DIVE_(?:HABITAT_VIEW|WATER_VIEW) ([^\r\n]+)', log)
        actual[name] = re.findall(r'DIVE_WATER_CAMERA ([^\r\n]+)', log)
        evidence.append(dict(**case, runtime_log=record(folder/'Runtime.log'), poses=poses[name], actual_water_cameras=actual[name], images=[record(p) for p in images],
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    comparisons = []
    for first, second, prefix in [('surface-legacy', 'surface-software', 'Water_1'),
                                  ('surface-software', 'surface-hardware', 'Water_1'),
                                  ('bed-legacy', 'bed-interface', 'Habitat_0')]:
        assert len(poses[first]) == 3 and poses[first] == poses[second], (first, second)
        if prefix == 'Water_1':
            assert len(actual[first]) == 3 and actual[first] == actual[second], (first, second)
        for i in range(3):
            old = pixels(run/first/f'{prefix}_{i}.png')
            new = pixels(run/second/f'{prefix}_{i}.png')
            comparisons.append(dict(first=first, second=second, view=i,
                mean_absolute_rgb_difference=float(np.abs(old-new).mean())))
    off = scene(run/'dark-interface-off'/'DiveUnderwater_0_torch.png')
    on = scene(run/'dark-interface-on'/'DiveUnderwater_0_torch.png')
    night = dict(mask_source='Tools/validate_lighting_frames.py:scene', off_p999=float(np.percentile(off, 99.9)),
                 off_mean=float(off.mean()), lit_pixels=int(((on>20)&(off<3)).sum()), mean_difference=float((on-off).mean()))
    assert night['off_p999'] <= 2 and night['lit_pixels'] > 5000 and night['mean_difference'] > 1, night
    build = ROOT/'artifacts/water-interface-validation-build.log'
    assert 'Result: Succeeded' in build.read_text(encoding='utf-8-sig')
    published = []
    for name, image, filename in [('surface-legacy', 'Water_1_0.png', 'WaterInterface_Standard.png'),
                                  ('surface-software', 'Water_1_0.png', 'WaterInterface_Prototype.png'),
                                  ('surface-software', 'Water_1_1.png', 'WaterInterface_Window.png')]:
        path = ROOT/'Documentation/Images'/filename
        assert sha(path) == sha(run/name/image)
        published.append(record(path))
    result = dict(status='PASS', scope='Native render/content integrity, matched cameras, storm medium transitions and night lighting; not visual acceptance',
                  host=host, host_file=record(run/'Host.json'), build=record(build), cases=evidence,
                  screenshot_count=sum(c['screenshots'] for c in cases), comparisons=comparisons, night=night,
                  published=published, production_enabled=False,
                  limitations=['Native editor only; no packaged, performance or long-term stability result',
                               'Pixel-normal offset is a screen-space approximation, not full Snell refraction',
                               'Existing global underwater fog still affects the transmitted sky',
                               'Reflection detail, critical-angle boundary and appearance require further visual work',
                               'A successful runtime test does not demonstrate reference-level graphics'])
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(f'PASS: 9 water-interface cases / {result["screenshot_count"]} native captures -> {args.output}')


if __name__ == '__main__':
    main()
