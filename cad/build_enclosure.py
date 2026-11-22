import json
from pathlib import Path

import cadquery as cq

root = Path(__file__).resolve().parent
outputs = root
mesh_dir = root / "meshes"
mesh_dir.mkdir(exist_ok=True)
pcb_file = root / "DSH-5-RevD-PCB.step"
installed = cq.importers.importStep(str(pcb_file)).solids().vals()
pcb_top = 1.595
x_center, y_center = 80, -127.5
width, height, wall = 96, 187, 2.55
rear_z, rear_inside = -35, -32.3
deck_bottom, deck_z = 10.85, 13
silver = cq.Color(.70, .71, .72)
navy = cq.Color(.10, .17, .24)
coal = cq.Color(.12, .14, .16)
dark = cq.Color(.055, .065, .072)
orange = cq.Color(.85, .18, .045)
ivory = cq.Color(.80, .79, .72)
green = cq.Color(.17, .28, .17)
reserve_color = cq.Color(.9, .42, .12, .35)


def slab(w, h, low, high, x=x_center, y=y_center, radius=0):
    piece = cq.Workplane("XY").box(w, h, high-low, centered=(True, True, False))
    if radius:
        piece = piece.edges("|Z").fillet(radius)
    return piece.translate((x, y, low))


def block(w, h, depth, x, y, z, radius=0, axis="Z"):
    result = cq.Workplane("XY").box(w, h, depth)
    if radius:
        result = result.edges("|" + axis).fillet(radius)
    return result.translate((x, y, z))


def hole(x, y, diameter, low, high):
    return cq.Workplane("XY", origin=(x, y, low)).circle(diameter/2).extrude(high-low)


def side_hole(x0, x1, y, z, diameter):
    return cq.Workplane("YZ", origin=(x0, y, z)).circle(diameter/2).extrude(x1-x0)


def bounds(shape):
    b = shape.BoundingBox()
    return tuple(round(v, 4) for v in (b.xmin, b.xmax, b.ymin, b.ymax, b.zmin, b.zmax))


def overlap(a, b):
    return all(min(a[i+1], b[i+1])-max(a[i], b[i])>.0001 for i in (0, 2, 4))


def interference(shape, others):
    span = bounds(shape)
    return [
        {"bounds_mm": bounds(other), "volume_mm3": round(volume, 4)}
        for other in others if overlap(span, bounds(other))
        and (volume := shape.intersect(other).Volume()) > .010
    ]


frame = slab(width, height, rear_inside, deck_bottom, radius=7.8)
frame = frame.cut(slab(width-2*wall, height-2*wall, rear_inside-.2, deck_bottom+.2, radius=5.25))

# Bring the PCB switch caps up to the outside wall with removable plungers.
button_y = (-83.75, -97.75, -111.75)
for y in button_y:
    frame = frame.cut(block(14, 10.2, 5.85, 126.4, y, 4.845, .8, "X"))
frame = frame.cut(block(12, 34, 8.3, 34, -97.25, 5.095, 1.5, "X"))
speed_window = block(7, 15, 7, 34.1, -142., -11.15, 1.5, "X")
frame = frame.cut(speed_window)
for y in (-121.7, -186):
    frame = frame.cut(side_hole(118, 130, y, -2.585, 10.4))
# Isolated panel headphone extension returns to the shield's headphone jack.
frame = frame.cut(side_hole(122, 130, -156, -17.9, 6.6))
frame = frame.cut(block(13.2, 12, 6.6, 76, -220, 2.73, 2, "Y"))

stations = ((36.5,-40.2),(123.5,-40.2),(36.5,-214.8),(123.5,-214.8))
for x,y in stations:
    frame = frame.union(hole(x,y,6.5,rear_inside,deck_bottom))
    frame = frame.cut(hole(x,y,2.05,rear_inside-.1,rear_inside+6.25))
    frame = frame.cut(hole(x,y,2.05,deck_bottom-6.25,deck_bottom+.15))
# Top-mounted bridge supports the PCB near H2 and clears the pack under it.
frame = frame.union(slab(8,12.7,-3.9,-.15,84.15,-40.25,1.2))
frame = frame.cut(hole(84.15,-44,2.15,-4.1,0))

back = slab(width,height,rear_z,rear_inside,radius=7.8)
logo = json.loads((root / "logo-profile.json").read_text())
logo_profile = [(x*10.6, y*10.6) for x,y in logo]
logo_outline = cq.Workplane("XY",origin=(80,-128,rear_z-.1)).polyline(logo_profile).close()
back = back.cut(logo_outline.extrude(.62))
logo_inlay = (
    cq.Workplane("XY",origin=(80,-128,rear_z+.02)).polyline(logo_profile).close()
    .extrude(.40)
)
for x,y in stations:
    back = back.cut(hole(x,y,2.7,rear_z-.2,rear_inside+.2))
    back = back.cut(hole(x,y,5.1,rear_z-.2,rear_z+.9))
