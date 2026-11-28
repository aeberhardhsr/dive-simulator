"""Prepare pinned swissBUILDINGS3D CityGML solids in the existing LV95 world.

Source coordinates are never fitted to the terrain or replaced by box models.
Whole buildings intersecting the landscape extent are retained, including at
its edge. Runtime meshes use a local 512 m origin and centimetre coordinates.
"""
import argparse
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import struct
import urllib.request
import xml.etree.ElementTree as ET
import zipfile

import mapbox_earcut
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / 'Geodata/Raw/Buildings'
OUT = ROOT / 'divingsim/Content/Diving/Geodata/Buildings'
SOURCES = ROOT / 'divingsim/Content/Diving/Geodata/buildings_sources.json'
BOUNDS = (2723000, 1215000, 2745000, 1228000)
ORIGIN = (2726000, 1219000, 419)
CELL = 512
G = '{http://www.opengis.net/gml}'
B = '{http://www.opengis.net/citygml/building/2.0}'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def asset_name(x, y):
    def coordinate(value):
        return ('M' if value < 0 else 'P') + str(abs(value))
    return 'SM_Buildings_' + coordinate(x) + '_' + coordinate(y)


def collision_probe(data):
    triangles = data[:, :9].reshape(-1, 3, 3).astype(np.float64)
    normals = np.cross(triangles[:, 2] - triangles[:, 0], triangles[:, 1] - triangles[:, 0])
    areas = np.linalg.norm(normals, axis=1)
    eligible = np.flatnonzero((data[:, 12] > .5) & (areas > 10000))
    index = int(eligible[np.argmax(areas[eligible])] if len(eligible) else np.argmax(areas))
    if areas[index] < .01:
        raise ValueError('Cell has no nondegenerate collision surface')
    return triangles[index].mean(axis=0).tolist(), (normals[index] / areas[index]).tolist()


def download(item):
    path = RAW / item['file']
    if path.exists() and digest(path) == item['sha256']:
        return path
    temporary = path.with_suffix('.tmp')
    with urllib.request.urlopen(item['url'], timeout=90) as response, temporary.open('wb') as out:
        while block := response.read(1024 * 1024):
            out.write(block)
    if digest(temporary) != item['sha256']:
        raise ValueError('Source checksum mismatch: ' + item['file'])
    temporary.replace(path)
    return path


def triangulate(rings):
    """Triangulate an arbitrary planar 3D polygon, retaining holes and winding."""
    clean = []
    for ring in rings:
        p = np.asarray(ring, dtype=np.float64)
        if p.ndim != 2 or p.shape[1] != 3 or not np.isfinite(p).all():
            raise ValueError('Invalid polygon coordinates')
        if len(p) > 1 and np.linalg.norm(p[-1] - p[0]) < 1.e-7:
            p = p[:-1]
        if len(p) < 3:
            raise ValueError('Ring has fewer than three points')
        clean.append(p)
    # Subtract a local origin before cross products at Swiss survey coordinates.
    p = clean[0] - clean[0][0]
    normal = np.cross(p, np.roll(p, -1, axis=0)).sum(axis=0)
    length = np.linalg.norm(normal)
    if length < 1.e-9:
        return np.empty((0, 3, 3))
    normal /= length
    all_points = np.concatenate(clean)
    projected = np.delete(all_points - all_points[0], np.argmax(np.abs(normal)), axis=1)
    if len(clean) == 1 and len(clean[0]) == 3:
        triangles = all_points[None, ...].copy()
    else:
        ends = np.cumsum([len(p) for p in clean], dtype=np.uint32)
        indices = mapbox_earcut.triangulate_float64(np.ascontiguousarray(projected), ends)
        triangles = all_points[indices.reshape(-1, 3)].copy()
    cross = np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
    backwards = (cross @ normal) < 0
    triangles[backwards] = triangles[backwards][:, [0, 2, 1]]
    valid = np.linalg.norm(cross, axis=1) > 1.e-9
    return triangles[valid]


def polygons(building):
    for boundary in building.findall(B + 'boundedBy'):
        for surface in boundary:
            for polygon in surface.iter(G + 'Polygon'):
                rings = []
                for tag in ('exterior', 'interior'):
                    for ring in polygon.findall(G + tag):
                        lists = list(ring.iter(G + 'posList'))
                        if len(lists) != 1:
                            raise ValueError('Unsupported ring encoding')
                        values = np.fromstring(lists[0].text or '', sep=' ')
                        if len(values) % 3:
                            raise ValueError('Not three-dimensional CityGML')
                        rings.append(values.reshape(-1, 3))
                if not rings:
                    raise ValueError('Missing polygon exterior')
                yield surface.tag.rsplit('}', 1)[-1], rings


