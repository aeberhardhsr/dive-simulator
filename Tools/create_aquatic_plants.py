"""Blender: original aquatic plant groups with curved blades and branch geometry."""
import bpy
import hashlib
import json
import math
import random
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'blender/exports/Plants'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
report=dict(schema=1,units='metres',authorship='Original procedural game geometry; botanical groups, not species-identification models',assets=[])

class Plant:
    def __init__(self,name,height,seed):
        self.name,self.height,self.rng=name,height,random.Random(seed)
        self.vertices,self.faces,self.uv,self.detail,self.colors=[],[],[],[],[]
    def vertex(self,p,uv,leaf,tint):
        i=len(self.vertices)
        self.vertices.append(tuple(p));self.uv.append(uv);self.detail.append((leaf,tint))
        rgb=(.19,.255,.075) if self.name=='Pondweed' else (.18,.235,.065) if self.name=='Milfoil' else (.23,.28,.105)
        self.colors.append(tuple(c*(.7+.5*tint) for c in rgb)+(max(0,min(1,p.z/self.height)),))
        return i
    def tube(self,points,radius,tint,sides=5):
        points=[Vector(p) for p in points];rings=[]
        for i,p in enumerate(points):
            tangent=(points[min(i+1,len(points)-1)]-points[max(0,i-1)]).normalized()
            axis=Vector((0,0,1)) if abs(tangent.z)<.9 else Vector((0,1,0))
            right=tangent.cross(axis).normalized();up=tangent.cross(right).normalized()
            r=radius*(1-.35*i/(len(points)-1))
            rings.append([self.vertex(p+r*(right*math.cos(j*2*math.pi/sides)+up*math.sin(j*2*math.pi/sides)),
                (j/sides,i/(len(points)-1)),0,tint) for j in range(sides)])
        for a,b in zip(rings,rings[1:]):
            for j in range(sides):
                k=(j+1)%sides;self.faces.extend([(a[j],a[k],b[k]),(a[j],b[k],b[j])])
        self.faces.extend([tuple(reversed(rings[0])),tuple(rings[-1])])
    def leaf(self,root,angle,length,width,tint,tilt):
        first_face=len(self.faces)
        direction=Vector((math.cos(angle),math.sin(angle),tilt)).normalized()
        side=Vector((-math.sin(angle),math.cos(angle),0));up=side.cross(direction).normalized()
        if up.z<0:up=-up
        rows=[];segments=14
        twist=self.rng.uniform(-.3,.3)
        for j in range(segments+1):
            v=j/segments
            center=root+direction*length*v+up*(.012*math.sin(math.pi*v)-.015*v*v)
            if j in (0,segments):
                rows.append([self.vertex(center,(.5,v),1,tint)])
                continue
            shape=math.sin(math.pi*v)**.7*(1-.25*v)
            row=[]
            for k in range(5):
                u=k/4;s=u*2-1
                ripple=.0015*math.sin(v*math.pi*5+tint*8)*s*s
                p=center+side*(s*width*shape)+up*(.0025*(1-abs(s))*shape+twist*s*width*v+ripple)
                row.append(self.vertex(p,(u,v),1,tint))
            rows.append(row)
        for a,b in zip(rows,rows[1:]):
            if len(a)==1:
                for k in range(4):self.faces.append((a[0],b[k],b[k+1]))
            elif len(b)==1:
                for k in range(4):self.faces.append((a[k],b[0],a[k+1]))
            else:
                for k in range(4):self.faces.extend([(a[k],b[k],b[k+1]),(a[k],b[k+1],a[k+1])])
        # Front normals face the blade's upper surface; the material also
        # shades its reverse side when the diver sees it from below.
        self.faces[first_face:]=[tuple(reversed(face)) for face in self.faces[first_face:]]
    def finish(self):
        mesh=bpy.data.meshes.new('SM_'+self.name)
        mesh.from_pydata(self.vertices,[],self.faces);mesh.update()
        if any(not math.isfinite(c) for p in self.vertices for c in p):raise RuntimeError('Invalid plant coordinates')
        if any(p.area<1e-14 for p in mesh.polygons):raise RuntimeError('Degenerate plant face')
        for name,values in [('LeafUV',self.uv),('SurfaceKind',self.detail)]:
            layer=mesh.uv_layers.new(name=name)
            for loop in mesh.loops:layer.data[loop.index].uv=values[loop.vertex_index]
        colors=mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
        for i,c in enumerate(self.colors):colors.data[i].color=c
        mesh.color_attributes.active_color=colors
        for face in mesh.polygons:face.use_smooth=True
        mesh.calc_loop_triangles()
        obj=bpy.data.objects.new('SM_'+self.name,mesh);bpy.context.collection.objects.link(obj)
        bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
        path=OUT/(obj.name+'.fbx')
        bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,apply_unit_scale=True,
            axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False)
        report['assets'].append(dict(name=obj.name,triangles=len(mesh.loop_triangles),vertices=len(mesh.vertices),
            height_metres=max(v[2] for v in self.vertices),uv_channels=2,root_weight=0,
            file=path.relative_to(ROOT).as_posix(),sha256=hashlib.sha256(path.read_bytes()).hexdigest()))

for name,height,count,seed in [('Pondweed',1.15,7,4721),('Stonewort',.45,12,4722),('Milfoil',.9,7,4723)]:
    plant=Plant(name,height,seed);r=plant.rng
    for stalk in range(count):
        h=height*r.uniform(.52,.94);base=Vector((r.uniform(-.22,.22),r.uniform(-.22,.22),0))
        bend=Vector((r.uniform(-.16,.16),r.uniform(-.16,.16),0));phase=r.uniform(0,6.28)
        def center(t):return base+Vector((0,0,h*t))+bend*t*t+Vector((.015*math.sin(t*3+phase),.012*math.cos(t*4+phase),0))*t
        tint=r.random();plant.tube([center(i/16) for i in range(17)],.0028 if name=='Pondweed' else .0017,tint,6)
        for level in range(1,9):
            t=.12+level*.09;p=center(t)
            if name=='Pondweed':
                a=phase+level*2.6+r.uniform(-.25,.25)
                plant.leaf(p,a,r.uniform(.11,.19)*(1-.35*t),r.uniform(.016,.03),r.random(),r.uniform(-.18,.48))
            else:
                rays=6 if name=='Stonewort' else 4
                for j in range(rays):
                    a=phase+j*math.tau/rays+level*.3
                    direction=Vector((math.cos(a),math.sin(a),.22));side=Vector((-math.sin(a),math.cos(a),.12))
                    reach=(.075 if name=='Stonewort' else .115)*(1-.55*t)*r.uniform(.8,1.15)
                    path=[p+direction*reach*u+Vector((0,0,.016*u*u)) for u in (0,.33,.66,1)]
                    plant.tube(path,.0016,tint,4)
                    if name=='Milfoil':
                        for k in range(1,7):
                            u=k/7;q=p+direction*reach*u+Vector((0,0,.016*u*u))
                            for sign in (-1,1):
                                tip=q+side*sign*(.027*(1-u*.8))+direction*.012
                                plant.tube([q,(q+tip)*.5+Vector((0,0,.002)),tip],.0009,r.random(),3)
                    else:
                        tip=path[-1]
                        for sign in (-1,1):plant.tube([tip,tip+side*sign*.009+Vector((0,0,.015))],.0006,tint,3)
    plant.finish()
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'blender/AquaticPlants.blend'))
(OUT/'Manifest.json').write_text(json.dumps(report,indent=2)+'\n')
print('AQUATIC_PLANT_GEOMETRY_PASS',json.dumps(report))
