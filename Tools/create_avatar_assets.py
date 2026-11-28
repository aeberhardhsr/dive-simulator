"""Original articulated suit and scuba assemblies, in metres. No external assets.

Run in a separate background Blender process; never replace an open user scene.
The scene and all exports have their own AvatarAssets/Avatar directory.
"""
import json
import math
from pathlib import Path
import bpy
import bmesh
from mathutils import Vector
from mathutils.kdtree import KDTree

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'blender'/'exports'/'Avatar'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
BLACK = (.028, .037, .043)
PANEL = (.065, .12, .14)
RUBBER = (.035, .042, .045)
METAL = (.42, .46, .48)
ACCENT = (.10, .38, .34)
manifest = []


def mesh(name, vertices, faces, colors, weights=None):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], faces)
    data.update()
    bm = bmesh.new()
    bm.from_mesh(data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(data)
    bm.free()
    obj = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(obj)
    attr = data.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='POINT')
    for a, color in zip(attr.data, colors):
        a.color = (*color[:3], 1)
    data.color_attributes.active_color = attr
    uv = data.uv_layers.new(name='UVMap')
    for poly in data.polygons:
        poly.use_smooth = True
        for loop in poly.loop_indices:
            co = data.vertices[data.loops[loop].vertex_index].co
            uv.data[loop].uv = (co.x/.65, co.y/.30)
    if weights:
        groups = {n: obj.vertex_groups.new(name=n) for w in weights for n in w if n not in obj.vertex_groups}
        for i, w in enumerate(weights):
            for n, value in w.items():
                if value > 0:
                    groups[n].add([i], value, 'REPLACE')
    return obj


def paint(obj, color):
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    attr = obj.data.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='POINT')
    for a in attr.data:
        a.color = (*color, 1)
    obj.data.color_attributes.active_color = attr
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


def box(pos, size, color=BLACK, bevel=.008):
    bpy.ops.mesh.primitive_cube_add(size=1, location=pos)
    obj = bpy.context.object
    obj.scale = size
    paint(obj, color)
    mod = obj.modifiers.new('Soft construction edge', 'BEVEL')
    mod.width = bevel
    mod.segments = 3
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def ellipsoid(pos, size, color=BLACK):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=20, radius=1, location=pos)
    obj = bpy.context.object
    obj.scale = size
    return paint(obj, color)


def tube(points, radius, color=RUBBER, sides=16):
    vertices, faces = [], []
    for i, point in enumerate(points):
        p = Vector(point)
        direction = (Vector(points[min(i+1, len(points)-1)])-Vector(points[max(0, i-1)])).normalized()
        a = direction.cross(Vector((0, 0, 1)))
        if a.length < .01:
            a = Vector((1, 0, 0))
        a.normalize()
        b = direction.cross(a).normalized()
        r = radius[i] if isinstance(radius, list) else radius
        for j in range(sides):
            angle = 2*math.pi*j/sides
            vertices.append(p+r*(a*math.cos(angle)+b*math.sin(angle)))
            if i:
                v = i*sides+j
                n = i*sides+(j+1)%sides
                faces.append((v, v-sides, n-sides, n))
    faces.extend([tuple(reversed(range(sides))), tuple((len(points)-1)*sides+j for j in range(sides))])
    return mesh('Tube', vertices, faces, [color]*len(vertices))


def curve(points, steps=8):
    result = []
    for i in range(len(points)-1):
        p0, p1, p2, p3 = [Vector(points[max(0, min(len(points)-1, j))]) for j in [i-1, i, i+1, i+2]]
        for k in range(steps):
            t = k/steps
            result.append(.5*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t*t+(-p0+3*p1-3*p2+p3)*t*t*t))
    return result+[Vector(points[-1])]


def strap(points, width=.05):
    points = curve(points)
    vertices, faces = [], []
    for i, p in enumerate(points):
        tangent = (points[min(i+1, len(points)-1)]-points[max(0, i-1)]).normalized()
        across = Vector((0, 1, 0))
        normal = tangent.cross(across).normalized()
        for a, b in [(-1, -1), (-1, 1), (1, 1), (1, -1)]:
            vertices.append(p+across*a*width/2+normal*b*.0025)
        if i:
            for j in range(4):
                v = i*4+j
                n = i*4+(j+1)%4
                faces.append((v, n, n-4, v-4))
    faces += [(3, 2, 1, 0), tuple((len(points)-1)*4+j for j in range(4))]
    return mesh('Webbing', vertices, faces, [BLACK]*len(vertices))


