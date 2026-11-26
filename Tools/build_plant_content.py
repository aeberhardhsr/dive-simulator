"""Unreal: original aquatic geometry, leaf surfaces and bounded distance LODs."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
BASE='/Game/Diving'
lib=u.MaterialEditingLibrary
def props(obj,**values):
    for name,value in values.items():obj.set_editor_property(name,value)
    return obj
def save(obj):
    if not u.EditorAssetLibrary.save_loaded_asset(obj):raise RuntimeError('Plant asset save failed')

def build_plants():
    tools=u.AssetToolsHelpers.get_asset_tools()
    material=u.load_asset(BASE+'/Materials/M_AquaticPlant')
    if not material:material=tools.create_asset('M_AquaticPlant',BASE+'/Materials',u.Material,u.MaterialFactoryNew())
    for expression in list(lib.get_material_expressions(material)):lib.delete_material_expression(material,expression)
    props(material,two_sided=True,used_with_instanced_static_meshes=True,tangent_space_normal=True,
          blend_mode=u.BlendMode.BLEND_OPAQUE,shading_model=u.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    def node(cls,**values):return props(lib.create_material_expression(material,cls),**values)
    def wire(a,pin,b,target):
        if not lib.connect_material_expressions(a,pin,b,target):raise RuntimeError('Plant connection: '+target)
    def output(a,pin,target):
        if not lib.connect_material_property(a,pin,target):raise RuntimeError('Plant material output')
    vertex=node(u.MaterialExpressionVertexColor)
    uv=node(u.MaterialExpressionTextureCoordinate,coordinate_index=0)
    detail=node(u.MaterialExpressionTextureCoordinate,coordinate_index=1)
    code=(ROOT/'Tools/Shaders/AquaticPlant.ush').read_text()
    shader=node(u.MaterialExpressionCustom,code=code,output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
        inputs=[props(u.CustomInput(),input_name=n) for n in ['UV','Detail','C']],
        additional_outputs=[props(u.CustomOutput(),output_name=n,output_type=t) for n,t in [
            ('SurfaceNormal',u.CustomMaterialOutputType.CMOT_FLOAT3),('Roughness',u.CustomMaterialOutputType.CMOT_FLOAT1),
            ('Transmission',u.CustomMaterialOutputType.CMOT_FLOAT3),('Density',u.CustomMaterialOutputType.CMOT_FLOAT1)]])
    wire(uv,'',shader,'UV');wire(detail,'',shader,'Detail');wire(vertex,'',shader,'C')
    for pin,target in [('',u.MaterialProperty.MP_BASE_COLOR),('SurfaceNormal',u.MaterialProperty.MP_NORMAL),
        ('Roughness',u.MaterialProperty.MP_ROUGHNESS),('Transmission',u.MaterialProperty.MP_SUBSURFACE_COLOR),('Density',u.MaterialProperty.MP_OPACITY)]:output(shader,pin,target)
    spec=node(u.MaterialExpressionConstant,r=.25);output(spec,'',u.MaterialProperty.MP_SPECULAR)
    motion=node(u.MaterialExpressionCustom,output_type=u.CustomMaterialOutputType.CMOT_FLOAT3,
        inputs=[props(u.CustomInput(),input_name=n) for n in ['P','T','W','UV','Detail']],
        code='float anchor=W*W; float flutter=sin(T*2.2+Detail.y*9)*(1-UV.y)*Detail.x*.22; return float3(sin(T*1.1+P.x*.009)*9*anchor,sin(T*.8+P.y*.008)*6*anchor,flutter*anchor);')
    wire(node(u.MaterialExpressionWorldPosition),'',motion,'P');wire(node(u.MaterialExpressionTime),'',motion,'T')
    wire(vertex,'A',motion,'W');wire(uv,'',motion,'UV');wire(detail,'',motion,'Detail')
    output(motion,'',u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    lib.recompile_material(material)
    u.EditorAssetLibrary.set_metadata_tag(material,'DivePlantShaderSHA256',hashlib.sha256(code.encode()).hexdigest());save(material)
    manifest=json.loads((ROOT/'blender/exports/Plants/Manifest.json').read_text())
    editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)
    flag='Interchange.FeatureFlags.Import.FBX';previous=u.SystemLibrary.get_console_variable_int_value(flag)
    u.SystemLibrary.execute_console_command(None,flag+' 0');records=[]
    try:
        for item in manifest['assets']:
            path=ROOT/item['file']
            if hashlib.sha256(path.read_bytes()).hexdigest()!=item['sha256']:raise RuntimeError('Plant geometry hash mismatch')
            existing=u.load_asset(BASE+'/Plants/'+item['name'])
            if existing:props(existing.get_editor_property('asset_import_data'),vertex_color_import_option=u.VertexColorImportOption.REPLACE)
            options=props(u.FbxImportUI(),import_mesh=True,import_as_skeletal=False,import_materials=False,import_textures=False,
                automated_import_should_detect_type=False,mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH)
            props(options.get_editor_property('static_mesh_import_data'),combine_meshes=True,auto_generate_collision=False,
                generate_lightmap_u_vs=False,vertex_color_import_option=u.VertexColorImportOption.REPLACE,
                normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
            task=props(u.AssetImportTask(),filename=str(path),destination_path=BASE+'/Plants',destination_name=item['name'],
                automated=True,replace_existing=True,replace_existing_settings=True,save=False,options=options)
            tools.import_asset_tasks([task]);mesh=u.load_asset(BASE+'/Plants/'+item['name'])
            if not mesh:raise RuntimeError('Missing aquatic plant mesh')
            mesh.set_material(0,material)
            reduction=props(u.StaticMeshReductionOptions(),auto_compute_lod_screen_size=False,reduction_settings=[
                props(u.StaticMeshReductionSettings(),percent_triangles=p,screen_size=s) for p,s in [(1.,1.),(.45,.14),(.16,.05),(.055,.018)]])
            if editor.set_lods(mesh,reduction)!=4:raise RuntimeError('Missing aquatic plant LODs')
            if not editor.set_lod_screen_sizes(mesh,[1.,.14,.05,.018]):raise RuntimeError('Plant LOD thresholds failed')
            props(mesh,positive_bounds_extension=u.Vector(12,12,2),negative_bounds_extension=u.Vector(12,12,2))
            triangles=[mesh.get_num_triangles(i) for i in range(4)]
            # This editor API inspects source MeshDescriptions; auto-reduced
            # LODs have none. Validate their actual vertex buffers in native code.
            channels=editor.get_num_uv_channels(mesh,0)
            if triangles[0]!=item['triangles'] or channels<2:raise RuntimeError(f"Plant geometry or source coordinates lost: {item['name']} expected={item['triangles']} actual={triangles} uv={channels}")
            if not all(a>b>0 for a,b in zip(triangles,triangles[1:])):raise RuntimeError('Invalid plant LOD reduction')
            if mesh.get_editor_property('asset_import_data').get_editor_property('vertex_color_import_option')!=u.VertexColorImportOption.REPLACE:raise RuntimeError('Root animation weights were not imported')
            save(mesh);records.append(dict(name=item['name'],triangles=triangles,source_uv_channels=channels,source_sha256=item['sha256']))
    finally:u.SystemLibrary.execute_console_command(None,flag+' '+str(previous))
    (ROOT/'artifacts/PlantContentBuild.json').write_text(json.dumps(records,indent=2)+'\n')
    u.log('DIVE_PLANT_CONTENT_PASS groups=3 lods=4 uv=2')

if __name__=='__main__':build_plants()
