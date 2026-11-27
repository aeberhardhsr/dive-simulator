"""Blender read-only audit of authored RGB, existing FBX and native UE bytes.

blender --background blender/AquaticPlants.blend --python Tools/AuditPlantColors.py
  -- --native-log PATH --output PATH
Run SmokeTest.ps1 -EditorSandbox -Scenario dive -PlantColorProbe first.
This verifies component ranges, not vertex ordering or rendered illumination.
"""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

import bpy
import numpy as np
from io_scene_fbx import parse_fbx

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-log', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:])
    log = args.native_log.read_text(encoding='utf-8', errors='replace')
    if 'DIVE_SMOKE_PASS' not in log or 'DIVE_SMOKE_FAIL' in log:
        raise RuntimeError('A successful native runtime probe is required')
    records = []
    for name in ['SM_Pondweed', 'SM_Stonewort', 'SM_Milfoil']:
        colors = bpy.data.objects[name].data.color_attributes['Color']
        linear = np.asarray([c.color[:] for c in colors.data])
        encoded = np.asarray([c.color_srgb[:] for c in colors.data])
        path = ROOT / 'blender/exports/Plants' / (name + '.fbx')
        tree, version = parse_fbx.parse(str(path))
        objects = next(n for n in tree.elems if n.id == b'Objects')
        geometry = next(n for n in objects.elems if n.id == b'Geometry')
        layer = next(n for n in geometry.elems if n.id == b'LayerElementColor')
        payload = next(n for n in layer.elems if n.id == b'Colors')
        exported = np.asarray(payload.props[0]).reshape(-1, 4)
        authored_range = np.array([linear.min(0), linear.max(0)])
        encoded_range = np.array([encoded.min(0), encoded.max(0)])
        fbx_range = np.array([exported.min(0), exported.max(0)])
        error = float(np.abs(fbx_range - encoded_range).max())
        if error > 1e-6:
            raise RuntimeError(name + ': FBX differs from authored sRGB ranges')
        matches = re.findall(r'DIVE_PLANT_COLOR_BUFFER mesh=' + name +
                             r' vertices=(\d+) min=([\d,]+) max=([\d,]+)', log)
        if not matches or len(set(matches)) != 1:
            raise RuntimeError(name + ': missing or inconsistent native buffer')
        count, lo, hi = matches[0]
        native = np.array([[int(v) for v in row.split(',')] for row in (lo, hi)])
        if native.shape != (2, 4) or np.abs(native - encoded_range * 255).max() > 1.01:
            raise RuntimeError(name + ': native bytes no longer follow the sRGB contract')
        rgb = native[:, :3] / 255.0
        decoded = np.where(rgb > .04045, ((rgb + .055) / 1.055) ** 2.4, rgb / 12.92)
        decode_error = float(np.abs(decoded - authored_range[:, :3]).max())
        alpha_error = float(np.abs(native[:, 3] / 255.0 - authored_range[:, 3]).max())
        if decode_error > .004 or alpha_error > 1.01 / 255:
            raise RuntimeError(name + ': RGB decoding or linear root weights differ')
        records.append(dict(mesh=name, author_vertices=len(linear), native_vertices=int(count),
                            fbx_colors=len(exported), fbx_version=version,
                            authored_range=authored_range.tolist(), fbx_range=fbx_range.tolist(),
                            native_byte_range=native.tolist(), decoded_rgb_range=decoded.tolist(),
                            fbx_range_error=error, decoded_range_error=decode_error,
                            linear_alpha_error=alpha_error, fbx_sha256=sha(path),
                            mesh_sha256=sha(ROOT / 'divingsim/Content/Diving/Plants' / (name + '.uasset'))))
    report = dict(status='PASS', scope='component ranges of LOD0; RGB sRGB, alpha linear',
                  blender_version=bpy.app.version_string, assets=records,
                  blend_sha256=sha(ROOT / 'blender/AquaticPlants.blend'),
                  native_log_sha256=sha(args.native_log),
                  shader_sha256=sha(ROOT / 'Tools/Shaders/AquaticPlant.ush'),
                  material_sha256=sha(ROOT / 'divingsim/Content/Diving/Materials/M_AquaticPlant.uasset'))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('DIVE_PLANT_COLOR_AUDIT_PASS assets=3 report=' + str(args.output))


if __name__ == '__main__':
    main()
