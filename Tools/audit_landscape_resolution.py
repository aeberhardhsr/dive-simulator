"""Compare the rendered 8 m triangle surface with verified native 2 m heights.

Read-only audit: does not replace terrain, meshes, collision or material data.
The 2 m DTM is a source reference, not a survey of overhangs or vertical faces.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def statistics(values):
    absolute = np.abs(values)
    return {'samples': int(values.size), 'signed_mean_m': float(values.mean()),
            'absolute_m': {key: float(np.quantile(absolute, q))
                           for key, q in [('p50', .5), ('p95', .95), ('p99', .99), ('max', 1)]}}


def triangle_height(grid, east, north, manifest):
    """Actual baker diagonal: (a, south, east), (east, south, southeast)."""
    x = (east - manifest['west']) / 8 - .5
    y = (manifest['north'] - north) / 8 - .5
    col = np.floor(x).astype(int); row = np.floor(y).astype(int)
    u = x - col; v = y - row
    valid = (col >= 0) & (row >= 0) & (col + 1 < grid.shape[1]) & (row + 1 < grid.shape[0])
    col = col.clip(0, grid.shape[1]-2); row = row.clip(0, grid.shape[0]-2)
    a = grid[row, col]; b = grid[row, col+1]
    c = grid[row+1, col]; d = grid[row+1, col+1]
    height = np.where(u+v <= 1, a+u*(b-a)+v*(c-a), d+(1-u)*(c-d)+(1-v)*(b-d))
    return height, valid


def main():
    import rasterio
    from PIL import Image
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=ROOT/'artifacts/landscape-resolution-audit.json')
    args = parser.parse_args()
    base = ROOT/'divingsim/Content/Diving/Geodata'
    manifest_path = base/'landscape.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    if manifest['spacing_m'] != 8 or manifest['horizontal_crs'] != 'EPSG:2056':
        raise ValueError('Unexpected runtime registration')
    runtime = base/manifest['file']
    if sha(runtime) != manifest['sha256']:
        raise ValueError('Runtime terrain changed')
    grid = np.fromfile(runtime, dtype='<f4').reshape(manifest['rows'], manifest['columns'])
    if not np.isfinite(grid).all():
        raise ValueError('Nonfinite runtime height')
    cover_manifest = json.loads((ROOT/'Documentation/CLIFF_COVERAGE.json').read_text(encoding='utf-8'))
    cover_path = ROOT/cover_manifest['output']['file']
    if sha(cover_path) != cover_manifest['output']['sha256']:
        raise ValueError('Coverage changed')
    with Image.open(cover_path) as image:
        cover = np.asarray(image)[..., :2]
    west, south, east_bound, north_bound = cover_manifest['bounds']
    groups = {name: [] for name in ('all_land', 'mapped_rock', 'northern_lakeside_rock')}
    tiles = []
    for item in manifest['tiles']:
        path = ROOT/'Geodata/Raw/Landscape'/item['file']
        if sha(path) != item['sha256']:
            raise ValueError('Native tile checksum mismatch: '+item['file'])
        with rasterio.open(path) as source:
            if source.crs.to_epsg() != 2056 or source.res != (2, 2) or tuple(source.bounds) != (
                    item['east'], item['north'], item['east']+1000, item['north']+1000):
                raise ValueError('Native registration mismatch')
            native = source.read(1, masked=True)
            if np.any(np.ma.getmaskarray(native)) or native.shape != (500, 500):
                raise ValueError('Incomplete native tile')
            native = np.asarray(native, dtype=np.float64) - manifest['water_elevation_m']
            if not np.isfinite(native).all():
                raise ValueError('Nonfinite native height')
        east = np.broadcast_to(item['east'] + 1 + np.arange(500)[None, :]*2, (500, 500))
        north = np.broadcast_to(item['north'] + 999 - np.arange(500)[:, None]*2, (500, 500))
        rendered, valid = triangle_height(grid, east, north, manifest)
        error = rendered-native
        valid &= (native > 2) & np.isfinite(native)
        cr = np.floor((north_bound-north)/(north_bound-south)*cover.shape[0]).astype(int)
        cc = np.floor((east-west)/(east_bound-west)*cover.shape[1]).astype(int)
        rock = valid & np.any(cover[cr, cc] > 127, axis=-1)
        # Explicit geographic diagnostic window covering the northern shore
        # opposite Murg/Mols. It is not a camera visibility/occlusion test.
        northern = rock & (east >= 2730000) & (east < 2739000) & (north >= 1220000) & (north < 1225000)
        for name, selection in [('all_land', valid), ('mapped_rock', rock), ('northern_lakeside_rock', northern)]:
            groups[name].append(error[selection].astype(np.float32))
        if np.any(rock):
            candidate = np.where(rock, np.abs(error), -1)
            row, col = np.unravel_index(np.argmax(candidate), candidate.shape)
            tiles.append({'file': item['file'], **statistics(error[rock]),
                          'largest_difference': {'east': int(east[row,col]), 'north': int(north[row,col]),
                             'native_m_ln02': float(native[row,col]+manifest['water_elevation_m']),
                             'runtime_triangle_m_ln02': float(rendered[row,col]+manifest['water_elevation_m']),
                             'runtime_minus_native_m': float(error[row,col])}})
    report = {'status': 'AUDIT_COMPLETE_NO_GEOMETRY_CHANGE', 'native_tiles_verified': len(manifest['tiles']),
              'runtime': {'file': runtime.relative_to(ROOT).as_posix(), 'sha256': sha(runtime)},
              'source_manifest': {'file': manifest_path.relative_to(ROOT).as_posix(), 'sha256': sha(manifest_path)},
              'coverage': cover_manifest['output'],
              'groups': {name: statistics(np.concatenate(parts)) for name, parts in groups.items()},
              'largest_rock_tile_differences': sorted(tiles, key=lambda x:x['absolute_m']['max'], reverse=True)[:15],
              'method': 'All native 2m pixel centres, land >421m LN02; exact 8m mesh triangle interpolation (not bilinear), excluding outer incomplete cells. Mapped solid OR loose rock from decoded coverage.',
              'limits': ['Vertical elevation differences, not perpendicular surface distances or screen-space visual error.',
                         'Native 2m DTM also cannot represent overhangs; not an assertion of survey accuracy.',
                         'Audit of the landscape source surface, not Nanite-selected runtime triangles, shaders, coast patches or collision.',
                         'Northern-lakeside group is a geographic window, not a visibility test.']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(json.dumps(report['groups'], indent=2))
    print('LANDSCAPE_RESOLUTION_AUDIT', args.output)


if __name__ == '__main__':
    main()
