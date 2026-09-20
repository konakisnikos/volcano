# Building Elemental

## Requirements

- CMake 3.16 or newer
- A C++11 compiler
- Git and an internet connection during the first default configuration
  (Dear ImGui is fetched at a pinned tag)
- An OpenGL 3.3 Core capable driver

GLFW, GLEW, GLM, SOIL, TinyXML2 and TinyObjLoader are included under
`external/`.

On Debian/Ubuntu, the GLFW source build generally needs:

```sh
sudo apt install build-essential cmake git libgl1-mesa-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

## Configure

Release build (recommended for presentation and performance testing):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

To build without the optional UI:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DELEMENTAL_ENABLE_IMGUI=OFF
```

## Compile

```sh
cmake --build build -j4
```

## Run

```sh
./build/elemental --windowed
```

Without `--windowed`, the program uses the primary monitor in fullscreen mode.
Runtime shader and asset paths are compiled from the source directory, so the
executable can be launched from either the repository root or `build/`.

## Smoke test

This command runs the deterministic sequence at 8× speed, captures the final
stage, and closes automatically:

```sh
./build/elemental --windowed --speed 8 --auto-exit \
  --screenshot /private/tmp/elemental-smoke-test.bmp \
  --screenshot-stage complete --screenshot-delay 1
```

The console should show every simulation stage in order and no shader,
framebuffer, or OpenGL errors.

For a complete uncapped performance run without a screenshot:

```sh
./build/elemental --windowed --speed 8 --auto-exit --report-performance
```
