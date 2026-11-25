"""Original parametric freshwater fish and plant meshes. Blender-only helper."""
import math
import random
import bpy
from mathutils import Vector
from mathutils.geometry import tessellate_polygon


def create_fish(mesh_object, export):
    # Tail is -X, head +X. Each profile gives half-width/height along the body.
    profiles = {
        'Perch': (.30, [.018,.034,.060,.105,.125,.110,.065,.012],
                  [.025,.050,.120,.210,.240,.200,.115,.018]),
        'Roach': (.24, [.015,.030,.055,.095,.115,.100,.060,.010],
                  [.025,.060,.135,.235,.260,.200,.105,.014]),
        'Pike': (.75, [.015,.035,.045,.054,.060,.068,.057,.035],
                 [.030,.065,.078,.088,.095,.095,.055,.023]),
        'Whitefish': (.38, [.015,.028,.050,.085,.095,.082,.045,.010],
                      [.025,.045,.100,.160,.180,.160,.085,.012]),
        'Burbot': (.55, [.015,.035,.060,.085,.110,.140,.130,.055],
                   [.025,.045,.065,.080,.100,.110,.085,.035])}
    for name, (length, widths, heights) in profiles.items():
        verts, faces, colors = [], [], []
        tail=-.37*length
        nose=.50*length
        def weight(x): return max(0, min(1, (.18*length-x)/(.65*length)))**1.5
        def body_color(t, a):
            top=(math.sin(a)+1)/2
            if name=='Perch':
                stripe=.40 if math.cos(t*math.pi*13)>.72 and top>.18 else 1
                c=((.52-.39*top)*stripe,(.57-.35*top)*stripe,(.32-.23*top)*stripe)
            elif name=='Pike':
                spots=1 if math.sin(t*71+a*7)*math.sin(t*37-a*9)>.57 else 0
                c=(.40-.29*top+spots*.20,.46-.28*top+spots*.20,.26-.19*top+spots*.09)
            elif name=='Burbot':
                mottled=.5+.5*math.sin(t*55+math.sin(a*5)*2)*math.cos(a*6-t*37)
                c=((.45-.25*top)*(.5+mottled*.5),(.37-.23*top)*(.5+mottled*.5),(.20-.14*top)*(.5+mottled*.5))
            else:
                c=(.65-.50*top,.70-.49*top,.67-.42*top)
                if name=='Roach': c=(c[0]*.88,c[1]*.94,c[2]*.80)
            # Small alternating scale highlights, baked colours, no imported textures.
            scale=1+.045*math.sin(t*320+round(a*24)*1.7)
            return tuple(v*scale for v in c)
        rings, sides = 57, 40
        for r in range(rings):
            t=r/(rings-1); f=t*(len(widths)-1); k=min(int(f),len(widths)-2); u=f-k
            width=(widths[k]*(1-u)+widths[k+1]*u)*length
            height=(heights[k]*(1-u)+heights[k+1]*u)*length
            x=tail+t*(nose-tail)
            for s in range(sides):
                a=s/sides*2*math.pi
                verts.append((x,math.cos(a)*width,math.sin(a)*height))
                colors.append((*body_color(t,a),weight(x)))
                if r<rings-1:
                    v=r*sides+s; n=r*sides+(s+1)%sides
                    faces.append((v,n,n+sides,v+sides))
        faces += [tuple(reversed(range(sides))),tuple((rings-1)*sides+s for s in range(sides))]
        def fin(points, rgb):
            # Ear clipping preserves the concave tail fork and serrated dorsal edge.
            start=len(verts)
            for j,p in enumerate(points):
                verts.append(p); shade=.72 if j%2 else 1
                colors.append((rgb[0]*shade,rgb[1]*shade,rgb[2]*shade,weight(p[0])))
            vectors=[Vector(p) for p in points]
            for triangle in tessellate_polygon([vectors]):
                faces.append(tuple(start+(p if isinstance(p,int) else vectors.index(p)) for p in triangle))
        def body_height(x):
            f=max(0,min(len(heights)-1,(x+.37)/.87*(len(heights)-1)))
            k=min(int(f),len(heights)-2); u=f-k
            return heights[k]*(1-u)+heights[k+1]*u
        def dorsal(start,end,extra,sign=1,spines=False):
            roots=[]; tips=[]
            for j in range(17):
                t=j/16; x=start+(end-start)*t; h=body_height(x)
                roots.append((x*length,0,sign*(h-.008)*length))
                rise=extra*math.sin(math.pi*t)**.65*(.72 if spines and j%2 else 1)
                tips.append((x*length,0,sign*(h+rise+.002)*length))
            fin(roots+list(reversed(tips)),grey if sign==1 else fincol)
        orange=(.53,.17,.035); grey=(.25,.29,.22); brown=(.29,.23,.10)
        fincol=orange if name in ('Perch','Roach') else brown if name=='Burbot' else grey
        if name=='Burbot':
            fin([(tail,0,-.025*length)]+[(-.39*length-.11*length*math.cos(a),0,.08*length*math.sin(a))
                for a in [-math.pi/2+j*math.pi/12 for j in range(13)]]+[(tail,0,.025*length)],fincol)
            dorsal(-.35,.05,.055)
            dorsal(.10,.22,.045)
            dorsal(-.35,.06,.05,-1)
        else:
            fin([(tail,0,0),(-.50*length,0,.17*length),(-.44*length,0,.02*length),
                 (-.50*length,0,-.17*length)],fincol)
            if name=='Pike': dorsal(-.32,-.10,.11)
            elif name=='Perch':
                dorsal(-.04,.24,.10,spines=True)
                dorsal(-.25,-.06,.09)
            else: dorsal(-.08,.13,.14)
            dorsal(-.30,-.08,.10,-1)
            if name=='Whitefish': # Small adipose fin behind the dorsal.
                fin([(-.28*length,0,.045*length),(-.26*length,0,.10*length),(-.19*length,0,.08*length)],grey)
        for sign in (-1,1):
            fin([(.21*length,sign*.07*length,-.015*length),
                 (.02*length,sign*.24*length,-.11*length),(.08*length,sign*.07*length,-.08*length)],fincol)
            fin([(-.02*length,sign*.045*length,-.09*length),
                 (-.14*length,sign*.105*length,-.19*length),(-.12*length,sign*.03*length,-.10*length)],fincol)
        fish=mesh_object('SM_'+name,verts,faces,colors)
        parts=[fish]
        def ellipsoid(location, scale, rgb):
            bpy.ops.mesh.primitive_uv_sphere_add(segments=16,ring_count=10,radius=1,location=location)
            obj=bpy.context.object; obj.scale=scale
            attr=obj.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
            for item in attr.data: item.color=(*rgb,0)
            obj.data.color_attributes.active_color=attr
            parts.append(obj)
        eye_x=.33 if name!='Pike' else .28
        eye_y=.075 if name not in ('Pike','Burbot') else .064 if name=='Pike' else .12
        eye_z=.057 if name!='Pike' else .045
        for sign in (-1,1):
            iris=(.65,.11,.025) if name=='Roach' else (.53,.45,.18)
            for radius,offset,rgb in [(.032,0,iris),(.018,.018,(.006,.009,.008))]:
                ellipsoid((eye_x*length,sign*(eye_y+offset)*length,eye_z*length),
                          (radius*length,radius*.55*length,radius*length),rgb)
            # Operculum line and mouth slit are geometry rather than a painted smile.
            ellipsoid((.235*length,sign*.091*length,.0),(.006*length,.006*length,.082*length),(.07,.09,.055))
            ellipsoid((.447*length,sign*.028*length,-.013*length),(.048*length,.004*length,.004*length),(.05,.06,.035))
        if name=='Burbot':
            ellipsoid((.43*length,0,-.092*length),(.007*length,.007*length,.065*length),(.48,.40,.22))
        bpy.ops.object.select_all(action='DESELECT')
        for obj in parts: obj.select_set(True)
        bpy.context.view_layer.objects.active=fish; bpy.ops.object.join()
        export(fish)


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
