# Terrain authoring workflow

The [29–30 October GT0 completion record](WPGRAPHICS_TERRAIN_GT0_COMPLETION.md) tracks
baseline reconciliation, focused validation and separate-process packaging.
Historical results below remain distinct from fresh candidate evidence.

The live Editor follow-up uses the normal Application event queue for terrain Lua
callbacks and caches viewport brush/recipe state outside the render frame. This
avoids a Lua/texture-manager lock cycle found during real component selection.
Selection and lifecycle generations discard queued rays from a retired terrain.
Terrain-specific adapters use LuaManager's Parameters/mutable-results ABI;
ordinary Lua return values and positional arguments are not that native ABI.
The nested terrain panels now fill the Inspector width so the final tabs remain
accessible when docked.

The live walkthrough exposed a second input boundary: terrain image coordinates
are normalized UVs, whereas `GraphicsCamera::getRay` expects pixels. Viewport tool
rays must use the existing `makeScenePickingRay` path with the image rectangle and
camera matrices, as scene drops and ordinary selection already do. Compilation
or a directly invoked world ray does not verify that pixel-to-ray boundary.

Terrain phase selection gives cancellation priority over release when the pointer
is outside the displayed image. This also covers a short drag whose first sampled
outside frame is the release frame. A successful active-stroke cancellation updates
the status; idle outside hover preserves the previous completed-stroke status.
The scene-picking and native Lua bridge regressions use this production phase
selector. Live input and separately saved payloads remain required evidence.

The [30 October observed walkthrough](../artifacts/terrain/gt0-20261029/editor-live-09/session-evidence.json)
records this repaired path at 96 DPI: native scene open and terrain save, a bounded
viewport sculpt, native-menu Undo/Redo, short-drag release-outside cancellation,
native scene Save As and distinct-process scene reload. Separately saved files
match byte for byte after Undo, Redo, cancellation and restart. This closes that
specific sequence; Escape/focus/selection interruption, other DPI/docking
geometries and live layer/hole paint remain separate checks.

`Tools/ValidateTerrainAuthoringEvidence.ps1` checks native v3 snapshots for a
bounded nonzero sculpt, Undo restoration, Redo/cancel preservation, native scene
component ownership and saved/restarted content. It rejects coercible invalid
JSON and native-invalid material/coordinate ranges. Content matching does not
attest mouse input, actual scene loading or a process restart; retain the live
action, process and capture evidence separately. Hardware and independent visual
acceptance are separate gates.

`Tools/ValidateTerrainCleanMachine.ps1 -Mode Prepare` creates an offline kit from
an explicitly pinned optimized gameplay archive and its accepted package report.
Prepare does not run a guest or modify host features. Use the generated README's
hash-pinned Launch/Collect commands on an already enabled Sandbox host; collection
requires fresh guest identity, pre-provisioning CRT/software inventory and native
package evidence. Restricted PATH execution on this development host is not a
clean-machine pass.

This guide describes the native terrain authoring services and the existing Lua Terrain Editor extended in this change. It is a workflow and format reference, not a claim that every release gate in [the terrain plan](WPGRAPHICS_TERRAIN_PROCEDURAL_REVIEW.md) has passed. Build, automated, GPU, interactive and packaged evidence are recorded separately in the implementation status and validation reports.

## Runtime tiles and cooked assets

Claw partitions the source into 64-cell tiles with four interior resolutions
(sample strides 1, 2, 4 and 8). Every tile boundary retains the original samples
and normals; a tile with holes stays at exact resolution. The projected error
budget defaults to two pixels with a separate coarsening threshold. Scene camera
demand is combined with the shadow view and a 15% view margin for prefetch.
The native adapter supports up to fifteen scene cameras plus a shadow view.

Multi-tile CPU geometry is generated asynchronously through a shared two-worker
world streaming scheduler. Single-tile assets use a bounded synchronous tool path
with the same working/result memory admission. Each terrain producer permits at most 256 pending requests,
one executing job, eight completed tiles and 8 MiB of reserved result capacity waiting
for upload. Priority bursts alternate with fair producer selection. Cancellation
invalidates older generations and unload drains its producer. Camera distance-order changes preserve useful work;
changed view membership reuses overlapping staged tiles. GPU uploads default to
four tiles per native frame, shared across its scene views.

