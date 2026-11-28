"""Original UV-mapped freshwater fish, explicit fin rays and smooth body profiles."""
import math
import json
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
import bpy
from mathutils import Vector
from fish_profiles import FISH, shape

ROOT=Path(__file__).resolve().parents[1]

def create_fish(mesh_object,export):
    records=[]
    for name,cfg in FISH.items():
        length=cfg['length']; vertices=[]; faces=[]; uv=[]; colors=[];fin_offsets=[];fin_kinds=[]
        def add(p,tex=(.99,.99),rgb=(1,1,1),rig=None):
            vertices.append(tuple(v*length for v in p));uv.append(tex)
            # Extra UV channels carry dimensionless hinge offsets, type and
            # longitudinal root position. Keep the color atlas independent.
            dy,dz,kind,rootx=rig if rig else (0,0,0,0)
            fin_offsets.append((.5+dy,.5+dz));fin_kinds.append((kind/8,.5+rootx))
            w=max(0,min(1,(.18-p[0])/.65))**1.5
            colors.append((*rgb,w));return len(vertices)-1
        # Main body atlas occupies V=0..0.72. Duplicate seam vertices give
        # continuous tangents and avoid interpolation across the cylindrical seam.
        rings=113;sides=64
        for r in range(rings):
            t=r/(rings-1);x=-.37+.87*t
            width=shape(name,x,'width');height=shape(name,x,'height');zc=shape(name,x,'centre')
            for s in range(sides+1):
                a=s/sides*math.tau
                add((x,math.cos(a)*width,zc+math.sin(a)*height),(t,s/sides*.72))
                if r and s:
                    v=r*(sides+1)+s
                    faces.append((v-sides-2,v-sides-1,v,v-1))
        for ring,reverse in [(0,True),(rings-1,False)]:
            indices=[ring*(sides+1)+i for i in range(sides)]
            faces.append(tuple(reversed(indices)) if reverse else tuple(indices))
        # Ruled fin surfaces: UV rays are attached to the topology, not world
        # space. Two-sided rendering supplies the thin fin membrane.
        fin_count=0
        def fin(roots,tips,tint=(1,1,1),spines=False,kind=0):
            nonlocal fin_count
            fin_count+=1;start=len(vertices);segments=6
            for j,(root,tip) in enumerate(zip(roots,tips)):
                root,tip=Vector(root),Vector(tip)
                for k in range(segments+1):
                    t=k/segments;p=root.lerp(tip,t)
                    p.y+=.0015*math.sin(t*math.pi)*math.sin(j*.8)
                    # Fin band leaves gutters around body and eye islands.
                    shade=1
                    if name=='Perch' and spines:
                        shade-=.82*math.exp(-((j/(len(roots)-1)-.16)/.12)**2-((t-.65)/.38)**2)
                    add(p,(j/(len(roots)-1),.76+t*.17),tuple(c*shade for c in tint),
                        (p.y-root.y,p.z-root.z,kind,root.x))
                    if j and k:
                        v=start+j*(segments+1)+k
                        faces.append((v-segments-2,v-segments-1,v,v-1))
        def dorsal(start,end,rise,sign=1,spines=False,tint=(1,1,1),rays=22,kind=None):
            roots=[];tips=[]
            for j in range(rays*2+1):
                t=j/(rays*2);x=start+(end-start)*t
                z=shape(name,x,'centre')+sign*shape(name,x,'height')
                h=rise*max(0,min(t/.72,(1-t)/.28))**.65
                if spines:h=rise*max(0,min(t/.35,(1-t)/.16))**.5
                if spines and j%2:h*=.82
                roots.append((x,0,z-sign*.004));tips.append((x-.018*math.sin(math.pi*t),0,z+sign*h))
            fin(roots,tips,tint,spines,kind if kind is not None else 2 if spines else 1)
        grey=(.55,.58,.52);orange=(1,.40,.18);brown=(.58,.48,.30)
        tint=orange if name in ('Perch','Roach') else brown if name=='Burbot' else grey
        # Caudal fin: broad attached root and curved fork, rather than one
        # four-vertex planar arrowhead. Burbot has a rounded, unforked tail.
        roots=[];tips=[]
        for j in range(41):
            t=j/40;q=t*2-1
            roots.append((-.365,0,q*.023))
            if name=='Burbot':tips.append((-.385-.115*math.sqrt(max(0,1-q*q)),0,q*.065))
            else:tips.append((-.425-.075*abs(q)**.60,0,q*(.125 if name!='Pike' else .105)))
        fin(roots,tips,tint)
        if name=='Perch':
            dorsal(-.045,.245,.115,spines=True,tint=grey,rays=14)
            dorsal(-.23,-.055,.070,tint=grey,rays=15)
            dorsal(-.26,-.10,.072,-1,tint=tint,rays=11)
        elif name=='Burbot':
            dorsal(.08,.20,.032,tint=brown,rays=12)
            dorsal(-.355,.065,.040,tint=brown,rays=58)
            dorsal(-.35,.04,.036,-1,tint=brown,rays=56)
        elif name=='Pike':
            dorsal(-.30,-.115,.083,tint=grey,rays=18)
            dorsal(-.30,-.12,.078,-1,tint=grey,rays=16)
        else:
            dorsal(-.10,.12,.112,tint=grey,rays=14)
            dorsal(-.27,-.13,.075,-1,tint=tint,rays=12)
            if name=='Whitefish':dorsal(-.28,-.22,.026,tint=grey,rays=5,kind=0)
        for sign in (-1,1):
            # Fan-shaped pectoral fins attached behind the gill cover.
            roots=[];tips=[]
            px=.235 if name!='Pike' else .18
            for j in range(25):
                t=j/24;z=-.005-.035*t
                roots.append((px-.022*t,sign*shape(name,px,'width')*.92,z))
                tips.append((px-.04-.14*math.sin(math.pi*t*.8),sign*(.065+.07*math.sin(math.pi*t)),z-.035*math.sin(math.pi*t)))
            fin(roots,tips,tint,kind=3 if sign>0 else 4)
            # Perch pelvic insertion is below the pectoral; burbot anterior.
            px=.20 if name=='Burbot' else .12 if name=='Perch' else -.015
            roots=[];tips=[]
            for j in range(19):
                t=j/18;x=px-.055*t;z=shape(name,x,'centre')-shape(name,x,'height')*.82
                roots.append((x,sign*.022,z))
                tips.append((x-.07*math.sin(math.pi*t),sign*(.025+.035*math.sin(math.pi*t)),z-.055*math.sin(math.pi*t)))
            fin(roots,tips,tint,kind=5 if sign>0 else 6)
        # Small tubes follow the actual body surface: mouth lips, gill margin
        # and burbot chin barbel. These are original modeled features.
        def tube(points,radius,tex=(.18,.975),tint=(1,1,1)):
            start=len(vertices);sides=8
            for j,p in enumerate(points):
                p=Vector(p);a=Vector(points[max(0,j-1)]);b=Vector(points[min(len(points)-1,j+1)])
                d=(b-a).normalized();right=d.cross(Vector((0,0,1)))
                if right.length<.01:right=d.cross(Vector((0,1,0)))
                right.normalize();up=d.cross(right)
                rad=radius*(1-.65*j/(len(points)-1))
                for k in range(sides):
                    angle=k*math.tau/sides
                    add(p+rad*(right*math.cos(angle)+up*math.sin(angle)),(tex[0]+.012*j/(len(points)-1),tex[1]+.012*(k/sides-.5)),tint)
                    if j:
                        v=start+j*sides+k;n=start+j*sides+(k+1)%sides
                        faces.append((v-sides,n-sides,n,v))
        for sign in (-1,1):
            points=[]
            for j in range(25):
                a=-1.12+j/24*2.37;x=.25-.023*math.cos(a)
                points.append((x,sign*(shape(name,x,'width')*math.cos(a)+.0015),shape(name,x,'centre')+shape(name,x,'height')*math.sin(a)))
            tube(points,.0014,tex=(.18,.975))
            points=[]
            for j in range(18):
                x=.50-j/17*(.16 if name=='Pike' else .085)
                points.append((x,sign*(shape(name,x,'width')+.0006),shape(name,x,'centre')-.004))
            tube(points,.0017,tex=(.18,.975))
        if name=='Burbot':
            tube([(.445-.02*t,.004*math.sin(t*3),-.043-.065*t) for t in [j/16 for j in range(17)]],.0025,tex=(.32,.975))
        fish=mesh_object('SM_'+name,vertices,faces,colors)
        layer=fish.data.uv_layers.new(name='UVMap')
        for loop in fish.data.loops:layer.data[loop.index].uv=uv[loop.vertex_index]
        for label,values in [('FinOffset',fin_offsets),('FinKind',fin_kinds)]:
            extra=fish.data.uv_layers.new(name=label)
            for loop in fish.data.loops:extra.data[loop.index].uv=values[loop.vertex_index]
        # The two end caps need area in UV space, even though they sample a
        # nearly uniform lip swatch. Collapsed UVs produce invalid tangents.
        for poly in fish.data.polygons:
            if len(poly.vertices)==64:
                for j,loop in enumerate(poly.loop_indices):
                    a=j/64*math.tau;layer.data[loop].uv=(.31+.018*math.cos(a),.975+.012*math.sin(a))
        parts=[fish]
        # UV-eye islands share the atlas: radial iris with a glossy pupil.
        for sign in (-1,1):
            x=cfg['eye'];r=cfg['eye_r'];z=shape(name,x,'centre')+shape(name,x,'height')*.40
            y=shape(name,x,'width')*math.sqrt(1-.4**2)
            for pupil in (False,True):
                bpy.ops.mesh.primitive_uv_sphere_add(segments=32,ring_count=20,radius=1)
                eye=bpy.context.object;eye.location=(x*length,sign*(y+.001+(r*.25 if pupil else 0))*length,z*length)
                eye.scale=(r*length*(.56 if pupil else 1),r*length*.38,r*length*(.56 if pupil else 1))
                color=eye.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
                for c in color.data:c.color=(1,1,1,0)
                eye.data.color_attributes.active_color=color
                for p in eye.data.polygons:p.use_smooth=True
                for loop in eye.data.loops:
                    old=eye.data.uv_layers.active.data[loop.index].uv.copy()
                    eye.data.uv_layers.active.data[loop.index].uv=((.76 if pupil else .48)+old.x*.18,.957+old.y*.036)
                # Joining meshes preserves all three channels, including a
                # neutral rig on eyes. Material coordinates remain channel 0.
                for label in ['FinOffset','FinKind']:
                    extra=eye.data.uv_layers.new(name=label)
                    for value in extra.data:value.uv=(0,.5) if label=='FinKind' else (.5,.5)
                parts.append(eye)
        for obj in parts:
            obj.data.uv_layers.active_index=0
            obj.data.uv_layers[0].active_render=True
        bpy.ops.object.select_all(action='DESELECT')
        for obj in parts:obj.select_set(True)
        bpy.context.view_layer.objects.active=fish;bpy.ops.object.join()
        export(fish)
        records.append(dict(name=name,length_m=length,vertices=len(fish.data.vertices),triangles=sum(len(p.vertices)-2 for p in fish.data.polygons),fin_surfaces=fin_count))
    (ROOT/'artifacts/FishMeshBuild.json').write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    def mesh_object(name,vertices,faces,colors):
        mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.update()
        obj=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(obj)
        attr=mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
        for i,value in enumerate(colors):attr.data[i].color=value
        mesh.color_attributes.active_color=attr
        for poly in mesh.polygons:poly.use_smooth=True
        return obj
    def export(obj):
        bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
        bpy.ops.export_scene.fbx(filepath=str(ROOT/'blender/exports'/f'{obj.name}.fbx'),use_selection=True,
            apply_unit_scale=True,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)
    create_fish(mesh_object,export)
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'blender/FishAssets.blend'))
