# Actor, mesh renderer, scene node, and city streaming implementation plan

Reviewed: 22 October 2026. Source baseline: `eeb6dfa80`.

Status: implementation plan based on a source audit. No runtime changes or new performance measurements were made for this review. Preserve the existing local `ResourceDatabase.cpp` modification.

## Recommended design

Use **static-aware incremental transform updates plus budgeted spatial streaming**. Keep the existing actor/component API, Lua bindings, scene serialization, and renderer backend. Extend the current transform storage, native caches, and streaming scheduler rather than introducing a second actor system or another worker pool.

A city should have a compact world index, a persistent gameplay layer, independently loadable spatial cells, and optional distant representations. Only cells admitted for installation/residency create full actors, renderer objects, collision, and asset handles. Frustum culling alone does not reduce residency, and loading every actor before hiding distant cells is not streaming.

Static means “reuse derived state until an input changes.” It must not mean “ignore transform edits.” Static transforms and streaming solve different problems and should share invalidation, ownership, and publication rules.

## Findings in the current source

| Area | Existing behavior | Planned improvement |
| --- | --- | --- |
| Actor hierarchy | `GameActor::updateTransform()` traverses descendants and invokes every component's transform update. | Process deduplicated dirty branches; avoid clean static subtree traversal. |
| Dirty scheduling | `GameManager` drains multiple actor/component transform queues. The actor-transform loop pops before checking its 2,048-item processing limit. | Unify/coalesce transform scheduling and test that budget limits never drop a popped item. |
| Transform storage | `TransformSystem` already has contiguous channels, generational handles, and bulk views, but individual getters/setters acquire its mutex. | Extend this storage with transform revisions and batched processing; keep lifetime generations separate. |
| Static flag | `Renderer::updateStatic()` already removes static renderers from continuous render-transform registration. | Preserve this fast path and add reliable one-shot invalidation for static edits, parent motion, node creation, and resource replacement. |
| Renderer completion | A graphics-object loading event calls `root->updateTransform()`. | Refresh the affected renderer once rather than revisit a whole imported hierarchy per completed mesh. |
| Mesh renderer | Mesh/material lookup and binding occur through component scans; renderer nodes are currently created under the graphics root. | Cache generation-validated bindings and update only changed renderer state. Preserve flat world-pose submission unless hierarchy semantics are deliberately migrated. |
| Scene-node bridge | Authoritative transform state is retained before native creation. Equal authored state can still require native synchronization. | Track authored versus applied revisions; an equality-only early return would break deferred initialization. Audit local/world setter behavior under a non-identity parent. |
| Native synchronization | Attach/detach explicitly acquire the graphics lock; other direct native-node entry points need the same ownership audit. | Verify load/unload, parent changes, transforms, and state callbacks obey the shared capture/mutation boundary before reducing locking. |
| Native transforms/culling | Native nodes already cache local/world matrices by revisions; transform jobs already snapshot hierarchy inputs and reuse shared prefixes and view publications. | Extend existing caches and invalidation. Do not reimplement matrix caching or create a competing transform job path. |
| Streaming | `WorldStreamingScheduler` already provides workers, immutable results, cancellation, queue bounds, and CPU/GPU/collision budget leases; terrain/foliage participate. | Add actor-cell producers and resource residency integration to that service. The scheduler's reservations currently bound participating producers, not total process memory. |

The previous New York validation observed about 6.7 FPS / 149 ms render time. That is historical evidence from 20 October, not a fresh baseline or proof that transforms account for the entire render cost. Cell streaming, transform propagation, material preparation, draw submission, and GPU work need separate measurements.

## Contracts to settle first

### Static actors and transforms

- Preserve the existing serialized static flag and explicit cascade behavior. A parent being static does not silently mark all descendants static.
- A clean static actor publishes its pose once and stays out of continuous transform submission. A changed static actor is queued for a one-shot update.
- Local edits, world edits, ancestor motion, reparenting, resource/node replacement, and world-origin changes invalidate dependent world transforms and bounds.
- A static child under a moving parent still moves. A dynamic child under a static parent remains independently updateable. Skipping a clean parent cannot suppress an independently dirty child.
- Explicit animation, physics, or script transform writers take precedence over the static optimization. Diagnose conflicting authoring and route these actors through a dynamic path; do not freeze gameplay or repeatedly toggle authored flags.
- Static does not disable scripts, physics, visibility changes, material edits, camera-dependent culling, LOD selection, or shadow updates.
- Retain current smooth-motion interpolation and zero/one/multiple-worker behavior. Render-sampled poses must not overwrite authoritative simulation transforms or vice versa.

