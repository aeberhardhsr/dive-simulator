"""Prepare mapped surface classes for future terrain materials; no UE import.

Each category is an independent bit. Overlaps and polygon holes survive; zero
means none of these mapped classes, never an inferred grass/road classification.
"""
from collections import Counter
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image
import shapefile
from rasterio.features import rasterize
from rasterio.transform import from_bounds
from shapely import STRtree, force_2d, points
from shapely.geometry import box, mapping, shape

ROOT = Path(__file__).resolve().parents[1]
BASE = 'swissTLM3D_TLM_BODENBEDECKUNG_OST'
CLASSES = {'Fels': 1, 'Fels locker': 2, 'Lockergestein': 4,
           'Lockergestein locker': 8, 'Felsbloecke': 16,
           'Felsbloecke locker': 32, 'Feuchtgebiet': 64}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    provenance = json.loads((ROOT / 'Documentation/WALENSEE_VEGETATION_SOURCE.json').read_text(encoding='utf-8'))
    raw = ROOT / 'Geodata/Raw/Vegetation'
    inputs = []
    for suffix in ('CPG', 'prj', 'shx', 'dbf', 'shp'):
        name = BASE + '.' + suffix
        expected = next(item for item in provenance['files'] if item['file'] == name)
        path = raw / name
        if path.stat().st_size != expected['size'] or digest(path) != expected['sha256']:
            raise ValueError('Pinned surface source differs: ' + name)
        inputs.append({'file': path.relative_to(ROOT).as_posix(), 'sha256': expected['sha256'], 'bytes': expected['size']})
    bounds = provenance['bounds']
    if bounds != [2723000, 1215000, 2745000, 1228000]:
        raise ValueError('Unexpected landscape registration')
    region = box(*bounds)
    features, geometries, bits = [], [], []
    candidates, selected, holes = Counter(), Counter(), Counter()
    with shapefile.Reader(str(raw / BASE), encoding='utf-8') as reader:
        for item in reader.iterShapeRecords(bbox=bounds):
            record = item.record.as_dict()
            category = record['OBJEKTART']
            candidates[category] += 1
            if category not in CLASSES:
                continue
            original = force_2d(shape(item.shape.__geo_interface__))
            if not original.is_valid:
                raise ValueError('Invalid source geometry; no automatic repair: ' + record['UUID'])
            clipped = original.intersection(region)
            if clipped.is_empty or clipped.area == 0:
                continue
            if clipped.geom_type not in ('Polygon', 'MultiPolygon') or not clipped.is_valid:
                raise ValueError('Unexpected clipped topology: ' + record['UUID'])
            parts = [clipped] if clipped.geom_type == 'Polygon' else list(clipped.geoms)
            holes[category] += sum(len(part.interiors) for part in parts)
            selected[category] += 1
            geometries.append(clipped)
            bits.append(CLASSES[category])
            features.append({'type': 'Feature', 'properties': {'source_uuid': record['UUID'],
                             'source_class': category, 'bit': CLASSES[category]}, 'geometry': mapping(clipped)})
    if not features or set(selected) != set(CLASSES):
        raise ValueError('Expected mapped classes are missing')
    width = height = 4096
    transform = from_bounds(*bounds, width, height)
    mask = np.zeros((height, width), dtype=np.uint8)
    for bit in CLASSES.values():
        layer = rasterize(((g, bit) for g, value in zip(geometries, bits) if value == bit),
                          out_shape=mask.shape, transform=transform, fill=0, all_touched=False, dtype='uint8')
        mask |= layer
    # Different algorithms: rasterizer versus analytic polygon containment at
    # 30,000 deterministic pixel centres, including overlaps as separate bits.
    rng = np.random.default_rng(271126)
    rows = rng.integers(0, height, 30000)
    cols = rng.integers(0, width, 30000)
    east, north = transform * (cols + .5, rows + .5)
    pairs = STRtree(geometries).query(points(east, north), predicate='within')
    expected = np.zeros(len(rows), dtype=np.uint8)
    np.bitwise_or.at(expected, pairs[0], np.asarray(bits, dtype=np.uint8)[pairs[1]])
    if not np.array_equal(expected, mask[rows, cols]):
        raise ValueError('Raster classes differ from polygon containment')

    destination = ROOT / 'Geodata/SurfaceCoverSource/cover.geojson'
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps({'type': 'FeatureCollection', 'crs': {
        'type': 'name', 'properties': {'name': 'urn:ogc:def:crs:EPSG::2056'}}, 'features': features},
        ensure_ascii=False, separators=(',', ':')) + '\n', encoding='utf-8')
    texture = ROOT / 'Art/GroundMaterials/SurfaceCoverMask.png'
    Image.fromarray(mask).save(texture)
    with Image.open(texture) as image:
        if image.mode != 'L' or not np.array_equal(np.asarray(image), mask):
            raise ValueError('Saved categorical PNG differs from raster')
    bit_counts = np.array([int(i).bit_count() for i in range(256)], dtype=np.uint8)
    report = {'schema': 1, 'status': 'OFFLINE_VALIDATED_NOT_RUNTIME_INTEGRATED',
              'source_url': provenance['url'], 'source_attribution': 'swisstopo',
              'source_manifest': 'Documentation/WALENSEE_VEGETATION_SOURCE.json',
              'crs': 'EPSG:2056', 'bounds': bounds, 'north_up': True,
              'width': width, 'height': height, 'pixel_metres': [(bounds[2]-bounds[0])/width, (bounds[3]-bounds[1])/height],
              'bits': CLASSES, 'bbox_candidates': dict(candidates), 'selected_features': dict(selected),
              'interior_rings': dict(holes), 'unclassified_pixels': int(np.count_nonzero(mask == 0)),
              'overlap_pixels': int(np.count_nonzero(bit_counts[mask] > 1)),
              'class_pixels': {name: int(np.count_nonzero(mask & bit)) for name, bit in CLASSES.items()},
              'independent_pixel_checks': len(rows), 'pixel_mismatches': 0,
              'inputs': inputs,
              'outputs': [{'file': p.relative_to(ROOT).as_posix(), 'sha256': digest(p), 'bytes': p.stat().st_size}
                          for p in (destination, texture)],
              'runtime_contract': 'Unorm8 bitfield: point sample, no sRGB, no lossy compression, no averaged mipmaps. Decode bits before any authored spatial blend.',
              'limits': 'Mapped cover categories, not measured substrate colours or species. Loose classes are not continuous bare ground. Zero is unclassified, not grass. No terrain, collision, material or content asset changed.'}
    (ROOT / 'Documentation/SURFACE_COVER_MASK.json').write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(json.dumps({key: report[key] for key in ('status', 'selected_features', 'interior_rings',
          'overlap_pixels', 'independent_pixel_checks', 'pixel_mismatches', 'outputs')}, ensure_ascii=False), flush=True)


if __name__ == '__main__':
    main()
