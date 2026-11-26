"""Bind source colour evidence, actual native/package captures and build inputs."""
import argparse
import io
import json
import re
from pathlib import Path
import numpy as np
from PIL import Image, ImageCms
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from ReportRocks import package_check


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('run', type=Path)
    p.add_argument('--baseline', type=Path, required=True)
    p.add_argument('--audit-before', type=Path, required=True)
    p.add_argument('--audit-after', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    run = a.run.resolve()
    host = json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    for item in host['files']:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    if host['packaged']:
        package_check(host)
    before = json.loads(a.audit_before.read_text())
    after = json.loads(a.audit_after.read_text())
    assert before['pass'] and after['pass'] and after['require_source_encoding']
    assert len(before['textures']) == len(after['textures']) == 18
    old = {t['asset']: t for t in before['textures']}
    for texture in after['textures']:
        prior = old[texture['asset']]
        for field in ['asset', 'source_format', 'platform_format', 'srgb', 'width', 'height', 'mips', 'source_center_rgb']:
            assert prior.get(field) == texture.get(field), (texture['asset'], field)
        assert texture['source_encoding'] == (2 if texture['srgb'] else 1)
    sources = []
    for manifest in ['BED_MATERIALS.json', 'ROCK_SURFACES.json']:
        for asset in json.loads((ROOT/'Documentation'/manifest).read_text())['assets']:
            for item in asset['maps'].values():
                assert sha(ROOT/item['file']) == item['sha256']
            source = ROOT/asset['maps']['Diffuse']['file']
            with Image.open(source) as image:
                description = ImageCms.getProfileDescription(ImageCms.ImageCmsProfile(io.BytesIO(image.info['icc_profile']))).strip()
                assert description == 'sRGB IEC61966-2.1'
                sources.append(dict(file=record(source), icc_description=description))
    cases = json.loads((run/'Cases.json').read_text(encoding='utf-8-sig'))
    expected = {'weesen':3, 'murg':3, 'ledi-close':3, 'ledi-bounded':3, 'torch':2, 'shore':1}
    assert len(cases) == len(expected) and {c['name'] for c in cases} == set(expected)
    evidence = []
    for case in cases:
        folder = run/case['name']
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
        if case['name'] in ['weesen','murg','ledi-close','ledi-bounded']:
            assert 'DIVE_POPULATION_PASS' in log
        if case['name'] == 'ledi-bounded':
            assert 'DIVE_BOUNDED_WATER_PASS native=1' in log
        images = sorted(folder.glob('*.png'))
        assert len(images) == case['screenshots'] == expected[case['name']]
        for image in images:
            pixels(image)
        evidence.append(dict(**case, runtime=record(folder/'Runtime.log'), images=[record(x) for x in images],
            engine_error_lines=[s for s in log.splitlines() if ': Error:' in s]))
    logs = [(folder/'Runtime.log').read_text(encoding='utf-8-sig') for folder in [a.baseline, run/'ledi-close']]
    poses = [re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)', text) for text in logs]
    assert len(poses[0]) == 3 and poses[0] == poses[1]
    comparisons = []
    for index in range(3):
        name = f'Habitat_0_{index}.png'
        b, c = a.baseline/name, run/'ledi-close'/name
        delta = float(np.abs(pixels(b)-pixels(c)).mean())
        assert delta > .1
        comparisons.append(dict(before=record(b), after=record(c), mean_absolute_rgb_difference=delta))
    result = dict(status='PASS', scope='Explicit sRGB source decoding for benthic maps; not overall visual acceptance',
        host=host, cases=evidence, screenshot_count=sum(expected.values()), comparisons=comparisons,
        source_profiles=sources, audit_before=dict(file=record(a.audit_before), result=before),
        audit_after=dict(file=record(a.audit_after), result=after),
        limitations=['Visual evaluation still finds sparse planting and smooth sediment transitions',
                    'Short rendering checks do not establish frame-rate or endurance acceptance',
                    'Experimental bounded water remains diagnostic only'])
    a.output.write_text(json.dumps(result, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(f'PASS {len(cases)} cases / {sum(expected.values())} captures -> {a.output}')


if __name__ == '__main__':
    main()
