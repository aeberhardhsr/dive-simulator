"""Run using UnrealEditor-Cmd -run=pythonscript -script=<absolute path>."""
import unreal as u
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
ASSETS=ROOT/'blender'/'exports'
TOOLS=u.AssetToolsHelpers.get_asset_tools()
LIB=u.MaterialEditingLibrary
BASE='/Game/Diving'
ONLY_ENVIRONMENT='-DiveEnvironmentOnly' in u.SystemLibrary.get_command_line()

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
        m.set_editor_property('tangent_space_normal',False)
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
        names=['P','T','C','V']+(['Wave0','Wave1','Wave2','Wave3','WavePhase','Viewer','WindStrength'] if water else ['Cycle','RX','RY','Phase','Amplitude'] if name=='M_Fish' else ['DrySuit'] if name=='M_Suit' else [])
        for input_name in names:
            item=u.CustomInput(); item.set_editor_property('input_name',input_name); inputs.append(item)
        exp.set_editor_property('inputs',inputs)
        position=node(u.MaterialExpressionWorldPosition,-400,0)
        time=node(u.MaterialExpressionScalarParameter if water else u.MaterialExpressionTime,-400,200)
        if water: time.set_editor_property('parameter_name','WaveTime')
        vc=node(u.MaterialExpressionVertexColor,-400,400)
        LIB.connect_material_expressions(position,'',exp,'P')
        LIB.connect_material_expressions(time,'',exp,'T')
        LIB.connect_material_expressions(col,'',exp,'C')
        LIB.connect_material_expressions(vc,'A',exp,'V')
        if water:
            for input_name in names[4:]:
                if input_name=='WindStrength':
                    data=node(u.MaterialExpressionScalarParameter)
                    data.set_editor_property('parameter_name',input_name)
                    data.set_editor_property('default_value',.25)
                    LIB.connect_material_expressions(data,'',exp,input_name)
                else:
                    data=node(u.MaterialExpressionVectorParameter)
                    data.set_editor_property('parameter_name',input_name)
                    rgba=node(u.MaterialExpressionAppendVector)
                    LIB.connect_material_expressions(data,'RGB',rgba,'A')
                    LIB.connect_material_expressions(data,'A',rgba,'B')
                    LIB.connect_material_expressions(rgba,'',exp,input_name)
        if name=='M_Fish':
            for index,input_name in enumerate(names[4:]):
                data=node(u.MaterialExpressionPerInstanceCustomData,-650,index*120)
                data.set_editor_property('data_index',index)
                LIB.connect_material_expressions(data,'',exp,input_name)
        if name=='M_Suit':
            dry=node(u.MaterialExpressionScalarParameter)
            dry.set_editor_property('parameter_name','DrySuit'); dry.set_editor_property('default_value',1)
            LIB.connect_material_expressions(dry,'',exp,'DrySuit')
        LIB.connect_material_property(exp,'',prop)
    target=u.MaterialProperty.MP_EMISSIVE_COLOR if unlit else u.MaterialProperty.MP_BASE_COLOR
    if custom_color: custom(custom_color,u.CustomMaterialOutputType.CMOT_FLOAT3,target)
    else: LIB.connect_material_property(col,'',target)
    if name=='M_Rain':
        # Keep fine streaks legible across the manual day/night exposure range.
        exposure=node(u.MaterialExpressionEyeAdaptation)
        divide=node(u.MaterialExpressionDivide)
        LIB.connect_material_expressions(col,'',divide,'A')
        LIB.connect_material_expressions(exposure,'',divide,'B')
        LIB.connect_material_property(divide,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    scalar(roughness,u.MaterialProperty.MP_ROUGHNESS)
    scalar(.5 if water else .22,u.MaterialProperty.MP_SPECULAR)
    if water:
        scalar(.05,u.MaterialProperty.MP_OPACITY)
        volume=node(u.MaterialExpressionSingleLayerWaterMaterialOutput,600,300)
        for pin,value in [('ScatteringCoefficients',(.00015,.00045,.00032)),
                          ('AbsorptionCoefficients',(.0016,.00035,.00055))]:
            coefficient=node(u.MaterialExpressionConstant3Vector)
            coefficient.set_editor_property('constant',u.LinearColor(*value,1))
            optical=node(u.MaterialExpressionScalarParameter)
            optical.set_editor_property('parameter_name','OpticalScale'); optical.set_editor_property('default_value',1)
            product=node(u.MaterialExpressionMultiply)
            LIB.connect_material_expressions(coefficient,'',product,'A')
            LIB.connect_material_expressions(optical,'',product,'B')
            if not LIB.connect_material_expressions(product,'',volume,pin):
                raise RuntimeError('Missing Single Layer Water input '+pin)
        phase=node(u.MaterialExpressionConstant); phase.set_editor_property('r',.35)
        LIB.connect_material_expressions(phase,'',volume,'PhaseG')
    if translucent:
        scalar(opacity,u.MaterialProperty.MP_OPACITY)
        if name=='M_Water': scalar(1.333,u.MaterialProperty.MP_REFRACTION)
    if custom_wpo: custom(custom_wpo,u.CustomMaterialOutputType.CMOT_FLOAT3,u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    if custom_normal: custom(custom_normal,u.CustomMaterialOutputType.CMOT_FLOAT3,u.MaterialProperty.MP_NORMAL)
    LIB.recompile_material(m)
    if not u.EditorAssetLibrary.save_loaded_asset(m):
        raise RuntimeError('Material save failed: '+name)
    return m

if not ONLY_ENVIRONMENT:
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
             custom_wpo='float sway=sin(Cycle+Phase-V*3)*Amplitude*V; return float3(RX*sway,RY*sway,0);')
    material('M_Landscape',(.2,.24,.12),.95,vertex=True)
    material('M_Equipment',(.04,.05,.05),.82,vertex=True,custom_color='return C.rgb*.35;')
    material('M_Suit',(.07,.11,.13),.9,vertex=True,custom_color='return C.rgb*lerp(float3(.12,.13,.13),float3(.5,.5,.5),DrySuit);')
wave_code='''
float2 xy=P.xy*.01;
float4 waves[4]={Wave0,Wave1,Wave2,Wave3};
float h=0; float2 slope=0;
for(int i=0;i<4;++i) {
    float phase=dot(waves[i].xy,xy)-waves[i].z*T+WavePhase[i];
    h+=waves[i].w*sin(phase);
    slope+=waves[i].xy*waves[i].w*cos(phase);
}
float2 delta=xy-Viewer.xy;
float d=length(delta);
float f=saturate((d-32)/16);
float fade=1-f*f*(3-2*f);
float2 fadeSlope=-6*f*(1-f)/16*delta/max(.001,d);
'''
material('M_Water',(.012,.035,.028),.13,water=True,
         custom_wpo=wave_code+'return float3(0,0,h*100*fade);',
         custom_normal=wave_code+'''
float2 micro=float2(sin(P.x*.13+P.y*.025+T*1.7)*.025+cos(P.y*.057-T*2.1)*.015,
    cos(P.y*.11+P.x*.018+T*1.5)*.02+sin(P.x*.19-T*2.9)*.01)*WindStrength;
return normalize(float3(-slope*fade-h*fadeSlope+micro,1));''')
cloud_path=BASE+'/Materials/MI_Cloud'
if not u.EditorAssetLibrary.does_asset_exist(cloud_path):
    source=u.load_asset('/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst')
    if not source or not TOOLS.duplicate_asset('MI_Cloud',BASE+'/Materials',source):
        raise RuntimeError('Cloud material copy failed')
u.EditorAssetLibrary.save_asset(cloud_path)
material('M_Rain',(.12,.16,.18),.2,translucent=True,opacity=.35,unlit=True)
if not ONLY_ENVIRONMENT:
    material('M_Bubble',(.4,.7,.65),.06,translucent=True,opacity=.21)
    material('M_Particle',(.18,.32,.26),1,unlit=True)
    material('M_Buoy',(.9,.22,.045),.35)
    material('M_Rope',(.65,.62,.42),.9)

    # A real world-space computer display; compensate exposure so it remains readable.
    screen=material('M_ComputerScreen',(.02,.04,.03),unlit=True,translucent=True,opacity=1)
    clear_expressions(screen)
    # Numeric pixels have no useful temporal history. Draw after TSR, while explicitly
    # restoring depth occlusion (this translucency pass disables the hardware depth test).
    screen.set_editor_property('translucency_pass',u.MaterialTranslucencyPass.MTP_AFTER_MOTION_BLUR)
    scene_depth=LIB.create_material_expression(screen,u.MaterialExpressionSceneDepth)
    pixel_depth=LIB.create_material_expression(screen,u.MaterialExpressionPixelDepth)
    depth_mask=LIB.create_material_expression(screen,u.MaterialExpressionCustom)
    depth_mask.set_editor_property('code','return step(Surface-0.1,Scene);')
    depth_mask.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
    depth_inputs=[]
    for input_name in ['Scene','Surface']:
        item=u.CustomInput(); item.set_editor_property('input_name',input_name); depth_inputs.append(item)
    depth_mask.set_editor_property('inputs',depth_inputs)
    LIB.connect_material_expressions(scene_depth,'',depth_mask,'Scene')
    LIB.connect_material_expressions(pixel_depth,'',depth_mask,'Surface')
    LIB.connect_material_property(depth_mask,'',u.MaterialProperty.MP_OPACITY)
    sample=LIB.create_material_expression(screen,u.MaterialExpressionTextureSampleParameter2D)
    sample.set_editor_property('parameter_name','DisplayTexture')
    sample.set_editor_property('texture',u.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
    exposure=LIB.create_material_expression(screen,u.MaterialExpressionEyeAdaptation)
    divide=LIB.create_material_expression(screen,u.MaterialExpressionDivide)
    LIB.connect_material_expressions(sample,'RGB',divide,'A')
    LIB.connect_material_expressions(exposure,'',divide,'B')
    LIB.connect_material_property(divide,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    LIB.recompile_material(screen); u.EditorAssetLibrary.save_loaded_asset(screen)

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
    TOOLS.import_asset_tasks(tasks)
    import sys
    sys.path.insert(0,str(ROOT/'Tools'))
    from build_audio_content import import_audio
    import_audio()
    from build_shore_content import build_shore_material
    build_shore_material()
    from build_imagery_content import build_imagery
    build_imagery()
    fish_meshes=['SM_Perch','SM_Roach','SM_Pike','SM_Whitefish','SM_Burbot']
    plant_meshes=['SM_Pondweed','SM_Stonewort','SM_Milfoil']
    equipment_meshes=['SM_LeftSleeve','SM_RightSleeve','SM_LeftGlove','SM_RightGlove','SM_WristComputer','SM_DiveTorch']
    for name,mat_name in [('SM_Rock','M_Rock')]+[(n,'M_Fish') for n in fish_meshes]+[(n,'M_Plant') for n in plant_meshes]+[(n,'M_Suit' if 'Sleeve' in n else 'M_Equipment') for n in equipment_meshes]:
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
    for path in [BASE+'/Meshes/'+n for n in fish_meshes+plant_meshes+equipment_meshes]+[BASE+'/Maps/L_DiveTraining']:
        if not u.EditorAssetLibrary.does_asset_exist(path): raise RuntimeError('Missing output '+path)
u.log('DIVE_CONTENT_OK')
