"""Prepare an opt-in, project-local UE 5.8.4 deferred-sun shader experiment.

Copies installed shader sources into ignored project Intermediate storage.
Never writes engine files. Optionally includes the directional volume source.
"""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '2be1df4a8ddbd6d33005e58c7cc924dcb777f3dcc4fcce14f9b9c6292706eb57'
EXPECTED_VOLUME = 'e81073e5f622d73a08af916789df0fabd992cfa9fb9407df585c449dedf2dda9'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, default=Path('C:/Program Files/Epic Games/UE_5.8'))
    parser.add_argument('--project', type=Path, default=ROOT/'Builds/EditorSandbox/divingsim')
    parser.add_argument('--volume', action='store_true', help='Also absorb the directional source at each fog scattering sample')
    args = parser.parse_args()
    project = args.project.resolve()
    if not project.is_relative_to(ROOT) or not (project/'divingsim.uproject').is_file():
        raise ValueError('Use an existing project inside this checkout')
    source = args.engine/'Engine/Shaders/Private'
    original = source/'DeferredLightPixelShaders.usf'
    if digest(original) != EXPECTED:
        raise ValueError('Unreviewed Unreal deferred-light shader revision; do not apply this patch')
    code = original.read_text(encoding='utf-8')
    anchor = '// Affect the light color by any other shadow/transmittance before the lighting is evaluated'
    output = '\tLightDataColor *= AttenuationRGB;'
    if code.count(anchor) != 1 or code.count(output) != 1:
        raise ValueError('Ambiguous deferred-light patch anchors')
    code = code.replace(anchor, '#include "/DiveRendering/SolarTransport.ush"\n\n'+anchor)
    code = code.replace(output, '\tLightDataColor *= AttenuationRGB * DiveSunWaterTransmission(InTranslatedWorldPosition);')
    volume_original = source/'VolumetricFog.usf'
    volume_code = None
    if args.volume:
        if digest(volume_original) != EXPECTED_VOLUME:
            raise ValueError('Unreviewed Unreal volumetric shader revision')
        volume_code = volume_original.read_text(encoding='utf-8')
        volume_anchor = '#include "Common.ush"'
        volume_term = 'LightScattering += DirectionalLightColor * LightFunctionColor * ShadowFactor * PhaseFunction(PhaseG, dot(InDirectionalLightDirection, -CameraVector));'
        if volume_code.count(volume_anchor) != 1 or volume_code.count(volume_term) != 1:
            raise ValueError('Ambiguous volumetric patch anchors')
        volume_code = volume_code.replace(volume_anchor, volume_anchor+'\n#include "/DiveRendering/SolarMedium.ush"')
        volume_code = volume_code.replace(volume_term, volume_term.replace('DirectionalLightColor *',
            'DirectionalLightColor * DiveDirectionalWaterTransmission(TranslatedWorldPosition, InDirectionalLightDirection) *'))
    destination = project/'Intermediate'/('DiveSolarVolumeStudy' if args.volume else 'DiveSolarStudy')/'Private'
    # A complete local mirror is required because UE resolves the longest
    # virtual directory prefix; missing sibling shaders do not fall back.
    shutil.copytree(source, destination, dirs_exist_ok=True)
    patched = destination/original.name
    patched.write_text(code, encoding='utf-8', newline='\n')
    assert digest(original) == EXPECTED
    report = dict(engine_file=str(original), original_sha256=EXPECTED,
                  patched_sha256=digest(patched), mapping='/Engine/Private',
                  destination=str(destination), shader_include_sha256=digest(ROOT/'divingsim/Shaders/SolarTransport.ush'),
                  common_include_sha256=digest(ROOT/'divingsim/Shaders/SolarMedium.ush'),
                  volume_enabled=args.volume,
                  scope='Directional direct surfaces'+(' and per-sample fog source' if args.volume else '')+'; mean water plane; no GI update or lake footprint.')
    if args.volume:
        volume_patched = destination/volume_original.name
        volume_patched.write_text(volume_code, encoding='utf-8', newline='\n')
        assert digest(volume_original) == EXPECTED_VOLUME
        report['volume'] = dict(engine_file=str(volume_original), original_sha256=EXPECTED_VOLUME,
                                patched_sha256=digest(volume_patched))
    (destination.parent/'Manifest.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('DIVE_SOLAR_SHADER_PREPARED', destination)


if __name__ == '__main__':
    main()
