# WPGraphics foliage production review and implementation plan

Source review: 9 October 2026 at `2a37b63ea`. Extends the [graphics production plan](WPGRAPHICS_PRODUCTION_PLAN.md) and [terrain/procedural review](WPGRAPHICS_TERRAIN_PROCEDURAL_REVIEW.md). User priority: **foliage LOD, lower batch count and fewer actual draw calls**. This is a review and implementation plan; no renderer implementation, new benchmark, build or GPU capture is claimed here.

## Recommendation and release outcome

Build a paged foliage renderer around Workphone's retained native C paging core, shared species assets, DX11 hardware instancing, per-instance or small-cluster LOD, tree impostors and dedicated grass rendering. Reuse working scene batching, LOD selection, mip filtering and asset services. Use spatial pages to organize residency and culling; combine compatible visible instances into draw groups after culling. Page count, job batches and scene actor count are different from GPU draw count.

**GF0 / R1 is mandatory:** cooked species/instances, near/mid/far foliage representations, measured draw-call reduction, bounded page residency, tree and grass rendering, coherent alpha/shadows/wind policy, basic deterministic scattering/painting, persistence and profiling. The essential LOD/batching work moves out of the earlier optional vegetation allowance.

**GF1 / R2:** robust textured impostor baking, richer plant generation and biome tools, scalable streaming/multiple views, advanced wind/interaction and integrated world authoring. GPU-driven culling/indirect submission is a later optimization after the CPU-culling/instanced baseline is measured. DX12 and other backends require separate certification.

## What Ogre PagedGeometry contributes

