"""Independently check packaged environment recordings against extracted source facts."""
import argparse
import hashlib
import json
import math
import re
import struct
import warnings
from pathlib import Path
import numpy as np
import rasterio
from rasterio.errors import NotGeoreferencedWarning


def font_pixels(path):
    with warnings.catch_warnings():
        warnings.simplefilter('ignore',NotGeoreferencedWarning)
        with rasterio.open(path) as source:
            rgb=source.read((1,2,3))
    white=np.min(rgb,axis=0)>150
    return {
        'wind':white[280:320,950:1100],
        'direction':white[280:320,1245:1400],
        'current':white[480:520,1245:1420],
        'clock':white[580:620,660:790],
        'date':white[580:620,1245:1460]
    }


def interpolate(depth, depths, values):
    if depth <= depths[0]:
        return values[0]
    for index, end in enumerate(depths[1:], 1):
        if depth <= end:
            start = depths[index-1]
            return values[index-1] + (values[index]-values[index-1])*(depth-start)/(end-start)
    return values[-1]


def report(root):
    source=json.loads((Path(__file__).resolve().parents[1]/'Documentation/WALENSEE_TEMPERATURE.json').read_text(encoding='utf-8'))
    cases=json.loads((root/'Cases.json').read_text(encoding='utf-8-sig'))
    host=json.loads((root/'Host.json').read_text(encoding='utf-8-sig'))
    assert hashlib.sha256(Path(host['executable']).read_bytes()).hexdigest().upper()==host['executable_sha256']
    required={'environment','winter','summer','manual','config','profiles','shore','torch'}
    assert {c['name'] for c in cases}==required, 'Incomplete case matrix'
    rows={}
    for case in cases:
        name=case['name']; folder=root/name
        log=(folder/'Runtime.log').read_text(encoding='utf-8-sig',errors='replace')
        assert 'DIVE_SMOKE_PASS' in log and not re.search(r'DIVE_\w+_FAIL|DIVE_ENVIRONMENT_PHASE \d+ FAIL',log),name
        images=list(folder.glob('*.png'))
        assert images,name
        for image in images:
            raw=image.read_bytes()
            assert raw[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack('>II',raw[16:24])==(1600,900),image
        row=dict(fresh_images=len(images),sha256={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(folder.iterdir()) if f.is_file()})
        if name in {'environment','winter','summer','manual','torch'}:
            dive=json.loads((folder/'Dive.json').read_text(encoding='utf-8-sig'))
            assert dive['is_test'] and not dive['status'].startswith('in_progress'),name
            env=dive['environment']; samples=[s for s in dive['samples'] if s['seconds']>0]
            assert len(samples)>=(8 if name=='environment' else 22),name
            auto=name in {'environment','winter','summer'}
            assert env['auto_temperature']==env['auto_visibility']==auto,name
            assert env['current_source']==int(auto),name
            if auto:
                season=0 if name=='winter' else 2
                assert env['scenario_date']==('2026-02-15' if season==0 else '2026-08-15'),name
                depths=source['depths_m']; values=source['temperature_c'][season]
                assert env['temperature_model']=='thermdis_walensee_v1',name
                assert len(env['temperature_profile'])==17,name
                for point,depth,value in zip(env['temperature_profile'],depths,values):
                    assert point['depth_m']==depth and abs(point['temperature_c']-value)<1e-10,name
                expected_visibility=18 if name=='winter' else 6.4 if name=='environment' else 12
                expected_current=.052 if name=='winter' else .112 if name=='environment' else .072
                assert abs(env['visibility_m']-expected_visibility)<1e-9,name
                assert abs(env['current_mps']-expected_current)<1e-9,name
                errors=[abs(s['water_c']-interpolate(s['depth_m'],depths,values)) for s in samples]
            else:
                assert env['temperature_model']=='manual_thermocline_v1' and 'temperature_profile' not in env,name
                assert env['visibility_m']==14 and env['current_mps']==(0 if name=='torch' else .08),name
                errors=[abs(s['water_c']-(18+(6-18)/(1+math.exp(-(s['depth_m']-10)/2)))) for s in samples]
            assert max(errors)<1e-8,(name,max(errors))
            row.update(temperature_model=env['temperature_model'],visibility_m=env['visibility_m'],current_mps=env['current_mps'],
                       samples=len(samples),water_range_c=[min(s['water_c'] for s in samples),max(s['water_c'] for s in samples)],
                       max_temperature_error_c=max(errors))
        if name=='environment':
            assert all(f'DIVE_ENVIRONMENT_PHASE {i} PASS' in log for i in range(8))
            assert 'DIVE_ENVIRONMENT_MATERIALS PASS visibility_cm=640.000000 extinction_per_cm=0.002265625' in log
            assert len(images)>=7
            first=font_pixels(folder/'EnvironmentWinterFirstFrame_DE.png')
            settled=font_pixels(folder/'EnvironmentWinter_DE.png')
            counts={}
            for field,minimum in {'wind':150,'direction':200,'current':250,'clock':200,'date':400}.items():
                count=int(first[field].sum());difference=int(np.count_nonzero(first[field]!=settled[field]))
                assert count>minimum,(field,'Missing first-frame text',count)
                assert difference<max(10,count*.05),(field,'Text changed after atlas warmed',difference)
                counts[field]=dict(first_frame_white_pixels=count,difference_from_settled=difference)
            row['first_frame_text']=counts
        rows[name]=row
    result=dict(status='PASS',host=host,cases=rows,artifact_directory=str(root.resolve()),
                temperature_source=source['source_url'],temperature_source_sha256=source['source_sha256'],
                limits='Seasonal means with authored calendar interpolation; visibility/current are scenario values. No live weather, spatial hydrodynamics or native macOS graphics validation.')
    (root/'Results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path)
    report(parser.parse_args().directory)
