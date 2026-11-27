"""Prepare diagnostic Lumen ray classification in project-local shader storage."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '981982dfc21c7ecba02b354d736292a0920df2d8ab654ad5ffa465344f9b8ce1'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, default=Path('C:/Program Files/Epic Games/UE_5.8'))
    parser.add_argument('--project', type=Path, default=ROOT / 'Builds/EditorSandbox/divingsim')
    parser.add_argument('--mode', choices=('classification', 'direction'), default='classification')
    args = parser.parse_args()
    project = args.project.resolve()
    if not project.is_relative_to(ROOT) or not (project / 'divingsim.uproject').is_file():
        raise ValueError('Use an existing project within this checkout')
    source = args.engine / 'Engine/Shaders/Private/Lumen'
    original = source / 'LumenReflectionHardwareRayTracing.usf'
    if sha(original) != EXPECTED:
        raise ValueError('Unreviewed Lumen reflection shader revision')
    code = original.read_text(encoding='utf-8')
    sky = '\t\tif ((ApplySkyLight != 0) && !Result.bIsHit)'
    output = '\t\tRWTraceRadiance[ReflectionTracingCoord.CoordFlatten] = MakeFinite(TraceResult.Lighting);'
    if code.count(sky) != 1 or code.count(output) != 1:
        raise ValueError('Ambiguous classification anchors')
    code = code.replace(sky, '\t\tbool DiveUsedSkyFallback = (ApplySkyLight != 0) && !Result.bIsHit;\n' + sky)
    classification = '''		// Diagnostic radiance only. Magenta: primary skylight fallback;
		// green: resolved hit (geometry, distant screen trace or cache);
		// yellow: unresolved without skylight fallback. These are not game colours.
		TraceResult.Lighting = 5000 * View.PreExposure * (DiveUsedSkyFallback ? float3(1,0,1)
			: Result.bIsHit ? float3(0,1,0) : float3(1,1,0));
'''
    direction = '''		// Diagnostic sky misses only: red travels downward, cyan upward.
		// Resolved hits remain green. No physical radiance is shown.
		TraceResult.Lighting = 5000 * View.PreExposure * (DiveUsedSkyFallback
			? (Ray.Direction.z < 0 ? float3(1,0,0) : float3(0,1,1))
			: Result.bIsHit ? float3(0,1,0) : float3(1,1,0));
'''
    code = code.replace(output, (classification if args.mode == 'classification' else direction) + output)
    destination = project / 'Intermediate/DiveReflectionRayStudy/Lumen'
    if not destination.resolve().is_relative_to(project / 'Intermediate'):
        raise ValueError('Shader output escapes project Intermediate')
    # Longest-prefix resolution requires the complete mapped directory.
    shutil.copytree(source, destination, dirs_exist_ok=True)
    patched = destination / original.name
    patched.write_text(code, encoding='utf-8', newline='\n')
    assert sha(original) == EXPECTED
    report = dict(original=str(original), original_sha256=EXPECTED, patched_sha256=sha(patched),
                  mapping='/Engine/Private/Lumen', destination=str(destination),
                  mode=args.mode,
                  scope='Diagnostic primary skylight fallback versus resolved Lumen reflection hit',
                  colours=(dict(magenta='primary skylight fallback', green='resolved hit', yellow='unresolved without skylight')
                           if args.mode == 'classification' else
                           dict(red='downward primary sky miss', cyan='upward primary sky miss', green='resolved hit', yellow='unresolved without skylight')),
                  files=[dict(file=p.relative_to(destination).as_posix(), sha256=sha(p))
                         for p in sorted(destination.rglob('*')) if p.is_file()])
    (destination.parent / 'Manifest.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'REFLECTION_RAY_STUDY_PREPARED={destination}')


if __name__ == '__main__':
    main()
