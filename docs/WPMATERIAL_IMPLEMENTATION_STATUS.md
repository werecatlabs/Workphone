# Material optimization implementation status

19 October 2026. Tracks [WPMATERIAL_OPTIMIZATION_PLAN.md](WPMATERIAL_OPTIMIZATION_PLAN.md).

## Implemented runtime changes

- `MaterialShaderParameterLayout` owns immutable parameter metadata, defaults, a sorted stable-ID lookup table and cached built-in bindings. Registered shader instances retain this layout instead of copying names/descriptors. Instance copies retain independent value bytes.
- Parameter handles carry a unique layout identity and slot. Typed resolution/access distinguishes float, signed/unsigned integer, bool, colour, vector, matrix and resource kinds. Invalid/stale/cross-layout/type/size/offset operations are rejected in every build. Values remain packed CPU bytes accessed through copies, independent of GPU packing.
- Parameter instances expose effective-write revisions and name-lookup/storage counters. Built-in PBR bridges use cached semantic handles. Serialization iterates slots directly; names remain at the asset/editor boundary.
- Standard material settings have numeric IDs and table-indexed access. The existing named editor API resolves a setting once before traversing authored passes.
- `MaterialPassStateData` publishes a retained immutable `MaterialRenderState` when its outer write lock closes. Nested writes coalesce into one version; unchanged values reuse the current version. Cold authoring changes do not rebuild the runtime record. Existing `SafePtr` mutation paths participate without requiring each caller to maintain a second dirty flag.
- The five shader-name/path/keyword buffers live in a lazily allocated authoring block. They are absent from render snapshots and consume no text-buffer storage on an unconfigured pass.
- Materials cache primary-state selection by graph revision. Technique/pass creation, replacement, removal, generic child edits and pass/context lifecycle invalidate selection. New/unselected authored materials preserve their available primary pass behavior.
- DX11 mesh draws read one immutable snapshot for surface values, flags, sampler controls and texture objects. Standard and extension texture-unit edits update snapshots at edit time. Native views are obtained through the existing texture adapter, preserving delayed readiness, render targets and device-owned resource behavior.
- Native material storage separates compact surface values from optional texture names. Empty slots allocate no strings; populated names allocate their actual bounded lengths. The existing size-tagged C descriptor is reconstructed on demand, and a new size-tagged bulk surface API avoids copying texture-name storage during synchronization. Native synchronization skips unchanged snapshots/revisions.
- Compatible parameter values survive reordered/extended shader schemas and partial updates; type changes use new defaults. An invalid candidate layout preserves the previous shader/layout/value state. Runtime handles are never serialized.
- Typed pending values preserve metalness, roughness and opacity through first-pass creation and loading. Existing LOD coverage exposed the previously missing pending-metalness path; the fix extends the existing pending-value mechanism.

## Compatibility and lifecycle boundaries

Existing material file keys (`ShaderParam.*`, `ShaderTex.*` and editor groups), public material/pass/editor methods and C descriptors remain supported. C++ consumers must rebuild against the changed headers and DLLs. Shader-name access within `MaterialPassStateData` now uses accessors over its private authoring block.

`Scalar`, `Vector2` and `Vector4` describe floating-point values explicitly. Integer shader parameters use `Int`, `UInt`, `IntVector2` or `IntVector4`; accidental same-size reinterpretation is rejected. Manually constructed offset/stride handles must be replaced with resolved layout-qualified handles. Unregistered custom layouts can be shared explicitly through `Reset(layout)`; registered shader instances share them automatically. Registry descriptor pointers still must not be retained across registration changes; retained layout references have independent lifetimes.

The current state-context write-lock boundary is the publication boundary. Direct writes to public numeric state fields must use the existing `SafePtr`/write-lock convention; detached authoring code can explicitly publish after initialization. Named/typed state setting methods also publish when used outside a nested write scope. Rendering retains immutable versions, so readers can finish using an older snapshot while edits publish a new one. There is no global history retaining every edit.

Snapshots retain CPU texture objects, not native SRV pointers. They therefore do not replace the texture/resource manager, introduce a competing device cache or require materials to know when a texture view is recreated. Native texture-view lookup/preparation remains a separate optimization and validation boundary.

## Validation

