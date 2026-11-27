"""Record the water-surface fog-grid correction and its packaged regressions."""
import argparse
import json
import re
from pathlib import Path

import numpy as np
from PIL import Image
from ReportNativeWaterLighting import ROOT, record, sha
from analyze_performance import analyze
from validate_lighting_frames import scene


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def boundary(path):
    # Region chosen from the observed straight boundary, below the horizon and
    # away from HUD/rope/shore reflections. This is not a whole-image quality score.
    with Image.open(path) as image:
        assert image.size == (1920,1080)
        rgb = np.asarray(image.convert('RGB'),dtype=float)
    rows = rgb[585:675,100:420].mean(axis=1)
    steps = np.abs(np.diff(rows,axis=0)).mean(axis=1)
    return dict(roi_xyxy=[100,585,420,675], max_mean_row_step_rgb=float(steps.max()),
                largest_step_y=int(585+steps.argmax()))


def first_slice(scale, depth_cm=60000, slices=64):
    # Installed UE CalculateGridZParams, near clipping .5 cm plus 9.5 cm offset.
    near = 10
    offset = (depth_cm-near*2**(slices/scale))/(depth_cm-near)
    slope = (1-offset)/near
    return (2**(.5/scale)-offset)/slope/100


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('--benchmark',type=Path,action='append',required=True)
    parser.add_argument('--output',type=Path,default=ROOT/'Documentation/SURFACE_NEAR_FIELD_RESULTS.json')
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run/'Host.json')
    assert host['packaged'] and host['water_preview']
    assert sha(Path(host['executable'])) == host['sha256']
    for item in host['files']: assert sha(ROOT/item['file']) == item['sha256'], item['file']
    package = Path(host['executable']).parents[3]
    for item in host['package_containers']:
        assert sha(package/'divingsim/Content/Paks'/item['file']) == item['sha256']
    cases = read(run/'Cases.json')
    expected = {'surface':3,'weesen':3,'murg':3,'ledi':3,'storm':3,'dark-on':2,'dark-off':2,
        'fog-fallback':3,'raster':3,'software':3,'stream':2,'shore':9,'sediment':7}
    assert len(cases)==13 and {c['name'] for c in cases}==set(expected)
    evidence = []
    for case in cases:
        folder = run/case['name']
        log_path = folder/'Runtime.log'
        log = log_path.read_text(encoding='utf-8-sig')
        assert sha(log_path)==case['log_sha256'] and case['status']=='PASS'
        assert 'DIVE_SMOKE_PASS' in log and 'shared_waves=1 ray_surface=0' in log
        assert f'DIVE_BOUNDED_WATER_PASS native={case["native_scattering"]}' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log)
        assert len(case['screenshots'])==len(list(folder.glob('*.png')))==expected[case['name']]
        for image in case['screenshots']:
            path = folder/image['file']
            assert sha(path)==image['sha256']
            with Image.open(path) as im: assert im.size==(1600,900)
        evidence.append(dict(**case,runtime_log=record(log_path)))
    off = scene(run/'dark-off/DiveUnderwater_0_torch.png')
    on = scene(run/'dark-on/DiveUnderwater_0_torch.png')
    night = dict(off_p999=float(np.percentile(off,99.9)),lit_pixels=int(((on>20)&(off<3)).sum()),
        mean_difference=float((on-off).mean()))
    assert night['off_p999']<=2 and night['lit_pixels']>5000 and night['mean_difference']>1
    launcher = read(run/'launcher/Launcher.json')
    assert launcher['status']=='PASS' and launcher['executable_sha256'].lower()==host['sha256']
    assert sha(run/'launcher/Launcher.log')==launcher['log_sha256'].lower()
    measurements = []
    for directory in args.benchmark:
        directory = directory.resolve()
        result = analyze(directory)
        m,h = result['metadata'],result['host']
        assert h['executable_sha256']==host['sha256'] and h['package_containers']==host['package_containers']
        assert not h['background_processes'] and not h['editor'] and h['water_preview']
        assert all(m[key] for key in ['water_preview','water_interface','bounded_water','native_scattering'])
        for key,value in {'width':1920,'height':1080,'r.ScreenPercentage':100,'r.VSync':0,'t.MaxFPS':0,
            'r.Lumen.HardwareRayTracing':1,'r.Lumen.HardwareRayTracing.LightingMode':2,
            'r.Lumen.HardwareRayTracing.HitLighting.Allowed':1,'r.Lumen.TranslucencyReflections.FrontLayer.Allow':1,
            'r.Lumen.Reflections.ScreenTraces':1,'r.VolumetricFog':1,'r.VolumetricFog.DepthDistributionScale':8,
            'r.VolumetricFog.GridSizeZ':64,'r.VolumetricFog.GridPixelSize':16}.items(): assert m[key]==value,(key,m[key])
        assert all(m[f'sg.{g}Quality']==2 for g in ['ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage'])
        entry = dict(result=result,files=[record(directory/name) for name in ['Capture.csv','Metadata.json','Host.json','Frame.png','Runtime.log']])
        if m['scenario']=='perf-surface':
            entry['boundary']=boundary(directory/'Frame.png')
            assert entry['boundary']['max_mean_row_step_rgb']<2
            final_surface=directory/'Frame.png'
        measurements.append(entry)
    assert len(measurements)==3 and {m['result']['metadata']['scenario'] for m in measurements}=={'perf-underwater','perf-surface','perf-stream'}
    previous_path = ROOT/'Documentation/WATER_PREVIEW_RESULTS.json'
    previous = read(previous_path)
    old_package=Path(previous['host']['executable']).parents[3]
    for item in previous['host']['package_containers']:
        assert sha(old_package/'divingsim/Content/Paks'/item['file'])==item['sha256']
    baseline = ROOT/'artifacts/perf-surface-1080p-q2-20261127-013134-976/Frame.png'
    baseline_record = next(f for b in previous['benchmarks'] for f in b['files'] if f['file']==baseline.relative_to(ROOT).as_posix())
    assert sha(baseline)==baseline_record['sha256'] and boundary(baseline)['max_mean_row_step_rgb']>4
    controls = []
    for name,command in [('screen-off','r.Lumen.Reflections.ScreenTraces 0'),
                         ('volume-off','r.VolumetricFog 0'),('depth-off','r.VolumetricFog.ConservativeDepth 0'),
                         ('water-volume-off','-DiveWaterVolumeGain=0'),('grid8','r.VolumetricFog.DepthDistributionScale 8')]:
        folder = ROOT/f'artifacts/surface-boundary-{name}'
        control_host = read(folder/'Host.json')
        assert control_host['sha256'].lower()==previous['host']['sha256']
        assert command in control_host['arguments']
        text = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert 'DIVE_PERF_CAPTURE_READY' in text and not re.search(r'DIVE_\w*FAIL|Fatal error:',text)
        metadata = read(folder/'Metadata.json')
        assert abs(metadata['simulation_seconds']-75)<.2
        controls.append(dict(name=name,host=control_host,metadata=metadata,boundary=boundary(folder/'Frame.png'),
            files=[record(folder/f) for f in ['Host.json','Metadata.json','Frame.png','Runtime.log','Capture.csv']]))
    published=[]
    for source,name in [(baseline,'Before'),(final_surface,'After')]:
        path=ROOT/f'Documentation/Images/SurfaceNearField_{name}.png'
        assert sha(path)==sha(source)
        published.append(record(path))
    build=ROOT/'artifacts/water-near-field-package-build.log'
    assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    result=dict(status='PASS',scope='Surface fog-grid correction, packaged water regressions and short measurements; not full visual acceptance',
        host=host,build=record(build),cases=evidence,screenshot_count=46,night=night,launcher=launcher,
        benchmarks=measurements,controls=controls,baseline=dict(**record(baseline),boundary=boundary(baseline)),
        previous_report=record(previous_path),published=published,
        nominal_first_slice_centres_metres={'scale32':first_slice(32),'scale8':first_slice(8)},
        engine_sources=[dict(file=str(path),sha256=sha(path)) for path in [
            Path('C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/RenderCore/Public/RenderUtils.h'),
            Path('C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Renderer/Private/VolumetricFog.cpp'),
            Path('C:/Program Files/Epic Games/UE_5.8/Engine/Shaders/Private/VolumetricFogVoxelization.usf')]],
        limitations=['Rotating benchmark captures have slightly different final times, not pixel-identical camera poses',
            'Boundary metric covers only the documented clear-water image region',
            'Redistribution trades distant fog depth resolution for near-surface resolution',
            'Software reflection gaps, complete refraction, reflected-water attenuation and reference quality remain open',
            'Manual menu-to-game input check and current endurance acceptance remain incomplete'])
    args.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print('PASS near-field surface correction, 13 package cases / 46 captures, three measured runs')


if __name__=='__main__': main()
