# WPProcedural implementation status

## Scanned city surface materials — 14 November 2026

The bundled [Poly Haven CC0 pack](../bin/Media/Procedural/City/Textures/README.md)
contains six surface sets and 24 unmodified 1K source maps (37,168,076 bytes).
Asphalt, pavement slabs, brick, plaster, concrete and grass now feed the generated
city and editable-road PBR materials. World projection uses physical tile sizes;
the brick tile has an explicit 1.5 m architectural override. Source downloads,
authors, hashes and licences are retained with a reproducible offline-verifiable
restore script. Runtime generation never downloads assets.

`CitySurfaceLibrary` bounds encoded files and source dimensions before decoding,
supports 8/16-bit channels and 256/512/1024 px outputs, and publishes a complete
surface transactionally. Colour downsamples in linear light, normals are
green-converted and vector-filtered, roughness uses fourth-moment filtering, and
AO/roughness/nonmetallic channels bind the engine's separate map slots. Source
buffers and codec work use the existing shared memory ledger. Missing, invalid
or denied sets leave the caller's previous data intact and use procedural
fallback materials.

New terrain imports converted grass maps through the existing immutable project
texture source writer. Canonical `data://` layer references survive source
saving, regeneration and dependency collection; existing terrain materials are
preserved. Scanned asphalt export writes the rendered maps with accurate CC0
provenance while retaining the legacy procedural export contract. The sample
reports live scanned material IDs, physical scale, dimensions and native map
bindings, plus terrain texture references.

Workphone, SampleVehicleAdvanced and Editor Debug builds pass. All six focused
checks pass on the final build in 182.79 s: the real-asset loader suite (24.84 s), texture export,
Lua material chain, road terrain mesh, road authoring, and Editor terrain source
save/reload (154.63 s). Loader checks include channel values/normal precision,
all output sizes, malformed/oversized files and metadata, embedded thumbnail
stripping, failure on the last map, memory-budget denial and complete admission
return after teardown. Source-pack offline verification passes for all 24 maps.
[Build log](../artifacts/procedural/city-pbr-textures-20261114/build-iteration-02-retry.log) ·
[Test log](../artifacts/procedural/city-pbr-textures-20261114/tests-iteration-02.log).

Three fresh matched native captures pass consistency checks, including exact
cameras, unchanged editable road/roundabout controls, current source/binary/map
hashes, and native material bindings. District captures use all six families
through eight materials with 40 uploaded 512 px maps, plus three saved terrain
maps. All 77 ordinary-junction collision probes pass. Source CPU peaks at
338.82 MiB and GPU admission is 142.68 MiB, under their 512 MiB limits with no
rejected reservations. [Comparison and visual review](../artifacts/procedural/city-pbr-textures-20261114/visual-review.md)
records the observed gains in brick, concrete and paving detail, plus remaining
grass repetition, plaster mottling, dark glazing and lighting limits. Captures
use Microsoft Basic Render Driver; no physical-GPU performance, blind F1
comparison or photorealism claim is made.

## Shared road connection geometry — 14 November 2026

