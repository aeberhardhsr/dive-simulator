"""Report native surface, volume-source and camera-leg lighting comparisons."""
import argparse
import json
import re
from pathlib import Path

from ReportSolarShaderStudy import ROOT, ROIS, difference, pixels, read, record, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--study', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/SOLAR_VOLUME_RESULTS.json')
    args = parser.parse_args()
    root = args.study.resolve()
    host = read(root/'Host.json')
    for item in host['files']:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    manifests = []
    for name, volume in [('DiveSolarStudy', False), ('DiveSolarVolumeStudy', True)]:
        manifest = read(root/(name+'.json'))
        assert manifest['volume_enabled'] == volume
        assert sha(Path(manifest['engine_file'])) == manifest['original_sha256']
        assert sha(Path(manifest['destination'])/'DeferredLightPixelShaders.usf') == manifest['patched_sha256']
        assert sha(ROOT/'divingsim/Shaders/SolarMedium.ush') == manifest['common_include_sha256']
        assert sha(ROOT/'divingsim/Shaders/SolarTransport.ush') == manifest['shader_include_sha256']
        if volume:
            assert sha(Path(manifest['volume']['engine_file'])) == manifest['volume']['original_sha256']
            assert sha(Path(manifest['destination'])/'VolumetricFog.usf') == manifest['volume']['patched_sha256']
        manifests.append(manifest)
    inputs = read(root/'Cases.json')
    assert len(inputs) == 10 and all(c['status'] == 'PASS' for c in inputs)
    expected = {f'{view}-{mode}' for view in ('low', 'sun') for mode in ('direct', 'volume', 'combined')}
    expected |= {f'{view}-{mode}' for view in ('torch', 'air') for mode in ('direct', 'volume')}
    assert {c['name'] for c in inputs} == expected
    cases = {}
    for case in inputs:
        folder = root/case['name']
        log_path = folder/'Runtime.log'
        log = log_path.read_text(encoding='utf-8-sig')
        assert ('DIVE_CAUSTICS_PROBE_PASS captures=4' if case['kind'] == 'air' else 'DIVE_SMOKE_PASS') in log
        assert not re.search(r'DIVE_\w*FAIL|Fatal error:|Failed to compile|Default Material will be used', log), folder
        assert 'DIVE_SOLAR_SHADER_STUDY_READY' in log and 'DIVE_WATER_PREVIEW_READY' in log
        assert ('DIVE_SOLAR_VOLUME_STUDY_READY' in log) == case['volume']
        assert ('DIVE_SPECTRAL_VIEW_READY' in log) == case['spectral']
        images = sorted(folder.glob('*.png'))
        assert len(images) == case['count']
        for image in images:
            pixels(image)
        poses = re.findall(r'DIVE_(?:HABITAT_VIEW|CAUSTICS_CAPTURE) [^\r\n]+', log)
        if case['kind'] == 'torch':
            assert 'DIVE_TORCH_PASS on=1 lumens=1000.0' in log
        else:
            assert len(poses) == case['count']
        cases[case['name']] = dict(options=case, log=record(log_path), images=[record(p) for p in images], poses=poses,
            diagnostics=sorted(set(re.findall(r'[^\r\n]*(?:Error:|Warning:)[^\r\n]*', log))))
    comparisons = {}
    fog_roi = (450, 50, 1150, 200)
    for view in ('low', 'sun'):
        baseline = cases[view+'-direct']
        for mode in ('volume', 'combined'):
            other = cases[view+'-'+mode]
            assert baseline['poses'] == other['poses']
            metrics = []
            for i in range(3):
                a, b = root/f'{view}-direct/Habitat_0_{i}.png', root/f'{view}-{mode}/Habitat_0_{i}.png'
                metrics.append(dict(full_frame=difference(a, b), upper_water_roi=difference(a, b, fog_roi)))
            comparisons[view+'-'+mode] = metrics
        # Visible source-color change exceeds the near-identity controls below.
        delta = comparisons[view+'-volume'][0]['upper_water_roi']['delta_rgb']
        assert delta[0] < -2 and delta[0] < delta[1] and delta[0] < delta[2]
    assert cases['air-direct']['poses'] == cases['air-volume']['poses']
    air = {name: difference(root/'air-direct/Caustics_01.png', root/'air-volume/Caustics_01.png', roi) for name, roi in ROIS.items()}
    torch = {p.name: difference(p, root/'torch-volume'/p.name) for p in sorted((root/'torch-direct').glob('*.png'))}
    assert all(v['mean_absolute_rgb'] < 2 for v in list(air.values())+list(torch.values()))
    report = dict(status='NATIVE_VOLUME_SOURCE_PASS_EXPERIMENTAL', packaged=False, default_enabled=False,
        visual_target_complete=False, host=host, shader_manifests=manifests, cases=cases, screenshots=sum(len(c['images']) for c in cases.values()),
        report_tools=[record(Path(__file__).resolve()), record(ROOT/'Tools/ReportSolarShaderStudy.py')],
        upper_water_roi=fog_roi, habitat_comparisons=comparisons, air_receiver_rois=ROIS, air_comparison=air, torch_comparison=torch,
        scale='Tone-mapped LDR RGB 0..255; not radiometric',
        limits=['Shared extra RGB absorption only; scalar volume light-function projection still uses a representative eye-depth slice.',
                'No refracted volumetric beam direction or three-dimensional caustic focusing.',
                'Mean plane Z=0, no horizontal lake mask or wave-resolved absorption boundary.',
                'Indirect lighting and subsequent reflected/refracted paths remain uncorrected.',
                'Combined camera path keeps its existing immersion and 16-interval approximations.',
                'Only native Windows D3D12; no new package or performance claims.'])
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('SOLAR_VOLUME_REPORT_PASS', args.output)


if __name__ == '__main__':
    main()
