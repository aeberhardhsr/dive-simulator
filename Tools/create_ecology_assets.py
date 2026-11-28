"""Original parametric freshwater fish and plant meshes. Blender-only helper."""
import math
import random
import bpy
from mathutils import Vector
from mathutils.geometry import tessellate_polygon


from create_fish_assets import create_fish

def create_fine_plants(mesh_object, export):
    rng=random.Random(806)
    for name,height,count in [('Stonewort',.45,12),('Milfoil',.90,7)]:
        verts,faces,colors=[],[],[]
        def stem(start,end,radius,rgb):
            start,end=Vector(start),Vector(end)
            direction=(end-start).normalized()
            right=direction.cross(Vector((0,1,0))).normalized()
            up=direction.cross(right).normalized()
            offset=len(verts)
            for p,w in [(start,radius),(end,radius*.65)]:
                for j in range(4):
                    a=j*math.pi/2
                    verts.append(p+right*math.cos(a)*w+up*math.sin(a)*w)
                    colors.append((*rgb,max(0,min(1,p.z/height))))
            for j in range(4): faces.append((offset+j,offset+(j+1)%4,offset+(j+1)%4+4,offset+j+4))
        for stalk in range(count):
            h=height*rng.uniform(.55,1)
            base=Vector((rng.uniform(-.2,.2),rng.uniform(-.2,.2),0))
            bend=Vector((rng.uniform(-.15,.15),rng.uniform(-.15,.15),0))
            previous=base
            for level in range(1,9):
                t=level/8; p=base+Vector((0,0,h*t))+bend*t*t
                stem(previous,p,.003,(.15,.20,.055)); previous=p
                for j in range(6 if name=='Stonewort' else 4):
                    a=j*math.pi/(3 if name=='Stonewort' else 2)+stalk*.8+level*.5
                    direction=Vector((math.cos(a),math.sin(a),.25))
                    reach=(.09 if name=='Stonewort' else .12)*(1-t*.65)
                    tip=p+direction*reach
                    stem(p,tip,.0018,(.17,.28,.055))
                    if name=='Milfoil':
                        side=Vector((-direction.y,direction.x,.2))
                        for k in range(1,7):
                            q=p+direction*reach*k/7
                            for sign in (-1,1):
                                stem(q,q+side*sign*.028*(1-k/8)+direction*.013,.0009,(.12,.25,.035))
                    else:
                        for sign in (-1,1):
                            stem(tip,tip+Vector((-.012*sign,.018*sign,.025)),.001,(.22,.32,.09))
        export(mesh_object('SM_'+name,verts,faces,colors))
