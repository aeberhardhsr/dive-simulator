"""Build an isolated far-cliff material; never rebuild shore/underwater assets."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
SOURCE = '/Game/Diving/Materials/M_LandscapePhoto'
TARGET = '/Game/Diving/Materials/M_LandscapeCliffStudy'


def coverage_texture():
    report = json.loads((ROOT / 'Documentation/CLIFF_COVERAGE.json').read_text(encoding='utf-8'))
    if report['status'] != 'DECODED_COVERAGE_VALIDATED' or report['bounds'] != [2723000,1215000,2745000,1228000]:
        raise RuntimeError('Validated cliff coverage required')
    item = report['output']
    path = ROOT / item['file']
    if hashlib.sha256(path.read_bytes()).hexdigest() != item['sha256']:
        raise RuntimeError('Cliff coverage source changed')
    asset = '/Game/Diving/Textures/T_CliffCoverage'
    texture = u.load_asset(asset) if u.EditorAssetLibrary.does_asset_exist(asset) else None
    if not texture or u.EditorAssetLibrary.get_metadata_tag(texture, 'DiveCliffCoverageSHA256') != item['sha256']:
        task = u.AssetImportTask()
        for key,value in [('filename',str(path)),('destination_path','/Game/Diving/Textures'),
                          ('destination_name','T_CliffCoverage'),('automated',True),('replace_existing',True),('save',False)]:
            task.set_editor_property(key,value)
        u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = u.load_asset(asset)
    if not texture:
        raise RuntimeError('Cliff coverage import failed')
    for key,value in [('srgb',False),('compression_settings',u.TextureCompressionSettings.TC_VECTOR_DISPLACEMENTMAP),
                      ('mip_gen_settings',u.TextureMipGenSettings.TMGS_SIMPLE_AVERAGE),
                      ('filter',u.TextureFilter.TF_TRILINEAR),('address_x',u.TextureAddress.TA_CLAMP),
                      ('address_y',u.TextureAddress.TA_CLAMP),('never_stream',True),('max_texture_size',4096),('lod_bias',0)]:
        texture.set_editor_property(key,value)
    u.EditorAssetLibrary.set_metadata_tag(texture,'DiveCliffCoverageSHA256',item['sha256'])
    if not u.EditorAssetLibrary.save_loaded_asset(texture):
        raise RuntimeError('Cannot save cliff coverage')
    return texture


def build():
    source_file = ROOT / 'divingsim/Content/Diving/Materials/M_LandscapePhoto.uasset'
    before = hashlib.sha256(source_file.read_bytes()).hexdigest()
    source = u.load_asset(SOURCE)
    if not source:
        raise RuntimeError('Existing landscape material required')
    material = u.load_asset(TARGET) if u.EditorAssetLibrary.does_asset_exist(TARGET) else None
    if not material:
        material = u.EditorAssetLibrary.duplicate_asset(SOURCE, TARGET)
    if not material:
        raise RuntimeError('Cannot create isolated cliff material')
    lib = u.MaterialEditingLibrary
    coverage = coverage_texture()
    customs = [node for node in lib.get_material_expressions(material)
               if isinstance(node, u.MaterialExpressionCustom)]
    if len(customs) != 1:
        raise RuntimeError('Expected exactly one landscape custom expression')
    custom = customs[0]
    source_customs = [node for node in lib.get_material_expressions(source)
                      if isinstance(node, u.MaterialExpressionCustom)]
    if len(source_customs) != 1:
        raise RuntimeError('Source material graph changed')
    original = source_customs[0].get_editor_property('code')
    if not original.rstrip().endswith('return result;'):
        raise RuntimeError('Unexpected source shader ending')
    extension = (ROOT / 'Tools/Shaders/LandscapeCliffs.ush').read_text(encoding='utf-8')
    code = original.rstrip()[:-len('return result;')] + extension + '\nreturn result;'
    custom.set_editor_property('code', code)
    inputs = list(custom.get_editor_property('inputs'))
    if not any(str(item.get_editor_property('input_name')) == 'FarOrtho' for item in inputs):
        item = u.CustomInput()
        item.set_editor_property('input_name', 'FarOrtho')
        inputs.append(item)
        custom.set_editor_property('inputs', inputs)
        texture = lib.create_material_expression(material, u.MaterialExpressionTextureObjectParameter)
        texture.set_editor_property('parameter_name', 'Ortho')
        texture.set_editor_property('texture', u.load_asset('/Game/Diving/Textures/T_Ortho_landscape'))
        texture.set_editor_property('sampler_type', u.MaterialSamplerType.SAMPLERTYPE_COLOR)
        if not lib.connect_material_expressions(texture, '', custom, 'FarOrtho'):
            raise RuntimeError('Cannot connect registered far-cliff tint')
    inputs = list(custom.get_editor_property('inputs'))
    if not any(str(item.get_editor_property('input_name')) == 'Coverage' for item in inputs):
        item = u.CustomInput()
        item.set_editor_property('input_name','Coverage')
        inputs.append(item)
        custom.set_editor_property('inputs',inputs)
        texture = lib.create_material_expression(material,u.MaterialExpressionTextureObjectParameter)
        texture.set_editor_property('parameter_name','CliffCoverage')
        texture.set_editor_property('texture',coverage)
        texture.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        if not lib.connect_material_expressions(texture,'',custom,'Coverage'):
            raise RuntimeError('Cannot connect cliff coverage')
    lib.recompile_material(material)
    u.EditorAssetLibrary.set_metadata_tag(material, 'DiveCliffStudyShaderSHA256', hashlib.sha256(code.encode()).hexdigest())
    if not u.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Cannot save cliff study material')
    if hashlib.sha256(source_file.read_bytes()).hexdigest() != before:
        raise RuntimeError('Original material unexpectedly changed')
    u.log('DIVE_CLIFF_STUDY_CONTENT_OK source_unchanged=1 shader=' + hashlib.sha256(code.encode()).hexdigest())


if __name__ == '__main__':
    build()
