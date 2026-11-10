# NVIDIA RTX engine upgrade

This branch promotes the pinned **ReForged** Quake III Arena R source from a
vendor reference to the repository's `engine/` submodule.

- Upstream: https://github.com/fknfilewalker/Quake-III-Arena-R
- Upstream branch: `reforged`
- Pinned commit: `f85ead64d13be64196aa665325e086337bd16caf`
- Engine source: `engine/code/`
- Vulkan renderer: `engine/code/renderer/`
- Ray-tracing implementation: `engine/code/renderer/tr_raytracing.c`
  and `engine/code/renderer/vulkan/`
- Ray-tracing shaders: `engine/shader/glsl/raytracer/`

## What changed in this repository

The complete upstream project is checked out at `engine/`, including its
top-level CMake project, shaders, runtime data, and nested dependencies. The
previous Quake III source remains in this repository's original `code/`
directory as a fallback/reference; it has not been deleted or rewritten.

The previous `vendor/Quake-III-Arena-R` submodule path is replaced by
`engine/`. Initialize all source and nested dependencies with:

```sh
git submodule update --init --recursive
```

## Windows build

The pinned upstream project documents a CMake build on Windows. Install:

- CMake 3.14 or newer
- Visual Studio C++ desktop development tools
- The Vulkan SDK
- An NVIDIA GPU and driver that expose the legacy
  `VK_NV_ray_tracing` extension required by this revision

From the repository root, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-rtx.ps1
```

Or configure/build manually:

```powershell
cmake -S engine -B build-rtx -G "Visual Studio 17 2022" -A x64
cmake --build build-rtx --config Release
```

The upstream CMake project sets the executable and library output directories
inside `engine/bin/`. This repository does not include Quake III commercial
game data; set `fs_basePath` to a legally obtained installation when running
the game. The upstream README also requires copying `engine/blue_noise_textures`
into `baseq3/blue_noise_textures` and using native game modules:

```text
+set fs_basePath "C:\\Path\\To\\Quake III" +set sv_pure 0 +set vm_game 0 +set vm_cgame 0 +set vm_ui 0
```

## Platform support found in the pinned source

| Platform | Status in this upstream revision |
|---|---|
| Windows | CMake path and NVIDIA `VK_NV_ray_tracing` extension are present; this is the intended RTX build target. |
| macOS | CMake/Vulkan (MoltenVK) setup exists, but the pinned source does not enable the NVIDIA ray-tracing extension there. Do not describe this as a verified RTX build. |
| Linux | The Vulkan surface setup and Vulkan CMake discovery are incomplete in this revision. The Linux target links the OpenGL path; Linux RTX is not supported by this pinned source as-is. |

This is a limitation of the selected upstream commit, not a claim that Vulkan
ray tracing is inherently Windows-only. A cross-platform RTX port would need
platform surface integration, supported ray-tracing extensions, and separate
build/test coverage.

## AI-agent integration status

The existing `agent-control/` gateway and browser UI are preserved. They
currently provide provider selection and authenticated HTTP chat requests, but
the game engine does not yet call the gateway and AI responses cannot yet issue
in-game actions. This engine upgrade does not claim otherwise.

## Licensing and provenance

The engine is tracked as a Git submodule at a fixed upstream commit so its
license files and attribution remain with the upstream project. The legacy
source tree and this repository's existing license notices remain intact.
