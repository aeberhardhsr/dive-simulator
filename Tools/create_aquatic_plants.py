"""Blender: original aquatic plant groups with curved blades and branch geometry."""
import bpy
import hashlib
import json
import math
import random
import shutil
import tempfile
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'blender/exports/Plants'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
report=dict(schema=1,revision='curved-shoots',units='metres',authorship='Original procedural game geometry; botanical groups, not species-identification models',assets=[])

class Plant:
    def __init__(self,name,height,seed):
        self.name,self.height,self.rng=name,height,random.Random(seed)
        self.vertices,self.faces,self.uv,self.detail,self.colors=[],[],[],[],[]
    def vertex(self,p,uv,leaf,tint):
        i=len(self.vertices)
        self.vertices.append(tuple(p));self.uv.append(uv);self.detail.append((leaf,tint))
        # Stem and photosynthetic tissue have different colour, with subdued
        # older lower growth. These are authored linear reflectance values.
        green=(.12,.205,.052) if self.name=='Pondweed' else (.105,.175,.044) if self.name=='Milfoil' else (.13,.205,.065)
        stem=(.12,.085,.035) if self.name=='Milfoil' else (.09,.13,.038)
        tissue=max(0,min(1,leaf*1.8))
        rgb=tuple(a+(b-a)*tissue for a,b in zip(stem,green))
        growth=max(0,min(1,p.z/self.height))
        self.colors.append(tuple(c*(.72+.4*tint)*(.8+.2*growth) for c in rgb)+(growth,))
        return i
    def tube(self,points,radius,tint,sides=5,tissue=0):
        points=[Vector(p) for p in points];rings=[]
        for i,p in enumerate(points):
            tangent=(points[min(i+1,len(points)-1)]-points[max(0,i-1)]).normalized()
            axis=Vector((0,0,1)) if abs(tangent.z)<.9 else Vector((0,1,0))
            right=tangent.cross(axis).normalized();up=tangent.cross(right).normalized()
            r=radius*(1-.72*i/(len(points)-1))
            rings.append([self.vertex(p+r*(right*math.cos(j*2*math.pi/sides)+up*math.sin(j*2*math.pi/sides)),
                (j/sides,i/(len(points)-1)),tissue,tint) for j in range(sides)])
        for a,b in zip(rings,rings[1:]):
            for j in range(sides):
                k=(j+1)%sides;self.faces.extend([(a[j],a[k],b[k]),(a[j],b[k],b[j])])
        self.faces.extend([tuple(reversed(rings[0])),tuple(rings[-1])])
    def pinnule(self,root,tip,width,tint):
        # A narrow, curved leaf surface rather than a round twig. Three rows
        # across retain its shallow ridge; the two-sided material shades below.
        first_face=len(self.faces)
        direction=(tip-root).normalized()
        side=direction.cross(Vector((0,0,1))).normalized()
        rows=[]
        for j in range(4):
            v=j/3
            center=root.lerp(tip,v)+Vector((0,0,.004*math.sin(math.pi*v)))
            if j in (0,3):
                rows.append([self.vertex(center,(.5,v),1,tint)])
            else:
                w=width*math.sin(math.pi*v)**.65
                rows.append([self.vertex(center+side*w*s+Vector((0,0,.0004*(1-abs(s)))),
                    ((s+1)/2,v),1,tint) for s in (-1,0,1)])
        for a,b in zip(rows,rows[1:]):
            if len(a)==1:
                for k in range(2):self.faces.append((a[0],b[k],b[k+1]))
            elif len(b)==1:
                for k in range(2):self.faces.append((a[k],b[0],a[k+1]))
            else:
                for k in range(2):self.faces.extend([(a[k],b[k],b[k+1]),(a[k],b[k+1],a[k+1])])
        self.faces[first_face:]=[tuple(reversed(face)) for face in self.faces[first_face:]]
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
        # Curved side shoots must stay inside the habitat's declared height.
        top=max(v[2] for v in self.vertices)
        height_scale=min(1,self.height/top)
        self.vertices=[(x,y,z*height_scale) for x,y,z in self.vertices]
        self.colors=[c[:3]+(max(0,min(1,p[2]/self.height)),) for p,c in zip(self.vertices,self.colors)]
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

