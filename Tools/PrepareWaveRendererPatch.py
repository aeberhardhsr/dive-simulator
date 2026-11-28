"""Prepare (never install) the UE 5.8.4 scene bindings for wave transport.

Reads an authorized engine tree. Modified Epic sources and the review diff stay
in ignored artifacts; only this small transformation is part of the project.
This does not build an engine or certify its binary compatibility.
"""
import argparse
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRIVATE = Path('Engine/Source/Runtime/Renderer/Private')
EXPECTED = {
    'LightRendering.cpp': 'd4280790d78f8350826c3b34f619a6e7fc3987f9ed49736e5be9d07e9f08f9f9',
    'VolumetricFog.cpp': '6b5c3a886405560f37f567f678f66de33338565ed9c88d4a3904ebdfd2220d51',
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def replace_exact(text, old, new, count=1):
    if text.count(old) != count:
        raise ValueError('Renderer patch anchor count changed; review required')
    return text.replace(old, new)


def add_scene_parameter(text, shader_class):
    start = text.index('class '+shader_class+' :')
    anchor = '\tBEGIN_SHADER_PARAMETER_STRUCT(FParameters, )\n'
    position = text.index(anchor, start) + len(anchor)
    return text[:position] + '\t\tSHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformParameters, Scene)\n' + text[position:]


def transform(name, text):
    if name == 'LightRendering.cpp':
        text = add_scene_parameter(text, 'FDeferredLightPS')
        text = replace_exact(text, '\tFDeferredLightPS::FParameters Out;\n',
            '\tFDeferredLightPS::FParameters Out;\n\tOut.Scene = GetSceneUniformBufferRef(GraphBuilder, View);\n', 2)
        marker = '''// Project capability marker: both deferred and volumetric passes bind Scene.
static TAutoConsoleVariable<int32> CVarDiveWaterSceneBindingVersion(
    TEXT("r.DiveWater.SceneBindingVersion"), 1,
    TEXT("Compiled DiveWater scene binding revision; not a runtime switch."), ECVF_ReadOnly);

'''
        text = replace_exact(text, 'class FDeferredLightPS :', marker+'class FDeferredLightPS :')
    else:
        text = add_scene_parameter(text, 'FVolumetricFogLightScatteringCS')
        anchor = '\t\t\tFVolumetricFogLightScatteringCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FVolumetricFogLightScatteringCS::FParameters>();\n'
        # This call is inside FSceneRenderer::ComputeVolumetricFog. Its member
        # overload hides the namespace-level helper taking (GraphBuilder, View).
        text = replace_exact(text, anchor, anchor+'\t\t\tPassParameters->Scene = GetSceneUniformBufferRef(GraphBuilder);\n')
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, default=Path('C:/Program Files/Epic Games/UE_5.8'))
    parser.add_argument('--output', type=Path, default=ROOT/'artifacts/wave-renderer-patch')
    args = parser.parse_args()
    engine, output = args.engine.resolve(), args.output.resolve()
    if not output.is_relative_to(ROOT/'artifacts') or output.is_relative_to(engine):
        raise ValueError('Output must be inside checkout artifacts and outside the engine tree')
    prepared = []
    # Validate every source before producing any modified file.
    for name, expected in EXPECTED.items():
        source = engine/PRIVATE/name
        original = source.read_bytes()
        if digest(original) != expected:
            raise ValueError(f'Unreviewed Renderer revision: {name}')
        text = original.decode('utf-8').replace('\r\n', '\n')
        modified = transform(name, text)
        newline = '\r\n' if b'\r\n' in original else '\n'
        prepared.append((name, original, text, modified, modified.replace('\n', newline).encode('utf-8')))
    diff = []
    report = {'status': 'prepared_not_compiled', 'engine': str(engine),
              'binding_revision': 1, 'installed_engine_modified': False, 'files': []}
    for name, original, text, modified, data in prepared:
        destination = output/PRIVATE/name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        relative = (PRIVATE/name).as_posix()
        diff.extend(difflib.unified_diff(text.splitlines(True), modified.splitlines(True),
                    fromfile='a/'+relative, tofile='b/'+relative, n=2))
        report['files'].append({'path': relative, 'original_sha256': digest(original),
                                'prepared_sha256': digest(data)})
        if (engine/PRIVATE/name).read_bytes() != original:
            raise RuntimeError('Engine input changed during preparation')
    (output/'renderer-scene-binding.patch').write_text(''.join(diff), encoding='utf-8', newline='\n')
    (output/'Manifest.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('DIVE_WAVE_RENDERER_PATCH_PREPARED', output)


if __name__ == '__main__':
    main()
