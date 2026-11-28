"""Enable the optional water depth pass without regenerating other water assets."""
import unreal as u

m=u.load_asset('/Game/Diving/Materials/M_WaterUnder')
if not m:raise RuntimeError('Missing water underside material')
m.set_editor_property('allow_translucent_custom_depth_writes',True)
m.set_editor_property('opacity_mask_clip_value',0.)
u.MaterialEditingLibrary.recompile_material(m)
if not u.EditorAssetLibrary.save_loaded_asset(m):raise RuntimeError('Save water depth material')
u.log('DIVE_SPECTRAL_DEPTH_CONTENT_OK custom_depth=1 opacity_clip=0')
