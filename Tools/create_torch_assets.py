"""Original lathed torch bodies. Run in a separate Blender background process."""
import math
from pathlib import Path
import bpy


def create_torches():
    out=Path(__file__).resolve().parents[1]/'blender'/'exports'
    out.mkdir(parents=True,exist_ok=True)
    for name,rear,body,head in [('SM_DiveTorch',.06,.024,.032),('SM_TorchSpot',.10,.020,.026),('SM_TorchWide',.045,.027,.055)]:
        # The full asset pipeline already creates the original hand torch.
        # Replace that object as well as its export, avoiding a second .001 body.
        if old:=bpy.data.objects.get(name):bpy.data.objects.remove(old,do_unlink=True)
        vertices,faces,colors=[],[],[]
        profile=[(rear,0),(rear,.018),(rear+.015,body)]
        for i in range(9):
            x=rear+.022+i*(.17-rear-.022)/9
            profile.extend([(x,body),(x+.002,body+.002),(x+.004,body)])
        profile.extend([(.175,body),(.19,head),(.211,head),(.223,head),(.224,head*.83),(.224,0)])
        sides=64
        for i,(x,radius) in enumerate(profile):
            colour=(.055,.07,.08,1) if i<len(profile)-5 else (.15,.20,.22,1)
            if i>=len(profile)-2: colour=(.28,.40,.44,1)
            for j in range(sides):
                angle=2*math.pi*j/sides
                vertices.append((x,math.cos(angle)*radius,-.038+math.sin(angle)*radius))
                colors.append(colour)
                if i:
                    a=i*sides+j;b=i*sides+(j+1)%sides
                    faces.append((a,a-sides,b) if profile[i-1][1]==0 else (a,a-sides,b-sides) if radius==0 else (a,a-sides,b-sides,b))
        mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.update()
        uv=mesh.uv_layers.new(name='UVMap')
        for face in mesh.polygons:
            crosses_seam=any(mesh.loops[k].vertex_index%sides==sides-1 for k in face.loop_indices)
            for k in face.loop_indices:
                index=mesh.loops[k].vertex_index;j=index%sides
                uv.data[k].uv=(1 if crosses_seam and j==0 else j/sides,(index//sides)/(len(profile)-1))
        obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
        attribute=mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
        for i,color in enumerate(colors):attribute.data[i].color=color
        mesh.color_attributes.active_color=attribute
        for face in mesh.polygons:face.use_smooth=True
        bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
        bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,apply_unit_scale=True,
            axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)
    print('DIVE_TORCH_ASSETS_OK models=3')


if __name__=='__main__':
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    create_torches()
    bpy.ops.wm.save_as_mainfile(filepath=str(Path(__file__).resolve().parents[1]/'blender'/'TorchAssets.blend'))
