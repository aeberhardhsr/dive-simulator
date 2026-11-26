"""Record native water-lighting experiments without claiming visual acceptance."""
import argparse
import hashlib
import json
from pathlib import Path
import re

import numpy as np
from PIL import Image
from validate_lighting_frames import scene
from ReportCaustics import report as caustics_report

ROOT = Path(__file__).resolve().parent.parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path), bytes=path.stat().st_size)


def pixels(path):
    with Image.open(path) as im:
        assert im.size == (1600, 900)
        return np.asarray(im.convert('RGB'), dtype=np.float64)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/NATIVE_WATER_LIGHTING_RESULTS.json')
    args = parser.parse_args()
    run = args.run.resolve()
    host = json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    assert host['native_editor'] and not host['packaged']
    for item in host['files']:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    cases = json.loads((run/'Cases.json').read_text(encoding='utf-8-sig'))
    expected = {'sun-legacy', 'sun-native', 'bed-legacy', 'bed-native', 'surface-legacy', 'surface-native',
                'dark-native-on', 'dark-native-off', 'fallback'}
    assert len(cases) == 9 and {c['name'] for c in cases} == expected
    evidence = []
    poses = {}
    for case in cases:
        name = case['name']
        folder = run/name
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert 'DIVE_SMOKE_PASS' in log and 'DIVE_MEDIUM_PASS' in log and 'DIVE_OPTICS_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
        active = int(case['native'] and not case['fallback'])
        assert f'native={active} requested={int(case["native"])}' in log
        assert 't.MaxFPS = "30"' in log
        if case['fallback']:
            assert 'r.VolumetricFog = "0"' in log
        if name.startswith('dark-'):
            elevation = float(re.search(r'DIVE_LIGHT_PASS elevation=([-\d.]+)', log)[1])
            assert elevation < -18
        images = sorted(folder.glob('*.png'))
        assert len(images) == case['screenshots'] == (2 if name.startswith('dark-') else 3)
        for path in images:
            pixels(path)
        poses[name] = re.findall(r'DIVE_(?:HABITAT_VIEW|WATER_VIEW) ([^\r\n]+)', log)
        evidence.append(dict(**case, runtime_log=record(folder/'Runtime.log'), poses=poses[name], images=[record(p) for p in images],
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    comparisons = []
    for group, prefix in [('sun', 'Habitat_0'), ('bed', 'Habitat_0'), ('surface', 'Water_1')]:
        assert len(poses[f'{group}-legacy']) == 3
        assert poses[f'{group}-legacy'] == poses[f'{group}-native'], group
        for i in range(3):
            old = pixels(run/f'{group}-legacy'/f'{prefix}_{i}.png')
            new = pixels(run/f'{group}-native'/f'{prefix}_{i}.png')
            comparisons.append(dict(group=group, view=i, mean_absolute_rgb_difference=float(np.abs(old-new).mean()),
                legacy_mean_rgb=old.mean(axis=(0, 1)).tolist(), native_mean_rgb=new.mean(axis=(0, 1)).tolist()))
    # Reuse the existing night mask and thresholds, excluding HUD and reticle.
    off = scene(run/'dark-native-off'/'DiveUnderwater_0_torch.png')
    on = scene(run/'dark-native-on'/'DiveUnderwater_0_torch.png')
    night = dict(mask_source='Tools/validate_lighting_frames.py:scene', off_p999=float(np.percentile(off, 99.9)),
                 off_mean=float(off.mean()), on_mean=float(on.mean()),
                 lit_pixels=int(((on>20)&(off<3)).sum()), mean_difference=float((on-off).mean()))
    assert night['off_p999'] <= 2 and night['lit_pixels'] > 5000 and night['mean_difference'] > 1, night
    build = ROOT/'artifacts/native-scattering-fixture-editor-build.log'
    assert 'Result: Succeeded' in build.read_text(encoding='utf-8-sig')
    caustics_path = (run/'CausticsRun.txt').read_text(encoding='utf-8-sig').strip()
    assert caustics_path.startswith('DIVE_CAUSTICS_VALIDATION=')
    caustics = caustics_report(Path(caustics_path.split('=', 1)[1]))
    assert not caustics['host']['packaged']
    published = []
    for name, filename in [('sun-legacy', 'NativeWaterLighting_Standard.png'), ('sun-native', 'NativeWaterLighting_Prototype.png')]:
        path = ROOT/'Documentation/Images'/filename
        assert sha(path) == sha(run/name/'Habitat_0_0.png')
        published.append(record(path))
    report = dict(status='PASS', scope='Experimental native rendering, camera matches and optical regressions; not visual acceptance',
        host=host, host_file=record(run/'Host.json'), build=record(build), cases=evidence,
        screenshot_count=sum(c['screenshots'] for c in cases)+4, comparisons=comparisons, night=night,
        default_caustics_regression=caustics,
        published=published,
        caustics_probe_material=record(ROOT/'divingsim/Content/Diving/Materials/M_CausticsProbe.uasset'),
        production_enabled=False,
        limitations=['Native editor only; no packaged build, performance or long-term stability result',
                    'The direct-light caustic field uses one representative depth slice, not full 3D refracted transport',
                    'A reduced analytic ambient contribution remains unshadowed',
                    'Water underside, distant water illumination and reference-level appearance remain unresolved'])
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(f'PASS: 9 scene cases + default caustics / {report["screenshot_count"]} native captures -> {args.output}')


if __name__ == '__main__':
    main()
