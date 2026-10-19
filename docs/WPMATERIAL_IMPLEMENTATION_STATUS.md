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
- DX11 prepares material constants, packed-map slot selection and sampler controls in a bounded cache per renderer. Repeated submeshes, shadow passes and views reuse preparation while the snapshot remains cached. The cache has 128 entries in four-way sets, uses no strings or per-miss allocation, and compares weak ownership as well as addresses to reject address reuse. It neither owns snapshots/textures nor stores device views, and is released with the renderer. Snapshot texture objects are borrowed during draws instead of incrementing their reference counts for each texture lookup.
- Cached DX11 constants contain only eight material vectors. Lookup tags sit together within each set, separate from prepared values. A last-used index handles consecutive draws of the same snapshot; other warm hits compare addresses and ownership identities without inspecting strong-reference counts. Draw composition copies three contiguous material groups and initializes the view/draw fields explicitly. The shader constant-buffer format remains unchanged.
- Roughness modes 2 (metallic alpha) and 4 (inverted smoothness alpha) retain their established metallic-texture slot 2 routing. The initial preparation-cache integration had incorrectly left those modes on the separate roughness slot 3; this continuation restores the original draw-path behavior. All six authoring modes now have explicit CPU and renderer-adapter routing coverage, including repeated cached draws and delayed native-view readiness.
- Native material storage separates compact surface values from optional texture names. Empty slots allocate no strings; populated names allocate their actual bounded lengths. The existing size-tagged C descriptor is reconstructed on demand, and a new size-tagged bulk surface API avoids copying texture-name storage during synchronization. Native synchronization skips unchanged snapshots/revisions.
- Compatible parameter values survive reordered/extended shader schemas and partial updates; type changes use new defaults. An invalid candidate layout preserves the previous shader/layout/value state. Runtime handles are never serialized.
- Typed pending values preserve metalness, roughness and opacity through first-pass creation and loading. Existing LOD coverage exposed the previously missing pending-metalness path; the fix extends the existing pending-value mechanism.

## Compatibility and lifecycle boundaries

Existing material file keys (`ShaderParam.*`, `ShaderTex.*` and editor groups), public material/pass/editor methods and C descriptors remain supported. C++ consumers must rebuild against the changed headers and DLLs. Shader-name access within `MaterialPassStateData` now uses accessors over its private authoring block.

`Scalar`, `Vector2` and `Vector4` describe floating-point values explicitly. Integer shader parameters use `Int`, `UInt`, `IntVector2` or `IntVector4`; accidental same-size reinterpretation is rejected. Manually constructed offset/stride handles must be replaced with resolved layout-qualified handles. Unregistered custom layouts can be shared explicitly through `Reset(layout)`; registered shader instances share them automatically. Registry descriptor pointers still must not be retained across registration changes; retained layout references have independent lifetimes.

The current state-context write-lock boundary is the publication boundary. Direct writes to public numeric state fields must use the existing `SafePtr`/write-lock convention; detached authoring code can explicitly publish after initialization. Named/typed state setting methods also publish when used outside a nested write scope. Rendering retains immutable versions, so readers can finish using an older snapshot while edits publish a new one. There is no global history retaining every edit.

Snapshots retain CPU texture objects, not native SRV pointers. They therefore do not replace the texture/resource manager, introduce a competing device cache or require materials to know when a texture view is recreated. Native texture-view lookup/preparation remains a separate optimization and validation boundary.

The DX11 preparation cache is owned by the graphics thread. Its fixed object storage is **24,848 bytes per renderer that draws snapshot-backed materials**, with at most 128 weak ownership records. This is 42.6% less than the initial 43,272-byte cache; each prepared value falls from 312 to 168 bytes. Weak references can retain shared-pointer control/allocation storage until eviction or renderer destruction; they do not retain the snapshot's live texture references. Cache collisions cause correct re-preparation rather than growth. Lighting, camera, fog, reflection environment and actual texture readiness are composed per draw and are absent from cached constants. A material edit publishes a different snapshot, which is prepared independently even if its pass-local revision equals another material's revision. Native-view lookup and the existing combined constant-buffer upload path remain active.

## Validation

- Rebuilt Debug `WorkphoneGraphicsShaderTests`: passed `WorkphoneGraphics.shader_contract`, including new compact-storage, actual texture-name allocation, clone/reset, descriptor round-trip, unchanged-revision and short-descriptor-buffer contracts. Empty native storage is **264 bytes**; the preserved public legacy descriptor alone is **2,172 bytes**.
- Parameter-layout/snapshot contracts pass in Debug and RelWithDebInfo, including failed-operation checks that do not depend on assertions.
- Final Debug material selection: **90/90 cases, 1,890/1,890 assertions passed**, covering all four `GraphicsMaterial*` suites and the global material lifecycle/pixel cases. This is the material selection from `UnitTests`, not the complete 1,592-case module.
- Rebuilt material load-sequence integration passes, including pre-load values, direct pass/state edits, texture-unit load order, extension slots, primary-pass removal, schema migration/rejection and typed property round-trips.
- Rebuilt Claw production rendering passes on **Microsoft Basic Render Driver/WARP**, feature level `b000`, driver `10.0.19041.7802`. No representative hardware capture/performance claim is made.
- The rebuilt Debug `Editor.all` aggregate passed **66/66 cases and 2,281,009 assertions** in 622.31 seconds against the final runtime changes, including compact DX11 preparation, the roughness routing correction and the earlier pending-value correction. Its CRT report retained the known 16-allocation/1,056-byte diagnostic baseline. This is not a GPU/custom-allocator leak certification or an interactive Editor quality/performance certification.

