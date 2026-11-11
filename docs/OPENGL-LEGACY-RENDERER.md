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
- Does **not** currently make the upstream CMake build independent of Vulkan:
  the pinned upstream CMake configuration still discovers and links Vulkan
  as a required build dependency. The current Windows build script therefore
  still requires the Vulkan SDK to compile the combined renderer.
- Does not guarantee support for very old OpenGL implementations. Actual
  compatibility depends on the OpenGL version/extensions and driver expected
  by this renderer revision. No legacy-hardware test has been performed.

This is an OpenGL runtime choice in the existing combined renderer, not yet a
separate Vulkan-free legacy build target. A truly Vulkan-independent build
needs a source/build-system change upstream that conditionally excludes the
Vulkan renderer sources, headers, and link dependency. Do not treat the
OpenGL launch option as proof that this has been completed.