### Ownership and publication

- Scene graph edits have one commit owner; render-side creation, transform application, attachment, and removal use the established `GraphicsSystem` synchronization boundary.
- Workers read immutable inputs and return immutable results. They do not mutate actors, native nodes, GPU objects, or catalog entries.
- Retain the cull snapshot allocation bound (`job->entry_count`), generation checks, and retained ownership until jobs and GPU users finish.
- Validate scene/session, cell, resource, and handle generations at commit. Unload/reload or project switching must invalidate late work without touching a replacement object at a reused address.
- Use bounded publication batches. Do not pause rendering for the entire city while reading or decoding cells.

## Phase 0 — Establish measurements and regression fixtures

Add opt-in counters/timings to existing profiler/statistics paths:

- Dirty requests versus unique actors, component scans, nodes visited, local/world recomputations, interpolation samples, and native transform writes.
- Transform/storage/graphics lock wait and hold time, scene commit duration, mesh/material reloads, and allocations.
- Frame preparation, culling, material preparation, CPU submission, GPU time, and present time separately. Record adapter identity and distinguish hardware from WARP.
- Resident actors/components/nodes, unique resources, CPU decoded mesh bytes, GPU bytes, queued reads, staging bytes, and bytes pending retirement.

Fixtures: stationary New York; a wide static hierarchy; a deep hierarchy; mixed static/dynamic children; moving vehicles; repeated reimport; repeated cell traversal. Keep camera routes, viewport, render settings, hardware, build configuration, and warm/cold cache conditions fixed for comparisons. Record p50/p95/p99 rather than one frame.

Acceptance: a repeatable baseline and counters that explain repeated work. Capture complete first-usable-view time separately from import, cell decode, actor installation, and steady-state frame time. No FPS promise until CPU/GPU attribution exists.

## Phase 1 — Make static transforms a correct incremental path

Extend `TransformSystem` with local/world revisions, parent dependency information, and deduplicated pending work keyed by its existing generational handles. Track a renderer's last submitted world revision separately from the transform's lifetime generation.

1. Make transform setters change revisions only when the authored pose changes; preserve an explicit force-invalidate operation for node recreation and lifecycle changes.
2. Coalesce dirty requests across the current actor/component queues. Resolve parents before dependents and avoid processing a child again when an already-dirty ancestor covers the same work.
3. Track independently dirty descendants or subtree work so clean static parents can be skipped safely.
4. Process transforms through bounded batches/bulk storage views, releasing storage locks before callbacks or scene mutation.
5. Keep the public recursive `updateTransform()` compatibility behavior where callers require immediate propagation; migrate normal scheduled updates and loading to the incremental path.
6. Make static/dynamic transitions update scheduling once and force one final correct publication. Fix the queue pop/budget ordering as part of this scheduling work.

Acceptance: after warmup, an unchanged static hierarchy performs zero world recomputations and zero renderer transform submissions. Repeated edits before one commit coalesce. A moved leaf updates only its required dependency path; a moved ancestor updates affected descendants once. Work remaining after a batch limit stays queued.

## Phase 2 — Coalesce renderer and native-node synchronization

Introduce renderer dirty categories for transform, mesh, material slots, visibility, bounds, and static classification. Keep mesh/resource loading distinct from render-object initialization and explicit reimport/reload.

- Replace completion-triggered root transform walks with targeted renderer initialization and one pending pose publication.
- Resolve the Mesh component and material-slot table once; invalidate these references on component replacement/removal, actor destruction, resource generation changes, and reimport.
- Apply mesh/material changes only when identity, generation, slot, or options change. A same-path reimport still reloads geometry.
- Maintain authored, queued, and native-applied transform revisions. Publish pre-load edits when the native node appears even if the pose compares equal to the last authored state.
- Batch native edits under the existing graphics lock. Publish local/world state, bounds, and hierarchy membership coherently; preserve rollback and detach-before-retirement ordering.
- Preserve the current actor-world-pose-to-flat-render-node convention initially. Mirroring actor parenting into native parenting without changing pose submission would apply parent transforms twice.

Acceptance: unchanged static mesh renderers do no repeated binding or transform work. Delayed native creation, same-path reimport, visibility changes, static toggles, reparenting, and node recreation remain correct. Local versus world setters pass under rotated/scaled parents.

## Phase 3 — Cook independently loadable city cells

Introduce additive, versioned world-manifest and cell formats, integrating the existing resource cooking/catalog/package pipeline.

The manifest contains compact cell IDs, bounds, content generations, byte/count estimates, dependency/resource IDs, and distant-representation references. Cell payloads contain the actor/component data required to instantiate that cell. Runtime startup must read the index and initial demand only, not the complete city actor graph or every asset dependency closure.

