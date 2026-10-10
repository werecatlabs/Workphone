# Workphone animation production implementation plan

Reviewed: 10 October 2026 against `cb1463b7a` in `G:\Workphone`.
Status: implementation plan based on current source inspection. No animation builds/tests, Editor sessions, GPU captures or benchmarks were run for this review. Existing test sources and historical graphics results are foundations, not current animation certification. The separate resource review records its limited catalog/resource regression run.

This expands `ANIM-01` through `ANIM-08` and gate GA in the [WPGraphics production plan](WPGRAPHICS_PRODUCTION_PLAN.md). It covers animation runtime, Editor authoring and preview, WPGraphics/Claw deformation, asset import/cooking/catalog integration, physics handoff, performance and shipping. Shared renderer/resource work remains coordinated with the graphics plan; animation does not create a second resource pipeline.

The [resource and asset production plan](WPRESOURCE_ASSET_PRODUCTION_PLAN.md) owns shared catalog/identity, build generations, runtime loading, Editor file operations and source-free packaging. Implement those services once and use them for animation assets.

## 1. Outcome and release scope

A user must be able to import a character, inspect and repair its rig/import settings, author clips and a controller, preview the actual runtime result, save/reopen it, instantiate several characters, and ship the same cooked assets in an application. Animation must remain correct across cameras, scene changes, Play/Stop, resource replacement, culling, LOD and failure.

Use staged releases without reducing the requested comprehensive scope:

| Release | Required outcome |
|---|---|
| **R1: production character foundation** | Windows x64, Claw/DX11; cooked skeleton/skin/clip/graph assets; correct sampling and pose blending; independent instances; clip playback, state transitions and 1D blends; events/sync markers; root motion; two-bone IK; sockets; CPU reference and GPU skinning; animated bounds, depth/shadows and deformation history; usable import/clip/graph/IK tools with undo and persistence; resource lifecycle, profiling and packaged validation |
| **R2: comprehensive animation** | R1 plus 2D blend spaces, additive/override layers and masks, graph composition and richer transitions, retargeting, mirroring, foot/hand/aim constraints, bounded motion warping, morph/facial channels, ragdoll handoff, animation-aware sequencing, clip editing/baking/compression, streaming and scalable animation LOD. Every feature has an asset, authoring workflow, runtime implementation and acceptance evidence |
| **Later optional extensions** | Motion matching, learned animation, advanced crowd animation/animation textures, unrestricted control-rig scripting, cloth/hair/muscle simulation, full-body optimization IK, dual-quaternion deformation, and additional platforms/backends. These do not block R1/R2 and receive separate proposals/gates |

Software rendering is a bounded CPU reference/fallback, with an explicit supported feature set. DX12, Ogre/OgreNext and other platforms do not inherit DX11 certification. Choose and document the first accepted source formats in M0; propose glTF 2.0/GLB and FBX through the existing Assimp integration, tested separately. An importer accepting a file extension is not proof of supported animation semantics.

## 2. Current source baseline

These are scoped observations from inspected code, not execution results.

| Area | Current evidence | Work required |
|---|---|---|
| Clip and graph runtime | [AnimationGraph.hpp](../Engine/cpp/Include/Workphone/Animation/AnimationGraph.hpp), [AnimationGraphNodes.hpp](../Engine/cpp/Include/Workphone/Animation/AnimationGraphNodes.hpp) and their implementations provide states, transitions, bool/float parameters, clip samples, Clip/Blend1D/Selector nodes, sync tracks and events | Retain this model; compile asset references into instance-safe pose evaluation. Validate actual deformed output and persistence, then extend node types |
| Scene update | [Animator.cpp](../Engine/cpp/Source/Workphone/Scene/Components/Animator.cpp), `update`/`onUpdate`, advances time, applies graph/clips to an `ISkeleton`, and solves IK | Complete the pose-to-render bridge, scheduling and transform ownership; prove one logical update per tick and independent instance state |
| Scene metadata | [Animation.cpp](../Engine/cpp/Source/Workphone/Scene/Components/Animation.cpp) stores clip name/length/loop/speed; Animator properties expose clips and IK | Replace name/index-only durable references with typed asset identity. Graph application to an Animator does not establish graph asset save/reopen |
| Importer | [AssimpLoader.cpp](../Engine/cpp/Source/WPAssimp/AssimpLoader.cpp), `importAnimations`/`importSceneAnimations`/`loadAnimations`, builds tracks, bone assignments and scene/skeleton animations, including XML cache output | Audit the two animation routes together. At merged channel key times, skeletal import selects preceding channel keys rather than interpolating each channel; missing channels use generic defaults. Preserve bind defaults, source semantics, mesh offsets and stable remapping |
| Engine/graphics skeleton boundary | [GraphicsSkeleton.cpp](../Engine/cpp/Source/Workphone/Graphics/GraphicsSkeleton.cpp) returns null from its inspected `createBone` overloads; [GraphicsMesh.cpp](../Engine/cpp/Source/Workphone/Graphics/GraphicsMesh.cpp) stores a controller/skeleton; Claw delegates to that base | Define the bridge between `ISkeleton`, `IGraphicsSkeleton`, legacy controllers and renderer palette data; do not assume these interfaces already provide a working Claw controller |
| CPU deformation | [Native skinning header](../Engine/c/Include/WorkphoneGraphics/workphone_graphics_skinning.h) and [implementation](../Engine/c/Source/WorkphoneGraphics/workphone_graphics_skinning.c) provide four influences, up to 256 joints, row-major matrices acting on column vectors, rigid/uniform-scale transforms and transactional validation | Keep as correctness reference. The native limit is not automatically a supported imported rig or GPU palette limit; define cook/backend limits |
| Claw integration | [ClawMesh.cpp](../Engine/cpp/Source/WPGraphics/ClawMesh.cpp), `setSkinningData`/`applySkinningPalette`, explicitly deforms positions/normals, replaces vertex storage, forgets the DX11 cache and updates bounds | Supply importer/controller palettes automatically. Avoid repeated mesh/buffer recreation. `clone` copies properties/skeleton but omits explicit bind vertices/output; fix immutable asset sharing and mutable instance ownership |
| Editor tools | [AnimationEditor.lua](../bin/Media/Scripts/Lua/Editor/AnimationEditor.lua), [AnimationWindow.cpp](../Tools/cpp/Editor/src/ui/AnimationWindow.cpp), [AnimationGraphWindow.cpp](../Tools/cpp/Editor/src/ui/AnimationGraphWindow.cpp) provide metadata/playback, graph editing/Apply and basic IK constraint controls | Lua `importClip` currently assigns a path to metadata. Connect actual import/cook/save jobs; complete graphical authoring, isolated runtime preview, undo, diagnostics and persistence. Basic IK authoring already exists; extend it |
| Resource foundation | [ResourceSystem.md](../Engine/cpp/Project/Workphone/ResourceSystem.md), [CatalogResourceAdapter.hpp](../Engine/cpp/Include/Workphone/Database/CatalogResourceAdapter.hpp) and [AssetDatabaseManager.cpp](../Engine/cpp/Source/Workphone/Database/AssetDatabaseManager.cpp) provide path-based ResourceIDs, compiler/dependency/container support and catalog snapshots | Add typed animation compilers/loaders and a coherent multi-resource generation. The adapter checks currency before/after work; final renderer publication still needs an atomic generation/ownership guard |
| Existing coverage | [AnimationGraphTests.cpp](../Tests/cpp/UnitTests/AnimationGraphTests.cpp), [AnimationIKTests.cpp](../Tests/cpp/UnitTests/AnimationIKTests.cpp), [SkinningTests.c](../Tests/GraphicsProduction/SkinningTests.c), [ClawProductionTests.cpp](../Tests/cpp/ClawProductionTests.cpp), catalog and resource tests | Preserve and extend them. Timing/solver/direct-palette tests do not establish imported, controller-driven, Editor-authored animation |

