"""Unreal: isolated lawn material, authored coverage and three grass mesh LODs."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
BASE='/Game/Diving'
lib=u.MaterialEditingLibrary
tools=u.AssetToolsHelpers.get_asset_tools()
editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)
def props(obj,**values):
    for k,v in values.items():obj.set_editor_property(k,v)
    return obj
def connect(a,p,b,q):
    if not lib.connect_material_expressions(a,p,b,q):raise RuntimeError('Graph connection '+q)
def save(asset):
    lib.recompile_material(asset) if isinstance(asset,u.Material) else None
    if not u.EditorAssetLibrary.save_loaded_asset(asset):raise RuntimeError('Asset save failed')

data=json.loads((ROOT/'divingsim/Content/Diving/Geodata/lawn.json').read_text())
source=ROOT/data['mask']['file']
assert hashlib.sha256(source.read_bytes()).hexdigest()==data['mask']['sha256']
task=props(u.AssetImportTask(),filename=str(source),destination_path=BASE+'/Textures',destination_name='T_WeesenLawnMask',automated=True,replace_existing=True,save=False)
tools.import_asset_tasks([task]);texture=u.load_asset(BASE+'/Textures/T_WeesenLawnMask')
props(texture,srgb=False,compression_settings=u.TextureCompressionSettings.TC_MASKS,address_x=u.TextureAddress.TA_CLAMP,address_y=u.TextureAddress.TA_CLAMP,never_stream=True,lod_bias=0,max_texture_size=1024)
save(texture)
path=BASE+'/Materials/M_ShoreLawn'
material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(BASE+'/Materials/M_ShoreBank',path)
original=u.load_asset(BASE+'/Materials/M_ShoreBank')
custom=next(x for x in lib.get_material_expressions(material) if isinstance(x,u.MaterialExpressionCustom))
code=next(x for x in lib.get_material_expressions(original) if isinstance(x,u.MaterialExpressionCustom)).get_editor_property('code').replace('\r\n','\n')
anchor='    // World-space variation softens slope contours without moving the mesh.'
assert code.count(anchor)==1
code=code.replace(anchor,'    green=lerp(green,1,saturate(LawnCoverage));\n    forest*=1-saturate(LawnCoverage);\n'+anchor)
anchor='    foliage*=lerp(.90,1.06,broad);'
assert code.count(anchor)==1
code=code.replace(anchor,'    grass*=lerp(1,.82,saturate(LawnCoverage));\n'+anchor)
custom.set_editor_property('code',code)
inputs=list(custom.get_editor_property('inputs'))
if not any(str(x.get_editor_property('input_name'))=='LawnCoverage' for x in inputs):
    inputs.append(props(u.CustomInput(),input_name='LawnCoverage'));custom.set_editor_property('inputs',inputs)
nodes=[x for x in lib.get_material_expressions(material) if isinstance(x,u.MaterialExpressionTextureSampleParameter2D) and str(x.get_editor_property('parameter_name'))=='LawnMask']
node=nodes[0] if nodes else lib.create_material_expression(material,u.MaterialExpressionTextureSampleParameter2D)
props(node,parameter_name='LawnMask',texture=texture,sampler_type=u.MaterialSamplerType.SAMPLERTYPE_MASKS)
connect(node,'R',custom,'LawnCoverage');save(material)

path=BASE+'/Materials/M_LawnGrass'
grass=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset('M_LawnGrass',BASE+'/Materials',u.Material,u.MaterialFactoryNew())
for node in list(lib.get_material_expressions(grass)):lib.delete_material_expression(grass,node)
props(grass,two_sided=True,tangent_space_normal=False,blend_mode=u.BlendMode.BLEND_OPAQUE,shading_model=u.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE,used_with_instanced_static_meshes=True,dithered_lod_transition=True)
v=lib.create_material_expression(grass,u.MaterialExpressionVertexColor)
c=lib.create_material_expression(grass,u.MaterialExpressionCustom)
props(c,code='return lerp(float3(.15,.21,.045),float3(.28,.36,.10),V.r)*lerp(.75,1,V.g);',output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,inputs=[props(u.CustomInput(),input_name='V')])
connect(v,'',c,'V');lib.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR)
normal=lib.create_material_expression(grass,u.MaterialExpressionVertexNormalWS)
soft=lib.create_material_expression(grass,u.MaterialExpressionCustom)
# Authored canopy shading normal reduces black edge-on blade contrast.
props(soft,code='return normalize(float3(N.xy*.35,abs(N.z)*.35+.65));',output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,inputs=[props(u.CustomInput(),input_name='N')])
connect(normal,'',soft,'N');lib.connect_material_property(soft,'',u.MaterialProperty.MP_NORMAL)
sub=lib.create_material_expression(grass,u.MaterialExpressionConstant3Vector);props(sub,constant=u.LinearColor(.10,.16,.025,1));lib.connect_material_property(sub,'',u.MaterialProperty.MP_SUBSURFACE_COLOR)
rough=lib.create_material_expression(grass,u.MaterialExpressionConstant);props(rough,r=.86);lib.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
save(grass)

manifest=json.loads((ROOT/'blender/exports/Lawn/Manifest.json').read_text())
for level in manifest['assets']:assert hashlib.sha256((ROOT/level['file']).read_bytes()).hexdigest()==level['sha256']
flag='Interchange.FeatureFlags.Import.FBX';previous=u.SystemLibrary.get_console_variable_int_value(flag)
u.SystemLibrary.execute_console_command(None,flag+' 0')
try:
    options=props(u.FbxImportUI(),automated_import_should_detect_type=False,import_mesh=True,import_as_skeletal=False,import_materials=False,import_textures=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH)
    props(options.get_editor_property('static_mesh_import_data'),combine_meshes=True,auto_generate_collision=False,vertex_color_import_option=u.VertexColorImportOption.REPLACE,normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task=props(u.AssetImportTask(),filename=str(ROOT/manifest['assets'][0]['file']),destination_path=BASE+'/Vegetation/Meshes',destination_name='SM_LawnGrass',automated=True,replace_existing=True,replace_existing_settings=True,save=False,options=options)
    tools.import_asset_tasks([task]);mesh=u.load_asset(BASE+'/Vegetation/Meshes/SM_LawnGrass')
    if not mesh:raise RuntimeError('Grass mesh missing')
    for level in manifest['assets'][1:]:
        if editor.import_lod(mesh,level['lod'],str(ROOT/level['file']))!=level['lod']:raise RuntimeError('Grass LOD import failed')
    mesh.set_material(0,grass)
    nanite=editor.get_nanite_settings(mesh);props(nanite,enabled=False);editor.set_nanite_settings(mesh,nanite,True)
    if not editor.set_lod_screen_sizes(mesh,[1,.04,.01]):raise RuntimeError('Grass screen sizes failed')
    save(mesh)
    triangles=[mesh.get_num_triangles(i) for i in range(3)]
    assert triangles==[960,192,20],triangles
    bounds=mesh.get_bounds();height=bounds.box_extent.z*2
    assert abs(height-manifest['assets'][0]['max_height_m']*100)<.02,height
finally:u.SystemLibrary.execute_console_command(None,flag+' '+str(previous))
report=dict(status='PASS',mask=data['mask'],shader_sha256=hashlib.sha256(code.encode()).hexdigest(),triangles=triangles,height_cm=height,nanite=False,collision=False)
(ROOT/'artifacts/above-water-lawn-20261128/ContentBuild.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
u.log('DIVE_LAWN_CONTENT_PASS')
