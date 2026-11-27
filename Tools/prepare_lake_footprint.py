"""Prepare an ignored lake-distance prototype from the pinned Walensee polygon."""
import argparse
import hashlib
import json
import math
from pathlib import Path

import numpy as np
from shapely.geometry import shape

from lake_footprint import generate, layout

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            value.update(block)
    return value.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'Geodata/Processed/WaterFootprint')
    args = parser.parse_args()
    output = args.output.resolve()
    if not any(output.is_relative_to(ROOT/folder) for folder in ['Geodata/Processed', 'artifacts']):
        raise ValueError('Keep this unvalidated prototype inside ignored processed data or artifacts')
    source = ROOT/'Geodata/Raw/WaterBoundary/walensee_polygon.geojson'
    origin_path = ROOT/'divingsim/Content/Diving/Geodata/manifest.json'
    origin = json.loads(origin_path.read_text(encoding='utf-8'))
    if not all(math.isfinite(origin[key]) for key in ['origin_east', 'origin_north']):
        raise ValueError('Invalid world origin')
    provenance = json.loads((ROOT/'Documentation/WALENSEE_COAST_SOURCE.json').read_text(encoding='utf-8'))
    if digest(source) != provenance['polygon_sha256']:
        raise ValueError('Lake outline differs from the reviewed survey')
    lake = shape(json.loads(source.read_text(encoding='utf-8'))['geometry'])
    if len(lake.interiors) != 3 or abs(lake.area-provenance['area_m2']) > .001:
        raise ValueError('Unexpected lake geometry or islands')
    grid = layout(lake)
    output.mkdir(parents=True, exist_ok=True)
    target = output/'walensee-distance.f16'
    temporary = output/'walensee-distance.f16.tmp'
    values = np.memmap(temporary, mode='w+', dtype='<f2', shape=(grid['rows'], grid['columns']))
    def progress(done, total):
        if done % 64 == 0 or done == total:
            print(f'LAKE_FOOTPRINT_TILES {done}/{total}', flush=True)
    generate(lake, grid, values, progress=progress)
    values.flush()
    del values
    temporary.replace(target)
    manifest = dict(schema=1, status='offline_prototype_not_runtime_integrated', **grid,
        encoding='IEEE754 float16 little endian', positive='water', negative='land including islands',
        row_order='north to south', column_order='west to east', sampling='cell centres; bilinear reference',
        horizontal_crs='EPSG:2056', attribution=provenance['attribution'], licence=provenance['licence'],
        source_sha256=digest(source), source_edition='swissTLM3D 2026-02', islands=3,
        world_origin_east=origin['origin_east'], world_origin_north=origin['origin_north'],
        world_origin_manifest_sha256=digest(origin_path),
        world_mapping='Unreal X=north, Y=east; coordinates in centimetres',
        file=target.name, bytes=target.stat().st_size, sha256=digest(target))
    (output/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    print('LAKE_FOOTPRINT_PREPARED', output)


if __name__ == '__main__':
    main()
