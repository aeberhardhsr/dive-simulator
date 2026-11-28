"""Independent decoded-mask check against mapped polygons, including holes."""
import hashlib
import json
from pathlib import Path
import numpy as np
from PIL import Image
from shapely import STRtree, points
from shapely.geometry import shape

ROOT = Path(__file__).resolve().parents[1]


def main():
    source = json.loads((ROOT/'Documentation/SURFACE_COVER_MASK.json').read_text(encoding='utf-8'))
    output = json.loads((ROOT/'Documentation/CLIFF_COVERAGE.json').read_text(encoding='utf-8'))
    geo = next(x for x in source['outputs'] if x['file'].endswith('.geojson'))
    path = ROOT/geo['file']
    if hashlib.sha256(path.read_bytes()).hexdigest() != geo['sha256']:
        raise ValueError('Mapped polygon source changed')
    features = json.loads(path.read_text(encoding='utf-8'))['features']
    coverage_path = ROOT/output['output']['file']
    coverage_hash = hashlib.sha256(coverage_path.read_bytes()).hexdigest()
    if coverage_hash != output['output']['sha256']:
        raise ValueError('Decoded coverage differs from preparation manifest')
    with Image.open(coverage_path) as image:
        if image.mode != 'RGBA' or image.size != (4096,4096):
            raise ValueError('Unexpected decoded coverage format')
        pixels = np.asarray(image)
    if np.any((pixels[...,:2] != 0) & (pixels[...,:2] != 255)):
        raise ValueError('Source coverage must be binary before mip generation')
    rng = np.random.default_rng(28112026)
    cols = rng.integers(0,4096,30000);rows = rng.integers(0,4096,30000)
    east = 2723000+(cols+.5)*22000/4096
    north = 1228000-(rows+.5)*13000/4096
    # Reconstruct the shader's world-centimetre registration separately from
    # the source rasterizer: X north of 1219000, Y east of 2726000.
    wx=(north-1219000)*100;wy=(east-2726000)*100
    uv=np.column_stack(((wy+300000)/2200000,(900000-wx)/1300000))
    sampled_cols=np.floor(uv[:,0]*4096).astype(int)
    sampled_rows=np.floor(uv[:,1]*4096).astype(int)
    if not np.array_equal(cols,sampled_cols) or not np.array_equal(rows,sampled_rows):
        raise ValueError('Shader UV registration disagrees with geographic centres')
    checked=[]
    for channel,category in enumerate(('Fels','Fels locker')):
        geometries=[shape(f['geometry']) for f in features if f['properties']['source_class']==category]
        hits=STRtree(geometries).query(points(east,north),predicate='within')
        expected=np.zeros(len(cols),dtype=bool);expected[hits[0]]=True
        actual=pixels[sampled_rows,sampled_cols,channel]>127
        if not np.array_equal(actual,expected):
            raise ValueError('Decoded pixels disagree with mapped polygons: '+category)
        checked.append({'category':category,'points':len(cols),'mismatches':0,'inside_points':int(expected.sum())})
    if np.any(pixels[...,2]) or not np.all(pixels[...,3]==255):
        raise ValueError('Constant unused channels changed')
    report={'status':'PASS','independent_polygon_checks':checked,'uv_registration':'PASS',
            'coverage_sha256':coverage_hash,
            'source_geojson_sha256':geo['sha256'],'limits':'Checks decoded offline pixels and declared shader registration; not a GPU texture readback or 3D rock-shape survey.'}
    (ROOT/'Documentation/CLIFF_COVERAGE_AUDIT.json').write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print('CLIFF_COVERAGE_INDEPENDENT_PASS',checked)


if __name__=='__main__':
    main()
