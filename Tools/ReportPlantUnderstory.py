"""Bind plant-understory comparisons to sources, package and performance."""
import argparse
import json
import re
from pathlib import Path
import numpy as np
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from ReportRocks import package_check
from analyze_performance import analyze


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('run', type=Path)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--benchmark', type=Path, action='append', default=[])
    p.add_argument('--control-benchmark', type=Path)
    p.add_argument('--build-log', type=Path, default=ROOT/'artifacts/plant-understory-package-build.log')
    a = p.parse_args()
    run = a.run.resolve()
    host = read(run/'Host.json')
    for item in host['files']:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    if host['packaged']:
        package_check(host)
    expected = {'close-before': 3, 'close-after': 3}
    if not host['preview']:
        expected.update(weesen=3, murg=3, walenstadt=3, raster=3, stream=2, shore=9, sediment=7)
    cases = read(run/'Cases.json')
    assert len(cases) == len(expected) and {c['name'] for c in cases} == set(expected)
    entries = read(ROOT/'divingsim/Content/Diving/Geodata/manifest.json')['entries']
    shores = read(ROOT/'divingsim/Content/Diving/Geodata/shoreline.json')['sites']
    logs, evidence, counts = {}, [], {}
    for case in cases:
        name = case['name']
        folder = run/name
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        logs[name] = log
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
        assert 'DIVE_WATER_PREVIEW_READY smoke=1 hardware=1 bounded=1 interface=1 lighting_mode=2' in log
        assert ('-DiveNoPlantUnderstory' in log) == (name == 'close-before')
        entry = int(re.search(r'-DiveEntry=(\d+)', log)[1])
        checks = [tuple(map(int, x)) for x in re.findall(
            r'DIVE_PLANT_BEDS_PASS legacy=(\d+) plants=(\d+) short=(\d+) tall=(\d+) unique_roots=(\d+)', log)]
        if name not in {'shore', 'sediment'}:
            assert len(checks) == 2
            assert all(old == 0 and n == unique and 0 <= short <= n and 0 <= tall <= n
                       for old, n, short, tall, unique in checks)
            for marker in ['DIVE_POPULATION_PASS', 'DIVE_ROCK_LAYOUT_PASS', 'DIVE_GRAVEL_PASS',
                           'DIVE_STEERING_WORLD_PASS', 'DIVE_POPULATION_RETIREMENT_PASS',
                           'DIVE_POPULATION_NEAR_TELEPORT_PASS']:
                assert marker in log, marker
            counts[name] = checks[0]
        if name == 'raster':
            assert 'r.Nanite = "0"' in log
        images = sorted(folder.glob('*.png'))
        assert len(images) == case['screenshots'] == expected[name]
        for path in images:
            pixels(path)
        evidence.append(dict(**case, entry=entry, start_on_shore=name == 'shore',
                             location=(shores if name == 'shore' else entries)[entry],
                             plant_checks=checks, runtime=record(folder/'Runtime.log'),
                             images=[record(x) for x in images],
                             engine_errors=[s for s in log.splitlines() if ': Error:' in s]))
    names = ['close-before', 'close-after']
    poses = [re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)', logs[n]) for n in names]
    assert len(poses[0]) == 3 and poses[0] == poses[1]
    before, after = [counts[n] for n in names]
    assert after[1] > before[1] > 0 and after[2]-before[2] == after[1]-before[1]
    assert after[3] == before[3]  # New layer adds short growth only.
    rock_states = [re.findall(r'DIVE_ROCK_POSE_STATE sha1=([0-9A-F]{40})', logs[n]) for n in names]
    assert rock_states[0] and rock_states[0] == rock_states[1]
    comparisons = []
    for i in range(3):
        b, c = [run/n/f'Habitat_0_{i}.png' for n in names]
        delta = float(np.abs(pixels(b)-pixels(c)).mean())
        comparisons.append(dict(before=record(b), after=record(c), mean_absolute_rgb_difference=delta))
    assert max(c['mean_absolute_rgb_difference'] for c in comparisons) > .1
    launcher, build, control = None, None, None
    benchmarks, published = [], []
    if host['packaged']:
        build_path = a.build_log.resolve()
        assert 'BUILD SUCCESSFUL' in build_path.read_text(encoding='utf-8-sig')
        build = record(build_path)
        launcher = read(run/'launcher/Launcher.json')
        assert launcher['status'] == 'PASS' and launcher['exit_code'] == 0
        assert launcher['executable_sha256'].lower() == host['sha256'].lower()
        assert launcher['log_sha256'].lower() == sha(run/'launcher/Launcher.log')
        for case, label in [('close-before', 'Before'), ('close-after', 'After')]:
            path = ROOT/f'Documentation/Images/PlantUnderstory_{label}.png'
            assert sha(path) == sha(run/case/'Habitat_0_0.png')
            published.append(record(path))

        def benchmark(folder, disabled):
            folder = folder.resolve()
            result = analyze(folder)
            h, m = result['host'], result['metadata']
            assert not h['editor'] and not h['background_processes'] and h['water_preview']
            assert h['executable_sha256'] == host['sha256'].lower()
            assert h['package_containers'] == host['package_containers']
            assert h['no_plant_understory'] == m['no_plant_understory'] == disabled
            for key, value in {'width': 1920, 'height': 1080, 'quality': 2, 'r.ScreenPercentage': 100,
                               'r.VSync': 0, 't.MaxFPS': 0, 'r.Lumen.HardwareRayTracing': 1}.items():
                assert m[key] == value, key
            entry = int(re.search(r'-DiveEntry=(\d+)', (folder/'Runtime.log').read_text(encoding='utf-8-sig'))[1])
            return dict(result=result, entry=entry, location=entries[entry],
                        files=[record(folder/n) for n in ['Capture.csv', 'Metadata.json', 'Host.json', 'Frame.png', 'Runtime.log']])

        assert len(a.benchmark) == 3 and a.control_benchmark
        benchmarks = [benchmark(folder, False) for folder in a.benchmark]
        assert {(b['result']['metadata']['scenario'], b['entry']) for b in benchmarks} == {
            ('perf-underwater', 0), ('perf-underwater', 2), ('perf-stream', 1)}
        control = benchmark(a.control_benchmark, True)
        assert control['entry'] == 0 and control['result']['metadata']['scenario'] == 'perf-underwater'
        normal = next(b for b in benchmarks if b['entry'] == 0)
        assert control['result']['metadata']['requested_capture_seconds'] == normal['result']['metadata']['requested_capture_seconds']
    else:
        assert not a.benchmark and not a.control_benchmark
    result = dict(status='PASS', scope='Low plant layer: visual comparison, habitat and streaming checks; not complete reference acceptance',
                  host=host, cases=evidence, comparisons=comparisons, comparison_poses=poses[0],
                  comparison_plant_counts=[before, after], unchanged_rock_states=rock_states[0],
                  entry_sources=[record(ROOT/'divingsim/Content/Diving/Geodata'/n) for n in ['manifest.json', 'shoreline.json']],
                  launcher=launcher, build=build, benchmarks=benchmarks, control=control, published=published,
                  limitations=['Authored growth distribution, not measured Walensee vegetation',
                               'Model detail, sediment transitions and lighting remain unfinished',
                               'Short single-run measurements do not prove minimum hardware or long-term stability'])
    a.output.write_text(json.dumps(result, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(f'PLANT_UNDERSTORY_PASS cases={len(cases)} captures={sum(expected.values())}')


if __name__ == '__main__':
    main()
