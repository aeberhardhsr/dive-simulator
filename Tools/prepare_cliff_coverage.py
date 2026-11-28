"""Decode mapped rock bits into filterable coverage before Unreal mip generation."""
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    manifest = ROOT / 'Documentation/SURFACE_COVER_MASK.json'
    source = json.loads(manifest.read_text(encoding='utf-8'))
    if (source['crs'] != 'EPSG:2056' or not source['north_up'] or
            source['bounds'] != [2723000, 1215000, 2745000, 1228000] or
            source['pixel_mismatches'] != 0):
        raise ValueError('Surface source registration/audit mismatch')
    item = next(x for x in source['outputs'] if x['file'].endswith('SurfaceCoverMask.png'))
    path = ROOT / item['file']
    if digest(path) != item['sha256']:
        raise ValueError('Categorical source changed')
    with Image.open(path) as image:
        if image.mode != 'L' or image.size != (4096, 4096):
            raise ValueError('Unexpected categorical source format')
        bits = np.asarray(image)
    # R/G are independent categories, not a normalized material mix. Overlaps
    # retain both bits. B is zero and A opaque, neither is a classification.
    rgba = np.zeros((*bits.shape, 4), dtype=np.uint8)
    rgba[..., 3] = 255
    channels = {}
    for channel, category in enumerate(('Fels', 'Fels locker')):
        selected = (bits & source['bits'][category]) != 0
        if int(selected.sum()) != source['class_pixels'][category]:
            raise ValueError('Category coverage differs from independent source audit')
        rgba[..., channel] = selected.astype(np.uint8) * 255
        channels['RG'[channel]] = {'source_class': category, 'source_bit': source['bits'][category],
                                  'nonzero_pixels': int(selected.sum())}
    if np.any(rgba[bits == 0, :2]):
        raise ValueError('Unclassified source acquired a rock category')
    output = ROOT / 'Art/GroundMaterials/CliffCoverage.png'
    Image.fromarray(rgba).save(output)
    with Image.open(output) as saved:
        if saved.mode != 'RGBA' or not np.array_equal(np.asarray(saved), rgba):
            raise ValueError('Saved decoded coverage differs from source categories')
    report = {'schema': 1, 'status': 'DECODED_COVERAGE_VALIDATED', 'crs': source['crs'],
              'bounds': source['bounds'], 'north_up': True, 'width': 4096, 'height': 4096,
              'channels': channels, 'constant_channels': {'B': 0, 'A': 255},
              'source_manifest': {'file': manifest.relative_to(ROOT).as_posix(), 'sha256': digest(manifest)},
              'source': item, 'output': {'file': output.relative_to(ROOT).as_posix(), 'sha256': digest(output)},
              'runtime_contract': 'Linear uncompressed RGBA8; bilinear/trilinear filtering and average mipmaps operate on decoded coverage, never categorical bit values.',
              'limits': 'Mapped footprint, not surveyed rock colour or three-dimensional cliff extent. Loose rock coverage strength is authored. Zero remains unclassified; other cover classes are not reclassified as rock.'}
    (ROOT / 'Documentation/CLIFF_COVERAGE.json').write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print('CLIFF_COVERAGE_PASS', channels)


if __name__ == '__main__':
    main()
