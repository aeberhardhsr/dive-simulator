"""Check paired 1600x900 night smoke frames for torch response and unlit glow.

Use the geodata Python environment (numpy/rasterio). This is a rendered-pixel
regression check, not a photometric measurement or a realism acceptance test.
"""
import argparse
import json
import warnings
import numpy as np
import rasterio
from rasterio.errors import NotGeoreferencedWarning


def scene(path):
    with warnings.catch_warnings():
        warnings.simplefilter('ignore',NotGeoreferencedWarning)
        with rasterio.open(path) as image:
            if (image.width,image.height)!=(1600,900): raise ValueError('Expected 1600x900 smoke frame')
            rgb=image.read((1,2,3)).astype(np.float64)
    # Exclude instrument panels, title + breathing-source line (y~140),
    # arms at the edges and centre reticle at screen coordinate (800,450).
    values=rgb[:,160:680,200:1400].max(axis=0)
    mask=np.ones(values.shape,dtype=bool)
    mask[280:300,590:610]=False
    return values[mask]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('lamp_on')
    parser.add_argument('lamp_off')
    args=parser.parse_args()
    on,off=scene(args.lamp_on),scene(args.lamp_off)
    report={'on_mean_channel_max':float(on.mean()),'off_mean_channel_max':float(off.mean()),
            'off_p999':float(np.quantile(off,.999)),
            'lit_pixels_above_20':int(np.count_nonzero((on>20)&(off<3))),
            'mean_difference':float((on-off).mean())}
    print(json.dumps(report,indent=2))
    if report['off_p999']>2: raise SystemExit('FAIL: unlit night scene contains a visible background glow')
    if report['lit_pixels_above_20']<5000 or report['mean_difference']<1:
        raise SystemExit('FAIL: torch does not visibly illuminate the scene')
    print('DIVE_LIGHT_FRAMES_PASS')


if __name__=='__main__': main()
