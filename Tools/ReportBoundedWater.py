"""Record bounded water-volume rendering and its controlled regressions."""
import argparse
import json
import re
from pathlib import Path
import numpy as np
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from validate_lighting_frames import scene


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('--output',type=Path,default=ROOT/'Documentation/BOUNDED_WATER_RESULTS.json')
    args=parser.parse_args();run=args.run.resolve()
    host=json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    assert host['native_editor'] and not host['packaged']
    for item in host['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    supplementary=None
    if (run/'OpticsSource.json').exists():
        supplementary=json.loads((run/'OpticsSource.json').read_text(encoding='utf-8-sig'))
        for item in supplementary['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    cases=json.loads((run/'Cases.json').read_text(encoding='utf-8-sig'))
    expected={'surface-before','surface-bounded','surface-zero-density','bed-before','bed-bounded',
              'sun-bounded','storm-bounded','dark-bounded-on','dark-bounded-off','night-sky','dark-moonlit','fallback'}
    assert len(cases)==12 and {c['name'] for c in cases}==expected
    evidence=[];poses={};actual={}
    for case in cases:
        name=case['name'];folder=run/name
        log=(folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert all(x in log for x in ['DIVE_SMOKE_PASS','DIVE_MEDIUM_PASS','DIVE_OPTICS_PASS','DIVE_WATER_INTERFACE_PASS'])
        assert re.search(r'DIVE_SKY_LAYER name=deep[^\r\n]*visible=1 sky_capture=1',log)
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log)
        assert ('DIVE_BOUNDED_WATER_PASS' in log)==case['bounded']
        if case['bounded']:
            native=int(not case['fallback']);gain=0 if case['zero'] else 1
            assert re.search(rf'DIVE_BOUNDED_WATER_PASS native={native}[^\r\n]*gain={gain:.3f}',log)
        if case['fallback']:assert 'r.VolumetricFog = "0"' in log
        assert 't.MaxFPS = "30"' in log
        if name.startswith('dark-') or name=='night-sky':
            assert float(re.search(r'DIVE_LIGHT_PASS elevation=([-\d.]+)',log)[1]) < -18
            if name!='dark-moonlit':
                assert float(re.findall(r'DIVE_SKY_LIGHT[^\r\n]*moon_elevation=([-\d.]+)',log)[-1])<0
        if name=='storm-bounded':
            assert 'DIVE_WEATHER_PASS kind=3 wind=22.0' in log
            for phase in ['air','transition','water']:assert f'DIVE_MEDIUM_PASS phase={phase}' in log
        images=sorted(folder.glob('*.png'))
        assert len(images)==case['screenshots']==(2 if name.startswith('dark-') else 3)
        for path in images:pixels(path)
        poses[name]=re.findall(r'DIVE_(?:HABITAT_VIEW|WATER_VIEW) ([^\r\n]+)',log)
        actual[name]=re.findall(r'DIVE_WATER_CAMERA ([^\r\n]+)',log)
        evidence.append(dict(**case,runtime_log=record(folder/'Runtime.log'),poses=poses[name],actual_water_cameras=actual[name],
            images=[record(p) for p in images],engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    comparisons=[]
    for first,second,prefix in [('surface-before','surface-bounded','Water_1'),
                               ('surface-zero-density','surface-bounded','Water_1'),('bed-before','bed-bounded','Habitat_0')]:
        assert len(poses[first])==3 and poses[first]==poses[second]
        if prefix=='Water_1':assert len(actual[first])==3 and actual[first]==actual[second]
        for i in range(3):
            a=pixels(run/first/f'{prefix}_{i}.png');b=pixels(run/second/f'{prefix}_{i}.png')
            comparisons.append(dict(first=first,second=second,view=i,mean_absolute_rgb_difference=float(np.abs(a-b).mean())))
    # The zero-density control keeps camera processing and air fog unchanged.
    # Thus a visible image difference demonstrates actual volume rendering,
    # rather than merely the existence of a registered mesh/material.
    density_effect=[c['mean_absolute_rgb_difference'] for c in comparisons if c['first']=='surface-zero-density' and c['view']<2]
    assert max(density_effect)>1,density_effect
    old=pixels(run/'surface-before'/'Water_1_2.png');new=pixels(run/'surface-bounded'/'Water_1_2.png')
    air_region=dict(rect=[400,0,1200,80],mean_absolute_rgb_difference=float(np.abs(old[:80,400:1200]-new[:80,400:1200]).mean()))
    assert air_region['mean_absolute_rgb_difference']<2,air_region
    off=scene(run/'dark-bounded-off'/'DiveUnderwater_0_torch.png');on=scene(run/'dark-bounded-on'/'DiveUnderwater_0_torch.png')
    night=dict(mask_source='Tools/validate_lighting_frames.py:scene',off_p999=float(np.percentile(off,99.9)),off_mean=float(off.mean()),
               lit_pixels=int(((on>20)&(off<3)).sum()),mean_difference=float((on-off).mean()))
    assert night['off_p999']<=2 and night['lit_pixels']>5000 and night['mean_difference']>1,night
    moon_log=(run/'dark-moonlit/Runtime.log').read_text(encoding='utf-8-sig')
    moon_sources=re.findall(r'DIVE_SKY_LIGHT sun_elevation=([-\d.]+) moon_elevation=([-\d.]+) moon_lux=([-\d.]+)',moon_log)
    sun,altitude,lux=map(float,moon_sources[-1])
    assert sun < -18 and altitude>0 and lux>0
    moon_frame=scene(run/'dark-moonlit/DiveUnderwater_0_torch.png')
    moon=dict(sun_elevation=sun,moon_elevation=altitude,moon_lux=lux,scene_mean=float(moon_frame.mean()))
    assert moon['scene_mean']>night['off_mean']+.1,moon
    build=ROOT/'artifacts/bounded-water-sky-material-build.log'
    assert 'Result: Succeeded' in build.read_text(encoding='utf-8-sig')
    published=[]
    for name,image,filename in [('surface-before','Water_1_1.png','BoundedWater_Before.png'),
                                ('surface-bounded','Water_1_1.png','BoundedWater_Window.png'),
                                ('surface-bounded','Water_1_0.png','BoundedWater_Surface.png')]:
        path=ROOT/'Documentation/Images'/filename
        assert sha(path)==sha(run/name/image)
        published.append(record(path))
    result=dict(status='PASS',scope='Native volume contribution, matched cameras, air control, storm transitions and night; not visual acceptance',
        host=host,host_file=record(run/'Host.json'),supplementary_optics_source=supplementary,build=record(build),cases=evidence,
        screenshot_count=sum(c['screenshots'] for c in cases),comparisons=comparisons,air_region=air_region,night=night,moonlit_control=moon,
        published=published,production_enabled=False,sky_background_fix_enabled_by_default=True,limitations=[
            'Native editor only; no packaged build, performance or long-term stability result',
            'Unreal volume uses scalar extinction and coloured albedo; no full spectral absorption transport',
            'A reduced unshadowed analytic ambient term remains in the camera pass',
            'Volume boundary is smoothed over 20 cm; finite froxel resolution and distance fade remain',
            'Refraction remains pixel-normal offset; full Snell projection and reflection detail remain unresolved',
            'The uncalibrated Milky Way texture is disabled; atmosphere, sun, moon and catalog stars remain in the sky',
            'The captured air-region control does not validate all above-water scenes'])
    args.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'PASS: 12 bounded-water cases / {result["screenshot_count"]} native captures -> {args.output}')


if __name__=='__main__':main()
