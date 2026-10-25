# Material system implementation plan

Date: 19 October 2026. Original planning baseline; current implementation and evidence are tracked in [WPMATERIAL_IMPLEMENTATION_STATUS.md](WPMATERIAL_IMPLEMENTATION_STATUS.md).

Implementation checkpoint: shared layouts, typed access, immutable pass snapshots, active DX11 consumption, compact native storage, persistent generation-checked material records shared across frames/views and direct Claw adapters, selectable sorting strategies, cached numeric bindings in the named-texture adapter and optional measured DX11 surface/view/binding uploads are now implemented. Combined GPU uploads remain the default; the split policy reduces copied bytes for partial changes but showed no CPU speedup in the WARP submission fixture. See [implementation status](WPMATERIAL_IMPLEMENTATION_STATUS.md) for validation evidence and remaining production gates; the findings and estimates below record the original planning baseline.

Latest lifecycle validation: resource-cache cleanup and repeated actor/camera teardown are repaired. The rebuilt Debug Editor aggregate passes 66/66 cases; selected material suites pass in Debug and RelWithDebInfo. See the implementation status for exact evidence and the remaining production-scene/hardware and cooked-schema gates.

M4 storage continuation: compatible PBR values now seed future passes directly from the shader buffer, retiring the redundant pending opacity/metalness/roughness copies. Legacy and fallback values use optional masked storage until compatible ownership transfers. Explicit import preserves pre-load edits and reads live values inside batches. This is a conditional object-storage reduction; non-PBR migration and measured whole-material/scene memory remain open. See the implementation status for costs and validation.

Unlit continuation: declared emission and opacity now share shader ownership across material/named/typed edits and seed newly attached passes directly. Compatible opacity no longer retains a future-pass legacy copy. Saved native aliases preserve pass-local overrides. ColorOnly, remaining fallback/storage adapters and whole-material/scene profiling remain open; M4 is partial.

ColorOnly continuation: declared Color and Opacity now share the existing shader value owner across material/typed/named edits and new-pass seeding. Color supplies diffuse RGB/emissive RGBA; opacity controls diffuse alpha. Saved aliases retain independent pass overrides, and partial/wrong-type schemas keep their fallback paths. Remaining physical storage/adapter work and measured whole-material/scene memory are still open; M4 remains partial.

Root storage continuation: the unused texture/name/path/PBR payload is removed from the root `MaterialStateData`. Its registered lifecycle marker, factory/context ownership and teardown remain; active values stay in existing material/pass/shader owners. C++ clients must rebuild and migrate direct access to the removed root fields. See the implementation status for measured object-storage evidence and remaining whole-material/scene memory gates.

Pass storage continuation: obsolete per-texture dirty flags and their adapter writes are removed. Immutable render-state publication detects binding changes directly and preserves unchanged snapshots. C++ pass-state consumers must rebuild; measured storage and regression evidence are tracked in the implementation status. Whole-material/scene and hardware gates remain open.

Numeric storage continuation: 20 additional non-rendered pass settings now share the existing lazy numeric authoring block. Runtime snapshots retain their surface/binding representation; render-mode seeding adapters use typed setting access. Default passes avoid the inline values while nondefault authoring pays the larger optional block. See implementation status for measured inventory and compatibility evidence.

Sparse numeric continuation: nondefault cold settings now use a packed mask/value allocation rather than copying the full numeric default block. Existing override updates stay in place; insertions/removals repack during authoring. Empty/default passes retain no cold allocation. Dense and sparse storage tradeoffs and validation are recorded in the implementation status.

Native name storage continuation: the C material's descriptor-slot name pointers now use an optional packed table with presence masks. Unnamed materials avoid the table, and unchanged slot strings retain borrowed-pointer stability through other edits. Public C descriptors remain unchanged; sparse/dense costs and failure/compatibility evidence are tracked in the implementation status.

Pass string continuation: optional shader names, path and keywords now use a single packed text allocation rather than five maximum-sized fixed buffers. Empty fields use schema defaults and the last clear frees the allocation; existing bounded accessors and serialized keys remain. See implementation status for storage and validation evidence.

Texture-unit name continuation: bound names now come from the texture resource rather than a second inline Claw string. Only unresolved paths retain an optional string in the shared base owner. Named resolution publishes the resulting resource directly, and base-qualified binding/clear calls retire pending names consistently. Unresolved descriptors and unload paths retain authoring names; base resource-picker properties and Claw path properties keep their existing schemas. See implementation status for object inventory, cold-allocation tradeoffs and validation; M4 remains partial.

