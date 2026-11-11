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

## Dependency audit and pinned updates

The engine's nested submodules are initialized from the upstream engine commit,
then the build preparation script checks out these tested-for-source-compatibility
revisions:

| Dependency | Previous pinned revision | Updated revision | Reason |
|---|---|---|---|
| [stb](https://github.com/nothings/stb) | `2c2908f50515dcd939f24be261c3ccbcd277bb49` | `2b2e64bc4778df6a06fbea3a5dbe61f7a76ecf19` | Brings stb headers forward to the March 2026 revision while retaining `stb_image_resize2.h`, which `code/renderer/tr_image.c` includes. The October 2026 upstream revision moved that header out of the stb repository, so it is not used here. |
| [TinyJPEG](https://github.com/serge-rgb/TinyJPEG) | `56b6635a23a439a86026c81a5e6c4d27c2d35cd0` | `e978b746714abad76e0f00264d2a154b52de8fc1` | Applies the upstream big-endian byte-order fix; the public header API remains the same. |

Run the dependency preparation independently:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\update-dependencies.ps1
```

The RTX build script runs this step automatically before CMake configuration.
The script fetches exact commit IDs and verifies each checked-out revision;
it does not follow moving `master` branches. Because the engine itself remains
a pinned upstream submodule, these dependency overrides are applied to the
nested submodule working directories at build preparation time. A fresh clone
should run the script again after initializing submodules.

The AI gateway has no third-party Python packages: `agent-control/server.py`
uses only the Python standard library. The native engine also depends on
system-provided CMake, Vulkan SDK, OpenGL, and (on Linux) X11 development
libraries; their versions are selected by the host platform and are not
vendored or automatically replaced by this script.

This is a source-level dependency update, not a claim that all architecture
builds or runtime paths have passed CI. Compile and test x86 and x64 separately
after installing matching 32-bit/64-bit Vulkan loader and driver components.

## Windows architecture targets

The build script accepts all three requested architecture labels:

| Requested label | CMake / Visual Studio target | Meaning |
|---|---|---|
| `x86` | `Win32` | 32-bit Intel/AMD |
| `amd64` | `x64` | 64-bit AMD64 |
| `x86-64` | `x64` | Same 64-bit architecture as AMD64 |
| `all` | `Win32` and `x64` | Builds both distinct architectures once |

AMD64 and x86-64 are aliases, not separate processor architectures, so the
script intentionally does not compile the same 64-bit target twice.

Install:

- CMake 3.14 or newer
- Visual Studio C++ desktop development tools (including x86 and x64
  toolchains)
- The Vulkan SDK
- For an x86 build, the 32-bit Vulkan loader and compatible 32-bit NVIDIA
  driver components must also be installed
- An NVIDIA GPU and driver that expose the legacy
  `VK_NV_ray_tracing` extension required by this revision

From the repository root, build one target:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-rtx.ps1 -Architecture x86
powershell -ExecutionPolicy Bypass -File .\scripts\build-rtx.ps1 -Architecture amd64
powershell -ExecutionPolicy Bypass -File .\scripts\build-rtx.ps1 -Architecture x86-64
```

Build both distinct architectures:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-rtx.ps1 -Architecture all
```

Or configure/build manually:

```powershell
cmake -S engine -B build-rtx-x86 -G "Visual Studio 17 2022" -A Win32
cmake --build build-rtx-x86 --config Release

cmake -S engine -B build-rtx-x64 -G "Visual Studio 17 2022" -A x64
cmake --build build-rtx-x64 --config Release
```

The upstream CMake project sets the executable and library output directories
inside `engine/bin/`. This repository does not include Quake III commercial
game data; set `fs_basePath` to a legally obtained installation when running
the game. The upstream README also requires copying
`engine/blue_noise_textures` into `baseq3/blue_noise_textures` and using
native game modules:

```text
+set fs_basePath "C:\\Path\\To\\Quake III" +set sv_pure 0 +set vm_game 0 +set vm_cgame 0 +set vm_ui 0
```

## Platform support found in the pinned source

| Platform | Status in this upstream revision |
|---|---|
| Windows | CMake path and NVIDIA `VK_NV_ray_tracing` extension are present; this is the intended RTX build target. The x86 and x64 configurations are exposed by the build script, but neither has been compiled or verified in this repository session. |
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


## Official Quake III Arena 1.32 / 1.32c multiplayer support

The pinned ReForged source declares `PROTOCOL_VERSION 68` in
`engine/code/qcommon/qcommon.h`. Its server initialization, server browser
responses, and client connection checks use that shared constant. Protocol 68
is the official Quake III Arena 1.32 network protocol and is retained by 1.32c.
This provides the wire-protocol baseline for connecting to original 1.32c
multiplayer servers; it is not a promise of compatibility with older 1.16,
1.29, or 1.31 servers.

The official 1.32 point release also supplies updated game data. The separate
1.32c update is a later executable security update; it does not change the
protocol number. To stage the official patch-data PK3 files from a legally
obtained patch-data package, initialize the engine submodule, install the
original Quake III Arena and Team Arena data, then run:

```powershell
powershell -ExecutionPolicy Bypass -File .\\scripts\\install-q3-132-patch-data.ps1 \\
  -PatchDataPath "C:\\Path\\To\\PatchData" \\
  -GamePath "C:\\Path\\To\\Quake III Arena"
```

The script copies the patch package's `pak*.pk3` files from both `baseq3`
and `missionpack`, requires the eight `baseq3/pak1.pk3` through
`pak8.pk3` files, and checks that the selected engine source still defines
protocol 68. It does not download, include, or redistribute proprietary game
data. It does not install the original 1.32c executable, PunkBuster, or claim
to port all 1.32c executable security fixes into the ReForged source.

For network compatibility context, ioquake3 explicitly documents that its
legacy protocol is compatible with the final id Software 1.32c release:
https://ioquake3.org/help/sys-admin-guide/

The engine's existing protocol-68 constant was verified in the pinned
upstream source at `f85ead64d13be64196aa665325e086337bd16caf`. No protocol
number was changed by this repository update. Multiplayer runtime testing
against an original 1.32c client/server has not yet been performed.


## OpenGL renderer option for legacy systems

The pinned renderer already includes an OpenGL backend alongside Vulkan. Select it at runtime with `+set r_glDriver opengl32 +set r_rtx 0` on Windows or `+set r_glDriver libGL.so.1 +set r_rtx 0` on Linux. A PowerShell helper is available at `scripts/launch-opengl.ps1`; usage and limitations are documented in `docs/OPENGL-LEGACY-RENDERER.md`. This selects OpenGL at runtime but does not remove the upstream CMake requirement for the Vulkan SDK during compilation. It is not yet a Vulkan-independent build, and compatibility with genuinely old OpenGL hardware has not been tested.

## Separate OpenGL-only legacy build

For systems that cannot use the Vulkan SDK or the ReForged Vulkan renderer,
this repository has a separate OpenGL-only build path based on the upstream
`original` branch pinned at `c95f4cecf7e0fec92e275d06645d5f2d14db286c`.
Use `scripts/build-rtx.ps1 -Renderer OpenGL -Architecture x86|amd64|all`.
This builds a distinct legacy engine and does not change the RTX engine
submodule or remove Vulkan from the ReForged source. See
`docs/OPENGL-LEGACY-RENDERER.md` for full instructions and limitations.
