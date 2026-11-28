"""Authored Weesen lawn footprint, independent of aerial-photo object colours."""
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image
from shapely import points, contains, distance
from shapely.geometry import Polygon, LineString

ROOT=Path(__file__).resolve().parents[1]

def main():
    source=next(x for x in json.loads((ROOT/'divingsim/Content/Diving/Geodata/imagery.json').read_text())['images'] if x['id']=='weesen_strandbad')
    assert source['width']==source['height']==2048 and source['east']-source['west']==source['north']-source['south']==384
    image=ROOT/'Geodata/Processed/Imagery'/source['file']
    assert hashlib.sha256(image.read_bytes()).hexdigest()==source['sha256']
    # Interpreted from the pinned image, excluding road/parking and shoreline.
    # This is an authored playing surface, not surveyed cadastral/biological data.
    trace=[(850,835),(920,816),(1000,791),(1065,775),(1137,749),
           (1153,786),(1190,795),(1225,790),(1255,787),(1285,779),(1319,780),
           (1319,796),(1280,805),(1250,816),(1220,836),(1190,851),(1160,862),
           (1130,873),(1101,886),(1096,892),(1080,864),(1064,843),(1042,830),
           (1018,830),(992,834),(970,847),(962,874),(940,879),(900,880),(850,870)]
    to_world=lambda x,y:(source['west']+x*.1875,source['north']-y*.1875)
    polygon=Polygon([to_world(x,y) for x,y in trace])
    assert polygon.is_valid
    size=1024
    yy,xx=np.mgrid[:size,:size]
    pts=points(source['west']+(xx.ravel()+.5)*384/size,source['north']-(yy.ravel()+.5)*384/size)
    inside=contains(polygon,pts)
    t=np.clip(distance(polygon.boundary,pts)/.75,0,1)
    mask=np.rint(255*t*t*(3-2*t)*inside).astype(np.uint8).reshape(size,size)
    target=ROOT/'Art/GroundMaterials/WeesenLawnMask.png'
    Image.fromarray(mask).save(target)
    bank=json.loads((ROOT/'divingsim/Content/Diving/Geodata/shore_stones.json').read_text())
    assert bank['source_image_sha256']==source['sha256']
    bankline=LineString([to_world(x,y) for x,y in bank['trace_pixels']])
    area=polygon.buffer(-.55).difference(bankline.buffer(3.8))
    rng=np.random.default_rng(281129)
    west,south,east,north=area.bounds
    candidates=[]
    for e in np.arange(west,east,.48):
        for n in np.arange(south,north,.48):
            x,y=e+rng.uniform(-.15,.15),n+rng.uniform(-.15,.15)
            if contains(area,points(x,y)):
                candidates.append(dict(east=round(x,4),north=round(y,4),yaw=round(rng.uniform(0,360),3),scale=round(rng.uniform(.85,1.15),3)))
    result=dict(schema=1,crs='EPSG:2056',site='weesen_strandbad',source_sha256=source['sha256'],
                source_file=image.relative_to(ROOT).as_posix(),bounds=[source[k] for k in ('west','south','east','north')],
                trace_pixels=trace,polygon_lv95=list(polygon.exterior.coords),area_m2=polygon.area,
                feather_m=.75,bank_source_sha256=hashlib.sha256((ROOT/'divingsim/Content/Diving/Geodata/shore_stones.json').read_bytes()).hexdigest(),
                mask=dict(file=target.relative_to(ROOT).as_posix(),size=size,sha256=hashlib.sha256(target.read_bytes()).hexdigest()),
                candidates=candidates,meaning='Authored lawn and decorative grass, not surveyed vegetation or object reconstruction.')
    (ROOT/'divingsim/Content/Diving/Geodata/lawn.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('LAWN_PREPARED',len(candidates),'tufts',polygon.area,'m2')

if __name__=='__main__':main()
