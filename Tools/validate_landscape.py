"""Audit the packaged grid against native, checksum-verified swissALTI3D pixels."""
import hashlib
import json
from pathlib import Path
import numpy as np
import rasterio

ROOT=Path(__file__).resolve().parents[1]


def main():
    base=ROOT/'divingsim/Content/Diving/Geodata'
    manifest=json.loads((base/'landscape.json').read_text(encoding='utf-8'))
    blob=(base/manifest['file']).read_bytes()
    if hashlib.sha256(blob).hexdigest()!=manifest['sha256']:
        raise ValueError('Runtime raster SHA256 mismatch')
    grid=np.frombuffer(blob,dtype='<f4').reshape(manifest['rows'],manifest['columns'])
    max_error=0.; count=0
    for tile in manifest['tiles']:
        path=ROOT/'Geodata/Raw/Landscape'/tile['file']
        if hashlib.sha256(path.read_bytes()).hexdigest()!=tile['sha256']:
            raise ValueError('Source raster SHA256 mismatch: '+tile['file'])
        with rasterio.open(path) as source:
            native=source.read(1)
        row=(manifest['north']-tile['north']-1000)//8
        col=(tile['east']-manifest['west'])//8
        # Three spatially separated windows per source tile, including both edges.
        # Reading native pixels catches incorrectly sampled COG overview pyramids.
        for y,x in ((0,0),(62,62),(124,124)):
            expected=float(np.mean(native[y*4:y*4+4,x*4:x*4+4],dtype=np.float64))-419
            error=abs(expected-float(grid[row+y,col+x]))
            if not np.isfinite(error) or error>.001:
                raise ValueError(f'Resampling mismatch at {tile["file"]} cell {x},{y}: {error} m')
            max_error=max(max_error,error); count+=1
    report=dict(tiles=len(manifest['tiles']),samples_checked=count,max_error_m=max_error,
                source_years=sorted({t['year'] for t in manifest['tiles']}),runtime_sha256=manifest['sha256'])
    output=ROOT/'artifacts/landscape-integrity.json'
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))


if __name__=='__main__': main()
