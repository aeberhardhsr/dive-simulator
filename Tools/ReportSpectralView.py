"""Bind the experimental RGB camera-path pass to frozen native inputs and captures.

Image differences establish that the pass ran and its zero-gain control is
close to the baseline. They do not measure physical accuracy or visual quality.
"""
import argparse
import json
import re
from pathlib import Path

import numpy as np
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from analyze_performance import analyze


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def inspect_run(run):
    host = read(run/'Host.json')
    cases = read(run/'Cases.json')
    assert [c['name'] for c in cases] == ['before', 'zero', 'after']
    for item in host['files']:
        assert sha(run/'inputs'/item['file']) == item['sha256'], item['file']
    # The tools may subsequently gain additional test options. Render inputs
    # must still be byte-identical to the tested snapshot at report creation.
    bound = [x for x in host['files'] if x['file'].startswith(
        ('divingsim/Source/', 'divingsim/Shaders/', 'divingsim/Content/', 'divingsim/Config/'))]
    for item in bound:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    logs = {}
    evidence = []
    for case in cases:
        name = case['name']
        folder = run/name
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        logs[name] = log
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Fatal error:|Failed to compile|Default Material will be used', log)
        active = name != 'before'
        gain = 0 if name == 'zero' else 1
        assert case['options']['SpectralView'] == active
        assert case['options']['AbsorptionGain'] == gain
        assert case['options']['Entry'] == host['entry']
        assert case['options']['HabitatSunView'] == host['sun_view']
        marker = f'DIVE_SPECTRAL_VIEW_READY gain={gain:.3f} samples=16 fog_resource=1 stage=BeforeDOF'
        assert (marker in log) == active
        assert len(case['images']) == 3 and len(set(case['images'])) == 3
        images = [folder/n for n in sorted(case['images'])]
        for image in images:
            pixels(image)
        evidence.append(dict(name=name, runtime=record(folder/'Runtime.log'),
                             images=[record(x) for x in images],
                             engine_errors=[s for s in log.splitlines() if ': Error:' in s]))
    cameras = [re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)', logs[n]) for n in ['before', 'zero', 'after']]
    assert len(cameras[0]) == 3 and cameras[0] == cameras[1] == cameras[2]
    differences = []
    for name in sorted(cases[0]['images']):
        before, zero, after = [pixels(run/case/name) for case in ['before', 'zero', 'after']]
        null_mad = float(np.abs(zero-before).mean())
        change_mad = float(np.abs(after-before).mean())
        # Fixed captures retain small temporal-lighting/noise differences.
        assert null_mad < 2, (run, name, null_mad)
        assert change_mad > null_mad, (run, name, change_mad, null_mad)
        differences.append(dict(image=name, zero_mean_absolute_rgb255=null_mad,
                                absorption_mean_absolute_rgb255=change_mad,
                                before_mean_rgb=before.mean(axis=(0, 1)).tolist(),
                                after_mean_rgb=after.mean(axis=(0, 1)).tolist()))
    return dict(host=host, frozen_host=record(run/'Host.json'), cases=evidence,
                equal_cameras=cameras[0], differences=differences)


def inspect_benchmark(folder):
    result = analyze(folder)
    host, metadata = result['host'], result['metadata']
    assert host['editor_sandbox'] and host['editor'] and not host['background_processes']
    assert host['water_preview'] and metadata['water_preview']
    for key, value in {'width': 1920, 'height': 1080, 'quality': 2,
                       'r.ScreenPercentage': 100, 't.MaxFPS': 0, 'r.VSync': 0}.items():
        assert metadata[key] == value, key
    log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
    active = host['spectral_view']
    assert ('DIVE_SPECTRAL_VIEW_READY gain=1.000' in log) == active
    if active:
        assert host['render_module_sha256'] == sha(ROOT/'Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveRendering.dll')
        assert host['spectral_shader_sha256'] == sha(ROOT/'divingsim/Shaders/SpectralWater.usf')
    entry = int(re.search(r'-DiveEntry=(\d+)\b', log).group(1))
    return dict(entry=entry, result=result, files=[record(folder/n) for n in
                ['Host.json', 'Metadata.json', 'Runtime.log', 'Frame.png', 'Capture.csv']])