- Start with a configurable horizontal grid plus explicit building/interior groups. Tune cell size using actor count, decoded bytes, installation cost, and the camera route; do not hard-code one distance or cell size as universally suitable.
- Place static content in cells; keep player, vehicles, session objects, and other persistent gameplay actors outside disposable cells. Moving actors migrate ownership without losing their runtime state.
- Assign stable actor IDs and resource identities across cook, reload, and save. Deduplicate shared meshes/materials/textures across cells.
- Avoid duplicating a large building into every intersected cell. Use one owner plus spatial coverage/dependency records, or split geometry deliberately during cooking.
- Validate cross-cell hierarchy/reference rules. Keep an inseparable hierarchy together or pin/promote its required parent; soft references must not recursively load the entire city. Detect dependency cycles and oversized hard dependency groups.
- Treat a source GLB as an authoring input. Offline cooking can inspect the complete GLB; runtime streaming consumes cell payloads and individually addressable cooked assets. A monolithic runtime GLB cannot provide this memory guarantee by distance filtering alone.
- Preserve legacy `.fbscene` and prefab loading. Add explicit conversion/opt-in world settings; do not silently change existing project assets or mark animated objects static.

Acceptance: a packaged build can discover and load one city region without reading or instantiating unrelated cell payloads. Resource references survive relocation, cook regeneration, and source-free execution.

## Phase 4 — Stream actor and resource residency under shared budgets

Add a scene-cell coordinator as a producer/consumer of `WorldStreamingScheduler`; reuse its cancellation, immutable result, priority, queue, and budget mechanisms. Extend resource residency where the asset platform lacks asynchronous single-flight loads and explicit pins; coordinate this with the resource production plan rather than creating another resource manager.

Cell lifecycle:

`Absent → Requested → Reading/Decoding → ReadyToInstall → Installing → Resident → Retiring → Absent`

Include explicit failed, cancelled, and stale outcomes. A demand change can cancel pending work; a partially committed installation must roll back or finish a documented atomic sub-batch.

- Demand comes from player/gameplay interest, camera, predicted motion, and relevant collision/navigation/audio needs. An editor spectator camera must not accidentally evict the player's collision world.
- Use load/retain radii, hysteresis, minimum residency, and bounded eviction to prevent boundary thrashing. Handle teleports by cancelling obsolete demand and prioritizing the destination.
- Reserve source, working, decoded/result, GPU, and collision memory **before** starting work. Count shared resources once, include upload staging and retired-but-in-flight allocations, and publish actual resident usage alongside reservations.
- Bound simultaneous reads, decodes, ready cells, install/uninstall time per frame, and live actor/component capacity. Respect `WP_MAX_ACTORS` and pool limits; streaming is not permission to raise every fixed capacity to city-wide size.
- Extend owner-thread graph installation to accept bounded actor groups while keeping incomplete cells hidden/non-interactive until dependencies are ready. Do not expose partial road collision or half-built building hierarchies.
- Coalesce resource requests by complete identity/variant/generation. Cancelling one cell releases its interest without cancelling a shared load still needed elsewhere.
- On eviction, detach/unregister actors and native objects first; retire queued/jobs/GPU references before releasing shared resource leases and reporting those bytes free.
- Persist runtime changes for unloadable gameplay content through a compact state overlay. Reinstantiation must not reset mission state, pickups, or object identity unexpectedly.
- If the working set cannot fit, reduce detail/prefetch distance, keep a safe coarse representation, or report admission failure. Do not oscillate, allocate without bounds, or silently drop required gameplay content.

Acceptance: increasing total offline city size with the same resident neighborhood does not increase live actor/GPU residency proportionally. Crossing cells repeatedly reaches a bounded memory plateau; cancellation, teleport, and teardown release reservations and leave no duplicate actors or dangling native references.

## Phase 5 — Distant city representation and static spatial data

After residency and correctness are established:

- Add HLOD/proxy cells for distant skyline/building groups; stream interiors separately. Show a coarse proxy until detailed cells are ready and switch coherently.
- Batch or instance compatible repeated static geometry while preserving picking IDs, per-object visibility, material compatibility, shadow behavior, and reimport invalidation.
- Reuse static cell bounds and the existing native transform/cull publication. Dynamic actors maintain separate dirty spatial updates. Camera changes still recull static bounds.
- Account for off-camera shadow casters and light-dependent demand. A visibility-only eviction rule would produce disappearing shadows.
- Measure whether a retained spatial index, fewer draw submissions, or material preparation is the next bottleneck before selecting further work.
- For genuinely large coordinates, add a coordinated floating-origin policy only with physics, rendering, navigation, audio, and save/load support. Rebasing invalidates affected static publications in one batch; static cannot mean permanently frozen coordinates.