The older [Editor upgrade plan](../Tools/cpp/Editor/docs/EDITOR_UPGRADE_PLAN.md) describes an earlier baseline, including absent IK authoring. Current `AnimationWindow` contains constraint editing and solve diagnostics. Use current code as the implementation baseline and preserve the [Editor conventions](../Tools/cpp/Editor/docs/CONVENTIONS.md), including Lua-first authoring and thin C++ bindings.

## 3. Architecture and non-negotiable contracts

### 3.1 One owner for each responsibility

| Responsibility | Owner and contract |
|---|---|
| Identity, discovery and source provenance | AssetDatabaseManager/catalog; durable UUIDs, source relationships, dependency/reference display |
| Cooking, dependency hashes and cooked IO | Existing ResourceSystem and compiler registry; WPSQLite compilation metadata; no animation-specific duplicate database/cache |
| Immutable animation data | Typed skeleton, mesh binding, clip, graph, mask, rig, retarget and morph resources, shared across instances and pinned by generation |
| Playback and evaluation | Workphone animation runtime/`scene::Animator`; per-instance time, parameters, graph state, scratch poses, events, root-motion state and IK |
| Actor movement and physics | A designated scene/movement/physics owner consumes root-motion requests and returns accepted movement; animation never also moves the same actor through imported node tracks |
| Renderer | WPGraphics/Claw consumes a published pose/bounds/history snapshot; owns CPU fallback deformation, GPU palettes/buffers and draw submission. Drawing a view never advances animation time |
| Editor | Existing Lua authoring and C++ window/binding/viewport infrastructure; an isolated preview instance invokes the same resource/runtime/render services |

Extend the existing graph and Animator rather than introducing competing controllers. Provide compatibility adapters for `IAnimationController` and legacy `IAnimation::apply` callers. Shared cooked clips cannot retain mutable actor/bone target pointers; bind their track IDs to each instance explicitly. Document and migrate legacy XML, imported scene tracks and name-based references without removing unrelated functionality.

### 3.2 Pose and transform rules

1. Assign stable semantic joint IDs, parent indices and a verified topological evaluation order. Keep source names/paths for diagnostics; duplicate names in different branches cannot silently alias. Use a skeleton compatibility signature covering joint identity, hierarchy, bind transforms and coordinate conventions.
2. Distinguish skeleton local pose, skeleton model pose, mesh bind space and actor/world transform. Cook mesh-specific joint mappings/offsets, including mesh-node transforms. In the common normalized space, `palette[j] = currentModelJoint[j] * inverseBind[j]`; where source spaces differ, fold the mesh/skeleton conversion into the binding explicitly. Actor world transform is applied once by scene submission.
3. Use the native row-major/column-vector convention at the C boundary and explicit conversion/transposition for C++ math and HLSL. Verify handedness, axes, units, multiplication order, root transforms and mesh-node transforms with analytic fixtures; never infer convention from storage layout alone.
4. Sample absent channels from the declared bind/reference pose. Define absolute versus delta tracks at import. Reset scratch poses to that reference each evaluation, combine contributions in pose buffers, and commit once; sequential target mutation must not accumulate drift or make blending depend on traversal order.
5. R1 supports rigid/uniform-scale deformation. Validate accumulated model/palette transforms, not just each local key: rotated children under scaled parents can produce unsupported transforms. Diagnose nonuniform/negative scale and shear; permit only an explicitly validated offline bake or documented rejection. R2 may expand support only with correct normal/tangent math and matching CPU/GPU tests.
6. Normalize finite valid weights; use deterministic top-four selection/tie-breaking and report discarded weight/error. Invalid indices, negative/NaN weights, bad hierarchy, singular binds or oversize rigs fail before publication. Match the reference zero-weight bind-geometry policy. Never silently truncate joints; cook palette partitions or reject with actionable limits.
7. R1 linear blend skinning is authoritative. Skin positions and normals, add correct tangent/handedness handling for normal-mapped materials, and test normalization under supported scale. Define morph-before-skin order in R2 and preserve current/previous results for every enabled deformation stage.
8. Evaluate sockets after the final constrained pose. A socket contains a stable joint ID plus authored local offset; attachment parenting and world transforms must not apply the actor transform twice.

