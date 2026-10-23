# Static transforms and actor-cell streaming

The existing actor, component, prefab and scene APIs remain available. Static actors reuse their transform publications until their pose, descendants or render objects change. Large worlds can opt into independently cooked actor cells through `WorldStreaming`.

## Cooking a city

Build the `WorkphoneSceneWorld` target. Import the GLB normally once to produce the engine's prefab, meshes, materials and textures, then cook that prefab offline:

```powershell
WorkphoneSceneWorld.exe INPUT.prefab OUTPUT_DIRECTORY --asset-root PROJECT_DIRECTORY --cell-size 256 --max-actors 64 --render-only-static
```

The tool writes `world.wpworld`, independent `.wpcell` payloads and `world-preview.fbscene`. It never changes the input prefab or GLB. `--render-only-static` explicitly permits converting render-only groups and flattening componentless grouping nodes while retaining world poses. Without it, only explicitly static render-only groups become disposable cells. Scripted, physical, animated, externally referenced and unknown-component groups stay in the persistent layer. An inseparable persistent group that exceeds the cell limits is rejected.

Geometry bounds determine cell ownership, including meshes with all actor positions at the origin. A building has one owner and expanded coverage bounds; it is not copied into every overlapping grid square. Actor/component UUIDs and existing asset references are retained. The manifest is published only after all new payloads are written, so a failed cook leaves the previously published generation usable. Repeated cooks currently leave older generated payload files on disk; remove obsolete generations only when no running world uses them.

Use project-relative asset paths and place the output under the project directory for relocation. Package the manifest, its referenced payloads, the existing cooked mesh/material/texture dependencies and their resource catalog. Runtime consumes cooked files, not the source GLB. This version reads loose world files through the filesystem; world files inside archive packages require an additional I/O adapter.

## Editor and runtime

Load `world-preview.fbscene` in the Editor. Its persistent `WorldStreaming` component explicitly uses the Editor camera for preview demand. For gameplay, attach the component to a persistent player/interest actor, or assign its `Observer` property, and disable `Preview Editor Camera`. The component runs on the scene owner task and opens `World Manifest` relative to the project directory.

The native `SceneWorldStreamer` API accepts up to eight interest positions per update. Gameplay, collision and spectator interests must be combined deliberately; preview-camera demand should not replace gameplay demand.

Configurable controls include load/retain radii, actor/cell residency limits, pending requests, minimum residency, install count and install time. Radius hysteresis and minimum residency reduce boundary churn. Teleports replace obsolete requests. The scheduler reads/decodes immutable payloads on its existing workers; the scene owner installs complete bounded groups. Rendering is paused for a cell's graph commit, not for disk reads or the full city.

Installation is atomic at cell granularity. The time limit is checked between cells; a single expensive cell can overshoot it. `installBudgetOvershoots` and `lastInstallMilliseconds` expose this. Reduce cell actor/byte limits when tuning activation latency. Actor admission also checks the existing global actor pool.

Installed roots and their descendants adopt the destination scene's Edit or Play state. This runs the normal component activation path and publishes renderer poses for actors added after initial scene loading. A scene that has not entered either mode keeps its existing loading behavior.

Edits to a disposable cell are compared with its installed baseline before eviction. Changed serialized state is retained in a budgeted in-memory overlay and reapplied on reentry, with the same UUIDs. Overlay allocation failure prevents eviction instead of dropping edits. Overlays last for the open world session; permanent game saves must persist them separately. Making an actor dynamic promotes its cell to retained ownership. `pinActor(uuid)` explicitly retains an actor's cell before gameplay acquires long-lived references. Pinned demand that exceeds capacity reports an error. Closing the world explicitly destroys its owned actors.

Baseline fingerprints exclude the runtime `state` field only from component records, so deferred activation and Edit/Play changes do not create edit overlays. This normalization applies to the hash copy; retained overlay payloads keep their complete component state data.

Scene generation changes cancel late publication. Failed cell reads remain errors until demand changes or the world is reopened. Graphics lifecycle tickets reject cancelled queued loads/reloads and superseded unloads. Retirement retains participating payload reservations until retained graphics objects finish unloading.

After graphics retirement, the mesh manager releases reloadable CPU geometry when only its cache and the retirement transaction own the resource. Cache acquisition uses the same lifecycle lock as eviction. Live components, queued loads, gameplay resource references, direct mesh references and other resources sharing geometry prevent reclamation. External weak references also conservatively pin resources/geometry because they can be promoted independently. Resource UUIDs, settings and paths stay cached for later loading; direct manager load APIs rehydrate evicted entries. Procedural meshes remain resident because they have no reload source.

Retirement retries identity tickets as external owners release their pins, examining at most 32 candidates per coordinator update. Tickets add no resource references and do not keep decoded cell payloads alive. Closing or destroying a coordinator transfers remaining work to the existing Primary job queue's coroutine path, where each update performs one bounded pass. Pending graphics objects keep their participating source reservations until unload acknowledgement. Without a running job service, close exhausts currently reclaimable CPU work; manager teardown handles remaining pinned resources. This is conservative reclamation, not a total asset memory cap.

The manifest builds a balanced bounds index once when opened. Observer queries visit nearby coverage bounds, with persistent/pinned/resident cells considered separately. Cell ownership remains unique even when geometry crosses many grid squares. Queue batches adapt to the shared scheduler's pending and ready capacities, including a service configured with one queue slot.

