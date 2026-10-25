# Open City Racing

Attach `RacingGameOpenCity` to a Script component on an empty actor. Enable updates
in play mode. Press **Generate** in the Inspector to preview, then enter Play.
Build the full Windows solution with WPProcedural and WPVehiclePhysics enabled.
An older native build reports missing Open City support instead of generating a circuit.

The main menu offers free driving or city races. City races offer three routes,
a lap challenge, and unlimited time trials. Follow the cyan arrows through the
street intersections; the shared race system requires ordered progress and rejects
shortcuts and reverse laps. Pause a time trial and select Finish time trial to see
results. Free driving awards no laps or records.

W/S or arrows drive, A/D steer, C changes camera, mouse wheel zooms, R returns to
the city start or restarts the race, and Escape pauses. Pause menus can switch
between free driving and races. Changes to city seed, size, route or appearance
apply when starting the next drive. Retries reuse generated assets.

Inspector options: Seed, Appearance Quality (0-3), Laps (1-10), City Blocks
(6, 8 or 10 per side), City Route (0-2), City Natural Roads, and Generate. Grid streets are 88-104 metres
apart according to seed. Dense central buildings, quieter outer districts and
park lots use the same deterministic CPU layout. Layout revision 2 gives parcels
stable IDs, street frontages and coherent commercial, residential and civic
districts. Building footprints retain an 11 metre road-centre setback and a
4 metre service gap through the block centre. Road and route coordinates remain
independent of appearance quality and route selection does not reshuffle parcels.

**City Natural Roads** selects a bounded seeded L-system instead of the grid.
Rewritten turtle branches connect to three shared trunk circuits; snapped crossings
split graph edges. The graph is capped at 160 nodes/240 edges. Full footprint-to-road
distance tests keep buildings and parks outside the 11 metre buffer. Route selection
changes the chosen circuit without reshuffling the graph or parcels. Buildings remain
axis aligned in this first kit. Acute/short segments can omit sidewalks where a safe
raised ribbon will not fit; continuous pedestrian junction paving is not yet implemented.

Grid roads and intersections reuse WPProcedural's IRoadSystem; natural streets
render the buffered graph with matching conservative sidewalk prisms. A shared flat city
street profile partitions the sidewalks and junction corners at a 15 cm grade;
the same box descriptors build their rendered surfaces and collision. Interior
block plazas meet that grade. Building geometry and collision consume identical
one-to-three-tier mass descriptions. Facades are grouped into 128 metre LOD
patches: nearby buildings use recessed glazing apertures, district-specific masonry,
mineral and office kits, mullions, entrances and canopies
and roof details, while distant buildings retain the same stepped massing and
simple glazing ribbons. Existing LODSystem selection and hysteresis manage the
patches. City boundaries remain inside the shared ground collision plane.

Street lamps and bollards use bounded deterministic placement with road and
junction clearance. Lamps have pole collision proxies; bollards are decorative
in this tier. With foliage enabled, the existing foliage world distributes its
bounded tree candidates across park parcels at the 16 cm turf surface, rather
than sampling only the first portion of the lot array.
Park paths, benches, planters, bins, utility covers and paving joints use batched
geometry. Solid props have bounded matching collision proxies; decorative overlays
remain close to the contact surface. Natural layouts retain parcel/park details and
omit grid-derived street decorations. Glazing is an opaque approximation with outdoor
HDR specular response; transmission and furnished interiors remain open.

## Native city showcase

`SampleVehicleAdvanced` retains its circuit default. Pass `--city`, optionally
`--city-blocks 6|8|10`, `--city-route 0|1|2` and `--city-layout grid|lsystem`, to use the same city generation
properties as the Lua/Editor workflow. `--view street` inspects a street frontage;
`--view boulevard` inspects the opposite street direction and `--view track` frames
the city overview. Natural street cameras follow an actual central road segment.
City-only flags require `--city` and
invalid values are rejected before scene startup.

After building the Debug sample, capture fresh runtime evidence with:

```powershell
& .\Tools\CaptureProceduralRacing.ps1 -Configuration Debug -Scene city `
    -CityLayout lsystem -CityBlocks 6 -CityRoute 0 -Quality high -Views street,track `
    -FoliageWorld -NoEffects -Seconds 3
```