Final M4 storage migration checkpoint (23 October): the permanent mutable pass surface/texture copy is retired in favor of the current immutable snapshot plus an edit-time candidate. Attached texture facades share canonical bindings; texture metadata and native callback registrations use optional cold storage. Scalar fallback handoff covers shader clear/schema changes and mutable parameter-buffer replacement. C++ raw pass-state fields are replaced by sealed/typed APIs and clients must rebuild; public C descriptors/functions and serialized properties remain supported. Debug/RelWithDebInfo focused suites pass 8/8 each, the selected Debug suite passes 105 cases/1,899 assertions, the rebuilt Debug Editor aggregate passes 66/66 cases, and the 10,000-material storage/retirement fixture passes. This completes the planned storage implementation and automated compatibility validation, while measured whole-material allocation targets, representative hardware/cache timing and interactive visual/Editor acceptance remain open. See the final migration section in the implementation status for costs, evidence and exclusions. Earlier continuation notes above record their respective checkpoints.

M4 full-allocation acceptance checkpoint (23 October): the original pre-optimization revision `9572201fa8b6100b4d53a0ffc2a239133aa100ea` and the integrated optimized runtime now run the same fresh-process 10,000-material workload for 8/32/128 mixed parameters, three repetitions each. Actual shared-CRT heap and independent process-private commitment include graph owners, shared schema/prototype, handles, controls, contexts/listeners and retained capacity. The first matched run missed the target (48.06% heap saving at 128 slots); profiling exposed 300,000 eagerly created idle task queues. Queues now initialize on first message, with unload generations, custom virtual dispatch and concurrent first-writer regression coverage. The final 18-process matrix passes every 50% threshold: conservative graph heap savings are **79.69-83.82%**, private-commit savings **79.48-83.73%**, and the smallest reduction charging all fixture startup is **73.80%**. Focused suites pass **11/11 in both builds**, the selected Debug suite passes **168 cases / 5,816 assertions**, and the final rebuilt Editor aggregate passes **66/66 cases / 2,781,724 assertions**. Peak/pending/retired memory, raw logs, frozen binary/source hashes, matched build metadata and exact validation are recorded in [implementation status](WPMATERIAL_IMPLEMENTATION_STATUS.md). This closes M4's controlled-fixture memory target; production-scene residency, fully materialized authoring/GPU resources, hardware/cache/frame timing and interactive soak remain separate M0/M6 gates. Earlier checkpoint statements about the memory target are superseded by this measurement.

Production-scene validation checkpoint (23 October): the existing procedural racing sample now has opt-in typed owner telemetry, separate heap/private-commit and heap-walk-free CPU modes, frozen-runtime profiling, strict rendering/occlusion guards and resumable evidence. All **12 required processes pass: three nominal 60-second memory runs and three nominal 30-second CPU repetitions per car/track/follow view**. Warmed whole-process heap is **1597.1-1620.8 MiB** and private commitment **1897.8-1930.4 MiB**. Three-run median material preparation is **123.257 / 133.440 / 124.737 microseconds per update** for car/track/follow, with zero new material record preparations and zero material preparation-workspace allocations during accepted intervals. Presentation recovered on the unchanged frozen runtime; all three resumed CPU runs have zero occluded/failed Presents, and earlier rejected evidence remains intact. A separate observer caught a startup Windows ghost window that cleared before measurement; it does not establish the cause of the earlier persistent occlusion. These are VM/WARP diagnostics, with hardware counters unavailable. M4's controlled memory target remains complete. Matched original/current CPU acceptance, hardware/cache/frame-p95 comparisons, longer growth/retirement, presentation robustness and interactive/device/visual certification remain open M0/M6 work; M5 backend/cooked-schema/reload work also remains. See the production-scene section in [implementation status](WPMATERIAL_IMPLEMENTATION_STATUS.md) for exact evidence, residuals, rejected runs and reproduction.


M5 runtime checkpoint (25 October): the active Claw mesh path now activates explicit compiled vertex/pixel programs after native geometry setup, captures immutable packed publications and resolves numeric texture/buffer/sampler owners at submission. Both Debug and a frozen RelWithDebInfo runtime pass all six focused suites, including actual shader pixels, resource refresh/denial, output-feedback safety, state recovery, stale maps, warm full-budget submission and admission rollback. Runtime attachments are lazy and upload buffers shared per program. Automatic atomic runtime reload for fixed CPU schemas is now implemented in the next checkpoint. Versioned cooked schemas, asset/shader-ID/file-watcher/Editor installation and CPU-schema migration remain open, as do custom instanced vertex/shadow contracts and hardware/device/interactive certification. See the runtime activation section in [implementation status](WPMATERIAL_IMPLEMENTATION_STATUS.md) for the exact build, frozen-runtime and validation boundaries.


