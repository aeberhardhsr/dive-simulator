"""Build transparent air bubbles and GPU-animated suspended grains in Unreal."""
from pathlib import Path
import unreal as u


def build_particles():
    lib=u.MaterialEditingLibrary;tools=u.AssetToolsHelpers.get_asset_tools()
    mesh_path='/Game/Diving/Meshes/SM_SuspendedGrain'
    mesh=u.load_asset(mesh_path) if u.EditorAssetLibrary.does_asset_exist(mesh_path) else u.EditorAssetLibrary.duplicate_asset('/Engine/BasicShapes/Sphere',mesh_path)
    if not mesh:raise RuntimeError('Cannot create suspended grain mesh')
    # Regular ISM GPU culling uses mesh bounds, independently of the material's
    # WPO clamp. Keep all potential wrapped positions inside each instance box.
    mesh.set_editor_property('positive_bounds_extension',u.Vector(2200,2200,2200))
    mesh.set_editor_property('negative_bounds_extension',u.Vector(2200,2200,2200))
    if not u.EditorAssetLibrary.save_loaded_asset(mesh):raise RuntimeError('Cannot save grain mesh bounds')
    def wire(source,output,target,input_name):
        if not lib.connect_material_expressions(source,output,target,input_name):
            raise RuntimeError('Cannot connect '+source.get_class().get_name()+' to '+target.get_class().get_name()+'.'+input_name)
        return True
    def connect_property(source,output,property_name):
        if not lib.connect_material_property(source,output,property_name):raise RuntimeError('Cannot connect material property '+str(property_name))
        return True
    def material(name):
        path='/Game/Diving/Materials/'+name
        m=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(name,'/Game/Diving/Materials',u.Material,u.MaterialFactoryNew())
        for node in list(lib.get_material_expressions(m)):lib.delete_material_expression(m,node)
        m.set_editor_property('used_with_instanced_static_meshes',True)
        m.set_editor_property('two_sided',False)
        return m
    def scalar(m,value,prop):
        n=lib.create_material_expression(m,u.MaterialExpressionConstant);n.set_editor_property('r',value)
        connect_property(n,'',prop);return n
    def color(m,value,prop=None):
        n=lib.create_material_expression(m,u.MaterialExpressionConstant3Vector);n.set_editor_property('constant',u.LinearColor(*value,1))
        if prop is not None:connect_property(n,'',prop)
        return n
    bubble=material('M_AirBubble')
    bubble.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
    bubble.set_editor_property('shading_model',u.MaterialShadingModel.MSM_THIN_TRANSLUCENT)
    bubble.set_editor_property('translucency_lighting_mode',u.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    bubble.set_editor_property('translucency_pass',u.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    bubble.set_editor_property('refraction_method',u.RefractionMode.RM_INDEX_OF_REFRACTION)
    bubble.set_editor_property('screen_space_reflections',True)
    color(bubble,(0,0,0),u.MaterialProperty.MP_BASE_COLOR)
    scalar(bubble,.08,u.MaterialProperty.MP_ROUGHNESS)
    scalar(bubble,.255,u.MaterialProperty.MP_SPECULAR)  # F0 ~= .0204 at the water/air boundary.
    scalar(bubble,0,u.MaterialProperty.MP_OPACITY)  # No opaque diffuse layer on top of transmission.
    scalar(bubble,1/1.333,u.MaterialProperty.MP_REFRACTION)
    thin=lib.create_material_expression(bubble,u.MaterialExpressionThinTranslucentMaterialOutput)
    wire(color(bubble,(1,1,1)),'',thin,'TransmittanceColor')
    grain=material('M_SuspendedWater')
    grain.set_editor_property('blend_mode',u.BlendMode.BLEND_OPAQUE)
    grain.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
    grain.set_editor_property('max_world_position_offset_displacement',2200)
    color(grain,(.45,.52,.43),u.MaterialProperty.MP_BASE_COLOR)
    scalar(grain,1,u.MaterialProperty.MP_ROUGHNESS)
    fn=lib.create_material_expression(grain,u.MaterialExpressionCustom)
    names=['Position','Anchor','Viewer','S0','S1','S2','FieldTime','WaterTime','FlowCm','GrainScale','Wave0','Wave1','Wave2','Wave3','WavePhase']
    inputs=[]
    for name in names:
        item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
    fn.set_editor_property('inputs',inputs)
    fn.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    fn.set_editor_property('code',(Path(__file__).parent/'Shaders'/'SuspendedWater.ush').read_text())
    for name in names:
        if name=='Position':n=lib.create_material_expression(grain,u.MaterialExpressionWorldPosition)
        elif name in ['S0','S1','S2']:
            n=lib.create_material_expression(grain,u.MaterialExpressionPerInstanceCustomData);n.set_editor_property('data_index',int(name[1]))
        elif name in ['FieldTime','WaterTime','FlowCm','GrainScale']:
            n=lib.create_material_expression(grain,u.MaterialExpressionScalarParameter);n.set_editor_property('parameter_name',name)
            n.set_editor_property('default_value',1 if name=='GrainScale' else 0)
        else:
            n=lib.create_material_expression(grain,u.MaterialExpressionVectorParameter);n.set_editor_property('parameter_name',name)
            if name.startswith('Wave'):
                rgba=lib.create_material_expression(grain,u.MaterialExpressionAppendVector)
                wire(n,'RGB',rgba,'A');wire(n,'A',rgba,'B');n=rgba
        if name in ['Anchor','Viewer','FieldTime','WaterTime']:
            previous=lib.create_material_expression(grain,u.MaterialExpressionVectorParameter if name in ['Anchor','Viewer'] else u.MaterialExpressionScalarParameter)
            previous.set_editor_property('parameter_name','Previous'+name)
            switch=lib.create_material_expression(grain,u.MaterialExpressionPreviousFrameSwitch)
            wire(n,'',switch,'Current Frame')
            wire(previous,'',switch,'Previous Frame')
            n=switch
        if not wire(n,'',fn,name):raise RuntimeError('Missing particle input '+name)
    connect_property(fn,'',u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    # Magnified smoke diagnostics use a distinct emissive colour so pixel checks
    # can distinguish actual GPU-rendered grains from terrain/TAA variation.
    # Normal gameplay fixes GrainScale=1, where this contributes exactly zero.
    debug=lib.create_material_expression(grain,u.MaterialExpressionCustom)
    debug_input=u.CustomInput();debug_input.set_editor_property('input_name','Scale')
    debug.set_editor_property('inputs',[debug_input]);debug.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    debug.set_editor_property('code','return Scale>1.01?float3(5,0,5):float3(0,0,0);')
    debug_scale=lib.create_material_expression(grain,u.MaterialExpressionScalarParameter);debug_scale.set_editor_property('parameter_name','GrainScale');debug_scale.set_editor_property('default_value',1)
    wire(debug_scale,'',debug,'Scale');connect_property(debug,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    for m in [bubble,grain]:
        lib.recompile_material(m)
        if not u.EditorAssetLibrary.save_loaded_asset(m):raise RuntimeError('Failed to save '+m.get_name())
    u.log('DIVE_PARTICLE_CONTENT_OK thin_bubbles=1 gpu_grains=1')


if __name__=='__main__':build_particles()
