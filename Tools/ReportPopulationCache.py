"""Bind layout-cache checks and a same-scenario performance comparison."""
import argparse
import json
from pathlib import Path
import re
import subprocess
from PIL import Image
from ReportRocks import ROOT, read, sha, package_check


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path), bytes=path.stat().st_size)


def performance(folder):
    summary = read(folder/'Summary.json')
    host = summary['host']
    assert not host['background_processes'] and not host['editor']
    package_check(dict(executable=host['executable'], sha256=host['executable_sha256'], package_containers=host['package_containers']))
    log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
    assert '-DiveEntry=1' in log and 'DIVE_PERF_CAPTURE_READY' in log
    assert not re.search(r'DIVE_\w*FAIL|Fatal error:', log)
    return dict(summary=summary, files=[record(folder/name) for name in
        ['Host.json', 'Metadata.json', 'Summary.json', 'Capture.csv', 'Runtime.log', 'Frame.png']],
        population_markers=re.findall(r'DIVE_POPULATION_CELL[^\r\n]*', log))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--performance', type=Path, required=True)
    parser.add_argument('--baseline', type=Path, default=ROOT/'artifacts/perf-stream-1080p-q2-20261126-144947-791')
    parser.add_argument('--build-log', type=Path, default=ROOT/'artifacts/population-cache-package-build.log')
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/POPULATION_CACHE_RESULTS.json')
    parser.add_argument('--staged', action='store_true')
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run/'Host.json')
    package_check(host)
    build = args.build_log.resolve()
    assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    launcher = read(run/'Launcher.json')
    assert launcher['status'] == 'PASS' and launcher['exit_code'] == 0
    assert launcher['executable_sha256'].lower() == host['sha256'].lower()
    assert launcher['log_sha256'].lower() == sha(run/'Launcher.log')
    cases = read(run/'Cases.json')
    assert len(cases) == 3 and {c['entry'] for c in cases} == {0, 1, 2}
    evidence = []
    for case in cases:
        folder = run/f"entry{case['entry']}"
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status'] == 'PASS' and not re.search(r'DIVE_\w*FAIL|Fatal error:', log)
        for marker in ['DIVE_SMOKE_PASS', 'DIVE_ROCK_LAYOUT_PASS layouts=25 max=25 fresh_equal=1',
                       'DIVE_HABITAT_PASS', 'DIVE_POPULATION_PASS retained_cells=6 reloaded_cells=9',
                       'DIVE_STEERING_WORLD_PASS species=5 steps_per_species=60', 'DIVE_HABITAT_INSPECTION_PASS views=3']:
            assert marker in log, marker
        counts = [int(n) for n in re.findall(r'DIVE_POPULATION_CELL[^\r\n]* layouts=(\d+)', log)]
        if args.staged:
            assert 'DIVE_POPULATION_STAGED_PASS cells=7,8,9' in log
            assert 'DIVE_POPULATION_NEAR_TELEPORT_PASS immediate=1' in log
        assert counts and max(counts) <= 25
        images = []
        assert {p.name for p in folder.glob('*.png')} == {f'Habitat_0_{i}.png' for i in range(3)}
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as im:
                assert im.size == (1600, 900)
                im.verify()
            images.append(record(path))
        evidence.append(dict(**case, log=record(folder/'Runtime.log'), images=images, layout_counts=counts,
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    sources = []
    for name in ['DiveWorld.h', 'DiveWorld.cpp', 'DivePopulation.cpp', 'DivePopulationTests.cpp', 'DiveEcology.cpp']:
        path = ROOT/'divingsim/Source/DiveSim'/name
        sandbox = ROOT/'Builds/EditorSandbox/divingsim/Source/DiveSim'/name
        assert path.read_bytes().replace(b'\r\n', b'\n') == sandbox.read_bytes().replace(b'\r\n', b'\n')
        committed = subprocess.run(['git', 'show', f"{host['source_head']}:{path.relative_to(ROOT).as_posix()}"],
            cwd=ROOT, check=True, capture_output=True).stdout
        assert path.read_bytes().replace(b'\r\n', b'\n') == committed.replace(b'\r\n', b'\n'), 'Runtime source differs from recorded build revision'
        sources.append(record(path))
    before = performance(args.baseline.resolve())
    after = performance(args.performance.resolve())
    assert after['summary']['host']['executable_sha256'].lower() == host['sha256'].lower()
    a, b = before['summary']['metadata'], after['summary']['metadata']
    for key in ['scenario', 'gpu', 'cpu', 'engine', 'width', 'height', 'quality', 'warmup_seconds',
                'requested_capture_seconds', 'r.ScreenPercentage', 'r.SecondaryScreenPercentage.GameViewport',
                'r.DynamicRes.OperationMode', 'r.VSync', 't.MaxFPS']:
        assert a[key] == b[key], key
    assert a['scenario'] == 'perf-stream'
    assert abs(before['summary']['distance_m']-after['summary']['distance_m']) < 5
    result = dict(status='PASS', scope='Cached/fresh geometry equivalence, bounded lifetime, packaged runtime and measured streaming performance',
        host=host, launcher=launcher, staged=args.staged, build=record(build), source_files=sources, cases=evidence,
        performance_before=before, performance_after=after,
        limitations=['The route configuration is the same; frame timing and fish motion are not identical.',
            'Short measurements do not establish long-term stability or guarantee hitch-free play everywhere.',
            'Geometry, vegetation density and visual quality targets are unchanged by this optimization.'])
    output = args.output.resolve()
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(status='PASS', cases=len(cases), report=str(output))))


if __name__ == '__main__':
    main()
