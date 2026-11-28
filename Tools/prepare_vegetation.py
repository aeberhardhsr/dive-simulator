"""Create stable tree candidates from verified forest polygons and surveyed tops.

Forest density, tree mixture and tree heights within polygons are authored rules.
Surveyed individual tops retain their location and derive height from native
swissALTI3D. The Unreal bake anchors roots on its actual rendered ground.
"""
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from scipy.spatial import cKDTree
from shapely import points, STRtree, polygons
from shapely.geometry import shape
import rasterio
from rasterio.merge import merge

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'divingsim/Content/Diving/Geodata'
OUT=ROOT/'Geodata/Processed/Vegetation'
SPACING=12


def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()


def mix(value):
    value=np.asarray(value,dtype=np.uint32)
    value=value^(value>>np.uint32(16));value=value*np.uint32(0x7feb352d)
    value=value^(value>>np.uint32(15));value=value*np.uint32(0x846ca68b)
    return value^(value>>np.uint32(16))


def main():
    manifest=json.loads((ROOT/'Documentation/WALENSEE_VEGETATION_SOURCE.json').read_text(encoding='utf-8'))
    for item in manifest['outputs']:
        if sha(ROOT/item['file'])!=item['sha256']:raise ValueError('Vegetation source changed')
    forest=json.loads((ROOT/'Geodata/VegetationSource/forest.geojson').read_text())['features']
    trees=json.loads((ROOT/'Geodata/VegetationSource/tree_tops.geojson').read_text())['features']
    geometries=[shape(f['geometry']) for f in forest]
    classes=np.asarray([f['properties']['class_id'] for f in forest],dtype=np.uint8)
    bounds=manifest['bounds'];west,south,east,north=bounds
    columns=np.arange(west//SPACING,(east+SPACING-1)//SPACING,dtype=np.int64)
    rows=np.arange(south//SPACING,(north+SPACING-1)//SPACING,dtype=np.int64)
    xx,yy=np.meshgrid(columns,rows);xx=xx.ravel();yy=yy.ravel()
    seed=mix((xx*73856093 ^ yy*19349663 ^ 1948).astype(np.uint32))
    e=(xx+.5)*SPACING+(mix(seed^0x1973).astype(np.float64)/4294967296-.5)*SPACING*.7
    n=(yy+.5)*SPACING+(mix(seed^0x7031).astype(np.float64)/4294967296-.5)*SPACING*.7
    print('FOREST_CANDIDATES',len(e),flush=True)
    tree=STRtree(geometries)
    pair=tree.query(points(e,n),predicate='within')
    # Deterministic class precedence in overlapping mapped polygons.
    kind=np.zeros(len(e),dtype=np.uint8)
    for key in (13,12,14,6):
        selected=pair[0,classes[pair[1]]==key];kind[selected]=key
    keep=(kind!=0)&(e>west+2)&(e<east-2)&(n>south+2)&(n<north-2)
    # Open forest is less dense; shrub/hedge classes use the small-tree model.
    keep&=(kind!=13)|(seed%3==0)
    e,n,seed,kind=e[keep],n[keep],seed[keep],kind[keep]
    surveyed=np.array([f['geometry']['coordinates'] for f in trees],dtype=np.float64)
    distance,_=cKDTree(surveyed[:,:2]).query(np.column_stack((e,n)),workers=2)
    keep=distance>5
    e,n,seed,kind=e[keep],n[keep],seed[keep],kind[keep]
    source=np.zeros(len(e),dtype=np.uint8)
    e=np.concatenate((e,surveyed[:,0]));n=np.concatenate((n,surveyed[:,1]))
    top=np.concatenate((np.zeros(len(source)),surveyed[:,2]))
    ids=np.array([f['properties']['source_oid'] for f in trees],dtype=np.uint32)
    seed=np.concatenate((seed,mix(ids^0xa31725)))
    kind=np.concatenate((kind,np.zeros(len(trees),dtype=np.uint8)))
    source=np.concatenate((source,np.ones(len(trees),dtype=np.uint8)))

    land=json.loads((DATA/'landscape.json').read_text())
    for item in land['tiles']:
        if sha(ROOT/'Geodata/Raw/Landscape'/item['file'])!=item['sha256']: raise ValueError('ALTI checksum changed')
    rasters=[rasterio.open(ROOT/'Geodata/Raw/Landscape'/t['file']) for t in land['tiles']]
    try: heights,transform=merge(rasters,bounds=bounds,res=2,nodata=-9999)
    finally:
        for raster in rasters:raster.close()
    heights=heights[0]
    px=(e-transform.c)/2-.5;py=(transform.f-n)/2-.5
    keep=(px>=0)&(px<heights.shape[1]-1)&(py>=0)&(py<heights.shape[0]-1)
    e,n,top,seed,kind,source,px,py=[a[keep] for a in (e,n,top,seed,kind,source,px,py)]
    ix=px.astype(int);iy=py.astype(int);tx=px-ix;ty=py-iy
    a=heights[iy,ix];b=heights[iy,ix+1];c=heights[iy+1,ix];d=heights[iy+1,ix+1]
    ground=((a*(1-tx)+b*tx)*(1-ty)+(c*(1-tx)+d*tx)*ty)
    if not np.isfinite(ground).all() or min(a.min(),b.min(),c.min(),d.min())<-9000:raise ValueError('Missing terrain')
    random=mix(seed^0x1984).astype(np.float64)/4294967296
    height=np.where(source==1,top-ground,np.where((kind==6)|(kind==14),3.8+random*2.7,14+random*10))
    keep=(ground>420)&(height>=3)&(height<=45)
    invalid=int((~keep).sum())
    e,n,ground,height,seed,kind,source=[a[keep] for a in (e,n,ground,height,seed,kind,source)]

    # Buildings are retained in their surveyed coordinates. Exclude trunks near
    # their roof footprints; never move a source tree to make it fit.
    building=json.loads((DATA/'buildings.json').read_text())
    roofs=[]
    for tile in building['tiles']:
        path=DATA/'Buildings'/tile['file']
        if sha(path)!=tile['sha256']:raise ValueError('Building geometry changed')
        blob=path.read_bytes()
        if blob[:4]!=b'DVB1':raise ValueError('Invalid building binary')
        values=np.frombuffer(blob,dtype='<f4',offset=8).reshape(-1,13)
        vertices=values[values[:,12]>.5,:9].reshape(-1,3,3).astype(np.float64)
        xy=np.stack((vertices[:,:,1]/100+tile['origin_east'],vertices[:,:,0]/100+tile['origin_north']),axis=2)
        roofs.extend(polygons(xy))
    hits=STRtree(roofs).query(points(e,n),predicate='dwithin',distance=2.5)
    keep=np.ones(len(e),dtype=bool);keep[hits[0]]=False
    building_rejected=int((~keep).sum())
    e,n,ground,height,seed,kind,source=[a[keep] for a in (e,n,ground,height,seed,kind,source)]
    # Exact outline excludes lake and islands according to the source topology.
    lake=shape(json.loads((ROOT/'Geodata/Raw/WaterBoundary/walensee_polygon.geojson').read_text())['geometry'])
    from shapely import contains_xy
    keep=~contains_xy(lake,e,n)
    water_rejected=int((~keep).sum())
    e,n,ground,height,seed,kind,source=[a[keep] for a in (e,n,ground,height,seed,kind,source)]
    # Order follows fixed 256m cells, then stable seed. No camera-dependent seed.
    cx=np.floor((e-2726000)/256).astype(int);cy=np.floor((n-1219000)/256).astype(int)
    order=np.lexsort((seed,cx,cy))
    dtype=np.dtype([('east','<f4'),('north','<f4'),('height','<f4'),('native_ground','<f4'),('seed','<u4'),('class','u1'),('source','u1'),('pad','<u2')])
    records=np.zeros(len(e),dtype=dtype)
    for key,val in [('east',e-2726000),('north',n-1219000),('height',height),('native_ground',ground-419),('seed',seed),('class',kind),('source',source)]:records[key]=val
    OUT.mkdir(parents=True,exist_ok=True)
    path=OUT/'candidates.trees';path.write_bytes(b'DVT1'+struct.pack('<I',len(records))+records[order].tobytes())
    result=dict(schema=1,file=str(path.relative_to(ROOT).as_posix()),sha256=sha(path),record_bytes=24,count=len(records),
                forest_candidates=int((source==0).sum()),surveyed_candidates=int((source==1).sum()),
                rejected_height_or_water_elevation=invalid,rejected_buildings=building_rejected,rejected_lake=water_rejected,
                height_range_m=[float(height.min()),float(height.max())],spacing_m=SPACING,
                source_manifest_sha256=sha(ROOT/'Documentation/WALENSEE_VEGETATION_SOURCE.json'),
                landscape_manifest_sha256=sha(DATA/'landscape.json'),buildings_manifest_sha256=sha(DATA/'buildings.json'),
                model='Forest positions, density and species are authored; individual tops retain measured XY. Heights use native ALTI; roots are subsequently anchored to rendered terrain. Roads and seasonal foliage are not yet modeled.')
    (OUT/'candidates.json').write_text(json.dumps(result,indent=2)+'\n')
    print('VEGETATION_CANDIDATES_PASS',json.dumps(result),flush=True)


if __name__=='__main__':main()
