"""Bind water-reflection comparisons, raster fallback and dark-night checks."""
import argparse
import json
from pathlib import Path
import re
import numpy as np
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from validate_lighting_frames import scene


def read_log(path):
    log=path.read_text(encoding='utf-8-sig')
    for marker in ['DIVE_SMOKE_PASS','DIVE_MEDIUM_PASS','DIVE_OPTICS_PASS','DIVE_WATER_INTERFACE_PASS','DIVE_BOUNDED_WATER_PASS native=1']:
        assert marker in log,(path,marker)
    assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log),path
    return log


def water_poses(log):
    return re.findall(r'DIVE_WATER_(?:VIEW|CAMERA) ([^\r\n]+)',log)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('--path-study',type=Path,required=True)
    parser.add_argument('--roughness-study',type=Path,required=True)
    parser.add_argument('--skip-published',action='store_true')
    parser.add_argument('--prior-report',type=Path)
    parser.add_argument('--output',type=Path,default=ROOT/'Documentation/WATER_REFLECTIONS_RESULTS.json')
    args=parser.parse_args();run=args.run.resolve()
    path_study=args.path_study.resolve();roughness=args.roughness_study.resolve()
    host=json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    assert host['native_editor'] and not host['packaged']
    for item in host['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    cases=json.loads((run/'Cases.json').read_text(encoding='utf-8-sig'))
    expected={'surface-software','surface-hardware-cache','surface-hit','surface-hit-raster','storm-hit','dark-hit-on','dark-hit-off'}
    assert len(cases)==7 and {case['name'] for case in cases}==expected
    evidence=[];poses={}
    for case in cases:
        name=case['name'];folder=run/name;log=read_log(folder/'Runtime.log')
        assert 'shared_waves=1 ray_surface=0' in log
        for setting in ['r.Lumen.TranslucencyReflections.FrontLayer.Allow = "1"','r.Lumen.TranslucencyReflections.FrontLayer.Enable = "1"','t.MaxFPS = "30"']:
            assert setting in log,(name,setting)
        if case['hardware']:
            assert 'r.Lumen.HardwareRayTracing = "1"' in log and 'Ray tracing shaders are enabled.' in log
        if case['hit']:
            assert 'r.Lumen.HardwareRayTracing.LightingMode = "2"' in log
            assert 'r.Lumen.HardwareRayTracing.HitLighting.Allowed = "1"' in log
        if case['raster']:assert 'r.Nanite = "0"' in log
        if name=='storm-hit':
            assert 'DIVE_WEATHER_PASS kind=3 wind=22.0' in log
            for phase in ['air','transition','water']:assert f'DIVE_MEDIUM_PASS phase={phase}' in log
        if name.startswith('dark-'):
            assert float(re.search(r'DIVE_LIGHT_PASS elevation=([-\d.]+)',log)[1]) < -18
            assert float(re.findall(r'DIVE_SKY_LIGHT[^\r\n]*moon_elevation=([-\d.]+)',log)[-1]) < 0
        images=sorted(folder.glob('*.png'))
        assert len(images)==case['screenshots']==(2 if name.startswith('dark-') else 3)
        for path in images:pixels(path)
        poses[name]=water_poses(log)
        evidence.append(dict(**case,runtime_log=record(folder/'Runtime.log'),images=[record(path) for path in images],
            poses=poses[name],engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    for name in ['surface-hardware-cache','surface-hit','surface-hit-raster']:
        assert len(poses[name])==6 and poses[name]==poses['surface-software']
    off=scene(run/'dark-hit-off/DiveUnderwater_0_torch.png');on=scene(run/'dark-hit-on/DiveUnderwater_0_torch.png')
    night=dict(off_p999=float(np.percentile(off,99.9)),off_mean=float(off.mean()),
        lit_pixels=int(((on>20)&(off<3)).sum()),mean_difference=float((on-off).mean()))
    assert night['off_p999']<=2 and night['lit_pixels']>5000 and night['mean_difference']>1,night
    studies=[]
    for root,names,rough in [(path_study,['cache','front','hardware'],.08),(roughness,['front','hardware','hit'],.005)]:
        for name in names:
            folder=root/name;log=read_log(folder/'Runtime.log')
            assert water_poses(log)==poses['surface-software']
            images=[folder/f'Water_1_{view}.png' for view in range(3)]
            for image in images:pixels(image)
            studies.append(dict(case=name,roughness=rough,runtime_log=record(folder/'Runtime.log'),images=[record(path) for path in images]))
    build=roughness/'Content_final.log'
    build_text=build.read_text(encoding='utf-8-sig')
    assert 'DIVE_REFLECTION_ROUGHNESS old=0.005 new=0.02' in build_text and 'Success - 0 error(s)' in build_text
    comparisons=[]
    for before,after,label in [(path_study/'front',roughness/'front','software_roughness'),
                               (path_study/'hardware',roughness/'hardware','hardware_roughness'),
                               (roughness/'hardware',roughness/'hit','hit_lighting'),
                               (run/'surface-hardware-cache',run/'surface-hit','final_hit_lighting')]:
        for view in range(3):
            a=before/f'Water_1_{view}.png';b=after/f'Water_1_{view}.png'
            comparisons.append(dict(control=label,view=view,before=record(a),after=record(b),
                mean_absolute_rgb_difference=float(np.abs(pixels(a)-pixels(b)).mean())))
    published=[]
    for source,name in [(path_study/'front/Water_1_0.png','Before'),(run/'surface-hardware-cache/Water_1_0.png','Cache'),
                        (run/'surface-hit/Water_1_0.png','After')]:
        path=ROOT/'Documentation/Images'/f'WaterReflections_{name}.png'
        if not args.skip_published:
            assert sha(source)==sha(path)
            published.append(record(path))
    result=dict(status='PASS',scope='Native reflection image comparisons and regressions; not visual or performance acceptance',
        host=host,host_file=record(run/'Host.json'),cases=evidence,screenshot_count=sum(c['screenshots'] for c in cases),
        exploratory_cases=studies,exploratory_screenshot_count=18,comparisons=comparisons,night=night,build=record(build),
        previous_material=record(roughness/'M_WaterUnder_before.uasset'),published=published,production_enabled=False,
        limitations=['Native editor only; no packaged performance or long-term stability result',
                    'Hit lighting requires supported hardware ray tracing; software reflections retain incomplete ground coverage',
                    'Ray-traced reflected water-path absorption and full Snell transmission remain unresolved',
                    'Water is excluded from secondary ray geometry; recursive reflections of the water interface are not represented',
                    'Roughness 0.02 is an authored visual setting, not a measured lake parameter',
                    'Exploratory runs have archived logs and images, but only the final series has a complete captured input manifest'])
    module_build=ROOT/'artifacts/water-reflection-ray-surface-build.log'
    assert 'Result: Succeeded' in module_build.read_text(encoding='utf-8-sig')
    result['module_build']=record(module_build)
    if args.prior_report:
        path=args.prior_report.resolve();prior=json.loads(path.read_text(encoding='utf-8-sig'))
        assert prior['status']=='PASS' and prior['screenshot_count']==19
        old={item['file']:item['sha256'] for item in prior['host']['files']}
        new={item['file']:item['sha256'] for item in host['files']}
        assert old.keys()==new.keys()
        changed=sorted(name for name in old if old[name]!=new[name])
        assert changed==['Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll',
                         'divingsim/Source/DiveSim/DiveWater.cpp','divingsim/Source/DiveSim/DiveWorld.cpp'],changed
        for case in prior['cases']:
            for item in [case['runtime_log'],*case['images']]:assert sha(ROOT/item['file'])==item['sha256']
            assert case['poses']==poses[case['name']]
        result['prior_series']=dict(report=record(path),changed_inputs=changed,visual_status='Rejected: visible viewport seam and water self-intersections despite passing runtime checks')
        seam_images=[]
        for collection,suffix in [(prior['cases'],'Before'),(evidence,'After')]:
            source=next(item for item in collection if item['name']=='surface-hit')['images'][1]
            assert Path(source['file']).name=='Water_1_1.png'
            published_path=ROOT/'Documentation/Images'/f'WaterReflections_Seam{suffix}.png'
            assert sha(published_path)==source['sha256']
            seam_images.append(record(published_path))
        result['prior_series']['published_comparison']=seam_images
        controls=[]
        for name,corrected in [('water-reflection-screen-control',False),('water-reflection-self-surface-control',True),('water-reflection-self-surface-hybrid',True)]:
            folder=ROOT/'artifacts'/name;log=read_log(folder/'Runtime.log')
            assert water_poses(log)==poses['surface-software']
            assert ('ray_surface=0' in log)==corrected
            if name.endswith('control'):assert 'r.Lumen.Reflections.ScreenTraces = "0"' in log
            controls.append(dict(name=name,water_proxy_excluded=corrected,runtime_log=record(folder/'Runtime.log'),images=[record(folder/f'Water_1_{view}.png') for view in range(3)]))
        result['ray_controls']=controls
    args.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'PASS: 7 final cases / {result["screenshot_count"]} captures, 6 exploratory cases / 18 captures -> {args.output}')


if __name__=='__main__':main()
