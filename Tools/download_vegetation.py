"""Extract forest polygons and surveyed tree tops from pinned swissTLM3D 2026-02.

National files remain a cache; only the landscape extent is exported. Polygon
holes and source identifiers survive clipping. Tree Z is crown/top height, not
ground height and not tree height. Runtime planting must respect this distinction.
"""
import hashlib
import json
from pathlib import Path
import shutil
import zipfile
from download_lake_boundary import RangeFile, URL

ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / 'Geodata/Raw/Vegetation'
DEST = ROOT / 'Geodata/VegetationSource'
MANIFEST = ROOT / 'Documentation/WALENSEE_VEGETATION_SOURCE.json'
BASES = ('swissTLM3D_TLM_BODENBEDECKUNG_OST', 'swissTLM3D_TLM_EINZELBAUM_OST')
CLASSES = {6: 'Gebueschwald', 12: 'Wald', 13: 'Wald offen', 14: 'Gehoelzflaeche'}


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    import shapefile
    from shapely.geometry import box, shape, mapping
    from shapely import force_2d

    RAW.mkdir(parents=True, exist_ok=True)
    DEST.mkdir(parents=True, exist_ok=True)
    previous = json.loads(MANIFEST.read_text(encoding='utf-8')) if MANIFEST.exists() else None
    remote = RangeFile(URL)
    coast = json.loads((ROOT / 'Documentation/WALENSEE_COAST_SOURCE.json').read_text())
    if remote.size != coast['archive_size'] or remote.etag != coast['etag']:
        raise ValueError('Pinned national archive changed')
    files = []
    with zipfile.ZipFile(remote) as archive:
        for base in BASES:
            for ext in ('CPG','prj','shx','dbf','shp'):
                name = base + '.' + ext
                info = archive.getinfo('TLM_BB/' + name)
                path = RAW / name
                if not path.exists():
                    print('FETCH', name, info.file_size, flush=True)
                    temporary = path.with_suffix(path.suffix + '.tmp')
                    # Chunked reads keep HTTP ranges and memory bounded. ZipFile
                    # checks the member CRC when its final bytes have been read.
                    with archive.open(info) as src, temporary.open('wb') as dst:
                        shutil.copyfileobj(src, dst, length=8*1024*1024)
                    temporary.replace(path)
                item = dict(file=name, size=path.stat().st_size, sha256=digest(path), crc32=info.CRC)
                if item['size'] != info.file_size or (previous and item not in previous['files']):
                    raise ValueError('Cached source differs: ' + name)
                files.append(item)
                print('VERIFIED', name, flush=True)

    terrain = json.loads((ROOT / 'divingsim/Content/Diving/Geodata/landscape.json').read_text())
    bounds = [terrain[k] for k in ('west','south','east','north')]
    region = box(*bounds)
    polygons, trees, years = [], [], set()
    reader = shapefile.Reader(str(RAW / BASES[0]), encoding='utf-8')
    for item in reader.iterShapeRecords(bbox=bounds):
        record = item.record.as_dict()
        kind = next((key for key,value in CLASSES.items() if value == record['OBJEKTART']), None)
        if kind not in CLASSES:
            continue
        original = force_2d(shape(item.shape.__geo_interface__))
        if not original.is_valid:
            raise ValueError('Invalid source polygon: ' + str(record.get('UUID')))
        clipped = original.intersection(region)
        if clipped.is_empty or clipped.area == 0:
            continue
        properties = dict(uuid=record['UUID'], class_id=kind, class_name=CLASSES[kind],
                          revision_year=record['REVISION_J'], source_oid=item.shape.oid)
        polygons.append(dict(type='Feature', properties=properties, geometry=mapping(clipped)))
        years.add(record['REVISION_J'])
    reader.close()
    reader = shapefile.Reader(str(RAW / BASES[1]), encoding='utf-8')
    for item in reader.iterShapeRecords(bbox=bounds):
        record = item.record.as_dict()
        if record['OBJEKTART'] != 'Einzelbaum':
            raise ValueError('Unexpected individual-tree class')
        e, n = item.shape.points[0]
        # Some Shapefile readers only apply bbox filtering to bbox-bearing shapes.
        if not bounds[0] <= e <= bounds[2] or not bounds[1] <= n <= bounds[3]:
            continue
        trees.append(dict(type='Feature', properties=dict(uuid=record['UUID'],
                          revision_year=record['REVISION_J'], source_oid=item.shape.oid),
                          geometry=dict(type='Point', coordinates=[e, n, item.shape.z[0]])))
        years.add(record['REVISION_J'])
    reader.close()
    outputs = []
    for name, features in (('forest',polygons), ('tree_tops',trees)):
        features.sort(key=lambda f:f['properties']['uuid'])
        if not features or len({f['properties']['uuid'] for f in features}) != len(features):
            raise ValueError('Empty/duplicate vegetation selection')
        path = DEST / (name + '.geojson')
        path.write_text(json.dumps(dict(type='FeatureCollection', features=features), separators=(',',':'))+'\n', encoding='utf-8')
        outputs.append(dict(file=path.relative_to(ROOT).as_posix(), count=len(features), bytes=path.stat().st_size, sha256=digest(path)))
    report = dict(url=URL, archive_size=remote.size, etag=remote.etag, files=files, outputs=outputs,
                  bounds=bounds, classes=CLASSES, feature_revision_years=sorted(years),
                  horizontal_crs='EPSG:2056', vertical_crs='EPSG:5728', attribution='© swisstopo',
                  tree_z='Surveyed crown or top elevation LN02; not terrain elevation or tree height.',
                  catalogue='https://www.swisstopo.admin.ch/dam/de/sd-web/A3kQ2dAgenqG/2026-02%20swissTLM3D%202.4%20OK-DE.pdf',
                  licence=coast['licence'])
    MANIFEST.write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print('VEGETATION_SOURCE_PASS', len(polygons), 'polygons,', len(trees), 'surveyed tree tops', flush=True)


if __name__ == '__main__':
    main()
