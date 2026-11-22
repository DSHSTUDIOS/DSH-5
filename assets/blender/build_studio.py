from pathlib import Path
import os
import bpy
from math import radians, pi, sin, cos
from mathutils import Matrix, Vector

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
output = Path(__file__).resolve().parent
mesh_dir = output.parent.parent / "cad" / "meshes"
draft = os.environ.get("DSH_PREVIEW") == "1"

def material(name, color, roughness, metal=0, emission=0, grain=False):
    result = bpy.data.materials.new(name)
    result.diffuse_color = (*color,1)
    result.use_nodes = True
    nodes = result.node_tree.nodes
    surface = nodes.get("Principled BSDF")
    surface.inputs["Base Color"].default_value = (*color,1)
    surface.inputs["Roughness"].default_value = roughness
    surface.inputs["Metallic"].default_value = metal
    if emission:
        surface.inputs["Emission Color"].default_value = (*color,1)
        surface.inputs["Emission Strength"].default_value = emission
    if grain:
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = 210
        noise.inputs["Detail"].default_value = 1.7
        uv = nodes.new("ShaderNodeTexCoord")
        stretch = nodes.new("ShaderNodeVectorMath")
        stretch.operation = "MULTIPLY"
        stretch.inputs[1].default_value = (.16,2.4,1.0)
        result.node_tree.links.new(uv.outputs["Generated"],stretch.inputs[0])
        result.node_tree.links.new(stretch.outputs["Vector"],noise.inputs["Vector"])
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = .21
        bump.inputs["Distance"].default_value = .012
        result.node_tree.links.new(noise.outputs["Fac"],bump.inputs["Height"])
        result.node_tree.links.new(bump.outputs["Normal"],surface.inputs["Normal"])
    return result

silver = material("Brushed silver aluminum",(.54,.55,.57),.36,.59,grain=True)
navy = material("Midnight blue frame",(.037,.061,.089),.34,.38)
back = material("Graphite rear cover",(.065,.084,.108),.36,.28,grain=True)
rubber = material("Rubber",(.017,.020,.025),.58)
knobs = material("Molded graphite",(.043,.050,.057),.39,.13)
record = material("Vermilion",(.73,.095,.019),.34,.16)
glass = material("OLED glass",(.009,.025,.030),.26,.12)
oled_ink = material("Blue-green OLED emission",(.24,.83,.72),.32,emission=.62)
logo = material("Warm silver inlay",(.64,.63,.57),.42,.27)
ink = material("Panel markings",(.033,.037,.041),.62)
face_text = material("Warm white type",(.93,.90,.84),.5,emission=.67)
sub_text = material("Warm gray type",(.63,.61,.56),.55,emission=.48)

base = {
    "frame":navy, "front":silver, "back":back, "shroud":knobs, "logo":logo,
    "record":record, "mix":knobs, "play":knobs, "rocker":knobs,
    "platter":knobs, "wheel-mark":record, "fader-cap":knobs, "fader-mark":record,
    "knob":knobs, "knob-mark":record, "oled":glass, "usb":silver,
    **{f"foot-{i}":rubber for i in range(1,5)},
}

font_path = Path("/System/Library/Fonts/Supplemental/Arial.ttf")
mono_path = Path("/System/Library/Fonts/Supplemental/Courier New.ttf")
font = bpy.data.fonts.load(str(font_path)) if font_path.exists() else bpy.data.fonts.get("Bfont")
mono = bpy.data.fonts.load(str(mono_path)) if mono_path.exists() else font

def text(name, message, size, mat, pose, *, tracking=1.0, family=None, centered=False):
    curve = bpy.data.curves.new(name, "FONT")
    curve.body = message
    curve.size = size
    curve.space_character = tracking
    curve.font = family or font
    curve.align_x = "CENTER" if centered else "LEFT"
    curve.materials.append(mat)
    obj = bpy.data.objects.new(name,curve)
    scene.collection.objects.link(obj)
    obj.matrix_world = pose
    return obj

def cube(name, dimensions, transform, position, material):
    bpy.ops.mesh.primitive_cube_add(size=1.0)
    obj=bpy.context.object
    obj.name=name
    obj.matrix_world=(transform @ Matrix.Translation(position) @
                      Matrix.Diagonal((*dimensions,1)))
    obj.data.materials.append(material)
    return obj

def cylinder(name, diameter, height, transform, position, material, axial=None):
    bpy.ops.mesh.primitive_cylinder_add(vertices=40,radius=diameter/2,depth=height)
    obj=bpy.context.object
    obj.name=name
    obj.matrix_world=transform @ Matrix.Translation(position) @ (axial or Matrix.Identity(4))
    obj.data.materials.append(material)
    return obj

def transform(x, y, yaw, rear=False):
    front_to_vertical=Matrix.Rotation(pi/2,4,"X")
    show_rear=Matrix.Rotation(pi if rear else 0,4,"Y")
    recenter=Matrix.Translation((-80,127.5,11))
    return (Matrix.Translation((x,y,9.357)) @ Matrix.Rotation(radians(yaw),4,"Z") @
            front_to_vertical @ show_rear @ Matrix.Scale(.1,4) @ recenter)

hero = transform(-.42,-.12,-14)
rear = transform(11.67,.63,-13,True)
for stem,mat in base.items():
    bpy.ops.wm.stl_import(filepath=str(mesh_dir/f"{stem}.stl"))
    ob=bpy.context.object
    ob.name=f"Front / {stem}"
    ob.data.materials.append(mat)
    ob.matrix_world=hero
    duplicate=ob.copy()
    duplicate.data=ob.data
    duplicate.name=f"Rear / {stem}"
    scene.collection.objects.link(duplicate)
    duplicate.matrix_world=rear

