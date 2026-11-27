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
    p.add_argument('--publish-prefix', default='PlantUnderstory')
    p.add_argument('--plant-models', action='store_true')
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
    if host.get('culling_compare'):
        expected.update(unculled=3, **{'clear-water': 3, 'clear-water-unculled': 3})
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
    culling_comparisons = []
    if host.get('culling_compare'):
        for limited, unlimited, distance in [('close-after', 'unculled', 7000), ('clear-water', 'clear-water-unculled', 20000)]:
            assert counts[limited] == counts[unlimited]
            assert re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)', logs[limited]) == re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)', logs[unlimited])
            assert set(re.findall(r'cull_cm=(\d+)', logs[limited])) == {str(distance)}
            assert set(re.findall(r'cull_cm=(\d+)', logs[unlimited])) == {'0'}
            for i in range(3):
                b, c = [run/n/f'Habitat_0_{i}.png' for n in [unlimited, limited]]
                culling_comparisons.append(dict(unlimited=record(b), limited=record(c),
                    mean_absolute_rgb_difference=float(np.abs(pixels(b)-pixels(c)).mean())))
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
            path = ROOT/f'Documentation/Images/{a.publish_prefix}_{label}.png'
            assert sha(path) == sha(run/case/'Habitat_0_0.png')
            published.append(record(path))

        def benchmark(folder, disabled):
            folder = folder.resolve()
            result = analyze(folder)
            h, m = result['host'], result['metadata']
            assert not h['editor'] and not h['background_processes'] and h['water_preview']
            assert h['executable_sha256'] == host['sha256'].lower()
            assert h['package_containers'] == host['package_containers']
            if host.get('culling_compare'):
                assert h['no_plant_understory'] == m['no_plant_understory'] == False
                assert h['unlimited_plants'] == m['unlimited_plants'] == disabled
            else:
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
    models = None
    if a.plant_models:
        manifest_path = ROOT/'blender/exports/Plants/Manifest.json'
        manifest = read(manifest_path)
        imported = read(ROOT/'artifacts/PlantContentBuild.json')
        assert manifest['revision'] == 'bounded-branch-tessellation'
        assert len(manifest['assets']) == len(imported) == 3
        for item in manifest['assets']:
            assert sha(ROOT/item['file']) == item['sha256']
            built = next(x for x in imported if x['name'] == item['name'])
            assert built['source_sha256'] == item['sha256'] and built['triangles'][0] == item['triangles']
            assert built['source_uv_channels'] == 2 and all(x > y > 0 for x, y in zip(built['triangles'], built['triangles'][1:]))
        assert next(x for x in imported if x['name'] == 'SM_Stonewort')['triangles'][0] == 29262
        backup = ROOT/'artifacts/plant-efficient-source-before'
        before_inputs = read(backup/'Inputs.json')
        for item in before_inputs:
            assert sha(backup/item['file']) == item['sha256']
        form_runs = []
        for phase in ['before', 'after']:
            folder = ROOT/f'artifacts/plant-efficient-{phase}'
            log_path = folder.with_suffix('.log')
            log = log_path.read_text(encoding='utf-8-sig')
            assert 'DIVE_PLANT_INSPECTION_PASS groups=3 states=2 views=2 captures=12' in log
            assert 'DIVE_SMOKE_PASS' in log and not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Fatal error:', log)
            images = sorted(folder.glob('Plant_*.png'))
            assert len(images) == 12
            for path in images:
                pixels(path)
            form_runs.append(dict(phase=phase, log=record(log_path), images=[record(x) for x in images]))
        source_files = [manifest_path, ROOT/'Tools/create_aquatic_plants.py', ROOT/'Tools/build_plant_content.py',
                        ROOT/'blender/AquaticPlants.blend', ROOT/'artifacts/PlantContentBuild.json',
                        ROOT/'artifacts/plant-efficient-blender.log', ROOT/'artifacts/plant-efficient-content.log']
        source_files += [ROOT/item['file'] for item in manifest['assets']]
        model_images = []
        for phase in ['before', 'after']:
            path = ROOT/f'Documentation/Images/PlantBranches_{phase.title()}.png'
            assert sha(path) == sha(ROOT/f'artifacts/plant-efficient-{phase}/Plant_3_0.png')
            model_images.append(record(path))
        models = dict(manifest=manifest, imported=imported, source_files=[record(x) for x in source_files],
                      before_inputs=before_inputs, before_files=[record(backup/x['file']) for x in before_inputs], forms=form_runs, published=model_images)
    result = dict(status='PASS', scope='Low plant layer: visual comparison, habitat and streaming checks; not complete reference acceptance',
                  host=host, cases=evidence, comparisons=comparisons, comparison_poses=poses[0],
                  comparison_plant_counts=[before, after], unchanged_rock_states=rock_states[0],
                  culling_comparisons=culling_comparisons,
                  models=models,
                  benchmark_control='Unlimited plants' if host.get('culling_compare') else 'Without low plant layer',
                  entry_sources=[record(ROOT/'divingsim/Content/Diving/Geodata'/n) for n in ['manifest.json', 'shoreline.json']],
                  launcher=launcher, build=build, benchmarks=benchmarks, control=control, published=published,
                  limitations=['Authored growth distribution, not measured Walensee vegetation',
                               'Model detail, sediment transitions and lighting remain unfinished',
                               'Short single-run measurements do not prove minimum hardware or long-term stability'])
    a.output.write_text(json.dumps(result, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(f'PLANT_UNDERSTORY_PASS cases={len(cases)} captures={sum(expected.values())}')


if __name__ == '__main__':
    main()
