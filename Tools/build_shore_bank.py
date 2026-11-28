"""Build an isolated shore-bank material; accepted terrain/rock assets stay intact."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
data=json.loads((ROOT/'divingsim/Content/Diving/Geodata/shore_stones.json').read_text(encoding='utf-8'))
source=ROOT/data['mask']['file']
assert hashlib.sha256(source.read_bytes()).hexdigest()==data['mask']['sha256']
base='/Game/Diving/'
lib=u.MaterialEditingLibrary
task=u.AssetImportTask()
for key,value in dict(filename=str(source),destination_path=base+'Textures',destination_name='T_WeesenBankMask',automated=True,replace_existing=True,save=False).items():task.set_editor_property(key,value)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture=u.load_asset(base+'Textures/T_WeesenBankMask')
if not texture:raise RuntimeError('Missing bank mask')
for key,value in dict(srgb=False,compression_settings=u.TextureCompressionSettings.TC_MASKS,address_x=u.TextureAddress.TA_CLAMP,address_y=u.TextureAddress.TA_CLAMP,never_stream=True,lod_bias=0,max_texture_size=1024).items():texture.set_editor_property(key,value)
if not u.EditorAssetLibrary.save_loaded_asset(texture):raise RuntimeError('Mask save failed')
original=u.load_asset(base+'Materials/M_ShoreNatural')
path=base+'Materials/M_ShoreBank'
material=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(base+'Materials/M_ShoreNatural',path)
if not original or not material:raise RuntimeError('Missing source material')
custom=[x for x in lib.get_material_expressions(material) if isinstance(x,u.MaterialExpressionCustom)]
source_custom=[x for x in lib.get_material_expressions(original) if isinstance(x,u.MaterialExpressionCustom)]
if len(custom)!=1 or len(source_custom)!=1:raise RuntimeError('Unexpected material graph')
code=source_custom[0].get_editor_property('code').replace('\r\n','\n')
anchor='    groundColor=lerp(GroundPhoto,groundColor,groundNear);'
if code.count(anchor)!=1:raise RuntimeError('Natural ground source differs')
addition='''    float bank=saturate(ShoreBank);
    float3 bankStone=min(lerp(dot(stone,float3(.2126,.7152,.0722)).xxx,stone,.18)*1.10,float3(.65,.65,.65));
    groundColor=lerp(groundColor,bankStone,bank);
    groundNormal=normalize(lerp(groundNormal,rnGround,bank));
    groundRough=lerp(groundRough,clamp(raGround.y,.70,.98),bank);
    groundAO=lerp(groundAO,lerp(1,raGround.x,.25),bank);
'''
custom[0].set_editor_property('code',code.replace(anchor,addition+anchor))
inputs=list(custom[0].get_editor_property('inputs'))
if not any(str(x.get_editor_property('input_name'))=='ShoreBank' for x in inputs):
    pin=u.CustomInput();pin.set_editor_property('input_name','ShoreBank');inputs.append(pin)
    custom[0].set_editor_property('inputs',inputs)
nodes=[x for x in lib.get_material_expressions(material) if isinstance(x,u.MaterialExpressionTextureSampleParameter2D) and str(x.get_editor_property('parameter_name'))=='ShoreBankMask']
if len(nodes)>1:raise RuntimeError('Duplicate bank mask input')
node=nodes[0] if nodes else lib.create_material_expression(material,u.MaterialExpressionTextureSampleParameter2D)
node.set_editor_property('parameter_name','ShoreBankMask')
node.set_editor_property('texture',texture)
node.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_MASKS)
if not lib.connect_material_expressions(node,'R',custom[0],'ShoreBank'):raise RuntimeError('Bank mask connection failed')
lib.recompile_material(material)
if not u.EditorAssetLibrary.save_loaded_asset(material):raise RuntimeError('Bank material save failed')
report=dict(mask=data['mask'],material=path,shader_sha256=hashlib.sha256(custom[0].get_editor_property('code').encode()).hexdigest())
report['rock_materials']=[]
for index in range(1,4):
    source_path=base+f'Materials/M_RockScan{index}'
    target_path=base+f'Materials/M_ShoreBankRock{index}'
    original=u.load_asset(source_path)
    target=u.load_asset(target_path) if u.EditorAssetLibrary.does_asset_exist(target_path) else u.EditorAssetLibrary.duplicate_asset(source_path,target_path)
    if not original or not target:raise RuntimeError('Missing scanned bank rock material')
    originals=[x for x in lib.get_material_expressions(original) if isinstance(x,u.MaterialExpressionCustom)]
    targets=[x for x in lib.get_material_expressions(target) if isinstance(x,u.MaterialExpressionCustom)]
    if len(originals)!=1 or len(targets)!=1:raise RuntimeError('Unexpected rock material graph')
    source_code=originals[0].get_editor_property('code')
    tint='float3 color=lerp(dot(Color,float3(.2126,.7152,.0722)).xxx,Color,.7);'
    if source_code.count(tint)!=1:raise RuntimeError('Unexpected scan color expression')
    rock_code=source_code.replace(tint,'float3 color=min(lerp(dot(Color,float3(.2126,.7152,.0722)).xxx,Color,.18)*1.10,float3(.65,.65,.65));')
    targets[0].set_editor_property('code',rock_code)
    lib.recompile_material(target)
    if not u.EditorAssetLibrary.save_loaded_asset(target):raise RuntimeError('Bank rock save failed')
    report['rock_materials'].append(dict(source=source_path,target=target_path,shader_sha256=hashlib.sha256(rock_code.encode()).hexdigest()))
(ROOT/'artifacts/ShoreBankBuild.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
u.log('DIVE_SHORE_BANK_BUILD_PASS')
