# Build the GLB for the site's 3D viewer (Clawd3D.vue) from the Ket model.
#
# 1. In Ket, export each body below as STL named viewer-<body>.stl
#    (they land in ~/Downloads). mx1/mx2 cannot be exported (open meshes),
#    so they are stand-in boxes measured from the model.
# 2. python3 viewer-glb.py   (needs trimesh, numpy, fast-simplification)
# 3. gltfpack -i clawd-v2.glb -o models/v2/web/clawd-v2.glb -cc -kn -km
#
# Ket is Z up, glTF is Y up: the meshes are rotated -90 degrees about X.
import trimesh, numpy as np, os, fast_simplification
D = os.path.expanduser("~/Downloads")
C = {  # body: (color rgba, max faces)
 "back_shell": ((217,119,69,255), 40000), "front_shell": ((217,119,69,255), 40000),
 "cap1": ((40,40,44,255), 6000), "cap2": ((40,40,44,255), 6000),
 "eye_cap_l": ((255,226,140,170), 500), "eye_cap_r": ((255,226,140,170), 500),
 "usb_bezel": ((220,235,245,150), 4000), "battery_plate": ((150,150,155,255), 4000),
 "lipo": ((185,190,198,255), 500), "board": ((31,90,53,255), 10**9),
 "part_U1": ((60,62,66,255), 15000), "part_K1": ((30,30,30,255), 6000), "part_K2": ((30,30,30,255), 6000),
 "part_SW1": ((120,120,125,255), 200), "part_SW2": ((120,120,125,255), 200),
 "part_SW3": ((70,70,75,255), 3000), "part_SW4": ((70,70,75,255), 3000),
 "part_D1": ((250,250,240,255), 200), "part_D2": ((250,250,240,255), 200),
 "part_R1": ((30,30,30,255), 200), "part_R2": ((30,30,30,255), 200), "part_J1": ((235,235,230,255), 200),
}
rot = trimesh.transformations.rotation_matrix(-np.pi/2, [1,0,0])
scene = trimesh.Scene()
def add(name, m, rgba):
    m.apply_transform(rot)
    lin = [((c/255)/12.92 if c/255 <= 0.04045 else ((c/255+0.055)/1.055)**2.4) for c in rgba[:3]] + [rgba[3]/255]
    mat = trimesh.visual.material.PBRMaterial(name=name, baseColorFactor=lin, metallicFactor=0.0,
          roughnessFactor=0.55, alphaMode="BLEND" if rgba[3] < 255 else "OPAQUE", doubleSided=True)
    m.visual = trimesh.visual.TextureVisuals(material=mat)
    scene.add_geometry(m, node_name=name, geom_name=name)
for n,(rgba,maxf) in C.items():
    m = trimesh.load(f"{D}/viewer-{n}.stl")
    m.merge_vertices()
    if n in ("part_K1", "part_K2", "part_U1"):
        m = trimesh.util.concatenate([c.convex_hull for c in m.split(only_watertight=False) if len(c.faces) > 40]) if n == "part_U1" else m.convex_hull
    if len(m.faces) > maxf:
        v,f = fast_simplification.simplify(m.vertices.astype(np.float32), m.faces.astype(np.int32), 1-maxf/len(m.faces))
        m = trimesh.Trimesh(v,f)
    print(n, len(m.faces))
    add(n, m, rgba)
# mx switches cannot be exported (open mesh): stand-in housings with the measured boxes
for n,(lo,hi) in {"mx1":((-17.325,-7.55,10.335),(-1.711,8.064,25.2)), "mx2":((1.725,-7.55,10.335),(17.339,8.064,25.2))}.items():
    lo,hi=np.array(lo),np.array(hi); b=trimesh.creation.box(extents=hi-lo); b.apply_translation((lo+hi)/2)
    add(n, b, (25,25,28,255))
scene.export("clawd-v2.glb")
print(os.path.getsize("clawd-v2.glb")//1024, "KB")
