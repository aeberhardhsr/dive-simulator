"""Build two independent above-water materials, preserving the accepted originals."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
BASE='/Game/Diving/Materials/'
old=(ROOT/'Tools/Shaders/GroundDetail.ush').read_text(encoding='utf-8').strip()
new=(ROOT/'Tools/Shaders/NaturalGround.ush').read_text(encoding='utf-8').strip()
lib=u.MaterialEditingLibrary
verify='-NaturalGroundVerify' in u.SystemLibrary.get_command_line()
results=[]
for source_name,target_name in [('M_LandscapePhoto','M_LandscapeNatural'),('M_ShorePhoto','M_ShoreNatural')]:
    source_path=ROOT/('divingsim/Content/Diving/Materials/'+source_name+'.uasset')
    before=hashlib.sha256(source_path.read_bytes()).hexdigest()
    source=u.load_asset(BASE+source_name)
    exists=u.EditorAssetLibrary.does_asset_exist(BASE+target_name)
    if verify and not exists: raise RuntimeError('Missing natural material '+target_name)
    material=u.load_asset(BASE+target_name) if exists else u.EditorAssetLibrary.duplicate_asset(BASE+source_name,BASE+target_name)
    if not source or not material: raise RuntimeError('Missing material '+source_name)
    originals=[x for x in lib.get_material_expressions(source) if isinstance(x,u.MaterialExpressionCustom)]
    targets=[x for x in lib.get_material_expressions(material) if isinstance(x,u.MaterialExpressionCustom)]
    if len(originals)!=1 or len(targets)!=1: raise RuntimeError('Unexpected custom expression count')
    code=originals[0].get_editor_property('code').replace('\r\n','\n')
    if code.count(old)!=1: raise RuntimeError('Original ground shader does not match source')
    prefix,suffix=code.split(old)
    if 'float Noise(' not in prefix: raise RuntimeError('Required noise helper is absent from source')
    replacement=prefix+new+suffix
    digest=hashlib.sha256(replacement.encode()).hexdigest()
    if verify:
        if targets[0].get_editor_property('code').replace('\r\n','\n')!=replacement or u.EditorAssetLibrary.get_metadata_tag(material,'DiveNaturalGroundSHA256')!=digest:
            raise RuntimeError('Natural material is stale '+target_name)
    else:
        targets[0].set_editor_property('code',replacement)
        lib.recompile_material(material)
        u.EditorAssetLibrary.set_metadata_tag(material,'DiveNaturalGroundSHA256',digest)
        if not u.EditorAssetLibrary.save_loaded_asset(material): raise RuntimeError('Save failed')
    if hashlib.sha256(source_path.read_bytes()).hexdigest()!=before: raise RuntimeError('Original material changed')
    results.append(dict(source=source_name,target=target_name,source_sha256=before,shader_sha256=digest,
                        prefix_sha256=hashlib.sha256(prefix.encode()).hexdigest(),suffix_sha256=hashlib.sha256(suffix.encode()).hexdigest()))
    u.log(('DIVE_NATURAL_GROUND_VERIFY_PASS ' if verify else 'DIVE_NATURAL_GROUND_BUILD_PASS ')+target_name)
if not verify:
    (ROOT/'artifacts/NaturalGroundBuild.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
