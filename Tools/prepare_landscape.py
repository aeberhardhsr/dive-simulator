"""Download official swissALTI3D tiles and prepare the Walensee surroundings.

Run in a virtual environment with requirements-geodata.txt installed.
Every download is checked against the STAC SHA256 multihash.
"""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import urllib.request
import numpy as np
from pyproj import Transformer
import rasterio

ROOT=Path(__file__).resolve().parents[1]
RAW=ROOT/'Geodata/Raw/Landscape'
DEST=ROOT/'divingsim/Content/Diving/Geodata'
BOUNDS=(2723000,1215000,2745000,1228000)  # LV95, includes surrounding mountain ridges.
SPACING=8
NODATA=-9999.


def read_json(url):
    with urllib.request.urlopen(url, timeout=90) as response:
        return json.load(response)


def discover():
    convert=Transformer.from_crs(2056,4326,always_xy=True)
    west,south,east,north=BOUNDS
    corners=[convert.transform(x,y) for x in (west,east) for y in (south,north)]
    bbox=','.join(str(v) for v in (min(x for x,y in corners),min(y for x,y in corners),
                                  max(x for x,y in corners),max(y for x,y in corners)))
    url='https://data.geo.admin.ch/api/stac/v1/collections/ch.swisstopo.swissalti3d/items?limit=100&bbox='+bbox
    chosen={}
    while url:
        page=read_json(url)
        for feature in page['features']:
            parts=feature['id'].split('_')
            if len(parts)!=3: continue
            x,y=(int(n)*1000 for n in parts[2].split('-'))
            if not(west<=x<east and south<=y<north): continue
            for name,asset in feature['assets'].items():
                if not name.endswith('_2_2056_5728.tif'): continue
                key=(x,y)
                if key not in chosen or parts[1]>chosen[key]['year']:
                    checksum=asset['file:checksum']
                    if not checksum.lower().startswith('1220') or len(checksum)!=68:
                        raise ValueError('Expected SHA256 STAC multihash')
                    chosen[key]=dict(year=parts[1],east=x,north=y,file=name,
                                     url=asset['href'],sha256=checksum[4:].lower())
        url=next((link['href'] for link in page['links'] if link['rel']=='next'),None)
        print(f'Discovered {len(chosen)} current tiles',flush=True)
    expected=(east-west)//1000*(north-south)//1000
    if len(chosen)!=expected: raise ValueError(f'Incomplete coverage: {len(chosen)}/{expected}')
    return sorted(chosen.values(),key=lambda t:(t['north'],t['east']))


def download(tile):
    path=RAW/tile['file']
    if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest()!=tile['sha256']:
        with urllib.request.urlopen(tile['url'],timeout=120) as response: data=response.read()
        if hashlib.sha256(data).hexdigest()!=tile['sha256']: raise ValueError(f'Checksum mismatch: {path}')
        path.write_bytes(data)
    return tile


def main():
    RAW.mkdir(parents=True,exist_ok=True); DEST.mkdir(parents=True,exist_ok=True)
    catalog=RAW/'catalog.json'
    tiles=json.loads(catalog.read_text()) if catalog.exists() else discover()
    catalog.write_text(json.dumps(tiles,indent=2))
    west,south,east,north=BOUNDS
    grid=np.full(((north-south)//SPACING,(east-west)//SPACING),NODATA,dtype='<f4')
    with ThreadPoolExecutor(max_workers=4) as executor:
        for i,tile in enumerate(executor.map(download,tiles)):
            with rasterio.open(RAW/tile['file']) as source:
                if source.crs.to_epsg()!=2056 or source.res!=(2,2): raise ValueError('Unexpected CRS/resolution')
                if source.bounds!=(tile['east'],tile['north'],tile['east']+1000,tile['north']+1000):
                    raise ValueError('Unexpected tile bounds')
                # Read native pixels: COG overview pyramids use their own sampling
                # and cannot be assumed to contain true 4x4 area averages.
                native=source.read(1,masked=True)
                if native.shape!=(500,500): raise ValueError('Unexpected native tile shape')
                values=native.reshape(125,4,125,4).mean(axis=(1,3),dtype=np.float64)
                values=(values-419).filled(NODATA).astype('<f4')
                row=(north-tile['north']-1000)//SPACING; col=(tile['east']-west)//SPACING
                grid[row:row+125,col:col+125]=values
            if (i+1)%20==0: print(f'Processed {i+1}/{len(tiles)} verified tiles',flush=True)
    if np.any(grid==NODATA): raise ValueError('Missing cells inside the requested landscape extent')
    output=DEST/'landscape.f32'; output.write_bytes(grid.tobytes())
    manifest=dict(schema=1,attribution='© swisstopo, swissALTI3D',horizontal_crs='EPSG:2056',
        vertical_crs='EPSG:5728',water_elevation_m=419,west=west,south=south,east=east,north=north,
        spacing_m=SPACING,rows=grid.shape[0],columns=grid.shape[1],file=output.name,
        encoding='little-endian float32 metres relative to 419 m LN02, north-to-south cell centres',
        resampling='Area average from 2 m source to 8 m runtime grid; distant mesh uses 32 m spacing',
        sha256=hashlib.sha256(output.read_bytes()).hexdigest(),tiles=tiles,
        licence='https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste')
    (DEST/'landscape.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    (DEST/'LANDSCAPE_NOTICE.txt').write_text(
        'Surrounding terrain: © swisstopo, swissALTI3D. Source tile editions in landscape.json.\n'
        'https://www.swisstopo.admin.ch/en/height-model-swissalti3d\n'
        'https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste\n'
        '2 m source terrain, averaged to 8 m; distant display mesh at 32 m.\n'
        'LV95 / LN02; fixed scenario water reference 419 m LN02.\n'
        'Vegetation/rock colours are illustrative, not surveyed land-cover data.\n',encoding='utf-8')
    print(f'Landscape ready: {grid.shape}, {grid.min()+419:.2f}..{grid.max()+419:.2f} m LN02',flush=True)


if __name__=='__main__': main()