M5 reload checkpoint (25 October): `MaterialProgramReloadDX11` now observes attached source/binary/define revisions during material publication preparation, builds a private native/program/map candidate and stages every subscribed material before a nonallocating cohort commit. Failed compile/reflection/subscriber or CPU/Gpu admission keeps the last-good generation; descriptor edits, restored/removed blockers and recovered budget headroom retry automatically. Retained old draw views remain valid, warm polling allocates/compiles nothing, and private-generation leases retire exactly. Frozen Debug and RelWithDebInfo acceptance each passes all six focused suites with zero input drift. This closes runtime atomic coordination for fixed CPU schemas; file/include watching, generic shader/Editor status, schema migration and cooked payloads remain open. Exact evidence and boundaries are in [implementation status](WPMATERIAL_IMPLEMENTATION_STATUS.md).

M5 completion checkpoint (25 October): file/include polling and the existing Material Editor now queue shader configuration/reload/watch commands into the graphics-owner boundary with truthful status. Reflected CPU schemas, pass states and packed publications migrate together; frame capture pins a cohort. Versioned typed source-free WPMT3/compiler4 cooking, HLSL include snapshots/build keys, automatic runtime installation, generic ClawShader compilation/status, clone/save restoration and device replacement/reinstallation are implemented. Final frozen acceptance passes **12/12 Debug (106.79 s)** and **8/8 RelWithDebInfo (34.48 s)** with zero input drift and the original cold budget fixture/timeouts. This completes the original M5 gates for the DX11 VS/PS PNTC contract. Managed reloads rebuild on device change; immutable cooked resources require explicit source-free reinstallation through their resource owner. Custom vertex instancing/shadow contracts, persistent buffer assets and automatic engine-wide cooked-owner restoration remain extensions, while representative hardware/interactive/performance certification remains M0/M6. See the final M5 section in [implementation status](WPMATERIAL_IMPLEMENTATION_STATUS.md) for usage, limits, evidence and remaining gates.

## Objective and scope

Make material access predictable, compact and cache friendly: resolve names at authoring/load/bind time, retain typed values in contiguous storage, and submit pre-resolved material records during rendering. Extend the existing material/shader/state systems rather than creating a competing material implementation.

First delivery targets Windows x64 Claw/DX11, with backend-neutral CPU contracts and additive C interfaces. Preserve the native C89/C90 update/render contract, C++ SDK boundaries, authored material graphs, Lua/editor behavior and resource/package identity. Other renderers retain their adapters until separately migrated and validated.

This is the material-specific implementation of `OPT-05` / `PERF-06` in [WPGRAPHICS_OPTIMIZATION_PLAN.md](WPGRAPHICS_OPTIMIZATION_PLAN.md). Share its resource generations, immutable publication, draw packets and retirement services. Coordinate cooked payload changes with [WPRESOURCE_ASSET_PRODUCTION_PLAN.md](WPRESOURCE_ASSET_PRODUCTION_PLAN.md).

## Findings from the current source

