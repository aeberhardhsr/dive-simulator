"""Build the sun's world-space underwater attenuation and caustics material."""
from pathlib import Path
import unreal as u


def build_caustics():
    root=Path(__file__).resolve().parent
    lib=u.MaterialEditingLibrary
    name='M_SolarWater';path='/Game/Diving/Materials/'+name
    material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,'/Game/Diving/Materials',u.Material,u.MaterialFactoryNew())
    for node in list(lib.get_material_expressions(material)):lib.delete_material_expression(material,node)
    material.set_editor_property('material_domain',u.MaterialDomain.MD_LIGHT_FUNCTION)
    nodes={'P':lib.create_material_expression(material,u.MaterialExpressionWorldPosition)}
    for name,default in [('Clock',0),('WindStrength',.25),('Extinction',1.45/14),('CausticStrength',1)]:
        node=lib.create_material_expression(material,u.MaterialExpressionScalarParameter)
        node.set_editor_property('parameter_name',name);node.set_editor_property('default_value',default);nodes[name]=node
    for name in ['Wave0','Wave1','Wave2','Wave3','WavePhase','SunRay']:
        node=lib.create_material_expression(material,u.MaterialExpressionVectorParameter)
        node.set_editor_property('parameter_name',name);nodes[name]=node
    fn=lib.create_material_expression(material,u.MaterialExpressionCustom)
    inputs=[]
    for name in nodes:
        item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
    fn.set_editor_property('inputs',inputs)
    fn.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
    code=(root/'Shaders/SolarCaustics.ush').read_text().replace('// WATER_OPTICS_INCLUDE',(root/'Shaders/WaterOptics.ush').read_text())
    fn.set_editor_property('code',code)
    for name,node in nodes.items():
        # Vector parameters need RGBA for wave amplitudes and all four phases.
        if name.startswith('Wave'):
            rgba=lib.create_material_expression(material,u.MaterialExpressionAppendVector)
            lib.connect_material_expressions(node,'RGB',rgba,'A')
            lib.connect_material_expressions(node,'A',rgba,'B')
            node=rgba
        if not lib.connect_material_expressions(node,'',fn,name):raise RuntimeError(name)
    if not lib.connect_material_property(fn,'',u.MaterialProperty.MP_EMISSIVE_COLOR):raise RuntimeError('Output')
    lib.recompile_material(material)
    if not u.EditorAssetLibrary.save_loaded_asset(material):raise RuntimeError('Save solar water material')
    probe_path='/Game/Diving/Materials/M_CausticsProbe'
    probe=u.load_asset(probe_path) if u.EditorAssetLibrary.does_asset_exist(probe_path) else u.AssetToolsHelpers.get_asset_tools().create_asset('M_CausticsProbe','/Game/Diving/Materials',u.Material,u.MaterialFactoryNew())
    for node in list(lib.get_material_expressions(probe)):lib.delete_material_expression(probe,node)
    color=lib.create_material_expression(probe,u.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant',u.LinearColor(.5,.5,.5,1))
    lib.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
    rough=lib.create_material_expression(probe,u.MaterialExpressionConstant);rough.set_editor_property('r',.9)
    lib.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(probe)
    if not u.EditorAssetLibrary.save_loaded_asset(probe):raise RuntimeError('Save diagnostic receiver')
    u.log('DIVE_CAUSTICS_CONTENT_OK shared_surface=1 light_function=1')


if __name__=='__main__':build_caustics()
