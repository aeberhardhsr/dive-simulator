"""Extract numeric Walensee seasonal profiles from Eawag's downloadable ThermDis data.

Input is a locally downloaded Lakes.json. No network is needed at build/runtime.
"""
import argparse
import hashlib
import json
from pathlib import Path


def extract(path):
    raw=path.read_bytes()
    source=json.loads(raw)['Walensee']['Observations']['Temperature regime']
    seasons=['winter','spring','summer','fall']
    depths=source['Profile depths [m]']
    temperatures=[source[f'Temperature profile in {season} [\N{DEGREE SIGN}C]'] for season in seasons]
    assert len(depths)==17 and all(b>a for a,b in zip(depths,depths[1:]))
    assert all(len(row)==len(depths) and all(isinstance(t,(int,float)) and 0<t<30 for t in row) for row in temperatures)
    return dict(model='thermdis_walensee_v1',source_url='https://www.thermdis.eawag.ch/static/templates/json/Lakes.json',
                source_sha256=hashlib.sha256(raw).hexdigest(),retrieved='2026-11-25',
                attribution='Eawag ThermDis; lake temperature observations courtesy of cantonal authorities',
                station_lv03_m=[source['Station X [m]'],source['Station Y [m]']],
                note='Published seasonal profiles, not a current measurement or forecast. Observation years are not stated in this JSON. Only numeric temperature facts are extracted.',
                seasons=seasons,depths_m=depths,temperature_c=temperatures)


def header(data):
    def values(row):return ','.join(format(v,'.12g') for v in row)
    return ('#pragma once\n// Generated numeric facts: Eawag ThermDis, Walensee seasonal temperature profiles.\n'
            '// Provenance: Documentation/WALENSEE_TEMPERATURE.json; regenerate with Tools/import_temperature_profile.py.\n'
            'namespace Dive::WalenseeTemperatureData\n{\n'
            f'inline constexpr int Count={len(data["depths_m"])};\n'
            f'inline constexpr double Depths[Count]={{{values(data["depths_m"])}}};\n'
            'inline constexpr double Profiles[4][Count]={\n'+
            ''.join('    {'+values(row)+'},\n' for row in data['temperature_c'])+'};\n}\n')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('source',type=Path)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1];data=extract(args.source)
    (root/'Documentation/WALENSEE_TEMPERATURE.json').write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')
    (root/'divingsim/Source/DiveSim/Simulation/WalenseeTemperatureData.h').write_text(header(data),encoding='utf-8')
    print('WALENSEE_TEMPERATURE_IMPORTED seasons=4 depths=17')
