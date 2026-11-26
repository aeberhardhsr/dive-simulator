"""A bounded water medium voxelized by Unreal's native volumetric-fog pass."""
from pathlib import Path
import unreal as u


def build_water_volume(wave_code):
    lib=u.MaterialEditingLibrary
    path='/Game/Diving/Materials/M_WaterVolume'
    m=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_WaterVolume','/Game/Diving/Materials',u.Material,u.MaterialFactoryNew())
    for node in list(lib.get_material_expressions(m)):lib.delete_material_expression(m,node)
    m.set_editor_property('material_domain',u.MaterialDomain.MD_VOLUME)
    m.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
    albedo=lib.create_material_expression(m,u.MaterialExpressionConstant3Vector)
    albedo.set_editor_property('constant',u.LinearColor(.07,.24,.16,1))
    lib.connect_material_property(albedo,'',u.MaterialProperty.MP_BASE_COLOR)
    nodes={'P':lib.create_material_expression(m,u.MaterialExpressionWorldPosition)}
    for name,param,value in [('T','WaveTime',0),('Visibility','VisibilityMetres',14),('DensityGain','DensityGain',1)]:
        n=lib.create_material_expression(m,u.MaterialExpressionScalarParameter)
        n.set_editor_property('parameter_name',param);n.set_editor_property('default_value',value);nodes[name]=n
    for name in ['Viewer','Wave0','Wave1','Wave2','Wave3','WavePhase']:
        n=lib.create_material_expression(m,u.MaterialExpressionVectorParameter);n.set_editor_property('parameter_name',name)
        a=lib.create_material_expression(m,u.MaterialExpressionAppendVector)
        lib.connect_material_expressions(n,'RGB',a,'A');lib.connect_material_expressions(n,'A',a,'B');nodes[name]=a
    fn=lib.create_material_expression(m,u.MaterialExpressionCustom)
    inputs=[]
    for name in nodes:
        item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
    fn.set_editor_property('inputs',inputs);fn.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    fn.set_editor_property('code',wave_code+(Path(__file__).parent/'Shaders/WaterVolume.ush').read_text())
    for name,node in nodes.items():lib.connect_material_expressions(node,'',fn,name)
    # Volume-domain extinction uses the subsurface slot. Unreal converts /metre
    # to /centimetre in VolumetricFogVoxelization.usf, so no /100 here.
    lib.connect_material_property(fn,'',u.MaterialProperty.MP_SUBSURFACE_COLOR)
    lib.recompile_material(m)
    if not u.EditorAssetLibrary.save_loaded_asset(m):raise RuntimeError('Water volume save failed')
    u.log('DIVE_WATER_VOLUME_CONTENT_OK extent=below_shared_waves units=inverse_metres')