def export(obj, rig=None):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    if rig:
        rig.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=str(OUT/(obj.name+'.fbx')), use_selection=True,
        apply_unit_scale=True, axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE',
        add_leaf_bones=False, bake_anim=False, use_armature_deform_only=True)
    manifest.append({'name': obj.name, 'vertices': len(obj.data.vertices),
        'triangles': sum(len(p.vertices)-2 for p in obj.data.polygons), 'skeletal': bool(rig)})


def combine(name, parts):
    bpy.ops.object.select_all(action='DESELECT')
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = parts[0]
    obj.name = name
    bpy.context.scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    export(obj)
    return obj


# Rest pose: standing, +X forward / +Y right / +Z up. Eyes at (8,0,170) cm.
bones = [('root', (0, 0, 0), (0, 0, .20), None),
         ('pelvis', (0, 0, .98), (0, 0, 1.12), 'root'),
         ('spine', (0, 0, 1.12), (0, 0, 1.32), 'pelvis'),
         ('chest', (0, 0, 1.32), (0, 0, 1.48), 'spine')]
for suffix, sign in [('l', -1), ('r', 1)]:
    bones += [(f'upper_{suffix}', (0, sign*.235, 1.43), (0, sign*.28, 1.113), 'chest'),
              (f'lower_{suffix}', (0, sign*.28, 1.113), (0, sign*.30, .804), f'upper_{suffix}'),
              (f'thigh_{suffix}', (0, sign*.105, .98), (0, sign*.11, .52), 'pelvis'),
              (f'shin_{suffix}', (0, sign*.11, .52), (0, sign*.11, .10), f'thigh_{suffix}')]
armature = bpy.data.armatures.new('DiverRig')
rig = bpy.data.objects.new('DiverRig', armature)
bpy.context.collection.objects.link(rig)
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
for name, head, tail, parent in bones:
    bone = armature.edit_bones.new(name)
    bone.head, bone.tail = head, tail
    if parent:
        bone.parent = armature.edit_bones[parent]
bpy.ops.object.mode_set(mode='OBJECT')
vertices, faces, colors, weights = [], [], [], []


def suit_rings(rings, get_weight, sides=48):
    start = len(vertices)
    for i, (center, rx, ry) in enumerate(rings):
        for j in range(sides):
            a = 2*math.pi*j/sides
            ripple = 1+.012*math.sin(i*1.7)*math.cos(a*5)
            vertices.append((center[0]+rx*math.cos(a)*ripple, center[1]+ry*math.sin(a)*ripple, center[2]))
            seam = j in (11, 12, 35, 36)
            c = PANEL if abs(math.sin(a)) > .72 else BLACK
            colors.append(tuple(v*(.48 if seam else 1) for v in c))
            weights.append(get_weight(i/(len(rings)-1), center[2]))
            if i:
                v = start+i*sides+j
                n = start+i*sides+(j+1)%sides
                faces.append((v, n, n-sides, v-sides))
    faces.extend([tuple(start+j for j in reversed(range(sides))),
                  tuple(start+(len(rings)-1)*sides+j for j in range(sides))])


def blend(a, b, t):
    t = max(0, min(1, t))
    t = t*t*(3-2*t)
    return {a: 1-t, b: t}


# Anatomical silhouette stations: pelvis, waist, ribs, shoulders, collar.
stations = [(.88, .10, .15), (.96, .115, .18), (1.04, .105, .172), (1.14, .097, .15),
            (1.25, .117, .18), (1.36, .132, .22), (1.43, .12, .235), (1.49, .09, .15), (1.54, .072, .075)]
rings = []
for left, right in zip(stations, stations[1:]):
    for k in range(5):
        t = k/5
        z, rx, ry = [a+(b-a)*t for a, b in zip(left, right)]
        rings.append(((0, 0, z), rx, ry))
rings.append(((0, 0, stations[-1][0]), *stations[-1][1:]))
suit_rings(rings, lambda t, z: blend('pelvis', 'spine', (z-1.03)/.15) if z < 1.24 else blend('spine', 'chest', (z-1.24)/.16))
for suffix, sign in [('l', -1), ('r', 1)]:
    rings = []
    for i in range(49):
        t = i/48
        z = .98-.88*t
        radius = .095*(1-t)+.042*t+.014*math.sin(math.pi*min(1, t*1.7))
        if .43 < t < .62:
            radius *= .90
        rings.append(((.006*math.sin(t*math.pi), sign*(.105+.005*t), z), radius*.92, radius))
    suit_rings(rings, lambda t, z, side=suffix: blend('thigh_'+side, 'shin_'+side, (.60-z)/.16))
    rings = []
    for i in range(41):
        t = i/40
        z = 1.44-.64*t
        y = sign*(.235+.065*t)
        radius = .066*(1-t)+.034*t+.007*math.sin(math.pi*t)
        rings.append(((0, y, z), radius, radius))
    suit_rings(rings, lambda t, z, side=suffix: blend('upper_'+side, 'lower_'+side, (1.18-z)/.14), 40)
