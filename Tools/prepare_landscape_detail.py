"""Prepare one isolated 2m terrain tile; preserve the existing 8m boundary."""
import json
import hashlib
from pathlib import Path
import numpy as np
import rasterio
from audit_landscape_resolution import ROOT, sha, triangle_height


def main():
    base=ROOT/'divingsim/Content/Diving/Geodata'
    manifest=json.loads((base/'landscape.json').read_text(encoding='utf-8'))
    meshes=json.loads((base/'landscape_meshes.json').read_text(encoding='utf-8'))
    # This fully inland tile contains the steep face opposite Murg. Restrict the
    # experiment to a full tile so shore cutouts and underwater geometry cannot change.
    tile=next(t for t in meshes['tiles'] if t['asset'].endswith('SM_Land_05_02.SM_Land_05_02'))
    if tile['triangles']!=131072 or tile['vertices']!=66049 or manifest['spacing_m']!=8:
        raise ValueError('Study requires an intact 256-cell inland tile')
    runtime=base/manifest['file']
    if sha(runtime)!=manifest['sha256']:
        raise ValueError('Runtime source checksum mismatch')
    grid=np.fromfile(runtime,dtype='<f4').reshape(manifest['rows'],manifest['columns'])
    west=manifest['west']+(tile['column']+.5)*8
    north=manifest['north']-(tile['row']+.5)*8
    east=west+2048;south=north-2048
    mw=int(west//1000)*1000;ms=int(south//1000)*1000
    me=int(np.ceil(east/1000))*1000;mn=int(np.ceil(north/1000))*1000
    native=np.full(((mn-ms)//2,(me-mw)//2),np.nan,dtype=np.float64)
    sources=[]
    for item in manifest['tiles']:
        if not(mw<=item['east']<me and ms<=item['north']<mn): continue
        path=ROOT/'Geodata/Raw/Landscape'/item['file']
        if sha(path)!=item['sha256']: raise ValueError('Native source checksum mismatch')
        with rasterio.open(path) as source:
            if source.crs.to_epsg()!=2056 or source.res!=(2,2) or tuple(source.bounds)!=(item['east'],item['north'],item['east']+1000,item['north']+1000):
                raise ValueError('Native registration mismatch')
            data=source.read(1,masked=True)
            if np.any(np.ma.getmaskarray(data)): raise ValueError('Incomplete native tile')
            r=(mn-item['north']-1000)//2;c=(item['east']-mw)//2
            native[r:r+500,c:c+500]=np.asarray(data,dtype=np.float64)-manifest['water_elevation_m']
        sources.append(item)
    if not np.isfinite(native).all(): raise ValueError('Native mosaic incomplete')
    e,n=np.meshgrid(west+np.arange(1025)*2,north-np.arange(1025)*2)
    fc=(e-mw)/2-.5;fr=(mn-n)/2-.5
    c=np.floor(fc).astype(int);r=np.floor(fr).astype(int);u=fc-c;v=fr-r
    fine=(native[r,c]*(1-u)+native[r,c+1]*u)*(1-v)+(native[r+1,c]*(1-u)+native[r+1,c+1]*u)*v
    coarse,valid=triangle_height(grid,e,n,manifest)
    if not valid.all() or fine.min()<80 or coarse.min()<80:
        raise ValueError('Study tile must remain fully inland and above shore geometry')
    distance=np.minimum.reduce((e-west,east-e,north-n,n-south))
    t=np.clip(distance/32,0,1);weight=t*t*(3-2*t)
    height=(coarse+(fine-coarse)*weight).astype('<f4')
    edge=distance==0
    if np.max(np.abs(height[edge]-coarse[edge]))>.0002:
        raise ValueError('Boundary differs from original triangles')
    out=base/'LandscapeStudy';out.mkdir(exist_ok=True)
    blob=out/'detail_05_02.f32';blob.write_bytes(height.tobytes())
    report={'schema':1,'base_asset':tile['asset'],
            'asset':'/Game/Diving/LandscapeStudy/SM_LandDetail_05_02.SM_LandDetail_05_02',
            'column':tile['column'],'row':tile['row'],'width':1025,'height':1025,'spacing_m':2,
            'west_vertex_east':west,'north_vertex_north':north,
            'origin_x_cm':tile['origin_x_cm'],'origin_y_cm':tile['origin_y_cm'],
            'file':blob.name,'sha256':sha(blob),'base_grid_sha256':manifest['sha256'],
            'base_asset_sha256':sha(ROOT/'divingsim/Content/Diving/Landscape/SM_Land_05_02.uasset'),
            # UE's generic SHA256 platform API is unimplemented on this Windows
            # source build. SHA256 remains the offline evidence hash; SHA1 adds
            # native corruption/staleness guards using Core's implemented API.
            'native_sha1':hashlib.sha1(blob.read_bytes()).hexdigest(),
            'base_grid_sha1':hashlib.sha1(runtime.read_bytes()).hexdigest(),
            'base_asset_sha1':hashlib.sha1((ROOT/'divingsim/Content/Diving/Landscape/SM_Land_05_02.uasset').read_bytes()).hexdigest(),
            'blend_border_m':32,'maximum_boundary_error_m':float(np.max(np.abs(height[edge]-coarse[edge]))),
            'interior_max_interpolation_roundoff_m':float(np.max(np.abs(height[weight==1]-fine[weight==1]))),
            'source_tiles':sources,'attribution':'© swisstopo, swissALTI3D; source editions/checksums listed above',
            'method':'Native 2m pixel-centre heights bilinearly resampled at the aligned 2m mesh vertices; 32m smooth boundary blend to exact original 8m triangles. No invented displacement.',
            'limits':'Isolated inland display study; no normal game, source raster, coast, bathymetry or collision replacement.'}
    (out/'detail_study.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print('LANDSCAPE_DETAIL_PREPARED',height.shape,'range_m',float(height.min()),float(height.max()),
          'edge_error_m',report['maximum_boundary_error_m'])


if __name__=='__main__': main()