### 3.3 Tick, history and lifecycle rules

Use a documented simulation timeline and frame/tick identity. Snapshot inputs, advance graph/clocks, sample/blend poses, apply procedural constraints, publish model pose/palette/bounds, and dispatch queued events on the designated scene thread. Root-motion extraction produces a movement request before final pose publication; the movement/physics owner resolves it at the agreed fixed-step boundary. Post-physics IK/ragdoll stages consume one coherent physics snapshot. M0 fixes the exact scheduling and latency with GameManager/physics integration.

For fixed-step gameplay, render interpolation samples coherent previous/current simulation poses and actor transforms at the same presentation time. Do not advance time for a second viewport, shadow pass, UI refresh or GPU retry. Offscreen update policy must explicitly preserve gameplay events and root movement. Cinematic/property tracks use the same arbitration contract for properties they drive.

Jobs own immutable inputs and per-instance output buffers. Publish only when instance, scene, catalog, asset-set and device generations remain current. Cancel/invalidate work during unload, selection changes, project switch, Play/Stop and resource replacement; release GPU resources on their owner thread after in-flight draws retire. Callbacks never outlive their owning scene or Editor session.

Previous pose, morph weights and actor transform are per instance and refer to the last presented history, not just the previous evaluation call. First frame, teleport, seek, clip discontinuity, camera cut, incompatible reload and LOD remap reset history through a tested policy. A paused character can still move with its actor; velocity must include that movement.

## 4. Comprehensive feature and authoring matrix

Each row is complete only when its runtime, resource format, Editor workflow and acceptance tests exist. R2 rows remain planned until their own gates pass.

| Feature | R1 commitment | R2 commitment | Required Editor experience |
|---|---|---|---|
| Skeleton and skin | Bind pose, hierarchy/remaps, four influences, joint/vertex validation, mesh-specific inverse binds | Skeletal LOD remaps, richer rig metadata and validated expanded scale policy if needed | Hierarchy, bone axes, bind/current pose overlay, weights/influence heatmap, import warnings |
| Clip sampling | Translation/rotation/scale, missing/constant channels, loop/clamp, seek/frame step, speed and reverse policy | Ping-pong/time remap, clip sections, richer curve interpolation where supported | Scrub/step/loop range, source fps vs runtime time, channel/curve inspection |
| State machine | Entry/default state, typed parameters, conditions/priorities, exit time, bounded cross-fades and interruption policy | Nested machines/subgraphs, reusable graph assets, any-state rules, transitions with robust pose continuity | Node canvas or equivalent connected view, transition inspector, validation and live active-state/weight display |
| Blending | Correct pose cross-fades, 1D blend and selectors | 2D blend spaces, additive local/mesh-space layers, override layers, hierarchical masks, cached poses and inertialized transitions | Blend-space handles/preview, mask painting, reference-pose selection and layer solo/mute |
| Synchronization | Named phase markers, duration/phase mapping, footstep sync across 1D transitions | Sync groups across layers/subgraphs and explicit leader/follower policy | Marker timeline, phase display and transition preview |
| Events and curves | Stable event IDs, point/duration events, typed payloads, exactly defined boundary/seek/transition behavior, gameplay-facing scalar curves | Blended curves and morph/material/audio parameter routes with explicit ownership | Event/curve tracks, payload editor, event monitor and preview side-effect toggle |
| Root motion | Extract translation/yaw, in-place/extract/apply modes, loop-aware deltas and movement handoff | Bounded target/orientation warping, distance matching and locomotion stride adjustment | Root trajectory, displacement graph, world/in-place preview and accepted-versus-requested motion |
| IK and procedural pose | Existing two-bone solver, pole target, weights and target spaces | Grounded foot IK/planting, pelvis adjustment, hand placement, aim/look-at, chain constraints and solver ordering | Bone picking, target/pole gizmos, contact/limit visualization, solve/error diagnostics and bake |
| Attachments | Stable sockets, offsets, attach/detach and missing-joint fallback | Socket/constraint tracks and authorable equipment pose offsets | Socket gizmos and prop preview; saved offsets and undo |
| Retarget and mirror | Compatibility validation; incompatible rigs fail clearly | Joint/chain maps, reference-pose alignment, root/scale compensation, offline/runtime retarget and mirror tables | Source/target side-by-side preview, mapping validation, contact/trajectory error view and bake |
| Morph and facial | Resource/deformation contract reserved, unsupported features reported | Sparse morph targets, clip/curve-driven weights, corrective shapes, facial pose library and authored viseme/blink channels | Target sliders, curve/timeline editing, combination preview and morph/skin ordering diagnostics |
| Physics interaction | Exclusive transform ownership and stable IK target snapshots | Ragdoll mapping/limits, animation-to-physics and recovery blend, partial-body hit reactions and bounded physical drives | Body/joint overlay, mapping/limits, drop/recover preview and collision controls |
| Sequencing and property animation | Preserve existing actor/property animation; define priority against Animator and Play/Stop restoration | Cutscene clip sections/blending, camera/actor/property tracks, audio/event synchronization and pose recording/baking | Extend CutsceneWindow/AnimationEditor; scrubbing does not trigger uncontrolled gameplay events |
| Import/edit/bake | Real import/reimport, clip split/trim descriptors, root/event/sync metadata | Non-destructive key/curve edits, resample/reduce, additive/mirror/retarget/IK bake and batch processing | Import report, before/after comparison, job progress/cancel and editable derived clips |
| Runtime control | Scene/prefab references, C++ and thin Lua control, per-instance parameters, pause/reset | Versioned instance-state snapshots for replay/save/load; network-facing state hooks if a product requires them | Inspector controls and live debugging separated from authored defaults |
| Performance | Shared assets, bounded jobs/scratch memory, reusable GPU buffers, basic update budgets and diagnostics | Key reduction/quantization, chunked clip streaming, bone/update-rate/mesh LOD, safe pose sharing and batching | Per-character and aggregate profiler, LOD forcing, compression error and residency display |

