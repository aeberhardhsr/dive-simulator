"""Build the underwater HDR attenuation material without reimporting other assets."""
import unreal as u


def build_optics():
    lib=u.MaterialEditingLibrary
    path='/Game/Diving/Materials/M_Underwater'
    post=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_Underwater','/Game/Diving/Materials',u.Material,u.MaterialFactoryNew())
    for node in list(lib.get_material_expressions(post)): lib.delete_material_expression(post,node)
    post.set_editor_property('material_domain',u.MaterialDomain.MD_POST_PROCESS)
    post.set_editor_property('blendable_location',u.BlendableLocation.BL_SCENE_COLOR_BEFORE_BLOOM)
    post.set_editor_property('disable_pre_exposure_scale',False)
    scene=lib.create_material_expression(post,u.MaterialExpressionSceneTexture,-500,0)
    scene.set_editor_property('scene_texture_id',u.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    depth=lib.create_material_expression(post,u.MaterialExpressionSceneDepth,-500,200)
    parameters={}
    for name,default in [('VisibilityCm',1400),('AmbientLight',0)]:
        parameter=lib.create_material_expression(post,u.MaterialExpressionScalarParameter)
        parameter.set_editor_property('parameter_name',name)
        parameter.set_editor_property('default_value',default)
        parameters[name]=parameter
    fog=lib.create_material_expression(post,u.MaterialExpressionCustom,0,0)
    inputs=[]
    for name in ['Scene','Depth','Visibility','Ambient']:
        item=u.CustomInput(); item.set_editor_property('input_name',name); inputs.append(item)
    fog.set_editor_property('inputs',inputs)
    fog.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    fog.set_editor_property('code','''
    float d=max(0,Depth);
    float transmission=exp(-d/max(150,Visibility)*1.45);
    float3 attenuation=exp(-d*float3(.00045,.00009,.00012));
    // Scene-linear values before bloom/tonemapping. No fixed display-space glow.
    return Scene.rgb*attenuation*transmission
        + float3(250,1050,900)*saturate(Ambient)*(1-transmission);
    ''')
    lib.connect_material_expressions(scene,'Color',fog,'Scene')
    lib.connect_material_expressions(depth,'',fog,'Depth')
    lib.connect_material_expressions(parameters['VisibilityCm'],'',fog,'Visibility')
    lib.connect_material_expressions(parameters['AmbientLight'],'',fog,'Ambient')
    lib.connect_material_property(fog,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(post)
    if not u.EditorAssetLibrary.save_loaded_asset(post): raise RuntimeError('Cannot save underwater optics')
    u.log('DIVE_OPTICS_CONTENT_OK stage=BeforeBloom ambient_default=0')


if __name__=='__main__': build_optics()
