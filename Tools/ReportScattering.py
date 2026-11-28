"""Bind the native scattering ablation captures; this is not a visual acceptance test."""
import argparse
import hashlib
import json
from pathlib import Path
import re

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path), bytes=path.stat().st_size)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--build-log', type=Path, default=ROOT/'artifacts/scatter-diagnostics-editor-build.log')
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/SCATTERING_DIAGNOSIS.json')
    args = parser.parse_args()
    run = args.run.resolve()
    host = json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    build = args.build_log.resolve()
    assert 'Result: Succeeded' in build.read_text(encoding='utf-8-sig')
    assert host['native_editor'] and not host['packaged']
    assert sha(Path(host['module']['path'])) == host['module']['sha256']
    for source in host['sources']:
        assert sha(ROOT/'divingsim'/source['path']) == source['sha256'], source['path']
        assert sha(ROOT/'Builds/EditorSandbox/divingsim'/source['path']) == source['sha256']
    results = []
    reference_poses = None
    reference_light = None
    reference_pixels = []
    cases = [('baseline', 1, 1), ('no-fog-addition', 0, 1), ('no-view-addition', 1, 0), ('no-additions', 0, 0)]
    for name, fog, view in cases:
        folder = run/name
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert 'DIVE_SMOKE_PASS rocks=1' in log and 'DIVE_HABITAT_INSPECTION_PASS views=3' in log
        assert 'DIVE_MEDIUM_PASS phase=water' in log and 'DIVE_OPTICS_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
        assert f'DIVE_SCATTER_MODEL fog_gain={fog:.3f} view_gain={view:.3f}' in log
        for argument in ['-DiveEntry=2', '-DiveHour=12', '-DiveHabitatView', '-ResX=1600', '-ResY=900']:
            assert argument in log, (name, argument)
        assert 't.MaxFPS = "30"' in log
        if name == 'baseline':
            assert '-DiveFogScatterGain=' not in log and '-DiveViewScatterGain=' not in log
        poses = re.findall(r'DIVE_HABITAT_VIEW view=(\d+) eye=([^\r\n]+)', log)
        assert len(poses) == 3 and [p[0] for p in poses] == ['0', '1', '2']
        light = re.findall(r'DIVE_LIGHT_PASS ([^\r\n]+)', log)
        assert light
        if reference_poses is None:
            reference_poses, reference_light = poses, light
        assert poses == reference_poses, 'Ablation camera mismatch'
        assert light == reference_light, 'Daylight or ambient model changed'
        images = []
        for i in range(3):
            path = folder/f'Habitat_0_{i}.png'
            with Image.open(path) as image:
                assert image.size == (1600, 900)
                pixels = np.asarray(image.convert('RGB'), dtype=np.float64)
            if name == 'baseline':
                reference_pixels.append(pixels)
            # Display-referred values document the change, not physical radiance,
            # simulation accuracy or a quality threshold. Moving fish may differ.
            luminance = pixels @ np.array([.2126, .7152, .0722])
            images.append(dict(**record(path),
                display_luma_mean=float(luminance.mean()),
                display_luma_p10=float(np.percentile(luminance, 10)),
                display_luma_p90=float(np.percentile(luminance, 90)),
                rgb_absolute_difference_from_baseline=float(np.abs(pixels-reference_pixels[i]).mean())))
        results.append(dict(name=name, fog_addition_gain=fog, view_addition_gain=view,
            poses=poses, lighting=light, runtime_log=record(folder/'Runtime.log'), images=images))
    published = []
    for name, filename in [('baseline', 'Scattering_Baseline.png'), ('no-additions', 'Scattering_NoAdditions.png')]:
        path = ROOT/'Documentation/Images'/filename
        assert sha(path) == sha(run/name/'Habitat_0_2.png'), 'Published diagnostic image differs'
        published.append(record(path))
    report = dict(status='PASS', scope='Native diagnostic controls and matched captures only; no visual acceptance or performance result',
        host=host, host_file=record(run/'Host.json'), build_log=record(build), cases=results, screenshot_count=12,
        published=published,
        unchanged='Production defaults, direct lighting, fog density, absorption, exposure model, discovery visibility, sediment and torch',
        limitations=['One entry, noon, three camera directions, 1600x900; no packaged test',
                    'Moving fish and the renderer temporal history can differ between runs',
                    'Zero additive gains do not disable native volumetric scattering or medium extinction',
                    'The final visual quality reference is not met'])
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(f'PASS: 4 cases / 12 matched native captures -> {args.output}')


if __name__ == '__main__':
    main()
