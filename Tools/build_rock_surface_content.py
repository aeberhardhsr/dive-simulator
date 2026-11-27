"""Matching UV-space scan materials with the shared wet lake coating."""
import hashlib
import json
from pathlib import Path
import sys
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'Tools'))
from texture_color import set_scan_source_encoding
BASE='/Game/Diving'


def build_rock_surfaces(manifest=None,report_name='RockSurfaceContentBuild.json',max_texture_size=2048):
    if manifest is None:manifest=json.loads((ROOT/'Documentation/ROCK_SURFACES.json').read_text(encoding='utf-8'))
    lib=u.MaterialEditingLibrary;tools=u.AssetToolsHelpers.get_asset_tools()
    editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)
    shared=(ROOT/'Tools/Shaders/Lakebed.ush').read_text()
    code=shared.split('BedSurface bed;')[0]+'''BedSurface bed;
float3 n=normalize(N);
float3 color=lerp(dot(Color,float3(.2126,.7152,.0722)).xxx,Color,.7);
SurfaceNormal=normalize(ScanNormal);
Roughness=ARM.g; Occlusion=ARM.r;
float IsRock=1;
'''+shared[shared.index('float wet='):]
    records=[]
    for source in manifest['assets']:
        index=int(source['mesh'].removeprefix('SM_LakeRock'))-1
        textures={}
        for kind,pin in [('Diffuse','Color'),('nor_dx','ScanNormal'),('arm','ARM')]:
            item=source['maps'][kind];p=ROOT/item['file']
            assert hashlib.sha256(p.read_bytes()).hexdigest()==item['sha256'],str(p)
            name=f'T_RockScan{index+1}_{pin}';path=BASE+'/Textures/'+name
            texture=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
            if not texture or u.EditorAssetLibrary.get_metadata_tag(texture,'DiveScanSHA256')!=item['sha256']:
                task=u.AssetImportTask()
                for key,value in dict(filename=str(p),destination_path=BASE+'/Textures',destination_name=name,automated=True,replace_existing=True,save=False).items():task.set_editor_property(key,value)
                tools.import_asset_tasks([task]);texture=u.load_asset(path)
                if not texture:raise RuntimeError('Missing texture '+path)
                u.EditorAssetLibrary.set_metadata_tag(texture,'DiveScanSHA256',item['sha256'])
            for key,value in dict(srgb=kind=='Diffuse',compression_settings=u.TextureCompressionSettings.TC_NORMALMAP if kind=='nor_dx' else u.TextureCompressionSettings.TC_MASKS if kind=='arm' else u.TextureCompressionSettings.TC_DEFAULT,
                flip_green_channel=False,lod_bias=0,max_texture_size=max_texture_size,never_stream=False).items():texture.set_editor_property(key,value)
            set_scan_source_encoding(texture, kind == 'Diffuse')
            if not u.EditorAssetLibrary.save_loaded_asset(texture):raise RuntimeError(path)
            textures[pin]=texture
        name=f'M_RockScan{index+1}';path=BASE+'/Materials/'+name
        m=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(name,BASE+'/Materials',u.Material,u.MaterialFactoryNew())
        for node in list(lib.get_material_expressions(m)):lib.delete_material_expression(m,node)
        for key,value in dict(two_sided=False,tangent_space_normal=True,used_with_nanite=True,used_with_instanced_static_meshes=True,shading_model=u.MaterialShadingModel.MSM_DEFAULT_LIT,blend_mode=u.BlendMode.BLEND_OPAQUE).items():m.set_editor_property(key,value)
        fn=lib.create_material_expression(m,u.MaterialExpressionCustom)
        fn.set_editor_property('code',code);fn.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
        inputs=[]
        for pin in ['P','N','Color','ScanNormal','ARM']:
            item=u.CustomInput();item.set_editor_property('input_name',pin);inputs.append(item)
        fn.set_editor_property('inputs',inputs)
        outputs=[]
        for pin in ['SurfaceNormal','Roughness','Occlusion']:
            item=u.CustomOutput();item.set_editor_property('output_name',pin);item.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3 if pin=='SurfaceNormal' else u.CustomMaterialOutputType.CMOT_FLOAT1);outputs.append(item)
        fn.set_editor_property('additional_outputs',outputs)
        for pin,cls in [('P',u.MaterialExpressionWorldPosition),('N',u.MaterialExpressionVertexNormalWS)]:
            node=lib.create_material_expression(m,cls);lib.connect_material_expressions(node,'',fn,pin)
        for pin,texture in textures.items():
            node=lib.create_material_expression(m,u.MaterialExpressionTextureSample)
            node.set_editor_property('texture',texture)
            node.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL if pin=='ScanNormal' else u.MaterialSamplerType.SAMPLERTYPE_MASKS if pin=='ARM' else u.MaterialSamplerType.SAMPLERTYPE_COLOR)
            # Native UV0 and normal decoding preserve the scan/geometry match.
            lib.connect_material_expressions(node,'RGB',fn,pin)
        for pin,prop in [('',u.MaterialProperty.MP_BASE_COLOR),('SurfaceNormal',u.MaterialProperty.MP_NORMAL),('Roughness',u.MaterialProperty.MP_ROUGHNESS),('Occlusion',u.MaterialProperty.MP_AMBIENT_OCCLUSION)]:lib.connect_material_property(fn,pin,prop)
        lib.recompile_material(m)
        u.EditorAssetLibrary.set_metadata_tag(m,'DiveScanShaderSHA256',hashlib.sha256(code.encode()).hexdigest())
        if not u.EditorAssetLibrary.save_loaded_asset(m):raise RuntimeError(path)
        mesh=u.load_asset('/Game/Diving/Rocks/'+source['mesh'])
        if not mesh or editor.get_num_uv_channels(mesh,0)<1:raise RuntimeError('Missing source UV0 '+source['mesh'])
        mesh.set_material(0,m)
        if not u.EditorAssetLibrary.save_loaded_asset(mesh):raise RuntimeError('Cannot assign scan material')
        records.append(dict(mesh=source['mesh'],material=path,uv_channels=editor.get_num_uv_channels(mesh,0),textures={k:v.get_path_name() for k,v in textures.items()},shader_sha256=hashlib.sha256(code.encode()).hexdigest()))
    (ROOT/'artifacts'/report_name).write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')
    u.log(f'DIVE_ROCK_SURFACE_CONTENT_PASS variants={len(records)} uv0=1 native_normal_decode=1')


if __name__=='__main__':build_rock_surfaces()
