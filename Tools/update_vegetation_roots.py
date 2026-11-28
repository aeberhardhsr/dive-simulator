"""Unreal editor: reimport only the corrected distant tree LODs, retaining LOD0/materials."""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT/'blender/exports/Vegetation/Manifest.json').read_text())
editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)
flag = 'Interchange.FeatureFlags.Import.FBX'
previous = u.SystemLibrary.get_console_variable_int_value(flag)
results = []
try:
    u.SystemLibrary.execute_console_command(None, flag+' 0')
    for name in ('SM_LeafTree', 'SM_FirA', 'SM_FirB', 'SM_FirC'):
        levels = [a for a in manifest['assets'] if a['name']==name and a['lod']>=2]
        if sorted(a['lod'] for a in levels) != [2,3]:
            raise RuntimeError('Missing distant LODs '+name)
        for item in levels:
            if item.get('preserved_root_vertices',0)<1 or abs(item['bounds_m'][0][2])>.01:
                raise RuntimeError('Root protection not validated '+name)
            if hashlib.sha256((ROOT/item['file']).read_bytes()).hexdigest()!=item['sha256']:
                raise RuntimeError('FBX checksum mismatch '+item['file'])
        mesh = u.load_asset('/Game/Diving/Vegetation/Meshes/'+name)
        if not mesh or editor.get_lod_count(mesh)!=4:
            raise RuntimeError('Expected existing four-LOD mesh '+name)
        slots = [str(s.get_editor_property('material_interface').get_path_name()) for s in mesh.get_editor_property('static_materials')]
        for item in levels:
            if editor.import_lod(mesh,item['lod'],str(ROOT/item['file']))!=item['lod']:
                raise RuntimeError('LOD import failed '+name)
        after = [str(s.get_editor_property('material_interface').get_path_name()) for s in mesh.get_editor_property('static_materials')]
        if slots!=after or editor.get_lod_count(mesh)!=4:
            raise RuntimeError('Material slots/LOD count changed '+name)
        if not u.EditorAssetLibrary.save_loaded_asset(mesh):
            raise RuntimeError('Save failed '+name)
        results.append(dict(mesh=name,lods=[2,3],materials=slots))
        u.log('DIVE_TREE_ROOT_IMPORT_PASS '+name)
finally:
    u.SystemLibrary.execute_console_command(None, flag+' '+str(previous))
destination=ROOT/'artifacts/VegetationRootImport.json'
destination.write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
u.log('DIVE_TREE_ROOT_IMPORT_COMPLETE')
