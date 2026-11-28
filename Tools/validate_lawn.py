"""Independent ray-crossing and segment-distance audit of the lawn mask."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path);parser.add_argument('--verify-content',action='store_true');args=parser.parse_args()
    if args.output and args.output.exists():raise RuntimeError('Refusing to overwrite evidence')
    data=json.loads((ROOT/'divingsim/Content/Diving/Geodata/lawn.json').read_text())
    assert data['schema']==1 and data['crs']=='EPSG:2056'
    assert hashlib.sha256((ROOT/data['source_file']).read_bytes()).hexdigest()==data['source_sha256']
    assert hashlib.sha256((ROOT/'divingsim/Content/Diving/Geodata/shore_stones.json').read_bytes()).hexdigest()==data['bank_source_sha256']
    source=ROOT/data['mask']['file'];assert hashlib.sha256(source.read_bytes()).hexdigest()==data['mask']['sha256']
    with Image.open(source) as im:
        assert im.mode=='L' and im.size==(1024,1024);mask=np.asarray(im)
    rng=np.random.default_rng(291128)
    cols=np.concatenate([rng.integers(0,1024,10000),rng.integers(415,675,30000)])
    rows=np.concatenate([rng.integers(0,1024,10000),rng.integers(365,455,30000)])
    west,south,east,north=data['bounds'];x=west+(cols+.5)*(east-west)/1024;y=north-(rows+.5)*(north-south)/1024
    inside=np.zeros(len(x),dtype=bool);minimum=np.full(len(x),np.inf)
    vertices=np.asarray(data['polygon_lv95'])
    for a,b in zip(vertices,vertices[1:]):
        if a[1]!=b[1]:inside^=((a[1]>y)!=(b[1]>y)) & (x<(b[0]-a[0])*(y-a[1])/(b[1]-a[1])+a[0])
        delta=b-a;t=np.clip(((x-a[0])*delta[0]+(y-a[1])*delta[1])/np.dot(delta,delta),0,1)
        minimum=np.minimum(minimum,np.hypot(x-a[0]-t*delta[0],y-a[1]-t*delta[1]))
    t=np.clip(minimum/data['feather_m'],0,1)
    expected=np.rint(255*t*t*(3-2*t)*inside).astype(np.uint8)
    differences=int(np.count_nonzero(expected!=mask[rows,cols]));assert differences==0
    probes={}
    for name,pixel,want in [('lawn',(1100,800),255),('road',(1100,730),0),('parking',(1220,750),0),('bank',(1086,896),0),('water',(1040,900),0)]:
        got=int(mask[pixel[1]//2,pixel[0]//2]);assert got==want,(name,got);probes[name]=got
    assert len(data['candidates'])<10000
    result=dict(status='PASS',pixel_checks=len(x),mismatches=differences,nonzero_checks=int(np.count_nonzero(expected)),probes=probes,
                authored_candidates=len(data['candidates']),area_m2=data['area_m2'],mask_sha256=data['mask']['sha256'],
                scope='Authored polygon and image registration; does not prove surveyed land cover or runtime terrain contacts.')
    if args.verify_content:
        content=json.loads((ROOT/'Documentation/LAWN_CONTENT.json').read_text(encoding='utf-8'))
        assert content['schema']==1
        for item in content['files']:
            assert hashlib.sha256((ROOT/item['file']).read_bytes()).hexdigest()==item['sha256'],item['file']
        result['content_files_verified']=len(content['files'])
    if args.output:args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result))
if __name__=='__main__':main()
