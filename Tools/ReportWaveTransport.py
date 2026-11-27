"""Validate paired native receiver captures and measure, without visual approval."""
import argparse
import hashlib
import json
from pathlib import Path
import re


FIXTURES = {
    'day': ('6', '0', '0', '0'), 'calm': ('0', '0', '0', '0'),
    'deep': ('6', '0', '0', '1'), 'air': ('6', '0', '1', '0'),
    'night': ('6', '1', '0', '0'),
}
REGIONS = {'lit': (250, 580, 1350, 820), 'shadow': (900, 410, 1200, 470),
           'sphere': (350, 15, 520, 75)}
CAPTURE = re.compile(
    r'DIVE_CAUSTICS_CAPTURE phase=(\d+) wind=(\d+) night=(\d+) air=(\d+) deep=(\d+) '
    r'strength=(\d+) clock=([\d.]+) camera=([^\r\n]+)')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def validate_metadata(log, host, fixture, wave):
    require(host['case'] == fixture and host['wave_transport_study'] == wave,
            'Incorrect capture variant')
    require(not host['packaged'] and host['water_preview'] and host['solar_shader_study']
            and host['solar_volume_study'], 'Incorrect rendering options')
    require(not re.search(r'DIVE_\w*FAIL|Fatal error:|Failed to compile|Default Material will be used|unbound parameters',
                          log, re.IGNORECASE), 'Render failure in capture')
    for marker in ('DIVE_CAUSTICS_PROBE_PASS captures=4', 'DIVE_WATER_PREVIEW_READY',
                   'DIVE_SOLAR_SHADER_STUDY_READY', 'DIVE_SOLAR_VOLUME_STUDY_READY'):
        require(marker in log, 'Missing runtime marker: '+marker)
    require(('DIVE_WATER_SCENE_READY waves=4' in log) == wave, 'Incorrect water scene binding')
    require('DIVE_SPECTRAL_VIEW_READY' not in log, 'Unexpected independent camera absorption')
    captures = CAPTURE.findall(log)
    require(len(captures) == 4 and [c[0] for c in captures] == ['0', '1', '2', '3'],
            'Missing or duplicated capture phases')
    require(all(tuple(c[1:5]) == FIXTURES[fixture] for c in captures), 'Incorrect fixture conditions')
    require([c[5] for c in captures] == ['1', '0', '1', '1'] and
            [float(c[6]) for c in captures] == [20, 20, 22, 22], 'Incorrect strength/time controls')
    require(len({c[7] for c in captures}) == 1, 'Camera moved between control phases')
    return captures


def differences(a, b):
    import numpy as np
    delta = b.astype(np.float32)-a.astype(np.float32)
    regions = dict(REGIONS, full=(0, 0, 1600, 900))
    return {name: dict(mean_absolute_rgb8=float(np.abs(delta[y0:y1, x0:x1]).mean(dtype=np.float64)),
                       mean_signed_rgb8=delta[y0:y1, x0:x1].mean(axis=(0, 1), dtype=np.float64).tolist())
            for name, (x0, y0, x1, y1) in regions.items()}


def report(root):
    import numpy as np
    from PIL import Image
    host = read(root/'Host.json')
    for item in host['files']:
        require(sha(Path(item['path'])) == item['sha256'], 'Changed input: '+item['path'])
    inputs = {str(Path(item['path']).resolve()): item['sha256'] for item in host['files']}
    entries = read(root/'Cases.json')
    expected = {f'{fixture}-{mode}' for fixture in FIXTURES for mode in ('projection', 'wave')}
    require(len(entries) == 10 and {c['name'] for c in entries} == expected,
            'Incomplete or duplicated paired cases')
    require(all(c['runtime_status'] == 'PASS' for c in entries), 'Incomplete native run')
    cases, pairs = {}, {}
    for fixture in FIXTURES:
        paired_images, poses = {}, {}
        for mode, wave in [('projection', False), ('wave', True)]:
            name = f'{fixture}-{mode}'
            folder = root/name
            case_host = read(folder/'Host.json')
            log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
            poses[mode] = validate_metadata(log, case_host, fixture, wave)
            for path, digest in [(case_host['executable'], case_host['executable_sha256']),
                                 (case_host['renderer_module']['path'], case_host['renderer_module']['sha256'])]:
                require(inputs.get(str(Path(path).resolve())) == digest.lower(),
                        'Case uses a different engine: '+name)
            require(case_host['sandbox_name'] == Path(host['project']).parents[1].name,
                    'Case uses a different project sandbox')
            paths = [folder/f'Caustics_{i:02d}.png' for i in range(4)]
            require(set(folder.glob('*.png')) == set(paths), 'Unexpected capture image set: '+name)
            images = []
            for path in paths:
                with Image.open(path) as image:
                    require(image.size == (1600, 900), 'Unexpected image dimensions: '+str(path))
                    images.append(np.asarray(image.convert('RGB')).copy())
            paired_images[mode] = images
            cases[name] = dict(host=case_host, captures=poses[mode],
                log_sha256=sha(folder/'Runtime.log'),
                images=[dict(file=p.name, sha256=sha(p)) for p in paths],
                diagnostics=sorted(set(re.findall(r'[^\r\n]*(?:Error:|Warning:)[^\r\n]*', log))),
                phase_comparisons={label: differences(images[a], images[b]) for label, a, b in
                    [('strength_on_vs_off', 0, 1), ('clock_20_vs_22', 0, 2), ('frozen_clock_repeat', 2, 3)]})
        require(poses['projection'] == poses['wave'], 'Paired camera or conditions differ: '+fixture)
        pairs[fixture] = [differences(paired_images['projection'][i], paired_images['wave'][i]) for i in range(4)]
    return dict(status='CAPTURE_INTEGRITY_CHECKED', visual_status='NOT_REVIEWED',
                visual_target_complete=False, default_enabled=False, host=host, cases=cases,
                projection_to_wave=pairs, regions_xyxy=REGIONS, report_tool_sha256=sha(Path(__file__)),
                limits=['LDR RGB8 differences are not radiometric accuracy or aesthetic acceptance.',
                        'Temporal repeat differences include rendering noise and must be reviewed.',
                        'Receiver fixtures do not cover habitat, torch, lake boundaries or performance.'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--study', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = report(args.study.resolve())
    output = args.output or args.study/'Report.json'
    output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print('WAVE_CAPTURE_INTEGRITY_CHECKED', output)


if __name__ == '__main__':
    main()
