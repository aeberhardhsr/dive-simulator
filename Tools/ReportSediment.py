"""Collect packaged sediment lifecycle and rendered comparison evidence."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import numpy as np
from PIL import Image


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def report(directory):
    root=Path(directory).resolve()
    host=json.loads((root/'Host.json').read_text(encoding='utf-8-sig'))
    cases=json.loads((root/'Cases.json').read_text(encoding='utf-8-sig'))
    expected={'sediment0','sediment1','sediment2','sediment-night','shore0','shore1','shore2','breathing','stream'}
    if {c['name'] for c in cases}!=expected:raise ValueError('Incomplete scenario matrix')
    for case in cases:
        folder=root/case['name'];runtime=folder/'Runtime.log'
        text=runtime.read_text(encoding='utf-8-sig')
        if case['status']!='PASS' or 'DIVE_SMOKE_PASS' not in text or re.search(r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used',text):
            raise ValueError('Failed scenario '+case['name'])
        case['log']={'path':str(runtime),'sha256':digest(runtime)}
        case['images']=[{'path':str(p),'sha256':digest(p)} for p in sorted(folder.glob('*.png'))]
        if len(case['images'])!=case['screenshots']:raise ValueError('Screenshot count changed')
        if not case['name'].startswith('sediment'):continue
        result=re.search(r'DIVE_SEDIMENT_PASS born=(\d+) live=(\d+) volumes=(\d+)',text)
        if not result or int(result[1])<=0 or int(result[2])!=0 or int(result[3])>24 or 'DIVE_SEDIMENT_PAUSE_PASS' not in text:
            raise ValueError('Invalid sediment lifecycle')
        case['born']=int(result[1]);case['pool_size']=int(result[3])
        view=re.search(r'DIVE_SEDIMENT_VIEW projected=1 pixel=X=([-\d.]+) Y=([-\d.]+)',text)
        if not view:raise ValueError('Missing projected cloud centre')
        visible=next(folder.glob('Sediment_*_Visible.png'));hidden=next(folder.glob('Sediment_*_Hidden.png'))
        a=np.asarray(Image.open(visible).convert('RGB'),dtype=np.float32)
        b=np.asarray(Image.open(hidden).convert('RGB'),dtype=np.float32)
        if a.shape!=b.shape:raise ValueError('Image pair size mismatch')
        x,y=(int(float(view[i])) for i in [1,2]);height,width=a.shape[:2]
        if not (0<=x<width and 0<=y<height):raise ValueError('Cloud centre outside the comparison view')
        box=[max(0,x-200),max(0,y-200),min(width,x+200),min(height,y+200)]
        delta=np.max(np.abs(a-b),axis=2)[box[1]:box[3],box[0]:box[2]]
        case['comparison']={'roi':box,'mean_max_channel_difference':float(delta.mean()),
            'pixels_over_5':int((delta>5).sum()),'pixels_over_15':int((delta>15).sum())}
        # The old invisible native-volume attempt changed fewer than this in
        # the scene ROI through residual temporal effects. This is a regression
        # gate for the standard day case, not a photorealism criterion.
        if case['name']=='sediment0' and case['comparison']['pixels_over_5']<2000:
            raise ValueError('No substantial visible sediment in the standard comparison')
    return {'status':'PASS','host':host,'cases':cases,
        'limits':['Pixel differences include residual temporal shading; manually inspect the image pairs.',
                  'Authored sediment response and approximate scattering, not measured local transport.',
                  'These Windows scenarios do not establish native macOS or full visual acceptance.']}


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('directory');parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();result=report(args.directory)
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f"PASS: {len(result['cases'])} cases, {sum(c['screenshots'] for c in result['cases'])} screenshots")
