"""Fetch the version-pinned CC0 mature beech and build-only decoder dependencies."""
import hashlib
import json
from pathlib import Path
from prepare_vegetation_assets import read_url

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'Documentation/VEGETATION_BEECH_SOURCE.json'
DEPENDENCIES = [
    ('meshopt_decoder.mjs', 'https://cdn.jsdelivr.net/npm/meshoptimizer@0.24.0/meshopt_decoder.module.js', 'd7ecda608bd766b28abd3009dc740eeae3f4f72414b2eab460bb3c890815d5dd'),
    ('basis_transcoder.cjs', 'https://cdn.jsdelivr.net/npm/three@0.180.0/examples/jsm/libs/basis/basis_transcoder.js', '8478b5b6d6b74e7d3082b89f6417321d8d1dc0307f2b30d4484bb11b441696a1'),
    ('basis_transcoder.wasm', 'https://cdn.jsdelivr.net/npm/three@0.180.0/examples/jsm/libs/basis/basis_transcoder.wasm', '6cf17dc889352c42e9acf8897107978d127005fe3386c36a0e3845e27967630a'),
]


def main():
    if MANIFEST.exists():
        report = json.loads(MANIFEST.read_text())
    else:
        metadata = json.loads(read_url('https://tevy.dev/models/european-beech.json'))
        version = next(v for v in metadata['versions'] if v['version'] == 1)
        if metadata['license'] != 'CC0-1.0':
            raise ValueError('Unexpected asset license')
        report = dict(source='https://tevy.dev/models/european-beech', license='CC0-1.0', author='tevy',
                      version=1, files=version['files'], local_species_distribution_claim=False)
    for item in report['files']:
        name = Path(item['path']).name
        path = ROOT / 'Art/Vegetation/european_beech' / name
        path.parent.mkdir(parents=True, exist_ok=True)
        data = path.read_bytes() if path.exists() else read_url('https://tevy.dev' + item['path'])
        if len(data) != item['bytes'] or hashlib.sha256(data).hexdigest() != item['sha256']:
            raise ValueError('Beech source checksum mismatch')
        path.write_bytes(data)
    for name, url, sha in DEPENDENCIES:
        path = ROOT / 'Tools/.downloads/tree-decoders' / name
        path.parent.mkdir(parents=True, exist_ok=True)
        data = path.read_bytes() if path.exists() else read_url(url)
        if hashlib.sha256(data).hexdigest() != sha:
            raise ValueError('Decoder checksum mismatch')
        path.write_bytes(data)
    report['build_only_decoders'] = [dict(file=n, url=u, sha256=s) for n,u,s in DEPENDENCIES]
    MANIFEST.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('BEECH_SOURCE_PASS 3 LODs; pinned decoders verified')


if __name__ == '__main__':
    main()
