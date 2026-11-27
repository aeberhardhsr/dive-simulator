"""Stage the validated R16F lake outline and independent GPU audit positions."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

import numpy as np
import shapely
from shapely.geometry import Polygon, shape

from lake_footprint import sample

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe-directory', type=Path, default=ROOT/'artifacts/lake-footprint-gpu')
    args = parser.parse_args()
    folder = ROOT/'Geodata/Processed/WaterFootprint'
    grid = json.loads((folder/'manifest.json').read_text(encoding='utf-8'))
    source = folder/grid['file']
    expected = '8dcfa5bc8561669a08819d7b6325623e6218ee616e119368978fdf2a3dc7769d'
    assert digest(source) == grid['sha256'] == expected
    assert [grid[k] for k in ['west', 'north', 'spacing_m', 'columns', 'rows', 'world_origin_east', 'world_origin_north']] == [2725871,1222350,1,15586,3003,2726000,1219000]
    raw = ROOT/'Geodata/Raw/WaterBoundary/walensee_polygon.geojson'
    assert digest(raw) == grid['source_sha256']
    assert digest(ROOT/'divingsim/Content/Diving/Geodata/manifest.json') == grid['world_origin_manifest_sha256']
    lake = shape(json.loads(raw.read_text(encoding='utf-8'))['geometry'])
    data = np.memmap(source, mode='r', dtype='<f2', shape=(grid['rows'],grid['columns']))
    groups = {}
    rng = np.random.default_rng(20261127)
    cols = rng.integers(0,grid['columns'],5000)
    rows = rng.integers(0,grid['rows'],5000)
    groups['pixel_centres'] = np.column_stack((grid['west']+cols+.5,grid['north']-rows-.5))
    for i, ring in enumerate([lake.exterior,*lake.interiors]):
        positions = np.linspace(0,ring.length,max(128,int(np.ceil(ring.length/8))),endpoint=False)
        centre = shapely.get_coordinates(shapely.line_interpolate_point(ring,positions))
        tangent = shapely.get_coordinates(shapely.line_interpolate_point(ring,(positions+.1)%ring.length)) - shapely.get_coordinates(shapely.line_interpolate_point(ring,(positions-.1)%ring.length))
        normals = np.column_stack((-tangent[:,1],tangent[:,0]))
        normals /= np.linalg.norm(normals,axis=1)[:,None]
        groups[f'shore_{i}'] = np.concatenate([centre+normals*offset for offset in [-2,-.8,-.3,0,.3,.8,2]])
    groups['island_interiors'] = np.array([[p.x,p.y] for p in [Polygon(r).representative_point() for r in lake.interiors]])
    groups['outside_grid'] = np.array([[grid['west']-10,grid['north']], [grid['west'],grid['north']+10],
        [grid['west']+grid['columns']+10,grid['north']], [grid['west'],grid['north']-grid['rows']-10]])
    rows_out=[]; ranges={}; offset=0
    for name, coordinates in groups.items():
        local = np.column_stack((coordinates[:,1]-grid['world_origin_north'],coordinates[:,0]-grid['world_origin_east'])).astype('<f4')
        # Reference the exact float32 positions delivered to the GPU, in double precision.
        east=local[:,1].astype(float)+grid['world_origin_east']; north=local[:,0].astype(float)+grid['world_origin_north']
        distance=sample(grid,data,east,north)
        t=np.clip((distance+.75)/1.5,0,1); wet=t*t*(3-2*t)
        rows_out.append(np.column_stack((local,distance,wet)).astype('<f4'))
        ranges[name]={'start':offset,'count':len(local)}; offset+=len(local)
    destination=ROOT/'divingsim/Content/Diving/Geodata'
    shutil.copyfile(source,destination/'walensee-distance.f16')
    shutil.copyfile(folder/'manifest.json',destination/'water_footprint.json')
    args.probe_directory.mkdir(parents=True,exist_ok=True)
    query=args.probe_directory/'Probes.f32'
    np.concatenate(rows_out).tofile(query)
    report={'raster_sha256':expected,'probe_sha256':digest(query),'count':offset,'groups':ranges,
        'format':'float32 little endian rows: local north metres, local east metres, CPU bilinear distance metres, wet weight',
        'gpu_distance_tolerance_m':.025,'gpu_wet_tolerance':.025,'grid':grid,
        'tool_sha256':digest(Path(__file__)),'sampling_tool_sha256':digest(ROOT/'Tools/lake_footprint.py')}
    (args.probe_directory/'Probes.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(f'Staged {source.stat().st_size} bytes; {offset} GPU audit positions in {query}')


if __name__ == '__main__':
    main()
