"""Prepare measured 1 m Walensee heights for the Unreal runtime (© swisstopo)."""
import array
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Geodata/Processed/Walensee'
DESTINATION = ROOT / 'divingsim/Content/Diving/Geodata'
WATER_ELEVATION = 419.0  # Scenario reference, not today's measured water level.


def main():
    manifest = json.loads((SOURCE / 'manifest.json').read_text(encoding='utf-8'))
    DESTINATION.mkdir(parents=True, exist_ok=True)
    for tile in manifest['tiles']:
        values = array.array('f')
        values.frombytes((SOURCE / tile['file']).read_bytes())
        if sys.byteorder != 'little':
            values.byteswap()
        heights = array.array('h')
        for value in values:
            height = -32768 if value == tile['nodata_value'] else round((value-WATER_ELEVATION)*100)
            if height < -32768 or height > 32767:
                raise ValueError('Measured elevation exceeds the runtime encoding range')
            heights.append(height)
        tile['file'] = Path(tile['file']).with_suffix('.i16').name
        tile['nodata_value'] = -32768
        if sys.byteorder != 'little':
            heights.byteswap()
        (DESTINATION / tile['file']).write_bytes(heights.tobytes())
    manifest.update(schema=2, water_elevation_m=WATER_ELEVATION, nodata=-32768,
                    encoding='little-endian int16 centimetres relative to water_elevation_m; north-to-south rows',
                    units='centimetres relative to the scenario water surface',
                    origin_east=2726000, origin_north=1219000)
    # Selected in measured shallow water, with an 8 m surveyed neighbourhood.
    # Game starts offshore; these markers do not claim to be real public access points.
    manifest['entries'] = [
        dict(name='Weesen / West', east=2726086.5, north=1221674.5),
        dict(name='Murg / Sued', east=2734890.5, north=1219849.5),
        dict(name='Walenstadt / Ost', east=2741110.5, north=1220619.5),
    ]
    (DESTINATION / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    (DESTINATION / 'NOTICE.txt').write_text(
        'Lakebed: © swisstopo, swissBATHY3D Walensee, survey 2020.\n'
        'https://www.swisstopo.admin.ch/de/hoehenmodell-swissbathy3d\n'
        'https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste\n'
        '1 m survey grid; centimetre quantisation (at most 0.005 m error).\n'
        'Water surface: scenario reference 419 m LN02, not live hydrological data.\n'
        'NoData remains absent; no invented bathymetric infill.\n', encoding='utf-8')
    print(f'Prepared {len(manifest["tiles"])} runtime tiles at 1 m resolution.')


if __name__ == '__main__':
    main()
