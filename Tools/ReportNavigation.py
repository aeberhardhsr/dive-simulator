"""Collect actual package navigation, click-target and regression evidence."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from PIL import Image


def evidence(path):
    return {'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def report(folder):
    host=json.loads((folder/'Host.json').read_text(encoding='utf-8-sig'))
    cases=json.loads((folder/'Cases.json').read_text(encoding='utf-8-sig'))
    expected={f'navigation-{mode}-{entry}' for mode in ['water','shore'] for entry in range(3)}
    expected.update(['navigation-1440p','navigation-4k','navigation-letterbox','shore','controls','tutorial','daycycle','sediment'])
    if {case['name'] for case in cases}!=expected: raise ValueError('Incomplete navigation matrix')
    for case in cases:
        directory=folder/case['name'];log=directory/'Runtime.log';text=log.read_text(encoding='utf-8-sig')
        if case['status']!='PASS' or 'DIVE_SMOKE_PASS' not in text or re.search(r'DIVE_\w+_FAIL|DIVE_NAV_PHASE \d+ FAIL|Failed to compile Material|Default Material will be used',text):
            raise ValueError('Scenario failed: '+case['name'])
        case['log']=evidence(log);images=sorted(directory.glob('*.png'))
        if len(images)!=case['screenshots']:raise ValueError('Screenshot count changed')
        size=tuple(int(v) for v in case['viewport'].split('x'))
        for image in images:
            with Image.open(image) as im:
                if im.size!=size:raise ValueError('Actual screenshot resolution mismatch: '+str(image))
        case['images']=[evidence(image) for image in images]
        if case['scenario']=='navigation':
            phases=[int(n) for n in re.findall(r'DIVE_NAV_PHASE (\d+) PASS',text)]
            if phases!=list(range(13)):raise ValueError('Missing navigation phase')
            result=re.search(r'DIVE_NAV_PASS entry=(\d) shore=(\d) selection=1 hitboxes=1 persistence=1 cancel=1 active_guard=1 pause=1',text)
            if not result or int(result[1])!=case['entry'] or bool(int(result[2]))!=case['shore']:
                raise ValueError('Wrong entry or start mode')
            labels=['Preview_DE','Shore_DE','Water_EN','Started','DiveMap','Locked','Controls','Resumed']
            if any(not (directory/f"Navigation_{case['entry']}_{label}.png").is_file() for label in labels):
                raise ValueError('Missing rendered navigation state')
            case['phases']=phases
    return {'status':'PASS','host':host,'cases':cases,'limits':[
        'Hit targets are resolved from rendered HUD coordinates, then dispatched through the HUD click handler; this is not a physical mouse-device test.',
        'The overview uses a fixed survey extent and horizontal straight-line navigation, without route planning.',
        'Windows checks do not establish native macOS gameplay or full visual acceptance.']}


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('directory',type=Path);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();result=report(args.directory.resolve())
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('PASS:',len(result['cases']),'cases;',sum(c['screenshots'] for c in result['cases']),'screenshots')