| Area | Evidence | Consequence |
| --- | --- | --- |
| A contiguous parameter buffer already exists | [MaterialShaderParameters.cpp](../Engine/cpp/Source/Workphone/Graphics/MaterialShaderParameters.cpp), `Reset`, `FindParameter`; [header](../Engine/cpp/Include/Workphone/Graphics/MaterialShaderParameters.hpp), instance members | Extend this representation. Each instance currently copies `MaterialShaderParameterInfo`, including each `String m_name`, and linearly scans it for lookup. |
| Handles need stronger validation | Same files, `MaterialShaderParameterHandle`, `WriteBytes`, `ReadBytes` | Current C++ handles contain only stride/offset. They cannot identify another layout or an obsolete layout. Size equality is asserted but is not enforced by the release-mode guards; equal-size types are not distinguished. |
| Shared descriptors already have an owner | [MaterialShader.cpp](../Engine/cpp/Source/Workphone/Graphics/MaterialShader.cpp), registry and built-ins | Put finalized immutable layouts here. The current registry stores descriptors by value in an array and returns pointers; array growth/replacement must not invalidate layouts retained by instances. |
| Generic properties carry authoring metadata | [Property.hpp](../Engine/cpp/Include/Workphone/Core/Property.hpp), private members; [Properties.cpp](../Engine/cpp/Source/Workphone/Core/Properties.cpp), `getProperty`, `getPropertyObject`, `getPropertyValue` | Names use `FixedString<128>`, while values retain a dynamic string and attributes have their own array. Lookups scan the array; several typed reads first call `hasProperty`, then scan again. Scalar values have a numeric cache, so not every typed read reparses text. |
| Standard material pass state is already typed | [MaterialPassStateData.hpp](../Engine/cpp/Include/Workphone/State/States/MaterialPassStateData.hpp); [implementation](../Engine/cpp/Source/Workphone/State/States/MaterialPassStateData.cpp), field tables | Numeric values/flags are direct fields, but string dispatch scans field tables. The same object also includes preview, import/compression, shader paths and keywords. Larger fixed strings do not solve cache locality. |
| Material getters repeatedly traverse graphs | [Material.cpp](../Engine/cpp/Source/Workphone/Graphics/Material.cpp), `getTechniques`, `getTexture`, colour/PBR getters, `getEditor*`; [MaterialTechnique.cpp](../Engine/cpp/Source/Workphone/Graphics/MaterialTechnique.cpp), `getPasses` | Technique snapshots are returned by value; `getPasses` constructs another array. Individual value/texture queries repeatedly traverse these arrays and state contexts. |
| Runtime bridges still resolve names | [Material.cpp](../Engine/cpp/Source/Workphone/Graphics/Material.cpp), `syncPBRToShaderParameters`, `applyShaderParametersToRenderer`, `applyDataDrivenTextures` | Existing data-driven values are mirrored through legacy setters/getters; built-in names and shader IDs are compared repeatedly. Texture names/paths are searched separately. |
| Native synchronization reconstructs a large descriptor | [ClawMaterial.cpp](../Engine/cpp/Source/WPGraphics/ClawMaterial.cpp), `getNativeMaterial`, `syncNativeMaterial`; [C material header](../Engine/c/Include/WorkphoneGraphics/workphone_graphics_material.h) and [implementation](../Engine/c/Source/WorkphoneGraphics/workphone_graphics_material.c) | Native access calls synchronization, which fetches many individual values and several named editor settings. Eight 256-byte texture-name slots contribute 2,048 bytes to the stored descriptor. `set_desc` sanitizes and compares a full descriptor before deciding whether it changed. |
| Active DX11 draws still assemble material state | [ClawRendererDX11.cpp](../Engine/cpp/Source/WPGraphics/ClawRendererDX11.cpp), `getPrimaryMaterialPassState`, `renderMesh` | Draws resolve graph state, texture views, channel selectors, sampler settings and constants. Many fields are already read directly; strings alone do not explain this cost. |
| Native caches already suppress some redundant work | [DX11 implementation](../Engine/c/Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.c), `wp_renderer_dx11_set_material`, material dirty/upload handling | Preserve these caches. Moving C++ resolution out of drawing addresses work they cannot eliminate. |
| C shader handles already have validation machinery | [workphone_graphics_shader.h](../Engine/c/Include/WorkphoneGraphics/workphone_graphics_shader.h), `wp_shader_param_handle`; [implementation](../Engine/c/Source/WorkphoneGraphics/workphone_graphics_shader.c) | Reuse the owner/index/definition-revision model and existing typed/indexed binding APIs. Do not introduce a second native shader registry. |

These are structural findings, not a measured attribution of frame time or heap use. String literals and short `String` values do not necessarily allocate; measure actual allocations, copied bytes and call frequency before assigning savings.

## Proposed representation

### 1. A shared immutable layout per shader schema

Evolve `MaterialShader` / `MaterialShaderRegistry` to own finalized layouts with stable lifetime. An instance holds one layout reference, its typed value bytes, resource slots and revisions. It does not own a copy of parameter names, UI labels, defaults or parameter descriptors.

Layout hot metadata contains compact type, size, offset, count, semantic and binding information. Cold metadata contains canonical names, labels, ranges, tooltips and serialization aliases. Store default bytes once per layout and copy them at instance creation. Resolve built-in semantic handles once when finalizing a layout.

Use explicit, stable enum IDs for built-in material settings. For custom parameters, define a fixed-width stable parameter ID scoped to a shader schema; specify its encoding/hash algorithm and version for cooked data. Validate duplicate canonical names and ID collisions during registration/cooking and reject ambiguous layouts. Do not treat the existing configurable `hash_type` as a portable file-format identity. Never use a hash as a direct byte offset.

Keep a shared sorted ID-to-slot table initially. A name lookup adapter may hash/compare names at the boundary and use the cold metadata to verify identity. The steady-state path uses a cached slot/handle, so a hash table is unnecessary per instance. Preserve case-sensitive behavior and provide explicit aliases for intentional renames.

### 2. Typed, layout-qualified handles

