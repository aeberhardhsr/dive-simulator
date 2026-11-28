"""Build an isolated far-cliff material; never rebuild shore/underwater assets."""
import hashlib
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
SOURCE = '/Game/Diving/Materials/M_LandscapePhoto'
TARGET = '/Game/Diving/Materials/M_LandscapeCliffStudy'


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
    lib.recompile_material(material)
    u.EditorAssetLibrary.set_metadata_tag(material, 'DiveCliffStudyShaderSHA256', hashlib.sha256(code.encode()).hexdigest())
    if not u.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Cannot save cliff study material')
    if hashlib.sha256(source_file.read_bytes()).hexdigest() != before:
        raise RuntimeError('Original material unexpectedly changed')
    u.log('DIVE_CLIFF_STUDY_CONTENT_OK source_unchanged=1 shader=' + hashlib.sha256(code.encode()).hexdigest())


if __name__ == '__main__':
    build()
