"""Prepare an isolated source-engine reflection-medium integration experiment."""
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
    parser.add_argument('--engine', type=Path, default=ROOT / 'Builds/UE584')
    parser.add_argument('--project', type=Path, default=ROOT / 'Builds/SourceEditorSandbox/divingsim')
    args = parser.parse_args()
    project = args.project.resolve()
    if not project.is_relative_to(ROOT) or not (project / 'divingsim.uproject').is_file():
        raise ValueError('Use an existing project in this checkout')
    source = args.engine / 'Engine/Shaders/Private/Lumen'
    original = source / 'LumenReflectionHardwareRayTracing.usf'
    if sha(original) != EXPECTED:
        raise ValueError('Unreviewed reflection shader revision')
    code = original.read_text(encoding='utf-8')
    include = 'LUMEN_HARDWARE_RAY_TRACING_ENTRY(LumenReflectionHardwareRayTracing)'
    output = '\t\tRWTraceRadiance[ReflectionTracingCoord.CoordFlatten] = MakeFinite(TraceResult.Lighting);'
    if code.count(include) != 1 or code.count(output) != 1:
        raise ValueError('Ambiguous reflection transport anchors')
    code = code.replace(include, '#include "/DiveRendering/ReflectionWater.ush"\n\n' + include)
    code = code.replace(output, '''#if !COMPUTESHADER
        // Only raw ray results: screen-hit radiance already contains the camera medium.
        if (bNeedsFog && !bNeedsHitLightingPass)
            TraceResult.Lighting = DiveIntegrateReflectionWater(TLAS, TraceResult.Lighting,
                TranslatedWorldPosition, Ray.Direction, ScreenUV, SceneDepth,
                Result.bIsHit ? TraceResult.OpaqueHitDistance : 1000000.0);
#endif
''' + output)
    destination = project / 'Intermediate/DiveReflectionTransportStudy/Lumen'
    if not destination.resolve().is_relative_to(project / 'Intermediate'):
        raise ValueError('Output escaped project')
    shutil.copytree(source, destination, dirs_exist_ok=True)
    patched = destination / original.name
    patched.write_text(code, encoding='utf-8', newline='\n')
    shutil.copyfile(ROOT / 'Tools/Shaders/WaterOptics.ush', destination / 'DiveReflectionWaterOptics.ush')
    report = dict(original_sha256=EXPECTED, patched_sha256=sha(patched),
                  shader_sha256=sha(ROOT / 'divingsim/Shaders/ReflectionWater.ush'),
                  scope='Experimental direct single scattering on raygen water front-layer results; screen hits unchanged; opaque near-field shadows; no multiple scattering',
                  files=[dict(file=p.relative_to(destination).as_posix(), sha256=sha(p))
                         for p in sorted(destination.rglob('*')) if p.is_file()])
    (destination.parent / 'Manifest.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    if sha(original) != EXPECTED:
        raise ValueError('Engine shader changed unexpectedly')
    print(f'REFLECTION_TRANSPORT_PREPARED={destination}')


if __name__ == '__main__':
    main()
