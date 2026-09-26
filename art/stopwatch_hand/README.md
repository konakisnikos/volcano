# Stopwatch hand artwork

The hand is a 2.5D Blender scene built from the owner-supplied image. It is not
a 3D mesh or skeletal hand. The fixed camera renders a static palm layer and
three independently animated upper fingers. Each finger has a `Rest` and a
`Button pressed` shape key, animated over frames 1–9 at 30 fps.

`reference.jpg` is the supplied image. `hand_cutout.png` is the transparent
version used on the image plane. The packed image in `stopwatch_hand.blend`
keeps the Blender file editable. `build_scene.py` reconstructs the scene and
renders the PNG layers to `elemental/assets/stopwatch_hand_frames/`.

From the repository root, regenerate the art with:

```text
blender --background --factory-startup --python art/stopwatch_hand/build_scene.py
```

The application draws the hand layers behind the existing ImGui watch. The
photo's dial acts only as a placement guide: the live dial, second hand, speed
text, and three controls cover it. `StopwatchWidget.cpp` selects one of the
rendered frames when a control is pressed. Keyboard `[` / `P` / `]` and mouse
clicks trigger the same animation.

The `PhotoRegion` values in `StopwatchWidget.cpp` must match the `crop` values
in `build_scene.py` if the artwork is adjusted. The transparent hand is placed
relative to photo watch center `(519, 668)` and source case radius `285`.
