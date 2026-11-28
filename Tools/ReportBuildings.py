"""Collect verified building data, build, native, and packaged gameplay evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import re

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'Documentation/BUILDINGS_RESULTS.json')
    args = parser.parse_args()
    cases = json.loads((args.run / 'Cases.json').read_text(encoding='utf-8-sig'))
    expected = {'buildings0', 'buildings1', 'buildings2', 'fallback1', 'shore0', 'shore1', 'shore2',
                'sediment0', 'avatar0', 'breathing', 'wrist', 'stream1'}
    assert {c['name'] for c in cases} == expected
    host = json.loads((args.run / 'Host.json').read_text(encoding='utf-8-sig'))
    executable = Path(host['executable'])
    assert sha(executable).lower() == host['executable_sha256'].lower()
    results = []
    for case in cases:
        folder = args.run / case['name']
        log = (folder / 'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status'] == 'PASS'
        assert 'DIVE_SMOKE_PASS' in log and 'DIVE_BUILDINGS_PASS buildings=16249 cells=715' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used', log)
        if case['name'].startswith('buildings') or case['name'] == 'fallback1':
            assert 'DIVE_BUILDINGS_INSPECTION' in log
        if case['name'] == 'fallback1':
            assert 'r.Nanite = "0"' in log
        images = []
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as image:
                image.load()
                assert image.width >= 512 and image.height >= 512
                images.append(dict(name=path.name, width=image.width, height=image.height, sha256=sha(path)))
        assert len(images) == case['screenshots'] and images
        results.append(dict(name=case['name'], status='PASS', log_sha256=sha(folder / 'Runtime.log'), images=images))
    data = json.loads((ROOT / 'artifacts/buildings-data-validation.json').read_text(encoding='utf-8'))
    assert data['passed'] and data['buildings'] == 16249 and data['triangles'] == 978500
    native = json.loads((ROOT / 'artifacts/BuildingsAutomation/index.json').read_text(encoding='utf-8-sig'))
    assert native['succeeded'] == 30 and native['failed'] == native['succeededWithWarnings'] == native['notRun'] == 0
    snapshot = []
    for directory in ['Source', 'Config']:
        for path in sorted((ROOT / 'divingsim' / directory).rglob('*')):
            if path.is_file():
                sandbox = ROOT / 'Builds/EditorSandbox/divingsim' / path.relative_to(ROOT / 'divingsim')
                assert sha(path) == sha(sandbox), path
                snapshot.append(dict(path=path.relative_to(ROOT).as_posix(), sha256=sha(path)))
    previous = json.loads((ROOT / 'Documentation/HEAD_TURN_BUILD.json').read_text(encoding='utf-8'))
    assert sha(Path(previous['executable'])) == previous['executable_sha256']
    for key in ['unchanged_avatar_assets', 'unchanged_ecology_assets']:
        for item in previous[key]:
            path = Path(item['path'])
            assert sha(path if path.is_absolute() else ROOT / path) == item['sha256']
    package = (ROOT / 'artifacts/buildings-package.log').read_text(encoding='utf-8-sig')
    assert 'BUILD SUCCESSFUL' in package and 'DIVE_BUILDINGS_BAKE_PASS' in package
    report = dict(status='PASS', executable=str(executable), executable_sha256=sha(executable),
                  package_archives=[dict(path=str(p), sha256=sha(p)) for p in sorted((executable.parents[2] / 'Content/Paks').iterdir()) if p.is_file()],
                  source_snapshot=snapshot, source_validation=data, native_tests=30, cases=results,
                  evidence=[dict(path='artifacts/' + name, sha256=sha(ROOT / 'artifacts' / name)) for name in
                            ['buildings-editor-build-final.log', 'buildings-package.log', 'buildings-mesh-build.log',
                             'BuildingsAutomation/index.json', 'buildings-data-validation.log',
                             'buildings-python-tests.log', 'buildings-core-tests.log']],
                  images=sum(len(c['images']) for c in results),
                  preserved_headturn_executable_sha256=previous['executable_sha256'],
                  unchanged_avatar_assets=len(previous['unchanged_avatar_assets']),
                  unchanged_ecology_assets=len(previous['unchanged_ecology_assets']),
                  limits=['Elevated building views are inspection cameras, not ordinary player positions.',
                          'Generic facades, no interiors; terrain and forest detail remain incomplete.',
                          'Performance and launcher validation are separate; no native macOS gameplay validation.'])
    retry_log = ROOT / 'artifacts/buildings-shore0-missing-capture.log'
    if retry_log.exists():
        report['retry_note'] = dict(log_sha256=sha(retry_log),
            reason='First shore0 runtime passed but the required ShoreDive capture was absent; the case was repeated and the complete image set checked.')
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print('PASS', len(results), 'cases', report['images'], 'images', sha(executable))


if __name__ == '__main__':
    main()