body = mesh('SK_DiverSuit', vertices, faces, colors, weights)
# Fuse shoulder caps and intersecting suit panels into one continuous skin.
# Reproject authored weights/colors after voxel union; never use automatic
# envelope weights that can link the two close legs or arms to the wrong side.
caps = []
for suffix, sign in [('l', -1), ('r', 1)]:
    cap = ellipsoid((0, sign*.218, 1.43), (.088, .083, .085), BLACK)
    cap.vertex_groups.new(name='upper_'+suffix).add(list(range(len(cap.data.vertices))), .5, 'REPLACE')
    cap.vertex_groups.new(name='chest').add(list(range(len(cap.data.vertices))), .5, 'REPLACE')
    caps.append(cap)
bpy.ops.object.select_all(action='DESELECT')
body.select_set(True)
for cap in caps:
    cap.select_set(True)
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()
source_positions = [v.co.copy() for v in body.data.vertices]
source_colors = [a.color[:3] for a in body.data.color_attributes['Color'].data]
source_weights = [{body.vertex_groups[g.group].name: g.weight for g in v.groups} for v in body.data.vertices]
tree = KDTree(len(source_positions))
for i, p in enumerate(source_positions):
    tree.insert(p, i)
tree.balance()
remesh = body.modifiers.new('Continuous tailored suit', 'REMESH')
remesh.mode = 'VOXEL'
remesh.voxel_size = .008
remesh.use_smooth_shade = True
bpy.ops.object.modifier_apply(modifier=remesh.name)
smooth = body.modifiers.new('Relax panel transitions', 'SMOOTH')
smooth.factor, smooth.iterations = .65, 5
bpy.ops.object.modifier_apply(modifier=smooth.name)
decimate = body.modifiers.new('Retain silhouette with bounded topology', 'DECIMATE')
decimate.ratio = .65
bpy.ops.object.modifier_apply(modifier=decimate.name)
body.vertex_groups.clear()
groups = {n: body.vertex_groups.new(name=n) for n, *_ in bones}
for attr in list(body.data.color_attributes):
    body.data.color_attributes.remove(attr)
attribute = body.data.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='POINT')
for vertex in body.data.vertices:
    nearest = tree.find_n(vertex.co, 4)
    norm = sum(1/max(.002, distance)**2 for co, index, distance in nearest)
    mixed, color = {}, Vector((0, 0, 0))
    for co, index, distance in nearest:
        factor = 1/max(.002, distance)**2/norm
        color += Vector(source_colors[index])*factor
        for name, weight in source_weights[index].items():
            mixed[name] = mixed.get(name, 0)+weight*factor
    for name, weight in mixed.items():
        groups[name].add([vertex.index], weight, 'REPLACE')
    attribute.data[vertex.index].color = (*color, 1)
body.data.color_attributes.active_color = attribute
uv = body.data.uv_layers.new(name='UVMap')
for polygon in body.data.polygons:
    for loop in polygon.loop_indices:
        co = body.data.vertices[body.data.loops[loop].vertex_index].co
        uv.data[loop].uv = (co.x*3+co.y*.7, co.z*2)
for polygon in body.data.polygons:
    polygon.use_smooth = True
body.parent = rig
modifier = body.modifiers.new('Articulated suit', 'ARMATURE')
modifier.object = rig
export(body, rig)
wet = body.copy()
wet.data = body.data.copy()
wet.name = 'SK_DiverWet'
bpy.context.collection.objects.link(wet)
for vertex in wet.data.vertices:
    dominant = max(vertex.groups, key=lambda g: g.weight)
    name = wet.vertex_groups[dominant.group].name
    center_y = 0
    if name.startswith(('upper_', 'lower_')):
        center_y = (-1 if name.endswith('_l') else 1)*(.235+.065*max(0, min(1, (1.44-vertex.co.z)/.64)))
    elif name.startswith(('thigh_', 'shin_')):
        center_y = -.11 if name.endswith('_l') else .11
    vertex.co.x *= .94
    vertex.co.y = center_y+(vertex.co.y-center_y)*.94
