"""Prepare the UE 5.8.4 CelestialVault cooked-star reference fix.

The original asset is an editor redirector stripped by cook. Use its inspected
destination directly. Epic source and the patch stay in ignored artifacts.
This tool prepares files only; it does not modify or build the engine.
"""
import argparse
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path('Engine/Plugins/Runtime/CelestialVault/Source/CelestialVault/Private/CelestialVaultDaySequenceActor.cpp')
EXPECTED = 'a8b1bb7357502d03199772a99e4572b80edbb6780ce653c77a7783d6d0ca9e84'
OLD = '/CelestialVault/Materials/MI_StarsStar_EnergyConservative.MI_StarsStar_EnergyConservative'
NEW = '/CelestialVault/Materials/MI_Stars_EnergyConservative.MI_Stars_EnergyConservative'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'artifacts/celestial-package-patch')
    args = parser.parse_args()
    engine, output = args.engine.resolve(), args.output.resolve()
    if not output.is_relative_to(ROOT / 'artifacts') or output.is_relative_to(engine):
        raise ValueError('Output must be in checkout artifacts and outside the engine')
    original = (engine / SOURCE).read_bytes()
    if hashlib.sha256(original).hexdigest() != EXPECTED:
        raise ValueError('Unreviewed CelestialVault source revision')
    text = original.decode('utf-8').replace('\r\n', '\n')
    if text.count(OLD) != 1:
        raise ValueError('Expected exactly one legacy star material reference')
    modified = text.replace(OLD, NEW)
    newline = '\r\n' if b'\r\n' in original else '\n'
    data = modified.replace('\n', newline).encode('utf-8')
    destination = output / SOURCE
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(data)
    patch = ''.join(difflib.unified_diff(text.splitlines(True), modified.splitlines(True),
                    fromfile='a/' + SOURCE.as_posix(), tofile='b/' + SOURCE.as_posix(), n=2))
    (output / 'celestial-cooked-star.patch').write_text(patch, encoding='utf-8', newline='\n')
    report = {'status': 'PREPARED_NOT_COMPILED', 'engine': str(engine),
              'file': SOURCE.as_posix(), 'original_sha256': EXPECTED,
              'prepared_sha256': hashlib.sha256(data).hexdigest(),
              'old_object': OLD, 'canonical_object': NEW}
    (output / 'Manifest.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('DIVE_CELESTIAL_PACKAGE_PATCH_PREPARED', output)


if __name__ == '__main__':
    main()
