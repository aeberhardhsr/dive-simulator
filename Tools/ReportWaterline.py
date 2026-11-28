"""Verify the camera-material waterline regression using fixed native/game captures."""
import argparse
import hashlib
import json
import re
import shutil
from pathlib import Path

import numpy as np
from PIL import Image
from validate_lighting_frames import scene

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def record(path):
    path = path.resolve()
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=digest)


def pixels(path):
    with Image.open(path) as image:
        assert image.size == (1600, 900), path
        return np.asarray(image.convert('RGB'), dtype=float)


def metrics(rgb):
    # Fixed empty-water region below the horizon; excludes terrain and HUD.
    luminance = rgb[500:700, 400:1200] @ np.array([.2126, .7152, .0722])
    return dict(roi_xyxy=[400, 500, 1200, 700], mean_display_luminance=float(luminance.mean()),
                fraction_luminance_below_5=float((luminance < 5).mean()))


def run_record(folder, current=False):
    host = read(folder / 'Host.json')
    if current:
        for item in host['files']:
            assert record(ROOT / item['file'])['sha256'] == item['sha256'], item['file']
    cases = read(folder / 'Cases.json')
    if isinstance(cases, dict):
        cases = [cases]
    evidence = []
    for case in cases:
        assert case['status'] == 'PASS'
        logpath = folder / case['name'] / 'Runtime.log'
        log = logpath.read_text(encoding='utf-8-sig')
        assert not re.search(r'DIVE_\w*FAIL|Fatal error:|Failed to compile Material|Default Material will be used', log)
        assert len(re.findall('DIVE_WATERLINE_VIEW view=', log)) == 3
        assert len(case['images']) == 3
        evidence.append(dict(name=case['name'], log=record(logpath),
                             images=[record(folder / case['name'] / name) for name in case['images']]))
    return dict(host=host, manifest=record(folder / 'Host.json'), cases=evidence)


def poses(folder, case):
    log = (folder / case / 'Runtime.log').read_text(encoding='utf-8-sig')
    return re.findall(r'DIVE_WATER_VIEW state=0 view=(\d) eye=(.*?) rotation=(.*?) exposure=', log)