def read_buildings(path):
    surface_ids, solid_ids = set(), set()
    with zipfile.ZipFile(path) as archive:
        names = [n for n in archive.namelist() if n.lower().endswith('.gml')]
        if len(names) != 1:
            raise ValueError('Expected one CityGML document')
        with archive.open(names[0]) as source:
            context = ET.iterparse(source, events=('start', 'end'))
            _, root = next(context)
            for event, element in context:
                if event != 'end' or element.tag != B + 'Building':
                    continue
                identity = element.get(G + 'id')
                if not identity:
                    raise ValueError('Building has no source identity')
                faces = list(polygons(element))
                if not faces:
                    # This export includes both product variants under the
                    # same building ID: separate surfaces and a rounded solid.
                    # Select the separate surfaces once, not both overlapping
                    # shells. At EOF require a surface model for every solid.
                    if element.find(B + 'lod2Solid') is None:
                        raise ValueError('Building has no supported surfaces: ' + identity)
                    solid_ids.add(identity)
                    element.clear()
                    root.clear()
                    continue
                surface_ids.add(identity)
                points = np.concatenate([r for _, rings in faces for r in rings])
                low, high = points.min(axis=0), points.max(axis=0)
                if high[0] >= BOUNDS[0] and low[0] <= BOUNDS[2] and high[1] >= BOUNDS[1] and low[1] <= BOUNDS[3]:
                    triangles, roofs = [], []
                    for kind, rings in faces:
                        tris = triangulate(rings)
                        if not len(tris):
                            continue
                        cross = np.cross(tris[:, 1] - tris[:, 0], tris[:, 2] - tris[:, 0])
                        upward = cross[:, 2] / np.linalg.norm(cross, axis=1)
                        triangles.append(tris)
                        roofs.append((upward > .2) & (kind == 'RoofSurface'))
                    if triangles:
                        yield identity, np.concatenate(triangles), np.concatenate(roofs), low, high
                element.clear()
                root.clear()
    if solid_ids - surface_ids:
        raise ValueError('Solid without a matching separate-surface model: ' + str(sorted(solid_ids - surface_ids)[:3]))


def prepare():
    manifest = json.loads(SOURCES.read_text(encoding='utf-8'))
    RAW.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(max_workers=4) as pool:
        paths = list(pool.map(download, manifest['sources']))
    cells = defaultdict(list)
    objects, seen, source_counts = [], {}, Counter()
    duplicates = 0
    for item, path in zip(manifest['sources'], paths):
        for identity, tris, roofs, low, high in read_buildings(path):
            source_hash = hashlib.sha256(np.round(tris, 4).tobytes()).hexdigest()
            if identity in seen:
                if seen[identity] != source_hash:
                    raise ValueError('Conflicting copies of building: ' + identity)
                duplicates += 1
                continue
            seen[identity] = source_hash
            center = (low + high) / 2
            cell = tuple(map(int, np.floor((center[:2] - np.array(ORIGIN[:2])) / CELL)))
            origin = np.array([ORIGIN[0] + cell[0] * CELL, ORIGIN[1] + cell[1] * CELL, ORIGIN[2]])
            # Swap east/north for UE's X=north,Y=east. Keeping source index
            # order produces Unreal's clockwise front faces after this reflection.
            vertices = ((tris - origin) * 100)[:, :, [1, 0, 2]].astype('<f4')
            token = hashlib.sha256(identity.encode()).digest()[0] / 255
            srgb = np.array([.69 + .1 * token, .68 + .095 * token, .65 + .09 * token])
            rgb = ((srgb + .055) / 1.055) ** 2.4
            colors = np.tile(np.r_[rgb, 0], (len(tris), 1)).astype('<f4')
            colors[:, 3] = roofs
            cells[cell].append(np.c_[vertices.reshape(-1, 9), colors].astype('<f4'))
            source_counts[item['id']] += 1
            objects.append({'id': identity, 'source': item['id'], 'cell': list(map(int, cell)),
                            'bounds_lv95_ln02': np.r_[low, high].tolist(), 'triangles': len(tris),
                            'source_geometry_sha256': source_hash})
        print('PREPARED', item['id'], source_counts[item['id']], flush=True)
    tiles = []
    for (x, y), batches in sorted(cells.items()):
        data = np.concatenate(batches)
        probe, normal = collision_probe(data)
        filename = f'buildings_{x}_{y}.bmesh'
        path = OUT / filename
        path.write_bytes(b'DVB1' + struct.pack('<I', len(data)) + data.tobytes())
        tiles.append({'cell': [x, y], 'file': filename, 'sha256': digest(path),
                      'md5': hashlib.md5(path.read_bytes()).hexdigest(), 'triangles': len(data),
                      'origin_east': ORIGIN[0] + x * CELL, 'origin_north': ORIGIN[1] + y * CELL,
                      'origin_x_cm': y * CELL * 100, 'origin_y_cm': x * CELL * 100,
                      'probe_local_cm': probe, 'probe_normal': normal,
                      'asset': f'/Game/Diving/Buildings/{asset_name(x, y)}.{asset_name(x, y)}'})
    result = {'schema': 1, 'horizontal_crs': 'EPSG:2056', 'vertical_crs': 'EPSG:5728',
              'water_elevation_m': ORIGIN[2], 'bounds_lv95': list(BOUNDS), 'cell_metres': CELL,
              'sources_sha256': digest(SOURCES), 'attribution': 'Buildings: © swisstopo, swissBUILDINGS3D 3.0 Beta',
              'encoding': 'DVB1, uint32 triangles, 13 little-endian float32 values per triangle: 3 XYZ corners in local cm, linear wall RGB and roof mask',
              'appearance': 'Surveyed exterior geometry; generic plaster facade colours; roof imagery from the separately pinned SWISSIMAGE dataset. No facade photos or interiors.',
              'duplicate_objects_removed': duplicates, 'building_count': len(objects),
              'triangle_count': sum(t['triangles'] for t in tiles), 'tiles': tiles, 'buildings': objects}
    target = OUT.parent / 'buildings.json'
    target.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print('BUILDINGS_READY', len(objects), len(tiles), result['triangle_count'], flush=True)


if __name__ == '__main__':
    argparse.ArgumentParser(description=__doc__).parse_args()
    prepare()
