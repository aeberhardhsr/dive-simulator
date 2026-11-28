"""Import original fish atlases/meshes, shared animation and four distance LODs."""
from pathlib import Path
import hashlib
import json
import unreal as u
ROOT=Path(__file__).resolve().parents[1];BASE='/Game/Diving'
tools=u.AssetToolsHelpers.get_asset_tools();lib=u.MaterialEditingLibrary
def props(obj,**values):
    for key,value in values.items():obj.set_editor_property(key,value)
    return obj
def save(obj):
    if not u.EditorAssetLibrary.save_loaded_asset(obj):raise RuntimeError('Fish save failed '+obj.get_name())
def asset(name,folder,cls,factory):
    path=BASE+'/'+folder+'/'+name
    return u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(name,BASE+'/'+folder,cls,factory())
def build_fish():
    manifest=json.loads((ROOT/'Documentation/FISH_TEXTURES.json').read_text(encoding='utf-8'));textures={}
    for entry in manifest['assets']:
        textures[entry['name']]={}
        for kind,info in entry['maps'].items():
            source=ROOT/info['file'];name='T_'+entry['name']+'_'+kind
            if hashlib.sha256(source.read_bytes()).hexdigest()!=info['sha256']:raise RuntimeError('Fish source hash mismatch')
            task=props(u.AssetImportTask(),filename=str(source),destination_path=BASE+'/Textures/Fish',destination_name=name,automated=True,replace_existing=True,save=False)
            tools.import_asset_tasks([task]);texture=u.load_asset(BASE+'/Textures/Fish/'+name)
            if not texture:raise RuntimeError('Fish texture import failed '+name)
            props(texture,srgb=kind=='Color',compression_settings=u.TextureCompressionSettings.TC_NORMALMAP if kind=='Normal' else u.TextureCompressionSettings.TC_DEFAULT if kind=='Color' else u.TextureCompressionSettings.TC_MASKS,
                flip_green_channel=False,address_x=u.TextureAddress.TA_CLAMP,address_y=u.TextureAddress.TA_CLAMP,never_stream=False,max_texture_size=2048)
            save(texture);textures[entry['name']][kind]=texture
    material=asset('M_Fish','Materials',u.Material,u.MaterialFactoryNew)
    for node in list(lib.get_material_expressions(material)):lib.delete_material_expression(material,node)
    props(material,two_sided=True,used_with_instanced_static_meshes=True,blend_mode=u.BlendMode.BLEND_OPAQUE,shading_model=u.MaterialShadingModel.MSM_SUBSURFACE,tangent_space_normal=True)
    def node(cls):return lib.create_material_expression(material,cls)
    def wire(a,pin,b,target):
        if not lib.connect_material_expressions(a,pin,b,target):raise RuntimeError('Fish wire '+target)
    def output(a,pin,prop):
        if not lib.connect_material_property(a,pin,prop):raise RuntimeError('Fish output '+str(prop))
    samples={}
    for kind in ['Color','Normal','Surface']:
        samples[kind]=props(node(u.MaterialExpressionTextureSampleParameter2D),parameter_name='Fish'+kind,texture=textures['Perch'][kind],
            sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='Normal' else u.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='Color' else u.MaterialSamplerType.SAMPLERTYPE_MASKS)
    vertex=node(u.MaterialExpressionVertexColor);mul=node(u.MaterialExpressionMultiply)
    wire(vertex,'',mul,'A');wire(samples['Color'],'RGB',mul,'B');output(mul,'',u.MaterialProperty.MP_BASE_COLOR)
    output(samples['Normal'],'RGB',u.MaterialProperty.MP_NORMAL);output(samples['Surface'],'G',u.MaterialProperty.MP_ROUGHNESS)
    output(samples['Surface'],'B',u.MaterialProperty.MP_OPACITY);output(mul,'',u.MaterialProperty.MP_SUBSURFACE_COLOR)
    spec=props(node(u.MaterialExpressionConstant),r=.45);output(spec,'',u.MaterialProperty.MP_SPECULAR)
    names=['Weight','Cycle','RX','RY','Phase','Amplitude','RZ','UX','UY','UZ','FinCycle','Effort','Turn','Length','Offset','Rig']
    bend=props(node(u.MaterialExpressionCustom),inputs=[props(u.CustomInput(),input_name=name) for name in names],output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
        code=(ROOT/'Tools/fish_motion_shader.hlsl').read_text(encoding='utf-8'))
    wire(vertex,'A',bend,'Weight')
    for index,name in enumerate(names[1:14]):wire(props(node(u.MaterialExpressionPerInstanceCustomData),data_index=index),'',bend,name)
    for index,name in [(1,'Offset'),(2,'Rig')]:wire(props(node(u.MaterialExpressionTextureCoordinate),coordinate_index=index),'',bend,name)
    output(bend,'',u.MaterialProperty.MP_WORLD_POSITION_OFFSET);lib.recompile_material(material);save(material)
    flag='Interchange.FeatureFlags.Import.FBX';previous=u.SystemLibrary.get_console_variable_int_value(flag)
    u.SystemLibrary.execute_console_command(None,flag+' 0')
    editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem);records=[]
    try:
        for entry in manifest['assets']:
            existing=u.load_asset(BASE+'/Meshes/SM_'+entry['name'])
            if existing:
                # FBX reimport uses the asset's stored policy even when the
                # task supplies REPLACE. An old IGNORE policy drops tint AND
                # the alpha weights that anchor the head during swimming.
                props(existing.get_editor_property('asset_import_data'),vertex_color_import_option=u.VertexColorImportOption.REPLACE)
            name=entry['name'];options=props(u.FbxImportUI(),import_mesh=True,import_as_skeletal=False,import_materials=False,import_textures=False,automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH)
            props(options.get_editor_property('static_mesh_import_data'),combine_meshes=True,auto_generate_collision=False,vertex_color_import_option=u.VertexColorImportOption.REPLACE)
            task=props(u.AssetImportTask(),filename=str(ROOT/'blender/exports'/('SM_'+name+'.fbx')),destination_path=BASE+'/Meshes',destination_name='SM_'+name,automated=True,replace_existing=True,replace_existing_settings=True,save=False,options=options)
            tools.import_asset_tasks([task]);mesh=u.load_asset(BASE+'/Meshes/SM_'+name)
            if not mesh:raise RuntimeError('Fish mesh import failed '+name)
            if mesh.get_editor_property('asset_import_data').get_editor_property('vertex_color_import_option')!=u.VertexColorImportOption.REPLACE:raise RuntimeError('Fish reimport ignored vertex data '+name)
            instance=asset('MI_Fish_'+name,'Materials',u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew)
            lib.set_material_instance_parent(instance,material)
            for kind,texture in textures[name].items():lib.set_material_instance_texture_parameter_value(instance,'Fish'+kind,texture)
            lib.update_material_instance(instance);save(instance);mesh.set_material(0,instance)
            settings=[props(u.StaticMeshReductionSettings(),percent_triangles=p,screen_size=s) for p,s in [(1.,1.),(.45,.35),(.18,.12),(.065,.04)]]
            reduction=props(u.StaticMeshReductionOptions(),auto_compute_lod_screen_size=False,reduction_settings=settings)
            if editor.set_lods(mesh,reduction)!=4:raise RuntimeError('Fish LOD creation failed '+name)
            props(mesh,positive_bounds_extension=u.Vector(12,12,8),negative_bounds_extension=u.Vector(12,12,8));save(mesh)
            records.append(dict(name=name,lods=editor.get_lod_count(mesh),material=instance.get_path_name(),uv_channels=editor.get_num_uv_channels(mesh,0)))
    finally:u.SystemLibrary.execute_console_command(None,flag+' '+str(previous))
    (ROOT/'artifacts/FishContentBuild.json').write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')
    u.log('DIVE_FISH_CONTENT_PASS species=5 lods=4 textures=15')
if __name__=='__main__':build_fish()