Replace the C++ stride/offset-only contract with a handle that identifies a unique immutable layout version and parameter slot. Derive type/offset/size from the validated layout. Give each new layout version a non-reused runtime identity; a handle may be shared by instances of that exact layout, but must fail for another version.

Provide typed C++ wrappers such as `MaterialParameterHandle<float>` and distinct float/int/uint/bool metadata. Preserve the existing `Scalar` API as a compatibility adapter with an explicit scalar-kind rule. Preserve vector, colour, matrix, array and resource distinctions even where their byte sizes match.

Validation must work in Release: valid layout/slot, matching expected type, array length, source size and overflow-safe bounds. Invalid writes return a failure without mutation. Define deterministic failed-read results and diagnostics. Debug assertions are additional diagnostics. Use byte copies for packed CPU storage instead of casting potentially unaligned bytes to engine math objects.

Illustrative API intent, not an existing interface:

```cpp
// Resolve during component/material binding; retain with the layout version.
auto roughness = material.resolve<float>( MaterialParameterId::Roughness );

// Animation/game update: no string construction, hashing or metadata scan.
material.set( roughness, 0.35f );
```

Do not serialize runtime handles, pointers or process-local layout identities. Persist schema IDs/versions and parameter/resource identities, then rebuild runtime handles on load.

### 3. One authoritative value for each parameter

Initially retain `MaterialPassStateData` as the owner of legacy standard-pass values and remove lookup from its consumers. Introduce typed setting IDs over its existing fields and a shared schema describing IDs, names, defaults and normalization rules.

Built-in scalar, enum and flag controls should use their known numeric setting IDs directly. Resolve names only in named editor/serialization adapters, sharing numeric read/write helpers so validation and default behavior cannot diverge. Preserve available authored-pass lookup before a technique is selected and retain virtual pass setters for backend effects. Verify zero named-adapter dispatch from built-in controls, named API compatibility, multi-pass propagation and a same-work/publication benchmark.

The end state is one value owner per material pass: standard surface data has a compact typed representation; custom shader values use the existing contiguous parameter buffer. Built-in shader parameters alias the corresponding standard semantic values rather than maintaining an independently writable second copy. Shared layouts describe these storage locations. Custom parameters without a built-in semantic remain in their layout-owned value block.

Migrate ownership only after the resolved render path works. During transition, make each bridge directional for a specific operation and revision: legacy edit -> canonical values -> render snapshot, or shader edit -> canonical values -> legacy adapter. Record the origin/revision to prevent recursive synchronization and stale writes. Retire the temporary duplicate copy when all reads are adapted.

Route direct pass setters, editor changes, Lua, imports, clones and shader-parameter changes through the same mutation/invalidation contract. The mutable `getShaderParameters()` API is a migration hazard: introduce a scoped edit operation or have every mutator report changes to its owner; avoid retaining an untracked writable byte buffer.

Keep current material-wide setters applying to all applicable authored passes. Preserve the current primary available pass used by DX11, including materials with no selected technique. Cache that selection by graph revision. Adding/removing/reordering techniques/passes invalidates the selection. This work does not change the number of passes drawn.

### 4. Separate frequently used state from authoring data

Render-facing storage contains surface constants, numeric flags, pipeline/sampler keys, resource slot indices, layout identity and revisions. Preview rotation/exposure, compression settings, labels, source paths, shader source names and keyword text live in cold authoring metadata. Compile keywords to bounded numeric feature/permutation keys at load/edit time.

Keep `Properties` as the editor/serialization adapter during migration. Generate it on demand; do not use it as the material's render representation or retain a second property tree for every runtime instance. A global replacement of `Property` / `Properties` would expand this change into unrelated engine systems and is outside the first delivery.

Behind the opaque `wp_graphics_material`, split the retained native descriptor into compact surface state and cold texture identity storage. Preserve `wp_graphics_material_desc`, its `struct_size` behavior and existing entry points as import/export compatibility APIs; construct the descriptor on demand. Introduce an additive typed bulk-update API so routine surface edits do not copy the 2 KB name block. Verify the remaining callers before eliminating any retained compatibility fields.

Do not impose one universal 64-byte material or align every allocation to a cache line. Measure actual `sizeof`, allocations, padding and working sets. Start with a compact header plus contiguous material constants and resource slots; use page/slab allocation for numerous same-layout instances if allocator/profile evidence justifies it. Choose array-of-structures for records consumed together; introduce structure-of-arrays only for independently scanned fields. Avoid sparse overrides on the draw path: flatten them on edit/publication.

### 5. Publish resolved immutable render records

