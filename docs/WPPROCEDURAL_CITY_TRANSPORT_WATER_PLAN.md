# Procedural city transport water and roundabout design

Status, 25 October 2026: native road/roundabout authoring now connects to generated grid/L-system cities through **City Terrain Roads**, one-time canonical v2 promotion, persistent terrain/source anchors and terrain-supported parcels. Regeneration preserves road edits and bound terrain while rebuilding derived city content. Compilation, tests and captures for this integration are pending; earlier road-only checks do not establish integrated or recorded Editor acceptance. Rivers, bridges, train tracks, cut/fill, project source assets, source-free reconstruction, hardware performance and AAA visual acceptance remain open. The wider transport design should extend the same terrain-bound source, with hydrology and rail reservations constraining roads and parcels before geometry is produced.

This extends the [city authoring plan](WPPROCEDURAL_CITY_AUTHORING_PLAN.md), P3/P8/P9 of the [procedural production plan](WPPROCEDURAL_PRODUCTION_PLAN.md), the [terrain workflow](TERRAIN_WORKFLOW.md), and WATER-01 through WATER-06 of the [graphics production plan](WPGRAPHICS_PRODUCTION_PLAN.md). The wider water/rail/bridge records, tools and acceptance gates are proposed work; implemented roundabout boundaries are identified separately.

## Existing foundations and current boundary

| Existing source | Reuse and boundary |
|---|---|
| [CityRoadSplineSource.hpp](../Engine/cpp/Include/Workphone/Interface/Procedural/CityRoadSplineSource.hpp) and [implementation](../Engine/cpp/Source/Workphone/Scene/Components/CityRoadSplineSource.cpp) | Strict v1 compatibility and actual v2 minimal `roundabouts` records, with GUID road references, relative Bezier handles, bounded sampling and Fixed/Drape profiles. `CityRoadAuthoring` persists one inline scene source and prepares native render/mesh collision. `Bridge` remains explicitly rejected. |
| [CityRoadRoundabout.hpp](../Engine/cpp/Include/Workphone/Interface/Procedural/CityRoadRoundabout.hpp), [terrain mesh helper](../Engine/cpp/Source/Workphone/Scene/Components/CityRoadTerrainMesh.cpp) and [CityRoadWindow.cpp](../Tools/cpp/Editor/src/ui/CityRoadWindow.cpp) | Implemented annulus with explicit island/outer radii, trimmed approaches, raised island/kerbs and derived splitter refuges, directed movements, conservative terrain coverage and shared endpoint checks. Editor placement creates four approaches; the CPU tier supports three to six. Current integration validation and live Editor acceptance remain open. |
| [OpenCityRoadNetwork.hpp](../Engine/cpp/Include/Workphone/Interface/Procedural/OpenCityRoadNetwork.hpp) and [OpenCityLayout.hpp](../Engine/cpp/Include/Workphone/Interface/Procedural/OpenCityLayout.hpp) | Seeded planar road output and parcel clearance are useful derived inputs. Generated node indices are not persistent authoring IDs. Rail and river crossings must not be flattened into ordinary road junctions. |
| [IRoadSystem.hpp](../Engine/cpp/Include/Workphone/Interface/Procedural/IRoadSystem.hpp), [RoadSystemTypes.hpp](../Engine/cpp/Include/Workphone/Interface/Procedural/RoadSystemTypes.hpp) and [WPRoadSystem.cpp](../Engine/cpp/Source/WPProcedural/WPRoadSystem.cpp) | `buildRoundabout(IntersectionSpec)` currently emits a fixed 3 m ring and centre island. It uses `radius` as the ring centreline radius although the spec comment describes an outer extent, and ignores `approaches`. It does not solve entry connections, lane topology, terrain or collision, and is not integrated into city generation. |
| [TerrainData.hpp](../Engine/cpp/Include/Workphone/Graphics/TerrainData.hpp) | Immutable heights, holes, placement validation and triangle-consistent height/ray queries. Outside-footprint and hole queries fail; no zero-height fallback. |
| [TerrainEditing.hpp](../Engine/cpp/Include/Workphone/Scene/Components/Terrain/TerrainEditing.hpp) and [implementation](../Engine/cpp/Source/Workphone/Scene/Components/Terrain/TerrainEditing.cpp) | `TerrainGradeRecipe::river` creates a parabolic channel profile below its centreline elevation. Existing grading caps work at 16 million sample-segments and 65,536 changed samples. It does not create water, bridges or a complete drainage model. |
| [TerrainIntegration.hpp](../Engine/cpp/Include/Workphone/Scene/Components/Terrain/TerrainIntegration.hpp) | Ground populations, path support and shoreline segments retain their terrain snapshot. `buildTerrainShorelineInput` uses one water level; a graded river requires reach/profile-aware intersection work. These outputs are not a water renderer. |
| [Terrain.hpp](../Engine/cpp/Include/Workphone/Graphics/Terrain.hpp) and [TerrainSystem.hpp](../Engine/cpp/Include/Workphone/Scene/Components/Terrain/TerrainSystem.hpp) | Expected-revision updates and publication gates coordinate prepared render/physics terrain consumers. Extend this existing commit boundary to dependent city candidates. Post-publication notifications alone cannot make a multi-system edit atomic. |
| [IGraphicsWater.hpp](../Engine/cpp/Include/Workphone/Interface/Graphics/IGraphicsWater.hpp) | An interface for scene/camera/viewport/position exists. The inspected Claw implementation has no concrete river/water rendering object. Depth, flow, water material, reflection and refraction publication still require implementation. |
| [WPPhysicsMeshShape3.cpp](../Engine/cpp/Source/WPPhysics/WPPhysicsMeshShape3.cpp) and [native collision API](../Engine/c/Include/WorkphonePhysics/workphone_physics_collisionshape.h) | Existing mesh-shape publication can consume prepared deck/track-bed collision. Validate actual native ray/contact behaviour and backing-data lifetime; a visible mesh is not a collider. |

