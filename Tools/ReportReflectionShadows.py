"""Bind native VSM-water reflection coverage, controls, images and performance."""
import argparse
import json
from pathlib import Path

import numpy as np

from ReportReflectionTransport import ROOT, bind, frame, load_run, region


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('coverage', 'near', 'fallback', 'reference', 'waterline', 'full-perf', 'vsm-perf'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    report = dict(status='EXPERIMENT_REVIEWED_NOT_RELEASED', production_fix=False, studies={}, metrics=[])
    for name, profile in [('coverage', 'vsm-coverage'), ('near', 'vsm'), ('fallback', 'vsm-fallback'), ('reference', 'full')]:
        folder = (ROOT / getattr(args, name)).resolve()
        report['studies'][name] = load_run(folder, ['near-hybrid', 'near-no-screens'])
        if report['studies'][name]['shader_manifest']['profile'] != profile:
            raise ValueError('Unexpected native shadow profile')
    waterline = (ROOT / args.waterline).resolve()
    report['studies']['waterline'] = load_run(waterline, ['baseline', 'transport-no-screens', 'transport-hybrid'])
    if report['studies']['waterline']['shader_manifest']['profile'] != 'vsm':
        raise ValueError('Waterline study must use mapped shadows')
    for case in ('near-hybrid', 'near-no-screens'):
        for view in range(3):
            reference = frame(ROOT / args.reference, case, view)
            fallback = frame(ROOT / args.fallback, case, view)
            mapped = frame(ROOT / args.near, case, view)
            fallback_error = float(np.abs(reference-fallback).mean())
            mapped_error = float(np.abs(reference-mapped).mean())
            if fallback_error > 1 or (view == 2 and mapped_error > 1):
                raise ValueError('Fallback or air reference differs by more than one display RGB level')
            report['metrics'].append(dict(case=case, view=view, fallback_reference_mae=fallback_error, mapped_reference_mae=mapped_error))
    for view in range(3):
        active = frame(waterline, 'transport-hybrid', view)
        if view < 2:
            values = region(active)
            if values['blue_dominant_fraction'] > .01 or values['mean_luma'] < 4:
                raise ValueError('Original waterline correction lost')
            report['metrics'].append(dict(waterline_view=view, region=values))
        else:
            error = float(np.abs(active-frame(waterline, 'baseline', view)).mean())
            if error > 1:
                raise ValueError('Air control changed')
            report['waterline_air_control_mae'] = error
    diagnostic = frame(ROOT / args.coverage, 'near-hybrid', 0)[80:350, 160:1100]
    green = float(np.mean((diagnostic[:, :, 1] > diagnostic[:, :, 0]+30) & (diagnostic[:, :, 1] > diagnostic[:, :, 2]+30)))
    if green < .95:
        raise ValueError('Native diagnostic does not demonstrate mapped shadow use in the selected region')
    report['coverage'] = dict(region_xyxy=[160,80,1100,350], green_display_pixel_fraction=green,
        scope='Display classification of the diagnostic image; not an exact GPU query counter or coverage of all gameplay views.')
    report['performance'] = {}
    for name, folder, profile in [('full', args.full_perf, 'full'), ('vsm', args.vsm_perf, 'vsm')]:
        folder = (ROOT / folder).resolve()
        summary = json.loads((folder / 'Summary.json').read_text(encoding='utf-8-sig'))
        manifest = json.loads((folder / 'ShaderManifest.json').read_text(encoding='utf-8-sig'))
        log = (folder / 'Runtime.log').read_text(encoding='utf-8-sig', errors='replace')
        if manifest['profile'] != profile or not manifest['unified'] or f'DIVE_REFLECTION_PROFILE_READY profile={profile} ' not in log:
            raise ValueError('Unexpected benchmark profile')
        meta = summary['metadata']
        if not meta['reflection_unified'] or meta['reflection_control'] or abs(meta['camera_rotation_degrees']['pitch']-25) > .01:
            raise ValueError('Unexpected benchmark camera or routing')
        report['performance'][name] = dict(summary=summary, evidence=[bind(folder / f) for f in
            ('Summary.json', 'Host.json', 'Metadata.json', 'Capture.csv', 'Frame.png', 'Runtime.log', 'ShaderManifest.json')])
    base, active = [report['performance'][name]['summary'] for name in ('full', 'vsm')]
    for key in ('width', 'height', 'quality', 'requested_capture_seconds', 'r.ScreenPercentage', 'r.Lumen.HardwareRayTracing.Inline'):
        if base['metadata'][key] != active['metadata'][key]:
            raise ValueError('Unmatched benchmark setting')
    for key in ('renderer_module_sha256', 'render_module_sha256', 'game_module_sha256'):
        if base['host'][key] != active['host'][key]:
            raise ValueError('Benchmark modules differ')
    report['performance']['fps_change_percent'] = 100*(active['average_fps']/base['average_fps']-1)
    report['performance']['scope'] = 'Sequential moving source-editor runs on this host, not a packaged release acceptance or identical-frame GPU test.'
    patch = ROOT / 'artifacts/reflection-shadow-patch/Manifest.json'
    report['engine_patch'] = json.loads(patch.read_text())
    report['build_evidence'] = [bind(patch), bind(ROOT / 'artifacts/reflection-shadow-study/editor-build-corrected.log')]
    report['limits'] = 'Current-view VSM with unique matching directional light only; finite lights, missing/ambiguous data and excluded coarse non-Nanite geometry retain the original ray queries. Native VSM bias/resolution differs from binary near-field rays. Complete lighting and moving-shadow validation remains open.'
    output = ROOT / 'Documentation/REFLECTION_SHADOW_RESULTS.json'
    output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(output)


if __name__ == '__main__':
    main()
