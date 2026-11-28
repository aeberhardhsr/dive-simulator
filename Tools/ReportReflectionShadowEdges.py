"""Measure paired physical occlusion and real fallback controls in native captures."""
import argparse
import hashlib
import json
import re
from pathlib import Path

import numpy as np
from PIL import Image
from ReportReflectionTransport import load_run, frame, region


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def pixels(path):
    with Image.open(path) as image:
        if image.size != (1600,900):
            raise ValueError('Unexpected native image dimensions')
        return np.asarray(image.convert('RGB'),dtype=np.float64)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('--response',type=Path,required=True)
    parser.add_argument('--reference-moon',type=Path,help='Explicit replacement for the full-path moon capture at measurement exposure')
    parser.add_argument('--coverage',type=Path,help='Replacement usage diagnostic with exposure-independent colors')
    parser.add_argument('--unshadowed-sun',type=Path,required=True)
    parser.add_argument('--unshadowed-moon',type=Path,required=True)
    args=parser.parse_args()
    runs={x['profile']:Path(x['directory']) for x in load(args.run/'Runs.json')}
    if args.coverage:
        runs['vsm-coverage']=args.coverage
    results=[]
    for case in ['sun-shadow','moon-shadow','missing-maps','ambiguous-light']:
        folders={profile:(args.reference_moon if profile=='full' and case=='moon-shadow' and args.reference_moon else folder)
                 for profile,folder in runs.items()}
        captures=[]
        cameras=[]
        for profile,folder in folders.items():
            cases=load(folder/'Cases.json')
            if isinstance(cases,dict):
                cases=[cases]
            native=next(x for x in cases if x['name']==case)
            if native['status']!='PASS' or len(native['captures'])!=3:
                raise ValueError('Incomplete physical control')
            if native['options']['ReflectionProfile']!=profile or not native['options']['ReflectionUnified']:
                raise ValueError('Unexpected render profile or routing')
            captures.append(native['captures'])
            runtime=(folder/case/'Runtime.log').read_text(encoding='utf-8-sig',errors='replace')
            camera=re.findall(r'DIVE_WATER_VIEW state=0 view=\d eye=[^\r\n]+',runtime)
            if len(camera)!=3:
                raise ValueError('Missing capture-time exposure/camera evidence')
            cameras.append(camera)
        if any(x!=captures[0] for x in captures[1:]):
            raise ValueError(f'Light/camera/caster setup differs across profiles: {case}')
        if any(x!=cameras[0] for x in cameras[1:]):
            raise ValueError(f'Capture-time camera/exposure differs across profiles: {case}')
        images={profile:[pixels(folder/case/f'Water_0_{view}.png') for view in range(3)] for profile,folder in folders.items()}
        masks=[]
        for diagnostic in images['vsm-coverage']:
            r,g,b=diagnostic.transpose(2,0,1)
            masks.append((((r>70)&(g<20))|((g>70)&(r<20)))&(b<20))
        mask=np.logical_and.reduce(masks)
        count=int(mask.sum())
        if count<1000:
            results.append(dict(case=case,status='FAIL',failures=['Insufficient reflection coverage'],reflection_pixels=count))
            continue
        metrics={}
        failures=[]
        for profile in ['full','vsm']:
            luminance=[x[mask]@np.array([.2126,.7152,.0722]) for x in images[profile]]
            means=[float(x.mean()) for x in luminance]
            open_mean=(means[0]+means[2])*.5
            drop=open_mean-means[1]
            restored=abs(means[0]-means[2])
            metrics[profile]=dict(clear=means[0],blocked=means[1],restored=means[2],drop=drop,restore_difference=restored)
            if drop<max(.1,.02*open_mean):
                failures.append(f'{profile}: shadow darkening not established')
            if restored>max(2.,.05*open_mean):
                failures.append(f'{profile}: clear reference did not recover')
        differences=[float(np.abs(a[mask]-b[mask]).mean()) for a,b in zip(images['full'],images['vsm'])]
        if max(differences)>3:
            failures.append('Mapped/full reference difference exceeds 3 RGB display levels')
        mapped=[float((x[:,:,1][mask]>70).mean()) for x in images['vsm-coverage']]
        if case in ['missing-maps','ambiguous-light'] and max(mapped)>.005:
            failures.append('Unavailable/ambiguous map still used')
        if case=='sun-shadow' and min(mapped)<.1:
            failures.append('Directional map coverage not established')
        isolation=None
        if case in ['sun-shadow','moon-shadow']:
            control=args.unshadowed_sun if case=='sun-shadow' else args.unshadowed_moon
            control_cases=load(control/'Cases.json')
            if isinstance(control_cases,dict):
                control_cases=[control_cases]
            control_case=next(x for x in control_cases if x['name']==case)
            if (control_case['status']!='PASS' or control_case['options']['ReflectionProfile']!='unshadowed'
                    or not control_case['options']['ReflectionUnified'] or control_case['captures']!=captures[0]):
                raise ValueError('Shadow-isolation profile or physical setup differs')
            runtime=(control/case/'Runtime.log').read_text(encoding='utf-8-sig',errors='replace')
            camera=re.findall(r'DIVE_WATER_VIEW state=0 view=\d eye=[^\r\n]+',runtime)
            if camera!=cameras[0] or 'DIVE_SMOKE_PASS' not in runtime:
                raise ValueError('Shadow-isolation control is incomplete or uses another camera')
            raw=pixels(control/case/'Water_0_1.png')[mask]@np.array([.2126,.7152,.0722])
            raw_mean=float(raw.mean())
            reference_effect=raw_mean-metrics['full']['blocked']
            mapped_effect=raw_mean-metrics['vsm']['blocked']
            isolation=dict(unshadowed_blocked=raw_mean,full_shadow_effect=reference_effect,mapped_shadow_effect=mapped_effect)
            if reference_effect<.1 or mapped_effect<.5*reference_effect:
                failures.append('Reflected-path shadow contribution not established independently of camera fog')
        results.append(dict(case=case,status='FAIL' if failures else 'PASS',failures=failures,
                            reflection_pixels=count,lighting=metrics,reference_rgb_mae=differences,mapped_fraction=mapped,isolation=isolation))
    evidence=[]
    for profile,folder in runs.items():
        evidence.append(dict(profile=profile,folder=str(folder),host=load(folder/'Host.json'),cases=load(folder/'Cases.json'),
            images=[dict(file=str(file),sha256=hashlib.sha256(file.read_bytes()).hexdigest()) for file in sorted(folder.glob('*/Water*.png'))]))
    if args.reference_moon:
        folder=args.reference_moon
        evidence.append(dict(profile='full-moon-measurement-override',folder=str(folder),host=load(folder/'Host.json'),cases=load(folder/'Cases.json'),
            images=[dict(file=str(file),sha256=hashlib.sha256(file.read_bytes()).hexdigest()) for file in sorted(folder.glob('*/Water*.png'))]))
    for label,folder in [('unshadowed-sun',args.unshadowed_sun),('unshadowed-moon',args.unshadowed_moon)]:
        evidence.append(dict(profile=label,folder=str(folder),host=load(folder/'Host.json'),cases=load(folder/'Cases.json'),
            images=[dict(file=str(file),sha256=hashlib.sha256(file.read_bytes()).hexdigest()) for file in sorted(folder.glob('*/Water*.png'))]))
    response=args.response.resolve()
    response_evidence=load_run(response,['baseline-no-screens','clear','turbid','near-baseline','near-transport',
                                        'night-off','night-lamp-baseline','night-lamp-transport'])
    response_metrics=[dict(case=case,view=view,region=region(frame(response,case,view)))
        for case in ['baseline-no-screens','clear','turbid','night-off','night-lamp-baseline','night-lamp-transport'] for view in (0,1)]
    off=region(frame(response,'night-off',0))['mean_luma']
    lamp_gain=region(frame(response,'night-lamp-transport',0))['mean_luma']-region(frame(response,'night-lamp-baseline',0))['mean_luma']
    response_pass=off<=1 and lamp_gain>=2
    report=dict(status='FAIL' if not response_pass or any(x['status']=='FAIL' for x in results) else 'PASS',cases=results,evidence=evidence,
                response=dict(status='PASS' if response_pass else 'FAIL',evidence=response_evidence,metrics=response_metrics,
                              night_luma=off,reflected_lamp_luma_gain=lamp_gain),
                limits='Numerical gates and fixed scenes; visual review and the response suite are additionally required. Editor evidence, not a package release.')
    (args.run/'Results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    for row in results:
        print(row)
    if report['status']!='PASS':
        raise SystemExit(1)


if __name__=='__main__':
    main()
