# Elemental

`Elemental` is a C++/OpenGL 3.3 university graphics project built around a
procedural volcanic landscape and a staged chain of elemental transformations.
The presentation begins with a quiet night scene, follows lava, ash, clouds,
rain and vegetation, then ends with lightning energising the river before the
landscape settles into a calm night.

The opening cloud bank is a slowly drifting transparent image in the skybox.
The later storm cloud is a dense procedural particle volume with a raised,
irregular center; rain uses a separate particle emitter.

The implementation uses OpenGL techniques including
procedural meshes, GLSL shaders, classic Phong lighting, a 2048×2048 shadow
map, instanced particles and vegetation, alpha-cutout tree billboards, and a
small Dear ImGui time controller.

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
shows the speed; there is no digital time readout. A hand holds the watch, and
the finger above each button briefly presses it for mouse and keyboard input.
The finger animation uses real time, so it remains visible while paused.
Other controls are available separately with F2. Its visual settings adjust
lava glow and shore reach, water brightness and reflections, and storm cloud
and cooling-smoke opacity. `Reset visuals` restores the selected default appearance;
`Copy visual settings` copies the current values as launch arguments so they
can be reused or shared. The camera keeps its position and viewing direction
throughout the sequence. The opening shot uses a 47-degree field of view;
showing the stopwatch smoothly widens it to 50 degrees. A brief scripted shake
precedes the eruption.

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
Add `--show-controls` to display the stopwatch or `--show-settings` to display
the F2 panel in an automated screenshot.

Lava and water texture strengths can also be selected from the command line:

```sh
# Range 0.00–0.40. Zero shows the original procedural-only lava.
./build/elemental --lava-texture-blend 0.20

# Range 0.00–1.00. Zero shows the original procedural-only water normals.
./build/elemental --water-normal-strength 0.65
```

Both texture strengths and the other visual settings are also available in the
`F2` panel during a run. For example, `--lava-emission 1.25
--lava-glow-width 140 --water-brightness 0.80` overrides the defaults.

## Code map

- `elemental/main.cpp`: application lifecycle, explicit stage triggers and
  render passes.
- `elemental/SceneDirector.*`: scaled simulation clock, pause/restart and stage
  state.
- `elemental/elements/Volcano.*`: procedural terrain/volcano mesh and sampled
  ground height.
- `elemental/effects/IntParticleEmitter.*`: shared instanced particle rendering.
- `elemental/effects/{Smoke,Cloud,Rain}Emitter.*`: stage-specific particle
  motion.
- `elemental/effects/LightningSystem.*`: branching bolt animation and impact events.
- `elemental/effects/CrackSystem.*`: crack events and terrain shader data.
- `elemental/GeometryFactory.*`: procedural grass and flower primitives.
- `elemental/InstancedPropRenderer.*`: two-batch flower rendering.
- `elemental/shaders/`: terrain, sky, particle, shadow, vegetation, tree and
  lightning GLSL programs.

The original assignment text is preserved in
[elemental_assignment.md](elemental_assignment.md).
