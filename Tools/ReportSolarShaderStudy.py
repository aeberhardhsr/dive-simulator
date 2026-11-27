"""Bind native direct-light transport comparisons to their sources and captures.

LDR image differences establish a rendering effect and regression bounds, not
radiometric accuracy or completion of the visual target.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ROIS = {'lit': (250, 580, 1350, 820), 'shadow': (900, 410, 1200, 470)}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path), bytes=path.stat().st_size)


def pixels(path, roi=None):
    with Image.open(path) as image:
        assert image.size == (1600, 900), path
        if roi:
            image = image.crop(roi)
        return np.asarray(image.convert('RGB'), dtype=np.float64)


def difference(before, after, roi=None):
    a, b = pixels(before, roi), pixels(after, roi)
    return dict(mean_absolute_rgb=float(np.abs(b-a).mean()),
                before_rgb=a.mean(axis=(0, 1)).tolist(),
                after_rgb=b.mean(axis=(0, 1)).tolist(),
                delta_rgb=(b-a).mean(axis=(0, 1)).tolist())


def capture(folder, count, enabled, fixture=False):
    path = folder/'Runtime.log'
    log = path.read_text(encoding='utf-8-sig')
    assert ('DIVE_CAUSTICS_PROBE_PASS captures=4' if fixture else 'DIVE_SMOKE_PASS') in log, folder
    assert not re.search(r'DIVE_\w*FAIL|Fatal error:|Failed to compile|Default Material will be used', log), folder
    assert ('DIVE_SOLAR_SHADER_STUDY_READY' in log) == enabled, folder
    assert 'DIVE_WATER_PREVIEW_READY' in log, folder
    # The independent camera-path correction must not confound this comparison.
    assert 'DIVE_SPECTRAL_VIEW_READY' not in log, folder
    images = sorted(folder.glob('*.png'))
    assert len(images) == count, folder
    for image in images:
        pixels(image)
    poses = re.findall(r'(DIVE_(?:HABITAT_VIEW|CAUSTICS_CAPTURE) [^\r\n]+)', log)
    if fixture:
        assert len(poses) == 4, folder
    elif folder.name.startswith('torch-'):
        assert 'DIVE_TORCH_PASS on=1 lumens=1000.0' in log, folder
    else:
        assert len(poses) == 3, folder
    diagnostics = sorted(set(re.findall(r'[^\r\n]*(?:Error:|Warning:)[^\r\n]*', log)))
    return dict(name=folder.name, enabled=enabled, log=record(path), images=[record(p) for p in images],
                poses=poses, diagnostics=diagnostics)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--study', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/SOLAR_SHADER_RESULTS.json')
    args = parser.parse_args()
    root = args.study.resolve()
    host = read(root/'Host.json')
    for item in host['files']:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    manifest = read(root/'ShaderManifest.json')
    assert sha(Path(manifest['engine_file'])) == manifest['original_sha256']
    assert sha(Path(manifest['destination'])/'DeferredLightPixelShaders.usf') == manifest['patched_sha256']
    assert sha(ROOT/'divingsim/Shaders/SolarTransport.ush') == manifest['shader_include_sha256']
    cases = []
    pairs = [('control', 'sun-path', 3, False), ('day-control', 'day-sun', 4, True),
             ('air-control', 'air-sun', 4, True), ('torch-control', 'torch-sun', 2, False)]
    for before, after, count, fixture in pairs:
        a, b = capture(root/before, count, False, fixture), capture(root/after, count, True, fixture)
        assert a['poses'] == b['poses'], (before, after)
        assert [Path(i['file']).name for i in a['images']] == [Path(i['file']).name for i in b['images']]
        cases.extend([a, b])
    habitat = [difference(root/'control'/f'Habitat_0_{i}.png', root/'sun-path'/f'Habitat_0_{i}.png') for i in range(3)]
    receivers = {}
    for fixture in ('day', 'air'):
        # Phase 1 disables moving caustic contrast, keeping scalar attenuation.
        receivers[fixture] = {name: difference(root/f'{fixture}-control/Caustics_01.png',
                                               root/f'{fixture}-sun/Caustics_01.png', roi)
                             for name, roi in ROIS.items()}
    torch = {p.name: difference(p, root/'torch-sun'/p.name) for p in sorted((root/'torch-control').glob('*.png'))}
    # Broad regression bounds account for temporal rendering noise. These are
    # image checks, not fitted physical coefficients or art acceptance tests.
    lit_delta = receivers['day']['lit']['delta_rgb']
    assert lit_delta[0] < -2 and lit_delta[0] < lit_delta[1] and lit_delta[0] < lit_delta[2]
    assert receivers['day']['shadow']['mean_absolute_rgb'] < 2
    assert all(value['mean_absolute_rgb'] < 2 for value in receivers['air'].values())
    assert all(value['mean_absolute_rgb'] < 2 for value in torch.values())
    report = dict(status='NATIVE_DIRECT_LIGHT_PASS_EXPERIMENTAL', visual_target_complete=False,
                  packaged=False, default_enabled=False, host=host, shader_manifest=manifest,
                  cases=cases, screenshots=sum(len(c['images']) for c in cases),
                  report_tool=record(Path(__file__).resolve()),
                  habitat_full_frame=habitat, receiver_rois=ROIS, receiver_differences=receivers,
                  torch_full_frame=torch, ldr_scale='0..255, tone mapped, not radiometric',
                  limits=['Mean water plane Z=0; no local wave boundary.',
                          'Authored RGB coefficients; no measured lake calibration.',
                          'Only deferred directional direct lighting; no GI, volumetric source or reflected-path correction.',
                          'LDR near-identity permits temporal noise and does not prove mathematical equality.',
                          'Native Windows D3D12 only; no package, performance or reference-quality acceptance.'])
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('SOLAR_SHADER_REPORT_PASS', args.output)


if __name__ == '__main__':
    main()
