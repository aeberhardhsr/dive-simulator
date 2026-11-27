"""Bind native selective-reflection comparisons and measured performance evidence."""
import argparse
import json
from pathlib import Path

import numpy as np

from ReportReflectionTransport import ROOT, bind, frame, load_run, region


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--waterline', type=Path, required=True)
    parser.add_argument('--near', type=Path, required=True)
    parser.add_argument('--control-perf', type=Path, required=True)
    parser.add_argument('--active-perf', type=Path, required=True)
    args = parser.parse_args()
    waterline, near, control, active = [(ROOT / p).resolve() for p in
        (args.waterline, args.near, args.control_perf, args.active_perf)]
    report = dict(status='EXPERIMENT_REVIEWED_NOT_RELEASED', production_fix=False,
        waterline=load_run(waterline, ['baseline', 'transport-no-screens', 'transport-hybrid']),
        near=load_run(near, ['near-hybrid', 'near-no-screens']), metrics=[])
    for name in ('waterline', 'near'):
        if not report[name]['shader_manifest']['unified']:
            raise ValueError('Expected unified routing')
    for view in range(3):
        water = frame(waterline, 'transport-hybrid', view)
        air_error = float(np.abs(water-frame(waterline, 'baseline', view)).mean())
        water_toggle = float(np.abs(water-frame(waterline, 'transport-no-screens', view)).mean())
        near_toggle = float(np.abs(frame(near, 'near-hybrid', view)-frame(near, 'near-no-screens', view)).mean())
        metric = dict(view=view, waterline_control_mae=air_error,
            waterline_screen_toggle_mae=water_toggle, near_screen_toggle_mae=near_toggle)
        if view < 2:
            if water_toggle > 1 or near_toggle > 1:
                raise ValueError('Underwater screen-toggle consistency exceeds one display RGB level')
            metric['waterline_region'] = region(water)
            if metric['waterline_region']['blue_dominant_fraction'] > .01 or metric['waterline_region']['mean_luma'] < 4:
                raise ValueError('Blue-region correction lost')
        elif air_error > 1:
            raise ValueError('Air control differs by more than one RGB display level')
        report['metrics'].append(metric)
    report['metric_scope'] = 'Full-image display RGB MAE; screen toggles also change non-water reflections. Inspect near detail and the Snell window visually; small MAE alone does not prove physical correctness.'
    report['performance'] = {}
    for label, directory, expected_control in [('control', control, True), ('unified', active, False)]:
        summary = json.loads((directory / 'Summary.json').read_text(encoding='utf-8-sig'))
        metadata = summary['metadata']
        if not metadata['reflection_transport'] or not metadata['reflection_unified'] or metadata['reflection_control'] != expected_control:
            raise ValueError('Unexpected performance mode')
        report['performance'][label] = dict(summary=summary, evidence=[bind(directory / f) for f in
            ['Summary.json', 'Metadata.json', 'Host.json', 'Capture.csv', 'Frame.png', 'Runtime.log', 'ShaderManifest.json']])
    a, b = [report['performance'][key]['summary'] for key in ('control', 'unified')]
    for key in ('width', 'height', 'quality', 'requested_capture_seconds', 'r.ScreenPercentage', 'r.CustomDepth', 'r.Lumen.HardwareRayTracing.Inline'):
        if a['metadata'][key] != b['metadata'][key]:
            raise ValueError(f'Mismatched benchmark setting: {key}')
    report['performance']['fps_change_percent'] = 100 * (b['average_fps'] / a['average_fps'] - 1)
    report['performance']['scope'] = 'Two sequential source-editor runtime captures on this host; moving simulation/camera, not a fixed-frame GPU microbenchmark or packaged release acceptance.'
    report['build_evidence'] = [bind(ROOT / 'artifacts/reflection-unified-study' / name) for name in ('editor-build.log', 'perf-build.log')]
    report['binding_patch'] = json.loads((ROOT / 'artifacts/reflection-renderer-patch/Manifest.json').read_text())
    output = ROOT / 'Documentation/REFLECTION_UNIFIED_RESULTS.json'
    output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(output)


if __name__ == '__main__':
    main()
