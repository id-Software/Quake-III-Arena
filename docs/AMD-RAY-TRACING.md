# AMD GPU ray-tracing support

## Status

This document defines the implementation target for an optional AMD hardware
ray-tracing renderer. **AMD hardware ray tracing is not implemented yet.**
The current checked-in renderer under `code/renderer` is the original
OpenGL renderer. It does not create Vulkan acceleration structures, issue
ray-tracing dispatches, or compile ray-tracing shaders. A setting or GPU-name
check alone would not provide real ray tracing.

## Target implementation

Add a separate, optional Vulkan renderer backend while preserving the existing
OpenGL renderer as the compatibility path.

The first hardware path should use the Vulkan KHR ray-tracing interfaces:

- `VK_KHR_acceleration_structure`
- `VK_KHR_ray_tracing_pipeline`
- `VK_KHR_deferred_host_operations`
- `VK_KHR_buffer_device_address`
- `VK_KHR_spirv_1_4` and `VK_KHR_shader_float_controls` where required
  by the selected Vulkan version and driver

AMD GPUs must be selected by queried Vulkan device capabilities, not by
vendor-name checks alone. AMD RDNA 2 and newer GPUs commonly provide hardware
ray tracing, but the driver must expose the required extensions and feature
bits. Unsupported AMD GPUs, other vendors, and systems without a suitable
Vulkan driver must continue to use the OpenGL renderer.

## Renderer design

1. Keep the OpenGL renderer buildable and selectable independently.
2. Introduce a Vulkan renderer module with explicit device/queue/swapchain
   lifetime management and a capability report.
3. Translate Quake III scene data (world surfaces, brush models, entities,
   transforms, materials and lightmaps) into GPU buffers and BLAS geometry.
4. Build/update a TLAS for visible scene instances; rebuild only when geometry
   changes and update instance transforms for moving entities.
5. Add ray-generation, miss, closest-hit and shadow-miss shaders. Start with
   hard shadows and one-bounce reflections as optional effects.
6. Composite the ray-traced result with the existing material/lightmap
   pipeline. Preserve legacy shader behavior and provide a raster fallback
   for unsupported materials and effects.
7. Expose runtime controls only after the backend is functional:
   `r_rayTracing 0|1`, `r_rayTracingShadows 0|1`,
   `r_rayTracingReflections 0|1`, and `r_rayTracingDebug 0|1`.
   The default must remain off until the backend passes validation.

## Acceptance criteria

- Vulkan feature/extension detection is performed at runtime.
- An AMD GPU is reported as hardware-RT capable only when all required Vulkan
  features are enabled successfully.
- The renderer falls back cleanly to OpenGL if Vulkan or RT features are absent.
- No game-server, demo, PK3, or network protocol changes are required.
- Validation covers AMD RDNA 2+ hardware, a non-RT GPU, and a system with no
  Vulkan support.
- Performance is measured separately for acceleration-structure build/update,
  ray dispatch, and denoising/compositing.

## Build and driver prerequisites

The eventual Vulkan backend will require the Vulkan SDK headers/tools at build
time and a Vulkan-capable AMD driver at runtime. The exact minimum driver
versions and GPU list must be documented only after the backend is built and
tested against the supported AMD driver matrix. The legacy OpenGL build must
not acquire a mandatory Vulkan SDK dependency.
