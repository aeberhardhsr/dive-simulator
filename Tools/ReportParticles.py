"""Check packaged particle evidence. Uses the geodata numpy/rasterio environment."""
import argparse
import hashlib
import json
import re
import warnings
from pathlib import Path

import numpy as np
import rasterio
from rasterio.errors import NotGeoreferencedWarning


def diagnostic_pixels(path):
    with warnings.catch_warnings():
        warnings.simplefilter('ignore', NotGeoreferencedWarning)
        with rasterio.open(path) as source:
            assert (source.width, source.height) == (1600, 900)
            r, g, b = source.read((1, 2, 3)).astype(np.int16)[:, 160:680, 200:1400]
    return int(np.count_nonzero((r > g + 8) & (b > g + 8)))


def report(root):
    cases = json.loads((root/'Cases.json').read_text(encoding='utf-8-sig'))
    host = json.loads((root/'Host.json').read_text(encoding='utf-8-sig'))
    assert hashlib.sha256(Path(host['executable']).read_bytes()).hexdigest().upper() == host['executable_sha256']
    rows = {}
    for case in cases:
        name = case['name']; folder = root/name
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig', errors='replace')
        assert 'DIVE_SMOKE_PASS' in log and not re.search(r'DIVE_\w+_FAIL', log), name
        # Validate the final frame too, after streaming or surfacing has occurred.
        match = re.search(r'DIVE_PARTICLES_PASS final=1 gpu_grains=(\d+) scale=([\d.]+) live_bubbles=(\d+) born=(\d+) surfaced=(\d+) immutable_instances=1 cell_moves=(\d+) pv_relative_error=([\deE.+-]+)', log)
        assert match, (name, 'Missing final particle validation')
        count, scale, live, born, surfaced, moves, error = map(float, match.groups())
        assert count == 640 and error < 1e-4, name
        assert scale == case['options'].get('ParticleScale', 1), name
        if case['options'].get('NoBubbles'):
            assert born == live == surfaced == 0, name
        elif name in ('wide', 'spot', 'day', 'off', 'turbid', 'audio', 'stream'):
            assert born >= 28, (name, 'No exhaled bubbles')
        if name == 'stream':
            assert moves >= 5, 'Streaming did not cross enough particle cells'
        rows[name] = dict(grains=int(count), diagnostic_scale=scale, live_bubbles=int(live),
                          born=int(born), surfaced=int(surfaced), cell_moves=int(moves),
                          max_final_pv_relative_error=error, fresh_images=len(list(folder.glob('*.png'))),
                          sha256={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(folder.iterdir()) if f.is_file()})
    required = {'wide','spot','day','off','turbid','gpu-visible','gpu-zero','surface','storm','stream','audio','breathing','pause'}
    assert rows.keys() >= required, 'Missing scenarios'
    for name in ('gpu-visible', 'gpu-zero'):
        rows[name]['diagnostic_coloured_pixels'] = diagnostic_pixels(root/name/'DiveUnderwater_1_torch.png')
    visible = rows['gpu-visible']['diagnostic_coloured_pixels']
    zero = rows['gpu-zero']['diagnostic_coloured_pixels']
    assert visible > 100 and visible > zero + 100, ('GPU grains not actually rendered', visible, zero)
    assert zero < 20, ('Zero-size grains still render', zero)
    assert rows['day']['surfaced'] > 0, 'No bubble surface removal observed'
    result = dict(status='PASS', host=host, cases=rows, artifact_directory=str(root.resolve()),
                  limits='GPU analytic WPO motion, not a compute fluid solver. Magnified emissive diagnostics are not gameplay visuals. Bubble refraction and normal-size appearance require visual review.')
    (root/'Results.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    report(parser.parse_args().directory)
