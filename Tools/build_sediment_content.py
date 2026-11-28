"""Create a lit, ray-integrated translucent sediment volume material."""
from pathlib import Path
import unreal as u


def build_sediment():
    lib=u.MaterialEditingLibrary
    path='/Game/Diving/Materials/M_FinSediment'
    m=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_FinSediment','/Game/Diving/Materials',u.Material,u.MaterialFactoryNew())
    for node in list(lib.get_material_expressions(m)):lib.delete_material_expression(m,node)
    m.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
    m.set_editor_property('translucency_lighting_mode',u.TranslucencyLightingMode.TLM_VOLUMETRIC_NON_DIRECTIONAL)
    # Compose after the opaque-distance underwater attenuation pass. Applying
    # that pass to an already composed nearby plume would erase it against a
    # distant background, even when the eye is inside the plume.
    m.set_editor_property('translucency_pass',u.MaterialTranslucencyPass.MTP_AFTER_DOF)
    m.set_editor_property('two_sided',True)
    # The integral clips against SceneDepth itself, including when the camera
    # is inside a volume and its exit face lies behind opaque terrain.
    m.set_editor_property('disable_depth_test',True)
    def property(node,prop):
        if not lib.connect_material_property(node,'',prop):raise RuntimeError(str(prop))
    color=lib.create_material_expression(m,u.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant',u.LinearColor(.36,.29,.18,1));property(color,u.MaterialProperty.MP_BASE_COLOR)
    for value,prop in [(1,u.MaterialProperty.MP_ROUGHNESS),(0,u.MaterialProperty.MP_SPECULAR)]:
        n=lib.create_material_expression(m,u.MaterialExpressionConstant);n.set_editor_property('r',value);property(n,prop)
    probe=lib.create_material_expression(m,u.MaterialExpressionVectorParameter);probe.set_editor_property('parameter_name','Probe')
    probe.set_editor_property('default_value',u.LinearColor(0,0,0,0))
    ambient=lib.create_material_expression(m,u.MaterialExpressionVectorParameter);ambient.set_editor_property('parameter_name','AmbientScattering')
    ambient.set_editor_property('default_value',u.LinearColor(0,0,0,0))
    add=lib.create_material_expression(m,u.MaterialExpressionAdd)
    lib.connect_material_expressions(probe,'',add,'A');lib.connect_material_expressions(ambient,'',add,'B')
    property(add,u.MaterialProperty.MP_EMISSIVE_COLOR)
    fn=lib.create_material_expression(m,u.MaterialExpressionCustom)
    names=['Position','Eye','Center','Radius','Optical','Age','Depth','Pixel','Face']
    inputs=[]
    for name in names:
        item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
    fn.set_editor_property('inputs',inputs);fn.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
    fn.set_editor_property('code',(Path(__file__).parent/'Shaders/FinSediment.ush').read_text())
    types={'Position':u.MaterialExpressionWorldPosition,'Eye':u.MaterialExpressionCameraPositionWS,
           'Depth':u.MaterialExpressionSceneDepth,'Pixel':u.MaterialExpressionPixelDepth,'Face':u.MaterialExpressionTwoSidedSign}
    for name in names:
        if name in types:n=lib.create_material_expression(m,types[name])
        else:
            n=lib.create_material_expression(m,u.MaterialExpressionVectorParameter if name=='Center' else u.MaterialExpressionScalarParameter)
            n.set_editor_property('parameter_name',name)
            if name!='Center':n.set_editor_property('default_value',1)
        if not lib.connect_material_expressions(n,'',fn,name):raise RuntimeError('Missing input '+name)
    property(fn,u.MaterialProperty.MP_OPACITY)
    lib.recompile_material(m)
    if not u.EditorAssetLibrary.save_loaded_asset(m):raise RuntimeError('Cannot save sediment material')
    u.log('DIVE_SEDIMENT_CONTENT_OK')


if __name__=='__main__':build_sediment()
