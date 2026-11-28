"""Independently audit the authored bank mask's geographic registration."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image
from shapely import LineString, distance, points

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output',type=Path)
    parser.add_argument('--verify-content',action='store_true')
    options=parser.parse_args()
    if not options.output and not options.verify_content:parser.error('--output or --verify-content is required')
    if options.output and options.output.exists():raise ValueError('Refusing to overwrite audit evidence')
    data=json.loads((ROOT/'divingsim/Content/Diving/Geodata/shore_stones.json').read_text(encoding='utf-8'))
    imagery=json.loads((ROOT/'divingsim/Content/Diving/Geodata/imagery.json').read_text())
    source=next(x for x in imagery['images'] if x['id']==data['site'])
    assert source['sha256']==data['source_image_sha256']
    assert hashlib.sha256((ROOT/data['source_image']).read_bytes()).hexdigest()==source['sha256']
    assert data['crs']=='EPSG:2056' and data['schema']==1
    assert data['source_size']==[source['width'],source['height']]
    assert data['source_bounds']==[source[k] for k in ('west','south','east','north')]
    sx=(source['east']-source['west'])/source['width']
    sy=(source['north']-source['south'])/source['height']
    line=LineString([(source['west']+x*sx,source['north']-y*sy) for x,y in data['trace_pixels']])
    assert line.is_simple and 50<line.length<150
    mask_path=ROOT/data['mask']['file']
    assert hashlib.sha256(mask_path.read_bytes()).hexdigest()==data['mask']['sha256']
    with Image.open(mask_path) as image:
        assert image.mode=='L' and image.size==(1024,1024)
        pixels=np.asarray(image)
    rng=np.random.default_rng(281128)
    # Focus on the small bank as well as uniform full-patch coverage.
    cols=np.concatenate((rng.integers(0,1024,10000),rng.integers(415,675,20000)))
    rows=np.concatenate((rng.integers(0,1024,10000),rng.integers(385,470,20000)))
    east=source['west']+(cols+.5)*(source['east']-source['west'])/1024
    north=source['north']-(rows+.5)*(source['north']-source['south'])/1024
    d=distance(points(east,north),line)
    t=np.clip((d-data['mask']['full_width_m']/2)/data['mask']['feather_m'],0,1)
    expected=np.rint(255*(1-t*t*(3-2*t))).astype(np.uint8)
    mismatches=int(np.count_nonzero(expected!=pixels[rows,cols]))
    assert mismatches==0
    stone_points=points([x['east'] for x in data['stones']],[x['north'] for x in data['stones']])
    max_distance=float(np.max(distance(stone_points,line)))
    assert max_distance<1.501
    for stone in data['stones']:
        assert 1<=stone['variant']<=3
        assert .4<=stone['width_m']<=1.5 and .4<=stone['length_m']<=1.5 and .2<=stone['height_m']<=1
    result=dict(status='PASS',pixel_centre_checks=len(rows),mismatches=mismatches,
        nonzero_checked=int(np.count_nonzero(expected)),trace_length_m=line.length,authored_candidates=len(data['stones']),
        max_candidate_distance_to_trace_m=max_distance,source_image_sha256=source['sha256'],mask_sha256=data['mask']['sha256'],
        scope='Authored trace and mask registration only; no survey accuracy or runtime contact claim.')
    if options.verify_content:
        content=json.loads((ROOT/'Documentation/SHORE_BANK_CONTENT.json').read_text(encoding='utf-8'))
        assert content['schema']==1 and content['mask_sha256']==data['mask']['sha256']
        for record in content['files']:
            assert hashlib.sha256((ROOT/record['file']).read_bytes()).hexdigest()==record['sha256'],record['file']
        result['content_files_verified']=len(content['files'])
    if options.output:options.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result))


if __name__=='__main__':main()
