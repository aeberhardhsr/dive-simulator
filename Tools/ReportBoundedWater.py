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
    parser.add_argument('--skip-published',action='store_true',help='Skip the original surface-image set; prior-report comparisons still validate their own published images')
    parser.add_argument('--build-log',type=Path,default=ROOT/'artifacts/bounded-water-sky-material-build.log')
    parser.add_argument('--prior-report',type=Path,help='Previously validated baseline for matched camera comparisons')
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
    build=args.build_log.resolve()
    build_text=build.read_text(encoding='utf-8-sig')
    assert 'Result: Succeeded' in build_text or ('DIVE_OPTICS_CONTENT_OK' in build_text and 'Success - 0 error(s)' in build_text)
    published=[]
    if not args.skip_published:
        for name,image,suffix in [('surface-before','Water_1_1.png','Before'),
                                  ('surface-bounded','Water_1_1.png','Window'),
                                  ('surface-bounded','Water_1_0.png','Surface')]:
            path=ROOT/'Documentation/Images'/f'BoundedWater_{suffix}.png'
            assert sha(path)==sha(run/name/image)
            published.append(record(path))
    bounded_passthrough='if(NativeScattering>1.5) return Scene.rgb;' in (ROOT/'Tools/Shaders/UnderwaterView.ush').read_text()
    result=dict(status='PASS',scope='Native volume contribution, matched cameras, air control, storm transitions and night; not visual acceptance',
        host=host,host_file=record(run/'Host.json'),supplementary_optics_source=supplementary,build=record(build),cases=evidence,
        screenshot_count=sum(c['screenshots'] for c in cases),comparisons=comparisons,air_region=air_region,night=night,moonlit_control=moon,
        published=published,bounded_camera_passthrough=bounded_passthrough,production_enabled=False,sky_background_fix_enabled_by_default=True,limitations=[
            'Native editor only; no packaged build, performance or long-term stability result',
            'Unreal volume uses scalar extinction and coloured albedo; no full spectral absorption transport',
            'Volume boundary is smoothed over 20 cm; finite froxel resolution and distance fade remain',
            'Refraction remains pixel-normal offset; full Snell projection and reflection detail remain unresolved',
            'The uncalibrated Milky Way texture is disabled; atmosphere, sun, moon and catalog stars remain in the sky',
            'The captured air-region control does not validate all above-water scenes'])
    if not bounded_passthrough:
        result['limitations'].append('A reduced unshadowed analytic ambient term remains in the camera pass')
    if args.prior_report:
        prior_path=args.prior_report.resolve()
        prior=json.loads(prior_path.read_text(encoding='utf-8-sig'))
        assert prior['status']=='PASS' and prior['screenshot_count']==33
        assert not prior['bounded_camera_passthrough'] and bounded_passthrough
        old_sources={item['file']:item['sha256'] for item in prior['host']['files']}
        new_sources={item['file']:item['sha256'] for item in host['files']}
        assert old_sources.keys()==new_sources.keys()
        changed=sorted(key for key in old_sources if old_sources[key]!=new_sources[key])
        assert changed==['Tools/Shaders/UnderwaterView.ush','divingsim/Content/Diving/Materials/M_Underwater.uasset'],changed
        pairs=[]
        assert len(prior['cases'])==len(evidence)
        for before,after in zip(prior['cases'],evidence):
            assert before['name']==after['name'] and before['poses']==after['poses']
            assert before['actual_water_cameras']==after['actual_water_cameras']
            assert len(before['images'])==len(after['images'])
            for item in [before['runtime_log'],*before['images']]:
                assert sha(ROOT/item['file'])==item['sha256'],item['file']
            for old_image,new_image in zip(before['images'],after['images']):
                assert Path(old_image['file']).name==Path(new_image['file']).name
                a=pixels(ROOT/old_image['file']);b=pixels(ROOT/new_image['file'])
                pairs.append(dict(case=before['name'],before=old_image,after=new_image,
                    mean_absolute_rgb_difference=float(np.abs(a-b).mean()),
                    before_mean_rgb=a.mean(axis=(0,1)).tolist(),after_mean_rgb=b.mean(axis=(0,1)).tolist()))
        result['camera_comparison']=dict(prior_report=record(prior_path),changed_inputs=changed,pairs=pairs,
            note='Includes moving lighting and temporal rendering variation; image differences are not a perceptual quality score')
        bed_pair=next(pair for pair in pairs if pair['case']=='bed-bounded' and Path(pair['after']['file']).name=='Habitat_0_0.png')
        published_comparison=[]
        for side in ['before','after']:
            path=ROOT/'Documentation/Images'/f'BoundedCamera_{side.title()}.png'
            assert sha(path)==bed_pair[side]['sha256']
            published_comparison.append(record(path))
        result['camera_comparison']['published']=published_comparison
    args.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'PASS: 12 bounded-water cases / {result["screenshot_count"]} native captures -> {args.output}')


if __name__=='__main__':main()
