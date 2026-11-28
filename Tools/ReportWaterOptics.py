"""Bind water surface comparisons and entry/transition checks to a packaged build."""
import argparse
import json
from pathlib import Path
import re
from PIL import Image, ImageChops, ImageStat
from ReportRocks import ROOT, read, sha, package_check


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path), bytes=path.stat().st_size)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run / 'Host.json')
    package_check(host)
    build_log = ROOT / 'artifacts/water-optics-package-sandbox-build.log'
    assert 'BUILD SUCCESSFUL' in build_log.read_text(encoding='utf-8-sig')
    launcher = read(run / 'Launcher.json')
    assert launcher['status'] == 'PASS' and launcher['exit_code'] == 0
    assert launcher['executable_sha256'].lower() == host['sha256'].lower()
    assert launcher['log_sha256'].lower() == sha(run / 'Launcher.log')
    cases = read(run / 'Cases.json')
    expected = {'look-day', 'look-night', 'surface0', 'surface1', 'surface2', 'storm', 'murky-torch'}
    assert len(cases) == len(expected) and {c['name'] for c in cases} == expected
    evidence = []
    for case in cases:
        folder = run / case['name']
        log_path = folder / 'Runtime.log'
        log = log_path.read_text(encoding='utf-8-sig')
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
        markers = re.findall(r'DIVE_ENTRY_MARKER entry=(\d+).*?clearance_cm=([\d.]+) bed_cm=([-\d.]+)', log)
        assert {int(m[0]) for m in markers} == {0, 1, 2} and len(markers) == 3
        assert all(float(m[1]) >= 150 and float(m[2]) < -100 for m in markers)
        if case['scenario'].startswith('surface'):
            assert 'surface=1' in log and 'DIVE_MEDIUM_PASS phase=air' in log and 'DIVE_MEDIUM_PASS phase=water' in log
        if case['name'] == 'storm':
            assert 'DIVE_WEATHER_PASS kind=3 wind=22.0' in log
        if case['name'] == 'murky-torch':
            assert 'DIVE_TORCH_PASS on=1' in log
        if case['scenario'] == 'rocks':
            assert 'DIVE_WATER_INSPECTION_PASS final_state=1 views=3 diagnostic=1' in log
            assert len(re.findall('DIVE_WATER_CAMERA actual=', log)) == 6
            assert {p.name for p in folder.glob('*.png')} == {f'Water_{s}_{v}.png' for s in range(2) for v in range(3)}
        images = []
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as im:
                assert im.size == (1600, 900)
                im.verify()
            images.append(record(path))
        assert len(images) == case['screenshots']
        evidence.append(dict(**case, log=record(log_path), images=images,
                             marker_clearance=markers,
                             engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    sources = []
    for name in ['DiveWorld.cpp', 'DiveWater.cpp', 'DiveRockInspection.cpp']:
        path = ROOT / 'divingsim/Source/DiveSim' / name
        sandbox = ROOT / 'Builds/EditorSandbox/divingsim/Source/DiveSim' / name
        assert path.read_bytes().replace(b'\r\n', b'\n') == sandbox.read_bytes().replace(b'\r\n', b'\n')
        sources.append(record(path))
    # Display-space measurements describe these captures only; they are not a
    # physical luminance calibration or a substitute for visual acceptance.
    means = {}
    above_difference = {}
    for time in ['day', 'night']:
        for state in range(2):
            with Image.open(run / f'look-{time}' / f'Water_{state}_1.png') as im:
                means[f'{time}_state{state}'] = ImageStat.Stat(im.convert('RGB')).mean
        with Image.open(run / f'look-{time}' / 'Water_0_2.png') as a, Image.open(run / f'look-{time}' / 'Water_1_2.png') as b:
            above_difference[time] = ImageStat.Stat(ImageChops.difference(a.convert('RGB'), b.convert('RGB'))).mean
    result = dict(status='PASS', scope='Packaged runtime checks; visual target remains open',
                  host=host, launcher=launcher, cases=evidence, screenshots=sum(c['screenshots'] for c in cases),
                  source_files=sources, package_build=record(build_log), upward_capture_mean_rgb_0_255=means,
                  above_water_mean_absolute_rgb_difference_0_255=above_difference,
                  comparison_states={'0': 'Forced previous surface volume on both sides',
                                     '1': 'No water volume behind the surface when fully submerged; camera attenuation remains active'},
                  limitations=[
                      'This corrects erroneous behind-surface absorption; it is not a complete underwater refraction/Snell-window renderer.',
                      'The first diagnostic camera was inside the entry rope. Those earlier flat images cannot identify a water-shader defect.',
                      'No new endurance or isolated frame-rate claim is made for this package.',
                      'Terrain, underwater composition, materials and UI still fall short of the supplied quality references.'])
    output = ROOT / 'Documentation/WATER_OPTICS_RESULTS.json'
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(status=result['status'], cases=len(cases), screenshots=result['screenshots'], report=str(output))))


if __name__ == '__main__':
    main()