Motion warping here means a bounded authored R2 feature with contact/error limits. Advanced environment-aware warping/crowd systems remain optional, consistent with the graphics roadmap. Automatic speech recognition/lip-sync generation and network transport are separate integrations; authored facial channels and versioned state are in scope.

## 5. Asset/resource implementation

### 5.1 Proposed typed resources

Names/extensions below are proposed contracts to finalize in M0. They fit the existing one-to-eight-character lowercase resource-type rule and must be registered without collisions.

| Resource | Proposed type | Payload and dependencies |
|---|---|---|
| Character definition | `charres` | Skeleton, skinned mesh/LODs, materials, graph/rig defaults, sockets, quality settings and typed install dependencies |
| Skeleton | `skelres` | Stable joint IDs/names, parents/topological order, local/model binds, compatibility signature and coordinate metadata |
| Skinned mesh binding | Existing mesh type plus versioned skin section, or `skinres` after registry review | Immutable vertices/indices, weights, joint remaps, mesh-specific inverse binds/space transforms, submesh palettes, bounds metadata and skeleton dependency |
| Animation clip | `animres` | Time/sample policy, tracks, compression blocks, root curve, events, sync markers, named float curves and skeleton signature |
| Animation graph | `agraph` | Versioned authored node IDs/parameters/transitions plus validated compiled evaluation plan; typed clip/mask/rig/subgraph dependencies |
| Bone mask / rig | `maskres` / `rigres` | Stable joint weights; chain/constraint settings, spaces/limits and skeleton dependency |
| Retarget profile | `retarget` | Source/target skeleton signatures, chain maps, reference offsets, mirror maps and root/scale/contact settings |
| Morph data | `morphres` | Mesh topology signature, sparse position/normal/tangent deltas, bounds and curve/channel identifiers |

Raw FBX/GLB sources remain source files. Start with explicit typed authoring descriptors, such as `data://characters/hero.skelres` and `data://characters/walk.animres`, whose compile dependencies include the raw source and import recipe. The catalog adapter must resolve the descriptor's actual type; it must not pretend a `.fbx` path is an `.animres` resource by changing its extension. If embedded resources use `data://characters/hero.charres:walk.animres`, add explicit derived-output identity/adapter support and test it before enabling that workflow.

Maintain raw-source UUID/provenance and durable derived descriptor UUIDs with stable semantic subasset IDs. Reimport must preserve clip IDs, graph references, events, sockets and user edits when topology is compatible. Joint removal, ambiguous renames and incompatible signatures produce an explicit remap/repair report. Clip display names and source array positions are not persistent identities.

### 5.2 Compiler and import work

- Implement `IResourceCompiler` registrations and typed decoders through existing ResourceSystem services. Separate editor-only Assimp/source parsing from runtime cooked loading; packaged playback must not require Assimp, source XML, the Editor or a writable catalog.
- Include source bytes, import units/axes, skeleton selection, mesh transform policy, clip range/time base, compression quality, target/backend, compiler version and all transitive dependencies in incremental hashes. Source bones/clip reorder must not change persistent identity accidentally.
- Import mesh offsets and node hierarchy together; support multiple skinned meshes/materials sharing a skeleton, helper nodes, animation-only files, source roots and separately imported clips. Diagnose missing targets, ambiguous nodes and unsupported source channels.
- Interpolate each source channel at union/resample times using its declared source mode. Preserve bind defaults for absent channels and shortest-path normalized quaternion interpolation. Do not convert linear source animation into a step curve by holding the previous key.
- Validate source counts/sizes/times and all decoded offsets before allocations/publication. Bound joints, tracks, keys, morph deltas, graph nodes/depth, strings, decompression blocks and dependency traversal; reject cycles, malformed/corrupt payloads and unsupported versions with asset-specific diagnostics.
- Cook error-bounded key reduction/quantization in R2 with position/angular/scale, end-effector and root-distance tolerances. Keep an uncompressed reference import. Compression cannot remove meaningful event/sync boundaries or break root-loop continuity.

### 5.3 Coherent replacement, streaming and packaging

Per-file atomic ResourceSystem output is necessary but does not make a character's skeleton, skin, clips and graph an atomic set. Stage all outputs for a dependency-compatible generation; validate/load the complete set, then publish one asset-set handle at the owning boundary. Retain the last good set on compile, IO, decode, validation or GPU allocation failure. Track pending/ready/failed states and show diagnostics without freezing the Editor.

Guard the final catalog/scene/device generation check and swap as one publication operation. Test stale completion after rename/delete/project switch and publication during active draws. Pin old CPU resources/GPU buffers until jobs and GPU work retire. Preserve playback state only when graph/joint IDs and signatures permit; otherwise reset explicitly with history cleared and a visible reload explanation.

R1 loads bounded resident clips. R2 adds independently decodable time blocks, lookahead and seek prefetch, cancellation, priority and memory budgets. Define a deterministic underrun policy: retain a valid pose or use a declared fallback while preserving logical event/root time. Decompression and IO never block the render thread. Keep skeleton/reference-pose metadata resident while needed.

Packaging walks typed install dependencies, includes all referenced graph variants/masks/rigs/morphs and required shader permutations, and rejects missing assets before shipping. Validate a relocated, read-only cooked package with source directories, Editor and importer unavailable. Rename/delete/reference repair and rebuild-from-source are first-class tools, not manual cache deletion instructions.

## 6. Implementation milestones and work packages

Milestone numbers in this document are local to animation. Resource/catalog work below is owned jointly with the dedicated resource production plan. Owners are roles to assign, not assumed staffing. Deliver small reviewable increments within each package; include source changes, tests, fixtures, captures, costs and remaining limitations in completion records.

