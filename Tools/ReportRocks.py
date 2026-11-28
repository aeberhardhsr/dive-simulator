"""Bind packaged rock comparisons and gameplay checks to their actual inputs."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def package_check(host):
    exe = Path(host['executable'])
    assert sha(exe) == host['sha256'].lower(), 'Executable changed'
    containers = [dict(file=p.name, bytes=p.stat().st_size, sha256=sha(p))
                  for p in sorted((exe.parents[2]/'Content/Paks').iterdir()) if p.is_file()]
    assert containers and containers == sorted(host['package_containers'], key=lambda x: x['file']), 'Cooked content changed'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--launcher-observation', type=Path,
                        help='Read-only evidence of the user-started root launcher and its actual child')
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run/'Host.json')
    package_check(host)
    cases = read(run/'Cases.json')
    expected = {'forms', 'forms-raster', 'stream0', 'stream1', 'stream2', 'shore0', 'shore1', 'shore2', 'discoveries'}
    assert len(cases) == len(expected) and {c['name'] for c in cases} == expected
    evidence = []
    for case in cases:
        folder = run/case['name']
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used', log)
        if case['scenario'] == 'rocks':
            assert 'DIVE_ROCK_INSPECTION_PASS forms=4 views=2 captures=8' in log
            assert {p.name for p in folder.glob('*.png')} == {f'Rock_{v}_{a}.png' for v in range(4) for a in range(2)}
        if case['raster']:
            assert 'r.Nanite = "0"' in log
        if case['scenario'] == 'stream':
            assert 'DIVE_POPULATION_PASS' in log and 'DIVE_STEERING_WORLD_PASS species=5 steps_per_species=60' in log
        if case['scenario'] == 'discoveries':
            assert 'DIVE_DISCOVERY_VISIBILITY_PASS' in log
        images = []
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as im:
                assert im.size in [(1600, 900), (768, 512)]
                im.verify()
            images.append(dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path)))
        assert images and len(images) == case['screenshots']
        evidence.append(dict(**case, log_sha256=sha(folder/'Runtime.log'), images=images))
    sources = read(ROOT/'Documentation/ROCK_ASSETS.json')
    geometry = read(ROOT/'blender/exports/Rocks/Manifest.json')
    content = read(ROOT/'artifacts/RockContentBuild.json')
    render_audit = read(ROOT/'artifacts/RockRenderAudit.json')
    assert len(sources['assets']) == len(geometry['assets']) == len(content) == 3
    for source, mesh, asset in zip(sources['assets'], geometry['assets'], content):
        assert sha(ROOT/source['file']) == source['sha256'] == mesh['source_sha256']
        assert sha(ROOT/mesh['file']) == mesh['sha256'] == asset['source_sha256']
        assert mesh['nonmanifold_edges'] == mesh['closed_source_boundary_edges'] == 0
        assert mesh['maximum_radius'] <= 1.00001
        assert asset['lods'] == 4 and asset['nanite'] and asset['collision_hulls'] > 0
        assert asset['name'] == mesh['name']
    assert len(render_audit) == 3
    for mesh, audit in zip(geometry['assets'], render_audit):
        assert audit['name'] == mesh['name'] and audit['triangles'][0] == mesh['triangles']
        assert 'PERCENT_TRIANGLES' in audit['fallback_target']
    files = []
    for directory in ['Source', 'Config']:
        for path in sorted((ROOT/'divingsim'/directory).rglob('*')):
            if not path.is_file():
                continue
            relative = path.relative_to(ROOT/'divingsim')
            data = path.read_bytes().replace(b'\r\n', b'\n')
            assert data == (ROOT/'Builds/EditorSandbox/divingsim'/relative).read_bytes().replace(b'\r\n', b'\n')
            files.append(dict(file=relative.as_posix(), canonical_lf_sha256=hashlib.sha256(data).hexdigest()))
    assets = list((ROOT/'divingsim/Content/Diving/Rocks').glob('*.uasset'))
    assert len(assets) == 3
    assets.append(ROOT/'divingsim/Content/Diving/Materials/M_RockSurface.uasset')
    logs = {}
    editor_startup_issues = []
    for name, marker in [('rock-content-full.log', 'DIVE_ROCK_CONTENT_PASS'),
                         ('rock-editor-inspection.log', 'DIVE_ROCK_INSPECTION_PASS'),
                         ('rock-editor-stream.log', 'DIVE_POPULATION_PASS'),
                         ('rock-render-audit.log', 'DIVE_ROCK_RENDER_AUDIT_PASS'),
                         ('rock-editor-raster-full.log', 'DIVE_ROCK_INSPECTION_PASS'),
                         ('rock-package.log', 'BUILD SUCCESSFUL')]:
        path = ROOT/'artifacts'/name
        text = path.read_text(encoding='utf-8-sig')
        assert marker in text and not re.search(r'DIVE_\w+_FAIL|Default Material will be used', text), name
        errors = re.findall(r'LogPython: Error: (.*)', text)
        if errors:
            # The editor's optional ToolsetRegistry test registration runs in
            # -game too, where PythonTestRunner is absent. Accept only this
            # exact engine-only traceback; import/audit commandlets stay strict.
            assert name.startswith('rock-editor-') and len(errors) == 5, (name, errors)
            assert errors[0] == 'Traceback (most recent call last):'
            assert errors[1].endswith('/Engine/Plugins/Experimental/ToolsetRegistry/Content/Python/init_unreal.py", line 7, in <module>')
            assert errors[2].strip() == 'tests._toolset_registry_test_runner = unreal.PythonTestRunner.create('
            assert set(errors[3].strip()) == {'^'}
            assert errors[4] == "AttributeError: module 'unreal' has no attribute 'PythonTestRunner'"
            editor_startup_issues.append(dict(log=name, error=errors[4], source='Installed engine ToolsetRegistry init_unreal.py:7'))
        logs[name] = sha(path)
    if args.launcher_observation:
        launcher = read(args.launcher_observation)
        assert launcher['status'] == 'OBSERVED_RUNNING'
        assert launcher['child_parent_id'] == launcher['launcher_id']
        assert Path(launcher['child']).resolve() == Path(host['executable']).resolve()
        assert launcher['executable_sha256'].lower() == host['sha256'].lower()
        assert Path(launcher['launcher']).resolve() == Path(host['executable']).parents[3]/'divingsim.exe'
        assert sha(Path(launcher['launcher'])) == launcher['launcher_sha256'].lower()
    else:
        launcher = read(ROOT/'artifacts/rock-bootstrap.json')
        assert launcher['status'] == 'PASS' and launcher['executable_sha256'].lower() == host['sha256'].lower()
        assert sha(ROOT/'artifacts/rock-bootstrap.log') == launcher['log_sha256'].lower()
    baseline = read(ROOT/'Documentation/FISH_STEERING_RESULTS.json')['executable']
    package_check(baseline)
    ci = read(ROOT/'Documentation/ROCK_GEOMETRY_CI.json')
    assert ci['source_commit'] == host['source_head']
    assert len(ci['jobs']) == 3 and all(j['status'] == 'completed' and j['conclusion'] == 'success' for j in ci['jobs'])
    report = dict(status='PASS', scope='Packaged rendering and gameplay regression; performance and endurance acceptance pending',
                  executable=host, cases=evidence, sources=files, geometry=geometry, content_build=content, render_data_audit=render_audit,
                  assets=[dict(file=p.relative_to(ROOT).as_posix(), sha256=sha(p)) for p in sorted(assets)],
                  logs=logs, editor_startup_issues=editor_startup_issues, root_launcher=launcher, preserved_baseline=baseline, source_ci=ci,
                  limits='Three rescaled CC0 forms with authored placement and shared lake-rock material, not local geology scans. Convex diver collision and conservative fish exclusion. Fixed-camera Nanite/raster comparisons do not establish every LOD transition or overall photorealism. Concurrent baseline soak is recorded in Host.json; these capped runs are not performance measurements.')
    (ROOT/'Documentation/ROCK_GEOMETRY_RESULTS.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(f'ROCK_REPORT_PASS cases={len(cases)} images={sum(c["screenshots"] for c in cases)}')


if __name__ == '__main__':
    main()
