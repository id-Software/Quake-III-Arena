# NVIDIA RTX renderer starting point

This fork now tracks the **ReForged** branch of
[fknfilewalker/Quake-III-Arena-R](https://github.com/fknfilewalker/Quake-III-Arena-R)
as a pinned Git submodule at `vendor/Quake-III-Arena-R`.

- Upstream branch: `reforged`
- Pinned upstream commit: `f85ead64d13be64196aa665325e086337bd16caf`
- Upstream project description: Vulkan RTX path-tracing implementation for Quake III
- Upstream source README: [Quake III Arena R](https://github.com/fknfilewalker/Quake-III-Arena-R/tree/reforged)

## Checkout

From the root of this repository:

```sh
git submodule update --init --recursive
```

The submodule is pinned to a specific commit so builds are reproducible and do
not silently follow upstream changes.

## Upstream build notes

The pinned upstream README documents a CMake build and a Windows Visual Studio
generator. Install CMake 3.14 or newer, Visual Studio 2019 C++ build tools, and
the Vulkan SDK, then run:

```powershell
cmake -S vendor/Quake-III-Arena-R -B build-rtx -G "Visual Studio 16 2019"
cmake --build build-rtx --config Release
```

These are upstream build instructions, not a build verified by this repository.
The upstream CMake configuration checks for Vulkan on Windows, while its README
only documents the Windows build path. Linux and macOS RTX builds are not
claimed as supported or tested here.

## Runtime data

The upstream project requires the blue-noise texture directory from its source
tree to be copied into the user's Quake III installation under
`baseq3/blue_noise_textures`. It also documents setting `fs_basePath` to the
Quake III installation and disabling the bytecode VM for its native modules:

```text
+set fs_basePath "C:\\Path\\To\\Quake III" +set sv_pure 0 +set vm_game 0 +set vm_cgame 0 +set vm_ui 0
```

Use legally obtained Quake III game data; game assets are not included here.

## Important integration status

This commit adds the upstream renderer as a separately pinned source dependency.
It does **not** merge the upstream renderer into this repository's original
`code/` tree, change the default executable, or claim that ray tracing is
already running in the AI-agent edition. The next engineering stage is to
reconcile the upstream engine refactor with this fork's AI gateway and then
build and test the combined target.

The original Quake III renderer should remain available as a fallback for
non-RTX GPUs. RTX mode requires a compatible Vulkan ray-tracing device and
driver; the upstream implementation's actual hardware/driver matrix must be
validated on target machines.
