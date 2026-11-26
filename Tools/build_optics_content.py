"""Build the underwater HDR attenuation material without reimporting other assets."""
import hashlib
from pathlib import Path
import unreal as u


def build_optics():
    lib=u.MaterialEditingLibrary
    path='/Game/Diving/Materials/M_Underwater'
    post=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_Underwater','/Game/Diving/Materials',u.Material,u.MaterialFactoryNew())
    for node in list(lib.get_material_expressions(post)): lib.delete_material_expression(post,node)
    post.set_editor_property('material_domain',u.MaterialDomain.MD_POST_PROCESS)
    post.set_editor_property('blendable_location',u.BlendableLocation.BL_SCENE_COLOR_BEFORE_DOF)
    post.set_editor_property('disable_pre_exposure_scale',False)
    scene=lib.create_material_expression(post,u.MaterialExpressionSceneTexture,-500,0)
    scene.set_editor_property('scene_texture_id',u.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    world=lib.create_material_expression(post,u.MaterialExpressionWorldPosition,-800,200)
    camera=lib.create_material_expression(post,u.MaterialExpressionCameraPositionWS,-800,350)
    ray=lib.create_material_expression(post,u.MaterialExpressionSubtract,-500,200)
    lib.connect_material_expressions(world,'',ray,'A')
    lib.connect_material_expressions(camera,'',ray,'B')
    parameters={}
    for name,default in [('VisibilityCm',1400),('AmbientLight',0),('EyeDepthCm',0),('NativeScattering',0)]:
        parameter=lib.create_material_expression(post,u.MaterialExpressionScalarParameter)
        parameter.set_editor_property('parameter_name',name)
        parameter.set_editor_property('default_value',default)
        parameters[name]=parameter
    fog=lib.create_material_expression(post,u.MaterialExpressionCustom,0,0)
    inputs=[]
    for name in ['Scene','Ray','Visibility','Ambient','EyeDepth','NativeScattering']:
        item=u.CustomInput(); item.set_editor_property('input_name',name); inputs.append(item)
    fog.set_editor_property('inputs',inputs)
    fog.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    code=(Path(__file__).parent/'Shaders/UnderwaterView.ush').read_text()
    fog.set_editor_property('code',code)
    lib.connect_material_expressions(scene,'Color',fog,'Scene')
    lib.connect_material_expressions(ray,'',fog,'Ray')
    lib.connect_material_expressions(parameters['VisibilityCm'],'',fog,'Visibility')
    lib.connect_material_expressions(parameters['AmbientLight'],'',fog,'Ambient')
    lib.connect_material_expressions(parameters['EyeDepthCm'],'',fog,'EyeDepth')
    lib.connect_material_expressions(parameters['NativeScattering'],'',fog,'NativeScattering')
    lib.connect_material_property(fog,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(post)
    u.EditorAssetLibrary.set_metadata_tag(post,'DiveViewShaderSHA256',hashlib.sha256(code.encode()).hexdigest())
    if not u.EditorAssetLibrary.save_loaded_asset(post): raise RuntimeError('Cannot save underwater optics')
    u.log('DIVE_OPTICS_CONTENT_OK stage=BeforeDOF ambient_default=0 path=world_ray water_exit=clipped')


if __name__=='__main__': build_optics()