def inspect_package(run):
    host = read(run/'Host.json')
    executable = Path(host['executable'])
    assert sha(executable) == host['sha256']
    runtime = executable.parents[2]
    for item in host['package_containers']:
        assert sha(runtime/'Content/Paks'/item['file']) == item['sha256']
    for item in host['files']:
        assert sha(run/'inputs'/item['file']) == item['sha256'], item['file']
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    cases = read(run/'Cases.json')
    assert [c['name'] for c in cases] == ['before', 'zero', 'after', 'night', 'fallback-before', 'fallback-after', 'shore', 'torch-before', 'torch-after']
    logs, evidence = {}, []
    for case in cases:
        name = case['name']
        folder = run/name
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        logs[name] = log
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Fatal error:|Failed to compile|Default Material will be used', log)
        if not name.startswith('fallback'):
            assert ('DIVE_SPECTRAL_VIEW_READY' in log) == (name in {'zero', 'after', 'night', 'shore', 'torch-after'})
        if name == 'zero':
            assert 'DIVE_SPECTRAL_VIEW_READY gain=0.000' in log
        if name.startswith('fallback'):
            assert 'r.VolumetricFog = "0"' in log
            assert re.search(r'DIVE_SCATTER_MODEL[^\r\n]*native=0 requested=1', log)
            assert log.rfind('DIVE_SPECTRAL_VIEW_READY') < log.index('DIVE_HABITAT_VIEW')
        if name == 'night':
            assert case['options']['Hour'] == 0 and '-DiveHour=0' in log
            for name_image in case['images']:
                assert np.quantile(pixels(folder/name_image), .999) < 3, 'Unlit night gained visible background glow'
        if name == 'shore':
            for phase in ['air', 'water', 'transition']:
                assert f'DIVE_MEDIUM_PASS phase={phase}' in log
        if name.startswith('torch'):
            assert 'DIVE_TORCH_PASS on=1 lumens=1000.0' in log
            target = re.search(r'DIVE_TORCH_TARGET hit=1 distance_m=([\d.]+)', log)
            assert target and 1 < float(target[1]) <= 20
            # Exclude the HUD; require a visible illuminated patch, not merely
            # a positive light-component setting or bright instrument text.
            from validate_lighting_frames import scene
            lit = scene(folder/'DiveUnderwater_1_torch.png')
            assert np.count_nonzero(lit > 20) > 5000
        assert len(case['images']) == (9 if name == 'shore' else 2 if name.startswith('torch') else 3)
        for name_image in case['images']:
            pixels(folder/name_image)
        evidence.append(dict(name=name, runtime=record(folder/'Runtime.log'),
                             images=[record(folder/n) for n in sorted(case['images'])],
                             engine_errors=[s for s in log.splitlines() if ': Error:' in s]))
    comparisons = []
    for names in [('before', 'zero'), ('before', 'after'), ('fallback-before', 'fallback-after')]:
        cameras = [re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)', logs[n]) for n in names]
        assert len(cameras[0]) == 3 and cameras[0] == cameras[1]
        values = [float(np.abs(pixels(run/names[0]/f'Habitat_0_{i}.png') -
                              pixels(run/names[1]/f'Habitat_0_{i}.png')).mean()) for i in range(3)]
        if names[1] != 'after':
            assert max(values) < 2, (names, values)
        comparisons.append(dict(cases=names, mean_absolute_rgb255=values, equal_cameras=cameras[0]))
    build = ROOT/'artifacts/spectral-package-build.log'
    assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    launcher = read(run/'launcher/Launcher.json')
    assert launcher['status'] == 'PASS' and launcher['executable_sha256'].lower() == host['sha256']
    assert launcher['log_sha256'].lower() == sha(run/'launcher/Launcher.log')
    assert 'DIVE_SPECTRAL_VIEW_READY' not in (run/'launcher/Launcher.log').read_text(encoding='utf-8-sig')
    return dict(host=host, cases=evidence, comparisons=comparisons, build=record(build),
                launcher=launcher, default_launcher_has_no_spectral_pass=True,
                startup_note='An early READY before the fallback takes effect is allowed; stable fallback images must match the control within 2/255 mean absolute RGB.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runs', type=Path, nargs='+')
    parser.add_argument('--benchmark', type=Path, action='append', default=[])
    parser.add_argument('--debug', type=Path)
    parser.add_argument('--package', type=Path)
    parser.add_argument('--startup-attempt', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/SPECTRAL_VIEW_RESULTS.json')
    args = parser.parse_args()
    runs = [p.resolve() for p in args.runs]
    results = [inspect_run(p) for p in runs]
    assert {(r['host']['entry'], r['host']['sun_view']) for r in results} == {(0, False), (0, True), (2, False)}
    common = [{x['file']: x['sha256'] for x in r['host']['files']} for r in results]
    assert common[0] == common[1] == common[2]
    published = []
    for entry, sun, label in [(0, False, 'Bed'), (0, True, 'Sun')]:
        run = next(p for p, r in zip(runs, results) if (r['host']['entry'], r['host']['sun_view']) == (entry, sun))
        for case in ['before', 'after']:
            destination = ROOT/f'Documentation/Images/SpectralView_{label}_{case}.png'
            destination.write_bytes((run/case/'Habitat_0_1.png').read_bytes())
            published.append(record(destination))
    benchmarks = [inspect_benchmark(p.resolve()) for p in args.benchmark]
    if benchmarks:
        assert len(benchmarks) == 4
        for benchmark in benchmarks:
            assert benchmark['result']['host']['game_module_sha256'] == common[0]['Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll']
            if benchmark['result']['host']['spectral_view']:
                assert benchmark['result']['host']['render_module_sha256'] == common[0]['Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveRendering.dll']
        for scenario, entry in [('perf-underwater', 0), ('perf-stream', 1)]:
            pair = [b for b in benchmarks if b['entry'] == entry and b['result']['metadata']['scenario'] == scenario]
            assert len(pair) == 2 and {b['result']['host']['spectral_view'] for b in pair} == {True, False}
            assert pair[0]['result']['host']['game_module_sha256'] == pair[1]['result']['host']['game_module_sha256']
    debug = None
    if args.debug:
        folder = args.debug.resolve()
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert '-DiveSpectralDepthDebug' in log and 'DIVE_SMOKE_PASS' in log
        horizon = pixels(folder/'Habitat_0_0.png')
        sky = pixels(folder/'Habitat_0_1.png')
        red = lambda p: float(((p[:, :, 0] > p[:, :, 2]*2) & (p[:, :, 0] > 50)).mean())
        assert .1 < red(horizon) < .9 and red(sky) > .95
        debug = dict(runtime=record(folder/'Runtime.log'), images=[record(p) for p in sorted(folder.glob('*.png'))],
                     red_horizon_fraction=red(horizon), red_sun_fraction=red(sky),
                     meaning='Red: displaced water boundary selected; blue: closer opaque geometry.')
    package = inspect_package(args.package.resolve()) if args.package else None
    startup_attempt = None
    if args.startup_attempt:
        folder = args.startup_attempt.resolve()
        host = read(folder/'Host.json')
        log = (folder/'fallback-after/Runtime.log').read_text(encoding='utf-8-sig')
        assert 'DIVE_SMOKE_PASS' in log
        assert 0 <= log.index('DIVE_SPECTRAL_VIEW_READY') < log.index('DIVE_HABITAT_VIEW')
        values = [float(np.abs(pixels(folder/'fallback-before'/f'Habitat_0_{i}.png') -
                              pixels(folder/'fallback-after'/f'Habitat_0_{i}.png')).mean()) for i in range(3)]
        assert max(values) < 2
        assert package and host['sha256'] == package['host']['sha256']
        startup_attempt = dict(status='HARNESS_ASSERTION_FAILED', host=record(folder/'Host.json'),
                               partial_cases=record(folder/'Cases.json'),
                               runtime=record(folder/'fallback-after/Runtime.log'),
                               images=[record(folder/c/f'Habitat_0_{i}.png') for c in ['fallback-before', 'fallback-after'] for i in range(3)],
                               fallback_mean_absolute_rgb255=values,
                               diagnosis='A one-time READY during startup was mistaken for a persistent active pass. Stable fallback images agree; the harness was corrected without changing the renderer or package.')
    report = dict(status='PASS_EXPERIMENTAL_CHECKS', native_runs=results, published=published, package=package,
                  depth_debug=debug, benchmarks=benchmarks, default_enabled=False, initial_package_attempt=startup_attempt,
                  limitations=[
                      'First camera leg only; reflected/refracted second legs and source-to-water RGB transport remain approximations.',
                      'Sixteen uniform intervals approximate cumulative native fog; no full spectral multiple scattering.',
                      'Absorption coefficients are authored, not measured Walensee optical data.',
                      'Immersion uses mean water elevation, not displaced waves; storm waterline remains unvalidated.',
                      'Translucent bubbles/particles do not have separate optical path lengths.',
                      'Native captures at fixed cameras are not final visual acceptance or a performance guarantee.',
                      'Private UE 5.8 renderer API; future engine versions require revalidation.',
                      'Before/after performance captures have temporal and host-load differences; total cost cannot isolate a GPU pass exactly.'
                  ])
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n', encoding='utf-8')
    print(f'SPECTRAL_VIEW_REPORT_PASS runs={len(results)} images={9*len(results)} benchmarks={len(benchmarks)}')


if __name__ == '__main__':
    main()