Implement the existing `PERF-06` material record once, owned by the graphics/resource lifecycle. Key a record by material handle/generation, pass/layout/content revisions, dependent texture generations and renderer/device generation. Keep resource identity distinct from native texture/SRV identity.

Build each changed record during the existing graphics state/resource update/publication stage. Resolve the primary pass once, read one coherent state snapshot, resolve semantic texture slots and samplers, and pack constants once. Draw packets retain a compact material handle/record index and read the prepared record; they do not call `getProperties`, `getEditor*`, `FindParameter`, `getTechniques`, `getPasses` or perform resource-name lookup.

Use a dense retained record table with generation-checked indices and free-slot reuse. Keep asset ownership/refcounts in the resource/record owner, rather than acquiring several smart pointers for every draw. Retain old record versions/resources until all queued consumers have finished using them. Track live bytes, retired bytes and reusable capacity separately, and bound pending publications so repeated edits cannot accumulate unbounded versions.

Dirty categories: surface constants, render state, resource bindings, layout and graph selection. Set a dirty category only for an effective value change after existing normalization. Queue each changed material once. An unchanged frame rebuilds zero records. A parameter-only edit does not reload textures or rebuild pipeline state.

Coalesce independent legacy, Lua/editor and typed edits at existing owning-thread update boundaries, retaining short state write locks. Commit before scene-job capture; rendering keeps the previous immutable version until then. Nested explicit material/parameter batches must retain their final write boundary. Keep standalone edits immediate, bound retained queue capacity, release temporary state owners after publication and test graph replacement/unload. Separate update threads and authored passes do not form a globally atomic frame transaction. The implemented boundary and measurements are tracked in the implementation status.

Publish completed snapshots at the existing state/render boundary. Retain the last valid record while asynchronous resources or a candidate shader are pending; publish complete replacements atomically at that boundary. Reuse the current state-context synchronization and graphics unload queue; no draw-time locks around mutable authoring objects. Shader reload migrates compatible values by stable ID/type and fills new slots from defaults; explicit conversions are versioned. A failed candidate preserves the old valid layout/record and produces an actionable diagnostic.

Texture replacement/streaming completion, sampler changes, slot removal, graph edits, unload/reuse, shader reload and device recreation must invalidate the correct records even when the material's numeric values are unchanged. Keep material handles and shader parameter handles separate: instance destruction invalidates the former; layout replacement invalidates the latter.

### 6. Adapt DX11 and the C shader path

First consume resolved records in `ClawRendererDX11::renderMesh` and `ClawMaterial::syncNativeMaterial`, retaining current native binding/dirty caches. Precompute the mapping between Workphone/Ogre PBS semantic slots and C/backend slots; do not replace the current mapping with texture declaration order.

The current `wp_material_dx11` combines material values with camera, lighting, fog and environment. First cache the material-only portion and compose the existing structure at submission without graph traversal. Then separate view/pass, material and object constant blocks if upload counters and profiles justify the shader/backend change. Camera/light changes must not rebuild every material record. Keep view-dependent texture transfer flags, shadow controls and environment selection correctly owned; a material record cannot bake whichever view happened to resolve it first.

Reuse `wp_shader_program` indexed/validated-handle APIs through an explicit layout-to-program binding map. Keep shared shader definitions separate from per-material values: the existing mutable program parameter buffer cannot be treated as independent storage for every material. Resolve/bind each instance through the backend adapter or an additive instance binding API. Cache bindings by schema/program definition revision and device generation. Do not upload the C++ CPU byte block directly as a GPU buffer without a verified packing map; reflected offsets, arrays, bools and matrices may differ.

This phase removes CPU material preparation overhead. It does not by itself reduce draw count. Existing compatible draw grouping can consume the new material key later; preserve transparent sorting, UI painter order and custom queue barriers.

### 7. Selectable draw sorting

Expose scene-level policies so content can trade material locality against early depth rejection:

| Policy | Opaque order within queue/z-order boundaries | Intended use |
| --- | --- | --- |
| QueueOrder | Existing stable submission order | Compatibility/default and explicit author ordering |
| FrontToBack | Camera-forward depth, then material | Reduce overdraw |
| MaterialFirst | Dense material ID, then depth | Group repeated material state |
| DepthBuckets | Depth range, then material, then depth | Balance locality and early depth rejection |

The first implementation uses captured numeric material IDs and object-origin view depth. All optimized policies put opaque draws before transparent draws and order transparency back-to-front within each eligible run. Equal keys preserve submission order. Queue groups, explicit z-order, UI/overlays, refraction, nonstandard depth comparisons, opaque materials without depth writes, unknown material state and mixed-material meshes constrain reordering. Missing cameras preserve the original order. Shadow submission retains its existing order.

