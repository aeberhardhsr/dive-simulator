"""Independently check prepared meshes against the pinned CityGML surfaces.

No triangulator or preparation helper is imported. Source polygon areas and
source corner coordinates are compared with every output building.
"""
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zipfile

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'divingsim/Content/Diving/Geodata'
G = '{http://www.opengis.net/gml}'
B = '{http://www.opengis.net/citygml/building/2.0}'


def validate():
    sources_path = DATA / 'buildings_sources.json'
    sources = json.loads(sources_path.read_text(encoding='utf-8'))
    manifest = json.loads((DATA / 'buildings.json').read_text(encoding='utf-8'))
    assert hashlib.sha256(sources_path.read_bytes()).hexdigest() == manifest['sources_sha256']
    meshes, offsets, objects = {}, {}, {}
    for tile in manifest['tiles']:
        key = tuple(tile['cell'])
        raw = (DATA / 'Buildings' / tile['file']).read_bytes()
        assert raw[:4] == b'DVB1'
        assert struct.unpack('<I', raw[4:8])[0] == tile['triangles']
        assert len(raw) == 8 + tile['triangles'] * 52
        assert hashlib.sha256(raw).hexdigest() == tile['sha256']
        assert hashlib.md5(raw).hexdigest() == tile['md5']
        meshes[key] = np.frombuffer(raw, dtype='<f4', offset=8).reshape(-1, 13)
        assert np.isfinite(meshes[key]).all()
        assert ((meshes[key][:, 9:] >= 0) & (meshes[key][:, 9:] <= 1)).all()
        offsets[key] = 0
    for item in manifest['buildings']:
        key = tuple(item['cell'])
        start = offsets[key]
        end = start + item['triangles']
        assert item['id'] not in objects
        objects[item['id']] = (item, meshes[key][start:end, :9].reshape(-1, 3, 3))
        offsets[key] = end
    assert all(offsets[key] == len(value) for key, value in meshes.items())
    checked, duplicates, max_error, max_area_error = set(), 0, 0., 0.
    for source in sources['sources']:
        path = ROOT / 'Geodata/Raw/Buildings' / source['file']
        assert hashlib.sha256(path.read_bytes()).hexdigest() == source['sha256']
        surface_ids, solid_ids = set(), set()
        with zipfile.ZipFile(path) as archive:
            names = [n for n in archive.namelist() if n.lower().endswith('.gml')]
            assert len(names) == 1
            with archive.open(names[0]) as stream:
                context = ET.iterparse(stream, events=('start', 'end'))
                _, root = next(context)
                for event, element in context:
                    if event != 'end' or element.tag != B + 'Building':
                        continue
                    identity = element.attrib[G + 'id']
                    boundaries = element.findall(B + 'boundedBy')
                    if not boundaries:
                        assert element.find(B + 'lod2Solid') is not None
                        solid_ids.add(identity)
                        element.clear(); root.clear()
                        continue
                    surface_ids.add(identity)
                    points, area = [], 0.
                    for boundary in boundaries:
                        for polygon in boundary.iter(G + 'Polygon'):
                            for tag, sign in [('exterior', 1), ('interior', -1)]:
                                for ring in polygon.findall(G + tag):
                                    pos = list(ring.iter(G + 'posList'))
                                    assert len(pos) == 1
                                    xyz = np.array([float(v) for v in pos[0].text.split()]).reshape(-1, 3)
                                    points.append(xyz)
                                    local = xyz - xyz[0]
                                    area += sign * np.linalg.norm(np.cross(local, np.roll(local, -1, axis=0)).sum(axis=0)) / 2
                    xyz = np.concatenate(points)
                    low, high = xyz.min(axis=0), xyz.max(axis=0)
                    intersects = high[0] >= 2723000 and low[0] <= 2745000 and high[1] >= 1215000 and low[1] <= 1228000
                    if intersects and area > 1.e-9:
                        assert identity in objects, 'Missing source building: ' + identity
                        if identity in checked:
                            duplicates += 1
                        else:
                            item, triangles = objects[identity]
                            assert item['source'] == source['id']
                            assert np.array_equal(np.r_[low, high], item['bounds_lv95_ln02'])
                            east, north = item['cell']
                            origin = np.array([2726000 + east * 512, 1219000 + north * 512, 419])
                            encoded = ((xyz - origin)[:, [1, 0, 2]] * 100).astype('<f4')
                            corners = {p.tobytes() for p in encoded}
                            assert all(p.tobytes() in corners for p in triangles.reshape(-1, 3)), identity
                            decoded = encoded.astype(np.float64)[:, [1, 0, 2]] / 100 + origin
                            max_error = max(max_error, float(np.abs(decoded - xyz).max()))
                            t = triangles.astype(np.float64) / 100
                            output_area = np.linalg.norm(np.cross(t[:, 1] - t[:, 0], t[:, 2] - t[:, 0]), axis=1).sum() / 2
                            relative = abs(output_area - area) / max(1., area)
                            assert relative < 1.e-4, (identity, area, output_area)
                            max_area_error = max(max_area_error, float(relative))
                            checked.add(identity)
                    element.clear(); root.clear()
        assert not solid_ids - surface_ids
        print('VERIFIED_SOURCE', source['id'], flush=True)
    assert len(checked) == len(objects) == manifest['building_count']
    assert duplicates == manifest['duplicate_objects_removed']
    report = dict(passed=True, sources=len(sources['sources']), buildings=len(checked),
                  cells=len(meshes), triangles=sum(len(v) for v in meshes.values()),
                  duplicate_objects_removed=duplicates, maximum_coordinate_rounding_m=max_error,
                  maximum_relative_area_error=max_area_error,
                  checks=['pinned ZIP SHA256', 'mesh SHA256 and MD5', 'complete source coverage',
                          'every output corner is an original source corner', 'independent polygon area',
                          'source bounds', 'no unconsumed mesh triangles'])
    (ROOT / 'artifacts/buildings-data-validation.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report), flush=True)


if __name__ == '__main__':
    validate()
