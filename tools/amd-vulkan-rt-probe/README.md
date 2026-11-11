# AMD Vulkan ray-tracing capability probe

Build with CMake 3.16+, a C++11 compiler, and a Vulkan 1.2 SDK/loader:

```sh
cmake -S . -B build
cmake --build build --config Release
```

Run `q3-amd-vulkan-rt-probe` (or `.exe` on Windows). Exit code 0 means at
least one AMD device passed the capability checks. Other exit codes identify
missing Vulkan/device support or an AMD GPU without the required feature set.

The probe checks AMD vendor ID, Vulkan 1.2, buffer device address, the
descriptor-indexing features required by the engine's bindless material path,
and the KHR acceleration-structure, ray-tracing-pipeline, and deferred-host-
operations extensions. It does not use NVIDIA's NV ray-tracing API.

This is a capability probe, not the Quake III renderer. It does not create
acceleration structures or trace rays. The current ReForged renderer
submodule uses NVIDIA-specific `VK_NV_ray_tracing`; a full KHR renderer
migration and runtime validation are still required before AMD in-game ray
tracing can be selected.
