"""Verify evidence for the packaged underwater population revision."""
import argparse
import json
from pathlib import Path
import re
import subprocess
from PIL import Image
from ReportRocks import ROOT, read, sha, package_check


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path), bytes=path.stat().st_size)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--performance', type=Path, action='append', default=[])
    parser.add_argument('--build', type=Path, default=ROOT/'artifacts/habitat-package-build.log')
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/HABITAT_RESULTS.json')
    parser.add_argument('--baseline-run', type=Path, help='Packaged ViewPath run with habitat0/1/2 cameras')
    parser.add_argument('--formations', action='store_true', help='Require connected outcrop checks at all entries')
    parser.add_argument('--prior-run', type=Path, help='Preserve an earlier failed package validation')
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run/'Host.json')
    package_check(host)
    build = args.build.resolve()
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
            if args.formations:
                formation = re.search(r'DIVE_FORMATION_PASS connected_large_pairs=(\d+)', log)
                assert formation and int(formation[1]) > 8, 'Missing connected outcrop check'
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
            formation_markers=re.findall(r'DIVE_FORMATION_PASS[^\r\n]*', log),
            habitat_markers=re.findall(r'DIVE_HABITAT_PASS[^\r\n]*', log),
            population_markers=re.findall(r'DIVE_POPULATION_CELL[^\r\n]*', log),
            ordinary_cell_crossings=re.findall(r'DIVE_POPULATION_CELL[^\r\n]*', log.split('DIVE_POPULATION_TEST_END', 1)[1]) if case['scenario'] == 'stream' else [],
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    assert any(case['ordinary_cell_crossings'] for case in evidence), 'No real gameplay cell crossing'
    sources = []
    for name in ['DivePopulation.cpp', 'DivePopulationTests.cpp', 'DiveRockInspection.cpp'] + (['DivePawn.cpp'] if args.formations else []):
        path = ROOT/'divingsim/Source/DiveSim'/name
        sandbox = ROOT/'Builds/EditorSandbox/divingsim/Source/DiveSim'/name
        assert path.read_bytes().replace(b'\r\n', b'\n') == sandbox.read_bytes().replace(b'\r\n', b'\n')
        if args.formations:
            committed = subprocess.check_output(['git', 'show', f"{host['source_head']}:{path.relative_to(ROOT).as_posix()}"], cwd=ROOT)
            assert committed.replace(b'\r\n', b'\n') == path.read_bytes().replace(b'\r\n', b'\n'), 'Source differs from tested commit'
        sources.append(record(path))
    # Baseline and current native views use the same fixture and terrain-only
    # camera selection; this checks the recorded poses, not visual quality.
    comparisons = []
    poses = []
    for phase in ([] if args.baseline_run else ['habitat-baseline', 'habitat-refined']):
        folder = ROOT/'artifacts'/phase
        log = (folder/'entry0.log').read_text(encoding='utf-8-sig')
        poses.append(re.findall(r'DIVE_HABITAT_VIEW[^\r\n]*', log))
        comparisons.append(dict(phase=phase, log=record(folder/'entry0.log'),
            images=[record(folder/'entry0'/f'Habitat_0_{i}.png') for i in range(3)]))
    if args.baseline_run:
        baseline = args.baseline_run.resolve()
        baseline_host = read(baseline/'Host.json')
        package_check(baseline_host)
        for entry in range(3):
            before, after = baseline/f'habitat{entry}', run/f'look{entry}'
            before_poses = re.findall(r'DIVE_HABITAT_VIEW[^\r\n]*', (before/'Runtime.log').read_text(encoding='utf-8-sig'))
            after_poses = re.findall(r'DIVE_HABITAT_VIEW[^\r\n]*', (after/'Runtime.log').read_text(encoding='utf-8-sig'))
            assert len(before_poses) == 3 and before_poses == after_poses, 'Comparison camera mismatch'
            comparisons.append(dict(entry=entry, baseline_host=baseline_host,
                before_log=record(before/'Runtime.log'), after_log=record(after/'Runtime.log'),
                before_images=[record(before/f'Habitat_0_{i}.png') for i in range(3)],
                after_images=[record(after/f'Habitat_0_{i}.png') for i in range(3)]))
    else:
        assert len(poses[0]) == 3 and poses[0] == poses[1]
    published_images = []
    if args.formations and args.baseline_run:
        for name, original in [('Before', args.baseline_run.resolve()/'habitat2/Habitat_0_2.png'),
                               ('After', run/'look2/Habitat_0_2.png')]:
            published = ROOT/f'Documentation/Images/Formations_{name}.png'
            assert sha(published) == sha(original), 'Published comparison differs from the captured image'
            published_images.append(record(published))
    performance = []
    for path in args.performance:
        path = path.resolve()
        summary = read(path/'Summary.json')
        assert summary['host']['executable_sha256'].lower() == host['sha256'].lower()
        assert not summary['host']['background_processes'], 'Concurrent Unreal game/editor in benchmark'
        assert summary['host']['package_containers'] == host['package_containers']
        performance.append(dict(summary=summary, files=[record(path/name) for name in
            ['Host.json', 'Metadata.json', 'Summary.json', 'Capture.csv', 'Runtime.log', 'Frame.png']]))
    prior_attempt = None
    if args.prior_run:
        prior = args.prior_run.resolve()
        prior_host = read(prior/'Host.json')
        package_check(prior_host)
        failures = []
        for log_path in sorted(prior.glob('*/Runtime.log')):
            markers = re.findall(r'DIVE_\w*FAIL[^\r\n]*', log_path.read_text(encoding='utf-8-sig'))
            if markers:
                failures.append(dict(log=record(log_path), markers=markers))
        assert failures, 'Prior attempt does not contain a recorded failure'
        prior_attempt = dict(host=prior_host, completed_cases=read(prior/'Cases.json'), failures=failures)
    result = dict(status='PASS', scope='Runtime checks; visual target remains open', host=host,
        prior_attempt=prior_attempt,
        launcher=launcher, build=record(build), source_files=sources, cases=evidence,
        screenshots=sum(c['screenshots'] for c in cases), comparisons=comparisons,
        published_images=published_images, performance=performance,
        limitations=['Authored rock and plant placement; not surveyed habitat distribution.',
            'The screenshots still fall short of the supplied realism reference, especially substrate, vegetation surfaces and lighting.',
            'Short performance captures do not establish long-term stability or performance everywhere.',
            'Underwater and streaming scenarios use different views and depths; their FPS are not a controlled object-density comparison.',
            'Menu/HUD/FPV redesign follows underwater and above-water graphics work.'])
    output = args.output.resolve()
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(status='PASS', cases=len(cases), screenshots=result['screenshots'], report=str(output))))


if __name__ == '__main__':
    main()
