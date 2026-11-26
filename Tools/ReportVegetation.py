"""Bind packaged vegetation evidence, native tests and compiled inputs to bytes."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--prior-failure', type=Path)
    parser.add_argument('--prior-controls-failure', type=Path)
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run/'Host.json')
    assert host['packaged'] and sha(Path(host['executable'])) == host['sha256'].lower()
    cases = read(run/'Cases.json')
    expected = {'weesen-land', 'murg-land', 'mols-land', 'murg-fallback', 'weesen-fallback', 'mols-water-4k',
                'weesen-shore', 'murg-shore', 'mols-shore', 'weesen-crossing',
                'murg-crossing', 'walenstadt-crossing', 'discoveries', 'controls'}
    assert len(cases) == len(expected) and {c['name'] for c in cases} == expected
    evidence = []
    for case in cases:
        folder = run/case['name']
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used', log)
        assert 'DIVE_VEGETATION_READY candidates=810598 cells=4042 models=5' in log
        match = re.search(r'DIVE_VEGETATION_PASS cells=(\d+) instances=(\d+) trunk_probes=(\d+)', log)
        assert match and int(match[2]) > 0 and int(match[1]) <= 121
        if case['name'] == 'weesen-land':
            assert re.search(r'DIVE_VEGETATION_STREAM_PASS retained=1 retired=1 returned=1 transforms=[1-9]\d*', log)
            assert int(match[3]) > 0
        if case['scenario'] == 'coast':
            assert f"DIVE_COAST_ROUTE_PASS entry={case['entry']} crossed_unmeasured=1 returned_measured=1" in log
            assert 'DIVE_TEST_KEYS_FLUSHED coast=1' in log
            assert log.count('DIVE_TEST_KEY_RESTORED') >= 2
        if case['scenario'] == 'shore':
            assert 'DIVE_SHORE_PASS' in log
        if case['scenario'] == 'discoveries':
            assert 'DIVE_DISCOVERY_PASS phases=14' in log
        if case['scenario'] == 'controls':
            assert 'DIVE_INPUT_PASS' in log
            restored = re.findall(r'DIVE_TEST_KEY_RESTORED key=(\S+)', log)
            assert not {'T', 'L'}.intersection(restored)
        if case['fallback']:
            assert any(marker in log for marker in ('r.Nanite = "0"', 'r.Nanite 0', 'r.Nanite = 0'))
        size = tuple(map(int, case['viewport'].split('x')))
        images = []
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as image:
                assert image.size == size
                image.verify()
            images.append(dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path)))
        assert len(images) == case['screenshots'] > 0
        evidence.append(dict(**case, images=images, runtime_sha256=sha(folder/'Runtime.log'),
                             cells=int(match[1]), instances=int(match[2]), trunk_probes=int(match[3])))
    native = read(ROOT/'artifacts/VegetationAutomation/index.json')
    assert native['succeeded'] == 35 and native['failed'] == 0
    build_log = ROOT/'artifacts/vegetation-package-final.log'
    assert 'BUILD SUCCESSFUL' in build_log.read_text(encoding='utf-8-sig')
    sources = []
    project = ROOT/'divingsim'
    sandbox = ROOT/'Builds/EditorSandbox/divingsim'
    for directory in ('Source', 'Config'):
        for path in sorted((project/directory).rglob('*')):
            if not path.is_file():
                continue
            relative = path.relative_to(project)
            canonical = path.read_bytes().replace(b'\r\n', b'\n')
            assert canonical == (sandbox/relative).read_bytes().replace(b'\r\n', b'\n'), str(relative)
            sources.append(dict(file=relative.as_posix(), canonical_lf_sha256=hashlib.sha256(canonical).hexdigest()))
    assets = [dict(file=p.relative_to(project).as_posix(), bytes=p.stat().st_size, sha256=sha(p))
              for p in sorted((project/'Content/Diving/Vegetation').rglob('*.uasset'))]
    containers = [dict(file=p.name, bytes=p.stat().st_size, sha256=sha(p))
                  for p in sorted((ROOT/'Builds/WindowsVegetation/divingsim/Content/Paks').iterdir()) if p.is_file()]
    assert sorted(host['package_containers'], key=lambda p:p['file']) == sorted(containers, key=lambda p:p['file'])
    baseline = ROOT/'Builds/WindowsCoast/divingsim/Binaries/Win64/divingsim-Coast.exe'
    assert sha(baseline) == '8be183006ebc405e7cffff0ec8529f31e798d24fdab2c9743afb3f909f7c6932'
    audits = {}
    for name in ('VEGETATION_SOURCE_AUDIT', 'VEGETATION_PLACEMENT_AUDIT'):
        path = ROOT/f'Documentation/{name}.json'
        assert read(path)['status'] == 'PASS'
        audits[name] = sha(path)
    assert read(ROOT/'Documentation/VEGETATION_SOURCE_AUDIT.json')['manifest_sha256'] == sha(ROOT/'Documentation/WALENSEE_VEGETATION_SOURCE.json')
    assert read(ROOT/'Documentation/VEGETATION_PLACEMENT_AUDIT.json')['binary_sha256'] == sha(project/'Content/Diving/Geodata/vegetation.trees')
    report = dict(status='PASS', executable=host, cases=evidence, native_tests=dict(succeeded=35, failed=0),
                  package_log_sha256=sha(build_log), sources=sources, assets=assets, package_containers=containers,
                  audits=audits, baseline_executable_sha256=sha(baseline),
                  candidate_sha256=sha(project/'Content/Diving/Geodata/vegetation.trees'),
                  scope='Packaged views, fallback and 4K rendering, cell retention/retirement/return, trunk ray collisions, wind/time uniforms and existing gameplay routes.',
                  limits='The streaming probe moves the focus, not a player over 3.5 km. Scripted input is not manual acceptance. No minimum-hardware, long-session or native macOS game certification.')
    launcher = read(ROOT/'artifacts/vegetation-bootstrap.json')
    assert launcher['status'] == 'PASS' and launcher['executable_sha256'].lower() == host['sha256'].lower()
    report['root_launcher'] = launcher
    if args.prior_failure:
        failed = args.prior_failure.resolve()
        text = failed.read_text(encoding='utf-8-sig')
        assert 'DIVE_COAST_ROUTE_FAIL phase=2 Movement timed out' in text
        report['input_regression_recovery'] = dict(
            prior_runtime=str(failed), prior_runtime_sha256=sha(failed),
            prior_failure='Walenstadt movement timed out after intended synthetic key holds were lost.',
            diagnosis_limit='The old log did not record the desktop focus event; loss of input was inferred from stopped propulsion without collision.',
            verified_fix='All three current coast routes deliberately flush engine key state, restore the held keys and complete the crossing and return.')
    if args.prior_controls_failure:
        failed = args.prior_controls_failure.resolve()
        text = failed.read_text(encoding='utf-8-sig')
        assert 'DIVE_INPUT_FAIL' in text and 'DIVE_TEST_KEY_RESTORED key=T' in text
        report.setdefault('input_regression_recovery', {})['toggle_guard'] = dict(
            prior_runtime=str(failed), prior_runtime_sha256=sha(failed),
            cause='The first driver fix also replayed toggle actions consumed before Unreal held-key tracking.',
            correction='Only continuous movement and buoyancy actions can be restored. The current remapping/lamp/regulator test passes without replaying toggle keys.')
    (ROOT/'Documentation/VEGETATION_RESULTS.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('PASS', len(cases), 'cases;', sum(len(c['images']) for c in evidence), 'images;', len(assets), 'assets')


if __name__ == '__main__':
    main()