export(wet, rig)

# Independent head/hood allows first-person eye clearance and a complete shadow.
parts = [ellipsoid((0, 0, 1.63), (.092, .083, .125), BLACK),
         tube([(0, 0, 1.49), (0, 0, 1.56)], [.074, .07], BLACK),
         ellipsoid((.079, 0, 1.63), (.025, .060, .079), (.38, .24, .16))]
combine('SM_DiverHood', parts)
parts = [box((.087, 0, 1.665), (.042, .155, .078), RUBBER, .018)]
parts.append(ellipsoid((.106, 0, 1.627), (.024, .018, .030), RUBBER))
for y in [-.038, .038]:
    parts.append(box((.11, y, 1.67), (.006, .063, .051), (.14, .25, .27), .009))
points = [(.015+math.cos(a)*.075, math.sin(a)*.09, 1.67) for a in [i*2*math.pi/64 for i in range(65)]]
parts.append(tube(points, .008, RUBBER))
combine('SM_DiveMask', parts)
parts = [ellipsoid((.115, 0, 1.585), (.035, .046, .027), RUBBER),
         box((.143, 0, 1.59), (.008, .051, .033), METAL, .01)]
for y in [-.045, .045]:
    parts.append(tube([(.11, y*.5, 1.57), (.09, y, 1.56), (.065, y*1.5, 1.57)], .009))
combine('SM_SecondStage', parts)

# Boots and flexible fin geometry use an ankle-local origin.
parts = [ellipsoid((.065, 0, -.025), (.145, .051, .052), RUBBER),
         tube([(0, 0, -.01), (0, 0, .09)], [.052, .045], BLACK),
         box((.065, 0, -.065), (.25, .105, .018), RUBBER, .012)]
combine('SM_DiveBoot', parts)
verts, polys, cols = [], [], []
for layer in [-1, 1]:
    for i in range(41):
        t = i/40
        x = .12+.52*t
        width = .058+.075*math.sin(t*math.pi*.65)
        for j in range(17):
            v = j/8-1
            z = -.05-.025*t+layer*.003+.012*math.sin(v*math.pi/2)**2
            verts.append((x, width*v, z))
            cols.append(ACCENT if abs(v) > .80 or abs(v) < .10 else RUBBER)
            if i and j:
                a = len(verts)-1
                quad = (a, a-1, a-18, a-17)
                polys.append(quad if layer > 0 else tuple(reversed(quad)))
n = 41*17
for i in range(40):
    for edge in [0, 16]:
        a = i*17+edge
        polys.append((a, a+17, a+17+n, a+n))
for j in range(16):
    for row in [0, 40]:
        a = row*17+j
        polys.append((a, a+n, a+n+1, a+1))
fin = mesh('SM_DiveFin', verts, polys, cols)
parts = [fin, tube(curve([(.12, -.058, -.01), (-.05, -.055, .015), (-.075, 0, .02),
                          (-.05, .055, .015), (.12, .058, -.01)]), .009, RUBBER)]
# Foot-pocket roof with open heel. The boot remains a separate part.
verts, polys = [], []
for i in range(12):
    x = -.005+i*.021
    for j in range(17):
        a = math.pi*j/16
        verts.append((x, .059*math.cos(a), -.025+.052*math.sin(a)))
        if i and j:
            v = len(verts)-1
            polys.append((v, v-1, v-18, v-17))
parts.append(mesh('FootPocket', verts, polys, [RUBBER]*len(verts)))
combine('SM_DiveFin', parts)

for material, radius in [('Steel', .092), ('Aluminium', .103)]:
    for litres in [10, 12, 15]:
        mass = 1.15*litres+1 if material == 'Steel' else 1.35*litres+.8
        volume = litres+(mass-.8)/(7.8 if material == 'Steel' else 2.7)
        length = volume/1000/(math.pi*radius**2)+2*radius/3
        profile = [(-length/2, .2), (-length/2+.015, .75), (-length/2+.04, .99),
                   (length/2-.07, 1), (length/2-.035, .85), (length/2-.01, .35), (length/2, .25)]
        points = [(0, 0, z) for z, r in profile]
        color = (.73, .77, .75) if material == 'Steel' else (.43, .49, .51)
        parts = [tube(points, [r*radius for z, r in profile], color, 64)]
        # A molded base boot, two mounting bands and valve/first stage.
        parts.append(tube([(0, 0, -length/2-.005), (0, 0, -length/2+.045)], radius*1.015, RUBBER, 64))
        for z in [-.13, .13]:
            parts.append(tube([(0, 0, z-.018), (0, 0, z+.018)], radius*1.025, BLACK, 48))
        parts += [box((0, 0, length/2+.027), (.055, .045, .06), METAL, .006),
                  tube([(0, -.035, length/2+.027), (0, -.065, length/2+.027)], .021, BLACK, 24),
                  ellipsoid((.015, .02, length/2+.047), (.034, .025, .026), METAL)]
        combine(f'SM_Cylinder{material}{litres}', parts)