for name,height,count,seed in [('Pondweed',1.15,7,4721),('Stonewort',.45,9,4722),('Milfoil',.9,7,4723)]:
    plant=Plant(name,height,seed);r=plant.rng
    for stalk in range(count):
        h=height*r.uniform(.42,.94);base=Vector((r.uniform(-.2,.2),r.uniform(-.2,.2),0))
        bend=Vector((r.uniform(-.26,.26),r.uniform(-.22,.22),0))*(h/height)
        phase=r.uniform(0,6.28)
        def center(t):
            return base+Vector((0,0,h*t))+bend*math.sin(t*math.pi*.65)+Vector((.025*math.sin(t*5+phase),.018*math.cos(t*6+phase),0))*t
        tint=r.random();plant.tube([center(i/16) for i in range(17)],.0028 if name=='Pondweed' else .0017,tint,6)
        # Unequal internodes become closer near the growing tip. Broad leaves
        # and fine whorls retain their distinct topology instead of pine cones.
        nodes=10 if name=='Stonewort' else 12
        levels=sorted(.09+.88*(i/nodes)**.75+r.uniform(-.012,.012) for i in range(1,nodes+1))
        for level,t in enumerate(levels,1):
            p=center(t)
            if name=='Pondweed':
                a=phase+level*2.6+r.uniform(-.25,.25)
                envelope=.65+.35*math.sin(math.pi*t)
                tip_scale=max(.2,min(1,(1-t)/.09))
                plant.leaf(p,a,r.uniform(.12,.21)*envelope*tip_scale,r.uniform(.016,.03)*tip_scale,r.random(),r.uniform(-.3,.65))
            else:
                rays=r.randint(5,8) if name=='Stonewort' else 4
                for j in range(rays):
                    a=phase+j*math.tau/rays+level*.3+r.uniform(-.18,.18)
                    lift=r.uniform(-.12,.5)+.45*t
                    direction=Vector((math.cos(a),math.sin(a),lift));side=Vector((-math.sin(a),math.cos(a),.12))
                    reach=(.095 if name=='Stonewort' else .13)*(.6+.4*math.sin(math.pi*t))*r.uniform(.7,1.2)
                    tip_scale=max(.2,min(1,(1-t)/.08))
                    reach*=tip_scale
                    arch=r.uniform(.006,.025)
                    path=[p+direction*reach*u+Vector((0,0,arch*u*u)) for u in (0,.25,.5,.75,1)]
                    plant.tube(path,.0013 if name=='Milfoil' else .0018,tint,5,tissue=.55)
                    if name=='Milfoil':
                        pairs=6 if t>.88 else 10
                        for k in range(1,pairs+1):
                            u=k/(pairs+1);q=p+direction*reach*u+Vector((0,0,arch*u*u))
                            for sign in (-1,1):
                                length=.045*(1-u*.75)*r.uniform(.8,1.2)*tip_scale
                                tip=q+side*sign*length+direction*(.008+.01*u)
                                plant.pinnule(q,tip,.0018*(1-.45*u),r.random())
                    else:
                        # Short pointed branchlets arise at uneven nodes along
                        # each ray, rather than identical two-pronged tips.
                        for u in (.43,.72):
                            q=p+direction*reach*u+Vector((0,0,arch*u*u))
                            for sign in (-1,1):
                                tip=q+side*sign*r.uniform(.007,.014)+direction*.012+Vector((0,0,r.uniform(.006,.017)))
                                plant.tube([q,tip],.0008,tint,3,tissue=.55)
    plant.finish()
# Save to a fresh temporary path: Blender's rename/backup sequence can fail
# while Windows indexes an existing .blend. Copy the closed result afterwards.
bpy.context.preferences.filepaths.save_version=0
with tempfile.TemporaryDirectory(prefix='dive-plants-') as temporary:
    generated=Path(temporary)/'AquaticPlants.blend'
    bpy.ops.wm.save_as_mainfile(filepath=str(generated))
    shutil.copyfile(generated,ROOT/'blender/AquaticPlants.blend')
(OUT/'Manifest.json').write_text(json.dumps(report,indent=2)+'\n')
print('AQUATIC_PLANT_GEOMETRY_PASS',json.dumps(report))
