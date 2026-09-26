"""Build the editable 2.5D stopwatch hand and render its HUD layers.

Run from the repository root:
    blender --background --factory-startup --python art/stopwatch_hand/build_scene.py

The supplied photograph stays on an image plane. Three polygonal image planes
carry the button fingers; keyframed shape keys move their tips toward the
pushers. Holdout planes remove the resting fingers from the still background.
"""

from pathlib import Path
import bpy


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "art" / "stopwatch_hand" / "hand_cutout.png"
BLEND = ROOT / "art" / "stopwatch_hand" / "stopwatch_hand.blend"
OUTPUT = ROOT / "elemental" / "assets" / "stopwatch_hand_frames"
OUTPUT.mkdir(parents=True, exist_ok=True)

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)

scene = bpy.context.scene
bpy.context.preferences.filepaths.save_version = 0
scene.render.engine = "BLENDER_EEVEE"
scene.render.film_transparent = True
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.render.image_settings.color_depth = "8"
scene.render.resolution_percentage = 75
scene.render.use_file_extension = True
scene.view_settings.view_transform = "Standard"
scene.render.film_transparent = True
scene.frame_start = 1
scene.frame_end = 9
scene.render.fps = 30

image = bpy.data.images.load(str(SOURCE), check_existing=True)
image.alpha_mode = "STRAIGHT"
width, height = image.size
scene.render.resolution_x = width
scene.render.resolution_y = height


def xy(pixel):
    return (pixel[0] - width / 2, height / 2 - pixel[1], 0)


def image_material():
    material = bpy.data.materials.new("Original hand photograph")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    links = material.node_tree.links
    output = nodes.new("ShaderNodeOutputMaterial")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Linear"
    emission = nodes.new("ShaderNodeEmission")
    transparent = nodes.new("ShaderNodeBsdfTransparent")
    mix = nodes.new("ShaderNodeMixShader")
    links.new(texture.outputs["Color"], emission.inputs["Color"])
    links.new(texture.outputs["Alpha"], mix.inputs[0])
    links.new(transparent.outputs[0], mix.inputs[1])
    links.new(emission.outputs[0], mix.inputs[2])
    links.new(mix.outputs[0], output.inputs["Surface"])
    return material


def holdout_material():
    material = bpy.data.materials.new("Remove static finger pixels")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    holdout = nodes.new("ShaderNodeHoldout")
    material.node_tree.links.new(holdout.outputs[0], output.inputs["Surface"])
    return material


PHOTO = image_material()
HOLDOUT = holdout_material()


def polygon_object(name, points, material, z):
    mesh = bpy.data.meshes.new(name + " mesh")
    vertices = [xy(point) for point in points]
    mesh.from_pydata(vertices, [], [tuple(range(len(vertices)))])
    mesh.update()
    uv_layer = mesh.uv_layers.new(name="Photo coordinates")
    for polygon in mesh.polygons:
        for loop_index in polygon.loop_indices:
            vertex_index = mesh.loops[loop_index].vertex_index
            px, py = points[vertex_index]
            uv_layer.data[loop_index].uv = (px / width, 1 - py / height)
    obj = bpy.data.objects.new(name, mesh)
    scene.collection.objects.link(obj)
    obj.location.z = z
    obj.data.materials.append(material)
    return obj


back = polygon_object(
    "Static palm and watch registration image",
    [(0, 0), (width, 0), (width, height), (0, height)], PHOTO, 0,
)

# Coordinates follow the alpha cutout, top-left origin. The lower edges hide
# behind the live ImGui bezel when composited in the app.
fingers = [
    {
        "name": "slower",
        "outline": [(90, 215), (105, 170), (140, 130), (200, 105),
                    (270, 115), (320, 150), (355, 205), (388, 280),
                    (445, 380), (455, 460), (280, 470), (245, 375),
                    (200, 345), (150, 320), (110, 280)],
        "tip_y": 105,
        "root_y": 470,
        "press": (8, 48),
        "crop": (65, 75, 480, 545),
    },
    {
        "name": "pause",
        "outline": [(365, 190), (365, 70), (415, 0), (525, 0),
                    (580, 35), (612, 105), (635, 190), (680, 280),
                    (712, 360), (710, 420), (485, 420), (435, 315),
                    (400, 250)],
        "tip_y": 0,
        "root_y": 420,
        "press": (0, 160),
        "crop": (335, 0, 745, 510),
    },
    {
        "name": "faster",
        "outline": [(680, 330), (700, 190), (760, 150), (835, 170),
                    (905, 230), (965, 285), (1030, 330), (1082, 365),
                    (1082, 570), (920, 560), (850, 490), (790, 430),
                    (720, 390)],
        "tip_y": 150,
        "root_y": 570,
        "press": (-125, 65),
        "crop": (620, 105, 1082, 650),
    },
]

finger_objects = []
holdout_objects = []
for config in fingers:
    name = config["name"]
    points = config["outline"]
    holdout_objects.append(polygon_object(name + " holdout", points, HOLDOUT, 0.01))
    finger = polygon_object(name + " animated finger", points, PHOTO, 0.02)
    finger.shape_key_add(name="Rest")
    pressed = finger.shape_key_add(name="Button pressed")
    dx, dy = config["press"]
    for index, (px, py) in enumerate(points):
        weight = max(0.0, min(1.0,
                              (config["root_y"] - py) /
                              (config["root_y"] - config["tip_y"])))
        pressed.data[index].co.x += dx * weight
        pressed.data[index].co.y -= dy * weight
    for frame, amount in [(1, 0.0), (3, 1.0), (4, 1.0), (9, 0.0)]:
        pressed.value = amount
        pressed.keyframe_insert(data_path="value", frame=frame)
    finger_objects.append(finger)

# Orthographic camera: one world unit equals one pixel in the supplied image.
camera_data = bpy.data.cameras.new("Fixed HUD camera")
camera = bpy.data.objects.new("Fixed HUD camera", camera_data)
scene.collection.objects.link(camera)
camera.location = (0, 0, 10)
camera_data.type = "ORTHO"
camera_data.ortho_scale = height
scene.camera = camera


def border(left, top, right, bottom):
    scene.render.use_border = True
    scene.render.use_crop_to_border = True
    scene.render.border_min_x = left / width
    scene.render.border_max_x = right / width
    scene.render.border_min_y = 1 - bottom / height
    scene.render.border_max_y = 1 - top / height


def render(path):
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


# Static layer contains a hole wherever a finger is animated.
for finger in finger_objects:
    finger.hide_render = True
border(0, 0, width, height)
scene.frame_set(1)
render(OUTPUT / "back.png")

back.hide_render = True
for holdout in holdout_objects:
    holdout.hide_render = True

for finger_index, config in enumerate(fingers):
    for index, finger in enumerate(finger_objects):
        finger.hide_render = index != finger_index
    border(*config["crop"])
    for frame in range(1, 10):
        scene.frame_set(frame)
        render(OUTPUT / (config["name"] + "_" + str(frame - 1) + ".png"))

# The saved .blend opens with all hand pieces visible at rest. The user can
# scrub frames 1..9, then select a finger to see its shape-key animation.
back.hide_render = False
for holdout in holdout_objects:
    holdout.hide_render = False
for finger in finger_objects:
    finger.hide_render = False
scene.render.use_border = False
scene.render.use_crop_to_border = False
scene.render.filepath = "//render_preview.png"
scene.frame_set(1)
image.pack()
bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
print("Saved", BLEND)