## Measurements and limits

`TransformSystem::statistics()` exposes local/world recomputations and unchanged writes. Its revision counters are independent of slot lifetime generations. Clean static subtrees return before visiting components; dirty descendants still invalidate their ancestors. Normal updates consume dirty queues rather than scanning all transform storage. Hierarchy/component membership locks are released before transform callbacks, allowing render capture to acquire them without a lock inversion. Native nodes continue to use the existing C transform/matrix caches and shared graphics lock.

Transform registry allocation/removal locks the complete slot transaction, preserving handles during concurrent cell activation and retirement. Actor transform acquisition retains ownership under the same short gate used for replacement; transform conversion and renderer publication keep that reference through their calls. Removed transforms are released after unlocking. State contexts dispatch retained inventory snapshots and lock duplicate insertion/removal transactions, so concurrent node retirement cannot invalidate the dispatch array.

Scene-node identities are allocated under a shared gate before their states enter the shared context. The Claw camera adapter converts the public degree-based field of view to native radians, and ImGui images use the texture's shader-resource view, including render targets.

Claw mesh material, visibility, shadow and geometry mutations share render capture's graphics lifecycle lock. Application-task activation cannot invalidate a native draw snapshot midway through capture/submission. Resource load/reload keeps its existing dispatch and render-pause ordering.

Factory pools serialize free-slot acquisition with block growth. Concurrent callers cannot consume the last published slot between an emptiness check and allocation, or spin waiting for a fixed-capacity pool to grow.

Scene resource inventory refresh publishes complete snapshots under its map lock. Replaced objects remain retained until after publication unlocks; lookups retain their result before releasing the read lock. Anonymous objects with nil UUIDs are excluded from this lookup cache. This fixes a live Editor failure where concurrent scene refreshes prematurely destroyed transforms still owned by actors. Scene collection appends components directly, avoiding repeated rebuilding of the accumulated array.

Scene membership events coalesce resource refresh into one pending Primary job per database. A concurrent mutation during a scan schedules a follow-up through the existing coroutine service, at most once per later update. Deferred scans verify that their database is still loaded and active before using its catalog. Explicit database refresh stays synchronous. This avoids reloading the catalog and rescanning the entire scene for every actor in an installed cell.

`SceneWorldStreamer::statistics()` reports actor/cell counts, read/install/eviction/failure counts, source payload reservations, pending work, commit overshoots and released CPU mesh resources. Readers receive the latest completed owner-operation snapshot, so UI inspection does not traverse mutable residency containers. `indexNodesVisited` and `indexCellsTested` describe the most recent observer query. The shared scheduler reports reservations by memory class and its queue capacities. Source/decoded payload sizes are conservative estimates, not measured total process memory.

This implementation bounds participating actor/cell payload work. Existing global mesh/material/texture caches and backend GPU allocations do not yet participate in a complete shared resource residency budget. A native mesh object's unload releases its renderer-owned geometry, but this is not a guarantee that all global cached CPU/GPU assets are reclaimed. HLOD/proxy generation, shared resource leases, draw batching/instancing, persistent save overlays and floating-origin coordination remain separate phases of the design plan. Do not interpret cell actor limits as a cap on total process or GPU memory.

## Validation

Build `WPActorSceneTests`, `WorkphoneSceneWorld` and `WPGraphicsClawProductionTests` against matching engine binaries. The focused actor test executable includes legacy actor-transform, mesh-renderer, graphics-node, reimport and async scene-loading cases alongside `SceneWorldStreamingTests`. The latter covers independent cell reads, repeated traversal, request batches, small shared queues, local index queries, cancelled scene publication, edit overlays, dynamic retention, failed reads, CPU mesh retirement/pins and static transform reuse. Native node contracts cover deferred publication and world/local conversion below rotated, scaled parents.

Run the registered `Workphone.actor_scene_streaming` CTest and the native Claw contracts. Real city cooking, visible rendering and frame/memory measurements must be recorded separately from these correctness checks.

The 2026-10-23 RelWithDebInfo build passed both `Workphone.actor_scene_streaming` and `Workphone.state_listener_fanout`. A broader selected run passed 111 cases and 6,105 assertions; 16 unrelated cases were excluded by its filter. Concurrent ownership coverage includes transform slot registration/removal, final actor transform replacement with four readers, complete state inventory removal during a callback, scene-node identity isolation, contended pool growth/exhaustion, native mesh mutation blocked by render capture, concurrent resource snapshot publication, and refresh burst/reentry/unload/replacement handling in both job queues. Streaming mode coverage checks Edit/Play descendants, renderer pose publication, clean eviction, edit preservation and rejected overlay capacity reductions. Native Claw lifecycle coverage separately passed 32 soak iterations. These checks do not establish a city frame-time improvement or a complete CPU/GPU memory ceiling.

The same build rendered a district cooked from `new_york_3.glb` in the DX11 Editor: 279 resident actors in six cells from a 1,212-actor, 42-cell cook. The final run recorded no stale-colour-snapshot failures, offscreen-job failures or engine assertions, and exited with code 0. Original project and Editor settings were restored byte for byte. City-distance eviction/reentry remains covered by deterministic tests rather than a manually completed Editor route; no comparable startup or frame-time benchmark was recorded.