def regression_record(folder, launcher_folder):
    host = read(folder / 'Host.json')
    assert host['packaged'] and host['water_preview']
    for item in host['files']:
        assert record(ROOT / item['file'])['sha256'] == item['sha256'], item['file']
    assert record(Path(host['executable']))['sha256'] == host['sha256']
    runtime = Path(host['executable']).parents[2]
    for item in host['package_containers']:
        assert record(runtime / 'Content/Paks' / item['file'])['sha256'] == item['sha256']
    cases = read(folder / 'Cases.json')
    expected = dict(surface=3, weesen=3, murg=3, ledi=3, storm=3,
                    **{'dark-on': 2, 'dark-off': 2, 'fog-fallback': 3},
                    raster=3, software=3, stream=2, shore=9, sediment=7)
    assert {c['name'] for c in cases} == set(expected) and len(cases) == len(expected)
    for case in cases:
        path = folder / case['name']
        assert case['status'] == 'PASS' and len(case['screenshots']) == expected[case['name']]
        assert record(path / 'Runtime.log')['sha256'] == case['log_sha256']
        log = (path / 'Runtime.log').read_text(encoding='utf-8-sig')
        assert 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Fatal error:|Failed to compile Material|Default Material will be used', log)
        for image in case['screenshots']:
            assert record(path / image['file'])['sha256'] == image['sha256']
            pixels(path / image['file'])
    off_path = folder / 'dark-off/DiveUnderwater_0_torch.png'
    on_path = folder / 'dark-on/DiveUnderwater_0_torch.png'
    # The redesigned breathing panel reaches y=174. The historical mask starts
    # at y=160 and measures that HUD panel as scene light. Keep the same light
    # thresholds, use an unobstructed scene rectangle, and exclude the reticle.
    mask = np.ones((480, 1200), dtype=bool)
    mask[240:260, 590:610] = False
    off = pixels(off_path)[200:680, 200:1400].max(axis=2)[mask]
    on = pixels(on_path)[200:680, 200:1400].max(axis=2)[mask]
    night = dict(roi_xyxy=[200, 200, 1400, 680], reticle_exclusion_xyxy=[790, 440, 810, 460],
                 historical_mask_p999=float(np.percentile(scene(off_path), 99.9)),
                 mask_change_reason='Historical y=160 mask includes redesigned breathing HUD through y=173.',
                 off_p999=float(np.percentile(off, 99.9)),
                 lit_pixels=int(((on > 20) & (off < 3)).sum()), mean_difference=float((on - off).mean()))
    assert night['off_p999'] <= 2 and night['lit_pixels'] > 5000 and night['mean_difference'] > 1
    launcher = read(launcher_folder / 'Launcher.json')
    assert launcher['status'] == 'PASS' and launcher['executable_sha256'].lower() == host['sha256']
    assert record(launcher_folder / 'Launcher.log')['sha256'] == launcher['log_sha256'].lower()
    return dict(host=host, manifest=record(folder / 'Host.json'), cases=cases, night=night,
                launcher=launcher, launcher_manifest=record(launcher_folder / 'Launcher.json'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', type=Path, required=True)
    parser.add_argument('--isolation', type=Path, required=True)
    parser.add_argument('--after', type=Path, required=True)
    parser.add_argument('--package', type=Path)
    parser.add_argument('--regression', type=Path)
    parser.add_argument('--launcher', type=Path)
    parser.add_argument('--package-build', type=Path)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    runs = dict(before=run_record(args.before), isolation=run_record(args.isolation),
                native=run_record(args.after, current=True))
    expected = {'normal', 'no-post-process-materials', 'fog-fallback'}
    assert {c['name'] for c in runs['native']['cases']} == expected
    assert runs['native']['host']['native_editor']
    comparisons = {}
    target_runs = [('native', args.after)]
    if args.package:
        runs['package'] = run_record(args.package, current=True)
        assert not runs['package']['host']['native_editor']
        assert {c['name'] for c in runs['package']['cases']} == expected
        target_runs.append(('package', args.package))
    if args.regression:
        assert args.package and args.launcher and args.package_build
        runs['regression'] = regression_record(args.regression, args.launcher)
        build_log = args.package_build.read_text(encoding='utf-8-sig')
        assert 'BUILD SUCCESSFUL' in build_log and 'ExitCode=0 (Success)' in build_log
        runs['package_build'] = record(args.package_build)
    original_poses = poses(args.before, 'normal')
    assert len(original_poses) == 3
    assert poses(args.isolation, 'no-post-process-materials') == original_poses
    for label, folder in target_runs:
        assert poses(folder, 'normal') == original_poses
        assert poses(folder, 'no-post-process-materials') == original_poses
        views = []
        for view in range(3):
            name = f'Water_0_{view}.png'
            before = pixels(args.before / 'normal' / name)
            isolated = pixels(args.isolation / 'no-post-process-materials' / name)
            after = pixels(folder / 'normal' / name)
            control = pixels(folder / 'no-post-process-materials' / name)
            old, new = metrics(before), metrics(after)
            delta = float(np.abs(after - control).mean())
            assert delta < 2, (label, view, delta)
            if view < 2:
                assert old['fraction_luminance_below_5'] > .99
                assert new['fraction_luminance_below_5'] < .01
                assert new['mean_display_luminance'] > 10
                assert metrics(isolated)['fraction_luminance_below_5'] < .01
            else:
                assert float(np.abs(after - before).mean()) < 2
            views.append(dict(view=view, before=old, after=new,
                              control_mean_absolute_rgb_difference=delta,
                              before_mean_absolute_rgb_difference=float(np.abs(after - before).mean())))
        comparisons[label] = views
    build = args.build.read_text(encoding='utf-8-sig')
    assert 'DIVE_OPTICS_CONTENT_OK' in build
    assert 'Failed to compile Material' not in build
    images = ROOT / 'Documentation/Images'
    final_folder = args.package or args.after
    for name, source in [('WaterlineBlend_Before.png', args.before / 'normal/Water_0_0.png'),
                         ('WaterlineBlend_After.png', final_folder / 'normal/Water_0_0.png')]:
        shutil.copyfile(source, images / name)
    report = dict(status='PASS', scope='Fixed-pose camera-blending regression, not full visual acceptance',
                  runs=runs, comparisons=comparisons, content_build=record(args.build),
                  published=[record(images / name) for name in ['WaterlineBlend_Before.png', 'WaterlineBlend_After.png']],
                  limitations=['Surface reflection blue band remains.',
                               'Different launches have temporal rendering variation.',
                               'Earlier diagnostic fixtures evolved; manifests record each revision.',
                               'No-volumetrics also selects the analytical camera fallback.',
                               'No manual UI acceptance or performance claim.'])
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(f'PASS {args.output}')


if __name__ == '__main__':
    main()
