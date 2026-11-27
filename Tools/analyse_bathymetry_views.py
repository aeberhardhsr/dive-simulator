"""Select reproducible survey-based camera candidates, without changing the game."""
import hashlib
import json
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'divingsim/Content/Diving/Geodata'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    manifest_path = DATA / 'manifest.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    sources = {}
    source_tiles = {}
    max_encoding_error = 0.0
    checked_samples = 0
    results = []
    for entry in manifest['entries']:
        # Cell centres match HeightGrid.h. A 10 m margin covers every stencil.
        east = entry['east'] + np.arange(-260, 261)
        north = entry['north'] + np.arange(-260, 261)
        heights = np.full((north.size, east.size), np.nan)
        for tile in manifest['tiles']:
            cols, rows = int(tile['ncols']), int(tile['nrows'])
            west, south = tile['xllcorner'], tile['yllcorner']
            if tile['cellsize'] != 1 or west % 1 or south % 1:
                raise ValueError('Expected the runtime integer-metre lattice')
            ix = np.flatnonzero((east >= west) & (east < west + cols))
            iy = np.flatnonzero((north >= south) & (north < south + rows))
            if not ix.size or not iy.size:
                continue
            path = DATA / tile['file']
            if path.stat().st_size != rows * cols * 2:
                raise ValueError(f'Wrong tile size: {path}')
            sources[tile['file']] = digest(path)
            grid = np.memmap(path, dtype='<i2', mode='r', shape=(rows, cols))
            raw = grid[np.ix_(rows - 1 - np.floor(north[iy] - south).astype(int),
                              np.floor(east[ix] - west).astype(int))]
            sample = np.where(raw == manifest['nodata'], np.nan, raw * .01)
            original_path = ROOT / 'Geodata/Processed/Walensee' / path.with_suffix('.f32').name
            original = np.memmap(original_path, dtype='<f4', mode='r', shape=(rows, cols))
            reference = original[np.ix_(rows - 1 - np.floor(north[iy] - south).astype(int),
                                         np.floor(east[ix] - west).astype(int))].astype(float)
            measured = reference != -9999
            if not np.array_equal(measured, np.isfinite(sample)):
                raise ValueError(f'NoData mismatch: {path}')
            if measured.any():
                error = float(np.max(np.abs(sample[measured] -
                              (reference[measured] - manifest['water_elevation_m']))))
                max_encoding_error = max(max_encoding_error, error)
                checked_samples += int(measured.sum())
                if error > .005001:
                    raise ValueError(f'Centimetre encoding mismatch: {path}')
            source_tiles[original_path.name] = digest(original_path)
            del original
            heights[np.ix_(iy, ix)] = sample
            del grid
        # All derivatives use a 10 m baseline; 25 samples span a 20 x 20 m square.
        centre = heights[10:-10, 10:-10]
        de = (heights[10:-10, 15:-5] - heights[10:-10, 5:-15]) / 10
        dn = (heights[15:-5, 10:-10] - heights[5:-15, 10:-10]) / 10
        samples = np.stack([heights[10+dy:511+dy, 10+dx:511+dx]
                            for dy in (-10, -5, 0, 5, 10)
                            for dx in (-10, -5, 0, 5, 10)])
        relief = samples.max(axis=0) - samples.min(axis=0)
        slope = np.degrees(np.arctan(np.hypot(de, dn)))
        ee, nn = np.meshgrid(east[10:-10], north[10:-10])
        distance = np.hypot(ee - entry['east'], nn - entry['north'])
        valid = (np.isfinite(samples).all(axis=0) & np.isfinite(slope)
                 & (distance <= 250) & (-centre >= 5) & (-centre <= 35))
        # Artistic inspection priority, not a geological classification or route.
        score = np.where(valid, slope + .5 * relief - .04 * distance, -np.inf)
        candidates = []
        for _ in range(3):
            index = np.unravel_index(np.argmax(score), score.shape)
            if not np.isfinite(score[index]):
                break
            e, n, z = float(ee[index]), float(nn[index]), float(centre[index])
            # UE +X = north, +Y = east; point the camera down the measured slope.
            yaw = float(np.degrees(np.arctan2(-de[index], -dn[index])))
            candidates.append({
                'east': e, 'north': n, 'bed_depth_m': round(-z, 2),
                'entry_horizontal_distance_m': round(float(distance[index]), 2),
                'slope_10m_degrees': round(float(slope[index]), 2),
                'sampled_relief_20m_m': round(float(relief[index]), 2),
                'camera_world_cm': [round((n-manifest['origin_north'])*100, 2),
                                    round((e-manifest['origin_east'])*100, 2),
                                    round((z+2)*100, 2)],
                'camera_yaw_degrees': round(yaw, 2),
                'camera_pitch_degrees': -10,
            })
            score[np.hypot(ee-e, nn-n) < 40] = -np.inf
        results.append({'entry': entry, 'eligible_cells': int(valid.sum()),
                        'candidates': candidates})
    report = {
        'status': 'OFFLINE_CANDIDATES_NOT_RUNTIME_VALIDATED',
        'manifest_sha256': digest(manifest_path), 'tile_sha256': sources,
        'source_float_tile_sha256': source_tiles,
        'source_validation': {'checked_local_samples': checked_samples,
                              'max_height_encoding_error_m': max_encoding_error,
                              'nodata_equal': True},
        'water_reference_m_LN02': manifest['water_elevation_m'],
        'method': {'radius_m': 250, 'bed_depth_band_m': [5, 35],
                   'slope_baseline_m': 10, 'relief_samples': 25,
                   'relief_sample_spacing_m': 5, 'candidate_separation_m': 40,
                   'score': 'slope_degrees + 0.5 * sampled_relief_m - 0.04 * horizontal_distance_m'},
        'limitations': [
            'No terrain, habitat, collision, entry or camera setting has been changed.',
            'Depths use scenario water reference, not current lake level.',
            'Relief is the range of 25 samples, not every cell in the square.',
            'NoData is excluded, never filled; unsampled gaps can remain between stencil points.',
            'Horizontal distance does not establish an accessible swimming route.',
            'Manifest entry markers are offshore game references, not surveyed public shore access points.',
            'Height data does not identify rock, sediment or vegetation.',
            'Camera is nominally 2 m above its centre bed sample; collision and view require native inspection.',
        ],
        'entries': results,
    }
    output = ROOT / 'Documentation/BATHYMETRY_VIEW_CANDIDATES.json'
    output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    for result in results:
        print(result['entry']['name'], result['eligible_cells'])
        for candidate in result['candidates']:
            print(json.dumps(candidate))


if __name__ == '__main__':
    main()
