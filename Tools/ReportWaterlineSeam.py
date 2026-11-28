"""Bind waterline material comparisons without claiming the remaining seam is fixed."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(path.read_bytes()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--static', type=Path, required=True)
    args = parser.parse_args()
    studies = {}
    for name, folder in [('candidate_sweep', args.candidate), ('baseline_sweep', args.baseline), ('candidate_static', args.static)]:
        folder = folder.resolve()
        cases = load(folder/'Cases.json')
        if isinstance(cases, dict):
            cases = [cases]
        if len(cases) != (1 if name == 'candidate_static' else 3) or any(c['status'] != 'PASS' for c in cases):
            raise ValueError('Missing completed native cases')
        pictures = []
        for case in cases:
            files = sorted((folder/case['name']).glob('Water*.png'))
            if len(files) != (3 if name == 'candidate_static' else 201):
                raise ValueError('Missing native images')
            pictures.extend(record(file) for file in files)
        studies[name] = dict(host=load(folder/'Host.json'), cases=cases, images=pictures,
                            manifest=record(folder/'ShaderManifest.json'))
    review = load(args.candidate/'SweepReview.json')
    if len(review) != 3 or any(len(c['baseline_comparison']) != 201 for c in review):
        raise ValueError('Missing per-frame material comparison')
    fixed = {}
    for name, folder in [('before', ROOT/'artifacts/reflection-transport-20261128-001730'), ('candidate', args.static.resolve())]:
        file = folder/'normal/Water_0_1.png'
        pixels = np.asarray(Image.open(file).convert('RGB'), dtype=np.float64)[425:450, 80:900]
        luma = pixels @ np.array([.2126, .7152, .0722])
        fixed[name] = dict(image=record(file), maximum=float(luma.max()), p99=float(np.percentile(luma,99)),
                           pixels_over_80=int((luma>80).sum()), mean=float(luma.mean()))
    result = dict(status='ROUND_CLOSED_WITH_DEFERRED_LIMITATIONS_NOT_RELEASED', iteration_1_complete=False,
                  iteration_1_round_closed=True, original_visual_acceptance_criterion_met=False,
                  remaining=['Visible bright seam', 'Brief boundary bands reproduced with previous material; deferred as UW-LATER-07'],
                  studies=studies, fixed_roi=dict(x=[80,900], y=[425,450], end_exclusive=True, values=fixed),
                  sweep_review=review,
                  scope='Native editor screenshots at fixed simulation times; not package, performance, stability or complete visual acceptance.')
    output = ROOT/'Documentation/WATERLINE_SEAM_RESULTS.json'
    output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(output)


if __name__ == '__main__':
    main()
