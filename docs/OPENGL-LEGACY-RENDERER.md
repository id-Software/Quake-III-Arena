# OpenGL renderer option for legacy systems

The pinned ReForged engine already contains both OpenGL and Vulkan renderer
initialization paths. The video menu exposes both renderer names, and the
runtime selects the backend through `r_glDriver`. OpenGL mode disables the
RTX path with `r_rtx 0`.

## Select OpenGL

Windows:

```bat
quake3.exe +set r_glDriver opengl32 +set r_rtx 0
```

Linux:

```sh
./quake3 +set r_glDriver libGL.so.1 +set r_rtx 0
```

A convenience launcher is provided at `scripts/launch-opengl.ps1`. From
PowerShell, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\launch-opengl.ps1 -GameExe .\engine\bin\quake3.exe
```

Pass additional game arguments after the executable path with
`-GameArguments`, for example:

```powershell
.\scripts\launch-opengl.ps1 -GameExe .\engine\bin\quake3.exe -GameArguments @('+connect', '127.0.0.1')
```

## What this option does—and does not do

- Selects the existing OpenGL renderer at runtime.
- Turns off the RTX feature path.
- Does not require an NVIDIA ray-tracing-capable GPU or the
  `VK_NV_ray_tracing` extension at runtime.
- The combined ReForged build still requires Vulkan at compile time. For a
  genuinely Vulkan-free build, use the separate legacy OpenGL build target
  described below.
- Does not guarantee support for very old OpenGL implementations. Actual
  compatibility depends on the OpenGL version/extensions and driver expected
  by this renderer revision. No legacy-hardware test has been performed.

This runtime selector is for the combined ReForged engine. The separate
Vulkan-free legacy build target below is the route for machines that cannot
install or use Vulkan.

## Vulkan-free OpenGL build target

The ReForged `reforged` engine revision is not cleanly separable into an
OpenGL-only binary: its renderer headers and initialization code include
Vulkan-specific types and entry points, and its CMake project requires Vulkan.
Rather than pretending a CMake toggle removes those dependencies, this
repository now provides a separate, reproducible legacy OpenGL build target
using the same upstream project's `original` branch. That branch's CMake
configuration links OpenGL and does not call `find_package(Vulkan)` or link
`Vulkan::Vulkan`.

The legacy source is pinned to upstream commit
`c95f4cecf7e0fec92e275d06645d5f2d14db286c`. Build it on Windows with:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-rtx.ps1 -Renderer OpenGL -Architecture x86
powershell -ExecutionPolicy Bypass -File .\scripts\build-rtx.ps1 -Renderer OpenGL -Architecture amd64
powershell -ExecutionPolicy Bypass -File .\scripts\build-rtx.ps1 -Renderer OpenGL -Architecture all
```

This target clones and checks out the pinned `original` source into
`build/legacy-opengl-source`, initializes its stb and TinyJPEG submodules,
and builds into separate `build-opengl-x86` / `build-opengl-x64` directories.
It does not require the Vulkan SDK. It is a separate legacy engine line, not
the RTX-enabled ReForged source with Vulkan surgically removed. The older
source revision may need compiler or platform compatibility fixes on modern
toolchains; this repository session has not run or verified these builds.

On Linux, the pinned branch's documented CMake path can be used after
installing its OpenGL and X11 development dependencies:

```sh
git clone https://github.com/fknfilewalker/Quake-III-Arena-R.git quake3-opengl
git -C quake3-opengl checkout c95f4cecf7e0fec92e275d06645d5f2d14db286c
git -C quake3-opengl submodule update --init --recursive
cmake -S quake3-opengl -B quake3-opengl-build
cmake --build quake3-opengl-build --config Release
```

The pinned upstream Linux CMake target also links legacy X11 libraries, so
current Linux distributions may need additional compatibility packages.

## Linux build helper

From the repository root, run:

```sh
sh ./scripts/build-opengl-legacy.sh
```

The helper uses the same pinned OpenGL-only upstream revision and initializes
its submodules. The upstream Linux CMake target links legacy X11 libraries
(including Xxf86dga and Xxf86vm); install the corresponding OpenGL/GLU and X11
development packages first. This old Linux dependency set may require
compatibility packages on modern distributions.
