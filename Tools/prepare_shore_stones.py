"""Author a small Weesen stone-bank study from the pinned north-up orthophoto.

The traced bank is an artistic interpretation, not surveyed stone geometry.
Individual dimensions, offsets and orientations are deterministic authored detail.
"""
import hashlib
import json
import math
from pathlib import Path
import random
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def main():
    manifest = json.loads((ROOT / 'divingsim/Content/Diving/Geodata/imagery.json').read_text())
    source = next(x for x in manifest['images'] if x['id'] == 'weesen_strandbad')
    path = ROOT / 'Geodata/Processed/Imagery' / source['file']
    assert hashlib.sha256(path.read_bytes()).hexdigest() == source['sha256']
    # Pixel coordinates in the original 2048 x 2048 image (pixel edges).
    # Follow the visible bank, including the small curved inlet west of entry.
    trace = [(850, 880), (900, 890), (940, 890), (973, 885),
             (981, 855), (998, 843), (1019, 840), (1040, 840),
             (1061, 849), (1079, 867), (1086, 896), (1101, 895),
             (1130, 882), (1160, 871), (1190, 860), (1220, 845),
             (1250, 825), (1280, 812), (1319, 804)]
    spacing = (source['east'] - source['west']) / source['width']
    control = [(source['west'] + x * spacing, source['north'] - y * spacing) for x, y in trace]
    rng = random.Random(281128)
    stones = []
    carry = 0.
    for a, b in zip(control, control[1:]):
        dx, dy = b[0] - a[0], b[1] - a[1]
        length = math.hypot(dx, dy)
        along = carry
        while along < length:
            for row in (-1.5, -.5, .5, 1.5):
                across = row * .9 + rng.uniform(-.15, .15)
                east = a[0] + dx * along / length - dy * across / length
                north = a[1] + dy * along / length + dx * across / length
                stones.append(dict(east=round(east, 4), north=round(north, 4),
                    width_m=round(rng.uniform(.95, 1.35), 3),
                    length_m=round(rng.uniform(1.05, 1.45), 3),
                    height_m=round(rng.uniform(.55, .85), 3),
                    yaw=round(rng.uniform(0, 360), 3), variant=rng.randrange(1, 4)))
            along += rng.uniform(.70, .90)
        carry = along - length
    # A single linear coverage channel. Zero outside this authored bank; it
    # must never be interpreted as a surveyed land-cover classification.
    size = 1024
    y, x = np.mgrid[:size, :size]
    east = source['west'] + (x + .5) * 384 / size
    north = source['north'] - (y + .5) * 384 / size
    squared = np.full((size, size), np.inf)
    for a, b in zip(control, control[1:]):
        dx, dy = b[0] - a[0], b[1] - a[1]
        t = np.clip(((east-a[0])*dx + (north-a[1])*dy)/(dx*dx+dy*dy), 0, 1)
        squared = np.minimum(squared, (east-a[0]-dx*t)**2 + (north-a[1]-dy*t)**2)
    t = np.clip((np.sqrt(squared)-2.3)/1.2, 0, 1)
    pixels = np.rint(255*(1-t*t*(3-2*t))).astype(np.uint8)
    mask = ROOT / 'Art/GroundMaterials/WeesenBankMask.png'
    Image.fromarray(pixels).save(mask)
    result = dict(schema=1, crs='EPSG:2056', site='weesen_strandbad',
        provenance='Authored interpretation of visible bank; individual stones are not surveyed.',
        attribution=manifest['attribution'], source_image_sha256=source['sha256'],
        source_image=path.relative_to(ROOT).as_posix(), source_bounds=[source[k] for k in ('west','south','east','north')],
        source_size=[source['width'], source['height']], trace_pixels=trace,
        placement='Runtime terrain anchoring; reject wet footprints and access corridor. Ground grids remain unchanged.',
        mask=dict(file=mask.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(mask.read_bytes()).hexdigest(),
                  width=size, height=size, full_width_m=4.6, feather_m=1.2), stones=stones)
    output = ROOT / 'divingsim/Content/Diving/Geodata/shore_stones.json'
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'AUTHORED_SHORE_STONES count={len(stones)} file={output}')


if __name__ == '__main__':
    main()