# BCDs are authored relative to the torso, not floating icons.
for name in ['Jacket', 'Travel', 'Wing']:
    parts = [box((-.13, 0, 1.25), (.045, .26, .40), RUBBER, .022)]
    for sign in [-1, 1]:
        pts = [(-.14, sign*.10, 1.07), (-.14, sign*.12, 1.43), (-.02, sign*.15, 1.49),
               (.105, sign*.15, 1.40), (.12, sign*.13, 1.15), (.06, sign*.16, 1.04)]
        parts.append(strap(pts, .05 if name != 'Travel' else .04))
        parts.append(box((.132, sign*.14, 1.25), (.026, .052, .065), METAL, .005))
        if name == 'Jacket':
            parts.append(ellipsoid((-.015, sign*.19, 1.19), (.15, .065, .22), BLACK))
            parts.append(box((.08, sign*.20, 1.11), (.07, .085, .125), PANEL, .022))
        elif name == 'Wing':
            parts.append(ellipsoid((-.18, sign*.17, 1.25), (.083, .09, .245), ACCENT))
        else:
            parts.append(ellipsoid((-.17, sign*.125, 1.27), (.065, .06, .19), PANEL))
    if name == 'Wing':
        for z in [1.045, 1.455]:
            parts.append(tube(curve([(-.18, -.17, z), (-.20, -.08, z), (-.20, .08, z), (-.18, .17, z)]), .058, ACCENT, 24))
    parts.append(box((.115, 0, 1.045), (.025, .34, .052), BLACK, .005))
    parts.append(box((.135, .012, 1.045), (.016, .055, .062), METAL, .004))
    # Corrugated inflator travels over the left shoulder; controls at chest.
    points = [(-.14, -.13, 1.42), (-.03, -.20, 1.49), (.11, -.20, 1.40), (.16, -.18, 1.22)]
    parts.append(tube(curve(points), .013, RUBBER))
    for i in range(16):
        t = i/15
        parts.append(ellipsoid((.11+.05*t, -.20+.02*t, 1.40-.18*t), (.016, .016, .004), BLACK))
    parts.append(box((.16, -.18, 1.20), (.037, .035, .065), RUBBER, .008))
    parts.append(ellipsoid((.18, -.18, 1.22), (.009, .012, .008), (.18, .48, .36)))
    combine('SM_BCD'+name, parts)

parts = []
for sign in [-1, 1]:
    for x in [-.075, .03, .115]:
        parts.append(box((x, sign*.166, .99), (.073, .052, .075), (.18, .20, .21), .006))
combine('SM_WeightPockets', parts)
parts = [tube(curve([(-.23, .045, 1.57), (-.12, .23, 1.48), (.03, .29, 1.40),
               (.19, .21, 1.43), (.21, .08, 1.57), (.13, .045, 1.585)]), .008, BLACK, 20)]
combine('SM_RegulatorHose', parts)
# Small chest-mounted compass with a recessed dial.
parts = [ellipsoid((.14, .085, 1.31), (.018, .034, .034), BLACK),
         ellipsoid((.157, .085, 1.31), (.002, .027, .027), (.13, .20, .19)),
         box((.16, .085, 1.31), (.003, .004, .043), (.75, .32, .15), .001)]
combine('SM_DiveCompass', parts)

bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'blender'/'AvatarAssets.blend'))
(OUT/'Manifest.json').write_text(json.dumps({'assets': manifest, 'bones': bones,
    'coordinate_system': 'metres, +X forward, +Y right, +Z up',
    'license': 'Original project-authored geometry; same license as repository'}, indent=2)+'\n', encoding='utf-8')
print('DIVE_AVATAR_ASSETS_OK', len(manifest), 'assets', sum(a['triangles'] for a in manifest), 'triangles')
