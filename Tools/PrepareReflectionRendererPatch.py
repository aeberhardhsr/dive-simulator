"""Prepare the custom-depth binding for selective water reflection routing."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RELATIVE = Path('Engine/Source/Runtime/Renderer/Private/Lumen/LumenReflectionTracing.cpp')
EXPECTED = 'a7df97fda68c15fa2464fc2a31b22dd1118ab54d4984f08757cfcb051c7813f3'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true', help='Apply only to the separate Builds/UE584 source checkout')
    args = parser.parse_args()
    engine = (ROOT / 'Builds/UE584').resolve()
    source = engine / RELATIVE
    original = source.read_bytes()
    if sha(original) != EXPECTED:
        raise ValueError('Unreviewed source-engine reflection tracing revision')
    text = original.decode('utf-8').replace('\r\n', '\n')
    marker = '''// Project capability: screen reflection pass binds custom water depth/stencil.
static TAutoConsoleVariable<int32> CVarDiveWaterReflectionBindingVersion(
    TEXT("r.DiveWater.ReflectionBindingVersion"), 1,
    TEXT("Compiled water reflection custom-depth binding; not a runtime switch."), ECVF_ReadOnly);

'''
    anchor = 'class FReflectionTraceScreenTexturesCS : public FGlobalShader'
    if text.count(anchor) != 1:
        raise ValueError('Ambiguous shader class')
    code = text.replace(anchor, marker + anchor)
    start = code.index(anchor)
    parameter = '\tBEGIN_SHADER_PARAMETER_STRUCT(FParameters, )\n'
    pos = code.index(parameter, start) + len(parameter)
    code = code[:pos] + '\t\tSHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneTextureUniformParameters, SceneTexturesStruct)\n' + code[pos:]
    allocation = '\t\tFReflectionTraceScreenTexturesCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FReflectionTraceScreenTexturesCS::FParameters>();'
    if code.count(allocation) != 1:
        raise ValueError('Ambiguous shader allocation')
    code = code.replace(allocation, allocation + '\n\t\tPassParameters->SceneTexturesStruct = SceneTextures.UniformBuffer;')
    newline = '\r\n' if b'\r\n' in original else '\n'
    output = ROOT / 'artifacts/reflection-renderer-patch'
    destination = output / RELATIVE
    destination.parent.mkdir(parents=True, exist_ok=True)
    data = code.replace('\n', newline).encode('utf-8')
    destination.write_bytes(data)
    (output / 'binding.patch').write_text(''.join(difflib.unified_diff(text.splitlines(True), code.splitlines(True), fromfile='a/'+RELATIVE.as_posix(), tofile='b/'+RELATIVE.as_posix(), n=2)), encoding='utf-8')
    if args.apply:
        if engine != (ROOT / 'Builds/UE584').absolute() or not source.resolve().is_relative_to(engine):
            raise ValueError('Only the real separate source checkout may be modified')
        if source.read_bytes() != original:
            raise ValueError('Source changed during preparation')
        source.write_bytes(data)
    (output / 'Manifest.json').write_text(json.dumps(dict(original_sha256=EXPECTED, prepared_sha256=sha(data), source=str(source), applied=args.apply, binding_version=1, installed_engine_modified=False), indent=2)+'\n', encoding='utf-8')
    print(f'REFLECTION_BINDING_PREPARED applied={args.apply} output={output}')


if __name__ == '__main__':
    main()
