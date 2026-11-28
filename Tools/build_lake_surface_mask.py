"""Shared geographic coverage node for both sides of the water interface."""
from pathlib import Path
import unreal as u


def lake_surface_mask(material):
    lib=u.MaterialEditingLibrary
    nodes={'P':lib.create_material_expression(material,u.MaterialExpressionWorldPosition)}
    grid=lib.create_material_expression(material,u.MaterialExpressionTextureObjectParameter)
    grid.set_editor_property('parameter_name','LakeGrid')
    grid.set_editor_property('texture',u.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
    nodes['LakeGrid']=grid
    enabled=lib.create_material_expression(material,u.MaterialExpressionScalarParameter)
    enabled.set_editor_property('parameter_name','LakeSurfaceEnabled');enabled.set_editor_property('default_value',0)
    nodes['LakeSurfaceEnabled']=enabled
    mapping=lib.create_material_expression(material,u.MaterialExpressionVectorParameter)
    mapping.set_editor_property('parameter_name','LakeMap')
    rgba=lib.create_material_expression(material,u.MaterialExpressionAppendVector)
    lib.connect_material_expressions(mapping,'RGB',rgba,'A');lib.connect_material_expressions(mapping,'A',rgba,'B')
    nodes['LakeMap']=rgba
    fn=lib.create_material_expression(material,u.MaterialExpressionCustom)
    inputs=[]
    for name in nodes:
        item=u.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
    fn.set_editor_property('inputs',inputs)
    fn.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
    shared=(Path(__file__).resolve().parents[1]/'divingsim/Shaders/LakeFootprint.ush').read_text()
    fn.set_editor_property('code',shared+'''
if(LakeSurfaceEnabled<.5) return 1;
FDiveLakeFootprint lake;
return step(0,lake.Distance(LakeGrid,LakeGridSampler,P.xy*.01,LakeMap));
''')
    for name,node in nodes.items():
        if not lib.connect_material_expressions(node,'',fn,name):raise RuntimeError('Lake surface input '+name)
    return fn