for x,y in ((36.5,-40.2),(123.5,-40.2),(36.5,-214.8),(123.5,-214.8)):
    cylinder("Flush face screw",4.28,.29,hero,(x,y,12.865),silver)
    cube("Screw slot",(2.05,.37,.035),hero,(x,y,13.026),ink)
for x,y in ((36.5,-40.2),(123.5,-40.2),(36.5,-214.8),(123.5,-214.8)):
    cylinder("Rear fastener",4.2,.26,rear,(x,y,-34.86),rubber)

text("Front brand","DSH-5",5.5,ink,hero @ Matrix.Translation((43,-51.3,13.022)),tracking=1.06)
text("Scratch deck marking","SCRATCH DECK",2.16,ink,hero @ Matrix.Translation((43.1,-56.6,13.022)),family=mono)
text("Crossfade marking","CROSSFADE",2.65,ink,hero @ Matrix.Translation((74.75,-180.2,13.022)),family=mono,centered=True)
text("Volume marking","LEVEL",2.65,ink,hero @ Matrix.Translation((110,-178.,13.022)),family=mono,centered=True)
text("Backing marking","A",2.35,ink,hero @ Matrix.Translation((58.8,-180.1,13.022)),family=mono,centered=True)
text("Scratch marking","B",2.35,ink,hero @ Matrix.Translation((90.7,-180.1,13.022)),family=mono,centered=True)
for i in range(11):
    cube("Fader graduation",(.24,1.20 if i in (0,5,10) else .68,.022),hero,
         (59.75+i*3,-172.1,13.016),ink)

text("OLED name","DSH-5",3.05,oled_ink,hero @ Matrix.Translation((92.45,-56.0,8.664)),family=mono)
text("OLED playback","PLAY   SCRATCH",1.56,oled_ink,hero @ Matrix.Translation((92.53,-58.67,8.664)),family=mono)
text("OLED speed","SPEED 100%",1.56,oled_ink,hero @ Matrix.Translation((92.53,-61.06,8.664)),family=mono)
text("OLED battery","11.7V",1.44,oled_ink,hero @ Matrix.Translation((92.53,-62.99,8.664)),family=mono)
# Mark the outer face of the speed lever and the two outer end pads.
side_text = Matrix.Rotation(-pi/2,4,"Y") @ Matrix.Rotation(pi/2,4,"Z")
for y,mark in ((-88.5,"+"),(-106.0,"−")):
    text("Speed rocker direction",mark,2.0,face_text,
         rear @ Matrix.Translation((31.00,y,4.4)) @ side_text,centered=True)

# Three-dimensional studio with a large neutral ground and broad area lights.
world=bpy.data.worlds.new("Dim studio")
world.use_nodes=True
world.node_tree.nodes["Background"].inputs[0].default_value=(.115,.132,.161,1)
world.node_tree.nodes["Background"].inputs[1].default_value=.25
scene.world=world
bpy.ops.mesh.primitive_plane_add(size=360,location=(0,0,0))
bpy.context.object.name="Studio floor"
bpy.context.object.data.materials.append(material("Warm gray studio floor",(.145,.139,.134),.62))
def aim(obj,target):
    obj.rotation_euler=(Vector(target)-obj.location).to_track_quat("-Z","Y").to_euler()
def light(name,power,position,target,size,color,size_y=None):
    data=bpy.data.lights.new(name,"AREA")
    data.energy=power
    data.shape="RECTANGLE" if size_y else "DISK"
    data.size=size
    if size_y: data.size_y=size_y
    data.color=color
    obj=bpy.data.objects.new(name,data)
    scene.collection.objects.link(obj)
    obj.location=position
    aim(obj,target)

light("Large warm key",5850,(-11,-19,27),(1,0,10),25,(1,.925,.84),14)
light("Cool edge strip",4850,(17,4,24),(6,0,10),15,(.75,.86,1),5)
light("Face fill",2950,(7,-21,14),(4,0,10),18,(.90,.94,1),18)
light("Low warm bounce",600,(-12,4,8),(2,-1,0),12,(1,.86,.74),7)

camera_data=bpy.data.cameras.new("Studio camera")
camera_data.type="ORTHO"
camera_data.ortho_scale=43.0
camera=bpy.data.objects.new("Studio camera",camera_data)
scene.collection.objects.link(camera)
camera.location=(0,-72,18.5)
aim(camera,(0,0,9.25))
scene.camera=camera
bpy.context.view_layer.update()
camera_pose=camera.matrix_world.copy()

text("Studio title","DSH-5",2.13,face_text,camera_pose @ Matrix.Translation((-19.05,7.93,-20)),tracking=1.115)
text("Studio subtitle","A handheld DJ deck",.89,sub_text,camera_pose @ Matrix.Translation((-19.01,6.05,-20)))

scene.render.engine="CYCLES"
scene.cycles.samples=16 if draft else 64
scene.cycles.use_denoising=True
scene.render.resolution_x=2800
scene.render.resolution_y=1575
scene.render.resolution_percentage=40 if draft else 100
scene.render.image_settings.file_format="PNG"
scene.render.film_transparent=False
scene.view_settings.view_transform="AgX"
scene.render.filepath=str((output / "studio-preview.png"
                          if draft else output/"DSH-5-RevD-Studio-Ad.png"))
if not draft:
    scene.render.filepath="//DSH-5-RevD-Studio-Ad.png"
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(output/"DSH-5-RevD-Studio.blend"))
bpy.ops.render.render(write_still=True)