for x,y in ((44,-44),(44,-212),(116,-212)):
    back = back.union(hole(x,y,5.35,rear_inside,-.15))
    back = back.cut(hole(x,y,2.08,-8,-.08))
for x,y in ((61,-79),(93,-79),(61,-111),(93,-111)):
    back = back.union(hole(x,y,5.55,rear_inside,-13.8))
    back = back.cut(hole(x,y,2.08,-20.2,-13.6))
# Raised hold-down tabs leave the middle of the rear bay available for the pack.
for x in (45,115):
    back = back.union(slab(3,18,rear_inside,-28.4,x,-59,1.1))
for y in (-77.2,-37.7):
    back = back.union(slab(34,2,rear_inside,-29.3,80,y,.8))

front = slab(width,height,deck_bottom,deck_z,radius=7.8)
for x,y in stations:
    front = front.cut(hole(x,y,2.65,deck_bottom-.2,deck_z+.2))
    front = front.cut(hole(x,y,4.9,deck_z-.66,deck_z+.2))
front = front.cut(hole(77,-95,56.1,deck_bottom-.2,deck_z+.2))
front = front.cut(slab(35.4,3.35,deck_bottom-.2,deck_z+.2,74.75,-167.75,1.4))
front = front.cut(hole(110,-165.5,7.45,deck_bottom-.2,deck_z+.2))
oled_bottom = cq.Workplane("XY",origin=(102,-57.9,8.35)).rect(22.6,11.6).wire().val()
oled_top = cq.Workplane("XY",origin=(102,-57.9,13.2)).rect(28.4,17.1).wire().val()
front = front.cut(cq.Solid.makeLoft([oled_bottom,oled_top],ruled=True))

# Roof the four tall connector bodies and capacitor with pockets cut from the inside.
rear_pocket_ids = []
for solid in installed:
    x0,x1,y0,y1,z0,z1 = bounds(solid)
    if deck_bottom-.10 < z1 <= 12.025 and z0 < 3.5 and x1-x0 > 4 and y1-y0 > 4:
        front = front.cut(slab(x1-x0+1.2,y1-y0+1.2,deck_bottom-.15,z1+.38,
                               (x0+x1)/2,(y0+y1)/2,.6))
        rear_pocket_ids.append(bounds(solid))
# Tapered wheel surround, built as a separate part for access to the platter.
surround_base = cq.Workplane("XY",origin=(77,-95,deck_z)).circle(30.45).wire().val()
surround_top = cq.Workplane("XY",origin=(77,-95,23.25)).circle(29.25).wire().val()
shroud = cq.Workplane(obj=cq.Solid.makeLoft([surround_base,surround_top],ruled=True))
shroud = shroud.cut(hole(77,-95,56.1,deck_z-.2,23.4))
for x,y in ((54.05,-72.05),(99.95,-72.05),(54.05,-117.95)):
    shroud = shroud.union(hole(x,y,7.25,deck_z,deck_z+1.85))
    shroud = shroud.cut(hole(x,y,2.15,deck_z-.12,deck_z+2))
    front = front.cut(hole(x,y,2.15,deck_bottom-.2,deck_z+.2))

hero_parts = {
    "frame": (frame, "Side chassis", navy),
    "front": (front, "Aluminum control plate", silver),
    "back": (back, "Rear lid with recessed logo", coal),
    "shroud": (shroud, "Platter surround", dark),
    "logo": (logo_inlay, "Rear logo inlay", ivory),
}
for i,y in enumerate(button_y):
    key = ("record", "mix", "play")[i]
    hero_parts[key] = (block(6.35,8.45,3.9,125.755,y,4.845,1.06,"X"),
                       key.upper()+" button plunger", orange if i==0 else dark)
hero_parts["rocker"] = (block(5.3,29.4,4.55,33.68,-97.25,5.095,1.75,"X"),
                        "Speed seesaw coupling cap", dark)
for i,(x,y) in enumerate(((42.5,-45.5),(117.5,-45.5),(42.5,-209.5),(117.5,-209.5))):
    hero_parts[f"foot-{i+1}"] = (hole(x,y,9.2,-35.93,rear_z),f"Rear rubber foot {i+1}",dark)

shell = cq.Assembly(name="DSH-5 Rev D enclosure")
for key,(shape,name,color) in hero_parts.items():
    assert shape.val().isValid() and len(shape.val().Solids()) == 1, name
    shell.add(shape,name=name,color=color)
    cq.exporters.export(shape,str(mesh_dir/(key+".stl")),tolerance=.072)
shell.export(str(outputs/"DSH-5-RevD-Enclosure.step"))

