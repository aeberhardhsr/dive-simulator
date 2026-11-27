"""Prepare source-engine VSM bindings for water reflection shadows."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REL = Path('Engine/Source/Runtime/Renderer/Private/Lumen')
EXPECTED = {
    'LumenReflections.h': '55e9d92a170566bb92c85efa77761eebe8e1dee0b15639258c9e705e80258324',
    'LumenReflections.cpp': 'f623b811b1425d8f37aa6d01ab89943a09c9cfc2c4bc00124839039ccca18de7',
    'LumenReflectionTracing.cpp': '769e09bccd456f77b0654d8a48d04c4bf912d4883394b4449033edb7dd083e02',
    'LumenReflectionHardwareRayTracing.cpp': '93e09637fac827551f478baeb045290ea704bc93463e68ede15e05e79bbdd8a9',
}
ARG = '\tconst FVirtualShadowMapSamplingParameters& DiveShadowSampling,\n'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def replace(code, old, new, count=1):
    if code.count(old) != count:
        raise ValueError(f'Ambiguous patch anchor: {old}')
    return code.replace(old, new)


def argument(code, function):
    start = code.index('void ' + function + '(\n')
    end = code.index(')', start)
    block = code[start:end]
    block = replace(block, '\tconst FViewInfo& View,\n', '\tconst FViewInfo& View,\n' + ARG)
    return code[:start] + block + code[end:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    engine = (ROOT / 'Builds/UE584').resolve()
    if engine != (ROOT / 'Builds/UE584').absolute():
        raise ValueError('Only the separate real source checkout is supported')
    originals, sources = {}, {}
    for name, expected in EXPECTED.items():
        path = engine / REL / name
        if not path.resolve().is_relative_to(engine):
            raise ValueError('Source path escaped checkout')
        data = path.read_bytes()
        if sha(data) != expected:
            raise ValueError(f'Unreviewed source revision: {name}')
        originals[name] = data
        sources[name] = data.decode('utf-8').replace('\r\n', '\n')
    code = sources['LumenReflections.h']
    code = replace(code, 'struct FSceneTextures;', 'struct FSceneTextures;\nclass FVirtualShadowMapSamplingParameters;')
    for function in ('TraceReflections', 'RenderLumenHardwareRayTracingReflections'):
        code = argument(code, function)
    sources['LumenReflections.h'] = code
    code = sources['LumenReflections.cpp']
    anchor = '\tTraceReflections(\n\t\tGraphBuilder,\n\t\tScene,\n\t\tView,\n'
    setup = '''\t// Water reflections may reuse only the current view's shadow resources.
    auto DiveShadowSampling = FVirtualShadowMapArray::CreateDummySamplingParameters(GraphBuilder);
    if (VirtualShadowMapArray.IsAllocated())
    {
        for (int32 DiveViewIndex = 0; DiveViewIndex < Views.Num(); ++DiveViewIndex)
        {
            if (&Views[DiveViewIndex] == &View)
            {
                DiveShadowSampling = VirtualShadowMapArray.GetSamplingParameters(GraphBuilder, DiveViewIndex);
                break;
            }
        }
    }

'''
    code = replace(code, anchor, setup + anchor + '\t\tDiveShadowSampling,\n')
    sources['LumenReflections.cpp'] = code
    code = argument(sources['LumenReflectionTracing.cpp'], 'TraceReflections')
    anchor = '\t\tRenderLumenHardwareRayTracingReflections(\n\t\t\tGraphBuilder,\n\t\t\tSceneTextures,\n\t\t\tSceneTextureParameters,\n\t\t\tScene,\n\t\t\tView,\n'
    code = replace(code, anchor, anchor + '\t\t\tDiveShadowSampling,\n')
    sources['LumenReflectionTracing.cpp'] = code
    code = sources['LumenReflectionHardwareRayTracing.cpp']
    include = '#include "DeferredShadingRenderer.h"'
    code = replace(code, include, include + '\n#include "VirtualShadowMaps/VirtualShadowMapArray.h"')
    marker = '''static TAutoConsoleVariable<int32> CVarDiveReflectionShadowBindingVersion(
    TEXT("r.DiveWater.ReflectionShadowBindingVersion"), 1,
    TEXT("Compiled water reflection VSM binding; not a runtime switch."), ECVF_ReadOnly);

'''
    code = replace(code, 'static TAutoConsoleVariable<int32> CVarLumenReflectionsHardwareRayTracing(', marker + 'static TAutoConsoleVariable<int32> CVarLumenReflectionsHardwareRayTracing(')
    # Only the reflection shader, not the later ray-traced translucency shader.
    start = code.index('class FLumenReflectionHardwareRayTracing :')
    end = code.index('IMPLEMENT_LUMEN_RAYGEN_AND_COMPUTE_RAYTRACING_SHADERS(FLumenReflectionHardwareRayTracing)', start)
    block = code[start:end]
    anchor = '\t\tSHADER_PARAMETER_STRUCT_INCLUDE(FLumenHardwareRayTracingShaderBase::FSharedParameters, SharedParameters)'
    block = replace(block, anchor, anchor + '\n\t\tSHADER_PARAMETER_STRUCT_INCLUDE(FVirtualShadowMapSamplingParameters, DiveShadowSampling)')
    anchor = '\t\tFLumenHardwareRayTracingShaderBase::ModifyCompilationEnvironment(Parameters, ShaderDispatchType, Lumen::ESurfaceCacheSampling::HighResPages, OutEnvironment);'
    block = replace(block, anchor, anchor + '\n\t\tFVirtualShadowMapArray::SetShaderDefines(OutEnvironment);')
    code = code[:start] + block + code[end:]
    for function in ('DispatchRayGenOrComputeShader', 'RenderLumenHardwareRayTracingReflections'):
        code = argument(code, function)
    anchor = '\tFLumenReflectionHardwareRayTracing::FParameters* Parameters = GraphBuilder.AllocParameters<FLumenReflectionHardwareRayTracing::FParameters>();'
    code = replace(code, anchor, anchor + '\n\tParameters->DiveShadowSampling = DiveShadowSampling;')
    anchor = 'DispatchRayGenOrComputeShader(GraphBuilder, SceneTextures, SceneTextureParameters, Scene, View, TracingParameters,'
    code = replace(code, anchor, 'DispatchRayGenOrComputeShader(GraphBuilder, SceneTextures, SceneTextureParameters, Scene, View, DiveShadowSampling, TracingParameters,', 3)
    sources['LumenReflectionHardwareRayTracing.cpp'] = code
    output = ROOT / 'artifacts/reflection-shadow-patch'
    output.mkdir(parents=True, exist_ok=True)
    records, diffs, prepared = [], [], {}
    for name, code in sources.items():
        original = originals[name]
        newline = '\r\n' if b'\r\n' in original else '\n'
        prepared[name] = code.replace('\n', newline).encode('utf-8')
        destination = output / REL / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(prepared[name])
        records.append(dict(file=(REL / name).as_posix(), original_sha256=sha(original), prepared_sha256=sha(prepared[name])))
        diffs.extend(difflib.unified_diff(original.decode('utf-8').replace('\r\n', '\n').splitlines(True), code.splitlines(True), fromfile='a/'+(REL/name).as_posix(), tofile='b/'+(REL/name).as_posix(), n=2))
    (output / 'binding.patch').write_text(''.join(diffs), encoding='utf-8')
    if args.apply:
        for name, original in originals.items():
            if (engine / REL / name).read_bytes() != original:
                raise ValueError('Source changed during preparation')
        for name, data in prepared.items():
            (engine / REL / name).write_bytes(data)
    (output / 'Manifest.json').write_text(json.dumps(dict(binding_version=1, applied=args.apply, installed_engine_modified=False, files=records), indent=2)+'\n', encoding='utf-8')
    print(f'REFLECTION_SHADOW_BINDING_PREPARED applied={args.apply} output={output}')


if __name__ == '__main__':
    main()
