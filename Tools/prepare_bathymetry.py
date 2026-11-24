"""Download and validate the full open Walensee dataset, preserving its measured grid.

Only Python's standard library is required. No interpolation or invented shoreline
is applied. Outputs are preparation assets; the training scene does not load them yet.
"""
import argparse
import array
import hashlib
import io
import json
from pathlib import Path
import sys
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
ITEM = 'https://data.geo.admin.ch/api/stac/v1/collections/ch.swisstopo.swissbathy3d/items/swissbathy3d_walensee'
ASSET = 'swissbathy3d_walensee_2056_5728.esriasciigrid.zip'


def parse_grid(stream):
    """ESRI rows run north to south; x/y origins denote the southwest cell corner."""
    header = {}
    for _ in range(6):
        line = stream.readline()
        while line and not line.strip():
            line = stream.readline()
        key, value = line.split()
        header[key.lower()] = float(value)
    columns, rows = int(header['ncols']), int(header['nrows'])
    values = array.array('f')
    valid_rows = 0
    for line in stream:
        if not line.strip():
            continue
        row = array.array('f', map(float, line.split()))
        if len(row) != columns:
            raise ValueError(f'Row {valid_rows}: expected {columns}, got {len(row)}')
        values.extend(row)
        valid_rows += 1
    if valid_rows != rows:
        raise ValueError(f'Expected {rows} rows, got {valid_rows}')
    return header, values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, help='Use an already downloaded archive')
    args = parser.parse_args()
    raw = ROOT / 'Geodata/Raw'
    processed = ROOT / 'Geodata/Processed/Walensee'
    raw.mkdir(parents=True, exist_ok=True)
    processed.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(ITEM, timeout=60) as response:
        item = json.load(response)
    asset = item['assets'][ASSET]
    archive = args.archive or raw / ASSET
    if not archive.exists():
        urllib.request.urlretrieve(asset['href'], archive)
    expected = asset['file:checksum']
    if not expected.lower().startswith('1220'):
        raise ValueError('Expected a SHA256 multihash')
    with archive.open('rb') as source:
        actual = hashlib.file_digest(source, 'sha256').hexdigest()
    if actual.lower() != expected[4:].lower():
        raise ValueError('Archive checksum differs from the official STAC catalogue')
    manifest = {
        'schema': 1, 'source': ITEM, 'download': asset['href'], 'sha256': actual,
        'attribution': 'Bathymetry: © swisstopo',
        'licence': 'https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste',
        'horizontal_crs': 'EPSG:2056 (CH1903+ / LV95)',
        'vertical_crs': 'EPSG:5728 (LN02 height)',
        'units': 'metres; absolute elevation, not water depth',
        'encoding': 'little-endian float32; row-major north to south, west to east',
        'cell_location': 'centre = xllcorner + (column + 0.5) * cellsize; yllcorner + (nrows - row - 0.5) * cellsize',
        'nodata_policy': 'Preserved. No synthetic fill, no extrapolation.',
        'tiles': [],
    }
    with zipfile.ZipFile(archive) as bundle:
        for name in sorted(bundle.namelist()):
            if not name.lower().endswith('.asc'):
                continue
            with bundle.open(name) as source:
                header, values = parse_grid(io.TextIOWrapper(source, encoding='ascii', newline=None))
            valid = [v for v in values if v != header['nodata_value']]
            filename = Path(name).stem + '.f32'
            if sys.byteorder != 'little':
                values.byteswap()
            with (processed / filename).open('wb') as output:
                values.tofile(output)
            manifest['tiles'].append(dict(header, file=filename, valid_cells=len(valid),
                                          min_elevation=min(valid) if valid else None,
                                          max_elevation=max(valid) if valid else None))
            print(f'{filename}: {len(valid):,} measured cells')
    (raw / 'walensee-stac.json').write_text(json.dumps(item, indent=2), encoding='utf-8')
    (processed / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    # Commit lightweight provenance while keeping reproducible bulk data out of Git.
    (ROOT / 'Documentation/WALENSEE_DATA.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(f'Prepared {len(manifest["tiles"])} tiles; originals remain unchanged.')


if __name__ == '__main__':
    main()
