"""Run using UnrealEditor-Cmd -run=pythonscript -script=<absolute path>."""
import unreal as u
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
ASSETS=ROOT/'blender'/'exports'
TOOLS=u.AssetToolsHelpers.get_asset_tools()
LIB=u.MaterialEditingLibrary
BASE='/Game/Diving'

def clear_expressions(material):
    # UE 5.8 DeleteAllMaterialExpressions mutates the collection it iterates,
    # leaving nodes behind. Iterate a snapshot to make regeneration idempotent.
    for expression in list(LIB.get_material_expressions(material)):
        LIB.delete_material_expression(material,expression)
    if LIB.get_num_material_expressions(material)!=0:
        raise RuntimeError('Material graph could not be cleared: '+material.get_name())

def material(name, color, roughness=.7, vertex=False, custom_color=None, custom_wpo=None,
             translucent=False, opacity=.4, unlit=False, custom_normal=None, water=False):
    path=BASE+'/Materials'
    full=path+'/'+name
    existing=u.load_asset(full) if u.EditorAssetLibrary.does_asset_exist(full) else None
    m=existing or TOOLS.create_asset(name,path,u.Material,u.MaterialFactoryNew())
    if existing: clear_expressions(m)
    m.set_editor_property('two_sided',True)
    m.set_editor_property('used_with_instanced_static_meshes',True)
    m.set_editor_property('blend_mode',u.BlendMode.BLEND_OPAQUE)
    m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
    if water:
        m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    if translucent:
        m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
    if unlit:
        m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    def node(cls,x=0,y=0): return LIB.create_material_expression(m,cls,x,y)
    def scalar(value,prop):
        n=node(u.MaterialExpressionConstant); n.set_editor_property('r',value)
        LIB.connect_material_property(n,'',prop)
    col=node(u.MaterialExpressionVertexColor) if vertex else node(u.MaterialExpressionConstant3Vector)
    if not vertex: col.set_editor_property('constant',u.LinearColor(*color,1))
    def custom(code,kind,prop):
        exp=node(u.MaterialExpressionCustom,400,0)
        exp.set_editor_property('code',code)
        exp.set_editor_property('output_type',kind)
        inputs=[]
        for name in ['P','T','C','V']:
            item=u.CustomInput(); item.set_editor_property('input_name',name); inputs.append(item)
        exp.set_editor_property('inputs',inputs)
        position=node(u.MaterialExpressionWorldPosition,-400,0)
        time=node(u.MaterialExpressionTime,-400,200)
        vc=node(u.MaterialExpressionVertexColor,-400,400)
        LIB.connect_material_expressions(position,'',exp,'P')
        LIB.connect_material_expressions(time,'',exp,'T')
        LIB.connect_material_expressions(col,'',exp,'C')
        LIB.connect_material_expressions(vc,'A',exp,'V')
        LIB.connect_material_property(exp,'',prop)
    target=u.MaterialProperty.MP_EMISSIVE_COLOR if unlit else u.MaterialProperty.MP_BASE_COLOR
    if custom_color: custom(custom_color,u.CustomMaterialOutputType.CMOT_FLOAT3,target)
    else: LIB.connect_material_property(col,'',target)
    scalar(roughness,u.MaterialProperty.MP_ROUGHNESS)
    scalar(.5 if water else .22,u.MaterialProperty.MP_SPECULAR)
    if water:
        scalar(.05,u.MaterialProperty.MP_OPACITY)
        volume=node(u.MaterialExpressionSingleLayerWaterMaterialOutput,600,300)
        for pin,value in [('ScatteringCoefficients',(.00015,.00045,.00032)),
                          ('AbsorptionCoefficients',(.0016,.00035,.00055))]:
            coefficient=node(u.MaterialExpressionConstant3Vector)
            coefficient.set_editor_property('constant',u.LinearColor(*value,1))
            if not LIB.connect_material_expressions(coefficient,'',volume,pin):
                raise RuntimeError('Missing Single Layer Water input '+pin)
        phase=node(u.MaterialExpressionConstant); phase.set_editor_property('r',.35)
        LIB.connect_material_expressions(phase,'',volume,'PhaseG')
    if translucent:
        scalar(opacity,u.MaterialProperty.MP_OPACITY)
        if name=='M_Water': scalar(1.333,u.MaterialProperty.MP_REFRACTION)
    if custom_wpo: custom(custom_wpo,u.CustomMaterialOutputType.CMOT_FLOAT3,u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    if custom_normal: custom(custom_normal,u.CustomMaterialOutputType.CMOT_FLOAT3,u.MaterialProperty.MP_NORMAL)
    LIB.recompile_material(m)
    u.EditorAssetLibrary.save_loaded_asset(m)
    return m

stone='''
float grain = frac(sin(dot(floor(P*1.4),float3(12.9898,78.233,37.719)))*43758.5453);
float strata = sin(P.z*.021 + sin(P.x*.004)*2 + sin(P.y*.008));
float patches = sin(P.x*.008)*sin(P.y*.011)*sin(P.z*.013);
float3 base = C.rgb*(.72+grain*.18+strata*.025+patches*.13);
float caustic = pow(1-abs(sin(P.x*.037+sin(P.y*.03+T*.35))*sin(P.y*.033+sin(P.x*.02-T*.4))),24);
return base + float3(.025,.04,.028)*caustic*saturate(1+P.z/1400);
'''
material('M_Rock',(.31,.32,.27),.87,custom_color=stone)
material('M_Lakebed',(.23,.26,.18),.94,custom_color=stone)
material('M_Wood',(.12,.08,.035),.95,custom_color=stone)
material('M_Plant',(.1,.3,.06),.7,vertex=True,
         custom_wpo='return float3(sin(T*1.1+P.x*.009)*9*V*V, sin(T*.8+P.y*.008)*6*V*V,0);')
material('M_Fish',(.4,.45,.3),.32,vertex=True,
         custom_wpo='return float3(0,sin(T*8+P.x*.055)*2.6*V, sin(T*4+P.y*.02)*.2*V);')
material('M_Landscape',(.2,.24,.12),.95,vertex=True)
material('M_Water',(.012,.035,.028),.13,water=True,
         custom_normal='return normalize(float3(sin(P.x*.13+P.y*.025+T*1.7)*.045+cos(P.y*.057-T*2.1)*.025, cos(P.y*.11+P.x*.018+T*1.5)*.035+sin(P.x*.19-T*2.9)*.015,1));')
material('M_Bubble',(.4,.7,.65),.06,translucent=True,opacity=.21)
material('M_Particle',(.18,.32,.26),1,unlit=True)
material('M_Buoy',(.9,.22,.045),.35)
material('M_Rope',(.65,.62,.42),.9)

# Screen-distance water absorption, distinct from the atmospheric height fog.
post_path=BASE+'/Materials/M_Underwater'
post=u.load_asset(post_path) if u.EditorAssetLibrary.does_asset_exist(post_path) else TOOLS.create_asset('M_Underwater',BASE+'/Materials',u.Material,u.MaterialFactoryNew())
clear_expressions(post)
post.set_editor_property('material_domain',u.MaterialDomain.MD_POST_PROCESS)
scene=LIB.create_material_expression(post,u.MaterialExpressionSceneTexture,-500,0)
scene.set_editor_property('scene_texture_id',u.SceneTextureId.PPI_POST_PROCESS_INPUT0)
depth=LIB.create_material_expression(post,u.MaterialExpressionSceneDepth,-500,200)
visibility=LIB.create_material_expression(post,u.MaterialExpressionScalarParameter,-500,400)
visibility.set_editor_property('parameter_name','VisibilityCm')
visibility.set_editor_property('default_value',1400)
fog=LIB.create_material_expression(post,u.MaterialExpressionCustom,0,0)
inputs=[]
for name in ['Scene','Depth','Visibility']:
    item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
fog.set_editor_property('inputs',inputs)
fog.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
fog.set_editor_property('code','''
float d=max(0,Depth);
float transmission=exp(-d/max(150,Visibility)*1.45);
float3 attenuation=exp(-d*float3(.00045,.00009,.00012));
return Scene.rgb*attenuation*transmission + float3(.025,.105,.09)*(1-transmission);
''')
LIB.connect_material_expressions(scene,'Color',fog,'Scene')
LIB.connect_material_expressions(depth,'',fog,'Depth')
LIB.connect_material_expressions(visibility,'',fog,'Visibility')
LIB.connect_material_property(fog,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
LIB.recompile_material(post)
u.EditorAssetLibrary.save_loaded_asset(post)

tasks=[]
for source in ASSETS.glob('*.fbx'):
    task=u.AssetImportTask()
    task.set_editor_property('filename',str(source))
    task.set_editor_property('destination_path',BASE+'/Meshes')
    task.set_editor_property('destination_name',source.stem)
    task.set_editor_property('automated',True)
    task.set_editor_property('save',True)
    task.set_editor_property('replace_existing',True)
    options=u.FbxImportUI()
    options.set_editor_property('import_mesh',True)
    options.set_editor_property('import_as_skeletal',False)
    options.set_editor_property('import_materials',False)
    options.set_editor_property('import_textures',False)
    options.set_editor_property('automated_import_should_detect_type',False)
    options.set_editor_property('mesh_type_to_import',u.FBXImportType.FBXIT_STATIC_MESH)
    data=options.get_editor_property('static_mesh_import_data')
    data.set_editor_property('combine_meshes',True)
    data.set_editor_property('auto_generate_collision',True)
    data.set_editor_property('vertex_color_import_option',u.VertexColorImportOption.REPLACE)
    task.set_editor_property('options',options)
    tasks.append(task)
for source in ASSETS.glob('*.wav'):
    task=u.AssetImportTask()
    task.set_editor_property('filename',str(source))
    task.set_editor_property('destination_path',BASE+'/Audio')
    task.set_editor_property('automated',True)
    task.set_editor_property('save',True)
    task.set_editor_property('replace_existing',True)
    tasks.append(task)
TOOLS.import_asset_tasks(tasks)
ambient=u.load_asset(BASE+'/Audio/A_Ambience')
if ambient:
    ambient.set_editor_property('looping',True)
    u.EditorAssetLibrary.save_loaded_asset(ambient)
for name,mat_name in [('SM_Rock','M_Rock'),('SM_Perch','M_Fish'),('SM_Pondweed','M_Plant')]:
    mesh=u.load_asset(BASE+'/Meshes/'+name)
    if not mesh: raise RuntimeError('Missing imported mesh '+name)
    mesh.set_material(0,u.load_asset(BASE+'/Materials/'+mat_name))
    u.EditorAssetLibrary.save_loaded_asset(mesh)

map_path=BASE+'/Maps/L_DiveTraining'
if not u.EditorAssetLibrary.does_asset_exist(map_path):
    world=u.EditorLoadingAndSavingUtils.new_blank_map(False)
    mode=u.load_class(None,'/Script/DiveSim.DiveGameMode')
    if not mode: raise RuntimeError('DiveSim game module missing')
    world.get_world_settings().set_editor_property('default_game_mode',mode)
    # World geometry is generated by ADiveWorld when the game starts.
    start=u.get_editor_subsystem(u.EditorActorSubsystem).spawn_actor_from_class(u.PlayerStart,u.Vector(-6500,-1800,-180))
    start.set_actor_label('Diver Entry')
    if not u.EditorLoadingAndSavingUtils.save_map(world,map_path): raise RuntimeError('Map save failed')
for path in [BASE+'/Meshes/SM_Perch',BASE+'/Meshes/SM_Pondweed',BASE+'/Maps/L_DiveTraining']:
    if not u.EditorAssetLibrary.does_asset_exist(path): raise RuntimeError('Missing output '+path)
u.log('DIVE_CONTENT_OK')
