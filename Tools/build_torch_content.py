"""Import torch variants and build lit suspended particles and water attenuation."""
from pathlib import Path
import unreal as u


def build_torch():
    root=Path(__file__).resolve().parents[1];lib=u.MaterialEditingLibrary;tools=u.AssetToolsHelpers.get_asset_tools()
    def material(name,domain):
        path='/Game/Diving/Materials/'+name
        result=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(name,'/Game/Diving/Materials',u.Material,u.MaterialFactoryNew())
        for node in list(lib.get_material_expressions(result)):lib.delete_material_expression(result,node)
        result.set_editor_property('material_domain',domain)
        return result
    particle=material('M_Particle',u.MaterialDomain.MD_SURFACE)
    particle.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
    particle.set_editor_property('used_with_instanced_static_meshes',True)
    color=lib.create_material_expression(particle,u.MaterialExpressionConstant3Vector)
    color.set_editor_property('constant',u.LinearColor(.45,.52,.43,1))
    lib.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
    rough=lib.create_material_expression(particle,u.MaterialExpressionConstant);rough.set_editor_property('r',1)
    lib.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
    torch=material('M_TorchWater',u.MaterialDomain.MD_LIGHT_FUNCTION)
    position=lib.create_material_expression(torch,u.MaterialExpressionWorldPosition)
    lamp=lib.create_material_expression(torch,u.MaterialExpressionVectorParameter);lamp.set_editor_property('parameter_name','LampPosition')
    surface=lib.create_material_expression(torch,u.MaterialExpressionScalarParameter);surface.set_editor_property('parameter_name','WaterSurfaceZ')
    extinction=lib.create_material_expression(torch,u.MaterialExpressionScalarParameter);extinction.set_editor_property('parameter_name','ExtinctionPerCm');extinction.set_editor_property('default_value',1.45/1400)
    fn=lib.create_material_expression(torch,u.MaterialExpressionCustom)
    inputs=[]
    for name in ['Position','Lamp','Surface','Extinction']:
        item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
    fn.set_editor_property('inputs',inputs);fn.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
    fn.set_editor_property('code','''
        float a=Surface-Lamp.z,b=Surface-Position.z;
        float fraction=(a>0&&b>0)?1:((a<=0&&b<=0)?0:(a>0?a/max(a-b,.001):b/max(b-a,.001)));
        return exp(-max(0,Extinction)*length(Position-Lamp.rgb)*saturate(fraction));
    ''')
    for node,output,input_name in [(position,'','Position'),(lamp,'','Lamp'),(surface,'','Surface'),(extinction,'','Extinction')]:
        lib.connect_material_expressions(node,output,fn,input_name)
    lib.connect_material_property(fn,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    for item in [particle,torch]:
        lib.recompile_material(item)
        if not u.EditorAssetLibrary.save_loaded_asset(item):raise RuntimeError('Failed to save '+item.get_name())
    for name in ['SM_DiveTorch','SM_TorchSpot','SM_TorchWide']:
        task=u.AssetImportTask();task.set_editor_property('filename',str(root/'blender'/'exports'/(name+'.fbx')))
        task.set_editor_property('destination_path','/Game/Diving/Meshes');task.set_editor_property('destination_name',name)
        for option in ['automated','save','replace_existing']:task.set_editor_property(option,True)
        options=u.FbxImportUI();options.set_editor_property('import_as_skeletal',False)
        options.set_editor_property('import_materials',False);options.set_editor_property('import_textures',False)
        options.set_editor_property('automated_import_should_detect_type',False);options.set_editor_property('mesh_type_to_import',u.FBXImportType.FBXIT_STATIC_MESH)
        data=options.get_editor_property('static_mesh_import_data');data.set_editor_property('combine_meshes',True)
        data.set_editor_property('vertex_color_import_option',u.VertexColorImportOption.REPLACE)
        task.set_editor_property('options',options);tools.import_asset_tasks([task])
        mesh=u.load_asset('/Game/Diving/Meshes/'+name)
        if not mesh:raise RuntimeError('Missing '+name)
        mesh.set_material(0,u.load_asset('/Game/Diving/Materials/M_Equipment'))
        if not u.EditorAssetLibrary.save_loaded_asset(mesh):raise RuntimeError('Failed to save '+name)
    u.log('DIVE_TORCH_CONTENT_OK models=3 lit_particles=1 water_attenuation=1')


if __name__=='__main__':build_torch()
