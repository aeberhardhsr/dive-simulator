"""Verify evidence for the packaged underwater population revision."""
import argparse
import json
from pathlib import Path
import re
from PIL import Image
from ReportRocks import ROOT, read, sha, package_check


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path), bytes=path.stat().st_size)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--performance', type=Path, action='append', default=[])
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run/'Host.json')
    package_check(host)
    build = ROOT/'artifacts/habitat-package-build.log'
    assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    launcher = read(run/'Launcher.json')
    assert launcher['status'] == 'PASS' and launcher['exit_code'] == 0
    assert launcher['executable_sha256'].lower() == host['sha256'].lower()
    assert launcher['log_sha256'].lower() == sha(run/'Launcher.log')
    cases = read(run/'Cases.json')
    expected = {f'{kind}{i}' for kind in ['look', 'stream', 'shore'] for i in range(3)} | {'look-raster', 'sediment', 'navigation'}
    assert len(cases) == len(expected) and {c['name'] for c in cases} == expected
    evidence = []
    for case in cases:
        folder = run/case['name']
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
        if case['scenario'] == 'rocks':
            assert 'DIVE_HABITAT_INSPECTION_PASS views=3' in log
            assert {p.name for p in folder.glob('*.png')} == {f'Habitat_0_{i}.png' for i in range(3)}
        if case['scenario'] == 'stream':
            for marker in ['DIVE_HABITAT_PASS', 'DIVE_POPULATION_PASS retained_cells=6 reloaded_cells=9',
                           'DIVE_STEERING_WORLD_PASS species=5 steps_per_species=60', 'DIVE_GRAVEL_PASS retained=6 reloaded=9']:
                assert marker in log, marker
            detail = re.search(r'DIVE_SMOKE_DETAIL entry=(\d) scenario=stream displacement=([\d.]+)', log)
            assert detail and int(detail[1]) == case['entry'] and float(detail[2]) > 80
        if case['scenario'] == 'shore':
            assert 'DIVE_SHORE_SURFACE' in log and 'DIVE_MEDIUM_PASS phase=air' in log
        if case['name'] == 'look-raster':
            assert 'r.Nanite = "0"' in log
        if case['scenario'] == 'navigation':
            assert 'DIVE_NAV_PASS' in log
        if case['scenario'] == 'sediment':
            assert 'DIVE_SEDIMENT_PASS' in log and 'DIVE_SEDIMENT_PAUSE_PASS' in log
        images = []
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as im:
                assert im.size == (1600, 900)
                im.verify()
            images.append(record(path))
        assert images and len(images) == case['screenshots']
        evidence.append(dict(**case, log=record(folder/'Runtime.log'), images=images,
            habitat_markers=re.findall(r'DIVE_HABITAT_PASS[^\r\n]*', log),
            population_markers=re.findall(r'DIVE_POPULATION_CELL[^\r\n]*', log),
            ordinary_cell_crossings=re.findall(r'DIVE_POPULATION_CELL[^\r\n]*', log.split('DIVE_POPULATION_TEST_END', 1)[1]) if case['scenario'] == 'stream' else [],
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    assert any(case['ordinary_cell_crossings'] for case in evidence), 'No real gameplay cell crossing'
    sources = []
    for name in ['DivePopulation.cpp', 'DivePopulationTests.cpp', 'DiveRockInspection.cpp']:
        path = ROOT/'divingsim/Source/DiveSim'/name
        sandbox = ROOT/'Builds/EditorSandbox/divingsim/Source/DiveSim'/name
        assert path.read_bytes().replace(b'\r\n', b'\n') == sandbox.read_bytes().replace(b'\r\n', b'\n')
        sources.append(record(path))
    # Baseline and current native views use the same fixture and terrain-only
    # camera selection; this checks the recorded poses, not visual quality.
    comparisons = []
    poses = []
    for phase in ['habitat-baseline', 'habitat-refined']:
        folder = ROOT/'artifacts'/phase
        log = (folder/'entry0.log').read_text(encoding='utf-8-sig')
        poses.append(re.findall(r'DIVE_HABITAT_VIEW[^\r\n]*', log))
        comparisons.append(dict(phase=phase, log=record(folder/'entry0.log'),
            images=[record(folder/'entry0'/f'Habitat_0_{i}.png') for i in range(3)]))
    assert len(poses[0]) == 3 and poses[0] == poses[1]
    performance = []
    for path in args.performance:
        path = path.resolve()
        summary = read(path/'Summary.json')
        assert summary['host']['executable_sha256'].lower() == host['sha256'].lower()
        assert not summary['host']['background_processes'], 'Concurrent Unreal game/editor in benchmark'
        assert summary['host']['package_containers'] == host['package_containers']
        performance.append(dict(summary=summary, files=[record(path/name) for name in
            ['Host.json', 'Metadata.json', 'Summary.json', 'Capture.csv', 'Runtime.log', 'Frame.png']]))
    result = dict(status='PASS', scope='Runtime checks; visual target remains open', host=host,
        launcher=launcher, build=record(build), source_files=sources, cases=evidence,
        screenshots=sum(c['screenshots'] for c in cases), native_comparisons=comparisons, performance=performance,
        limitations=['Authored rock and plant placement; not surveyed habitat distribution.',
            'The screenshots still fall short of the supplied realism reference, especially substrate, vegetation surfaces and lighting.',
            'Short performance captures do not establish long-term stability or performance everywhere.',
            'Underwater and streaming scenarios use different views and depths; their FPS are not a controlled object-density comparison.',
            'Menu/HUD/FPV redesign follows underwater and above-water graphics work.'])
    output = ROOT/'Documentation/HABITAT_RESULTS.json'
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(status='PASS', cases=len(cases), screenshots=result['screenshots'], report=str(output))))


if __name__ == '__main__':
    main()