| Milestone | Owner roles | Depends on | Exit gate |
|---|---|---|---|
| M0: contracts and baseline | Animation, graphics, assets, tools, QA/technical art | Current source | A0: supported formats/limits, ownership, fixtures and executable baseline agreed |
| M1: imported cooked data | Assets and animation | M0; shared catalog/compiler contracts | A1: deterministic source-to-cooked skeleton/skin/clip round trip |
| M2: evaluated characters in scene | Animation and scene/graphics | M1 | A2: two independent imported characters deform through normal scene updates |
| M3: gameplay animation foundation | Animation, gameplay/physics | M2 | A3: graph transitions, sync/events, root motion, basic IK and sockets work together |
| M4: production DX11 deformation | Graphics and animation | M2; shared render-pass/history contracts | A4: CPU/GPU parity, animated passes/bounds/history and stable buffers |
| M5: production Editor workflow | Tools, assets, technical art | Start after M1; complete with M3/M4 | A5: import → author → preview → undo → save/reopen → Play/Stop |
| M6: R1 integration and certification | All owners and QA/build | M1–M5; shared resource publication/package gates | A6: R1 correctness, lifecycle, measured budgets and packaged sample |
| M7: advanced graph and rig tools | Animation and tools | Stable M2/M3 asset/pose contracts; R1 gates stay green | A7: layers/masks/2D/retarget/warping/contact fixtures and workflows |
| M8: morphs, physics and sequencing | Animation, graphics, physics, tools/audio | M7 as needed; shared physics/sequencer contracts | A8: comprehensive deformation and ownership transitions |
| M9: scale and R2 certification | Assets, animation, graphics, QA/tools | M7/M8; streaming/package services | A9: comprehensive features pass quality, scale and shipping gates |

Critical path: M0 → M1 → M2 → M3/M4 → M5 → M6. Renderer and tools work can progress together once data/pose contracts are stable. M7–M9 may develop incrementally after those contracts, but cannot bypass R1 regressions. Do not wait for unrelated water/foliage features to establish imported animation; do wait for actual shared depth/shadow/velocity/resource services before certifying their integration.

### M0 — establish contracts and an honest baseline

- **AN-001: inventory and compatibility.** Trace both importer routes, scene Mesh/Animator, legacy graphics controllers, C++/Lua tools and packaged loading end-to-end. Record active plugin/configuration, source formats, coordinate/time policies, joint/influence/clip/node limits, scale restrictions and migration choices. Keep native C90 and integration C++17 conventions.
- **AN-002: fixtures and test registration.** Commit redistributable analytic fixtures and their expected transforms/vertices; establish separate headless, imported/component, GPU and Editor acceptance suites. Register mandatory tests explicitly so missing binaries/backend/media cannot look like a pass. Record toolchain/device/source/fixture provenance.
- **AN-003: scheduling and budgets.** Agree frame/tick/event/root-motion ownership with GameManager and physics; document the provisional budgets in section 8. Measure the existing CPU deformation cost, allocations and buffer recreations. Resolve any API additions through additive/versioned contracts; extend existing tools rather than duplicate them.

**A0:** reviewed contracts, licensed fixtures and a reproducible baseline report distinguishing passed, failed, skipped and unavailable tests. Old graphics pass counts cannot close this gate.

### M1 — complete import and typed cooked resources

- **AN-004: skeleton/skin compiler.** Preserve hierarchy, helper nodes, mesh offsets, inverse binds and coordinate conversion; deterministic joint remaps, top-four influence selection and actionable validation. Round-trip bind geometry and submesh palettes through typed runtime decode.
- **AN-005: clip compiler.** Correct independent channel interpolation/defaults, source time units and animation-only import. Support descriptor-based clip split/trim, root channel selection, events, curves and sync markers. Preserve stable derived IDs across compatible reimport.
- **AN-006: resource integration.** Register types/loaders, catalog mappings, hashes/dependencies and compatibility signatures; stage coherent character sets with last-good fallback. Migrate legacy XML/name references through explicit adapters and emit reports for ambiguity.

**A1:** a tiny imported skeleton/skin/clip cooks reproducibly, decodes without Assimp, reproduces bind pose and known animated samples, and rebuilds only affected dependents. Corrupt/mismatched input preserves the previous valid set.

### M2 — build instance-safe pose evaluation and scene publication

- **AN-007: sampling and pose math.** Introduce/reuse contiguous pose buffers and model-transform propagation under the existing runtime. Test empty/one-key/duplicate-time/zero-duration channels, shortest quaternion paths, finite validation and reference-pose defaults. Define duplicate-time handling as deterministic import repair with a diagnostic or rejection.
- **AN-008: instance ownership and blending.** Bind shared clip tracks per skeleton instance, evaluate into scratch poses and normalize contributions before one commit. Separate shared bind mesh/skeleton data from mutable CPU output, palettes, clocks and history. Fix clone/prefab instantiation; no instance deforms another's mesh.
- **AN-009: scene-to-Claw bridge.** Publish final model pose/palette/bounds automatically from Animator; adapt legacy controllers to that authority. Evaluate once per tick and reuse across views/passes; support pause, stop, reset, seek, disabled components and scene unload. Add a CPU-rendered reference path before optimizing.

**A2:** two instances of the same cooked character play different clips/times and retain independent poses after cloning, parent transform changes and reload. A second camera cannot change their clocks or double-submit events. A normal scene draw supplies the palette without test-only manual calls.

### M3 — finish playback, graphs and gameplay contracts

