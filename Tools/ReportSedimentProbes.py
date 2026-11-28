"""Check that a nearby diagnostic volume survives against distant water."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image


def report(folders):
    cases = []
    for folder in folders:
        text = (folder / 'Runtime.log').read_text(encoding='utf-8-sig')
        if 'DIVE_SEDIMENT_PROBE_COMPLETE' not in text or 'DIVE_SEDIMENT_FAIL' in text:
            raise ValueError('Incomplete diagnostic probe')
        paths = [folder / f'Sediment_0_{name}.png' for name in ['Visible', 'Hidden']]
        a, b = [np.asarray(Image.open(p).convert('RGB'), dtype=np.float32) for p in paths]
        if a.shape != (900, 1600, 3) or a.shape != b.shape:
            raise ValueError('Unexpected diagnostic viewport')
        # This band lies within the probe sphere, in front of distant water.
        # An opaque-depth post-process previously erased the nearby volume here.
        delta = np.max(np.abs(a-b), axis=2)[300:420, 700:900]
        fraction = float((delta > 15).mean())
        if fraction < .9:
            raise ValueError(f'Nearby cloud erased against distant background: {folder} ({fraction:.3f})')
        cases.append({'directory': str(folder), 'roi': [700, 300, 900, 420],
            'fraction_changed_over_15': fraction,
            'evidence': [{'path': str(p), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
                         for p in [folder / 'Runtime.log', *paths]]})
    return {'status': 'PASS', 'cases': cases,
            'limits': 'Magenta diagnostic volumes test composition and camera-inside visibility, not normal sediment colour or realism.'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('outside', type=Path)
    parser.add_argument('inside', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    data = report([args.outside.resolve(), args.inside.resolve()])
    args.output.write_text(json.dumps(data, indent=2)+'\n', encoding='utf-8')
    print('PASS:', [c['fraction_changed_over_15'] for c in data['cases']])