Reuse contiguous key/permutation buffers with scene material storage. Reorder a private immutable cull view before publication, retaining shared captured transforms and resource ownership; never reorder a published consumer's array in place. Count the additional reordered-view allocations separately from reusable material-table storage. Validate pixel parity against explicit transparent ordering, retained views, policy changes, repeated keys and independent view lifetimes. Measure sorting CPU cost, adjacent material changes and depth reversals separately; require representative GPU/frame measurements before selecting an optimized default. Section-level packets and more precise transparent geometry ordering can extend this path later.

## Delivery sequence and review gates

Each row is a separately reviewable implementation slice. Estimates are provisional engineering days, excluding unavailable hardware and broad pre-existing build failures; refine after M0.

| Slice | Implementation and principal files | Depends on | Exit gate | Estimate |
| --- | --- | --- | --- | --- |
| M0: baseline | Add material-resolution/allocation/copy counters beside existing render statistics; reproducible CPU/material and scene fixtures in `Tests/cpp` | None | Repeated Debug correctness and RelWithDebInfo performance baselines, memory ownership breakdown and captured device/build provenance | 1-2 days |
| M1: shared schemas and handles | `MaterialShaderParameters.*`, `MaterialShader.*`; shared finalized layouts, stable lifetime/version identity, typed validation, cached built-in handles | M0 | Instances share metadata; wrong-layout/stale/wrong-type/size cases fail in Release; clone remains independent | 3-5 days |
| M2: typed settings and changes | `MaterialPassStateData.*`, `MaterialPass.cpp`, `Material.cpp`, `MaterialTechnique.cpp`; setting IDs, schema-backed adapters, change categories, graph revisions and tracked parameter edits | M1 | All mutation paths invalidate correctly; no-op edits do not publish; current normalization, all-pass writes and primary-pass reads preserved | 3-5 days |
| M3: resolved draw records | `ClawMaterial.*`, `ClawRendererDX11.*`, existing graphics publication/resource owners; dense records and dependency-aware retirement | M2 | Warm draws perform no material graph/name lookup or material-preparation allocation; live changes, unload/reuse and texture replacement remain correct | 4-7 days |
| M4: remove runtime duplication | Split hot/cold pass/native storage; adapt `workphone_graphics_material.*`; consolidate standard/shader value ownership and retire transition copies | M3 | Measured total material memory falls; old C descriptor callers and Lua/editor operations still round-trip; no untracked writable data | 4-7 days |
| M5: cook and backend binding | Resource compiler material adapter, schema/versioned payload, `wp_shader_program` binding map; optional constant-block split if measured | M1-M4 | Cooked source-free package, schema migration, program reload and device recreation pass; packing map verified | 3-6 days |
| M6: certification and cleanup | Remove obsolete runtime bridges after consumers migrate; full suite, Editor workflow, representative hardware profile/soak and documentation | M5 | Performance/memory gates and visual equivalence met with recorded limitations | 2-4 days |

Baseline scope: approximately 20-36 engineering days. M1-M3 provide the first useful integrated result; stopping after only replacing string lookups leaves graph traversal and duplicate data costs in place.

## Measurement and acceptance

Measure before optimizing in RelWithDebInfo using the same compiler, backend, device, assets, camera, resolution and pass set. Keep assertions/contract checks enabled in dedicated Release tests. Report medians and p95 over repeated runs; distinguish CPU submission, GPU time and present/wait time. Timings must include record preparation/publication as well as drawing so moving work between stages is visible.

Fixtures:

- CPU-only instance storage and handle access: 1,000 and 10,000 instances, shared layouts with 8/32/128 parameters, scalar/vector/matrix/resource mixtures; creation/clone, stable reads, 0%/1%/100% edits, name adapter versus retained handle. Consume checksums so the optimizer cannot discard the work.
- Scene submission: 10,000 material-consuming draws with one shared material, 100 shared materials and 10,000 distinct materials; capture colour and shadow passes and at least two views. Alternate state/texture variants to expose cache behavior; preserve equal draw count/coverage during the material-only comparison.
- Lifecycle/editor: repeated material selection and slider drags, save/reload, partial updates, clones, new/no-technique materials, pass removal, Lua animation, shader changes, texture replacement/unbinding, resource unload and device recreation.

Record: shared layout/metadata bytes; unique value/resource bytes; authoring/native compatibility bytes; allocator overhead; live/retired/capacity high-water bytes; name lookups/comparisons and property parsing; technique/pass snapshots and copied bytes; state/texture queries; records rebuilt and invalidation reason; publication/resolve/submit CPU time; constant/resource uploads and native state changes. Attribute only material work when counting draw allocations. Where available, use CPU sampling and hardware cache counters to verify the access-layout hypothesis rather than inferring cache misses from wall-clock time.