The generated grid/L-system [city scene builder](../Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp) can promote its initial roads and eligible roundabouts into the existing strict v2 authoring source. **City Terrain Roads** uses the selected terrain and checks whole-parcel cell coverage, relief and edited-road exclusions before deriving pads/buildings. Source and terrain remain scene-owned anchors while car, buildings, pads, dressing and foliage regenerate. Ordinary road junctions are geometric contacts rather than solved lane topology. There is no runtime river, supported bridge or railway generation in this city path; train vehicles, signalling, hydraulics and flood simulation remain unimplemented.

Enable **City Terrain Roads**, launch the sample with `--city-terrain-roads`, or use capture switch `-CityTerrainRoads`. Refresh the hierarchy if needed, select **Editable generated city roads**, then open **Window > City Roads**. **Bind Selected Terrain** binds/rebinds that canonical source. **Draw Road** accepts terrain clicks and dragged Bezier handles; Enter/**Commit Road** commits and Escape cancels. **Move Knot**, **In Handle**, **Out Handle**, **Apply Road Fields**, **Undo**/**Redo** and **Save Scene** operate on the same source. **Place Roundabout (4 approaches)** uses a conservative local grade-plane fit; unsuitable terrain fails visibly rather than being flattened. Seed/size changes preserve promoted roads; controlled regrowth/overrides require future v3 work. Once terrain-city anchors are active, returning that component to flat-city mode is explicitly unavailable; use a fresh scene. See [the practical workflow](WPPROCEDURAL_CITY_AUTHORING_PLAN.md#current-practical-workflow).

## One source with stable transport identity

Extend the authoring plan's proposed **version 3** schema with typed `rivers`, `railTracks`, `railNodes`, `crossings` and `bridges` records. Actual v2 is already reserved for the minimal road/roundabout format: strict source metadata plus `terrain`, `roads` and `roundabouts`; it rejects these future records and unknown fields. Current persistence is one canonical source inline in a scene-owned `CityRoadAuthoring`, its terrain-component binding, and the vehicle component's source-anchor UUID plus **City Terrain Roads** flag. Project asset catalog/importer integration and a project-source-asset UUID reference remain future work and must not introduce a second editable copy.

Current source controls use world metres with +Y up and an identity authoring-actor transform. City-local source placement is part of the future v3 asset design. Retain terrain identity/placement and evaluate gauge, widths, grades, water levels and clearances in world metres, accounting for terrain scale. Water follows world gravity. Unsupported transforms must fail; a nonuniform transform must not silently stretch rail gauge.

| Proposed record | Required source data |
|---|---|
| River | GUID, source/receiver references, ordered knot GUIDs with relative handles, per-station water elevation, wet width, bed depth, bank width/slope, flood/reservation margin, flow appearance, locked/generated provenance. |
| Rail track | GUID, knot GUIDs/handles, explicit horizontal and vertical profile, gauge/profile asset UUID, sleeper spacing, ballast/shoulder section, maximum grade/radius/cant/cant-rate, station/yard links, locks and generation lineage. |
| Rail node | GUID, typed ports and explicit permitted connections; switch/diamond/terminal kind, selected route and conflict groups. A geometric crossing does not automatically connect tracks. |
| Crossing | GUID, carrier transport kind and GUID, crossed river/road/rail GUID, source-curve intervals, crossing kind, approval/override state and clearance policy. Accepted crossings retain identity when their sampled points move. |
| Bridge | GUID, crossing GUID, carrying road/rail GUID, crossed feature GUID, deck profile/width/thickness, abutments, span boundaries, optional piers, railings/barriers, foundation policy, approach constraints and referenced style assets. |

A spline segment is referenced by its stable endpoint knot IDs and normalized curve interval, not a tessellation index. Automatic candidates may be ephemeral. Accepting one creates a durable crossing/bridge GUID; ambiguous reassociation after a split or multiple crossings requires review instead of moving a bridge to a different river silently. Duplicate, dangling and cross-kind IDs fail validation.

Use one shared bounded curve-sampling core for roads, rivers and rail. Keep transport-specific frames, profiles and topology rules separate. Reuse `SourceBudgetText`, `SourceBudgetVector`, existing project/cook identity and generation tokens. Runtime terrain revision/placement serials are concurrency guards, not durable IDs to replay after reopening.

## Generation order and terrain ownership

1. Capture one immutable source revision and terrain snapshot/placement. Validate all locked curves, receivers, profiles and references before generation.
2. Generate or edit the river network and its channel/bank/flood reservation. Prepare terrain changes, water profiles and supported-bank masks without publishing them.
3. Reserve rail alignments, stations, yards, loading envelopes and grade/curve transitions against that hydrology and terrain candidate.
4. Grow or fit roads around water and rail reservations. Proposed crossings become explicit candidates; ordinary planar snapping must not connect different transport levels.
5. Solve accepted crossings, bridges and approaches. Regenerate affected channel/support terrain, then revalidate the river profile and all transport grades against the same final terrain candidate.
6. Derive parcels, pads, vegetation, street furniture and navigation from the final exclusions. Publish only after all geometry, collision and asset consumers are prepared.

Generated rivers should follow a terrain-supported downstream path or an authored profile. A decorative meander through a hill must fail or request a channel cut, rather than remain suspended above it. Locks and deletion tombstones apply before new growth; changing an unrelated road must not reshuffle a river or railway.

Retain actual channel bed terrain beneath water. Do not use holes to hide an uncarved channel or retain the flat city collision plane beneath a river. Grade the supported banks and approaches from the prepared terrain, with explicit cut-only/fill permissions; the present grading recipe needs an extension before it can enforce all of these policies.

## River profiles and water surfaces

Store the centreline water elevation at source stations and interpolate a bounded downstream profile. For the first steady-flow tier, elevation may remain level or decrease downstream; uphill reaches, incompatible confluence levels and receiver cycles fail. Waterfalls, tides, pumped channels and closed drainage loops need separate explicit feature policies.

Wet width, bank width, bed depth and reserve width are distinct. Channel preparation derives the bed below the water surface and blends banks into supported terrain. Validate positive depth, finite widths, inner-offset reversals, self-intersections, terrain coverage and tributary seams over the complete swept footprint. Sampling only the centreline or knot endpoints is insufficient.

Generate a watertight ribbon per supported reach with arc-length UVs and tangent-based flow direction. Width/elevation transitions and confluence joins must not crack or create overlapping transparent layers. Derive bank/intersection foam and wetness from the channel and water profile; the constant-level shoreline helper remains useful for level reaches and ponds.

Implement the missing Claw water object through the existing water/graphics architecture and WATER-01 through WATER-06. A prepared immutable water surface carries geometry, bounds, flow/material inputs and source revision. The native frame path owns depth/color inputs, water pass order and final tone mapping; the sample must not run its own post-processing sequence.

Begin with depth-aware shallow/deep colour, absorption approximation, Fresnel and normal flow. Use probe/IBL fallback before optional planar reflections. Refraction samples a matching-view pre-water colour/depth copy; reflection views suppress recursive water drawing. Bounds include supported wave displacement, and multiple surfaces share an explicit reflection budget. A blue opaque ribbon alone does not pass water acceptance.

## Rail geometry and operating envelopes

Rail is a separate swept cross-section: two profiled rails at explicit gauge, sleepers at arc-length spacing, ballast, shoulders and drainage margins. It must not reuse a 12 m road quad or a centreline-only collider. Use continuous transported frames through inflections, with cant applied to the track cross-section and corresponding clearance envelope.

Each rail profile supplies independent minimum horizontal radius, maximum longitudinal grade, vertical curvature, cant and cant-transition rate. Proposed first demonstrator limits are a 150 m minimum radius, 2% maximum grade and 0.10 m maximum rail-height difference; these are game-content targets, not railway standards or vehicle-dynamics certification. Yards/trams require explicitly different profiles rather than silent relaxation of a mainline profile.

Fit a continuous vertical alignment through cut/fill and transition curves. Do not drape each rail sample independently onto bumps. Check gauge, rail/sleeper separation, grade, cant rate and the complete train loading envelope along every segment; reject a requested alignment when its transitions cannot fit within the reserved corridor.

Switches need explicit compatible ports, diverging routes, frog/blade geometry and route connectivity. Crossing tracks can remain disconnected through a diamond asset; nearby endpoints do not become a switch automatically. Generated yards use stable child lineage, bounded track counts and an explicit throat topology.

Stations reserve platform geometry, clearance from the loading envelope, accessible approaches and track stopping intervals before parcels are generated. Platforms need a supported pad; station tracks normally select a zero-cant policy. Yard limits, buffers and service access are authored records, not arbitrary street props.

Define route reservations over stable track-block and switch IDs before adding trains: reserve connected blocks and incompatible junction movements atomically, release them on completion/unload, and reject stale routes after topology edits. This is a future operating layer. Static track generation does not establish functioning trains, signals, braking or safe route occupancy.

## Crossings bridges and approaches

The first water-crossing tier permits supported bridges only. At-grade road/water overlap is rejected. A future ford requires an explicit crossing record, supported channel bed, permitted depth/flow/vehicle policy and surface response; it cannot be selected as an automatic fallback when bridge fitting fails.

Road/rail level crossings likewise require an explicit record. Solve one compatible crossing elevation, rail/flange geometry, road transition and clearance reservation; keep the two transport graphs distinct. Grade separation uses a bridge record with its carrying transport and crossed road/rail envelope. Future gates/signals belong to the operating tier and must not be advertised from static crossing art.

For each water bridge, intersect full transport and bank/flood corridors, then select supported abutment regions outside the active channel. A near-parallel crossing may require relocation or a longer span. An initial 45-degree minimum crossing angle and 50 m clear-span limit are proposed content limits; larger/skewed bridges require an explicit supported asset/profile.

The lowest deck structure must exceed the design high-water profile plus freeboard and allowed surface displacement over the entire crossing. The road/rail running surface sits above that underside by the actual deck thickness. A proposed 2 m freeboard is an initial art/gameplay parameter; changing it invalidates the fitted deck and approaches.

Approach length must satisfy the carrier's grade and vertical-curve limits. For example, gaining 8 m requires at least 100 m at 8% road grade or 400 m at 2% rail grade, before vertical transitions. If terrain, locked streets or parcel reservations cannot accommodate that length, report an invalid crossing; do not tilt a short ramp steeply or leave a floating road.

Bridge candidates contain deck surface and underside, edge barriers/railings, abutments, span joints, optional piers and foundations. Rail bridges use rail-compatible deck/ballast containment and preserve gauge/cant/profile constraints. Road bridge sidewalks continue across the deck with a documented barrier and collision policy.

A pier requires supported foundation terrain and an explicit permission to occupy the water corridor. First-tier automatic bridges prefer a clear span. General scour, structural loading, flood hydraulics and destructive simulation are outside this visual/gameplay tier.

Use separately prepared deck/approach collision, barriers and foundation/support terrain. Collision follows the actual running surface and leaves open space below the bridge. Decorative girders do not create a solid bounding box across the channel, and channel terrain must not rise into the underside. Validate bridge ends against road/rail mesh, collider and terrain seams on both banks.

Surface queries must identify the supported layer in 3D. The current city uses a 2D distance-to-road query; that cannot distinguish a bridge deck from the riverbank, road or railway below it. Resolve vehicle contact, grip and effects from the actual hit shape/surface GUID and elevation, and keep route connectors attached to their carrying layer. Include simultaneous above/below-bridge probes so a valid deck does not make the whole channel driveable.

## Roundabouts and junction authoring

Actual source v2 implements a dedicated `roundabouts` array with junction GUID, centre, explicit radii/island profile, circulation, grade/sampling bounds and ports referencing existing road GUIDs. The CPU solver generates an annulus, raised central island/kerbs, derived splitter refuges and directed entry/exit routes; native terrain preparation validates complete support and exact shared approach cross-sections. Initial graph promotion selects real eligible roundabouts under a bounded count, while Editor placement creates four connected approaches. Current integration tests/captures, live Editor acceptance, design-vehicle sweeps and the broader policy fields below remain open. The legacy `IRoadSystem::buildRoundabout` helper is not the authoring solver.

| Future v3 junction/profile extension | Required meaning |
|---|---|
| Identity and kind | Stable junction GUID, `Roundabout` kind, source revision, generated lineage/locks and referenced terrain identity. Promotion from a generated junction preserves its GUID. |
| Approach ports | Stable port GUIDs referencing road GUID, endpoint knot GUID or stable spline interval, incoming/outgoing lanes, width and tangent. Split/trim edits preserve references; moving a sampled graph index cannot reattach a port. |
| Ring and island | Explicit outer carriageway radius, raised island radius, lane count/width, optional permitted truck apron, kerb/sidewalk widths and style asset UUIDs. Validate positive usable annulus width and enough space for every entry. No ambiguous legacy `radius` conversion. |
| Movement policy | Driving side, permitted circulation, entry priority, lane connectors, crossing setbacks and signage/marking profile UUID. Validate circulation against the driving-side profile. A country/profile selects explicit defaults with per-junction overrides; European cities do not share one homogeneous driving-side or signage rule. |
| Ground/support | Ring elevation/profile, crossfall/drainage policy, approach grade/vertical-transition limits, cut/fill permission and terrain revision/placement guard. |

The implemented tier supports a single 3..7 m circulating lane, three to six CPU ports, at most eight source roundabouts and at most 256 ring samples including port boundaries, under SourceBudget admission. The generated sample selects at most four roundabouts; Editor placement selects four ports. Approach geometry, raised surfaces and directed routes are bounded derived output. Ordinary intersections currently provide geometric contact, not a complete lane graph. If an edit no longer supports the pinned generated race route, racing becomes unavailable with a diagnostic while the static city and canonical source remain; no replacement route is invented. Multi-lane, turbo, mini-roundabout, truck-apron and tram-through policies need explicit later profiles.

Solve trimmed spline approaches, entry deflection and exit connectors against a selected design vehicle's swept body/wheel envelope, minimum turning radius and clearance margin. Offset envelopes must not fold or overlap the raised island, splitter islands or opposing lanes. A larger vehicle may use an explicitly permitted apron; ordinary routes do not inherit that permission. Acute or closely spaced approaches fail or request road reshaping before geometry publication.

Generate each splitter island, entry give-way line, lane arrow, sign, kerb return and optional pedestrian crossing from solved ports and the selected profile. Crossings sit back from entries/exits with supported refuge/sidewalk connections, clear visibility reservations and explicit pedestrian policy. Keep street furniture and planting outside swept vehicle and sight envelopes. Avoid placing generic junction markings beneath the roundabout kit.

Replace the single traversable graph centre with directed entry/exit connectors and ring arcs in circulation order. Route choice follows permitted lane connectivity and can pass several exits; no edge or route shortcut joins ports through the island centre. Stable movement identity derives from junction/port GUIDs. Road-distance, driveable-surface and parcel-exclusion queries use the actual annulus plus connectors, with separate island/sidewalk exclusions; nearest distance to the old node is not proof of driveability.

Fit a continuous supported ring and approach profiles against one terrain candidate. Independently draping ring vertices can twist the carriageway or create entry steps. Validate longitudinal grade, crossfall, vertical transitions, drainage and full-width terrain coverage; retain cuts/fills in the same undo/publication transaction. Water and rail reservations still apply, and geometric overlap cannot create an implicit ford or rail crossing.

Prepare ring/connector surface collision, island and splitter kerbs, sidewalks and visible geometry from the same solved boundaries. No solid disk collider fills the island or masks unsupported terrain; no junction-wide bounding box becomes an invisible wall. Mesh normals/winding, seam heights and contact behaviour must agree around every entry and the inner kerb. Reuse the existing road mesh/result streams and extend `buildRoundabout` compatibly behind validated dimensions/ports, with explicit legacy-radius handling; do not introduce a separate owning junction renderer or physics system.

| Existing slice | Roundabout work and exit gate |
|---|---|
| T0 source contracts | Add typed junction/port/profile records and strict migration; GUID/port references, save/reopen, duplicate/dangling IDs and invalid radii tests pass. |
| T1 hydrology and terrain | Reserve the complete junction footprint and prepare supported ring/approach grades; holes, steep ground, water overlap and stale terrain fail atomically. |
| T2 rail corridor | Validate country/profile defaults, design-vehicle swept paths, entry deflection and rail/crossing reservations; a rail intersection remains an explicit separate crossing. |
| T3 first bridges | Publish shared solved ring/connector geometry and collision; native tyre/ray probes cross every entry and follow the ring without island shortcuts or seam impacts. |
| T4 crossings and Editor | Draw/convert a junction, drag approach ports and edit island/profile controls with live movement preview; one gesture supports undo/redo, cancel and save/reopen. |
| T5 generated districts | Seed eligible junction conversion before parcel packing; preserve locked approaches and movement IDs, with bounded geometry/physics cost and packaged reconstruction. |

Required full-tier coverage includes driving-side profiles, asymmetric ports, seed replay, narrow annuli, acute entries, overlapping splitters, design-vehicle sweeps and later-exit routes. Repeat focused solver and native terrain/collision, island-exclusion, circulation and route-height checks for the current integration. Acceptance still requires actual overhead movement, driver-height lap, inner kerb/entry/refuge and graded edit/undo/reopen captures, with source/profile IDs, movements, swept clearance, contacts, LOD seams and frame costs. A visible circle or mesh-only test cannot close the gate.

## Editor and publication workflow

Extend the existing city authoring controller with River, Rail, Crossing and Bridge tools. Reuse its selected terrain ray and spline handle conventions; [TerrainWindow](../Tools/cpp/Editor/src/ui/TerrainWindow.cpp) provides listener/cancel ownership, and [UIManager](../Tools/cpp/Editor/src/ui/UIManager.cpp) provides the active window lifecycle. No new general owning runtime system is required.

- Drawing a river previews the water line, bed, banks and downstream arrows. Handles retain GUIDs; width/depth/level edits reveal invalid uphill or unsupported sections before commit.
- Drawing rail previews both rails, the loading/cant envelope and grade/radius violations. A Switch action connects explicit compatible ports. Station/yard reservations are visible before approval.
- Water/road/rail intersections produce labelled candidates: reroute, supported bridge, explicit level crossing, or reject. Accepting one records stable source references and exposes deck, abutment and approach handles.
- Manual bridge overrides preserve their identity and locks. Moving the river or terrain makes an override valid, stale or invalid; it must not silently detach the bridge or flatten the carrier.
- Escape, selection/project changes, hide and unload cancel the active preview. Terrain painting, actor picking and spline handles arbitrate one active pointer tool.

Current road/roundabout gestures use concrete `CityRoadEditCommand` history over the inline canonical source, with scene/project/authored-state guards. **Save Scene** persists the source/terrain anchors and vehicle source reference; generated caches are excluded. Derived-city refresh watches the current source revision plus retained terrain snapshot/placement, attempts a changed tuple once, and regenerates derived content without replacing those anchors. Whole-parcel road exclusions cull new buildings from edited corridors. Foliage retains an identity standalone carrier with holes/no rendered ground and uses world positions plus pad heights, avoiding a second bound-terrain transform. Project-asset UUID/importer/file replacement, external conflicts and joint source/terrain history remain future work.

Prepare one candidate bundle containing source/terrain/placement generations, channel terrain, water, rail/roads, bridge meshes, collisions, exclusions and dependency hashes. Heavy preparation runs outside publication locks with cancellation and SourceBudget admission. A stale generation, missing support, unsupported asset, failed GPU upload or physics-capacity failure retains the previous complete city.

Extend the terrain publication gate with the dependent city commit. Every consumer must be ready before the commit performs no-fail swaps under the established render/physics ownership rules. Do not call a live terrain grade, publish it, and then attempt bridge generation. Split large edits into bounded prepared chunks but commit the complete edit under one revision. Terrain integration notifications refresh dependent caches after this joint publication, not instead of it.

Authored source edits against unchanged terrain prepare and swap matching road render/collision candidates. Terrain edits remain **non-atomic across terrain and roads**: after terrain publishes, a failed road recook deactivates the old roads and reports the failure. Retaining a complete old-or-new terrain/city bundle remains future work, as does last-good replacement for the separate whole generated city. The CPU source still rejects Bridge mode; Editor controls do not remove that restriction.

## Initial bounded budgets

These are proposed first-slice limits and profiling targets, to be validated against the actual city and chosen hardware.

| Resource | Initial bound or acceptance target |
|---|---|
| Source and sampling | Up to 4 river reaches, 8 rail splines and 8 bridges; at most 16,384 combined water/rail profile samples. Source IDs, controls and topology also have bounded counts. |
| Terrain work | Retain existing 65,536-change and 16-million-sample-segment limits per prepared grading chunk; bound total chunks/work before allocating. No partial visible commit when a later chunk fails. |
| CPU preparation | At most 32 MiB additional admitted transport/water candidate storage for the first bounded-city slice; preserve SourceBudget category accounting and last-good resident data. |
| Visible geometry | Target no more than 50,000 water, 100,000 rail/sleeper and 100,000 bridge triangles per main view; use patch LODs, shared sleeper instances and bounded materials. |
| Physics | At most 64 additional static bodies for this slice, admitted against actual remaining scene capacity. Use continuous track-bed/deck collision; no actor per sleeper, pane, girder or foam strip. |
| Reflections | One optional half-resolution planar water reflection per view with probe fallback; cap additional GPU water/transport residency at 64 MiB for the first quality profile. Views own independent targets/history. |
| Measured cost | Initial 1080p targets: water including its reflection at most 1.5 ms GPU and rail/bridges at most 1 ms GPU on the agreed reference hardware. Report actual percentiles/residency and failure cases. WARP is functional evidence only. |

## Executable implementation sequence

| Slice | Concrete change | Exit evidence |
|---|---|---|
| T0 source contracts | Extend proposed v3 with river/rail/crossing/bridge records and common spline sampling; preserve actual v1/v2 compatibility. | Identity-preserving round trips; duplicates/dangling references/oversized input rejected; locked edits and tombstones survive regeneration. |
| T1 hydrology and terrain | Prepare one authored river/channel with downstream water profile, bank reservation and bounded terrain patch. Add explicit water renderer through WATER-01/02. | Bed/bank/water agreement, supported shoreline, cancellation/failed upload retains old city; actual shallow/deep water captures. |
| T2 rail corridor | Add one authored rail alignment, gauge/sleepers/ballast, terrain profile and station reservation; no train simulation. | Radius/grade/cant/clearance tests, supported cut/fill and stable rail mesh/collision; close and distant rail captures. |
| T3 first bridges | Fit one road bridge and one rail bridge over the same supported river policy, including approaches, deck/underpass collision and abutments. | Vehicle/ray contacts and clearance agree with rendered surfaces; invalid short approaches and unsupported foundations fail without publication. |
| T4 crossings and Editor | Draw/edit all source types, switch/level-crossing topology, accepted crossing candidates, manual bridge overrides, undo/redo and save/reopen. | Recorded real Editor interaction and source-to-runtime reconstruction; external conflict, unload and stale terrain tests. |
| T5 generated districts | Apply hydrology/rail reservations before natural road growth, then crossing approval and parcel/vegetation generation. Add WATER-03 through WATER-06 at the supported tier. | Same seed/source reproduces topology and crossings; generation preserves manual edits; packaged source-free city and hardware performance capture. |
| T6 rail operations | Add train-following, occupancy/reservations, switch operations and signalling under a separately scoped vehicle/simulation plan. | Connected reserved routes, conflict rejection, stopping/clearance and unload recovery; static tracks alone cannot close this slice. |

## Verification and visual acceptance

Required future CPU coverage includes downstream/confluence constraints, crossing classification, rail radius/grade/cant/gauge, switch connectivity and full-footprint transport reservations, alongside the road/terrain cases. Include seeds 0, 7, 42 and maximum u32 across supported city sizes, plus tight bends, parallel crossings and nearly coincident intersections. These are acceptance requirements, not claims that water/rail tests currently exist or pass.

Publication tests inject terrain/source/placement changes between preparation and commit, SourceBudget denial, missing material/profile assets, physics-capacity denial and device upload failure. Undo/redo, cancel, delete, duplicate and reopen must preserve the correct source identity and either the complete old or complete new generation. A height query at a few knots does not prove continuous support or collider agreement.

| Capture | Required checks |
|---|---|
| City overview and downstream/ upstream views | Coherent river course, rail corridor reservations, supported banks, connected approaches and believable skyline crossings. |
| Water edge close-up and bridge underside | No cracks, floating banks, hidden flat collision plane, deck penetration, missing abutments or opaque solid-channel collider. |
| Road drive over both approaches | Surface/physics seam agreement, grade transitions, barrier behaviour and conservative bridge LODs in motion. |
| Rail eye-level and low side view | Stable gauge, sleeper spacing, cant transitions, turnout continuity, ballast contact and platform clearance. |
| Two cameras plus reflection view | Matching depth/colour inputs, no recursive water, independent visibility/LOD/history and bounded reflection cost. |
| Before/edit/cancel/undo/reopen | Source and terrain revisions match visible geometry and collision; invalid edits retain the last complete scene. |

Record source and dependency hashes, terrain/source generations, seed, active transport/crossing counts, actor/shape capacity, CPU/GPU residency, draw/triangle counts and frame-time distributions with each capture. Functional tests and compilation are not visual or production acceptance. This work closes only after the real Editor, native runtime, source-free package and representative hardware demonstrate the same supported river/bridge/rail/roundabout scene.