Acceptance: representative city traversal improves measured frame time and memory while preserving scene appearance, collision continuity, selection, and shadow coverage. Report CPU and GPU improvements separately.

## Validation and rollout

Deliver phases as separate reviewable changes in dependency order: instrumentation/contracts → static dirty scheduling → renderer/node synchronization → cell format/cooker → bounded residency → HLOD/batching.

Extend existing coverage rather than create a parallel test framework:

- `ActorTransformTests`, `GraphicsSceneNodeTests`, and `ClawSceneNodeTransformContracts`: static/dirty hierarchy, deferred creation, local/world semantics, negative/nonuniform scale, reparenting, and unchanged-frame counters.
- `ComponentTestsMeshRenderer`, mesh reimport, prefab, and async scene tests: targeted initialization, static toggles during loading, cancellation, rollback, duplicate requests, and reused handles.
- Native scene/transform/cull and Claw lifetime contracts: immutable snapshots, allocated entry counts, stale generations, concurrent attach/detach, shared views, and delayed retirement.
- `WorldStreamingContracts` plus actor-cell tests: budget exhaustion, shared dependencies, corrupt/missing cells, version mismatch, cancellation at every state, project switch, teleports, boundary oscillation, deterministic reload, and persistent gameplay state.
- Scale tests: one resident district within progressively larger cooked cities; compare cold and warm startup, stationary frames, traversal, worst-case cell activation, and peak CPU/GPU/staging memory. Keep a legacy full-load reference where the content fits, and compare visible actors/bounds/render output.

Build affected libraries and the Editor with matching binaries. Run focused correctness tests on each phase, native sanitizer/lifetime checks where supported, and a live rebuilt Editor/game validation using the exact New York asset plus a larger partitioned city fixture. Plans, test registration, and compilation alone do not establish performance or memory residency.

Proposed measurable gates, finalized after Phase 0:

| Scenario | Gate |
| --- | --- |
| Stationary static district | Zero unchanged actor-world recomputations/native pose writes after warmup; no recurring transform-path allocations. |
| One changed branch | Each affected actor is recomputed/published at most once per commit; independent dirty children are never skipped. |
| Active streaming | Admission stays within configured participating CPU/GPU/collision and live-actor limits, including staging/retirement. Track unbudgeted engine allocations separately. |
| Large offline city | Runtime initialization reads compact metadata plus demanded cells; full city payload/actor graph is never materialized. |
| Repeated traversal | Resident memory plateaus for a fixed route/working set; no monotonic actor/resource growth. |
| Responsiveness | Configurable installation/upload budgets meet measured p95/p99 frame goals; record single-operation overshoots and split oversized content. |
| Compatibility | Existing assets, Lua APIs, editor editing/undo, reimport, interpolation, scene save/load, and package execution remain valid. |

## Source and plan entry points

- [GameActor.cpp](C:/dev/Workphone/Engine/cpp/Source/Workphone/Scene/GameActor.cpp:977), [GameManager.cpp](C:/dev/Workphone/Engine/cpp/Source/Workphone/Scene/GameManager.cpp:406), [TransformSystem.hpp](C:/dev/Workphone/Engine/cpp/Include/Workphone/Scene/TransformSystem.hpp).
- [Renderer.cpp](C:/dev/Workphone/Engine/cpp/Source/Workphone/Scene/Components/Renderer.cpp:629), [MeshRenderer.cpp](C:/dev/Workphone/Engine/cpp/Source/Workphone/Scene/Components/MeshRenderer.cpp:240).
- [ClawSceneNode.cpp](C:/dev/Workphone/Engine/cpp/Source/WPGraphics/ClawSceneNode.cpp:115), [native scene nodes](C:/dev/Workphone/Engine/c/Source/WorkphoneGraphics/workphone_graphics_scenenode.c:407), [native transform jobs](C:/dev/Workphone/Engine/c/Source/WorkphoneGraphics/workphone_graphics_transform_job.c).
- [WorldStreamingScheduler.hpp](C:/dev/Workphone/Engine/cpp/Include/Workphone/System/WorldStreamingScheduler.hpp), [resource/asset production plan](C:/dev/Workphone/docs/WPRESOURCE_ASSET_PRODUCTION_PLAN.md), [graphics optimization plan](C:/dev/Workphone/docs/WPGRAPHICS_OPTIMIZATION_PLAN.md).
