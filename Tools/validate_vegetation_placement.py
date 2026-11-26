"""Audit serialized placement against source geometry, independently of generation."""
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
from scipy.spatial import cKDTree
from shapely import STRtree, points
from shapely.geometry import shape

ROOT = Path(__file__).resolve().parents[1]


def main():
    path = ROOT/'divingsim/Content/Diving/Geodata/vegetation.trees'
    raw = path.read_bytes()
    manifest = json.loads((path.parent/'vegetation.json').read_text())
    digest = hashlib.sha256(raw).hexdigest()
    assert digest == manifest['sha256'] and raw[:4] == b'DVT1'
    count, = struct.unpack_from('<I', raw, 4)
    assert len(raw) == 8+24*count and count == manifest['count']
    dtype = np.dtype([('e','<f4'),('n','<f4'),('h','<f4'),('z','<f4'),
                      ('seed','<u4'),('kind','u1'),('source','u1'),('pad','<u2')])
    records = np.frombuffer(raw, dtype=dtype, offset=8)
    xy = np.column_stack((records['e'].astype(float)+2726000,
                          records['n'].astype(float)+1219000))
    assert np.isfinite(xy).all() and np.isfinite(records['h']).all()
    assert np.all((records['h'] >= 3) & (records['h'] <= 45))
    assert not records['pad'].any()
    source = records['source'] == 1
    tops = json.loads((ROOT/'Geodata/VegetationSource/tree_tops.geojson').read_text())['features']
    coordinates = np.array([f['geometry']['coordinates'] for f in tops])
    distance, indices = cKDTree(coordinates[:, :2]).query(xy[source])
    # At up to 19 km east, a float32 offset has a 1.953125-mm ULP.
    # Verify exact nearest-float encoding, not an assumed sub-millimetre bound.
    assert distance.max() < .002
    assert np.array_equal(records['e'][source], (coordinates[indices, 0]-2726000).astype('<f4'))
    assert np.array_equal(records['n'][source], (coordinates[indices, 1]-1219000).astype('<f4'))
    assert len(np.unique(indices)) == source.sum()
    crown_error = np.abs(records['z'][source].astype(float)+419+
                         records['h'][source]-coordinates[indices, 2])
    assert crown_error.max() < .001
    forest = json.loads((ROOT/'Geodata/VegetationSource/forest.geojson').read_text())['features']
    geometry = [shape(f['geometry']) for f in forest]
    classes = np.array([f['properties']['class_id'] for f in forest])
    authored = records[~source]
    # The 2-mm tolerance permits only local-coordinate serialization rounding.
    pairs = STRtree(geometry).query(points(xy[~source]), predicate='dwithin', distance=.002)
    valid = np.zeros(len(authored), dtype=bool)
    matched = authored['kind'][pairs[0]] == classes[pairs[1]]
    valid[pairs[0, matched]] = True
    assert valid.all(), 'An authored tree lies outside its mapped woodland class'
    spacing, _ = cKDTree(coordinates[:, :2]).query(xy[~source])
    assert spacing.min() > 4.998
    result = dict(status='PASS', binary_sha256=digest, records=count,
                  surveyed=int(source.sum()), authored=int((~source).sum()),
                  maximum_surveyed_xy_error_m=float(distance.max()),
                  maximum_surveyed_crown_height_error_m=float(crown_error.max()),
                  authored_outside_mapped_class=0, minimum_authored_to_surveyed_m=float(spacing.min()),
                  scope='Serialized candidates and source topology; runtime ground anchoring and cell lifetime are checked separately in the packaged game.')
    (ROOT/'Documentation/VEGETATION_PLACEMENT_AUDIT.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
