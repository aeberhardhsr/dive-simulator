"""Validate the offline footprint against the complete surveyed vector outline."""
import json
from pathlib import Path

import numpy as np
import shapely
from shapely.geometry import Polygon, shape

from lake_footprint import sample
from prepare_lake_footprint import ROOT, digest


def reference(lake, points, limit):
    distance = np.minimum(shapely.distance(points, lake.boundary), limit)
    return np.where(shapely.contains(lake, points), distance, -distance)


def main():
    folder = ROOT/'Geodata/Processed/WaterFootprint'
    grid = json.loads((folder/'manifest.json').read_text(encoding='utf-8'))
    raw = ROOT/'Geodata/Raw/WaterBoundary/walensee_polygon.geojson'
    pinned = json.loads((ROOT/'Documentation/WALENSEE_COAST_SOURCE.json').read_text(encoding='utf-8'))
    if digest(raw) != grid['source_sha256'] or digest(raw) != pinned['polygon_sha256']:
        raise ValueError('Survey identity changed')
    if digest(ROOT/'divingsim/Content/Diving/Geodata/manifest.json') != grid['world_origin_manifest_sha256']:
        raise ValueError('Runtime coordinate manifest changed')
    lake = shape(json.loads(raw.read_text(encoding='utf-8'))['geometry'])
    data_path = folder/grid['file']
    if digest(data_path) != grid['sha256'] or data_path.stat().st_size != grid['rows']*grid['columns']*2:
        raise ValueError('Footprint identity or byte size changed')
    data = np.memmap(data_path, mode='r', dtype='<f2', shape=(grid['rows'], grid['columns']))
    water_cells = 0
    for row in range(0, grid['rows'], 64):
        block = data[row:row+64]
        if not np.isfinite(block).all() or np.max(np.abs(block)) > grid['distance_limit_m']:
            raise ValueError('Invalid signed distances')
        water_cells += int(np.count_nonzero(block > 0))
    rng = np.random.default_rng(20261127)
    cols = rng.integers(0, grid['columns'], 5000)
    rows = rng.integers(0, grid['rows'], 5000)
    x = grid['west']+(cols+.5)*grid['spacing_m']
    y = grid['north']-(rows+.5)*grid['spacing_m']
    actual = data[rows, cols].astype(float)
    expected = reference(lake, shapely.points(x, y), grid['distance_limit_m'])
    centre_error = np.abs(actual-expected)
    if centre_error.max() > .008:
        raise ValueError('Pixel centre distances exceed float16 quantization allowance')

    coordinates = []
    for ring in [lake.exterior, *lake.interiors]:
        length = ring.length
        positions = np.linspace(0, length, max(128, int(np.ceil(length/8))), endpoint=False)
        centre = shapely.get_coordinates(shapely.line_interpolate_point(ring, positions))
        before = shapely.get_coordinates(shapely.line_interpolate_point(ring, (positions-.1) % length))
        after = shapely.get_coordinates(shapely.line_interpolate_point(ring, (positions+.1) % length))
        tangent = after-before
        normals = np.stack([-tangent[:, 1], tangent[:, 0]], axis=1)
        normals /= np.linalg.norm(normals, axis=1)[:, None]
        for offset in [-2, -.8, -.3, 0, .3, .8, 2]:
            coordinates.append(centre+normals*offset)
    coordinates = np.concatenate(coordinates)
    actual = sample(grid, data, coordinates[:, 0], coordinates[:, 1])
    expected = reference(lake, shapely.points(coordinates), grid['distance_limit_m'])
    errors = np.abs(actual-expected)
    # A signed distance is 1-Lipschitz. At 1 m spacing, bilinear interpolation
    # has <= sqrt(0.5) m uncertainty, plus at most .008 m float16 quantization.
    error_bound = .75
    wrong_sign = (np.abs(expected) > error_bound) & ((actual > 0) != (expected > 0))
    if errors.max() > error_bound or wrong_sign.any():
        raise ValueError('Near-shore sampling exceeds the declared spatial uncertainty')
    islands = []
    for ring in lake.interiors:
        point = Polygon(ring).representative_point()
        distance = float(sample(grid, data, point.x, point.y))
        if distance >= 0:
            raise ValueError('An island was classified as water')
        islands.append(dict(east=point.x, north=point.y, sampled_distance_m=distance))
    report = dict(status='offline_distance_validation_passed_not_runtime_integrated', manifest=grid,
        generator_sha256=digest(ROOT/'Tools/prepare_lake_footprint.py'),
        algorithm_sha256=digest(ROOT/'Tools/lake_footprint.py'), reporter_sha256=digest(Path(__file__)),
        full_raster_finite_and_bounded=True, water_cells=water_cells,
        raster_water_area_m2=water_cells*grid['spacing_m']**2, survey_water_area_m2=lake.area,
        centre_samples=len(centre_error), centre_max_error_m=float(centre_error.max()),
        near_shore_samples=len(errors), near_shore_max_error_m=float(errors.max()),
        near_shore_p99_error_m=float(np.percentile(errors, 99)),
        declared_near_shore_uncertainty_m=error_bound, wrong_sign_outside_uncertainty=int(wrong_sign.sum()),
        island_probes=islands,
        limits=['Fixed horizontal outline, not a dynamic shoreline or flooding model.',
                'No GPU upload, runtime image comparison, or GPU memory/performance validation yet.',
                'Full-resolution uncompressed texture would occupy about 89.3 MiB before driver overhead.'])
    destination = ROOT/'Documentation/LAKE_FOOTPRINT_RESULTS.json'
    destination.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({k:report[k] for k in ['status','centre_max_error_m','near_shore_samples',
        'near_shore_max_error_m','near_shore_p99_error_m','wrong_sign_outside_uncertainty']}))


if __name__ == '__main__':
    main()
