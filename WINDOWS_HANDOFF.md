# Elemental — Windows / new-chat handoff

## Read this first

This file records the current state of the university OpenGL project so a new
Codex conversation on Windows can continue without reconstructing earlier
decisions from chat history.

Before changing code:

1. Read `AGENTS.md` completely and follow it.
2. Read `elemental_assignment.md`, `README.md` and `BUILDING.md`.
3. Inspect `git status`, this branch's commits and the current diff.
4. Preserve the existing simulation stages, timing chain and visual direction.
5. Build and verify after every coherent change. Keep the code simple enough to
   explain in a university presentation/viva.

## Repository state

- Repository: `https://github.com/konakisnikos/volcano.git`
- Working branch: `refactor/lab-technique-alignment`
- Base branch: `main`
- The branch should be clean after pulling the latest commits.
- Do not reset or overwrite any later local changes made by the user.

Branch commits, oldest first:

1. `61f7dcb fix: make terrain lighting consistently directional`
2. `1383f6c feat: add lab-style classic Phong lighting`
3. `520a94e refactor: clarify particle physics state`
4. `171bc84 feat: blend animated lava texture layers`
5. `84693a1 docs: map rendering techniques to course labs`
6. `b141f7e feat: add animated water normal mapping`

## Current creative and assignment decisions

- The final style is cinematic/stylized and remains nighttime.
- The tornado was intentionally removed and must not be restored.
- The Part B.7 two-element combination is lightning/air energy plus water,
  shown as an electrified river.
- The ending is a calm night: clearer sky, visible moon/stars, persistent scorch
  marks and slow pale volcanic steam.
- The established domino chain is intentional and should remain:
  awakening -> lava -> progressive cooling/smoke/ash -> clouds -> rain -> river
  filling -> grass/flowers/trees -> lightning/scorch marks -> electrified river
  -> calm night -> completed landscape.
- Grass grows only after the river has filled. It spreads broadly across the
  lower terrain but not high up the volcano.
- Flowers are small but visible, use natural red/yellow/pink variation and
  extend beyond the immediate river bank.
- Two camera-facing billboard tree species are present, including a flowering
  almond-like tree. Trees grow by scale and remain smaller than the volcano but
  clearly larger than flowers.
- Lightning impacts and scorch marks are synchronized. Burned ground remains
  visible in the ending.
- Keep the HUD hidden by default. `F1` opens the compact time-control panel.
- Avoid fluid simulation, architectural rewrites and new major features.
- Target approximately 45–60 FPS on an ordinary university laboratory PC.

## Architecture and important files

- `elemental/main.cpp`: application lifecycle, input, explicit stage triggers,
  render passes, shader uniforms, deterministic screenshot arguments and UI.
- `elemental/SceneDirector.*`: scaled simulation clock, pause/restart and stage
  state.
- `elemental/elements/Volcano.*`: procedural terrain/volcano mesh, river masks
  and ground-height sampling.
- `elemental/IntParticleEmitter.*`: common instanced particle drawing.
- `elemental/elements/{Smoke,Cloud,Rain}Emitter.*`: stage-specific emitters and
  motion.
- `elemental/LightningSystem.*`: branching lightning visuals and impact events.
- `elemental/GeometryFactory.*` and `InstancedPropRenderer.*`: procedural grass
  and flowers plus instanced rendering.
- `elemental/shaders/Volcano.fragmentshader`: terrain, lava, river, lighting,
  shadows, vegetation masks, scorch masks and electrified-water shading.
- `elemental/shaders/Prop.fragmentshader`: vegetation lighting/scorch response.
- `elemental/shaders/Skybox.fragmentshader`: night grading, stars, moon and calm
  transition.
- `README.md`: controls, lab mapping and deterministic test arguments.

## What this branch changes relative to `main`

### Lab 6 — consistent directional moonlight

- Terrain and vegetation shading now use `lightDirection_worldspace`.
- The light direction is uploaded from `common/light.*`.
- The orthographic shadow pass and surface shading therefore agree that the
  moonlight is directional.
- Existing 2048x2048 shadow mapping, slope bias and 3x3 PCF are retained.

Known deliberate visual choice: the procedural moon disc is still placed with
a fixed composition-friendly direction in `elemental/main.cpp`, rather than
being derived from `-moonlight->direction`. The lighting and shadow pass are
technically consistent with each other, but the visible moon is artistically
positioned. Only change this after a screenshot comparison; if strict physical
alignment hurts the establishing composition, keep the present placement and
explain it as an artistic background choice.

### Lab 5 — Classic Phong comparison

- Classic Phong uses `reflect(-L, N)` and `R dot V`.
- It is the current default for terrain, water and procedural vegetation.
- The previous Blinn–Phong halfway-vector implementation is retained as a
  comparison path.
- Toggle at runtime through `F1`, or launch with `--classic-phong` and
  `--blinn-phong`.
- The user previously found the visual difference small. Do not spend time
  forcing a large difference: the educational value is the implemented and
  explainable comparison.

### Labs 7/8 — particle semantics

- Smoke/cloud state now exposes named `velocity`, `acceleration` and `mass`
  semantics rather than relying on opaque vector slots.
- Motion behaviour was intentionally preserved.
- The emitters still use `dt`, life and instanced camera-facing particle quads.

### Lab 3 — animated lava texture

- `elemental/assets/lava_overlay.png` is sampled twice with independently
  moving UVs and procedural FBM distortion.