Provisional targets, to confirm or revise after M0 with an explicit reason:

| Metric | Target |
| --- | --- |
| Warm material draw path | Zero property-name lookup/hash/parse, graph snapshots, material resource-name resolution and material-preparation heap allocations after warm-up |
| Unchanged materials | Zero record rebuilds/publications; retain existing native upload/binding suppression |
| One effective edit | One coalesced rebuild of the affected material/pass record per publication boundary; no unrelated records rebuilt |
| Metadata duplication | One name/default/descriptor set per immutable layout, zero per-instance descriptor/name copies |
| Memory | At least 50% less material-owned memory in the 10,000-instance shared-layout fixture, including retained compatibility/authoring storage and amortized shared allocations; separately disclose peak and retired bytes |
| CPU | At least 50% less material resolution/preparation CPU in the material stress fixture, counting update/publication plus submit work; no statistically meaningful whole-frame p95 regression in representative scenes |
| Correctness | Same material values, resources, pass selection, ordering and visible output; no stale handle/resource use or unbounded retired growth |

These are acceptance goals, not claimed speedups. A fixture target does not establish an equivalent percentage improvement in the Editor or game. WARP/software evidence does not establish hardware cache/GPU performance.

## Validation and compatibility

Extend existing material tests in [GraphicsMaterialTests.cpp](../Tests/cpp/UnitTests/GraphicsMaterialTests.cpp), [GraphicsMaterialPassTests.cpp](../Tests/cpp/UnitTests/GraphicsMaterialPassTests.cpp), [MaterialLoadSequenceTests.cpp](../Tests/cpp/MaterialLoadSequenceTests.cpp) and [ClawMaterialPassLifetimeContracts.hpp](../Tests/cpp/ClawMaterialPassLifetimeContracts.hpp). Preserve existing pixel tests for material colour, channel selection, emission, UVs, sampler and culling behavior. Extend [WorkphoneGraphicsShaderTests.c](../Tests/c/WorkphoneGraphicsShaderTests.c) and add focused C descriptor/runtime storage contracts where appropriate.

Add semantic tests for same-size wrong types; cross-layout and stale handles; overflow/bounds; defaults; clone independence with shared layout; shader registration replacement; removed/renamed/type-changed parameters; coherent concurrent publication; coalesced edits; all dependent generation changes; safe in-flight retirement and bounded capacity. Test an edit after a retained mutable parameter reference as long as that API remains supported.

Asset compatibility must cover existing `ShaderParam.*` and `ShaderTex.*` properties, standard editor settings, symbolic/numeric enums, unknown authoring extensions, UUID texture identities and partial updates. Preserve unknown authoring data for round-trip where supported; reject unsupported cooked schemas clearly rather than silently dropping values. A cooked format specifies version, endianness, types/array counts, lengths, offsets and dependency identities; validate before allocation and invalidate build keys when schema/layout changes. Never dump compiler-native structs as a portable file format.

Use the current `wpgraphics-baseline` configuration and its Debug/RelWithDebInfo build presets for focused integration. Build the changed libraries and applicable `WPMaterialLoadSequenceTests`, `WPGraphicsClawProductionTests`, `WPGraphicsClawRendererTests`, `WorkphoneGraphicsShaderTests` and cooked resource targets before running their registered CTest cases. Add new CPU-only layout/record tests to CMake explicitly; proposed tests are not existing evidence. Run the full affected material suites and the Editor aggregate in the full build, plus Lua material/editor coverage, rebuilt SDK consumers and C89/C90 compile contracts when their interfaces change. Restart the Editor after DLL changes.

Pair structural/counter tests with interactive Editor and rendered scene checks: PBR, unlit/colour-only, packed maps, cutout, double-sided, transparency, shadows, reflection probes, fog, UI and terrain adapters. Include multiple cameras/render targets and a hardware run. Report absent fixtures/devices and skipped tests separately. A clean compile or focused unit result does not certify visible behavior or performance.

## Recommended first implementation

Start with M0-M1: establish material-specific costs, share immutable layouts in the existing shader parameter layer, add validated typed handles and cache built-in bindings. Follow immediately with M2-M3 to remove repeated graph traversal from active DX11 rendering. Delay allocator specialization and constant-buffer reorganization until the counters identify the remaining bottleneck.

The original planning review changed documentation only. Subsequent implementation and validation are tracked in the linked implementation status.
