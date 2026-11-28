"""Original first-person glove, sleeve, wrist computer and torch meshes (metres)."""
import bpy
import math
from mathutils import Vector

def create_equipment(mesh_object, export):
    def combine(name,parts):
        bpy.ops.object.select_all(action='DESELECT')
        for p in parts: p.select_set(True)
        bpy.context.view_layer.objects.active=parts[0]; bpy.ops.object.join()
        obj=parts[0]; obj.name=name
        bpy.context.scene.cursor.location=(0,0,0)
        bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
        export(obj)
    def paint(obj,color):
        attr=obj.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
        for item in attr.data: item.color=(*color,1)
        obj.data.color_attributes.active_color=attr
        for p in obj.data.polygons: p.use_smooth=True
        return obj
    def round_box(location,scale,color,bevel=.006):
        bpy.ops.mesh.primitive_cube_add(size=1,location=location)
        obj=bpy.context.object; obj.scale=scale
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        modifier=obj.modifiers.new('Rounded edges','BEVEL'); modifier.width=bevel; modifier.segments=4
        bpy.ops.object.modifier_apply(modifier=modifier.name)
        return paint(obj,color)
    def ellipsoid(location,scale,color):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=12,radius=1,location=location)
        obj=bpy.context.object; obj.scale=scale
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        return paint(obj,color)
    def tube(points,radii,color,name='Tube',sides=16):
        vertices,faces=[],[]
        for i,p in enumerate(points):
            center=Vector(p)
            tangent=Vector(points[min(i+1,len(points)-1)])-Vector(points[max(0,i-1)])
            right=tangent.normalized().cross(Vector((0,0,1))).normalized()
            if right.length<.1: right=Vector((1,0,0))
            up=tangent.normalized().cross(right).normalized()
            for j in range(sides):
                a=j/sides*2*math.pi
                vertices.append(center+(right*math.cos(a)+up*math.sin(a))*radii[i])
                if i:
                    v=i*sides+j; n=i*sides+(j+1)%sides
                    faces.append((v,v-sides,n-sides,n))
        faces.extend([tuple(reversed(range(sides))),tuple((len(points)-1)*sides+j for j in range(sides))])
        return mesh_object(name,vertices,faces,[(*color,1)]*len(vertices))
    black=(.022,.027,.030); rubber=(.04,.045,.047); grey=(.11,.14,.15)
    for handed in ['Left','Right']:
        sign=1 if handed=='Left' else -1
        vertices,faces,colors=[],[],[]
        for i in range(33):
            t=i/32; x=-.34+t*.31
            wrinkle=1+.025*math.sin(t*76)+.018*math.sin(t*117)
            for j in range(32):
                a=j/32*math.pi*2
                width=(.054-.018*t)*wrinkle; height=(.047-.014*t)*wrinkle
                vertices.append((x,math.cos(a)*width,math.sin(a)*height))
                seam=.55 if j in (7,8,24,25) else 1
                colors.append((.07*seam,.11*seam,.13*seam,1))
                if i:
                    v=i*32+j; n=i*32+(j+1)%32
                    faces.append((v,v-32,n-32,n))
        sleeve=mesh_object('SM_'+handed+'Sleeve',vertices,faces,colors); export(sleeve)
        parts=[ellipsoid((.047,0,-.002),(.064,.043,.024),black),
               tube([(-.044,0,0),(-.028,0,0),(-.018,0,0)],[.039,.039,.036],rubber)]
        for index,(y,length) in enumerate([(.031,.080),(.010,.092),(-.012,.085),(-.031,.065)]):
            pts=[]
            for k in range(9):
                t=k/8
                if handed=='Right':
                    pts.append((.088+length*.52*math.sin(math.pi*t),sign*y*(1-.3*t),-.005-.065*(1-math.cos(math.pi*t))/2))
                else: pts.append((.080+length*t,sign*y,-.005-.022*t*t))
            radii=[(.010-index*.0006)*(1-.25*k/8) for k in range(9)]
            parts.append(tube(pts,radii,black))
            parts.append(ellipsoid(pts[-1],(radii[-1],)*3,black))
            for k in (2,5):
                p=Vector(pts[k])+Vector((0,0,radii[k]*.8))
                parts.append(ellipsoid(p,(.003,.007,.0015),rubber))
        thumb=[(.025,sign*.034,0),(.048,sign*.060,-.009),(.078,sign*.068,-.025),(.102,sign*.050,-.035)]
        if handed=='Right': thumb=[(.025,-.034,0),(.062,-.052,-.028),(.095,-.030,-.057),(.106,-.006,-.061)]
        parts.append(tube(thumb,
                          [.015,.014,.012,.008],black))
        parts.append(ellipsoid((.04,0,.021),(.030,.030,.003),rubber))
        combine('SM_'+handed+'Glove',parts)
    # Local +Z faces the diver when the wrist is raised; long screen axis is Y.
    parts=[round_box((-.005,0,.044),(.078,.105,.022),black,.009)]
    # Four raised bezel sides leave a real recess for the runtime screen.
    for pos,size in [((.031,0,.057),(.008,.089,.006)),((-.041,0,.057),(.008,.089,.006)),
                     ((-.005,-.048,.057),(.064,.009,.006)),((-.005,.048,.057),(.064,.009,.006))]:
        parts.append(round_box(pos,size,grey,.002))
    for y in (-.056,.056):
        for x in (-.022,.014): parts.append(ellipsoid((x,y,.043),(.008,.005,.006),(.24,.29,.28)))
    # Two elastomer straps wrap the forearm instead of a floating watch face.
    for x in (-.026,.017):
        pts=[(x,math.cos(a)*.039,math.sin(a)*.035) for a in [i*2*math.pi/48 for i in range(49)]]
        parts.append(tube(pts,[.006]*49,rubber))
    combine('SM_WristComputer',parts)
    # Torch lies in the right glove, lens pointing +X in the swim direction.
    parts=[tube([(.07,0,-.038),(.095,0,-.038),(.195,0,-.038),(.215,0,-.038)],
                [.020,.023,.025,.027],(.065,.08,.09),sides=32)]
    for i in range(9):
        x=.105+i*.009
        parts.append(tube([(x,0,-.038),(x+.002,0,-.038)],[.024,.024],rubber,sides=24))
    parts.append(tube([(.210,0,-.038),(.223,0,-.038)],[.032,.032],grey,sides=32))
    parts.append(ellipsoid((.224,0,-.038),(.001,.026,.026),(.28,.39,.42)))
    parts.append(round_box((.09,0,-.011),(.018,.012,.007),(.11,.22,.18),.003))
    combine('SM_DiveTorch',parts)