- It is blended into the established procedural lava rather than replacing it.
- Default blend is `0.20`; range is `0.00–0.40`.
- `--lava-texture-blend 0` restores the earlier procedural-only result.
- The same value is adjustable from the `F1` panel.

### Lab 3 — animated water normal texture

- `elemental/assets/water_normal.png` is a 1024x1024 CC0 normal map. Its source
  and license are recorded in `elemental/assets/ASSET_LICENSES.md`.
- Two samples move at different rates/directions. Their longitudinal coordinate
  follows the curved river distance, so the motion does not look like one flat
  world-space translation.
- The texture modifies only small-scale water normals. Procedural river colour,
  fill masks, waves and the electrified-water effect remain intact.
- Default strength is `0.65`; range is `0.00–1.00`.
- `--water-normal-strength 0` is the exact procedural-only comparison.
- Strength is also adjustable in the `F1` panel.
- Last macOS measurement showed no meaningful performance change (about 65 FPS
  both with strength `0` and `0.65` in the same automated comparison).

### Documentation

- `README.md` maps the implementation to Labs 3, 5, 6, 7 and 8.
- It documents controls, test stages, performance reporting and texture/lighting
  comparison arguments.

## Current controls

| Control | Action |
| --- | --- |
| Mouse | Look around |
| `W`, `A`, `S`, `D` | Move camera |
| Up / Down | Change field of view |
| `C` | Restore establishing shot |
| `F1` | Show/hide time-control panel |
| `P` | Pause/resume simulation |
| `[` / `]` | Halve/double simulation speed |
| `0` | Restore 1x speed |
| `R` | Restart the complete sequence |
| `B` | Trigger a demonstration lightning strike |
| `Esc` | Exit |

## Build on Windows with Visual Studio 2022

Install Visual Studio 2022 with **Desktop development with C++**, CMake tools
and a Windows SDK. From a Developer PowerShell:

```powershell
git clone https://github.com/konakisnikos/volcano.git
cd volcano
git switch refactor/lab-technique-alignment
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release -j 4
```

Open the repository with **File -> Open -> Folder** in Visual Studio. For a
multi-configuration Visual Studio build the executable is expected under a
configuration directory, normally:

```powershell
.\build\Release\elemental.exe
```

If the generated target is placed elsewhere, locate it with:

```powershell
Get-ChildItem build -Recurse -Filter elemental.exe
```

The first configure needs internet access because Dear ImGui is obtained via
CMake `FetchContent`. GLFW, GLEW, GLM, SOIL, TinyXML2 and TinyObjLoader are
already included under `external/`.

## Deterministic verification

Valid screenshot stages are:

`awakening`, `lava`, `ash`, `cloud`, `rain`, `river`, `vegetation`, `storm`,
`electric`, `calm`, `complete`.

PowerShell example:

```powershell
.\build\Release\elemental.exe --windowed --speed 8 --auto-exit `
  --screenshot "$env:TEMP\elemental-final.bmp" `
  --screenshot-stage complete --screenshot-delay 1
```

Performance run:

```powershell
.\build\Release\elemental.exe --windowed --speed 8 --auto-exit `
  --report-performance
```

For water comparisons capture both `river` and `electric` with:

```powershell
--water-normal-strength 0
--water-normal-strength 0.65
```

For lighting comparisons repeat the same shot with:

```powershell
--classic-phong
--blinn-phong
```

Check the console for shader compilation, framebuffer and OpenGL errors, and
confirm the complete stage order reaches `Complete`.

## Last verification before this handoff

- Command: `cmake --build build -j4`
- Result: successful; target `elemental` built.
- Runtime renderer: Apple M4, OpenGL 4.1 Metal.
- `river` and `electric` deterministic captures completed successfully with the
  water normal asset loaded.
- All shaders compiled and linked in both runs.
- No reported OpenGL, shader or framebuffer errors.
- Temporary screenshots were written only under `/private/tmp` and are not part
  of the repository.

## What the next conversation should do

The branch is a lab-alignment refinement, not a feature expansion. Continue in
this order:

1. Pull the branch on Windows and perform a clean Release configure/build.
2. Run the full deterministic sequence once and confirm platform compatibility,
   asset loading, all stage transitions and absence of OpenGL errors.
3. Visually compare water strength `0` versus `0.65` during river filling and
   the electrified-river stage. Tune only if the normal pattern is too strong,
   too repetitive or hides the electrical effect.
4. Compare Classic Phong and Blinn–Phong using identical screenshots. Keep
   Classic Phong as default if it remains visually equivalent; retain the
   existing toggle either way.
5. Measure Release performance and distinguish true low FPS from frame-pacing
   issues. Interactive runs use VSync; automated performance runs are uncapped.
6. Only if time remains, compare the current fixed moon composition with a moon
   direction derived from the light. Do not keep the change if it harms the
   composition.

Optional experiment deliberately not implemented: sampling `smoke3.png` in the
particle fragment shader. The current large procedural puffs were preferred;
the texture risks obvious repeated sprites. Only revisit it if a comparison
shows a clear improvement.

## Systems that should remain unchanged

- SimulationStage values and the domino timing chain unless a concrete bug is
  reproduced.
- Progressive lava cooling from the end of the flow back toward the source.
- Rain-fed gradual river fill rather than lava-style directional filling.
- Persistent scorch marks and the synchronized visible lightning impacts.
- Electrified river as the assignment's two-element mixing result.
- Nighttime calm ending and pale slow volcanic steam.
- Procedural terrain, cubemap, instancing, ImGui and existing particle system.
- The user's current terrain texture/lighting character unless a small verified
  correction is requested.