The upstream design organizes geometry into pages with configurable detail levels and a loader, using batches for nearby trees and impostors at distance. Its documentation also describes transition bands and the tradeoff between page size and loading cost. These are useful architectural references, not a performance guarantee for Workphone. [PagedGeometry introduction](https://github.com/OGRECave/ogre-pagedgeometry), [configuration tutorial](https://ogrecave.github.io/ogre-pagedgeometry/tut1.html).

Grass deserves a separate path: the upstream grass tutorial uses GrassPage/GrassLoader, density maps and terrain-height sampling, and discourages assigning a tree-style impostor to every grass blade. Workphone should use instanced clumps/cards with density and distance LOD rather than blindly reuse its tree representation. [Grass tutorial](https://ogrecave.github.io/ogre-pagedgeometry/tut3.html).

The implementation proposal below is Workphone-specific: hardware instancing, separate residency/culling/draw grouping, retained shared resources, immutable worker results and per-view draw lists. Importing the Ogre-dependent addon wholesale is unnecessary when the native paging core already exists. If upstream code is adapted, record its exact revision and retain the applicable [license notice](https://github.com/OGRECave/ogre-pagedgeometry/blob/master/LICENSE.txt).

## Current engine findings

| Area | Inspected evidence | Implementation implication |
|---|---|---|
| Native paging foundation exists | [C API](../Engine/c/Include/WorkphoneGraphics/workphone_graphics_paged_geometry.h) and [implementation](../Engine/c/Source/WorkphoneGraphics/workphone_graphics_paged_geometry.c) implement callback-driven pages, detail ranges, fades, reload/preload and loaded/pending counts; [CMake](../Engine/c/Project/WorkphoneGraphics/CMakeLists.txt) includes native graphics sources by glob | Reuse and contract-test the core before extending it. Paging alone does not render or batch foliage |
| C++ wrapper was deliberately removed | HEAD `2a37b63ea` deletes ClawPagedGeometry.hpp/.cpp. Searches of Engine/Tests/Tools found no remaining calls to the paging API outside its own C source/header | Integrate the retained C API from an internal foliage owner and page callbacks. Do not restore deleted wrapper classes as a prerequisite or claim an active renderer bridge exists |
| Paging callbacks execute synchronously | `wp_paged_load_page` calls loader/build callbacks inline and marks the page loaded; active-range loads can occur during update; grid scrolling allocates temporary storage | Add readiness/failure/cancellation state and bounded worker preparation/render-thread publication. A callback that queues work must not falsely mark a GPU page ready; profile scrolling and remove avoidable hot-path allocations |
| Paging model is camera/distance based | Native core stores one camera and detail-level grids; it has page bounds and range selection, not a complete per-instance/frustum/multi-view renderer | Keep residency selection distinct from per-view visibility and LOD. Extend through tested native APIs/internal adapters only where required; avoid swapping shared scene visibility between cameras |
| Terrain tree generation creates actors | [TerrainSystem.cpp](../Engine/cpp/Source/Workphone/Scene/Components/Terrain/TerrainSystem.cpp), generation paths, create prefab actors in loops and assign `(0,0,0)`; density/enable properties are stored separately | Replace the bulk render representation with stable instance records and real terrain-aware placement. Audit/migrate generated actors; preserve authored trees and gameplay ownership |
| Tree/grass layers are authoring data | [TerrainTreeLayer.cpp](../Engine/cpp/Source/Workphone/Scene/Components/Terrain/TerrainTreeLayer.cpp) and [TerrainGrassLayer.cpp](../Engine/cpp/Source/Workphone/Scene/Components/Terrain/TerrainGrassLayer.cpp) store prefab/texture/density/index; inspected setters do not establish a page render path | Retain these controls and adapt them to species/scatter assets with dirty-page regeneration and explicit units |
| Working sample tree batching already exists | [ProceduralRaceSceneBuilder.cpp](../Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp) groups generated pines into 64 m patches, merges near geometry with a shared palette material and creates far impostor geometry plus LODGroup/detail bounds | Preserve this real baseline. Extract reusable descriptors/fixtures; compare new instancing against both actor-per-tree and existing merged-patch approaches. A sample-specific merge is not general species paging |
| LOD system has useful contracts | [LODSystem.cpp](../Engine/cpp/Source/Workphone/Scene/Systems/LODSystem.cpp) snapshots data/jobs with generation checks; [LODGroup.hpp](../Engine/cpp/Include/Workphone/Scene/Components/LODGroup.hpp) supports projected-size levels, hysteresis, biases and detail bounds; cross-fade width is reserved for future implementation | Reuse selection math and metadata. Current group selection applies one level to all its renderers; it does not put individual tree instances into independent LOD draw bins |
| Three-view impostor reference exists | [MeshImposterGenerator](../Engine/cpp/Source/Workphone/Mesh/MeshImposterGenerator.cpp) CPU-rasterizes triangle colours with fixed lighting into two side views and a top view; [tests](../Tests/cpp/MeshImposterTests.cpp) check deterministic pixels, gutters, depth and LOD selection | Retain as a reference/fallback. Extend for textured leaf alpha, multi-angle views and relightable output; current bake is not a complete PBR foliage impostor pipeline |
| DX11 mesh path has no instanced foliage submission | [native DX11 renderer](../Engine/c/Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.c), `wp_renderer_dx11_draw_geometry_pntc`, ends in DrawIndexed. Inspection found no DrawIndexedInstanced path there | Add a native instanced layout/shader/buffer/draw API and wrapper integration. Existing DX12 quad draws with instance count 1 do not implement foliage instancing |
| Existing metrics can seed the baseline | [ClawHammerSystem.cpp](../Engine/cpp/Source/WPGraphics/ClawHammerSystem.cpp) reports native draws/triangles, uploads, state bindings and CPU/GPU timing | Extend to foliage/per-pass/per-view counters, successful instances and draw-split reasons. Ensure instanced triangle statistics multiply by instance count |
| Leaf import is incomplete and optional | [FoliageLoader.cpp](../Engine/cpp/Source/Workphone/Mesh/FoliageLoader.cpp) is gated by `WP_USE_NGPLANT`, default OFF in root CMake. Enabled geometry helpers build temporary arrays without installing vertex/index buffers or adding submeshes to the returned mesh; raw allocations/plant instance lifetime and UV/material handling need repair | Do not make production depend on this importer. Support normal mesh assets first; repair and test ngPlant only as an explicit optional source path |
| Procedural foliage texture is not a plant generator | [WPTextureForge.cpp](../Engine/cpp/Source/WPProcedural/WPTextureForge.cpp), `bakeFoliage`, produces leaf-like colour/height with opaque alpha | Reuse its noise/bake infrastructure, but add actual leaf silhouettes/alpha, branch/leaf geometry, UVs, wind weights, LODs and species recipes before claiming procedural plant authoring |
| Existing UI should be extended | [TerrainEditor.lua](../bin/Media/Scripts/Lua/Editor/TerrainEditor.lua) adds/edits tree and grass layers and records biome/impostor/LOD actions; legacy [wx FoliageWindow](../Tools/cpp/FBGameEditorWx/src/ui/FoliageWindow.cpp) has commented integrations | Extend active Lua/native editor conventions. Legacy wx controls are reference material, not a second production editor |
| Dedicated foliage evidence is missing | LODSystemTests, MeshImposterTests and terrain/procedural tests exist; no paging call sites/tests or complete instanced foliage GPU fixture were found in the inspected tree | Add core paging, species/scatter, real DX11 instance/LOD/alpha and draw-budget fixtures. Existing test files are not new execution evidence |

Findings are scoped to the inspected paths. The current tree was clean at the start of this review. No existing sample performance, ngPlant configuration or foliage visual output was measured during the review.

## Architecture for fewer draw calls

Use one foliage world/scene owner, named during implementation, backed by renderer-neutral data and native paging. Dense decoration is stored as compact instances rather than one scene actor, renderer and LODGroup per plant. Promote selected trees to interactive actors only when gameplay requires it, with a stable link back to their instance IDs.

| Responsibility | Data and policy |
|---|---|
| Species asset | Persistent UUID; mesh LODs/submesh ranges; material family/texture array or atlas references; impostor metadata; bounds/pivot; wind profile; shadow/density/collision/quality settings; compiler version and dependencies |
| Instance | Stable ID/species ID; cell-local position, rotation and scale; deterministic colour/wind/variation seed; edit/removal flags. No per-instance cloned material |
| Spatial storage | Sparse signed cell coordinates and retained CPU instance blocks. Start with measured 32/64/128 m page candidates; render/cull clusters may be smaller. Terrain tiles and foliage pages need not have identical sizes |
| Residency | Shared streaming scheduler owns CPU/GPU/upload budgets and priorities. Native paging supplies range/preload/reload behavior; explicit pending/ready/failed states guard publication |
| Visibility/LOD | Page/cluster coarse culling, then instance culling/LOD where beneficial. Per-view immutable visible index lists and compatible draw bins; scene data is not globally toggled by one camera |
| Submission | Shared immutable mesh/index buffers plus reusable instance buffers. Batch key includes pass, mesh/LOD/submesh, material/shader variant, raster/depth/alpha state and resource compatibility |
| Temporal state | Stable IDs and current/previous transforms/wind phase; transition seed. Bounds include wind and instance scale. History resets on teleport/reload/cut and LOD transitions have a tested policy |
| Editing/gameplay | Sparse manual overrides and removal masks on generated data; brush undo/dirtied cells; proximity-limited colliders/interactive objects independent of visual LOD |

### Batching rules

1. **Hardware instancing is the repeated-tree baseline.** Share geometry/materials per species and LOD; upload visible transforms/variation data in bounded buffers and issue one indexed instanced draw per compatible group/capacity chunk. Add independent per-instance GPU output tests before promoting the path.
2. **Pages organize loading and culling, not mandatory draw boundaries.** After visibility/LOD, aggregate compatible instances across visible cells within upload/sorting limits. Dirty pages update retained data; they do not force rebaking the entire forest. Benchmark this against per-page merged meshes to quantify the memory/culling/submission tradeoff.
3. **Keep material count controlled.** Reuse canonical material instances; use per-instance tint/variation, shared texture arrays or padded atlases where device/resource/format compatibility permits. Preserve genuine alpha/wind/shader differences; count and explain every state-driven split. One tree with trunk and leaf materials normally contributes two groups, not magically one draw.
4. **Static merging is a bounded alternative.** Retain the sample's merged patches for suitable static unique geometry or a backend fallback, preserving correct bounds/normals/indices. Measure duplicated geometry memory and culling losses. Do not merge the whole world into one mesh merely to improve its draw counter.
5. **LOD alone does not guarantee fewer draws.** Replacing each actor's tree mesh with an individual billboard can keep one draw per tree. Far impostors must also share atlas/material resources and batch; LOD reduces geometry/overdraw while instancing/grouping reduces calls.
6. **Count all real passes.** Colour, depth, directional/local shadows, reflections and additional views each submit separately. Cross-fades can draw two representations; count that cost explicitly. Compare matching views, shadow settings, visible coverage and quality against the baseline.

For an instanced path, predicted draws for one pass/view are:

`D = sum over compatible groups k of ceil(visible instances in k / instance capacity for k)`

Groups include mesh LOD and submesh/material state. If a measured implementation intentionally retains page boundaries, include page in the key and report its additional calls. Atlas allocation, upload segmentation, transitions and resource limits must appear in the same accounting. Log empty/culled groups separately; they must not issue draws.

```mermaid
flowchart LR
    A[Species assets and scatter recipes] --> B[Stable cell instance data]
    B --> C[Paging and bounded residency]
    C --> D[Per-view cell and instance culling]
    D --> E[Tree LOD and grass density selection]
    E --> F[Group by mesh, material, LOD and pass]
    F --> G[Reusable instance buffers]
    G --> H[DX11 indexed instanced draws]
    H --> I[Per-pass draw, instance and timing evidence]
```

## LOD and visual feature policy

| Vegetation | Near | Middle | Far | Required controls |
|---|---|---|---|---|
| Trees | Detailed shared trunk/leaf meshes, full wind | Simplified branches/leaves, reduced wind complexity | Batched multi-view impostors or authored simplified representation, then species-specific fade/cull | Projected size/error, per-species bias, hysteresis, transition width, maximum range and shadow LOD |
| Bushes | Instanced authored/generated meshes | Simplified clumps/cards | Batched cards/impostors where useful, shorter maximum range | Stable density selection, alpha coverage and screen-space silhouette error |
| Grass | Instanced clumps/blades/cards in small cull clusters | Simpler clumps and deterministic density reduction | Fade/cull into terrain material coverage; no default individual impostor per blade | Density/range/quality, wind, slope/exclusion masks, optional restricted shadow range |
| Rocks/ground cover | Instanced opaque meshes | Simplified meshes | Small shared proxies or cull | Material compatibility, collision policy and projected size |

Use screen-relative size/error for tree/cluster LOD and explicit per-species distance caps for residency/quality. Reuse LODSystem selection/hysteresis math, with independent visible-instance bins rather than one worst-case tree forcing an entire large page to high detail. Validate camera FOV, orthographic views, resolution, scaled instances and forced LOD. Culling bounds and LOD detail bounds are separate.

Implement bounded dithered cross-fade or a documented hard-switch fallback; the current LODGroup fade field is not an implemented transition. Keep stable seed-based coverage in colour/depth/shadow passes, and evaluate transitions with and without temporal AA. Reduce grass density using a deterministic subset of the same placement population so motion does not reshuffle survivors.

Foliage materials need depth-writing alpha test with coverage-preserving mips, a defined double-sided normal policy, normal/roughness maps and optional leaf transmission. Keep wind deformation/pivot/weights consistent across colour/depth/shadow and future velocity passes. Near trees can cast mesh shadows; far trees use a documented shadow proxy/range policy. Grass shadow range is separately budgeted. Do not disable shadows or thin visible coverage only for the optimized benchmark.

The existing baked-colour impostor is a useful bounded fallback. General species baking must capture source textures and leaf alpha, retain gutters/mips, and either produce relightable normals/depth/material channels or document fixed lighting and validate transitions under the supported lighting tier. Bake offline through ResourceSystem; avoid synchronous atlas creation on a forest streaming frame.

## Procedural foliage tools

Separate **plant generation** (species geometry/material/LOD assets) from **placement generation** (instances of species across terrain). Both use versioned recipes, the existing catalog/compiler pipeline and native worker jobs.

- **Placement rules:** stable seed; species mixture/weights; density in instances per square metre; slope/height/terrain-layer/moisture masks; road/water/building exclusions; minimum spacing and cluster/Poisson-style distribution; yaw/scale variation; normal alignment and ground offset. Sample the corrected authoritative terrain contract from TERR-01/02.
- **Stable cell generation:** hash world cell/candidate IDs and recipe version; use signed coordinates and border ownership, plus neighbor halos for spacing/exclusions. Generation order and worker count must not change outputs or duplicate borders. A density change selects a stable subset where applicable.
- **Manual authoring:** paint/erase/reseed, density/species masks, rectangle/lasso selection, move/rotate/scale, replace species, exclusion splines/volumes, regenerate selected cells, lock hand edits, stroke undo/redo and save/reopen. Generated results and manual deltas retain stable identity across regeneration.
- **Plant/species authoring:** native branch/trunk and leaf-card/clump generators with leaf atlas references, bounds, UVs, normals/tangents, pivots and wind weights; LOD simplification, cutout-material atlas and impostor bake. Imported trees remain first-class species; optional ngPlant support cannot block ordinary foliage assets.
- **Authoring feedback:** near/mid/far preview, forced LOD, visible/resident counts, material/group splits, actual draws per pass, triangles, overdraw estimate, atlas/instance memory, generation/upload latency and shadow cost. Warn when unique materials or incompatible atlases fragment groups. Changes preview through the runtime path.
- **Integration:** dirtied terrain edits resample affected placements; roads/water update exclusions; collisions activate near gameplay demand and survive visual LOD/page transitions. Extend TerrainEditor.lua and relevant existing procedural editors with thin native bindings per [editor conventions](../Tools/cpp/Editor/docs/CONVENTIONS.md).

## Implementation work packages

IDs reference the parent plan or the terrain/procedural companion where appropriate. R1 rows can have a bounded initial tier; R2 extensions require their own evidence.

| ID / tier | Concrete deliverable | Dependencies | Acceptance evidence |
|---|---|---|---|
| FOL-00 / R1 first | Baseline fixtures/captures and per-pass/per-view foliage counters; compare actors, existing merged pine patches and new paths; record draw-split reasons | BASE-04/05; existing DX11 statistics | Reproducible seed/camera/quality/content; matching native call counts; colour/shadow/reflection totals and CPU/GPU/upload/memory results separated |
| FOL-01 / R1 | Species/instance/cell formats, stable IDs, materials, bounds/pivots, LOD metadata and cooked dependencies; adapters for existing terrain layers and sample trees | DB-07/08, PROC-01/03; normal mesh import | Cook/load/rename/reload; no cloned resource per tree; invalid bounds/indices rejected; old assets migrate or fail clearly |
| FOL-02 / R1 bounded, R2 scale | Contract-test retained native paging; internal scene owner/page callbacks; explicit pending/ready/failed and source generations; worker preparation/render publication | CORE-01/02/05, FOL-01; shared STREAM-01 | Negative cells, boundaries, reload/preload, range/transition selection, failed build and unload callbacks; delayed candidate cannot publish into reused page; native C90 builds |
| FOL-03 / R1 essential | DX11 indexed instanced geometry/material path, shared mesh buffers, reusable bounded instance uploads, compatibility grouping across cells and counted capacity splits | FOL-00/01, GEO-01, MAT-01/TEX-01 | Distinct transforms/colours, zero/one/many instances, reload/device lifetime/state restoration; actual DrawIndexedInstanced counts follow formula and beat actor baseline |
| FOL-04 / R1 essential | Per-instance/small-cluster tree/bush LOD, hysteresis, independent per-view selection, forced levels, shadow LOD and bounded transition policy | FOL-01/03, LODSystem math | Near/mid/far sequences reduce triangles; calls remain grouped; no whole-page over-detail; FOV/scale/boundary/two-view tests; transition and shadow cost recorded |
| FOL-05 / R1 far tier, R2 bake fidelity | Shared batched impostor draw path; reuse CPU reference; offline textured/multi-view/relightable atlas baking and mip/alpha metadata | FOL-01/03/04, PROC-03/05/06, TEX-01 | Far tree count can grow within buffer capacity without one draw per tree; silhouettes/alpha survive mips; camera rotation/lighting/mesh transition tests; atlas dependencies cook/reload |
| FOL-06 / R1 | Dedicated grass/clump pages and instance draws, terrain sampling, stable density LOD, fade, exclusion masks, near wind and shadow policy | FOL-02/03/04, TERR-01/02 | No actor or draw per blade; density subsets stable; terrain/road borders correct; grass overdraw/range/GPU cost measured |
| FOL-07 / R1 core, R2 richer | Cutout/two-sided material rules, alpha mips, wind/pivot weights, conservative bounds, matching depth/shadows; later transmission/weather/velocity | MAT-01/TEX-01/SHADOW-01, FOL-03/04 | Textured leaves/grass show correct coverage/normals; wind remains inside bounds and shadow matches; state does not leak to following mesh/UI draws |
| FOL-08 / R1 basic, R2 biomes | Deterministic placement recipes, ground/slope/density masks, border ownership, exclusions and regeneration/manual-delta identity | FOL-01/02, TERR-01/02, PROC-01/02/03 | Same seed/cell population across request order and worker count; no seam duplicates; non-origin terrain placement; density/enable controls visibly affect results |
| FOL-09 / R1 basic, R2 complete | Active Lua foliage/layer controls, brush paint/erase, undo/save, runtime preview and batch/LOD diagnostics; later selection/biome/exclusion tools | FOL-08, TERR-04 command services, TOOL-01/02 | Paint→undo→redo→save→restart reproduces instances; error/cancellation clear; forced LOD and real draw counters visible; owned generated trees do not accumulate |
| FOL-10 / R2 | Plant geometry/leaf/grass clump generation, wind attributes, authored LOD reduction and species/impostor bake; optional ngPlant repair behind capability checks | FOL-01/05/07, PROC-04/05/06 | Recipe yields nonempty textured plant with valid topology/UVs/bounds and independent LODs; cook/reimport and seed tests; ngPlant OFF/ON behavior explicit |
| FOL-11 / R1 collision policy, R2 interaction | Nearby collision proxies and interaction promotion/demotion; stable removal masks, selective wind/bend response and gameplay events | FOL-01/02/08, TERR-02 physics bridge | Visual LOD does not remove needed colliders; selected tree promotion has no duplicate render/collision; edits/destruction persist across page eviction |
| FOL-12 / R1 bounded recovery, R2 scale | Shared streaming/VRAM/upload budgets, multi-camera residency union, cache eviction, teleport behavior, project/scene/device recovery | FOL-02/03, CORE-03/04, STREAM-01; TERR-06 coordination | Cold/warm flythrough, rapid teleport, two views/reflections, failed decode/upload, device reconstruction and unload; last-good pages retained or documented bounded fallback |
| FOL-13 / both | Focused paging/scatter/instance/LOD/draw-budget targets, visual sequences, tools/soak/packaged scene and support documentation | Continuous; gate depends on relevant FOL rows | GF0/GF1 artifacts record source/executable/fixture hashes, hardware/driver, pass counts, images, timing and actual unavailable outcomes |

## Delivery sequence and estimate

| Batch | Reviewable scope | Exit condition |
|---|---|---|
| F-A: measure and prove instancing | FOL-00, minimal FOL-01, FOL-03; use a fixed imported/generated tree with two material sections | Real DX11 output preserves distinct instances; fixed-LOD draw counts meet formula and baseline reduction; compare existing merged patches before adopting a default |
| F-B: useful tree LOD/paging | FOL-02/04/05/07; authored mesh LODs and documented far representation; page generation/publication hardening | Trees retain silhouette through near/mid/far and shadows; per-pass calls stay bounded by groups/capacity; page moves/reloads do not stall or resurrect stale data |
| F-C: grass and foliage authoring | FOL-06/08/09/11/12 initial tiers; deterministic masks, basic brush/persistence, bounded residency/collision | Painted/scattered forest and grass run from cooked content outside editor, undo/reload stable, GF0 performance/visual/recovery evidence complete |
| F-D: comprehensive tools and scale | FOL-05/07–12 richer tiers, plant generator, biome/exclusion tools and large streaming scenes | Textured/relightable impostors, rich plant/placement recipes and multi-view streaming pass GF1, with renewed whole-renderer certification |

F-A starts beside the shared cooked-material/publication work; synthetic CPU instance data can test draw mechanics before full terrain streaming. F-B must not wait for water or TAA. F-C requires reliable terrain sampling/edit commands; F-D uses shared streaming and wider procedural asset tools.

Add **M6E / GF0 (R1 foliage core)** and **M6F / GF1 (R2 foliage tools/scale)** to the parent plan. Provisional allowances are 5–8 and 4–7 engineer-weeks respectively, including foliage-specific tests/docs, excluding shared device/catalog/general instancing/streaming infrastructure and programme certification. These 9–15 weeks are unmeasured estimates, not dates. They extract and expand the old TERR-07/M6B vegetation allowance: re-estimate that remaining work and remove overlap with GEO-01/PROC/TERR packages before rolling up a programme total.

Assign a foliage/rendering owner for GF0/GF1, a procedural/tools owner for scatter/plant generation and Lua tools, and explicit asset/physics/QA/technical-art tasks. Each PR reports changed behavior, relevant counters/captures and evidence limits. This request authorizes the plan; runtime implementation remains a subsequent task.

## Acceptance: batch count, draw calls and frame cost

Measure **actual successful native draw submissions** and record submitted instances, not just logical render items or page batches. Count CPU preparation and upload time separately from GPU time; triangles and alpha overdraw remain essential alongside calls.

| Fixture | Locked comparison | Proposed exit condition |
|---|---|---|
| Controlled instancing | 10,000 visible tree instances, one species, one forced mesh LOD, two material sections, one colour view; identical transforms/coverage, optional passes disabled equally in both paths | With 4,096 instances per draw group, no more than `2 × ceil(10000/4096) = 6` foliage colour draws. Actor baseline normally needs 20,000 section draws; require measured reduction ≥90%, matching images and no per-instance mesh/material allocation |
| Existing merged-patch baseline | Same population/cameras/materials in current-style CPU merges versus instancing; both use matching visibility/LOD/shadows | Publish calls, submission time, duplicated geometry/instance memory, rebuild/upload cost and culling loss. Choose default using the combined result; instancing need not beat a single ideal static merge on draw count alone |
| Mixed species/materials | Fixed species list, submesh counts, three mesh/far tiers, fixed cells and visible instances; shadows enabled with locked quality | Per-pass predicted groups/capacity splits match actual draws; record material/LOD/atlas/page/transition split reasons. Scaling population within existing capacity does not create one draw per tree |
| LOD sequence | Fixed camera path with pinned FOV/resolution, forced-LOD reference captures and automatic selection | Near/mid/far representation and triangle reductions correct; hysteresis stable; transition overlap bounded; grouped far impostors preserve the call benefit |
| Grass | Fixed planted area/clump geometry and density; matching coverage for renderer comparison, then separately test density LOD | Calls follow compatible clump groups/capacity, not blade count; density selection stable; p95 GPU/overdraw measured at grazing angles and under wind |
| Dense forest/world | Versioned mixed forest/grass scene, declared species/materials/density/visible area and two-view/shadow/reflection variants | Whole-frame budgets calibrated against parent 1080p targets; record p50/p95/p99 CPU/GPU, pass calls, triangles, alpha coverage, VRAM, CPU/collision residency and upload spikes |

The six-draw example proves controlled batching, not an entire forest-frame budget. Shadow maps, depth prepass, transitions, reflection views and additional cameras add their own accounted calls. Freeze asset/state/capacity assumptions in the fixture manifest; update expectations only through review, not by reducing visible content or disabling passes in one side of the comparison.

**GF0 release checklist:** F-A/B/C supported tiers complete; actual batch/draw reduction measured; hardware instance/LOD/alpha/shadow tests pass; tree/grass cooked scene works; basic scatter/brush/undo/save/reopen are real; page edits/unload/device failures are bounded; no resource accumulation or unresolved collision disagreement; capabilities and unsupported operations accurately reported. No mandatory GPU/visual/performance check may be counted as passing when unavailable.

**GF1 release checklist:** GF0 plus advertised textured impostor/plant/biome/interaction/streaming tiers; multi-view shadow/reflection sequences; long flythrough/teleport; 1,000 regeneration/page/load/unload cycles and parent eight-hour soak; installed package contains all species/atlas/recipe/instance dependencies. Re-certify the whole renderer's R2 gate.

Use existing MeshImposterTests and LODSystemTests, extending their references. Proposed additional targets, not yet implemented: native paging callback contracts, foliage species/scatter contracts, DX11 instanced foliage output/draw budgets, foliage scene/tool persistence and forest performance/capture runner. Build native C90/C++17 and Debug/RelWithDebInfo configurations; retain headless/GPU/interactive/performance/packaging evidence separately.

## Decisions for the first implementation increment

1. Lock the controlled tree fixture, current merged-patch comparison and per-pass counters before optimization.
2. Select instance layout/capacity, material grouping and the first measured page/cluster sizes. Define normal transforms, negative/nonuniform scale support and fallback/error behavior.
3. Decide how the existing C pager reports async readiness/failure and how one scene owns multi-view residency without duplicate loading or global visibility toggles.
4. Freeze species/instance/recipe schemas, terrain sampling units, IDs and authored-actor migration policy. Separate plant generation from placement.
5. Choose initial authored mesh LODs/far representation and alpha/wind/shadow policy; implement the fuller impostor baker only after the grouped renderer is proven.
6. Name owners and calibrate scene-specific batch/frame/memory/latency budgets from F-A evidence. Keep batching and LOD in R1 even if richer foliage tools slip.
