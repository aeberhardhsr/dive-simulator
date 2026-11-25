"""Verify packaged population continuity fixtures and ordinary gameplay runs."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from PIL import Image


def evidence(path):
    return {'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}


def report(folder):
    host=json.loads((folder/'Host.json').read_text(encoding='utf-8-sig'))
    cases=json.loads((folder/'Cases.json').read_text(encoding='utf-8-sig'))
    expected={'stream0','stream1','stream2','shore0','shore1','shore2','sediment','navigation'}
    if len(cases)!=8 or {c['name'] for c in cases}!=expected:raise ValueError('Incomplete population matrix')
    for case in cases:
        directory=folder/case['name'];log=directory/'Runtime.log';text=log.read_text(encoding='utf-8-sig')
        if case['status']!='PASS' or 'DIVE_SMOKE_PASS' not in text or re.search(r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used',text):
            raise ValueError('Failed scenario: '+case['name'])
        case['log']=evidence(log);images=sorted(directory.glob('*.png'))
        if len(images)!=case['screenshots'] or len(images)<2:raise ValueError('Missing rendered states')
        for image in images:
            with Image.open(image) as im:
                if im.size!=(1600,900):raise ValueError('Wrong viewport: '+str(image))
        case['images']=[evidence(image) for image in images]
        if case['scenario']=='stream':
            result=re.search(r'DIVE_POPULATION_PASS retained_cells=6 reloaded_cells=9 unchanged_near_fish=(\d+) distant_spawns=(\d+) active_cells=9 slots=77',text)
            if not result or int(result[1])<1 or int(result[2])<1:raise ValueError('Missing continuity/refill proof')
            case['unchanged_near_fish']=int(result[1]);case['distant_spawns']=int(result[2])
            # Exclude the synthetic query fixture and initial entry setup.
            ordinary=text.split('DIVE_POPULATION_TEST_END',1)[1]
            transitions=re.findall(r'DIVE_POPULATION_CELL x=(-?\d+) y=(-?\d+) loaded=(\d+) unloaded=(\d+) active=9 rocks=(\d+) milliseconds=([\d.]+)',ordinary)
            case['ordinary_cell_transitions']=[{'x':int(x),'y':int(y),'loaded':int(a),'unloaded':int(b),'rocks':int(r),'milliseconds':float(ms)} for x,y,a,b,r,ms in transitions]
            detail=re.search(r'DIVE_SMOKE_DETAIL entry=(\d) scenario=stream displacement=([\d.]+)',text)
            if not detail or int(detail[1])!=case['entry'] or float(detail[2])<50:raise ValueError('Stream route did not travel 50 m')
            case['displacement_metres']=float(detail[2])
    if not any(c.get('ordinary_cell_transitions') for c in cases):raise ValueError('No ordinary gameplay cell crossing')
    return {'status':'PASS','host':host,'cases':cases,'limits':[
        'The identity/reload/refill fixture changes the streaming query within one frame, then restores gameplay state; it is not a rendered traversal.',
        'The separate ordinary stream scenarios drive the pawn through real gameplay and log actual cell crossings.',
        'Static dressing is deterministic per cell and dive configuration, not a surveyed habitat map. Distant fish slots are recycled, not persisted as a lake-wide population.',
        'These capped Windows scenarios are not performance or native macOS evidence.']}


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('directory',type=Path);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();result=report(args.directory.resolve())
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('PASS:',len(result['cases']),'cases;',sum(c['screenshots'] for c in result['cases']),'screenshots')
