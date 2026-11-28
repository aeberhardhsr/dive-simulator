"""Identify suit triangles under screen pixels in an explicit editor FPV probe.

This CPU LOD0, two-sided ray query classifies geometry, not pixel visibility:
other meshes, materials, LOD selection and temporal rendering may differ.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np


def cast_rays(geometry, width, height, pixels, near_cm=10):
    rows = geometry['vertices']
    vertices = np.asarray([r[:3] for r in rows], dtype=np.float64)
    indices = np.asarray(geometry['indices'], dtype=np.int64).reshape(-1, 3)
    if vertices.ndim != 2 or vertices.shape[1] != 3 or not np.isfinite(vertices).all():
        raise ValueError('Invalid vertex positions')
    if not len(indices) or indices.min() < 0 or indices.max() >= len(vertices):
        raise ValueError('Invalid triangle indices')
    if width <= 0 or height <= 0 or not 0 < geometry['horizontal_fov_degrees'] < 180:
        raise ValueError('Invalid view geometry')
    triangles = vertices[indices]
    a = triangles[:, 0]
    e1, e2 = triangles[:, 1] - a, triangles[:, 2] - a
    q = np.cross(-a, e1)
    scale = np.tan(np.radians(geometry['horizontal_fov_degrees']) / 2)
    results = []
    for x, y in pixels:
        ray = np.array([1, (2*x/width-1)*scale, (1-2*y/height)*scale*height/width])
        p = np.cross(ray, e2)
        determinant = np.einsum('ij,ij->i', e1, p)
        inverse = np.divide(1, determinant, out=np.zeros_like(determinant), where=np.abs(determinant) > 1e-10)
        u = np.einsum('ij,ij->i', -a, p) * inverse
        v = np.einsum('j,ij->i', ray, q) * inverse
        depth = np.einsum('ij,ij->i', e2, q) * inverse
        valid = (np.abs(determinant) > 1e-10) & (u >= 0) & (v >= 0) & (u+v <= 1) & (depth > near_cm)
        record = {'pixel': [x, y], 'hit': bool(valid.any())}
        if record['hit']:
            i = int(np.where(valid, depth, np.inf).argmin())
            record.update(depth_cm=float(depth[i]), triangle=i,
                          bones=[rows[j][3] for j in indices[i]],
                          vertex_rgb8=[rows[j][4:7] for j in indices[i]])
        results.append(record)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    root = args.directory
    geometry = json.loads((root/'FPVGeometry.json').read_text(encoding='utf-8-sig'))
    metadata = json.loads((root/'Metadata.json').read_text(encoding='utf-8-sig'))
    width, height = metadata['width'], metadata['height']
    # Fixed relative positions allow different resolutions without changing rays.
    pixels = [(x*width/1920, y*height/1080) for x, y in
              [(960, 540), (960, 800), (960, 860), (960, 900), (1200, 860),
               (1200, 900), (1400, 950), (600, 950), (800, 950)]]
    report = {
        'scope': 'CPU LOD0 two-sided suit ray classification; no visual acceptance',
        'near_plane_x_cm': 10,
        'geometry_sha256': hashlib.sha256((root/'FPVGeometry.json').read_bytes()).hexdigest(),
        'metadata_sha256': hashlib.sha256((root/'Metadata.json').read_bytes()).hexdigest(),
        'tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'samples': cast_rays(geometry, width, height, pixels),
    }
    (root/'FPVRays.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(f"Classified {sum(s['hit'] for s in report['samples'])}/{len(pixels)} sampled rays against the suit.")


if __name__ == '__main__':
    main()