- **AN-010: state and parameter semantics.** Extend existing graph definitions with serialized typed defaults, validated references, deterministic priority/tie-breaking and bounded graph traversal. Specify self/any-state transitions, interruption and snapshot-pose cross-fade behavior. Implement R1 clip/Blend1D/Selector results as real poses.
- **AN-011: clocks, sync and events.** Support signed playback speed/reverse under an explicit policy; maintain unwrapped time/cycle counts so large deltas and multiple wraps do not collapse into one boolean loop. Use a documented half-open event interval with special start/end rules, ordered dispatch and explicit weighted-source/transition deduplication. Define seek suppression, duration enter/update/exit, rewind, queue limits and overflow reporting. Rendering and Editor inspection do not emit gameplay events.
- **AN-012: root motion and movement.** Extract delta translation/yaw from unwrapped root trajectories, blend in a declared space and remove the extracted component from the visual root. Offer in-place, extract-only and movement-owned modes. Test collision-rejected displacement, parent transforms, loops, transitions, pause, teleport and fixed-step/render interpolation; root displacement is applied exactly once.
- **AN-013: IK and sockets.** Integrate existing two-bone constraints after blending with explicit world/model/local target conversions, pole fallback and weighting. Publish final socket transforms; test unreachable/degenerate chains, animated targets and actor scale. Deliver a grounded/prop-equipped R1 reference character; more advanced planting is M7.

**A3:** idle/walk/run transitions, synchronized footsteps, root displacement, hand target and attached prop behave coherently in a scene and survive pause/restart/reload. Verify event sequences and root displacement analytically as well as visually.

### M4 — implement production deformation in WPGraphics/Claw

- **AN-014: GPU skinning.** Add skinned vertex declarations/attributes, shader permutations and per-instance palette binding to the native/Claw DX11 path. Keep bind geometry immutable, reuse palette/dynamic buffers and track dirty generations. Verify palette alignment/capacity, independent draws, material submeshes and correct normal/tangent results. CPU fallback has per-instance dynamic output and does not invalidate static caches every frame.
- **AN-015: pass parity and history.** Use the same deformation inputs in colour, depth, picking, shadow and velocity passes, including alpha/material variants. Retain previous palettes/actor transforms until consumed; implement resets for discontinuities. If the shared frame pipeline lacks a required pass, record that dependency as open rather than certify synthetic replacement output.
- **AN-016: bounds, visibility and LOD.** Cook clip envelopes and/or bone bounds, then expand/refit conservatively for blend, IK, root motion, attachments and morphs. Test main camera, Editor preview, shadow and reflection views independently. A character hidden from one view can remain a caster/visible in another. Add hysteresis and a tested history/remap policy before enabling reduced bone/update rates.
- **AN-017: lifecycle and capabilities.** Reject unsupported asset/backend combinations clearly; device reset/recreation restores skin/morph/shader buffers from valid CPU resources. Validate unload/reload during jobs/draws, out-of-memory failure and last-good rendering. Capability reporting follows actually compiled and available paths.

**A4:** GPU positions/normals agree with CPU reference within frozen tolerances; skinning affects every advertised pass, limbs/casters do not disappear at bounds, and steady-state animation reuses buffers. Separate mathematical parity, captured rendered output and actual hardware performance evidence.

### M5 — deliver a complete Editor workflow

- **AN-018: import/reimport UI.** Extend ProjectAssetsWindow, existing import jobs and AnimationEditor with real typed import/cook requests, source/subasset selection, settings, progress/cancel, diagnostics and reference repair. Clip import creates loadable data rather than only a metadata name. Preview source and cooked differences; preserve authored overrides on reimport.
- **AN-019: clip/timeline tools.** Scrub/step/loop clips; display skeleton/weights/bounds/root paths; edit split/trim/root/event/sync/curve data non-destructively. R2 adds key editing and resample/reduce/bake. Use command-based undo/redo, dirty state, atomic save, reload conflict handling and asset-version migration.
- **AN-020: graph/IK authoring.** Extend existing AnimationGraphWindow/AnimationWindow and Lua authoring through thin native bindings, with one shared serialized model. Add a connected graph view, validated pins/IDs, parameter defaults, transition/1D controls and live state/weight/event tracing. Extend present IK controls with bone picking, target/pole gizmos, solve status and saved rig resources; no second IK editor with competing state.
- **AN-021: isolated preview and scene integration.** Reuse runtime resource/evaluation/deformation in a dedicated preview world. Selection, drag/drop, graph Apply and inspector edits publish safely on the correct thread. Test asset-browser and scene/graphics-mesh selection routes, multiple preview windows, camera controls and native input. Preview events are suppressed by default; explicit audition routes bounded audio/effects locally.
- **AN-022: persistence and Play/Stop.** Save graph/clip/rig references and overrides in scenes/prefabs; reopening restores authored state without saved transient pointers. Playing does not overwrite defaults; stopping restores editor state, camera and transforms. Undo after Apply, selection changes, cancellation and Lua hot reload cannot retain stale listeners or mutate the wrong Animator.

**A5:** a technical artist imports a fixture, authors a 1D controller and IK/socket setup, previews through DX11, undoes/redoes edits, saves/reopens, duplicates actors, enters/exits Play and packages the same result. Capture the actual interactive workflow; compile-only checks do not close this gate.

### M6 — certify R1

- **AN-023: integration/lifecycle coverage.** Exercise asynchronous import/reload/scene switch, failure injection, legacy migration, parented actors, multi-camera/shadow use, job cancellation and repeated Play/Stop. Repair integration failures without reverting unrelated renderer/catalog fixes.
- **AN-024: profile and optimize.** Capture per-stage CPU/GPU costs, allocations, upload bytes, draw/pass counts and memory. Remove hot-path allocations and buffer recreation; benchmark jobs and CPU/GPU crossover. Preserve event/root correctness when update budgets or visibility change.
- **AN-025: package/docs/release.** Produce a standalone animation sample and reproducible cook manifest, artist workflow guide, supported-format/limit matrix and release evidence. Run full Windows solution integration plus focused CI suites; complete both Debug and optimized coverage.

**A6 / animation R1:** A0–A5 and section 9's R1 gates pass on the declared backend/configurations. Animation remains experimental if imported runtime output, Editor workflow, lifecycle or packaged assets are unverified.

### M7 — comprehensive graphs, rigs and locomotion

