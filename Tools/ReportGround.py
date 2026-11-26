"""Bind ground-material package checks, screenshots and compiled inputs to bytes."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import numpy as np
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run / 'Host.json')
    executable = Path(host['executable'])
    assert host['packaged'] and sha(executable) == host['sha256'].lower()
    package = executable.parents[3]
    containers = [dict(file=p.name, bytes=p.stat().st_size, sha256=sha(p))
                  for p in sorted((package / 'divingsim/Content/Paks').iterdir()) if p.is_file()]
    assert sorted(host['package_containers'], key=lambda p:p['file']) == containers
    cases = read(run / 'Cases.json')
    expected = {f'entry-{e}-{view}' for e in range(3) for view in ['land', 'ground', 'shore', 'coast']}
    expected |= {'weesen-before-land', 'weesen-before-ground', 'murg-fallback-ground'}
    assert len(cases) == len(expected) and {c['name'] for c in cases} == expected
    evidence = []
    for case in cases:
        folder = run / case['name']
        log = (folder / 'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used', log)
        enabled = 0 if case['ground_off'] else 1
        assert f'DIVE_GROUND_PASS materials=5 textures=7 enabled={enabled} resident_bytes=33751040' in log
        for marker in ['DIVE_IMAGERY_PASS', 'DIVE_LANDSCAPE_PASS', 'DIVE_BUILDINGS_PASS', 'DIVE_VEGETATION_PASS']:
            assert marker in log
        if case['scenario'] == 'coast':
            assert f"DIVE_COAST_ROUTE_PASS entry={case['entry']} crossed_unmeasured=1 returned_measured=1" in log
        if case['scenario'] == 'shore':
            assert 'DIVE_SHORE_PASS' in log
            for medium in ['air', 'water', 'transition']:
                assert f'DIVE_MEDIUM_PASS phase={medium}' in log
        if case['fallback']:
            assert 'r.Nanite = "0"' in log
        images = []
        for image in sorted(folder.glob('*.png')):
            with Image.open(image) as im:
                assert im.size == (1600, 900)
                im.verify()
            images.append(dict(file=image.relative_to(ROOT).as_posix(), sha256=sha(image)))
        assert len(images) == case['screenshots'] and images
        evidence.append(dict(**case, runtime_sha256=sha(folder / 'Runtime.log'), images=images))
    # Same package/camera with only the detail parameter disabled. This numeric
    # crop check detects a disconnected/no-op material, not aesthetic quality.
    before = Image.open(run / 'weesen-before-ground/Shore_0_Ground.png').convert('RGB')
    after = Image.open(run / 'entry-0-ground/Shore_0_Ground.png').convert('RGB')
    crop = (600, 100, 1000, 300)
    a, b = before.crop(crop), after.crop(crop)
    def energy(im):
        return float(np.std(np.asarray(im, dtype=float) - np.asarray(im.filter(ImageFilter.GaussianBlur(2)), dtype=float)))
    comparison = dict(crop=list(crop), mean_absolute_difference=float(np.mean(np.abs(np.asarray(a, dtype=float)-np.asarray(b, dtype=float)))),
                      before_highpass_std=energy(a), after_highpass_std=energy(b))
    assert comparison['mean_absolute_difference'] > 2
    assert comparison['after_highpass_std'] > comparison['before_highpass_std'] * 1.2
    sources = []
    project = ROOT / 'divingsim'
    sandbox = ROOT / 'Builds/EditorSandbox/divingsim'
    for directory in ['Source', 'Config']:
        for path in sorted((project / directory).rglob('*')):
            if not path.is_file(): continue
            relative = path.relative_to(project)
            canonical = path.read_bytes().replace(b'\r\n', b'\n')
            assert canonical == (sandbox / relative).read_bytes().replace(b'\r\n', b'\n'), str(relative)
            sources.append(dict(file=relative.as_posix(), canonical_lf_sha256=hashlib.sha256(canonical).hexdigest()))
    assets = list((project / 'Content/Diving/Textures').glob('T_Ground_*.uasset'))
    assets += [project / f'Content/Diving/Materials/{name}.uasset' for name in ['M_ShorePhoto', 'M_LandscapePhoto']]
    assert len(assets) == 9
    mask = read(ROOT / 'Documentation/GROUND_MASK.json')
    assert mask['status'] == 'PASS' and mask['independent_pixel_center_checks'] == 20000
    assert sha(ROOT / mask['file']) == mask['sha256']
    assert sha(ROOT / 'Geodata/VegetationSource/forest.geojson') == mask['source_sha256']
    for asset in read(ROOT / 'Documentation/GROUND_MATERIALS.json')['assets']:
        for item in asset['maps'].values(): assert sha(ROOT / item['file']) == item['sha256']
    build_log = ROOT / 'artifacts/ground-package.log'
    text = build_log.read_text(encoding='utf-8-sig')
    assert 'BUILD SUCCESSFUL' in text and not re.search(r'Failed to compile Material|Default Material will be used', text)
    baseline = ROOT / 'Builds/WindowsVegetation/divingsim/Binaries/Win64/divingsim-Vegetation.exe'
    assert sha(baseline) == '264144503e29fc4cca00a4b582f0cbca7830bb0c7712e0d9bdba9928af9a360e'
    launcher = read(ROOT / 'artifacts/ground-bootstrap.json')
    assert launcher['status'] == 'PASS' and launcher['executable_sha256'].lower() == host['sha256'].lower()
    report = dict(status='PASS', executable=host, cases=evidence, image_comparison=comparison,
        root_launcher=launcher, mask_audit=mask, sources=sources,
        assets=[dict(file=p.relative_to(project).as_posix(), sha256=sha(p)) for p in sorted(assets)],
        package_log_sha256=sha(build_log), preserved_vegetation_executable_sha256=sha(baseline),
        limits='Authored ground classes; automated routes and image checks are not visual acceptance, minimum-hardware or native macOS certification.')
    report['build_inputs'] = [dict(file=name, canonical_lf_sha256=hashlib.sha256((ROOT / name).read_bytes().replace(b'\r\n', b'\n')).hexdigest())
        for name in ['Tools/build_ground_content.py', 'Tools/build_bed_content.py', 'Tools/build_imagery_content.py',
                     'Tools/Shaders/GroundDetail.ush', 'Tools/Shaders/Lakebed.ush',
                     'Documentation/GROUND_MATERIALS.json', 'Documentation/GROUND_MASK.json']]
    (ROOT / 'Documentation/GROUND_RESULTS.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('GROUND_PASS', len(cases), 'cases', sum(len(c['images']) for c in evidence), 'images', comparison)


if __name__ == '__main__':
    main()
