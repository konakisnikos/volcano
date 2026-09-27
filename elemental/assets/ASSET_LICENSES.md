# Asset provenance and licenses

This file records the visual files that contribute to the final application.
Unknown provenance is stated explicitly rather than inferred from appearance or
file metadata.

## `heightmap_8bit.png` and `Diffusemap.png`

- Source: [Dark Alien Landscape Height Map](https://www.motionforgepictures.com/sdm_downloads/dark-alien-landscape-height-map/), Chris J. Mitchell, Motion Forge Pictures.
- Original package: 2048 x 2048 height, diffuse and bump maps in PNG/EXR formats.
- Project processing: the height and diffuse maps were edited and stored at 1536 x 1536.
- License: royalty-free, non-exclusive use in commercial and non-commercial projects under the [Motion Forge Pictures terms](https://www.motionforgepictures.com/terms-and-conditions/).
- Required credit: **Models or Textures Supplied by Motion Forge Pictures**.

## `skybox_blue/*.png`

- Generated for this project with [Space 3D](https://wwwtyro.github.io/space-3d/) by wwwtyro.
- The seed, colours, nebulae and star density were configured before exporting the six cubemap faces.
- `skybox_blue` is the final runtime cubemap. The other skybox directories are retained alternatives.
- The Space 3D source code is released under the [Unlicense](https://github.com/wwwtyro/space-3d/blob/master/LICENSE).

## `water_normal.png`

- Source: [ProcTexture Wave Normal Map](https://proctexture.com/textures/water/normal-maps/wave-normal-map).
- Original downloaded file: `normal.png` from the 1K PBR pack.
- License: [CC0](https://proctexture.com/about/).
- Used as two moving normal-map layers in the river shader.

## `ambient_cloud_bank.png`

- Generated specifically for this project with the Codex image-generation tool.
- Processed as a transparent panoramic cloud bank for the opening sky.

## `stopwatch_hand_frames/`

- The source hand-and-stopwatch image was generated for this project with ChatGPT Image Generation.
- The background was removed and the image was separated into layers for a 2.5D Blender scene.
- The static layer and three nine-frame finger sequences were rendered from `art/stopwatch_hand/stopwatch_hand.blend`.
- The application draws a live Dear ImGui face inside the photographic case and places invisible hitboxes over the three photographed buttons.

## `lava_overlay.png`

- Derived from the existing `lava.png` by cropping/resizing it into a smaller square tile.
- **Unresolved:** the original source and license of `lava.png` are not recorded in the repository.
- Before distribution, identify the original source or replace this texture with a project-created or clearly licensed image.

## `tree_billboard.png` and `almond_tree_billboard.png`

- Used as alpha-cutout, camera-facing tree billboards.
- **Unresolved:** their original creator/source and license are not recorded in the repository.
- Before distribution, document their provenance or replace them with project-created or clearly licensed images.

## Smoke images retained in the asset directory

- `smoke.png` and `smoke2.png` are not loaded by the final application.
- `smoke3.png` is still loaded, but `ParticleShader.fragmentshader` does not sample it; the visible smoke, cloud and rain shapes are generated procedurally from UV coordinates.
- These files have no visual contribution in the current build. Their provenance is not recorded, so they should be removed from the final delivery together with the unused runtime load, or documented if they are retained.
