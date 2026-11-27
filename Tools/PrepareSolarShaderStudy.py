"""Prepare an opt-in, project-local UE 5.8.4 deferred-sun shader experiment.

Copies installed shader sources into ignored project Intermediate storage.
Never writes engine files. Only the copied deferred-light shader is patched.
"""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '2be1df4a8ddbd6d33005e58c7cc924dcb777f3dcc4fcce14f9b9c6292706eb57'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, default=Path('C:/Program Files/Epic Games/UE_5.8'))
    parser.add_argument('--project', type=Path, default=ROOT/'Builds/EditorSandbox/divingsim')
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
    destination = project/'Intermediate/DiveSolarStudy/Private'
    # A complete local mirror is required because UE resolves the longest
    # virtual directory prefix; missing sibling shaders do not fall back.
    shutil.copytree(source, destination, dirs_exist_ok=True)
    patched = destination/original.name
    patched.write_text(code, encoding='utf-8', newline='\n')
    assert digest(original) == EXPECTED
    report = dict(engine_file=str(original), original_sha256=EXPECTED,
                  patched_sha256=digest(patched), mapping='/Engine/Private',
                  destination=str(destination), shader_include_sha256=digest(ROOT/'divingsim/Shaders/SolarTransport.ush'),
                  scope='Deferred directional direct lighting only; mean water plane; no GI or volumetric update.')
    (destination.parent/'Manifest.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('DIVE_SOLAR_SHADER_PREPARED', destination)


if __name__ == '__main__':
    main()
