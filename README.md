# Elemental

`Elemental` is a C++/OpenGL 3.3 university graphics project built around a
procedural volcanic landscape and a staged chain of elemental transformations.
The presentation begins with a quiet night scene, follows lava, ash, clouds,
rain and vegetation, then ends with lightning energising the river before the
landscape settles into a calm night.

The implementation intentionally uses direct, lab-style OpenGL techniques:
procedural meshes, GLSL shaders, classic Phong lighting, a 2048×2048 shadow
map, instanced particles and vegetation, alpha-cutout tree billboards, and a
small Dear ImGui time controller.

## Relation to the graphics labs

- **Lab 3 — textures and blending:** the terrain uses a repeating diffuse map.
  Lava combines two independently moving samples of `lava_overlay.png`; FBM
  noise distorts their UVs and the result is blended into the procedural lava
  pattern. The river combines two moving samples of `water_normal.png` to add
  small-scale animated normals while keeping its procedural colour, fill mask
  and electrified-water effect. Alpha blending is used for smoke, clouds and
  rain.
- **Lab 5 — lighting:** terrain, water and vegetation use ambient, diffuse and
  specular terms. Classic Phong uses `reflect(-L, N)` followed by `R · V`.
- **Lab 6 — shadows:** moonlight is a directional light in both the terrain
  shader and the orthographic depth pass. The 2048×2048 depth texture is sampled
  with slope-dependent bias and 3×3 PCF.
- **Lab 7 — motion:** smoke and rain use velocity, acceleration and frame `dt`
  with semi-implicit Euler integration (`v += a * dt`, then `p += v * dt`).
- **Lab 8 — particles:** smoke, ash, clouds and rain use emitter classes,
  particle life and camera-facing billboards. Per-particle transforms are sent
  in one interleaved instance buffer and rendered with instanced drawing.

Procedural noise, the cubemap, vegetation instancing and ImGui are retained as
small extensions of those techniques rather than replacements for them.

## Simulation sequence

1. Awakening and camera shake
2. Glowing lava flow
3. Progressive lava cooling with smoke and ash
4. Cloud formation and rain
5. Rain-fed river filling
6. Grass, flowers and two growing tree species
7. Cloud-to-ground lightning with persistent scorch marks
8. Lightning plus water: an electrified-river mixing effect
9. Calm night and completed living landscape

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/elemental
```

Dear ImGui is enabled by default and is downloaded by CMake's `FetchContent` on
the first configure. It can be disabled with
`-DELEMENTAL_ENABLE_IMGUI=OFF`. Detailed platform notes are in
[BUILDING.md](BUILDING.md).

## Controls

| Control | Action |
| --- | --- |
| `F1` | Show or hide the stopwatch |
| `F2` | Show or hide the other controls |
| `P` | Pause or resume simulation time |
| `[` / `]` | Halve or double simulation speed |
| `0` | Restore 1× simulation speed |
| `R` | Restart the complete sequence |
| `B` | Trigger a demonstration lightning strike |
| `Esc` | Exit |

The stopwatch is hidden by default. Its left, middle and right buttons halve
simulation speed, pause/resume, and double simulation speed. The second hand
completes a 30-second simulation-time dial and leaves faint orange afterimages
at 2× speed and above. It pauses and accelerates with the scene. The dial
shows the speed; there is no digital time readout. Other controls, including
the lava and water sliders, are available separately with F2. The camera uses
a fixed view of the volcano and river throughout the sequence, apart from the
brief scripted shake before the eruption.

## Automated visual checks

The executable provides deterministic screenshot arguments used during
development and final verification:

```sh
./build/elemental --windowed --speed 8 --auto-exit \
  --screenshot /private/tmp/elemental-final.bmp \
  --screenshot-stage complete --screenshot-delay 1
```

Valid stages are `awakening`, `lava`, `ash`, `cloud`, `rain`, `river`,
`vegetation`, `storm`, `electric`, `calm`, and `complete`.
Add `--report-performance` to print the uncapped average frame rate together
with p95, p99 and maximum frame times. On macOS, use fullscreen for the final
presentation: the deprecated windowed OpenGL path can have uneven compositor
frame pacing even when the renderer has ample performance headroom.
Add `--show-controls` to display the stopwatch in an automated screenshot.

Lava and water texture strengths can also be selected from the command line:

```sh
# Range 0.00–0.40. Zero shows the original procedural-only lava.
./build/elemental --lava-texture-blend 0.20

# Range 0.00–1.00. Zero shows the original procedural-only water normals.
./build/elemental --water-normal-strength 0.65
```

Both texture strengths are also available in the `F2` panel during a run.

## Code map

- `elemental/main.cpp`: application lifecycle, explicit stage triggers and
  render passes.
- `elemental/SceneDirector.*`: scaled simulation clock, pause/restart and stage
  state.
- `elemental/elements/Volcano.*`: procedural terrain/volcano mesh and sampled
  ground height.
- `elemental/IntParticleEmitter.*`: shared instanced particle rendering.
- `elemental/elements/{Smoke,Cloud,Rain}Emitter.*`: stage-specific particle
  motion.
- `elemental/LightningSystem.*`: branching bolt animation and impact events.
- `elemental/GeometryFactory.*`: procedural grass and flower primitives.
- `elemental/InstancedPropRenderer.*`: two-batch flower rendering.
- `elemental/shaders/`: terrain, sky, particle, shadow, vegetation, tree and
  lightning GLSL programs.

The original assignment text is preserved in
[elemental_assignment.md](elemental_assignment.md).