- Rebuilt Debug `WorkphoneGraphicsShaderTests`: passed `WorkphoneGraphics.shader_contract`, including new compact-storage, actual texture-name allocation, clone/reset, descriptor round-trip, unchanged-revision and short-descriptor-buffer contracts. Empty native storage is **264 bytes**; the preserved public legacy descriptor alone is **2,172 bytes**.
- Parameter-layout/snapshot contracts pass in Debug and RelWithDebInfo, including failed-operation checks that do not depend on assertions.
- Final Debug material selection: **90/90 cases, 1,890/1,890 assertions passed**, covering all four `GraphicsMaterial*` suites and the global material lifecycle/pixel cases. This is the material selection from `UnitTests`, not the complete 1,592-case module.
- Rebuilt material load-sequence integration passes, including pre-load values, direct pass/state edits, texture-unit load order, extension slots, primary-pass removal, schema migration/rejection and typed property round-trips.
- Rebuilt Claw production rendering passes on **Microsoft Basic Render Driver/WARP**, feature level `b000`, driver `10.0.19041.7802`. No representative hardware capture/performance claim is made.
- The rebuilt Debug `Editor.all` aggregate passed **66/66 cases and 2,258,833 assertions** in 627.21 seconds. Its CRT report retained the known 16-allocation/1,056-byte diagnostic baseline. That aggregate run preceded the final pending-value correction; the final binaries were rebuilt and the material-focused suites/load/render contracts rerun afterward. The full Editor aggregate was not repeated after that small lifecycle correction. This is not a GPU/custom-allocator leak certification or an interactive Editor quality/performance certification.

New coverage is in `MaterialParameterLayoutTests.cpp` and `MaterialLoadSequenceTests.cpp`: shared metadata, independent clones, typed/default resource values, forged/wrong/stale handles, schema replacement, no-op revisions, coherent publication, concurrent readers, direct pass/state edits, extension texture updates, cached graph selection and unload.

The optional parameter benchmark compares the old copied-metadata/linear-name-scan representation with shared layouts/cached handles for 10,000 instances and 8/32/128 scalar parameters. It reports accounted object/container/string capacity bytes and seven-run median update time, with observable checksums and value parity. This is a parameter-layer fixture; allocator bookkeeping, complete material graphs, GPU/device resources and whole-frame timings are outside that measurement.

RelWithDebInfo benchmark, with the other validation workloads idle:

| Parameters per instance | Old accounted bytes | Shared accounted bytes | Old median update loop | Handle median update loop |
| --- | ---: | ---: | ---: | ---: |
| 8 | 11,360,000 | 1,041,480 | 40.0944 ms | 13.4241 ms |
| 32 | 43,040,000 | 2,005,032 | 167.161 ms | 13.8978 ms |
| 128 | 232,760,000 | 5,865,540 | 750.252 ms | 14.5442 ms |

Each timed loop updates all 10,000 instances 20 times; each median uses seven samples. The 32-parameter fixture accounts for **95.3% less parameter storage** and about **12 times faster updates**. These numbers compare a reference reproduction of the prior parameter representation with cached-handle access; they are not a before/after scene benchmark. Empty pass storage changes from **3,080 bytes** to **736 bytes plus a 456-byte immutable snapshot**, excluding shared-pointer control/allocator overhead and optional authoring data. The benchmark includes a frozen reference to the former pass field layout for this comparison.

Reproduction: build `WPMaterialParameterLayoutTests` in `RelWithDebInfo` and run it with `--benchmark`. Local outputs are retained in `project_x64/material_parameter_benchmark_final.txt`, `material_native_test.log`, `material_unit_tests_final.log`, `material_unit_assertions.log`, `material_focused_tests_final.log` and `material_validation_tests.log`. The last file records the five-test run including the Editor aggregate before the pending-value correction.

## Remaining plan gates

This is an integrated CPU storage/access and active DX11 consumption change, not completion of all production gates in the plan. Remaining work includes:

1. Whole-material/scene allocation and cache profiling, changed-material stress workloads, multiple-view hardware timing and visual/Editor soak certification.
2. Removing the transitional legacy/data-driven value mirror, and introducing the final dense resource-record table/draw-packet ownership so repeated draws also avoid per-draw resource reference acquisition.
3. Scheduling one material publication per graphics update across multiple independent high-level setters, beyond the current coalescing within a state write scope.
4. Backend-specific prepacked constants/binding maps and optional view/material/object constant-buffer separation, guided by upload measurements.
5. A versioned binary cooked material schema and full automatic live shader/program reload coordination. The current file/package formats and their load adapters remain in use.

No claim of a complete frame-time improvement, native-view-cache removal, GPU speedup or full-plan certification follows from the parameter microbenchmark or a successful build.