- **AN-026: graph expansion.** Implement 2D blend topology/interpolation (including degenerate/outside-hull behavior), additive/override layers, hierarchical masks, reusable subgraphs/cached poses, typed triggers and richer interruption/inertialization. Compile and validate cycles, parameter/pose type compatibility and evaluation ordering. Test layered aim/reload over locomotion with lower-body motion unaffected.
- **AN-027: retarget/mirror.** Deliver reusable maps, reference-pose alignment, root scale policy, missing-chain diagnostics, left/right mirror conventions and offline/runtime retarget. Preserve root distance and contact quality across proportionally different rigs; compare errors before/after bake.
- **AN-028: contact IK and warping.** Add foot contact/plant/release state, ground probing from a coherent physics snapshot, pelvis adjustment, slope/stair alignment, hand constraints and aim/look-at limits. Add bounded motion/stride/distance matching with explicit reach/contact thresholds and interruption fallback. Never use an IK target to hide an invalid skeleton/import transform.
- **AN-029: authoring/baking.** Add blend-space canvases, mask painting, retarget side-by-side views, contact/warp windows and rig debug overlays to existing tools. Bake mirror/retarget/additive/constraint results into versioned clips with undo and source provenance.

**A7:** representative layered locomotion, alternate-rig retargeting and slope/stair/target interaction pass numeric contact/trajectory tolerances and artist review, with save/reopen and packaged playback for each asset type.

### M8 — morph/facial, physics and sequencing

- **AN-030: morph/facial pipeline.** Import/cook sparse morphs and topology signatures; implement CPU/GPU morph-before-skin with correct bounds/normals/tangents/history. Support curve/pose-driven facial weights, corrective combinations and authored viseme/blink channels. Validate mixed morph/skin/LOD/reload and budget target counts.
- **AN-031: physics pose bridge.** Coordinate with the [physics production plan](WPPHYSICS_PRODUCTION_PLAN.md): skeleton/body mapping, exclusive ownership, partial/full ragdoll, blended handoff and recovery, interpolation and optional bounded physical drives. Define root and event behavior during ragdoll. Physics owns body simulation; animation owns final presentation pose.
- **AN-032: sequencing/property tracks.** Extend [CutscenePlayer.hpp](../Engine/cpp/Include/Workphone/Scene/Components/CutscenePlayer.hpp) and existing CutsceneWindow with clip sections, pose/curve blending and explicit priority over gameplay Animator. Restore prior state on stop/cancel; safe scrub/reverse/seek and audio-time synchronization; record/bake a runtime pose sequence without actor pointer persistence.
- **AN-033: R2 Editor parity.** Complete facial sliders/curves, ragdoll mapping/limits and sequencing previews using actual runtime resources. Ensure undo/save/reopen/Play/Stop and failure diagnostics for every advanced feature.

**A8:** one reference scene combines locomotion, facial morphs, prop/socket IK, ragdoll/recovery and cinematic takeover. No double transform ownership, missing deformation pass or stale history is accepted.

### M9 — streaming, scalability and comprehensive certification

- **AN-034: compression and streaming.** Implement error-bounded clip reduction/quantization, seekable blocks, prefetch/cancel/residency and explicit underrun behavior through shared resource services. Compare compressed/uncompressed poses, root curves and end effectors over entire clips and transitions.
- **AN-035: animation LOD and safe sharing.** Reuse existing LODSystem selection/hysteresis conventions. Add update-rate and bone LOD, bounds-aware visibility policy, staggered jobs and bounded interpolation; preserve event/root timelines at every quality level. Share evaluated poses only for identical effective inputs/time/skeleton/constraints; keep per-instance roots, events and history. GPU instancing/batching must index independent palettes and demonstrate measured benefits before promotion.
- **AN-036: state snapshots.** Version playback/graph/parameter/root/contact state for save/replay restoration; define deterministic behavior for fixed inputs on the certified platform. Do not promise cross-platform bitwise deterministic math or add an unrelated network stack.
- **AN-037: R2 certification.** Run all feature/asset/Editor/package gates, scale tests and soaks; produce captures, quality error reports, performance tables and a documented capability matrix. Unsupported optional extensions remain explicit.

**A9 / animation R2:** every R2 matrix row meets its runtime, resource, authoring, lifecycle and measured acceptance criteria. A graph node or UI control alone does not count as a feature.

## 7. Test fixtures and mandatory validation

| Fixture/suite | Required coverage |
|---|---|
| Analytic rigid/strip rigs | One joint, two-joint bend, three-joint IK; expected bind/current vertex and normal values; shuffled joints, unequal channel key times, missing channels and quaternion antipodes |
| Import convention pack | Source root/mesh-node transforms, helper nodes, units/handedness, duplicate names, multiple skins/materials, animation-only files, source resampling modes and unsupported scale; one committed fixture per promised source format |
| Humanoid/alternate rig | Licensed idle/walk/run/jump/turn/aim/reload clips, root and in-place variants, marker/event/curve tracks, prop sockets, different proportions and skeletal LODs |
| Comprehensive deformation | Mesh with sparse facial morphs and normal maps; layered/additive/masked poses, mirror/retarget, foot/hand IK, ragdoll recovery and sequencer takeover |
| Runtime integration | Two shared-asset instances, clones/prefabs, independent clocks/parameters; tick vs render rates, zero/large deltas, multi-loop/reverse/seek event ordering, interruption and scene/component removal |
| Renderer integration | CPU/GPU data comparison plus colour/depth/shadow/picking/velocity captures; non-identity actor/parent transforms, frustum-edge limbs, multiple views, first-frame/history reset and paused-but-moving actors |
| Lifecycle/failure | Corrupt source/cook, oversized payload, compiler/decode/upload failure, skeleton incompatibility, reload during jobs/draws, rename/delete/project switch, device recreation and bounded cancellation |
| Editor integration | Both scene and asset/graphics selection routes, real import/reimport, graph/rig editing, undo/redo, dirty/save/reopen, drag/drop, multiple previews, native camera input, Lua hot reload and repeated Play/Stop |
| Shipping/scale | Source-free relocated read-only package, dependency closure, compression/streaming stress, 1/10/100/500-character workloads and sustained memory/resource stability |