The capture manifest records the scene mode and city options. City evidence
checks route closure/street alignment, footprint clearance, the exact shared
building-mass collision count and one active level per city LOD patch. These
structural checks do not certify image quality, drivability or hardware speed.

Preferences use `RacingGameOpenCity.records` in the project directory. Lap records
use separate `RacingGameOpenCity.<blocks>.<route>.records` files, keyed by city seed.
Appearance quality does not change the street layout or lap record identity.
Natural-layout records add `.lsystem` before `.records`, keeping grid records independent.
City size and route are Inspector/session options; seed, quality and laps persist.

This implementation is a bounded procedural city with solo racing and exploration.
Traffic, opponents, interiors and streamed world sectors are not implemented.

## Editable splines, terrain and connected transport

[The authoring plan](WPPROCEDURAL_CITY_AUTHORING_PLAN.md) documents the native
**Window > City Roads** tool: bind a selected terrain, click/drag Bezier knots and
handles, commit or cancel drafts, edit road fields, and undo/redo committed edits.
The scene's `CityRoadAuthoring` component owns canonical source and a persistent
terrain binding. Generated render and collision meshes are disposable caches.

`CityRoadSplineSource` retains strict version-1 compatibility and the
[hand-editable example](examples/city-road-source-v1.cityroads.json). Version 2 adds
empty documents and typed roundabouts with three to six connected approach ports,
raised islands/kerbs and directed circulation routes. The Editor placement tool
creates four approaches. Exact road fitting clips footprints against terrain cell
triangles, rejecting holes, unsupported coverage, excessive grades and disconnected
junction joins. Roads and junctions share their fitted render/collision geometry;
terrain edits that cannot be recooked deactivate stale road collision while retaining
the authored source. Terrain grading and fully atomic terrain/city replacement remain
unfinished.

Generated grid/L-system cities can now use the same editable terrain-road system.
Enable **City Terrain Roads** on `ProceduralRaceScene`, or pass
`--city-terrain-roads` to `SampleVehicleAdvanced`. This creates persistent
**Generated city terrain** and **Editable generated city roads** actors. Select the
road actor in **Window > City Roads** to edit the generated Bezier splines and
roundabouts. The component's **City Road Authoring UUID** retains that source
binding when the scene is saved. Regeneration reuses the edited document rather
than replacing it with the original seed.

`RacingGameOpenCity` exposes the same two script properties. Terrain-city
regeneration keeps the native vehicle, camera and source binding through quality,
seed and source-selection changes. Script property serialization also retains
the binding. Terrain-city lap records remain in memory for the current play
session and reset after regeneration or a detected route change. Persistent
records need a stable fingerprint of the edited route; the menu/HUD explains
this limitation. Flat-city record persistence is unchanged.

The conversion uses stable source/road/knot/junction IDs, coalesces collinear
streets, and replaces selected junctions with directed roundabout routes. Roads
follow the real terrain triangles; unsupported parcels are removed and surviving
building foundations, collision, parks and foliage share sampled ground heights.
Source or terrain changes trigger a rebuild of the derived city. Moving a road
away from the pinned racing route disables racing until the route has continuous
supported coverage again; free editing does not silently create a replacement lap.

Rebuilding completes the terrain/source and generated-collider lifecycle before
driving resumes. During scene reconstruction or before the vehicle is attached,
physics readiness stays false and activation retries on the Application task.
Removed or transferred actors cannot be reactivated by a stale queued load;
valid children retain activation through their live scene-owned parent hierarchy.

Terrain-city streets also have fitted 2.4 m pedestrian strips and roundabout-edge
walks, 15 cm above terrain, with concrete kerbs and shared mesh collision. Road
mouths and unsupported terrain pieces stay open. Splitter refuges retain their
raised concrete rims and inset turf. `--view boulevard` places the terrain-city
camera beside an approach at pedestrian height; `street` retains the elevated
junction inspection view.

```powershell
& .\Tools\CaptureProceduralRacing.ps1 -Configuration Debug -CityTerrainRoads `
    -CityLayout lsystem -CityBlocks 6 -Quality high -Views street,track `
    -FoliageWorld -NoEffects -Seconds 3
```