This iteration follows the dynamic connection approach described in the
[EasyRoads3D intersection documentation](https://www.easyroads3d.com/v3/html/intersections.html):
matching road ports, a shared intersection surface and continuous corner pavements.
It is an independent implementation for Workphone's editable terrain roads.

Ordinary draped T/X contacts derive a shared junction bake from the canonical
splines. Trimmed approaches retain their original arc distances, while rounded
corners respect each road's width. Asphalt and collision use the joined footprint;
footways and edge markings consume the same corner boundaries. Centre dashes
stop at the connection ports. Source knots, handles, identities and saving remain
unchanged. Roundabouts retain their existing explicit source and fitting contract.

The C++ policy bounds corner radius, subdivision and trimming. This does not yet
provide independent intersection gizmos, saved per-corner radius controls or
custom connector mesh assets. Unsupported terrain and geometry retain the last
published result. Synthetic footway terrain-fitting batches explicitly bypass
junction solving so their already-defined footprints stay unchanged.

The sample adds a `junction` inspection view and native collision probes at
connection centres and rounded corners. The existing `corner` view remains
available for a matching camera comparison with the previous curved-road build.

Workphone, SampleVehicleAdvanced and Editor Debug builds pass. All ten focused
checks pass: the terrain mesh, source conversion and footway suites in 159.76 s,
and the remaining five geometry/authoring suites plus two Editor tests in
170.20 s. Viewport editing passes in 14.53 s; source-anchor save/reload passes in
140.45 s. Coverage includes unequal-width T/X connections, curved/oblique
approaches, translated corners, overlapping surfaces, terrain holes, unrelated
roads, roundabout conflicts, malformed samples and allocation rollback.
[Geometry tests](../artifacts/procedural/city-road-connections-20261114/tests-geometry-05-full.log)
[Editor and integration tests](../artifacts/procedural/city-road-connections-20261114/tests-integration-05-full.log)

The first validation exposed curved-cap clearance and large-coordinate pavement
stitching failures. Bounded trim refitting resolves the former. Pavement skirts
now follow only the outlines of successfully fitted pieces, with opposite-side
intervals cancelling shared caps; terrain-cell diagonals cannot become internal
kerbs. Tests require both unobstructed walking arcs and retained real kerbs.
Exact top-surface render/collision triangles remain intact. Sharing vertices
within each clipped terrain-cell polygon reduces the initial junction candidate's
ten-block grid from 354,516 to 276,001 road vertices without changing its 118,172 triangles. The
existing 350,000-vertex regression limit and terrain coverage checks are unchanged.

All three final native captures pass and exit 0: overview, matching bend view
and close junction inspection. The city has 21 ordinary connections (17 T and
four X), 56 rounded corners and 77/77 native connection collision probes.
All 279 building meshes are ready, the 762-sample route remains driveable, and
no memory reservations are rejected. The matching bend view visibly replaces
the pointed junction mouth with continuous rounded pavement corners and stops
centre markings before the crossing.

Capture integrity verifies 1,354 current source files and 11 binaries, all image
hashes, unchanged road/roundabout controls and the identical bend-camera pose.
The new junction camera is a separate inspection. Runtime road triangles rise
from 103,426 to 106,819; overview peak Source CPU is 322.26 MiB versus 322.63 MiB
in the baseline. Microsoft Basic Render Driver captures do not establish
physical-GPU performance. Existing roundabout pavement corners, building
materials and lighting remain outside this ordinary-junction geometry pass.
[Final visual review](../artifacts/procedural/city-road-connections-20261114/visual-review.md)
[Capture integrity](../artifacts/procedural/city-road-connections-20261114/capture-integrity.json)
[Native junction capture](../artifacts/procedural/city-road-connections-20261114/after/junction.bmp)

## Terrain-aware editable road curves — 14 November 2026

Natural terrain cities promote the centre-led road graph into genuine cubic
splines. Safe degree-two corners share tangents; T/X attachments and roundabout
ports remain pinned with straight approach collars. Terrain-scored lateral
alternatives consider grade, crossfall and elevation-profile variation. Arterial
spans permit at most 3.5% additional horizontal length; collectors/local roads
permit at most 14%. Existing 12/9/7 m widths and all branches remain intact.
When one half of a shared corner cannot fit safely, both sides are refitted
with matching original directions rather than publishing mismatched caps.

The source retains editable knots, handles and stable road/endpoint identities.
Route fitting follows the current monotonic curve within the original corridor,
including sparse pinned routes, and retains continuous surface/island checks.
It does not rewrite the canonical source. Existing straight saved sources and
the explicit C++ `terrainCurves=false` compatibility policy remain supported.

New natural-city terrain grades the selected central junction district to one
tangent plane, then blends into broad rolling relief. The whole approach pads
and covering terrain cells meet the existing 1 mm seam contract. This shaping
applies only when creating terrain; a retained user terrain remains unchanged
and must meet existing support/grade/roundabout fit requirements. Flat previews
retain the original straight graph renderer. This is bounded corridor fitting,
not a global path planner around cliffs, waterways or missing terrain.

Capture evidence now includes the canonical editable source, curved spans,
centreline deviation/grade, road height range and arterial length ratio. The
native road ray probes the most curved sample with a 1 cm height tolerance, so
the terrain 2.5 cm below it cannot masquerade as road collision. The city corner
camera stays by the road and clears building sight lines.

Workphone, SampleVehicleAdvanced and Editor Debug builds pass. Ten focused tests
pass: road-source conversion in 72.17 s, plus seven geometry/authoring suites and
two Editor integration tests in 233.20 s. The source suite covers 72 additional
seed/size/route conversions, mirrored terrain response, grade/detour limits,
T/X pins, paired corner fallback, saved edits, legacy sources and admission
rollback. Mirroring the test ridge changes the local road's mean lateral offset
from -5.503 m to +5.503 m. Editor viewport authoring and source-anchor save/reload
both pass. [Source tests](../artifacts/procedural/city-terrain-curves-20261114/tests-road-source-02.log)
[Integration tests](../artifacts/procedural/city-terrain-curves-20261114/tests-integration.log)

The first source suite hit its unchanged 180 s limit. Precomputed obstacle bounds
and shared-attachment lists remove redundant curve-clearance work; duplicate
short-span candidates are skipped, and wholly straight sources retain their
original route sampling. Exact geometric checks and time limits were not relaxed.
Fresh native overview and bend captures pass validation and exit cleanly. The
seed-42 six-block city has 42 editable roads, 252 knots and ten curved spans;
maximum lateral bow is 4.376 m, centreline grade 2.293%, and road height range
10.144 m. Main approaches remain direct (maximum arterial length/chord ratio
1.0000006). All 279 building meshes are ready, all four native collision probes
pass, and no memory reservation is rejected. The final captures match 1,352
current source files and 11 binaries by SHA-256.

The close inspection shows road edges, markings and pavements following an
actual bend continuously. Short constrained junctions remain angular; dark
facades, simple grass areas and subdued lighting still limit visual realism.
The captures use Microsoft Basic Render Driver and do not certify physical-GPU
performance or commercial-game visual parity. The bend camera is a candidate
inspection, not an identical world-space before/after comparison.
[Capture review and iteration history](../artifacts/procedural/city-terrain-curves-20261114/visual-review.md)
[Final capture integrity](../artifacts/procedural/city-terrain-curves-20261114/capture-integrity.json)
[Editable source diagram](../artifacts/procedural/city-terrain-curves-20261114/native-spline-plan.png)

## City centre and road hierarchy — 14 November 2026

Natural layout revision 4 adds four direct main approaches into the city centre
at the origin. Main roads are 12 m wide, district collectors 9 m and local streets
7 m. Added local junctions on main roads require at least 65% of nominal block
spacing on each side. The four outward main-road ends are intentional expansion
gateways. Road classes survive graph splits, and unlike classes never coalesce
into one editable source spline. Roundabout approach widths match their roads.
The configurable source width scales this hierarchy, with a 4 m lower bound;
grid road widths are unchanged.

The City Roads window keeps linked roundabout approach widths synchronized when
applying road fields or committing an edited draft. These updates share the road's
undo/redo transaction; invalid profiles leave the published source unchanged.

Central parcel candidates are packed first. Every natural building envelope
reserves 5 m beyond the actual road edge; local frontages no longer assume a
12 m carriageway. Flat and terrain scenes both use the different widths for
roads and footways. Existing saved road widths remain canonical and editable.
Revision-3 pinned race route coordinates remain identical, and the revision-2
route compatibility path remains available. Natural lap records use a new
namespace to account for changed road surfaces, preserving previous files.

The seed-42 six-block graph has 14 main-road edges, 22 collector edges and 19
local edges. Its main-road network reaches all four sides and remains within
1.22% of direct distance from the centre; the diagnostic seed corpus stays
within 2.05%. These are geometric route measurements, not vehicle travel-time
or traffic-speed results. [Graph comparison](../artifacts/procedural/city-centre-roads-20261114/graph-comparison.png)

Workphone, SampleVehicleAdvanced and Editor Debug builds pass. The six focused
native tests pass in 127.87 s, covering layout, road graph, source conversion,
street details, parcel support and footways. Tests verify the three width classes,
four connected main approaches, mixed-width roundabouts, safe building envelopes,
centre development and saved-source compatibility. The Lua racing fixture also
passes, including preservation of historical lap-record files.
[Native test log](../artifacts/procedural/city-centre-roads-20261114/tests-native.log)

Both Editor integration tests pass in 155.45 s. Viewport authoring checks linked
width edits, queued publication, undo/redo, rejected edits and repaired draft
commits. The terrain-city source-anchor roundtrip checks saved source retention.
[Editor test log](../artifacts/procedural/city-centre-roads-20261114/tests-editor.log)

Fresh High terrain overview/boulevard and Low flat overview captures pass and
exit 0. The terrain city publishes 42 editable roads at 7/9/12 m, four roundabouts,
140 supported parcels, 279/279 ready building meshes and four passing native
collision probes. The flat path has 95/95 ready building meshes. Neither path
rejects Source CPU, Working CPU or GPU reservations. All 1,352 inventoried source
files, 11 binaries and captured images match the final evidence hashes.
[Capture integrity](../artifacts/procedural/city-centre-roads-20261114/capture-integrity.json)

The inspected overview shows the four main approaches and subordinate local
streets; the boulevard has closer, taller central frontage and continuous
roundabout connections. Dark glazing, simple lighting and vacant grass remain
visible. Captures use the Microsoft Basic Render Driver; geometry-dependent
camera framing and concurrent Editor validation prevent performance comparisons.
[Native captures and visual review](../artifacts/procedural/city-centre-roads-20261114/visual-review.md)

This checkpoint did not yet include smooth curves, terrain-cost routing, traffic
control or speed-limit enforcement. [Usage and editing](RacingGameOpenCity.md)

## Natural road districts — 14 November 2026

Natural layout revision 3 replaces the three nearly identical nested loops with
a compact eastern district, a western neighbourhood and a wider eastern perimeter
around a shared avenue. The L-system grows streets along district directions,
uses 80–100 degree branch turns, stops at existing connections and closes suitable
dead ends. New connections require 24 m junction spacing, a 50 degree incident
angle and 18 m clearance from unrelated road centrelines. Seeded trunk corners
have a separate 45 degree regression floor. Junctions have at most four arms;
generation stops at 120 edges, below the canonical source's 128-road capacity.

All three routes retain the origin/−Z start and deterministic closure without
changing the graph or parcels when the route selector changes. Existing authored
terrain-road sources are not replaced. If their roads no longer cover the current
pinned route, the builder also validates the exact revision-2 route against the
current authored surface. Neither route is accepted without continuous road and
roundabout coverage. Flat natural-city lap records use a new filename namespace;
old files remain intact. [Usage](RacingGameOpenCity.md)

The seed-42 six-block graph changes from 83 nodes/101 edges to 40 nodes/51 edges,
with 23 dead ends reduced to zero. Its shortest segment increases from 4.80 m to
25.72 m and its smallest incident angle from 4.29 to 51.85 degrees. These graph
measurements are CPU diagnostics, separate from native capture validation.
[Graph comparison](../artifacts/procedural/city-natural-layout-20261114/graph-comparison.png)

Workphone and SampleVehicleAdvanced Debug builds pass. Six focused native CTests
pass in 123.96 s: layout, street details, road network, road-source conversion,
parcel support and footways. They include 204 seed/size graph cases with all three
routes, 72 additional source conversions using the runtime roundabout limit, and frozen revision-2
route compatibility with edited/missing/malformed roads. The Lua fixture also
passes, including separation from historical natural-city lap records.
[Native test log](../artifacts/procedural/city-natural-layout-20261114/tests-native.log)

Fresh matched High-quality overview and boulevard captures pass and exit 0 on
the Microsoft Basic Render Driver. The revised scene has 42 editable roads,
four roundabouts, 141 supported parcels (previously 117), 266/266 ready building
meshes and four passing native collision probes in each view. Source CPU,
Working CPU and GPU reservation rejections are all zero. All 1,352 inventoried
source files and 11 binaries still match the final capture hashes. The overview
shows more balanced connected districts and fewer narrow asphalt wedges; the
boulevard retains large grass gaps, dark windows and simple surfaces.
[Captures and visual review](../artifacts/procedural/city-natural-layout-20261114/visual-review.md)

Streets still use straight spans, buildings remain axis aligned, and terrain
fitting does not yet choose horizontal routes by terrain cost. Rivers, bridges,
rail routing and continuous curved collectors remain separate work.

## Expanded built-in building styles — 14 November 2026

The built-in selector adds **European Terrace**, **Parisian Mansard**,
**Warehouse Loft**, **Mediterranean**, **Art Deco**, and **Timber Gabled** at
indices 4–9. The original indices remain stable. The native Inspector, Lua
Script Inspector, sample command line and capture harness accept the expanded
set. Mesh overrides remain independent of the preset; clear a slot to restore
that preset's built-in part. [Preset names and workflow](RacingGameOpenCity.md#building-styles-and-mesh-slots)

District Mix chooses the expanded families by neighbourhood and stable parcel
variant. New geometry includes projecting bays, mansard dormers on all four
elevations, sawtooth skylights, terracotta tile courses, stepped crowns, and
timber gables. Both LODs retain the main roof forms. Optional ornaments share
a city-wide vertex allowance, divided per building; mesh staging is released
after publication. Above 128 building lots, window and mansard-dormer spacing
increases, prioritizing street elevations. Above 256 building lots, the fine
ornament allowance is halved and shared façade, paving, and soil maps use
256-pixel sources. Vehicle and carriageway maps keep their independent quality
settings. Existing memory limits are unchanged.

The Lua fixture passed after exercising all ten style indices, generation-cache
invalidation, bounds clamping, retention of the edited terrain-road source and
custom mesh slots, and saving/restoring the final preset. The native scene
round-trip fixture now saves all ten presets through the real scene file and
checks the source binding and module paths after reopening. Workphone,
SampleVehicleAdvanced, and Editor Debug builds succeeded. The procedural block,
ImGui viewport, City Roads authoring, and native scene round-trip CTests passed
(4/4), and the Lua open-city fixture passed.

Seven matched seed-42 six-block High captures cover all six new styles and
District Mix. All generated building meshes uploaded with zero rejected
Source CPU, Working CPU, or GPU reservations. The final larger-city revision
also passed Parisian captures at six, eight, and ten blocks; the ten-block city
loaded 576/576 building meshes with a 499.9 MiB Source CPU peak under the existing
512 MiB limit. These are DX11 software-renderer captures, not physical-GPU
performance certification. Dark glazing, coarse surfaces, sparse upper-floor
ornament, and empty streets remain visible; the commercial-game visual target
is still open. [Native captures and review](../artifacts/procedural/city-building-variation-20261113/visual-review.md)

## Modular city buildings — 13 November 2026

The initial **City Building Style** kit introduced District Mix, Masonry
Residential, Civic Stone, and Modern Office. Each supplies façade and roof geometry. Five
independent mesh slots replace the built-in wall, window, entrance, corner,
or roof modules using a project asset or a static mesh file. Invalid modules
report a diagnostic and use built-in geometry. The same building masses
continue to define terrain support and collision. The native sample adds a
fixed façade view for matched captures. [Building generator workflow](RacingGameOpenCity.md#building-styles-and-mesh-slots)

The four presets passed native capture at the same seed-42 parcel and camera.
The five-role smoke run placed 4,444 wall, 740 window, 111 entrance, 444 corner
and 111 roof modules. The wall and window budgets triggered built-in fallback
for the remaining placements. The generator geometry and budget contracts,
ImGui viewport test, City Roads regression, scene save/reopen and Lua city
fixtures passed. [Native evidence](../artifacts/procedural/city-building-kit-20261113/visual-review.md)

## Street editing and fitted pavement furniture — 8 November 2026

**Window > City Roads** now selects a spline by clicking its carriageway.
**Insert Knot** splits the clicked span at its nearest point on the selected
spline. The split preserves its cubic shape, existing knot IDs and roundabout
connections. The source edit uses the same Undo/Redo and Save Scene path as
the other road tools.

**City Street Detail** is a saved 0–100 control on `ProceduralRaceScene` and
`RacingGameOpenCity`. Regeneration places benches, planters, bins and pedestrian
lamps at fitted sidewalks. Admission checks the whole base against the published
sidewalk triangles, with relief and clearance limits. Higher values select a
larger deterministic subset of up to 88 fixtures at High quality. Fixtures use
up to five visual batches. [Editor workflow](WPPROCEDURAL_CITY_AUTHORING_PLAN.md#current-practical-workflow)

Matched seed-42 native boulevard captures at 0 and 100 report 0 and 88 fixtures.
Both retain 70 roads, four roundabouts, 117 parcels and 528 route samples. The
grid roundabout and new pavement-height **frontage** views use 22 roads, four
roundabouts, 128 parcels and 689 route samples. The matched grid frontage runs
also report 0 and 88 fixtures. All five captures pass native structural checks
and exit 0. Each pair used the same 11 retained runtime binaries.

Ten Debug geometry and authoring cases, two Debug Editor cases and the Lua
open-city fixture passed. The Editor tests exercise the viewport command path
and scene save/reopen. The WARP captures show the added fixtures and their
pavement contact. Dark glazing, empty verges and simple surface detail are
still visible. Recorded Editor pointer testing and commercial visual
comparison remain open. [Capture evidence and review](../artifacts/procedural/city-street-editing-20261108/visual-review.md)

## Generated cities with editable terrain roads — 25 October 2026

Final retained-binary captures 13/14/15 pass native structural validation and exit
0; normal run 16 passes functional edit/regeneration retention and collision with
exit 0 in 249.76 s. All three captures match the snapshot's 11 runtime hashes.
The original 180-second timing gate, native 09's unknown historical crash caller,
hardware performance, recorded Editor pointer acceptance and AAA visual acceptance
remain open. The independent grid review rates commercial quality **not met**.

`ProceduralRaceScene` now promotes grid and L-system streets into the existing
`CityRoadAuthoring` source when **City Terrain Roads** is enabled. The source and
terrain are persistent scene anchors; generated render/collision actors are
disposable and excluded from scene serialization. Regeneration preserves manual
road edits and terrain rebinding. Application updates rebuild derived city data
after source revisions, explicit road rebuilds and terrain snapshot/placement
changes. Failed or missing bindings report an error and remove stale derived
collision. The scene cannot silently return to a second flat ground world.

The converter assigns deterministic IDs, merges collinear chains, selects bounded
roundabouts and fits the original lap through their directed circulation routes.
Edited sources must still support that lap continuously, including terrain contact
heights and island exclusion. Unsupported laps become unavailable in native and
Lua racing flows. They are not replaced by an invented route.

Buildings use exact full-parcel terrain-cell coverage and road/junction clearance,
including the full 2.45 m roadside pavement reservation (5 cm gap plus 2.4 m walk).
Holes, excessive relief or unsupported parcels are culled; retained foundations,
building collision, parks and existing foliage placement use the same fitted
heights. Source roads own generated asphalt, kerb, turf and paint materials with
semantic texture/mip bindings. Flat street overlays are omitted in terrain mode.

The first fresh street capture exposed missing pedestrian edges and grass-only
splitter tops. The next pass adds concrete splitter rims and fitted 2.4 m footways
beside streets, approaches and roundabout edges. Footways sit 15 cm above terrain,
retain exact cell ridges/holes, omit crossing mouths and unsupported pieces, and
share their rendered top/kerb geometry with one concave collision mesh. Spatial
admission and batched fitting bound candidate, vertex and validation work.

Native iteration also found a redundant startup regeneration: terrain integration
serial notifications advanced the authored source revision. Serial-only updates
now acknowledge the unchanged snapshot/pose; actual terrain recooks preserve
the authored token. Focused tests cover both paths and source detach/reattach.

The native sample and capture script accept `--city-terrain-roads` and
`-CityTerrainRoads`. Evidence checks actual source/terrain revisions, fitted lots,
native surface rays, driveable route coverage and uploaded road material bindings.
Collision smoke additionally exercises manual edit retention, regeneration,
terrain rebinding and a same-source-revision terrain refresh. The final normal
run passes functional retention in 249.76 s under its separate 600 s diagnostic
limit. The original 180-second timing gate remains open.

`RacingGameOpenCity` now persists/forwards the terrain mode and canonical source
UUID. Quality/seed/source changes reuse the native vehicle/camera/component in
terrain mode; recreation from serialized script properties rebinds the same
document. Flat mode retains its existing lifecycle. Terrain-city lap records are
session-local until a stable edited-route fingerprint is exposed; regeneration
and detected route changes reset them and the UI states the policy. The real Lua city/component suites pass with fixture
checks for retained knot IDs/widths, cache invalidation and source restoration.

The retained build-21 baseline built the Debug sample, Editor and focused
targets. Its city/Editor CTest run passed 12/12 in 138.56 seconds, including all
seed/size source profiles, maximum footway profiles, exact support/holes/grade,
eight connected roundabout pedestrian
corridors, oblique seams, allocation rollback, source revisions and save/reopen.
The existing production-render GPU suite passed in 26.74 seconds. Both real Lua
city/component fixture runs passed, including 50 component scripts. These Editor
fixtures prove native controls and saved anchors; recorded mouse/DPI interaction
and a complete restored driving scene remain separate acceptance gates.

The preserved build-21 native baseline uses six-block cities, high quality,
foliage, 1280x720, braked replay and the actual DX11 HDR path:

| Capture | Roads / roundabouts | Supported parcels / building masses | Result |
|---|---|---|---|
| [Grid street and overview](../artifacts/procedural/city-terrain-20261025-04/manifest.json) | 22 / 4 | 144 / 216 | Both native validations and clean process exits pass |
| [Natural pedestrian view](../artifacts/procedural/city-terrain-natural-20261025-05/manifest.json) | 70 / 4 | 119 / 143; 3 parcels culled | Native validation and clean process exit pass |

All three views verify four native road/ring/island/terrain rays, a separate
native hit on the single fitted footway collider, island exclusion, continuous
driveable route heights and 16 uploaded road PBR textures. Footway height errors
are at most 0.00000012 m. Source, active and applied road revisions agree. The
maximum-city corpus is CPU geometry/admission evidence; these native runs use
six blocks.

Fresh captures exposed and fixed a real renderer lifetime defect: the sky cache
held a legacy weak pointer that dereferenced freed sky storage during renderer
shutdown. It now uses the existing independent `ObjectObserver` lifetime token
and retires expired entries. The GPU regression physically frees six sky owners
across three renderer lifecycles before cache destruction. The failed capture
and exception stack remain in `city-terrain-20261025-03`. A separate hot cleanup
stack exposed global update-list sorting on every actor removal; stable erasure
now preserves survivor order, with a regression in the save/reopen fixture.

The images are visibly improved at road/kerb contact but remain prototype art:
repeated opaque glazing/massing, sparse ground content, limited street props,
simple lighting and visible aliasing still fail commercial visual acceptance.
The adapter is Microsoft Basic Render Driver/WARP. Grid street/overview mean/P95
are 207.61/234.37 ms and 258.42/442.61 ms; natural pedestrian mean/P95 are
598.16/977.98 ms, over 29 measured frames each. These short software-renderer
runs, including concurrent workspace activity, do not certify hardware speed.

Capture 04 records the verified build and source/binary hashes. Capture 05 uses
the same binary hashes; concurrent material/Lua work subsequently replaced shared
Debug DLLs. The later retention runs use a separately frozen binary set, not the
04/05 baseline, and do not certify unrelated concurrent changes.

| Diagnostic evidence | Observed outcome |
|---|---|
| [06: original smoke](../artifacts/procedural/city-terrain-retention-20261025-06/manifest.json) | Timed out at 180.37 s; the original timing failure remains open |
| [07: extended diagnostic](../artifacts/procedural/city-terrain-retention-20261025-07-diagnostic/manifest.json) | Same frozen binaries/fixture, separate 600 s limit; access violation `0xc0000005` after 259.52 s |
| [08: CDB diagnostic](../artifacts/procedural/city-terrain-retention-20261025-08-debugger/outcome.json) | Same frozen binaries under launch-time debugging; fourth generation still progressing at the 600 s timeout, no reproduced exception stack or functional pass |
| [09: post-FSM functional diagnostic](../artifacts/procedural/city-terrain-retention-20261025-09-fsm-fixed/manifest.json) | Four generations complete; first resumed Physics update finds a foundation without its active static body, followed by an access violation at 250.07 s; no native retention pass |
| [10: normal launch and debugger attachment](../artifacts/procedural/city-terrain-retention-20261025-10-debugger/outcome.json) | Same frozen binaries/fixture; foundation assertion reproduced, then native/debugger exit 1 with clean teardown at 255.19 s. No access violation reproduced or exception dump captured |
| [11: Editor first-chance diagnostic](../artifacts/procedural/city-terrain-20261025-11-editor-debugger/debugger.log) | Captured the null scripting-manager assertion before Boost handled it, then an Editor test-exit WPAssimp/type-pool lifetime fault. Dumps and debugger cleanup are retained; this was not a passing test or a reproduction of native 09's caller |
| [16: final normal functional run](../artifacts/procedural/city-terrain-lifecycle-20261025-16/manifest.json) | Functional retention and collision pass; exit 0, no timeout, unchanged binaries, 249.76 s under the separate 600 s limit. The original 180 s timing gate remains open |

For 07, Windows Error Reporting identifies `Workphone.dll+0x14511c8`; following
the frozen DLL's export thunks resolves it to `FSM::getAutoChangeState+0x18`.
Source inspection found an FSM cached pointer into manager flag storage that can
move when the manager grows. The fix resolves the current slot through locked
manager access and prevents unloaded FSMs/deferred references from touching a
retired slot. The Editor regression forces storage relocation and checks live
flag values plus retired-queue behaviour. This diagnosis uses the recorded fault
address and source evidence; 08 did not capture a matching exception stack.

Build 23 completes the synchronized FSM repair and full pavement reservation.
The [final city/Editor/Play matrix](../procedural-city-integration-20261025-tests18-final-city-play.log)
passes 18/18 in 186.84 s: the 12 city/Editor checks plus six Play-mode lifecycle
checks. This includes forced FSM storage relocation, retired/deferred-slot safety
and full pavement/parcel exclusions at default grid roundabouts and edited roads.
The [09 retained snapshot](../artifacts/procedural/city-terrain-retention-20261025-09-fsm-fixed/binary-snapshot.json)
preserves all 11 runtime artifacts, matching Core/Graphics/Sample/Physics/Procedural
symbols and scoped source hashes; all 11 live runtime hashes matched the snapshot.
The [build-23 production-render GPU suite](../procedural-city-integration-20261025-tests19-final-renderer.log)
also passed in 28.73 s. Native 09 exposes a further publication-order defect:
new generated actors wait for the Application scene play queue, while Physics
can resume before their static bodies are activated. The collision assertions
remain unchanged. The attached
[10 diagnostic](../artifacts/procedural/city-terrain-retention-20261025-10-debugger/manifest.json)
uses a normal launch of the same frozen binaries with matching symbols.
The separate 09 crash maps to an atomic byte compare/exchange at
`Workphone.dll+0x2091dd`; its caller remains unknown because the WER dump was
removed and 10 did not reproduce the access violation. It is not a confirmed
recurrence of the earlier FSM fault.

The production activation barrier resolves parent transforms and activates generated
actors plus persistent terrain/source anchors on the Application task, under
render/physics locks. It verifies attached native static bodies, shapes and world
poses before driving readiness becomes true. Scene restoration or an unattached
owner leaves readiness false with braking applied; publication errors remain
visible. This uses actor/component lifecycle entry directly, without pumping
unrelated scene updates. Queued Edit/Play events reject unloaded, removed or
transferred actors and events for the wrong current scene state. Live children
remain admissible through a loaded ancestor anchored in the current scene.

The earlier [activation/anchor/FSM regression](../procedural-city-integration-20261025-tests20-publication-anchor.log)
passed in 24.27 s. [Tests 21](../procedural-city-integration-20261025-tests21-publication-city-play.log)
passed the nine other geometry checks but failed all six Play cases. CDB 11
identified the first failure as `EditorApplication::update` querying a null
scripting manager at line 1126; that polling now checks the manager first. Its
later Editor exit fault occurred because an aborted fixture left plugins alive
when test Main retired TypeManager's pools. Main now unloads any orphan singleton
manager before those pools, preserving Boost's failure result and never calling
an abandoned stack Application. This Editor diagnosis does not identify native
09's unknown shutdown caller.

[Build 28](../procedural-city-integration-20261025-build28-lifecycle.log) completed
the Sample and native dependencies; an Editor regression const-handle compilation
error was corrected. [Build 29](../procedural-city-integration-20261025-build29-lifecycle-tests.log)
then rebuilt Editor and the renderer-test target against the completed
dependencies with project-reference rebuilding disabled. [Tests 23](../procedural-city-integration-20261025-tests23-lifecycle.log)
pass 9/9 in 71.96 s: authoring, viewport, anchor persistence and all six Play cases.
The anchor fixture covers both root and child queued activation, repeated drains,
removed/transferred actors, FSM storage growth/retirement and saved identities.
The [final renderer suite](../procedural-city-integration-20261025-tests24-lifecycle-renderer.log)
passes in 30.76 s.

The [16 lifecycle snapshot](../artifacts/procedural/city-terrain-lifecycle-20261025-16/binary-snapshot.json)
retains 11 runtime artifacts and five matching PDBs with scoped source hashes;
the normal Debug media working directory is still required. Its
[normal native retention run](../artifacts/procedural/city-terrain-lifecycle-20261025-16/manifest.json)
passes with exit 0, no timeout and unchanged binaries in 249.7603777 s. The
unchanged fixture preserves the edited road GUID and 11.75 m width through four
generations, terrain rebinding, retirement of the old terrain and a terrain
snapshot refresh. Generation times are 38.829/40.720/41.853/42.473 s. Native
collision validates 452 static boxes, 178 building masses, four surface rays,
one footway ray and 16 uploaded road PBR textures. This closes functional
retention/contact acceptance for that fixture, not its original 180 s timing gate.

All three final captures pass native validation, exit 0 without timeout and use
the same frozen snapshot-16 binaries. The recorded 11 runtime hashes in each
manifest match that snapshot, including independently checked 14/15 inventories.

| Final capture | Roads / roundabouts | Supported lots (building / park) | Building masses / culled parcels | Route samples | WARP mean / P95 ms |
|---|---|---|---|---|---|
| [13 grid street](../artifacts/procedural/city-terrain-20261025-13-grid-street/manifest.json) | 22 / 4 | 128 (111 / 17) | 178 / 16 | 689 | 173.889 / 189.998 |
| [14 natural boulevard](../artifacts/procedural/city-terrain-20261025-14-natural-boulevard/manifest.json) | 70 / 4 | 117 (107 / 10) | 139 / 5 | 528 | 214.650 / 229.344 |
| [15 natural overview](../artifacts/procedural/city-terrain-20261025-15-natural-overview/manifest.json) | 70 / 4 | 117 (107 / 10) | 139 / 5 | 528 | 107.743 / 125.342 |

Each view validates four baseline native surface rays, one fitted footway body/ray
and 16 uploaded road PBR textures. Both natural views contain 111,530 authored
road runtime triangles and retain driveable route coverage and island exclusion.
The [independent visual review](../artifacts/procedural/city-terrain-20261025-13-grid-street/visual-review.md)
finds clearer pavement/foundation contact but still rates the scene a prototype;
the commercial visual gate is not met. Each WARP view measures 29 frames; these
short software-renderer intervals do not establish hardware performance. The
three capture exits plus normal functional run 16 leave native 09's historical
shutdown caller unresolved; newer success does not identify that crash's cause.
CDB launch may perturb allocation costs; 08 is not performance evidence.
Hardware performance, recorded Editor pointer/DPI acceptance and source-free
reconstruction remain open.

Remaining gates include ordinary junction reconstruction, automatic grading,
arbitrary edited-lap topology, transport/water integration, representative GPU
performance and independent commercial-quality visual comparison.

## Racing foundations and visual slice — 12 October 2026

This implements parts of P0–P5, P8 and P9 from the [production plan](WPPROCEDURAL_PRODUCTION_PLAN.md). It does not complete P0–P10 or certify AAA quality. Changes reuse the existing procedural services, racing component, terrain foliage attachment and resource compiler.

### Runtime and authoring changes

- `MeshGeneratorDefault` now generates terrain and road meshes, supports the corresponding object adapters, retains validated settings and publishes progress/history. Budgets, malformed dimensions, nonfinite data and unsupported options fail explicitly. Failed requests retain the previous generated mesh. LOD generation, simplification, tangents, arbitrary object conversion and collision generation remain unsupported by this adapter.
- `ProceduralMesh` now computes retained bounds, validates geometry indices/finite positions, preserves UVs and masks during append, and handles self-append. Invalid append candidates do not alter geometry. Public direct vertex edits still require explicit bounds recomputation.
- Fixed vehicle sidepod loft indices that referenced the central body instead of the appended pod vertices. Added closed pods, an open cockpit, interior/rails, raised airbox, cambered wings, pylons/endplates, mirrors and diffuser details. Wheel/axle physics metadata remains shared with the existing vehicle generator.
- Circuit and city reuse generated asphalt maps; city facades have generated concrete maps, roof/parapet/plinth modules and roof service details. Pit garages have an apron, roof caps, fascia, glazing, mullions and door slats. This is an incremental procedural art pass, not a production architectural kit.
- Optional `Foliage World` selection uses the existing terrain-attached foliage stream and Claw renderer: four seeded species with real reduced mesh LODs, a seeded 256px leaf-spray atlas with individual leaves/twigs/alpha gaps, alpha-cutout mips, wind, circuit exclusions/city-park placement and trunk colliders. Legacy patch trees remain available. Scene teardown clears/unregisters the foliage attachment before destroying generated textures. Runtime texture resolution now finds generated manager-owned atlases.
- Native and Lua samples share the component's foliage selection. Lua saves it through the sample's properties. Native quality names now include `cinematic`, matching the existing four-value quality enum; the name itself makes no visual/performance guarantee.
- Fixed graphics resource property updates clearing resource names: partial edits now retain the name, and explicit name edits apply correctly. The generated foliage atlas uses its registered identity and verifies the species retains its cutout binding. This repair was prompted by actual captures showing opaque white leaf cards despite healthy paging diagnostics.
- Shared CPU image encoders produce real PNG/TGA/PAM with straight alpha and opaque-only PPM. Unsupported formats and malformed sizes fail; colour/data values are not implicitly gamma-converted. Asphalt source export writes six TGA maps with semantic `.texres` descriptors, a current-tier `.matres` and a completion manifest into a fresh directory. Existing output directories are refused.
- Texture Editor supported chains now execute in stable creation order and publish only successful complete candidates. Versioned JSON recipe save/load validates detached plain data and retains the previous generated texture until a replacement succeeds. This is a linear supported chain, not a general texture DAG.

### Review harness

Native sample additions:

| Option | Behavior |
|---|---|
| `--evidence PATH.json` | Writes actual adapter/driver, capture-target dimensions, requested settings, completed native frame diagnostics, render timings/counters and structural validation result |
| `--replay PATH.csv` | Applies one control row per physics-task tick, in contiguous tick order, then holds the brakes at end |
| `--foliage-world` | Selects paged instanced vegetation instead of legacy merged tree patches |
| `--export-materials NEW_DIR` | Writes the generated asphalt source set; parent directory must already exist |

Replay header is `tick,throttle,brake,steering`. Ticks begin at zero with no gaps. Throttle/brake are in [0,1], steering in [-1,1]; values must be finite. The physics task targets 120 Hz, but existing physics uses measured/clamped deltas: input replay is not a fixed-step deterministic physics implementation or a cross-platform replay guarantee. Inputs start when the post-load physics configuration is ready. Timing, reset and camera-track recording remain future work.

`Tools/powershell/CaptureProceduralRacing.ps1` runs selected views into a fresh output directory and saves source/diff/binary/media/plugin hashes, exact arguments, logs, output image hashes and runtime reports. It defaults to silent audio and accepts `-FoliageWorld`, `-ExportMaterials`, `-Replay`, `-Views`, `-Seconds`, `-Resolution` and `-Configuration`. Build separately before capture. Asset hashes are a media inventory, not a runtime dependency trace. The script rejects reused output directories and reports capture/manifest/timeout failures.

```powershell
& 'C:\Program Files\CMake\bin\cmake.exe' -S . -B project_x64 -DWP_BUILD_PROCEDURAL_SERVICE_TESTS=ON
& 'C:\Program Files\CMake\bin\cmake.exe' --build project_x64 --config Debug --target SampleVehicleAdvanced ProceduralServiceTests VehicleReviewTests WPGraphicsCookedResourceTests
& .\Tools\powershell\CaptureProceduralRacing.ps1 -Configuration Debug -Views car,corner -FoliageWorld -ExportMaterials
```

### Evidence and remaining gates

Debug sample and focused native targets build. Six focused CTest checks passed after rebuilding the graphics-resource-name fix: procedural services/geometry/component properties (including the name regression), replay/evidence contracts, Lua texture recipes, native texture encoders, real vehicle Lua presentation bindings and cooked material/resource GPU contracts. Standalone Release export/recipe checks passed 2/2; independent Pillow decoding confirmed PNG/TGA/PAM pixels and alpha. The native generated asphalt test exercised six semantic map cooks, exact uploaded albedo mip bytes, a base-colour-only DX11 draw, failed-rebake retention and source-free package closure/reupload. Structural validation reports do not establish image quality or the performance gate.

The earlier capture set at `artifacts/procedural/implementation-baseline-02` passed capture/native-frame/winding/LOD checks on Microsoft Basic Render Driver/WARP. Car/corner mean intervals were 192.541/207.309 ms with P95 216.277/218.981 ms; the runs contain only 29 measured renderer frames each. That set predates the completed hero geometry and optional foliage path. It fails the proposed frame target and remains visibly prototype quality. It is not hardware certification.

`artifacts/procedural/implementation-final` contains successful car/corner captures with repaired hero geometry, working leaf cutout binding, paged foliage, source material export and replay input. Both reports pass native frame, winding, vehicle LOD and foliage readiness checks. Car/corner mean intervals are 132.443/117.185 ms and P95 143.434/128.823 ms over 29 measured renderer frames each, on the same software adapter. Replay consumed 58/120 rows in each run; this is input-application evidence, not proof of deterministic physics. Visual inspection confirmed the atlas repair but found oversized leaf silhouettes; the subsequent canopy capture tests the finer clustered atlas. These are short diagnostic runs and do not close hardware/performance or AAA visual gates.

The final `artifacts/procedural/implementation-canopy/car.bmp` capture was inspected after the clustered-atlas pass: the green leaf sprays and denser crowns render correctly, with no white opaque cards. Its report passes structural validation, with 44 resident foliage pages, 144 instances, no failed pages, and 58 replay ticks consumed. WARP mean/P95 are 128.794/141.021 ms over 29 measured frames. Capture runner exits successfully. The scene still fails the requested AAA visual target: lighting/terrain remain simple, branching/card repetition is apparent, ground treatment is sparse and vehicle surfaces/content need further art and renderer work. No commercial-reference comparison was performed.

### Cooked PBR material increment — 13 October 2026

The cooked material schema now accepts optional `normalTexture` and `ormTexture` references. Each dependency retains its exact source/payload/compiler generation; the decoder accepts legacy base-colour payloads and validates the extended semantic slots, unique install closure, target/build mode and linear Normal/Data filtering. GPU staging uploads and retains all three maps before publication. One ORM upload is shared across the renderer's explicit channel slots: red occlusion, green roughness and blue metalness. Generated asphalt source export now writes these bindings with unit material multipliers. The compiler version advances to invalidate stale material cooks.

The sample's sampler, texture-scale and vehicle material controls now use the existing explicit material-state setters. Their previous top-level property bags did not apply those editor-state fields. Clearcoat state can now be authored correctly, but its native shader support still requires separate inspection and verification.

The fresh Debug rebuild and both targeted CTest checks pass: `WPProcedural.services` and `WPGraphics.cooked_material_resource` (15.22 seconds for the GPU suite). The material test verifies exact GPU mip bytes for all three maps and real draw changes from normal, roughness(G), occlusion(R) and metalness(B). Invalid normal semantics and missing ORM sources retain the last visible material. The original base-colour compatibility checks remain passing. A stale test fixture expecting a zero metalness multiplier was corrected to match the exported unit multiplier.

`ClawMaterialResource::stageRuntime` shares the typed validation and GPU construction path with authoring, but publishes an immutable loaded generation without requiring a source catalog or compiler. The passing package contract constructs and draws a fresh three-map GPU material while the source directory is hidden, rather than relying only on the previously retained authoring material. It also rejects missing runtime resources and mismatched package modes. This proves the material-level source-free GPU path; it does not certify a source-free racing scene package. No full P2 completion is claimed: Editor/swatch/sample equivalence and visual acceptance remain to be proven.

### Colour transfer repair — 13 October 2026

A real mid-grey draw regression confirmed double colour decoding: tinted BGRA UNORM produced RGB 66/66/66 while identical BGRA sRGB produced 37/37/38. The native PNTC shader now uses the bound SRV's actual format to distinguish hardware sRGB decoding from encoded UNORM samples. Encoded albedo/emission uses the piecewise sRGB transfer; hardware-decoded samples are used directly. Colour and vertex tint decode independently. Alpha, normal and ORM remain data. The draw owns the transfer flags, and unchanged material submission preserves them to avoid forcing repeated constant-buffer uploads.

The rebuilt `WPGraphics.cooked_material_resource` suite passes in 14.04 seconds, including the prior multimap/source-free checks. With a 0.6 colour tint and the fixture's controlled lighting:

| Draw | BGRA UNORM | BGRA sRGB | BC1/BC3 sRGB |
|---|---|---|---|
| Mid-grey albedo RGB | 65/65/65 | 65/65/65 | 66/66/67 |
| Mid-grey emission RGB | 97/97/98 | 97/97/98 | 100/99/100 |

The compressed profiles remain within the declared four-code-value tolerance. Emission testing also exposed `Material::setTexture` requiring an authoring resource database for an already-created texture. That path now permits direct texture assignment without a database and no longer loads an unused `panel.png` resource; the source-free fixture exercises it. This repair covers ordinary/instanced PNTC albedo and emission. Terrain layer colour, environment maps, HDR/exposure and final output transfer still need their own calibration and regression evidence.

The fresh `artifacts/procedural/colour-transfer-20261013/car.bmp` was inspected after rebuilding. Capture runner and structural validation pass, with 58 replay inputs applied. The short WARP run reports 143.956 ms mean and 159.949 ms P95 over 29 frames. Ordinary generated UNORM materials retain their appearance with the corrected transfer; the controlled DDS tests establish the sRGB fix. The scene remains prototype quality, with flat venue/terrain treatment and visibly repeated foliage. This is a colour correctness increment, not an AAA visual acceptance result.

### Terrain and environment colour — 13 October 2026

A new production-terrain draw fixture reproduced double decoding with nonwhite tint: the same mid-grey texture gave red 37 through UNORM and 28 through sRGB. Terrain staging now derives each albedo layer's decode flag from its actual view format, independently of normal strength. Both planar and triplanar paths decode colour/tint consistently. The analytic fixture also exposed two-sided raster state flipping the terrain's upward lighting normal into the ground; heightfield lighting now preserves the authored surface normal. Snow tint follows the same sRGB decode convention.

`ClawCubemap` accepts encoded RGBA/BGRA8 UNORM/sRGB faces and linear RGBA16F/32F faces, at the selected 2D SRV mip, up to 2048 pixels per dimension. Float radiance is preserved above one through the GGX roughness levels; encoded byte faces use the piecewise sRGB transfer. Unsupported views, foreign devices, nonfinite/negative source radiance and nonfinite filtered output fail before replacing the previous cube. Context/device identity is checked, including the cached-view path.

Fresh Debug targets build. `WPGraphics.cooked_material_resource` passes in 14.20 seconds, including matching planar/triplanar UNORM/sRGB draws with nonwhite tint and the expected lighting result. `WPGraphics.cubemap_pbr` passes after the selected-mip test in 10.76 seconds: orientation, filtering, caching, PBR draw, encoded sRGB equivalence, half/full-float radiance (2/.5/.125 at smooth and rough levels), selected source mip and invalid-source retention. The racing sample still generates 8-bit reflection source faces; this increment proves native HDR cubemap input, not an HDR racing environment or final exposure/lighting calibration.

The rebuilt `artifacts/procedural/terrain-environment-20261013/car.bmp` was inspected. Capture/structural checks pass; WARP mean/P95 are 151.517/176.166 ms over 29 frames. This view remains visibly prototype quality. The sample's foliage carrier is all-hole terrain and its meadow is a separate mesh, so the dedicated production-terrain GPU fixture supplies terrain lighting evidence. The sample's native reflection input still traverses `ClawTextureManager::createManual`/`copyData`, which currently assumes 8-bit storage and ignores the requested manual format. A typed float upload path is required before claiming HDR radiance reaches the racing reflection probe.

### HDR sample bridge — 13 October 2026

`ITexture::copyLinearFloatData` adds an explicit, non-pure backend capability for RGBA32F linear data; unsupported backends return a diagnostic. Claw validates dimensions, exact pixel count, finite/nonnegative RGB and alpha in [0,1] before staging a candidate. It retains float pixels for GPU reupload and unload/load, rejects lossy BGRA8 snapshots, and clears obsolete representations when switching storage. GPU allocation/upload failure retains the previous view. This path stages one float mip; the cubemap prefilter owns the probe's roughness levels. The existing manual-format argument remains a separate unsupported-format issue.

The shared racing builder now supplies linear float reflection faces and retains an unclipped solar core; the visible sky remains LDR. Capture evidence reads back the actual current-device RGBA32F probe, reporting dimensions, mip count, finite values and peak radiance. A capture requires radiance above one to pass its new HDR validation. This is a deliberately authored sun/sky model, not measured photometric calibration or an HDR display/output pipeline.

The first Debug build passed both GPU suites (cubemap/float texture 12.36 seconds; cooked material/terrain 14.17 seconds). Tests verify exact values up to 12, malformed-input retention and unload/load reupload; linear float terrain colour also matches the encoded UNORM/sRGB paths. The new interface method has since been moved to the end of the existing virtual texture methods to preserve their ordering. The coordinated Debug rebuild of native targets, Lua bindings and Editor succeeds. The final cubemap/float-texture suite passes in 12.70 seconds and the material/terrain suite in 14.33 seconds. An Editor build is not interactive workflow acceptance.

`artifacts/procedural/hdr-sample-20261013/car.bmp` was inspected; the capture runner and structural/HDR checks pass. GPU readback confirms a current-device 128×128 RGBA32F probe with eight mips, finite values and peak linear radiance 16.5446. This establishes the float source → prefilter → sample probe path above the former 8-bit clamp. WARP mean/P95 are 156.207/166.644 ms over 29 frames, not a hardware performance pass. The scene remains prototype quality: the new dynamic range does not close exposure, material response, foliage shape or venue composition gates.

The rebuilt sample capture at `artifacts/procedural/pbr-runtime-20261013/car.bmp` was inspected. Native frame, geometry/LOD and foliage checks pass; the report records 58 applied replay ticks and no failed foliage pages. Mean/P95 intervals are 141.912/169.678 ms over 29 frames on Microsoft Basic Render Driver/WARP. The sample still uses its direct generated-material path; the separate GPU contract proves cooked/package rendering. The image remains prototype quality and does not demonstrate commercial-reference parity or a hardware performance pass.

## Broadleaf silhouette and venue structure

Native broadleaf generation now uses a segmented tapered leader, rising primary/secondary forks and densely filled crowns of small leaf sprays with varied pitch/yaw and rounded lighting normals. The three LODs retain stable cluster subsets and reduce topology. Bark/leaf material sections, authored wind and bounds across all LODs remain supported; grass and groundcover generation retain their previous topology. Default near/mid/far tree budgets are 1,376/492/204 triangles; maximum 128-branch near trees remain below 28,000 vertices and 14,000 triangles. These are geometry budgets, not performance acceptance.

The sample start gantry uses open galvanized chords and diagonal braces instead of solid dark columns. Existing simple collision envelopes remain in place. Non-preview distant terrain sampling increases from 32 to 96 divisions without changing the flat racing/foliage contact footprint.

`SampleVehicleAdvanced` and `ProceduralServiceTests` rebuilt successfully; `WPProcedural.services` passes (1.26 s). Both fresh `artifacts/procedural/venue-foliage-20261013` car/corner captures pass structural and HDR validation and were inspected. Fuller canopies and the open truss are visible, but the image remains prototype quality: crowns are still too regular, venue/ground treatment is sparse, vehicle design/materials need more work, and an angular sky band needs investigation. WARP mean/P95 intervals are 165.669/183.334 ms for car and 158.293/184.614 ms for corner over 29 frames each. Car geometry submission is about 230,030 triangles per frame, reflecting the added crown detail and terrain grid; this is not a hardware performance pass.

`WPFoliageAuthoringTests` is now an independently buildable CTest target, reusing the existing authoring contracts and adding seeded geometry repeatability, finite unit normals, valid/nondegenerate triangles, complete material sections, all-LOD bounds, strict LOD reduction, maximum branch budgets and unchanged grass topology checks.

Both registered tests pass after a fresh build: `WPGraphics.foliage_plant_geometry` (0.47 s) and `WPGraphics.foliage_authoring` (5.20 s). The first full authoring run exceeded its 60 s timeout because the population persistence/concurrency fixture repeatedly serialized full-detail generated trees. That fixture now uses a small validated species mesh; full-detail/default/maximum recipes retain their separate geometry checks. The population edits, persistence, cancellation, mask/exclusion and concurrent read/write checks remain in the authoring run.

The angular background band was isolated by temporarily omitting only the distant terrain mesh (`artifacts/procedural/sky-isolation-20261013`). The grid/index/upload inspection found no evident corruption. The authored terrain remained raised at its square outer boundary, producing straight fog-coloured skyline edges. The restored mesh now retains the flat inner 300 m footprint, uses a lower, finer ridge profile and returns smoothly to flat ground at radius 600 m, before any square boundary. Fully flat inner/outer cells are omitted; the high-quality ridge submits 10,136 triangles instead of 18,432. The diagnostic omission is removed from the final source and rebuilt sample.

The final `artifacts/procedural/ridge-foliage-20261013/car.bmp` was inspected: the former large diagonal wedge is replaced by a low continuous ridge, with the new foliage/truss visible. Capture, structural and HDR checks pass; the probe retains finite peak radiance 16.5446. The 29-frame WARP run records 168.601 ms mean / 183.257 ms P95 and 213,438 submitted triangles per frame. It remains a short software-renderer diagnostic, not representative hardware or AAA acceptance. Final rebuilt service/authoring/plant checks pass 3/3 (1.44/5.34/0.47 s); `git diff --check` passes. Capture source hashing now includes the general scene/foliage sources, graphics interfaces and native C renderer paths as well as the previous sample/backend roots; both plant generator and native DX11 renderer hashes are present in the final manifest.

## Hero vehicle surfaces and cockpit

The vehicle aero skins now share vertices along chord and span, removing hard normal breaks at every wing strip. Separate perimeter faces retain crisp leading/trailing/side boundaries. All three LODs use curved span profiles with restrained gull shape, taper and twist. Body, sidepod and airbox lofts use bounded shape-preserving Hermite interpolation at near/mid detail; coarse LOD retains the authored stations. Nose tip width increases from 50 to 150 mm. Sidepods have broader rolled decks and a raised shallow inlet with a painted lip, recessed carbon walls and backing instead of a flat dark rectangle over a painted cap. The airbox has the same genuine recess construction. Smooth shoulder shaping also removes the former cross-section discontinuity at the horizontal seam.

The halo is a shared swept curved tube with a centre support in all three LODs, including presets without fine details. The cockpit floor is lower with internal walls to give the opening depth. Wheel hubs, tracks, tyre dimensions and physics pivots are unchanged. The body tail and airbox stations now remain ordered at the supported short/long wheelbase extremes instead of folding backward along Z. No driver, cockpit controls, steering animation upgrade or modern regulation compliance is implied by this increment.

`ProceduralServiceTests` and `SampleVehicleAdvanced` rebuild successfully. Extended service checks cover default/2 m/5 m wheelbases, finite orthonormal vertex frames, complete/nondegenerate triangles, bounds, strict LOD reductions, shared curved wing surfaces, recessed inlet depth, retained tyre/hub/axle dimensions, halo retention and exact default recipe replay. The rebuilt services and sample review contracts pass 2/2 (1.79/0.03 s). Both `hero-surfaces-20261013/car.bmp` and the subsequent `hero-cockpit-20261013/car.bmp` were inspected; the second iteration makes the inlet shallower and retains the curved halo. Final capture/structural/HDR-probe checks pass. WARP mean/P95 are 169.595/201.671 ms over 29 frames, with 229,910 submitted triangles per frame. Geometry detail increases; no performance improvement or AAA art acceptance is claimed. Vehicle proportions/material response, driver content, tyre/rim art, venue composition, ground density and foliage variation still need further work.

A source audit confirms the DX11 sample does not currently use the C HDR post pipeline: `ClawHammerSystem::beginGraphicsPipelineFrame` admits the software renderer only. Native PNTC lighting applies a fixed Reinhard tone map and gamma before direct DX11 presentation. The float reflection probe establishes radiance preservation above one, not adjustable exposure, an HDR display path, or GPU post-processing acceptance. The previously suspected double post-tone-map does not occur on this active DX11 path.

## Explicit sky-matched sunlight

`ClawScene` now accepts a nonvirtual primary directional-light preference. It holds a weak reference, validates current scene membership/type/visibility during selection and falls back to the existing first-visible directional light. Invalid requests preserve the previous preference; hiding permits fallback, removal and scene clear prevent stale retained lights from remaining active. The rendering path uses this same selection helper. No `IGraphicsScene` virtual interface change is required.

The racing builder always owns a sun derived from the procedural sky. The standalone sample selects it after scene generation; host light parameters remain unchanged. Capture validation checks the selected light against the owned sun and records direction/power plus `lightingOutputPath=dx11-fixed-reinhard` and `hdrPostPipelineActive=false` so floating-point reflections are not mistaken for an active GPU HDR post chain.

The registration tests exposed an existing initialization defect: `ClawLight` setters configured before its queued load had no native light/state storage and silently lost values. Scene light creation now initializes the CPU-only native C light before returning it, for both interface-ID and Claw class-ID paths. This introduces no GPU calls on the generation thread. Native light cloning likewise initializes before copying settings and now preserves position. Tests verify nondefault direction/colour/power before any render tick, clone configuration, normal fallback, preference, invalid foreign/generic/point lights, hidden/removed retained objects and clear.

The sample, procedural services, production renderer test and Editor build successfully. After fixture initialization/queue cleanup repairs, the final focused run passes 3/3: services 1.80 s, vehicle review 0.03 s, and `WPGraphics.claw_production_render` 6.17 s. The last suite includes existing GPU terrain/foliage/shadow contracts as well as the new CPU light-selection contracts. This is compiled Editor evidence, not an interactive Editor acceptance run. The production plan now makes active native DX11 linear scene output, one final tone map, display UI ordering and exposure/transfer/resize/device checks an explicit P6 backlog item.

The final `artifacts/procedural/hero-sun-20261013/car.bmp` was inspected and capture validation passes. Runtime evidence reports the selected sky-matched sun at power 1.8 and direction approximately `(0.006285, -0.761013, 0.648706)`; the changed shadow direction is visible. The output-path report confirms fixed Reinhard and no active HDR post pipeline. WARP mean/P95 are 167.355/189.251 ms over 29 frames, with 234,426 submitted triangles per frame. These short diagnostic results do not close hardware performance, driver content, advanced material response, temporal quality or independent visual comparison gates. The scene remains visibly prototype quality.

## Bounded city generation revision 2 — 24 October 2026

The shared `OpenCityLayout` now gives parcels stable signed-cell IDs, independent seed streams, coherent commercial/residential/civic districts, street frontage and parcel envelopes. Four parcels per block retain an 11 m road-centre setback and a 4 m central service gap. Route choice does not reshuffle the lots; expanding the grid preserves common parcel identities and geometry. One-to-three-tier building volumes are shared by the scene's rendered massing and its static collision boxes. Existing six-field lot aggregates remain valid, and new buffers retain source-budget accounting.

City façades use individual near glazing quads, floor bands, corner piers, entrance fascias and roof details. Quads replace the old six-face box per window. Near/far batches live in 128 m patches selected by the existing `LODGroup`/`LODSystem`, with hysteresis and a coarse tier retaining massing, roof silhouette and glazing ribbons. Facade materials share complete generated concrete PBR maps and use world-box projection; the old XZ projection collapsed vertical wall texture coordinates. The first visual review caught an incomplete low-level concrete bake with a metallic fallback; the corrected scene uses `bakeSurface(Concrete)` and requires albedo, normal and ORM outputs. A surrounding landscape surface provides context outside the bounded driving area.

`OpenCityStreetDetails` partitions flat 15 cm sidewalks, junction corners and plaza contact surfaces without the old slab overlap/height seams. The rendered pieces and compact collision union cover the same area. Interior block surfaces merge into one collider each, while outside pieces remain separate so road mouths stay open. Six/eight/ten-block cities use 112/164/224 pavement collision boxes instead of 400/676/1024. This repair follows a real ten-block run that exhausted the scene-root actor capacity; global capacities were not increased. Lamps and decorative bollards use bounded seeded placement with road/junction clearance; lamp posts have collision proxies. Lane markings sit 6 mm above the flat road surface and their winding is normalized upward. Park trees use parcel coordinates at turf grade, distribute across all parks under the existing candidate budget, and use 128 m city pages with a bounded overview visibility range. Circuit foliage policy is unchanged.

The native sample now accepts `--city --city-blocks 6|8|10 --city-route 0|1|2`. Street and city-overview cameras use cached layout dimensions, with a city-sized far clip. The capture runner accepts matching scene options and inventories the procedural interface headers as source inputs. City evidence checks route closure/alignment, footprint clearance, exact building-mass collision actor counts and one active level per city patch. The existing collision smoke mode supports city native shape/pose probes plus road and sidewalk height rays; it does not claim a complete driving lap.

The sample and focused tests rebuild successfully. The latest layout/street tests pass 2/2 (0.57/0.35 s), including parcel/mass nonoverlap, replay/expansion stability, connected routes, bounds, pavement-union equivalence and street furniture. The mocked Lua city workflow passes, and four invalid native CLI combinations exit with the required argument error. Before collider consolidation, the six-block native smoke passed with seed 7: 793 attached static boxes, 225 building masses, road Y=0 and sidewalk Y=0.15.

The first street/overview capture set (`city-scene-20261024-01`, seed 42, six blocks, high) passes native structural/HDR checks. It contains 144 parcels, 125 building lots, 19 parks, 216 building collision volumes and 36 city LOD groups. Street selects 29 near/7 far groups; overview selects 0 near/36 far. Its WARP mean/P95 intervals are 282.224/356.625 ms for street (29 frames) and 79.292/94.270 ms for overview (38 frames). These images were inspected and rejected as visual acceptance: the material issue above darkened façades, and the former tree range hid most parks in the overview. This is a retained diagnostic set, not the corrected art result or a hardware performance pass.

This advances the bounded city portion of P8. Curved/hierarchical road networks, graded or multi-level streets, streamed world sectors, authored architectural kits, interiors, traffic/pedestrians, full driving/Editor/package acceptance and independent visual comparison remain open.

After collision consolidation, the maximum-size native smoke exits successfully: seed 42, ten blocks, route 2, preview quality with foliage. It verifies 1,193 registered static boxes, including 439 building masses, with road Y=0 and sidewalk Y=0.15. The 2,880 m route is generated, but this test probes static shapes and grades rather than driving the route. The earlier failed maximum run remains in `procedural-city-max-physics.*.log`; the successful replacement is `procedural-city-max-compact.*.log`. The final compact street tests pass (0.35 s), alongside layout tests (0.57 s), and prove that collider consolidation preserves the original pavement/plaza union and open carriageways.

The corrected `artifacts/procedural/city-scene-20261024-02` street and overview captures both pass and the runner exits successfully. Both images were inspected. Façade colour/material families are readable and park foliage is now present across the overview. The overview has 19 resident pages and 220 tested/resident tree instances, with no pending or failed pages; its tree LOD counts are 0 near, 13 mid and 207 far. City building LODs remain 29 near/7 far at street level and 0 near/36 far in the overview. Native HDR output and city structural checks pass. Both new procedural headers are included in the source hash inventory.

On Microsoft Basic Render Driver/WARP at 1280×720, final street mean/P95 intervals are 221.145/226.078 ms over 29 frames (467,649 submitted triangles/frame); overview is 79.984/92.593 ms over 38 frames (407,637 triangles/frame). Different views, culling, shadows and tree coverage affect these counters; they are not an isolated LOD benchmark or representative hardware certification. The manifests explicitly record visual acceptance as not accepted. Internal review still finds flat dark glazing, repetitive box-tier architecture and four-parcel grids, sparse human-scale entrances/ground dressing, and a basic surrounding landscape. Calibrated glass/indirect lighting, authored district kits and collision-safe street/park detail are the next visual priorities. No commercial-reference comparison or AAA acceptance is claimed.

The old `Tools/cpp/Editor/src/procedural/ProceduralTextureBindings` source is dormant, excluded from the Editor target and uses outdated texture APIs; its export call was updated to the shared encoder but native Editor generation/export integration remains unavailable. Mocked Lua editor tests do not remove that gap.

Open work includes general recipe identity/versioning and async/cancellable last-good scene publication; full generic geometry/CSG/UV tools; native texture-editor binding; full P2 workflow acceptance; road grading/elevation/junction production work; architectural kits and traffic/crowd/driver content; weather/night/water; GPU temporal quality and reflection work; sustained hardware performance/residency tests; full interactive Editor and source-free racing package acceptance; camera/replay recording; and independent blind comparison. No commercial-reference preference result is claimed.

## City lighting, frontage packing and editable transport foundation — 24 October 2026

The city now has three district facade kits: masonry housing, pale mineral civic/podium surfaces and contemporary office ribbons. Near walls are segmented around actual recessed apertures, with reveals, sills, mullions, closed entrances, canopies, parapets and bounded roof equipment. The stepped occupied building volumes and collision remain shared. Window panes have restrained seeded tint/roughness variation and occasional privacy glass. This is opaque glazing with outdoor HDR specular response; transmission and furnished interiors are not implemented.

City-only ambient/sun balance is now neutral `(0.55,0.57,0.60)`/power 3, facade micro-AO is reduced, and city materials share the existing HDR outdoor probe. Mineral weathering is restrained instead of applying the forge's full contrast to large panels. City fog uses scene-linear radiance in the native filmic path. Circuit lighting policy is retained. Street dressing adds batched park paths, benches, planters, bins, utility covers and paving joints with bounded solid collision proxies. Fresh images exposed dotted black seams and corrugated asphalt: joints were raised to a 6 mm bias, widened and softened, and city asphalt now uses seeded isotropic aggregate over an 8 m tile.

`--city-layout grid|lsystem`, capture `-CityLayout`, the native `City Natural Roads` property and the Lua Inspector select the bounded natural layout. A two-iteration branching grammar snaps/splits its planar graph around three shared closed trunk routes. Limits remain 160 nodes/240 edges; route selection does not reshuffle topology or parcels. Natural sidewalks are conservative rotated prisms checked against every road; acute/short segments can omit footways. Continuous pedestrian junction paving is still open.

The first natural capture exposed a sparse 31-parcel settlement and dense short road forks. Wider branching angles, fewer growth anchors and frontage-based sampling now produce 122 parcels, 112 building lots and 10 parks for seed 42/size 6, on 83 nodes/101 edges. Both sides of actual roads receive bounded candidates; complete envelopes retain an 11 m setback, 0.75 m separation and city bounds. Natural layout preparation moved to `OpenCityLayout.cpp` so future packing changes do not require modifying its public implementation. Seed-corpus size-6/8/10 packing and route independence pass; generated graph indices remain derived identity, not editable source GUIDs.

The rebuilt sample and four focused CTests pass: layout 11.89 s, street descriptors 0.36 s, road graph 0.42 s and spline source 0.13 s. Tests include planarity/connectivity, replay, full-envelope clearance, packing and bounds. The real Lua city fixture passes, including switching layout/record namespaces; this is mocked native/UI workflow evidence. Four invalid CLI combinations exit 2. The revised maximum-size native collision smoke exits 0: seed 42, ten blocks, route 2, preview quality with foliage, 1,015 registered static boxes and 385 building masses, road Y=0, sidewalk Y=0.15, route 1,473.2 m. It checks shapes/poses/rays, not a complete driving lap. The earlier natural smoke (589 boxes/125 masses) predates the packing revision and is retained separately.

Fresh capture sets were inspected and used to revise source. `city-lighting-20261024-03` caught uniform glazing, pavement aliasing and asphalt ruts. `city-natural-20261024-04` caught sparse packing and the gray background slab. Final `city-detail-20261024-05` and `city-frontage-20261024-06` runners exit 0 and all views pass city, foliage and native HDR checks. Both new out-of-line layout/spline sources are present in the final source-hash inventory.

| Final set/view, 1280×720 Debug WARP | Frames | Mean/P95 interval ms | Submitted triangles/frame | Draws/frame |
|---|---:|---:|---:|---:|
| `city-detail-20261024-05/street` | 29 | 206.414 / 214.669 | 739,501 | 389 |
| `city-frontage-20261024-06/street` | 29 | 206.022 / 214.518 | 512,711 | 238 |
| `city-frontage-20261024-06/track` | 47 | 64.540 / 70.227 | 221,917 | 241 |

The final natural street selects 12 near/10 far building patches; overview selects 0 near/22 far. Both have ready foliage with 220 resident tree instances. These different views/layouts are not an isolated LOD or hardware performance benchmark. Review finds substantially clearer street frontages and neighbourhood packing, but opaque glass, repeated silhouettes, aliasing, awkward road forks, incomplete junction walks and untreated land-use gaps still read as a prototype. Manifests record visual acceptance as **not accepted**, with no commercial comparison or hardware certification.

`CityRoadSplineSource` adds a strict version-1 GUID/JSON source and editable relative Bezier handles, bounded adaptive sampling, Fixed/Drape terrain callbacks, centre/edge grade/crossfall and local fold rejection. Invalid input and allocation denial retain previous outputs. RapidJSON scratch allocations use operation-owned SourceCpu tracking; a headroom denial sweep verifies exact baseline residency, including abandoned parse children. The [hand-editable example](examples/city-road-source-v1.cityroads.json) round-trips through the real codec test. Terrain fitting is point-sampled and cannot prove continuous hole/cell coverage. This CPU foundation does not yet drive Editor handles, scene regeneration, terrain carving or native road collision.

[The authoring plan](WPPROCEDURAL_CITY_AUTHORING_PLAN.md) specifies the existing Editor viewport/Undo/asset paths and last-good terrain/city publication work. [The transport and water plan](WPPROCEDURAL_CITY_TRANSPORT_WATER_PLAN.md) now covers editable rivers, road/rail bridges, train tracks and first-class roundabouts, including terrain support, lane-aware island-avoiding routes and 3D surface queries. Those city runtime features remain proposed. Native drawing/terrain integration, real Editor acceptance, source-free packaging, representative hardware and independent blind comparison remain open.

## Terrain road authoring and single-lane roundabouts — 24 October 2026

This increment implements the first native authoring tier described in the [city authoring plan](WPPROCEDURAL_CITY_AUTHORING_PLAN.md). **Window > City Roads** provides terrain binding, click/drag Bezier drawing, knot/handle editing, road fields, draft commit/cancel, Undo/Redo and scene saving. One scene-owned `CityRoadAuthoring` component retains canonical source and a persistent terrain-component UUID. Version 1 remains compatible; strict version 2 adds empty documents and typed roundabouts. Project asset/catalog/importer ownership is still planned, rather than a second editable copy of the scene source. The existing generated grid/L-system city retains its separate flat-support path.

`CityRoadTerrainMesh` clips the entire carriageway footprint against the actual terrain cell triangles. It supports translated, yawed and positively scaled terrain, rejects holes and unsupported coverage, and checks full-cell grades. Fixed profiles cannot float above the terrain. Roundabouts support three to six ports, a real annulus around a raised island/kerb, derived splitter refuges and directed entry/exit routes. Referenced splines must join the solved approach cross-section in position, width, tangent and height; a matching GUID alone cannot admit a disconnected road. The Editor places four connected approaches and conservatively rejects nonplanar junction support. Terrain cut/fill and swept-path approval are unfinished.

Native publication prepares mesh and physics inventories before swapping them under the existing render/physics locks. The same fitted road/island/kerb geometry drives rendering, native mesh collision and driveable-surface queries. Markings are clipped/projected onto the prepared road triangles with a 5 mm visual lift, including terrain ridges; they have no collider. Generated actors are excluded from scene serialization. Failed source edits retain the previous publication. If a changed terrain can no longer support the roads, the component removes stale road collision while preserving source intent and an error. This is safe deactivation, not fully atomic terrain/city replacement or proven GPU-upload failure recovery.

The Debug sample, Editor and four CPU targets build. All five integrated CTests pass in 20.13 seconds: source compatibility/identity, roundabout topology/routes, exact terrain fitting, component revision/lifetime behavior and native Editor viewport authoring. Coverage includes allocation denial and retained outputs, transformed terrain, between-sample holes/ridges, strict joins and terrain-refresh deactivation/recovery. The Editor fixture retains its publication adapter: it verifies Render-delivered controls, owner-task input, consecutive roads, handles after a 5 m terrain rise, multi-step undo/redo, rejected-history cursor preservation, atomic junction deletion, hide cancellation and scene serialization. It does not certify a live Editor session or native GPU publication. It caught the engine's distinct `Parameter` C-string/String storage: city phase producers now construct explicit `String` values.

Four fresh `SampleVehicleAdvanced --road-authoring-showcase` street/overview capture iterations were inspected. The first established native terrain and road collision; the second added markings, splitter refuges and stronger terrain relief; the third moves the close inspection camera above the junction; the fourth refreshes evidence after rebuilding the final graphics library. Final [street](../artifacts/procedural/city-road-authoring-20261024-04/street.bmp) and [overview](../artifacts/procedural/city-road-authoring-20261024-04/track.bmp) captures both pass native structural/HDR checks, four native road/ring/island/terrain ray probes, island exclusion, circulation direction and route/surface height agreement. The fixture contains four splines, one roundabout, 9,409 terrain samples and 24,561 runtime triangles. The final manifest inventories 1,235 sources, including the native ImGui path, and 11 binaries.

At 1280×720 on Microsoft Basic Render Driver/WARP, the final close view records 50 frames with mean/P95 intervals 60.507/87.944 ms; overview records 78 frames at 39.076/55.184 ms. These short software runs are diagnostic. The manifests explicitly record visual acceptance as **not accepted**: asphalt/terrain shading is simple, surroundings are empty and thin distant lines alias. No representative hardware result, full driving lap, commercial blind comparison or AAA parity is claimed. Rivers, bridges, rail, arbitrary junction editing, generated-city source conversion and production art remain open.

The Debug executable now opens the ordinary Editor when launched without arguments or with `--interactive`; explicit Boost arguments retain the CTest runner. Development startup resolves relative plugin/media paths from the executable directory. A live native window was exposed, but Windows capture failed with `FrameArrived timed out`/`window capture timed out`; a native accessibility read exposed title-bar controls only, and input reported `coordinate input geometry is unavailable`. The same limitation occurred with twelve and zero workers, so no threaded-render defect is established. Both owned instances were stopped and the ordinary worker policy restored. [The recorded attempt](../artifacts/procedural/city-road-authoring-editor-20261024-attempt/attempt.json) explicitly leaves mouse-driven draw/edit/undo/reopen acceptance unverified. The final control pass also gives newly drawn roads the same 25 mm terrain surface bias as the native showcase, preventing coplanar road/terrain surfaces.

## Vehicle asset memory ownership (25 October 2026)

Nested vehicle geometry/LOD, physics mass/gearing, appearance/validation/issue storage and direct owning heap bodies now use the engine's shared SourceCpu budget. Complete copies stage before publication; procedural scratch, actual effects output and component/manager inventories preserve admitted spare capacity and last-good state under denial. CVehicleManager retains its custom SharedPtr APIs with admitted typed body/control storage. Existing five-preset geometry/physics outputs are preserved; matching C++ callers/plugins must rebuild for allocator/layout changes. Detailed ownership and remaining city/road/terrain allocation families are recorded in SCENE_WORLD_STREAMING.md and WPMATERIAL_IMPLEMENTATION_STATUS.md.

Validation passes **28/28 selected RelWithDebInfo CTest groups (95.19 seconds)** with **130 actor/HLOD cases**, including 31 budget/playback cases. The five new vehicle cases also pass in Debug. Real service tests cover effects RNG/output rollback, exact foreign-capacity admission, large-output scratch and full-cap warm/quiet paths. Frozen runtime/configuration and scoped vehicle snapshots have zero hash drift. This extends automated memory ownership coverage and does not change the visual/hardware acceptance boundaries recorded above.
