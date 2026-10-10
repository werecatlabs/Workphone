# Workphone animation production implementation plan

Reviewed: 10 October 2026 against `cb1463b7a` in `G:\Workphone`.
Status: implementation plan based on current source inspection. No builds, tests, Editor sessions, GPU captures or benchmarks were run for this review. Existing test sources and historical graphics results are foundations, not current animation certification.

This expands `ANIM-01` through `ANIM-08` and gate GA in the [WPGraphics production plan](WPGRAPHICS_PRODUCTION_PLAN.md). It covers animation runtime, Editor authoring and preview, WPGraphics/Claw deformation, asset import/cooking/catalog integration, physics handoff, performance and shipping. Shared renderer/resource work remains coordinated with the graphics plan; animation does not create a second resource pipeline.

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
