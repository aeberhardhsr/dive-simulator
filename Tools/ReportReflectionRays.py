"""Bind native ray-classification captures to their diagnostic evidence.

This report proves the origin/direction of a selected reflection region. It is
not an optical-quality acceptance test and does not establish a transport fix.
"""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ROI = (400, 220, 1200, 360)  # Frozen Weesen waterline view, 1600 x 900.


def binding(path):
    return dict(file=path.relative_to(ROOT).as_posix(),
                sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def inspect_run(folder, mode):
    host_path, cases_path = folder / 'Host.json', folder / 'Cases.json'
    host = json.loads(host_path.read_text(encoding='utf-8-sig'))
    cases = json.loads(cases_path.read_text(encoding='utf-8-sig'))
    expected = ['normal', f'ray-{mode}', f'ray-{mode}-no-screens']
    if host['study'] != 'reflection' or not host['native_editor']:
        raise ValueError('Expected an editor reflection study')
    if host['pose'] != [329185.873, 172781.736, 180.0, 0.0]:
        raise ValueError('The measurement region is only valid for the frozen Weesen pose')
    if [c['name'] for c in cases] != expected:
        raise ValueError('Missing or unexpected cases')
    result = dict(mode=mode, host=binding(host_path), cases=binding(cases_path),
                  input_manifest=host, measurements=[], evidence=[])
    for case in cases:
        if case['status'] != 'PASS' or len(case['images']) != 3:
            raise ValueError('Incomplete case')
        log_path = folder / case['name'] / 'Runtime.log'
        log = log_path.read_text(encoding='utf-8-sig', errors='replace')
        if 'DIVE_SMOKE_PASS' not in log or ('DIVE_REFLECTION_RAY_STUDY_READY' in log) != (case['name'] != 'normal'):
            raise ValueError('Diagnostic activation does not match the case')
        result['evidence'].extend([binding(log_path), binding(folder / case['name'] / 'Summary.log')])
        for view in range(3):
            path = folder / case['name'] / f'Water_0_{view}.png'
            result['evidence'].append(binding(path))
            image = Image.open(path).convert('RGB')
            if image.size != (1600, 900):
                raise ValueError('Unexpected capture dimensions')
            if view == 2:
                continue  # Above-water image is reviewed, not subject to the underwater ROI.
            c = np.asarray(image.crop(ROI), dtype=float).reshape(-1, 3)
            r, g, b = c.T
            fractions = dict(magenta=float(np.mean((r > 1.4*g) & (b > 1.4*g) & (r > 20))),
                             red=float(np.mean((r > 1.4*g) & (r > 1.4*b) & (r > 20))),
                             cyan=float(np.mean((g > 1.4*r) & (b > 1.4*r) & (g > 20))))
            if case['name'] != 'normal':
                required = 'magenta' if mode == 'classification' else 'red'
                if fractions[required] < .99:
                    raise ValueError(f'{case["name"]}/{view}: diagnostic finding is not reproduced')
            result['measurements'].append(dict(case=case['name'], view=view,
                mean_display_rgb=c.mean(axis=0).tolist(), colour_fractions=fractions))
    archived = folder / 'ShaderManifest.json'
    if archived.exists():
        result['shader_manifest'] = json.loads(archived.read_text(encoding='utf-8-sig'))
        result['evidence'].append(binding(archived))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--classification', type=Path, required=True)
    parser.add_argument('--direction', type=Path, required=True)
    args = parser.parse_args()
    folders = {mode: (ROOT / getattr(args, mode)).resolve() for mode in ('classification', 'direction')}
    report = dict(status='DIAGNOSIS_REPRODUCED', production_fix=False,
                  region_xyxy=list(ROI), display_scale='8-bit RGB after presentation; not radiometric',
                  limitation='One frozen Weesen pose at -20 cm and -2 cm. Green can include geometry, distant screen trace or cache. Recursive secondary misses are not classified separately.',
                  runs=[inspect_run(folders[mode], mode) for mode in folders])
    out = ROOT / 'Documentation/WATER_REFLECTION_DIAGNOSIS_RESULTS.json'
    out.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for mode, folder in folders.items():
        source = folder / f'ray-{mode}-no-screens/Water_0_0.png'
        shutil.copyfile(source, ROOT / f'Documentation/Images/WaterReflection_{mode}.png')
    shutil.copyfile(folders['classification'] / 'normal/Water_0_0.png',
                    ROOT / 'Documentation/Images/WaterReflection_Normal.png')
    print(f'REFLECTION_RAY_DIAGNOSIS={out}')


if __name__ == '__main__':
    main()
