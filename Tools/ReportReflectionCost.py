"""Describe native reflection cost ablations without treating them as quality modes."""
import argparse
import json
from pathlib import Path

import numpy as np

from ReportReflectionTransport import ROOT, bind, frame, load_run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('full', 'routing', 'unshadowed', 'cached', 'visual', 'previous-visual'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    report = dict(status='PROFILED_NOT_RELEASED', production_fix=False, runs={})
    for name in ('full', 'routing', 'unshadowed', 'cached'):
        folder = (ROOT / getattr(args, name)).resolve()
        summary = json.loads((folder / 'Summary.json').read_text(encoding='utf-8-sig'))
        manifest = json.loads((folder / 'ShaderManifest.json').read_text(encoding='utf-8-sig'))
        log = (folder / 'Runtime.log').read_text(encoding='utf-8-sig', errors='replace')
        if manifest['profile'] != name or not manifest['unified'] or f'DIVE_REFLECTION_PROFILE_READY profile={name} ' not in log:
            raise ValueError('Mismatched diagnostic profile')
        meta = summary['metadata']
        if not meta['reflection_unified'] or meta['reflection_control'] or abs(meta['camera_rotation_degrees']['pitch']-25) > .01:
            raise ValueError('Unexpected reflection benchmark state')
        reflection = next(p['mean_ms'] for p in summary['top_gpu_passes_ms_not_additive'] if p['name'] == 'GPU/LumenReflections')
        report['runs'][name] = dict(directory=folder.relative_to(ROOT).as_posix(),
            fps=summary['average_fps'], low1_fps=summary['one_percent_low_fps'],
            frame=summary['frame'], gpu=summary['gpu'], reflection_gpu_ms=reflection,
            metadata=meta, shader_manifest=manifest,
            evidence=[bind(folder / f) for f in ('Summary.json', 'Host.json', 'Metadata.json', 'Runtime.log', 'ShaderManifest.json', 'Capture.csv', 'Frame.png')])
    base = report['runs']['full']
    for name in ('routing', 'unshadowed', 'cached'):
        for key in ('width', 'height', 'quality', 'requested_capture_seconds', 'r.ScreenPercentage', 'r.Lumen.HardwareRayTracing.Inline'):
            if base['metadata'][key] != report['runs'][name]['metadata'][key]:
                raise ValueError('Unmatched benchmark settings')
    report['reflection_pass_shadow_difference_ms'] = base['reflection_gpu_ms']-report['runs']['unshadowed']['reflection_gpu_ms']
    report['cached_reflection_change_percent'] = 100*(report['runs']['cached']['reflection_gpu_ms']/base['reflection_gpu_ms']-1)
    report['scope'] = 'Sequential moving source-editor runs on one host. Diagnostic removals identify dominant cost; they are not acceptable visual replacements. Camera paths and shader layouts may differ slightly; GPU pass durations are not additive.'
    report['decision'] = 'Keep the complete 16-step shadowed integration as the reference. Cached light constants were not promoted. Investigate reuse of existing scene shadows with valid-data checks and a ray-traced fallback.'
    report['reference_visual'] = load_run((ROOT / args.visual).resolve(), ['near-hybrid', 'near-no-screens'])
    if report['reference_visual']['shader_manifest']['profile'] != 'full':
        raise ValueError('Reference visuals must use complete integration')
    previous = (ROOT / args.previous_visual).resolve()
    report['previous_visual'] = load_run(previous, ['near-hybrid', 'near-no-screens'])
    report['restored_reference_rgb_mae'] = []
    for case in ('near-hybrid', 'near-no-screens'):
        for view in range(3):
            difference = float(np.abs(frame(ROOT / args.visual, case, view)-frame(previous, case, view)).mean())
            if difference > 1:
                raise ValueError('Restored full reference differs by over one display RGB level')
            report['restored_reference_rgb_mae'].append(dict(case=case, view=view, mean_absolute_rgb_difference=difference))
    output = ROOT / 'Documentation/REFLECTION_COST_RESULTS.json'
    output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(output)


if __name__ == '__main__':
    main()
