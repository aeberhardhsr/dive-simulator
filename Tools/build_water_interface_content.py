"""Build the water/air interface seen from below; shared waves with the top."""
from pathlib import Path
import unreal as u


def build_underwater_interface(wave_code):
    lib=u.MaterialEditingLibrary
    path='/Game/Diving/Materials/M_WaterUnder'
    m=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_WaterUnder','/Game/Diving/Materials',u.Material,u.MaterialFactoryNew())
    for node in list(lib.get_material_expressions(m)):lib.delete_material_expression(m,node)
    m.set_editor_property('two_sided',False)
    m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
    m.set_editor_property('tangent_space_normal',False)
    m.set_editor_property('translucency_lighting_mode',u.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    m.set_editor_property('translucency_pass',u.MaterialTranslucencyPass.MTP_BEFORE_DOF)
    # An infinite plane cannot use a bulk screen-space offset without exposing
    # unavailable pixels at the viewport edges. Distort only the wave normals.
    # Fresnel below still uses the physical water-to-air index ratio.
    m.set_editor_property('refraction_method',u.RefractionMode.RM_PIXEL_NORMAL_OFFSET)
    m.set_editor_property('screen_space_reflections',True)
    m.set_editor_property('allow_front_layer_translucency',True)
    def scalar(value,prop):
        n=lib.create_material_expression(m,u.MaterialExpressionConstant);n.set_editor_property('r',value)
        if not lib.connect_material_property(n,'',prop):raise RuntimeError(str(prop))
    # Unit reflectance BRDF, with the dielectric Fresnel weight in opacity.
    # Metallic avoids a diffuse layer; the alpha mixture supplies transmission.
    white=lib.create_material_expression(m,u.MaterialExpressionConstant3Vector)
    white.set_editor_property('constant',u.LinearColor(1,1,1,1))
    lib.connect_material_property(white,'',u.MaterialProperty.MP_BASE_COLOR)
    scalar(1,u.MaterialProperty.MP_METALLIC)
    # Resolved waves already supply the surface slopes. Keep only a small
    # unresolved micro-roughness instead of blurring the entire reflected scene.
    scalar(.02,u.MaterialProperty.MP_ROUGHNESS)
    scalar(1/1.333,u.MaterialProperty.MP_REFRACTION)
    nodes={'P':lib.create_material_expression(m,u.MaterialExpressionWorldPosition),
           'View':lib.create_material_expression(m,u.MaterialExpressionCameraVectorWS)}
    for input_name,param,default in [('T','WaveTime',0),('WindStrength','WindStrength',.25)]:
        node=lib.create_material_expression(m,u.MaterialExpressionScalarParameter)
        node.set_editor_property('parameter_name',param);node.set_editor_property('default_value',default)
        nodes[input_name]=node
    for name in ['Viewer','Wave0','Wave1','Wave2','Wave3','WavePhase']:
        vector=lib.create_material_expression(m,u.MaterialExpressionVectorParameter)
        vector.set_editor_property('parameter_name',name)
        rgba=lib.create_material_expression(m,u.MaterialExpressionAppendVector)
        lib.connect_material_expressions(vector,'RGB',rgba,'A');lib.connect_material_expressions(vector,'A',rgba,'B')
        nodes[name]=rgba
    def custom(code,kind,prop,with_view=False):
        fn=lib.create_material_expression(m,u.MaterialExpressionCustom)
        inputs=[]
        selected={name:node for name,node in nodes.items() if name!='View' or with_view}
        for name in selected:
            item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
        fn.set_editor_property('inputs',inputs);fn.set_editor_property('output_type',kind);fn.set_editor_property('code',code)
        for name,node in selected.items():
            if not lib.connect_material_expressions(node,'',fn,name):raise RuntimeError(name)
        if not lib.connect_material_property(fn,'',prop):raise RuntimeError(str(prop))
    custom(wave_code+'return float3(0,0,h*100*fade);',u.CustomMaterialOutputType.CMOT_FLOAT3,u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    optics=(Path(__file__).parent/'Shaders/WaterOptics.ush').read_text()
    normal=optics+wave_code+'''
DiveWaterOptics optical;
float height;float2 allSlope;float3 curvature;
optical.Surface(xy,T,waves,WavePhase,WindStrength,ddx(xy),ddy(xy),height,allSlope,curvature);
float3 normal=normalize(float3(slope*fade+h*fadeSlope+(allSlope-slope),-1));
'''
    custom(normal+'return normal;',u.CustomMaterialOutputType.CMOT_FLOAT3,u.MaterialProperty.MP_NORMAL)
    fresnel=(Path(__file__).parent/'Shaders/WaterInterfaceFresnel.ush').read_text()
    custom(normal+fresnel,u.CustomMaterialOutputType.CMOT_FLOAT1,u.MaterialProperty.MP_OPACITY,with_view=True)
    lib.recompile_material(m)
    if not u.EditorAssetLibrary.save_loaded_asset(m):raise RuntimeError('Water underside save failed')
    u.log('DIVE_WATER_INTERFACE_CONTENT_OK fresnel=water_to_air reflection=lumen_front_layer')
