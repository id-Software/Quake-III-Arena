# AMD GPU ray-tracing support

## Current implementation status

The ReForged engine submodule already contains a Vulkan ray-tracing renderer,
scene geometry upload, acceleration-structure management, ray-tracing
pipelines, and ray-tracing shaders. Its acceleration structures, pipeline
stages, dispatch, shader binding table, and buffer usage currently use the
NVIDIA-specific `VK_NV_ray_tracing` API. That is not a portable AMD path.

This repository now includes a standalone AMD Vulkan KHR capability probe in
`tools/amd-vulkan-rt-probe`. It checks AMD vendor identity, Vulkan 1.2,
buffer-device-address and descriptor-indexing features, and the KHR
acceleration-structure, ray-tracing-pipeline, and deferred-host-operations
extensions.

**The game renderer has not yet been ported to KHR and does not yet provide
AMD in-game ray tracing.** Passing the probe is necessary but not sufficient.

## Required renderer port

The existing NV path must be converted to KHR equivalents in the engine
submodule, including:

- KHR acceleration-structure creation, build-size queries, device-address
  retrieval, build/update commands, and instance records;
- KHR buffer usage and device-address allocation for geometry, scratch,
  acceleration-structure, and shader-binding-table buffers;
- KHR acceleration-structure descriptor writes;
- KHR ray-tracing pipeline creation, shader group handles, shader-binding-table
  alignment/regions, and `vkCmdTraceRaysKHR` dispatch;
- KHR shader-stage identifiers and synchronization/barrier stage/access masks;
- runtime feature-chain enablement and extension negotiation without requiring
  NVIDIA extensions on AMD devices.

Keep the NV implementation only as an optional legacy backend. OpenGL remains
the compatibility fallback.

## Capability probe

See [tools/amd-vulkan-rt-probe/README.md](../tools/amd-vulkan-rt-probe/README.md)
for build and execution instructions. A passing result confirms advertised
device/driver features only; it does not certify the engine renderer port.