For repeatable captures while other configurations are building, the harness
also accepts `-BinaryDirectory <retained-build-folder>`. Retain the sample and
all its plugin DLLs together, and set that folder's `wp_plugins_samples.cfg`
`PluginFolder` to the same absolute directory. The manifest records those
binary hashes; media still resolves from the normal configuration directory.
This does not certify a source-free package or the freshness of retained files.

The flat city remains the default. Once a scene has saved terrain/source anchors,
switching that scene back to flat mode is explicitly rejected; use a separate
scene for the flat generator. Terrain-city source material export is not yet
supported. Arbitrary topology edits, automatic cut/fill, ordinary junction
reconstruction and atomic terrain/city replacement remain unfinished.

The separate native authoring showcase remains useful as a small isolated fixture:

```powershell
& .\Tools\CaptureProceduralRacing.ps1 -Configuration Debug -Scene city `
    -RoadAuthoringShowcase -Quality high -Views street,track -NoEffects -Seconds 3
```

`--road-authoring-showcase` also selects this mode directly in `SampleVehicleAdvanced`.
Evidence checks terrain publication, native road/ring/island/terrain ray hits,
island exclusion from driveable surfaces and route circulation/height agreement.
This fixture is independent of the generated-city conversion above.
[Rivers, road/rail bridges and train tracks](WPPROCEDURAL_CITY_TRANSPORT_WATER_PLAN.md),
project asset import and country-specific junction approval remain planned.

Validation:

- `luavm.exe Tests/lua/RacingGameOpenCityTests.lua` exercises the real Lua game
  against the existing native/UI fixture, including preview, Play, free drive,
  races, pause, route records, regeneration, time trial completion and cleanup.
- `Tests/cpp/OpenCityLayoutTests.cpp` is a standalone C++17 test of layout
  determinism, building clearance, route continuity, street alignment and bounds.
- `Tests/cpp/OpenCityStreetDetailsTests.cpp` checks the shared sidewalk/corner
  partition, flat grade, furniture clearance, bounds and deterministic placement.
- `Tests/cpp/OpenCityRoadNetworkTests.cpp` checks planar connected graph topology,
  bounds, replay, route independence and closed sampled routes across the seed corpus.
- `Tests/cpp/CityRoadSplineSourceTests.cpp` checks the real JSON example, retained edits,
  version-1/version-2 compatibility, bounded Bezier sampling and allocation-denial cleanup.
- `OpenCityRoadSourceTests` checks generated-source identity, roundabout approach
  clearances and continuous island-free routes across grid/natural seeds and sizes.
  `CityRoadParcelSupportTests` checks whole-parcel support and road exclusions;
  `CityRoadFootwaysTests` checks open junctions, terrain cells/holes, shifted oblique
  seams, maximum cities and bounded transactional allocation failures.

The 25 October native integration checkpoint retains road width/control IDs through
four generations, a terrain rebind and a terrain-only refresh, then verifies actual
static boxes, building masses, road/terrain rays, the fitted footway and uploaded
PBR maps before a clean exit. Fresh grid street, natural pedestrian and natural
overview captures use the same retained Debug binaries. See the
[evidence and remaining gates](WPPROCEDURAL_IMPLEMENTATION_STATUS.md).
These software-renderer captures remain prototype art; recorded Editor pointer
interaction, representative hardware performance and commercial visual acceptance
are separate gates.
- `CityRoadRoundaboutTests`, `CityRoadTerrainMeshTests` and `CityRoadAuthoringTests`
  cover junction topology/routes, exact terrain coverage and component revision/lifetime
  behavior. `Editor.city_road_viewport_authoring_test` exercises native controls and
  queued viewport rays with a test publication adapter; it is not a live visual check.
- The native sample's `--collision-smoke-test` supports `--city`: it checks native
  static shape attachment/poses and short face probes, the exact building-mass
  count, a road ray at Y=0 and a sidewalk ray at Y=0.15. It exits after those
  checks; the circuit mode retains its separate garage-impact test. This city
  probe does not certify a complete driving lap or vehicle handling.
- With `--city-terrain-roads`, the smoke additionally exercises retained manual
  road edits, terrain rebinding and a same-source-revision terrain refresh. It
  checks native road/ring/island/ground rays, fitted footway collision, uploaded
  road PBR bindings and the actual terrain-supported building masses.
- Native sample captures and live Editor play sessions remain separate acceptance
  checks; a native build or CPU test does not establish either result.
