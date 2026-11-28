"""Import original synthesized sounds without touching meshes/materials/maps."""
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
LOOPS={'A_Inflate','A_Vent','A_Underwater','A_Surface','A_Wind','A_Rain'}
NAMES=['A_Inhale','A_Exhale','A_Bubbles','A_Inflate','A_Vent','A_Heart',
       'A_Underwater','A_Surface','A_Wind','A_Rain','A_Warning']

def import_audio():
    for name in NAMES:
        source=ROOT/'blender'/'exports'/(name+'.wav')
        if not source.is_file(): raise RuntimeError('Missing sound '+str(source))
        task=u.AssetImportTask()
        task.set_editor_property('filename',str(source))
        task.set_editor_property('destination_path','/Game/Diving/Audio')
        task.set_editor_property('automated',True)
        task.set_editor_property('replace_existing',True)
        task.set_editor_property('save',False)
        u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        sound=u.load_asset('/Game/Diving/Audio/'+name)
        if not sound: raise RuntimeError('Audio import failed: '+name)
        sound.set_editor_property('looping',name in LOOPS)
        sound.set_editor_property('virtualization_mode',u.VirtualizationMode.PLAY_WHEN_SILENT)
        if not u.EditorAssetLibrary.save_loaded_asset(sound): raise RuntimeError('Audio save failed: '+name)
    print('DIVE_AUDIO_IMPORT_OK')

if __name__=='__main__': import_audio()
