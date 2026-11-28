"""Validate packaged torch recordings and compare rendered night frames.

Run with Tools/.geodata-env/Scripts/python.exe (numpy/rasterio). Pixel metrics
are regression evidence, not photometry or a visual realism acceptance.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

import numpy as np
from validate_lighting_frames import scene


def report(root):
    cases=json.loads((root/'Cases.json').read_text(encoding='utf-8-sig'))
    host=json.loads((root/'Host.json').read_text(encoding='utf-8-sig'))
    assert hashlib.sha256(Path(host['executable']).read_bytes()).hexdigest().upper()==host['executable_sha256'], 'Executable changed during validation'
    expected={'allround':(1000,54,120),'spot':(700,16,180),'wide':(3000,110,55)}
    rows={}
    images={}
    for case in cases:
        name=case['name']; folder=root/name; options=case['options']
        log=(folder/'Runtime.log').read_text(encoding='utf-8-sig',errors='replace')
        assert 'DIVE_SMOKE_PASS' in log and not re.search(r'DIVE_\w+_FAIL',log), name
        row={'scenario':options['Scenario'],'fresh_images':len(list(folder.glob('*.png'))),
            'sha256':{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(folder.iterdir()) if f.is_file()}}
        if options['Scenario']=='torch':
            model=['allround','spot','wide'][options.get('Torch',0)]
            lumens,cone,runtime=expected[model]
            match=re.search(r'DIVE_TORCH_PASS on=(\d) lumens=([\d.]+).*?model=(\w+) full_cone=([\d.]+) battery=([\d.]+)',log)
            assert match and match[3]==model and float(match[4])==cone, name
            assert int(match[1])==int(not options.get('LampOff',False)), name
            target=re.search(r'DIVE_TORCH_TARGET hit=1 distance_m=([\d.]+)',log)
            assert target and 1<float(target[1])<=20, name
            dive=json.loads((folder/'Dive.json').read_text(encoding='utf-8-sig'))
            lamp=dive['torch']
            assert (lamp['model'],lamp['nominal_lumens'],lamp['full_beam_degrees'],lamp['runtime_minutes'])==(model,lumens,cone,runtime), name
            start=options.get('Battery',100); on=not options.get('LampOff',False)
            def battery(seconds): return max(0,start-(100*seconds/(runtime*60) if on else 0))
            errors=[abs(s['lamp_battery_percent']-battery(s['seconds'])) for s in dive['samples']]
            errors.append(abs(lamp['remaining_percent']-battery(dive['duration_seconds'])))
            assert len(errors)>=23 and max(errors)<1e-6,(name,max(errors))
            images[name]=scene(folder/'DiveUnderwater_1_torch.png')
            row.update(model=model,nominal_lumens=lumens,full_cone_degrees=cone,
                observed_lumens=float(match[2]),target_distance_m=float(target[1]),
                remaining_percent=lamp['remaining_percent'],max_battery_error=max(errors),
                scene_mean=float(images[name].mean()),scene_p99=float(np.quantile(images[name],.99)))
        rows[name]=row
    required={'allround','spot','wide','off','empty','low','turbid','clear','day','day-spot','day-wide','config','profiles','surface','storm'}
    assert rows.keys()>=required, 'Missing scenarios'
    off=images['off']
    for name in ['allround','spot','wide']:
        increase=images[name]-off
        count=int(np.count_nonzero(increase>10))
        rows[name]['pixels_brighter_than_off_by_10']=count
        # Turning the torch off enables stronger dark adaptation. The mean of
        # the entire scene can therefore increase while the lit patch vanishes.
        # Test a substantial brighter patch, rather than total screen energy.
        assert count>500,(name,'no visible illumination')
    # Empty and switched-off lamps must agree in the same unlit scene. Allow
    # minor temporal variations from breathing/bubbles and temporal rendering.
    assert abs(images['empty'].mean()-off.mean())<.5, 'Empty lamp still lights scene'
    assert rows['empty']['observed_lumens']==0
    assert 0<rows['low']['observed_lumens']<rows['allround']['observed_lumens']*.55
    assert images['low'].mean()<images['allround'].mean(), 'Low battery is not visually dimmer'
    assert images['turbid'].mean()<images['clear'].mean(), 'Visibility does not reduce torch reach'
    result={'status':'PASS','host':host,'cases':rows,'artifact_directory':str(root.resolve()),
        'limits':'Authored game presets; pixel checks are not photometry. Visual review required.'}
    (root/'Results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path)
    report(parser.parse_args().directory)
