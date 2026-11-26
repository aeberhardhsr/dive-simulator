"""Rasterize verified woodland polygons, including their interior rings."""
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image
from rasterio.features import rasterize
from rasterio.transform import from_bounds
from shapely import STRtree, points
from shapely.geometry import shape

ROOT = Path(__file__).resolve().parents[1]


def main():
    source = ROOT / 'Geodata/VegetationSource/forest.geojson'
    provenance = json.loads((ROOT / 'Documentation/WALENSEE_VEGETATION_SOURCE.json').read_text(encoding='utf-8'))
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    expected = next(x['sha256'] for x in provenance['outputs'] if x['file'].endswith('/forest.geojson'))
    if digest != expected:
        raise ValueError('Woodland source checksum mismatch')
    features = json.loads(source.read_text(encoding='utf-8'))['features']
    bounds = provenance['bounds']
    width = height = 4096
    transform = from_bounds(*bounds, width, height)
    geometries = [shape(f['geometry']) for f in features]
    mask = rasterize(((g, 255) for g in geometries), out_shape=(height, width),
                     transform=transform, fill=0, all_touched=False, dtype='uint8')
    # Independently verify north-up pixel centers against the original topology.
    rng = np.random.default_rng(419)
    cols = rng.integers(0, width, 20000)
    rows = rng.integers(0, height, 20000)
    e, n = transform * (cols + .5, rows + .5)
    pairs = STRtree(geometries).query(points(e, n), predicate='within')
    expected_mask = np.zeros(len(cols), dtype=bool)
    expected_mask[pairs[0]] = True
    if not np.array_equal(mask[rows, cols] > 0, expected_mask):
        raise ValueError('Raster mask differs from independent point-in-polygon audit')
    output = ROOT / 'Art/GroundMaterials/WoodlandMask.png'
    output.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray(mask).save(output)
    report = {'schema': 1, 'crs': 'EPSG:2056', 'bounds': bounds,
        'width': width, 'height': height, 'north_up': True,
        'file': output.relative_to(ROOT).as_posix(), 'source_sha256': digest,
        'sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
        'features': len(features), 'independent_pixel_center_checks': len(cols),
        'status': 'PASS', 'woodland_pixels': int(np.count_nonzero(mask)),
        'meaning': 'Mapped woodland coverage, not measured ground material; bilinear edge blending is authored.'}
    (ROOT / 'Documentation/GROUND_MASK.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
