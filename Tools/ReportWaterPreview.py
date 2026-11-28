"""Bind packaged water-profile checks and measurements; not visual acceptance."""
import argparse
import json
import re
from pathlib import Path

import numpy as np
from PIL import Image
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from validate_lighting_frames import scene
from analyze_performance import analyze


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--benchmark', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/WATER_PREVIEW_RESULTS.json')
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run/'Host.json')
    assert host['packaged'] and host['water_preview']
    assert sha(Path(host['executable'])) == host['sha256']
    for item in host['files']:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    package = Path(host['executable']).parents[3]
    for item in host['package_containers']:
        assert sha(package/'divingsim/Content/Paks'/item['file']) == item['sha256']
    cases = read(run/'Cases.json')
    expected = {'surface':3, 'weesen':3, 'murg':3, 'ledi':3, 'storm':3, 'dark-on':2,
                'dark-off':2, 'fog-fallback':3, 'raster':3, 'software':3, 'stream':2, 'shore':9, 'sediment':7}
    assert len(cases) == len(expected) and {c['name'] for c in cases} == set(expected)
    evidence = []
    for case in cases:
        name = case['name']
        folder = run/name
        log_path = folder/'Runtime.log'
        log = log_path.read_text(encoding='utf-8-sig')
        assert sha(log_path) == case['log_sha256'] and case['status'] == 'PASS'
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
        hardware, native = int(name != 'software'), int(name != 'fog-fallback')
        assert f'DIVE_WATER_PREVIEW_READY smoke=1 hardware={hardware}' in log
        assert f'DIVE_BOUNDED_WATER_PASS native={native}' in log
        assert 'shared_waves=1 ray_surface=0' in log and 'DIVE_SMOKE_PASS' in log
        assert len(case['screenshots']) == len(list(folder.glob('*.png'))) == expected[name]
        for image in case['screenshots']:
            path = folder/image['file']
            assert sha(path) == image['sha256']
            pixels(path)
        if name in {'weesen','murg','ledi','stream','fog-fallback'}:
            assert 'DIVE_PLANT_BEDS_PASS' in log and 'DIVE_POPULATION_PASS' in log
        if name == 'fog-fallback': assert 'r.VolumetricFog = "0"' in log
        if name == 'raster': assert 'r.Nanite = "0"' in log
        if name == 'storm': assert 'DIVE_WEATHER_PASS kind=3 wind=22.0' in log
        evidence.append(dict(**case, runtime_log=record(log_path),
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    off = scene(run/'dark-off/DiveUnderwater_0_torch.png')
    on = scene(run/'dark-on/DiveUnderwater_0_torch.png')
    night = dict(off_p999=float(np.percentile(off,99.9)), off_mean=float(off.mean()),
        lit_pixels=int(((on>20)&(off<3)).sum()), mean_difference=float((on-off).mean()))
    assert night['off_p999'] <= 2 and night['lit_pixels'] > 5000 and night['mean_difference'] > 1
    launcher = read(run/'launcher/Launcher.json')
    assert launcher['status'] == 'PASS' and launcher['executable_sha256'].lower() == host['sha256']
    assert sha(run/'launcher/Launcher.log') == launcher['log_sha256'].lower()
    manual = ROOT/'artifacts/water-preview-manual'
    manual_log = (manual/'Runtime.log').read_text(encoding='utf-8-sig')
    assert 'DIVE_WATER_PREVIEW_READY smoke=0 hardware=1' in manual_log
    assert '-DiveSmokeTest' not in manual_log
    manual_check = dict(status='MENU_ONLY', gameplay_input_verified=False,
        reason='Click blocked twice by PickerHost.exe after reactivation; no further UI input attempted',
        files=[record(manual/name) for name in ['Runtime.log','Menu.png','Process.json']])
    measurements = []
    for directory in args.benchmark:
        directory = directory.resolve()
        result = analyze(directory)
        m, h = result['metadata'], result['host']
        assert h['executable_sha256'] == host['sha256'] and h['package_containers'] == host['package_containers']
        assert not h['editor'] and not h['background_processes'] and h['water_preview']
        assert all(m[key] for key in ['water_preview','water_interface','bounded_water','native_scattering'])
        for key,value in {'r.Lumen.HardwareRayTracing':1,'r.Lumen.HardwareRayTracing.LightingMode':2,
            'r.Lumen.HardwareRayTracing.HitLighting.Allowed':1,'r.Lumen.TranslucencyReflections.FrontLayer.Allow':1,
            'r.VolumetricFog':1,'r.ScreenPercentage':100,'r.VSync':0,'t.MaxFPS':0}.items():
            assert m[key] == value, (key,m[key])
        assert m['width'] == 1920 and m['height'] == 1080
        assert all(m[f'sg.{group}Quality'] == 2 for group in ['ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage'])
        with Image.open(directory/'Frame.png') as image: assert image.size == (1920,1080)
        measurements.append(dict(result=result, files=[record(directory/name) for name in ['Capture.csv','Metadata.json','Host.json','Frame.png','Runtime.log']]))
    assert len(measurements) == 3 and {m['result']['metadata']['scenario'] for m in measurements} == {'perf-underwater','perf-surface','perf-stream'}
    build = ROOT/'artifacts/water-preview-package-build.log'
    assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    published = []
    for source, filename in [('surface/Water_1_0.png','WaterPreview_Reflection.png'),
                             ('weesen/Habitat_0_0.png','WaterPreview_Bed.png'),
                             ('software/Water_1_0.png','WaterPreview_Software.png')]:
        path = ROOT/'Documentation/Images'/filename
        assert sha(path) == sha(run/source)
        published.append(record(path))
    output = dict(status='PASS', scope='Packaged opt-in water profile and measured performance; not full visual acceptance',
        host=host, build=record(build), cases=evidence, screenshot_count=sum(expected.values()), night=night,
        launcher=launcher, manual_check=manual_check, benchmarks=measurements, published=published,
        limitations=['Surface benchmark image retains a straight near-water brightness boundary requiring investigation',
            'Software reflections retain incomplete terrain coverage',
            'Full Snell transmission and reflected underwater-path attenuation remain unresolved',
            'Water excluded from secondary ray geometry; no recursive water reflections',
            'Short measurements on one PC do not prove system requirements or endurance stability',
            'World visual reference and deferred menu/HUD redesign remain incomplete'])
    args.output.write_text(json.dumps(output,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'PASS {len(cases)} package cases / {sum(expected.values())} images, root launcher, three measured runs')


if __name__ == '__main__':
    main()
