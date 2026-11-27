"""Report the limited native reflection transport experiment; not a release gate."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def bind(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def load_run(folder, names):
    host = folder / 'Host.json'
    cases = folder / 'Cases.json'
    items = json.loads(cases.read_text(encoding='utf-8-sig'))
    if [c['name'] for c in items] != names or any(c['status'] != 'PASS' for c in items):
        raise ValueError('Incomplete native study')
    evidence = [bind(host), bind(cases), bind(folder / 'ShaderManifest.json')]
    for case in items:
        log = folder / case['name'] / 'Runtime.log'
        text = log.read_text(encoding='utf-8-sig', errors='replace')
        mode = int(not case['options']['ReflectionTransportControl'])
        if f'DIVE_REFLECTION_TRANSPORT_READY enabled={mode}' not in text or 'DIVE_SMOKE_PASS' not in text:
            raise ValueError('Missing transport mode or runtime completion')
        if len(case['images']) != 3:
            raise ValueError('Unexpected image count')
        evidence.extend([bind(log), bind(folder / case['name'] / 'Summary.log')])
        evidence.extend(bind(folder / case['name'] / image) for image in case['images'])
    return dict(input_manifest=json.loads(host.read_text(encoding='utf-8-sig')),
                shader_manifest=json.loads((folder / 'ShaderManifest.json').read_text(encoding='utf-8-sig')),
                cases=items, evidence=evidence)


def frame(folder, case, view):
    image = Image.open(folder / case / f'Water_0_{view}.png').convert('RGB')
    if image.size != (1600, 900):
        raise ValueError('Unexpected capture dimensions')
    return np.asarray(image, dtype=float)


def region(image):
    c = image[220:360, 400:1200]
    return dict(mean_display_rgb=c.mean((0, 1)).tolist(),
                mean_luma=float((c @ np.array([.2126, .7152, .0722])).mean()),
                blue_dominant_fraction=float(np.mean((c[:, :, 2] - c[:, :, 1]) > 10)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--waterline', type=Path, required=True)
    parser.add_argument('--response', type=Path, required=True)
    args = parser.parse_args()
    waterline, response = (ROOT / args.waterline).resolve(), (ROOT / args.response).resolve()
    report = dict(status='EXPERIMENT_REVIEWED_NOT_RELEASED', production_fix=False,
        waterline=load_run(waterline, ['baseline', 'transport-no-screens', 'transport-hybrid']),
        response=load_run(response, ['baseline-no-screens', 'clear', 'turbid', 'near-baseline', 'near-transport',
                                    'night-off', 'night-lamp-baseline', 'night-lamp-transport']),
        underwater_region_xyxy=[400, 220, 1200, 360], waterline_metrics=[], response_metrics=[])
    for view in (0, 1):
        before = region(frame(waterline, 'baseline', view))
        after = region(frame(waterline, 'transport-hybrid', view))
        if before['blue_dominant_fraction'] < .95 or after['blue_dominant_fraction'] > .01 or after['mean_luma'] < 4:
            raise ValueError('The targeted blue-area correction is not reproduced')
        report['waterline_metrics'].append(dict(view=view, before=before, after=after))
    air_difference = float(np.abs(frame(waterline, 'baseline', 2)-frame(waterline, 'transport-hybrid', 2)).mean())
    if air_difference > 1:
        raise ValueError('Above-water control differs by more than one display RGB step')
    report['above_water_mean_absolute_rgb_difference'] = air_difference
    for case in ['baseline-no-screens', 'clear', 'turbid', 'night-off', 'night-lamp-baseline', 'night-lamp-transport']:
        for view in (0, 1):
            report['response_metrics'].append(dict(case=case, view=view, region=region(frame(response, case, view))))
    night_off = region(frame(response, 'night-off', 0))['mean_luma']
    lamp_before = region(frame(response, 'night-lamp-baseline', 0))['mean_luma']
    lamp_after = region(frame(response, 'night-lamp-transport', 0))['mean_luma']
    if night_off > 1 or lamp_after - lamp_before < 2:
        raise ValueError('The reflected lamp scattering control is not reproduced')
    report['lamp_reflection_luma_increase'] = lamp_after - lamp_before
    report['near_view_mean_absolute_rgb_difference'] = [float(np.abs(frame(response, 'near-baseline', view)-frame(response, 'near-transport', view)).mean()) for view in range(3)]
    # Response measurements remain descriptive: increasing extinction does not
    # necessarily darken an optically infinite, illuminated homogeneous medium.
    report['response_metric_scope'] = 'Fixed display region, no blanket monotonic-brightness assertion; inspect the native images and lighting controls.'
    report['quadrature'] = json.loads((ROOT / 'artifacts/reflection-transport-study/quadrature.json').read_text())
    report['quadrature_evidence'] = bind(ROOT / 'artifacts/reflection-transport-study/quadrature.json')
    report['build_logs'] = [bind(ROOT / 'artifacts/reflection-transport-study' / name) for name in
                            ['editor-build.log', 'control-build.log', 'stencil-build.log', 'response-build.log']]
    out = ROOT / 'Documentation/REFLECTION_TRANSPORT_RESULTS.json'
    out.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for case, suffix in [('baseline', 'Before'), ('transport-hybrid', 'After')]:
        shutil.copyfile(waterline / case / 'Water_0_0.png', ROOT / f'Documentation/Images/ReflectionTransport_{suffix}.png')
    print(f'REFLECTION_TRANSPORT_REPORT={out}')


if __name__ == '__main__':
    main()
