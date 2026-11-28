"""Independently verify complete local source selection, polygon holes and tree-top Z."""
import hashlib
import json
from pathlib import Path
import struct
import numpy as np
import shapefile
from shapely import force_2d
from shapely.geometry import box, shape

ROOT=Path(__file__).resolve().parents[1]
RAW=ROOT/'Geodata/Raw/Vegetation'


def sha(path):
    with path.open('rb') as stream: return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    report=json.loads((ROOT/'Documentation/WALENSEE_VEGETATION_SOURCE.json').read_text(encoding='utf-8'))
    for item in report['files']:
        path=RAW/item['file']
        assert path.stat().st_size==item['size'] and sha(path)==item['sha256'], str(path)
    for item in report['outputs']:
        assert sha(ROOT/item['file'])==item['sha256'], item['file']
    bounds=report['bounds'];region=box(*bounds)
    forest=json.loads((ROOT/'Geodata/VegetationSource/forest.geojson').read_text())['features']
    by_oid={f['properties']['source_oid']:f for f in forest}
    assert len(by_oid)==len(forest)
    reader=shapefile.Reader(str(RAW/'swissTLM3D_TLM_BODENBEDECKUNG_OST'),encoding='utf-8')
    selected=set();holes=0;areas={}
    for candidate in reader.iterShapes(bbox=bounds):
        record=reader.record(candidate.oid).as_dict()
        if record['OBJEKTART'] not in ('Gebueschwald','Wald','Wald offen','Gehoelzflaeche'): continue
        original=force_2d(shape(candidate.__geo_interface__))
        if not original.intersects(region) or original.intersection(region).area==0: continue
        selected.add(candidate.oid)
        feature=by_oid[candidate.oid];actual=shape(feature['geometry'])
        assert actual.is_valid and region.covers(actual)
        # Symmetric-difference area and exact topology include interior holes.
        expected=original.intersection(region)
        assert actual.equals(expected) and actual.symmetric_difference(expected).area<1e-8
        assert feature['properties']['uuid']==record['UUID']
        assert feature['properties']['class_name']==record['OBJEKTART']
        pieces=list(actual.geoms) if actual.geom_type=='MultiPolygon' else [actual]
        holes+=sum(len(p.interiors) for p in pieces)
        label=record['OBJEKTART'];areas[label]=areas.get(label,0)+actual.area
    assert selected==set(by_oid)
    reader.close()

    # Independent full PointZ scan through the binary SHP layout. This proves
    # completeness even if a library's bbox option ignores point geometries.
    path=RAW/'swissTLM3D_TLM_EINZELBAUM_OST.shp'
    header=path.open('rb').read(100)
    assert struct.unpack_from('<i',header,32)[0]==11
    dtype=np.dtype([('id','>i4'),('words','>i4'),('type','<i4'),('east','<f8'),('north','<f8'),('top','<f8'),('measure','<f8')])
    assert dtype.itemsize==44 and (path.stat().st_size-100)%44==0
    points=np.memmap(path,dtype=dtype,mode='r',offset=100)
    assert (points['type']==11).all() and (points['words']==18).all()
    valid=(points['east']>=bounds[0])&(points['east']<=bounds[2])&(points['north']>=bounds[1])&(points['north']<=bounds[3])
    expected_ids=set(np.flatnonzero(valid).tolist())
    trees=json.loads((ROOT/'Geodata/VegetationSource/tree_tops.geojson').read_text())['features']
    assert expected_ids=={f['properties']['source_oid'] for f in trees} and len(trees)==len(expected_ids)
    reader=shapefile.Reader(str(path.with_suffix('')),encoding='utf-8')
    for f in trees:
        oid=f['properties']['source_oid'];p=points[oid];r=reader.record(oid).as_dict()
        assert list(f['geometry']['coordinates'])==[p['east'],p['north'],p['top']]
        assert f['properties']['uuid']==r['UUID'] and r['OBJEKTART']=='Einzelbaum'
    reader.close()
    evidence=dict(status='PASS',forest_polygons=len(forest),preserved_interior_rings=holes,
                  surveyed_tree_tops=len(trees),national_east_tree_records=len(points),
                  forest_area_by_class_m2=areas,maximum_coordinate_error_m=0,
                  manifest_sha256=sha(ROOT/'Documentation/WALENSEE_VEGETATION_SOURCE.json'))
    (ROOT/'Documentation/VEGETATION_SOURCE_AUDIT.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print('VEGETATION_SOURCE_AUDIT_PASS',len(forest),len(trees),holes,flush=True)


if __name__=='__main__': main()