Preserve existing graph/IK/native skinning tests and the real Claw GPU harness. Add focused animation targets using existing CMake conventions rather than relying on the monolithic UnitTests executable alone. Proposed targets: `WPAnimationCoreTests`, `WPAnimationImportTests`, `WPAnimationSceneTests`, `WPAnimationAssetTests` and `WPAnimationClawTests`; these are planned names, not existing runnable targets. Register Editor acceptance separately and include the suites in CI/validation required lists.

Start integration builds with the **full `windows-x64` / `project_x64` solution**. Focused graphics/resource builds supplement that coverage and accelerate iteration. Match Editor plugins, backend and configuration to built binaries; build transitive dependencies before diagnosing cascading missing libraries. Re-run current graph/IK suites as part of the resulting executable baseline.

Use headless math/format tests for deterministic numeric checks and actual DX11 offscreen/presented runs for render/lifecycle checks. WARP may supply a correctness lane; physical GPU runs supply device/driver correctness and performance evidence. Missing backend, binary or fixture is an explicit unavailable/skip result and blocks any gate that requires it. Every release run records source revision, executable/fixture hashes, toolchain, configuration, device/driver, test selection and result details.

## 8. Quality, performance and soak budgets

Freeze budgets and reference hardware in M0, then update estimates from measured M2/M4 output. The following are proposed acceptance targets, not benchmark results or claims about current performance:

- CPU/GPU position error no more than `1e-4 * max(1, fixture extent)` in normalized asset units; normal/tangent angular error at most 0.5 degrees for supported transforms. Compression adds a separately authored error budget, measured at joints, end effectors and mesh vertices.
- Root motion and event order match the analytic uncompressed timeline under fixed/variable steps, loops, transitions and seek policy. Set absolute root-distance, orientation and planted-foot tolerances per fixture in meters/degrees; unit conversion must not change their physical meaning.
- Reference R1 workload: 100 independently animated characters, 64–128 joints, 10k–20k vertices each, four influences, 1080p, a recorded camera path and identical material/shadow/view settings for comparisons. Report 1/10/100/500-character scaling; 500 is a stress case, not an implicit guaranteed full-quality population.
- Initial 60 Hz targets on the selected machine: animation evaluation/constraints/publication ≤2.5 ms CPU at p95, incremental animation/deformation GPU cost ≤3 ms at p95, and no steady-state per-character heap allocation or mesh/index-buffer recreation. Record p50/p95/p99, worker utilization, uploads, resident memory and draw counts; revise with an explicit workload/quality decision if measurements miss, never quietly reduce content.
- Separate cold import/cook/startup from warm runtime. Preview parameter edits should become visible within two presented frames under the reference workload; sustained asset search/compile work must not block viewport/input. Large jobs expose progress and cancellation.
- Proposed release soak: two hours of mixed playback/LOD/streaming and at least 1,000 automated spawn/despawn or reload/Play/Stop cycles. After quiescence, live resource counts and pinned generations return to baseline; no monotonic memory/handle growth, stale callbacks or device errors. Include deterministic injected failures and user-driven Editor acceptance.

Animation LOD, compression, pose sharing and GPU batching are measured tradeoffs. Compare equal visible content/quality and all passes/views, including transition overhead, root/event work and bounds updates. Report throughput and memory alongside visual error; fewer evaluations/draws alone do not prove a useful optimization.

## 9. Release gates and first executable increments

| Gate | R1 requirement | R2 addition |
|---|---|---|
| Asset correctness | Typed stable identity, bind/clip round trip, migration, deterministic cook and coherent last-good replacement | All advanced resource types, compression and streaming quality/residency |
| Runtime correctness | Actual imported poses, independent instances, graphs/sync/events/root/IK/sockets and safe scheduling | Advanced graph/retarget/contact/warp/morph/physics/sequencing/state restore |
| Renderer correctness | Actual DX11 deformation in advertised passes, CPU/GPU parity, conservative bounds/history and recovery | Morph+skin, skeletal LOD, safe sharing/batching and streamed deformation |
| Editor completeness | Actual import/author/preview/undo/save/reopen/Play/Stop workflow with runtime parity | Full advanced-feature authoring/debug/bake workflows |
| Reliability/performance | Failure/cancellation/lifecycle coverage, measured budgets, soak and source-free packaged sample | Comprehensive scale/quality/streaming soaks and package closure |

Implement next in this order:

1. **Contracts and fixture PR:** AN-001–003; settle spaces, references, limits, clock/update ownership and a tiny committed source fixture. Add missing focused test registration and document the actual baseline.
2. **Cooked analytic character PR:** AN-004–006; source → catalog descriptor → typed skeleton/skin/clip → runtime decode → known bind/animated values. Use shared publication contracts and test failed replacement. No new animation database.
3. **Scene character PR:** AN-007–009; two independent normal scene instances deform through the CPU reference and multiple cameras without manual palette injection. Fix clone and target binding here.
4. **Gameplay and GPU PRs:** AN-010–017 in bounded increments, preserving analytic reference output. Prove root/events/IK and actual GPU pass parity independently, then together.
5. **Artist workflow PRs:** AN-018–022; replace metadata-only import behavior, complete durable graph/rig authoring and prove save/reopen/Play/Stop using actual runtime output.
6. **R1 certification, then comprehensive features:** AN-023–037 with each feature's resource, runtime, Editor and acceptance evidence delivered together.

Cross-reference with the graphics plan: AN-004–009 complete the core of `ANIM-01/02/03/07`; AN-010–013 cover `ANIM-04/07`; AN-014–017 cover `ANIM-05/06`; AN-026–037 expand `ANIM-08`. A6 satisfies GA only when shared catalog/package and real renderer gates also pass.

Assign one accountable owner per milestone plus shared asset, graphics, tools, physics and QA reviewers. Estimate package sizes after M0 and reforecast after M2/M4 using measured integration work. Calendar dates before staffing, reference content/hardware and shared renderer/resource dependencies are fixed would imply precision the current evidence does not support.
