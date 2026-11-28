"""Update only the five generated fish in the original Blender source collection."""
from pathlib import Path
import bpy
ROOT=Path(__file__).resolve().parents[1]
names=['SM_'+n for n in ['Perch','Roach','Pike','Whitefish','Burbot']]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'blender/DivingAssets.blend'))
for name in names:
    obj=bpy.data.objects.get(name)
    if obj:bpy.data.objects.remove(obj,do_unlink=True)
with bpy.data.libraries.load(str(ROOT/'blender/FishAssets.blend'),link=False) as (source,target):
    assert all(name in source.objects for name in names)
    target.objects=names
for obj in target.objects:bpy.context.scene.collection.objects.link(obj)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'blender/DivingAssets.blend'))
print('FISH_COLLECTION_SYNC_PASS')