The default geometry budget is 2,048 resident tiles and 128 MiB each for active
and staged generations. These are **geometry budgets**, not a total application
memory guarantee: authoritative source samples/weights, collision, CPU work,
texture dependencies and the material control map use additional memory. A
replacement must fit its complete staged budget before publication; otherwise the
last complete terrain remains active and `getTerrainResidencyError()` explains
the failure. `getTerrainResidency()` exposes pending, staged, active, completed,
cancelled and upload counts/bytes. Unchanged terrain does not re-upload geometry.
Use `setTerrainResidencyLimits()` before first publication to select stricter
limits; changing limits on a loaded generation is rejected.

`WorldStreamingScheduler::shared()` coordinates participating producers. Its
defaults are two workers, 128 producers, 2,048 pending jobs and 64 ready jobs.
Separate shared reservations cover source CPU data (512 MiB), pending job data
(16 MiB), working CPU storage (128 MiB), result storage (64 MiB), GPU resources
(512 MiB) and collision (512 MiB). `setBudget()` rejects a reduction below live
reservations; `stats()` exposes limits and reservations. GPU buffers shared by
active and staged generations are charged once. Owned control/weather maps and cooked uploaded textures also retain GPU leases. Source references held by
different consumers may be charged conservatively more than once. Ready-byte
counters report reserved capacity. These limits cover participating resources,
not textures borrowed from other owners, authoring history or arbitrary caller-owned allocations.

If a separate stream source reservation leaves insufficient room for job
metadata, the renderer retries using its already-admitted reservation for that
same immutable snapshot. Stream callbacks and retained mesh results keep that
lease alive after caller release or cancellation. Shared leases must belong to
the producer's scheduler ledger, cover the source byte count and use `SourceCpu`;
standalone stream requests retain independent source admission. Superseded
staging retires before cancellation needs bookkeeping, and failed requests do
not commit a new demand/device state.

For packaged loading, register `TerrainResourceCompiler` with the existing
ResourceSystem registry and compile a `data://...terrain` descriptor containing
the same version-three JSON as an editor save. Each nonempty layer map must name a
`data://...texres` texture descriptor. The cooked terrain stores its exact texture
generations and a versioned tile manifest (bounds, geometric errors, cell extents,
hole state and border policy). Version-one and version-two cooked payloads remain readable.

Use `decodeCookedTerrainResource()` on a CPU worker, then
`stageCookedTerrainMaterial(renderer, decoded, error)` on the graphics owner and
`ClawTerrain::applyCookedTerrain(candidate, error)`. The sealed candidate retains
its dependencies and validated tile metadata; first render does not rescan the
whole source to rediscover the manifest. No compiler or authoring database is
needed by this consumer. Descriptor, dependency, target, build-mode, hash or GPU
failures preserve the prior published asset.