# Volume models for the unassembled harness components. These can be hidden in Fusion.
layout_boxes = {
    "Protected 3S pack and wrap, 65 by 37 by 20 mm": slab(65,37,-31.8,-11.8,80,-57.5,2.0),
    "Teensy 4.1 installed board and card envelope": slab(61.1,18.1,-12.9,-8.72,78,-142,.7),
    "Stacked audio adapter and header envelope": slab(36.5,36.2,-24.7,-13.15,90.1,-142,1),
    "Chassis brake resistor and mounting allowance": slab(11,28,-30.9,-19.7,116,-174,1.4),
    "Isolated headphone feedthrough and cable tail": slab(11.8,10,-23,-12.8,119.48,-156,1.1),
}
regulator = cq.importers.importStep(str(root.parent / "hardware" / "kicad" / "models" / "Pololu_S18V20F12.step")).val().translate((58,-206,-30.4))
reserved = cq.Assembly(name="DSH-5 Rev D internal parts and fit allowances")
for name, shape in layout_boxes.items():
    reserved.add(shape,name=name,color=reserve_color)
reserved.add(regulator,name="Pololu S18V20F12 chassis installation",color=cq.Color(.23,.4,.21))
reserved.export(str(outputs/"DSH-5-RevD-Internal-Layout.step"))

# A single STEP for an assembled deck, with the shell and PCB in the same coordinates.
complete = cq.Assembly(name="DSH-5 Rev D assembled deck")
for shape,name,color in hero_parts.values():
    complete.add(shape,name=name,color=color)
pcb = cq.Assembly(name="Populated Rev D PCB")
for i,solid in enumerate(installed):
    b = bounds(solid)
    color = green if b[1]-b[0]>79 and b[3]-b[2]>175 else coal
    pcb.add(solid,name=f"PCB part {i+1}",color=color)
complete.add(pcb,name="PCB and populated controls")
complete.add(reserved,name="Internal component placement")
complete.export(str(outputs/"DSH-5-RevD-Deck-Assembly.step"))

# Export the visible board parts for Blender using their actual BRep surfaces.
groups = {}
selectors = {
    "platter": lambda b: abs(b[1]-b[0]-54)<.05 and b[4]>24,
    "wheel-mark": lambda b: 76.3<b[0]<76.5 and 77.5<b[1]<77.7 and 28.2<b[4]<28.4,
    "fader-cap": lambda b: 69.4<b[0]<69.6 and 79.9<b[1]<80.1 and 13.8<b[4]<14,
    "fader-mark": lambda b: 74.4<b[0]<74.5 and -170.0<b[2]<-169.5 and 20.0<b[4]<20.2,
    "knob": lambda b: 102.9<b[0]<103.1 and b[1]>116.9 and b[4]>21.4,
    "knob-mark": lambda b: 109.6<b[0]<109.8 and 110.2<b[1]<110.4 and 30.3<b[4]<30.5,
    "oled": lambda b: 90.9<b[0]<91.1 and 112.9<b[1]<113.2 and 8.45<b[4]<8.55,
    "usb": lambda b: b[2]<-216.95 and 71<b[0]<81 and .5<b[4]<1.8,
}
for k,select in selectors.items():
    selected = [s for s in installed if select(bounds(s))]
    assert selected, k
    groups[k] = selected
    cq.exporters.export(cq.Compound.makeCompound(selected),str(mesh_dir/(k+".stl")),tolerance=.06)

checks = {}
for key,(shape,name,_) in hero_parts.items():
    checks[name] = interference(shape.val(),installed)
fixed_interfaces = list(hero_parts.values())
for index,(shape,name,_) in enumerate(fixed_interfaces):
    checks[name+" against other case parts"] = interference(shape.val(),
        [other.val() for other,_,_ in fixed_interfaces[index+1:]])
for name,shape in layout_boxes.items():
    checks[name + " against PCB"] = interference(shape.val(),installed)
    checks[name + " against case"] = interference(shape.val(),[s.val() for s,_,_ in hero_parts.values()])
checks["Motor supply against PCB"] = interference(regulator,installed)
checks["Motor supply against case"] = interference(regulator,[s.val() for s,_,_ in hero_parts.values()])
fixed_shell = [s.val() for k,(s,_,_) in hero_parts.items() if k not in {"record","mix","play","rocker"}]
fader_lever = slab(34,1.2,6.54,18.095,74.75,-167.75)
fader_cap = slab(40.5,7.8,13.895,20.225,74.75,-167.75)
platter_sweep = hole(77,-95,54,24.395,28.505)
checks["Fader full stroke against fixed case"] = (interference(fader_lever.val(),fixed_shell) +
                                                   interference(fader_cap.val(),fixed_shell))
checks["Platter full rotation against fixed case"] = interference(platter_sweep.val(),fixed_shell)
(root/"Fit-check.json").write_text(json.dumps(checks,indent=2)+"\n")
for name,collisions in checks.items():
    if collisions:
        print(name,collisions,flush=True)
assert not any(checks.values()), "Investigate reported interference"
print("Parts:",len(hero_parts),"meshes:",len(groups),"check entries:",len(checks),flush=True)
