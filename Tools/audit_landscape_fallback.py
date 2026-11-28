"""Inspect exported native fallback buffers at the 04_02 / 05_02 shared edge.

No screenshots or source triangles are substituted for the actual LOD0 buffers.
The optional acceptance gate applies only to this shared edge, not all 77 tiles.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def open_edges(vertices, triangles):
    unique, inverse = np.unique(vertices, axis=0, return_inverse=True)
    canonical = inverse[triangles]
    edges = np.concatenate([canonical[:, [0, 1]], canonical[:, [1, 2]], canonical[:, [2, 0]]])
    edges.sort(axis=1)
    edges, counts = np.unique(edges, axis=0, return_counts=True)
    if np.any(counts > 2):
        raise ValueError('Nonmanifold fallback edge')
    return unique[edges[counts == 1]]


def section(edges, target_y, positions):
    # Native decimation displaces the boundary. Select complete segments within
    # 10m of the expected edge, and exclude horizontal north/south end edges.
    segments = edges[(np.abs(edges[:, :, 1]-target_y).max(axis=1) < 1000)
                     & (np.abs(edges[:, 0, 0]-edges[:, 1, 0]) > .001)]
    samples = np.full((len(positions), 2), np.nan)
    coverage = np.zeros(len(positions), dtype=int)
    for a, b in segments:
        hit = (positions >= min(a[0], b[0])) & (positions < max(a[0], b[0]))
        t = (positions[hit]-a[0])/(b[0]-a[0])
        samples[hit] = a[1:]+t[:, None]*(b[1:]-a[1:])
        coverage[hit] += 1
    if not np.all(coverage == 1):
        raise ValueError(f'Ambiguous/missing edge intersection: {np.unique(coverage, return_counts=True)}')
    samples[:, 0] -= target_y
    return samples


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('folder', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--require-closed', action='store_true')
    args = parser.parse_args()
    manifest_path = args.folder/'manifest.json'
    manifest = json.loads(manifest_path.read_bytes())
    # Same north/south origin for all three tiles. Ignore 8m at either end so
    # the edge classification cannot accidentally include a neighbouring side.
    stations = np.arange(-204000., -799., 50.)
    meshes = []
    sections = {}
    for item in manifest['meshes']:
        name = item['name']
        vp = args.folder/(name+'.positions.f32')
        ip = args.folder/(name+'.indices.u32')
        vertices = np.fromfile(vp, dtype='<f4').reshape(-1, 3).astype(np.float64)
        triangles = np.fromfile(ip, dtype='<u4').reshape(-1, 3)
        if len(vertices) != item['vertices'] or len(triangles) != item['triangles']:
            raise ValueError('Native buffer counts changed')
        if not np.isfinite(vertices).all() or triangles.max() >= len(vertices):
            raise ValueError('Invalid native buffers')
        edges = open_edges(vertices, triangles)
        target_y = 204800. if name == 'SM_Land_04_02' else 0.
        samples = section(edges, target_y, stations)
        sections[name] = samples
        meshes.append({**item, 'open_edges': len(edges),
                       'max_shared_edge_horizontal_drift_cm': float(np.abs(samples[:, 0]).max()),
                       'buffers': [{'file': str(p), 'sha256': digest(p)} for p in (vp, ip)]})
    pairs = []
    west = sections['SM_Land_04_02']
    for name in ('SM_Land_05_02', 'SM_LandDetail_05_02'):
        diff = sections[name]-west
        pairs.append({'east_mesh': name, 'stations': len(stations),
                      'max_horizontal_separation_cm': float(np.abs(diff[:, 0]).max()),
                      'max_vertical_separation_cm': float(np.abs(diff[:, 1]).max()),
                      'max_separation_cm': float(np.linalg.norm(diff, axis=1).max())})
    result = {'manifest': {'file': str(manifest_path), 'sha256': digest(manifest_path)},
              'meshes': meshes, 'shared_edge_pairs': pairs,
              'scope': 'Actual native fallback buffers, one shared edge, 0.5m stations excluding 8m corners. Not all landscape seams or full surface/collision agreement.'}
    args.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(pairs))
    if args.require_closed and any(p['max_separation_cm'] > .03 for p in pairs):
        raise SystemExit('Shared fallback edge exceeds 0.03cm tolerance')


if __name__ == '__main__':
    main()