New coverage is in `MaterialParameterLayoutTests.cpp` and `MaterialLoadSequenceTests.cpp`: shared metadata, independent clones, typed/default resource values, forged/wrong/stale handles, schema replacement, no-op revisions, coherent publication, concurrent readers, direct pass/state edits, extension texture updates, cached graph selection and unload.

The DX11 preparation continuation adds 20,000 repeated CPU accesses with exactly one preparation and unchanged snapshot ownership, plus changed/default/UI/sampler/channel cases, same-address distinct-owner checks, retired-snapshot release, and three passes over 384 retained snapshots to exercise bounded replacement. The production-render regression alternates red/green material edits over 16 UI draws with changing view lighting, then verifies a lit material responds to two lighting configurations without a material edit, using GPU readback. These contracts establish reuse and correctness, not a frame-time speedup.

For the compact-cache continuation, rebuilt Debug `UnitTests`, `Editor`, material load-sequence and production-render targets, and the RelWithDebInfo load-sequence target. The four Debug parameter/layout/load/native-shader/render CTest contracts pass; RelWithDebInfo load-sequence/preparation contracts also pass. Re-ran the 90-case material UnitTests selection and the complete Editor aggregate after the final roughness routing correction. Current outputs are in `project_x64/material_cache_compact_debug_contracts.log`, `material_cache_compact_rwi_contracts.log`, `material_cache_compact_unit_tests.log` and `material_cache_compact_editor_all.log`. Full device/readback, optimized contracts and Editor/assertion/CRT output is preserved separately in `material_cache_compact_debug_evidence.log`, `material_cache_compact_rwi_evidence.log` and `material_cache_compact_editor_evidence.log`.

## Compact DX11 cache measurements

`MaterialRenderCacheReference.hpp` preserves the initial cache representation and lookup for comparison, with the roughness routing correction applied to both implementations. `MaterialRenderCacheBenchmark.hpp` compares it with the compact implementation using the same retained snapshots and bucket occupancy. Debug and RelWithDebInfo contracts compare every byte of the composed native block, texture mappings and sampler values over 768 accesses, including bounded eviction. Null defaults, copy/move after the last-use hit, snapshot retirement and same-address/different-owner protection are also covered. The CPU suite independently checks all six roughness source slots; the renderer suite verifies the requested texture owners for all six modes on both new and cached draws.

RelWithDebInfo on Intel Core i9-9820X, Windows reporting 20 logical processors, with the other validation workloads idle:

| CPU workload | Snapshots | Former median | Compact median | Preparations in each implementation |
| --- | ---: | ---: | ---: | ---: |
| 64 consecutive draws per material, one snapshot per set | 32 | 52.8964 ms | 46.3426 ms | 32 |
| Four colliding snapshots, alternating hits | 4 | 53.7419 ms | 53.1406 ms | 4 |
| Eight snapshots per four-way set, alternating capacity misses | 256 | 126.189 ms | 87.7048 ms | 1,000,256 |

Each sample performs 1,000,000 lookups and full draw-block compositions, view-field assignments and an out-of-line checksum reading all submitted bytes. Each median uses seven samples with alternating execution order. Both implementations use identical inputs, produce identical checksums and perform identical numbers of preparations. Counts include prewarming outside the timer. Grouped draws improve about 12% and capacity misses about 30% in this fixture; the four-way warm case is close to the former result. This measures CPU preparation/composition with checksum overhead, excluding graph selection, native texture resolution, GPU submission, complete material/scene allocation, hardware cache counters and whole-frame timing. Fixed storage sizes exclude weak-pointer-retained control/allocation storage, which has the same 128-entry bound.

Reproduction: build `WPMaterialLoadSequenceTests` in `RelWithDebInfo` and run it with `--benchmark-preparation`. Output is in `project_x64/material_cache_compact_benchmark.txt`; final Debug and optimized contracts are in `material_cache_compact_debug_contracts.log`, `material_cache_compact_debug_evidence.log`, `material_cache_compact_rwi_contracts.log` and `material_cache_compact_rwi_evidence.log`. The final 90-case material selection is in `material_cache_compact_unit_tests.log`.

## Parameter storage measurements

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
4. Backend shader-program binding maps and optional view/material/object constant-buffer separation, guided by upload measurements. Built-in DX11 mesh constants and semantic texture-slot maps now have bounded preparation caching; the combined per-draw submission remains.
5. A versioned binary cooked material schema and full automatic live shader/program reload coordination. The current file/package formats and their load adapters remain in use.

No claim of a complete frame-time improvement, native-view-cache removal, GPU speedup or full-plan certification follows from the parameter microbenchmark or a successful build.
