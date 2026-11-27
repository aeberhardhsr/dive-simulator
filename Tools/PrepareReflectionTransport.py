"""Prepare an isolated source-engine reflection-medium integration experiment."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '981982dfc21c7ecba02b354d736292a0920df2d8ab654ad5ffa465344f9b8ce1'
EXPECTED_SCREEN = '6397e9feae73a76f4d0bc4d81e6f01cc641e4175b5a04f8e16fdaedae12d4f28'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_once(code, old, new):
    if code.count(old) != 1:
        raise ValueError(f'Ambiguous routing anchor: {old}')
    return code.replace(old, new)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, default=ROOT / 'Builds/UE584')
    parser.add_argument('--project', type=Path, default=ROOT / 'Builds/SourceEditorSandbox/divingsim')
    parser.add_argument('--unified', action='store_true', help='Route only underwater water reflections through hardware tracing')
    parser.add_argument('--profile', choices=['full', 'routing', 'unshadowed', 'cached', 'vsm', 'vsm-fallback', 'vsm-coverage'], default='full',
                        help='Diagnostic ablation; routing and unshadowed are not visual fixes')
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
    profile = ['full', 'routing', 'unshadowed', 'cached', 'vsm', 'vsm-fallback', 'vsm-coverage'].index(args.profile)
    code = code.replace(include, f'#define DIVE_REFLECTION_PROFILE {profile}\n#include "/DiveRendering/ReflectionWater.ush"\n\n' + include)
    code = code.replace(output, '''#if !COMPUTESHADER
        // Only raw ray results: screen-hit radiance already contains the camera medium.
        if (bNeedsFog && !bNeedsHitLightingPass)
            TraceResult.Lighting = DiveIntegrateReflectionWater(TLAS, TraceResult.Lighting,
                TranslatedWorldPosition, Ray.Direction, ScreenUV, SceneDepth,
                Result.bIsHit ? TraceResult.OpaqueHitDistance : 1000000.0);
#endif
''' + output)
    screen_original = source / 'LumenReflectionTracing.usf'
    screen_code = None
    if args.unified:
        if sha(screen_original) != EXPECTED_SCREEN:
            raise ValueError('Unreviewed screen-tracing shader revision')
        origin = '\t\tconst bool bIsFirstPerson = RayData.bIsFirstPersonPixel;'
        code = replace_once(code, origin, origin+'\n\t\tbool DiveWaterRay = DiveIsWaterReflection(TranslatedWorldPosition, ScreenUV, SceneDepth);')
        for old, new in [
            ('TraceMaxDistance = min(TraceMaxDistance, RayData.RadianceCacheMaxTraceDistance);', 'if (!DiveWaterRay) TraceMaxDistance = min(TraceMaxDistance, RayData.RadianceCacheMaxTraceDistance);'),
            ('if (Result.bIsHit && (HitLightingForceEnabled != 0 || !Result.bIsRadianceCompleted))', 'if (Result.bIsHit && (DiveWaterRay || HitLightingForceEnabled != 0 || !Result.bIsRadianceCompleted))'),
            ('if (Result.bIsHit && SampleSceneColor > 0)', 'if (Result.bIsHit && SampleSceneColor > 0 && !DiveWaterRay)'),
            ('if (!Result.bIsHit)', 'if (!Result.bIsHit && !DiveWaterRay)'),
            ('if (!Result.bIsHit && RayData.bUseRadianceCache)', 'if (!Result.bIsHit && RayData.bUseRadianceCache && !DiveWaterRay)')]:
            code = replace_once(code, old, new)
        screen_code = screen_original.read_text(encoding='utf-8')
        screen_start = '#ifdef ReflectionTraceScreenTexturesCS'
        screen_code = replace_once(screen_code, screen_start, screen_start+'\n#include "/DiveRendering/ReflectionWaterMask.ush"')
        begin = screen_code.index(screen_start)
        end = screen_code.index('RWBuffer<uint> RWCompactedTraceTexelAllocator;', begin)
        block = screen_code[begin:end]
        ray = '\t\t\tFRayData RayData = GetRayData(ReflectionTracingCoord.CoordFlatten);'
        block = replace_once(block, ray, ray+'''\n            // ClearTraces leaves an unresolved ray at distance zero. Continue
            // in hardware for marked underwater water, preserving air SSR.
            if (DiveIsWaterReflection(TranslatedWorldPosition, ScreenUV, SceneDepth)) return;''')
        screen_code = screen_code[:begin]+block+screen_code[end:]
    destination = project / 'Intermediate/DiveReflectionTransportStudy/Lumen'
    if not destination.resolve().is_relative_to(project / 'Intermediate'):
        raise ValueError('Output escaped project')
    shutil.copytree(source, destination, dirs_exist_ok=True)
    patched = destination / original.name
    patched.write_text(code, encoding='utf-8', newline='\n')
    if screen_code is not None:
        (destination / screen_original.name).write_text(screen_code, encoding='utf-8', newline='\n')
    (destination.parent / 'RoutingMode.txt').write_text('unified' if args.unified else 'hybrid', encoding='utf-8')
    (destination.parent / 'ProfileMode.txt').write_text(args.profile, encoding='utf-8')
    shutil.copyfile(ROOT / 'Tools/Shaders/WaterOptics.ush', destination / 'DiveReflectionWaterOptics.ush')
    report = dict(original_sha256=EXPECTED, patched_sha256=sha(patched),
                  shader_sha256=sha(ROOT / 'divingsim/Shaders/ReflectionWater.ush'),
                  mask_sha256=sha(ROOT / 'divingsim/Shaders/ReflectionWaterMask.ush'),
                  shadow_sha256=sha(ROOT / 'divingsim/Shaders/ReflectionWaterShadow.ush'),
                  unified=args.unified, profile=args.profile, screen_original_sha256=EXPECTED_SCREEN,
                  scope=('Underwater water routed through raw hardware hit lighting; air SSR unchanged' if args.unified else 'Raygen water front-layer results; screen hits unchanged')+'; direct single scattering; opaque near-field shadows; no multiple scattering',
                  files=[dict(file=p.relative_to(destination).as_posix(), sha256=sha(p))
                         for p in sorted(destination.rglob('*')) if p.is_file()])
    (destination.parent / 'Manifest.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    if sha(original) != EXPECTED:
        raise ValueError('Engine shader changed unexpectedly')
    print(f'REFLECTION_TRANSPORT_PREPARED={destination}')


if __name__ == '__main__':
    main()