When a `TerrainSystem` owns the scene ground, reconcile the decoded terrain through
`TerrainSystem::applyTerrainData()` before attaching the cooked candidate to its
`ClawTerrain`. The equal-data application preserves the component's immutable
snapshot identity, so query, native collision and rendering publish one generation.
The [representative gameplay package](WPGRAPHICS_TERRAIN_GT0_COMPLETION.md#representative-terrain-gameplay-package)
uses this path in the existing `SampleVehicleAdvanced`/`ProceduralRaceScene`
lifecycle. It cooks four-layer terrain and pins a versioned runtime vehicle recipe
with a regenerated geometry/material/physics hash check; it does not export baked
vehicle mesh assets. Run `ValidateTerrainGameplayPackage.ps1`, or add
`-IncludeTerrainGameplayPackage` to the existing baseline builder, for its separate
driving, capture, negative-input and ZIP-extraction checks.

These bounded tiles stream generated render geometry from a resident heightfield.
Disk paging of height/control data and collision, adoption of the shared scheduler
by other world systems, and floating-origin rebasing remain outside this runtime tier.
Placement uses the existing finite float coordinate policy. Large world projects
must account for float precision and the collision limits below.

## Edit and save a terrain

1. Select an actor with `TerrainSystem` and open the existing Terrain Editor. The component owns an immutable rectangular heightfield; renderer recreation does not discard its authored data.
2. Set the local brush centre and radius in the Sculpt panel, or enable **Paint in Scene Viewport** and drag over the terrain. Choose raise, lower, smooth, flatten, noise, terrace, paint, stamp, hole or fill-hole mode. The native scene viewport supplies the world ray and terrain surface preview; Lua sends the operation to the native terrain service.
3. Release the viewport stroke to create one undo item. Escape, focus loss, leaving the viewport, changing selection or unloading the editor cancels the active stroke. A cancelled stroke restores its own heights, weights and holes. A conflict with a newer edit is reported rather than overwritten. Undo/redo preserves edits outside the command's affected values.
4. Use **Save** to write `.terrain.json`, then **Reload** to reopen it. Saved data includes dimensions, spacing, origin, height scale, all height samples, four material descriptors, appearance settings, control weights, wetness/snow masks and holes. Scene serialization retains the same source data.

Brush size is a radius in terrain-local metres. Raise/lower/stamp/noise strength is a local height displacement, converted through the terrain height scale. Smooth/flatten/terrace/paint use strength in `[0,1]`. Negative height scale is supported; a zero scale renders a flat heightfield and rejects height editing. The current brush falloff is smooth radial falloff. Viewport updates are sampled at input events; distance-resampled strokes, tablet pressure and mirrored strokes are not enabled.

**Stamp** accepts a binary PGM height image with normalized samples; an empty stamp path uses a smooth radial mound. **Terrace** uses the displayed terrace interval in local height metres. **Noise** uses a deterministic integer hash of the sample index and seed, producing bounded additive sample noise.

Hole values belong to cells, not vertices. A painted hole removes both runtime triangles of that cell from rendering, gameplay ray/height queries and collision. In **Fill Hole** mode, the Editor picks and previews the retained authored heightfield inside the void, allowing a repair stroke to start within a hole. Other tools retain solid-only picking. The repair query shares the existing triangulation and placement rules and creates no fallback collision plane.

## Four terrain materials and masks

Terrain has four fixed material slots, numbered `0` to `3`. Select a slot and assign stable resource identifiers or resource paths for albedo, normal and packed ORM. The ORM channels are occlusion, roughness and metallic. The editor exposes UV cycles per local metre, roughness, metallic and normal strength. Reading or changing the selected slot does not replace another slot's settings.

Painting increases the chosen layer's weight and proportionally reduces the others. Clearing a layer redistributes its weight across the other channels. Normalization has a deterministic layer-zero fallback for an all-zero value. Empty control data also means layer zero everywhere. Weights use the full height sample grid, so rectangular terrain and tile borders use the same authored coordinates.

**Apply Biome Layer Rules** and **Auto Paint Layer** apply the selected layer's height/slope filters within the configured painting radius. These operations create actual normalized control weights and support undo. They do not generate tree species or invent a separate vegetation database.

**Export Height/Slope Rule Mask** writes a real PGM16 image of the current height/slope/radius coverage. **Export Hydraulic Displacement Mask** prepares the configured erosion result, computes absolute predicted displacement, normalizes it by its maximum, and writes a PGM16 image without modifying the terrain. A zero-displacement result produces a black image. Use foliage's placement rules and the shared terrain integration inputs to consume appropriate masks and surface data.

The terrain-wide appearance panel controls planar/triplanar projection, height blending, detail normals, macro variation, wetness and snow. Height blending reads linear ORM alpha with per-layer height scale/bias and a global transition width. Detail normals use one optional resource with their own tiling/strength. Macro tint uses a stored seed and terrain-local coordinates, so it is stable across tile LOD changes. Wetness changes colour/roughness; snow changes colour/roughness and mapped normals. These controls update persistent terrain data through undoable commands.

Paint/clear wetness and snow with the same direct or viewport brush workflow. Each sample stores two independent mask channels; painting one preserves the other. Empty masks mean full coverage, while global wetness and snow start at zero, leaving older assets unchanged. Grouped strokes, cancellation, undo and ordered recipes include mask edits. Legacy per-layer projection/UV-set/UV-offset/UV-rotation and standalone AO controls remain explicitly unavailable; terrain-wide projection and packed ORM provide the supported paths.

## Height and mesh interchange

Set **Output Path** and choose **Height Encoding** before importing/exporting. The supported formats are deliberately explicit:

| Encoding | File extension | Sample layout and precision |
|---|---|---|
| PGM image | `.pgm` | Binary P5; import accepts 8-bit and 16-bit grayscale; export writes 16-bit big-endian samples with maximum 65535 |
| Raw U16 | `.raw` or `.r16` | Unsigned 16-bit little-endian samples; raw width and depth are required for import |
| Raw Float32 | `.raw` or `.r32` | Finite IEEE-754 float32 little-endian samples; raw width and depth are required for import; finite sample bits roundtrip exactly |
| Terrain geometry | `.obj` | Actual local-metre positions, UVs, normals and triangle faces, with holes omitted |

All height files are row-major, `index = z * width + x`. U16 import/export uses the explicit **U16 Minimum/Maximum** values in unscaled sample units. Export rejects samples outside that range; it does not silently clamp. The error bound from a U16 roundtrip is half an encoding step, subject to float rounding. Raw float is appropriate for exact signed heights. **Export Height Data** chooses raw Float32.

Height import preserves spacing, height scale, material descriptors and appearance. If dimensions change, the new grid is centred around local zero and its weights, surface masks and holes reset; if dimensions match, those values and authored placement remain. Failed or truncated imports retain the previous snapshot. Files cannot be exported with misleading extensions. PNG, TIFF, EXR and other image codecs are not implemented by this height interchange service; assigning a texture preview is distinct from decoding source height samples.

The `.terrain.json` format is version 3. Version 1 files migrate to a solid terrain with default layer-zero weights and default material descriptors. Version 2 retains its weights, layers and holes with inert defaults for the new appearance effects. Revisions are publication tokens and are not restored from files. Saved source files use an atomic same-directory replacement; an unsuccessful write leaves the old file intact. Export refuses source JSON larger than the reader's 128 MiB cap.

## Ordered generation and cancellation

The **Ordered Non-destructive Recipe** panel retains a base heightfield plus an ordered operation list. Enable **Queue Actions into Recipe** before using sculpt, paint, erosion or road/river grading actions. Operations remain editable: select a zero-based index to reorder, disable or remove one. **Reset Base** explicitly captures the current authored terrain and clears the previous list.

**Prepare Preview** evaluates the list on a worker. The Scene Viewport overlays the prepared triangle edges, holes and diagnostic layer-weight colours on the current scene; the overlay does not modify live terrain or collision. It uses the selected actor's transform, omits subpixel edges and is bounded by the recipe's 65,536 samples. The wireframe is an authoring comparison, not a textured material preview. **Commit Prepared Result** hides the overlay and submits the complete result through normal terrain publication as one undo item. **Clear / Cancel Preview** removes it without changing terrain. Manual edits or undo invalidate both the displayed candidate and its commit. Geometry recipes preserve current material descriptors.

**Cancel Preparation**, recipe edits and unload cancel and drain the worker. Generation tokens prevent a retired worker result from publishing. Failure retains the authored terrain. Save/load ordered recipes as `.terrain.recipe.json`; this file contains the versioned base and complete operation parameters, including stamp samples and road centreline points. Reopening a recipe requires a selected terrain with matching grid metadata. It never silently changes the grid. The older **Export Recipe Settings** action remains a separate editor-settings export.

Thermal erosion exchanges excess material above the configured talus slope with neighbouring selected samples. Hydraulic erosion applies deterministic rain, water flow, sediment capacity, erosion/deposition and evaporation; retained sediment is deposited before producing the candidate. Both preserve samples outside the selected write region. Preparation reads a clipped halo from the same authoritative heightfield: one cell per thermal iteration or two cells per hydraulic iteration. Adjacent regions prepared from the same base therefore reproduce the whole-domain simulation at their shared border. The halo may affect the result but is never written outside the selection; a partial write need not conserve total terrain mass across that boundary. These are bounded CPU authoring algorithms. Their direct action buttons run a bounded synchronous operation; queue them into the recipe stack for cancellable worker preparation.

## Roads, rivers and engine integration

The road/river centreline file contains plain local-metre `x y z` rows, with optional `#` comment lines. Road grading interpolates centreline elevation across the road width, then blends through the shoulder. River grading additionally lowers a parabolic bed by the configured depth. The native command changes only samples inside that footprint and records their prior values for undo. It does not generate road meshes, water surfaces or an unrelated spline asset format.

Terrain authoring state may be ahead of the active runtime revision. The editor reports **awaiting render/collision publication** while the previous complete runtime revision remains active. Renderer and physics preparation must succeed before the publication gate switches active geometry, picking and collision together. Collision errors are surfaced to the editor. **Build Collision** requires an attached supported `CollisionTerrain`/physics body and verifies that it matches the active source. **Rebuild LOD** requests a refresh from the attached terrain renderer; it is not a substitute for a configured renderer.

Authoritative collision rebuilds synchronously at the final runtime publication boundary, at full resolution, and omits holes. Preview edits update requested authoring data without rebuilding collision while the physics manager is busy. Its ceiling is 524,288 solid triangles, equivalent to a fully solid 513 × 513 sample field. An edit above that bound fails before mesh allocation and preserves the previous requested source and active render/query/collision state. `TerrainCollisionData.maximumTriangles` may lower this limit but cannot raise it. Initial attachment also rejects a rigid body with all 16 shape slots occupied; replacements reuse their existing slot. Larger source grids remain supported for graphics and authoring, while tiled collision streaming is deferred.

**Export Navigation Input OBJ** exports hole-aware transformed geometry for a navigation consumer. **Export Shoreline Input JSON** exports terrain/water-plane intersection data with its source revision. These are explicit inputs to navigation and water systems; the actions do not claim to build a navmesh or render a water plane. Navmesh generation, water simulation, foliage species authoring, arbitrary modelling/CSG and other engines retain their own plan ownership and validation gates.

Native consumers can retain a `TerrainIntegrationBinding` on a `TerrainSystem`. Enable ground, navigation, shoreline and/or path inputs in `TerrainIntegrationOptions`; the binding refreshes one immutable input bundle after each successful active publication, outside the terrain/render/physics publication locks. Pending authoring edits keep the previous bundle. A failed input build keeps the complete previous bundle and exposes `getLastError()`; `configure()` can change the budgets and rebuild the latest active revision. The default navigation budget is 524,288 triangles and shoreline budget is 262,144 segments, with a hard ceiling of 1,048,576 for either option. Runtime replacement clears inputs until a matching active generation exists; `unbind()` or owner unload ends the subscription. These retained inputs do not themselves regenerate external navmeshes, water rendering or GPU foliage batches.

`TerrainGroundPopulation` accepts optional per-sample density and per-cell exclusion grids through `setMasks()`. Density acceptance is deterministic for each stable seed ID. A seed can require a minimum combined weight from selected terrain layers; exact triangle interpolation and the shared zero-weight fallback match terrain sampling. Height/hole/mask edits resample affected cells, while weight-only edits resample only seeds with layer filtering. Retained-binding and mask contracts pass in Debug and RelWithDebInfo.

## Capability and evidence matrix

This maps the implementation to TERR-01-10 in the terrain plan. The evidence column identifies executable coverage, not automatic release approval. The historical 11 October full graphics baseline recorded 19 passed and one external-media test unavailable per configuration. The [29 October focused follow-up](WPGRAPHICS_TERRAIN_GT0_COMPLETION.md#final-evidence-29-october-2026) records seven mandatory terrain suites passed per configuration, two Debug Editor contracts, both Editor builds and both source-free fixture ZIPs. The [representative gameplay follow-up](WPGRAPHICS_TERRAIN_GT0_COMPLETION.md#representative-gameplay-evidence-29-october-2026) then reconciles seven fresh suites per configuration and both terrain/vehicle gameplay ZIPs, including extracted driving and separate captures. Actual GPU execution used DX11 WARP. Live Editor interaction, independent visual acceptance, representative hardware/performance, clean-machine deployment and the broader release soak retain their own gates.

| Plan row | Implemented capability and bounds | Evidence and remaining scope |
|---|---|---|
| TERR-01 | Version-three rectangular source, version-one/two migration, explicit metre spacing/origin/height scale, finite/overflow validation including renderer coordinates and UV tiling. At most 8193 per axis and 4,194,304 samples. | [Data contracts](../Tests/cpp/TerrainDataContracts.hpp) exercise asymmetric grids, scale policy, transforms and invalid input. Placement supports positive nonuniform actor scale and yaw; arbitrary pitch/roll and floating-origin rebasing are unsupported. |
| TERR-02 | Shared triangle-consistent picking/height inputs, mesh normals, world bounds and full-resolution native collision. Publication retains previous active geometry/query/collision while replacement is pending. Collision is capped at 524,288 solid triangles. | [Physics contracts](../Tests/cpp/TerrainPhysicsContractsTests.cpp) use native rays, narrowphase contacts, a falling probe, holes, busy-manager and publication/lifecycle cases. The representative scene adds terrain-border/hill/hole probes and five-phase vehicle driving in copied/extracted packages. Collision paging, hardware and interactive acceptance remain separate. |
| TERR-03 | Four albedo/normal/ORM layers, normalized controls, height blending, triplanar/detail normals, seeded macro tint and wetness/snow masks; common shadows/fog and staged/cooked dependencies. | [Material pixel contracts](../Tests/cpp/ClawTerrainMaterialContracts.hpp) cover layer colours, normal/ORM response, shadows, zero controls and failure retention. Appearance is opaque and limited to four layers, one detail-normal texture and two surface-mask channels; weather simulation and additional material layers remain separate work. |
| TERR-04 | PGM/raw import, exact float-height and OBJ export, sculpt/stamp/paint, dirty rectangles, viewport picking/brush display, grouped commit/cancel/undo and persistent weights/holes. A stroke changes at most 65,536 values. | [Authoring contracts](../Tests/cpp/TerrainAuthoringContracts.hpp) and [actual Lua workflow](../Tests/cpp/TerrainLuaBindingTests.cpp) cover save/reopen, decoding, undo and cancellation. Native viewport wiring is present; interactive usability/visual sign-off remains separate. PNG/TIFF/EXR, tablet/mirror modifiers and distance-resampled strokes are unsupported. |
| TERR-05 | 64-cell tiles, four interior resolutions, conservative errors, full-resolution shared boundary rings/normals, culling and coarsening hysteresis. Tiles containing holes retain LOD0. | [Tile contracts](../Tests/cpp/TerrainTileContracts.hpp) and [moving-camera GPU contracts](../Tests/cpp/ClawTerrainMotionContracts.hpp) cover borders, changing demand and complete replacement. The chosen border-ring policy replaces morphing; broad visual threshold and shadow-camera acceptance still requires representative scenes. |
| TERR-06 | Versioned cooked tile manifests; asynchronous CPU geometry generation; prefetch, cancellation, eviction and bounded staged uploads for up to fifteen scene views plus shadow demand. Default geometry budgets are 2,048 resident tiles and 128 MiB per active/staged generation. | [Residency contracts](../Tests/cpp/ClawTerrainStreamingContracts.hpp) and [cooked terrain contracts](../Tests/cpp/CookedTerrainContracts.hpp) cover runtime replacement, warm reuse and source-free asset loading. Heights/controls remain resident; shared CPU/GPU/collision reservations coordinate participating producers; disk paging of those sources or collision, other world-producer adoption and origin rebasing remain broader work. |
| TERR-07 | Terrain ground/normal/hole inputs, stable placement identities, dirty-cell resampling, real rule/displacement masks, deterministic density/exclusion and layer-weight acceptance, retained input refresh, and an adapter to the existing foliage batch. A ground population has at most 65,536 seeds. | [Integration contracts](../Tests/cpp/TerrainIntegrationContracts.hpp) cover local edits, transformed placement, masks, filtered weight invalidation and retained input lifecycle. Species/biome authoring and GPU batch regeneration remain foliage-consumer responsibilities. |
| TERR-08 | Deterministic thermal/hydraulic erosion with authoritative read halos, terrace/noise, normalized masks and a persistent ordered recipe. Worker preparation is cancellable; the viewport shows a bounded geometry/weight wireframe without changing gameplay state. Recipes allow 32 operations over at most 65,536 base samples. | [Authoring contracts](../Tests/cpp/TerrainAuthoringContracts.hpp) compare adjacent halo results with whole-domain simulation and cover replay/reorder/cancel/stale rejection; Lua checks retain recipes across garbage collection and save/load. Erosion permits 262,144 halo samples and 16,777,216 sample iterations. Representative latency/peak-memory measurement remains acceptance work; the wireframe is not a textured preview. |
| TERR-09 | Persistent holes, bounded road cut/fill and parabolic river-bed grading, exact native collision, transformed navigation triangles, shoreline intersections and ground-projected path inputs. A retained binding rebuilds enabled inputs after active publication, retaining the prior complete bundle on failure. Grading supports 2-1024 points and at most 65,536 changed samples. | [Authoring](../Tests/cpp/TerrainAuthoringContracts.hpp), [integration](../Tests/cpp/TerrainIntegrationContracts.hpp) and [physics](../Tests/cpp/TerrainPhysicsContractsTests.cpp) fixtures cover preservation, holes, handoffs and subscription/budget failures. Navigation/water consumers own their generated products. |
| TERR-10 | This capability/format guide, [cooked format reference](WPGRAPHICS_TERRAIN_RESOURCE_FORMAT.md), focused CPU/Lua/physics/GPU/cooked targets, residency/error diagnostics and the baseline validation script. Unsupported editor actions report failure explicitly. | Build/test evidence is recorded independently in the implementation status and generated validation outputs. GT0/GT1 are not declared complete by these fixtures: interactive visuals, representative performance/soak, device coverage and final packaged configurations still require their own evidence. |

## Bounds and verification

| Operation | Bound |
|---|---|
| Source grid | At least 2 samples per side; at most 8193 per side and 4,194,304 samples total |
| Stroke / immediate undo patch | At most 65,536 changed values across heights, weights, surface-mask channels and holes |
| Recipe base / operation list | At most 65,536 samples and 32 ordered operations |
| Stamp | At most 65,536 normalized samples |
| Erosion | 1-1024 iterations; 65,536 write samples; 262,144 simulation samples including halo; at most 16,777,216 simulation sample iterations |
| Road/river | 2–1024 centreline points, at most 16,777,216 sample/segment evaluations and 65,536 changed samples |
| Height input / terrain JSON / recipe JSON | 32 MiB / 128 MiB / 32 MiB |
| OBJ export | At most 1,048,576 vertices and 256 MiB output |
| Authoritative collision | At most 524,288 solid triangles; synchronous full-resolution rebuild |

The authoring fixtures exercise grouped and cancelled strokes, undo/redo, material/weight/hole persistence, legacy migration, hole-aware rays and mesh export, the three height codecs, real rule/erosion mask decoding, deterministic erosion, cancellation, grading, ordered recipe replay/reordering, save/reopen and stale-commit rejection. The Lua fixture drives the actual existing editor through the native bindings. Test definitions alone are not execution evidence; consult the current validation report for which configurations and backend paths ran successfully.

From the repository root, build and run the graphics baseline with the Editor:

```powershell
./Tools/BuildWPGraphicsBaseline.ps1 -BuildDirectory project_terrain_validation -IncludeEditor
```

This builds Debug and RelWithDebInfo, runs the registered contracts and writes XML
results plus source/binary/device evidence. For an already configured tools build,
`cmake --build <build-directory> --config RelWithDebInfo --target WPTerrainValidation`
builds the terrain, physics, Lua, cooked-resource, GPU and Editor targets together.
The focused test selection is
the nine required tests in `ValidateClawGraphics.ps1 -TerrainOnly`, including
`WPEditor.scene_picking` and `WPLua.editor_deferred_script_events`. These input and
dispatch regressions must be built with the native terrain targets; a direct Lua
world-ray fixture alone does not cover viewport pixel conversion or queued input.
