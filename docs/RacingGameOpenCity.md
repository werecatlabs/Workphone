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
(6, 8 or 10 per side), City Route (0-2), City Natural Roads, City Terrain Roads,
City Street Detail (0-100), City Building Style (0-9), five building mesh slots,
and Generate. Grid streets are 88-104 metres
apart according to seed. Dense central buildings, quieter outer districts and
park lots use the same deterministic CPU layout. Layout revision 4 retains
stable parcel IDs, street frontages and coherent commercial, residential and civic
districts. Grid building footprints retain an 11 metre road-centre setback and a
4 metre service gap through the block centre. Road and route coordinates remain
independent of appearance quality and route selection does not reshuffle parcels.

**City Natural Roads** selects a bounded seeded L-system instead of the grid.
Revision 4 gives the city four direct main approaches into its centre at the
origin. The 12 m main roads connect the outskirts to the centre; 9 m district
roads distribute traffic around the neighbourhoods, and 7 m local streets fill
the blocks. New local junctions on main roads require more spacing. Their outer
ends are intentional gateways for future expansion. This is a geometry and
connectivity hierarchy; it does not add traffic simulation or enforced speed limits.

Central building plots receive priority, producing a developed commercial core
before the remaining building budget fills the surrounding neighbourhoods.
Building footprints reserve 5 m beyond each road's actual carriageway edge for
pavement and frontage, so narrower streets have closer frontages. Roundabouts,
fitted sidewalks and collision use the actual editable road widths. Classes stay
separate during source conversion, even where consecutive streets are collinear.
Editing a connected road's width updates its roundabout approaches in the same
undoable change. Invalid widths are rejected without replacing the published city.

Local streets branch at near-right angles; growth stops at existing connections,
rejects cramped or acute junctions, and closes nearby dead ends where there is room.
Generated roads remain within the editable source's 128-road limit. Route selection
does not reshuffle the graph or parcels. Revision-3 pinned route coordinates remain
unchanged; natural-city lap records use a new revision namespace because road
widths changed. Historical files are preserved. Buildings remain axis aligned.
The flat preview renders straight graph spans; terrain cities promote that graph
into editable curved roads.

With **City Terrain Roads** enabled, natural streets use cubic Bezier splines.
Safe two-road corners share a tangent, while crossings and roundabout approaches
retain their attachment points. Bounded lateral alternatives are evaluated against
terrain grade, crossfall and profile smoothness. Main roads retain tight detour
limits so the approaches into the centre stay direct. Generated natural terrain
has a graded central junction district blending into broad rolling relief;
a supplied terrain is sampled without being reshaped and must satisfy the
roundabout solver's existing planar-approach fit requirements.
Roads, sidewalks, collision and the race route follow the canonical curved source.
Unsupported terrain or invalid geometry still fails validation.

Ordinary terrain-road T/X contacts are fitted as shared connection surfaces.
Approaches stop at matching ports; rounded corners use their actual road widths,
and pavement and edge markings follow the same boundary. Centre dashes stop at
the junction mouth. Moving the editable splines rebuilds these derived connections
without changing their saved knots. The C++ junction policy controls corner radius,
corner subdivision and maximum approach trimming; it does not add separate saved
intersection handles or a custom connection-mesh importer.
These connection surfaces rebuild on reload or regeneration from the retained
road source; existing authored roads do not need to be replaced.

Use **Window > City Roads** to select a generated spline, move its knots or
handles, insert knots, and apply width changes. Undo/redo and scene saving retain
the authored controls. Generation does not overwrite an existing road source.

To see the new layout, enable **City Natural Roads** on a new **ProceduralCityScene**
actor and press **Regenerate**. Enable **City Terrain Roads** for terrain fitting
and editable splines. Existing terrain-road sources remain canonical: regeneration
preserves hand edits rather than replacing their layout with the new generator.
When reopening a saved natural city, the builder can reuse its original race
route if that route still fits the edited roads. If neither the new nor original
route remains supported, racing is unavailable while free driving and road
editing remain available.

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

## Standalone generation components

Add **ProceduralCityScene** to an empty scene actor and press **Regenerate** to
build a city without a race vehicle. Set **City Terrain Roads** to create an
editable road source and terrain, or enter an existing **City Road Authoring
UUID** to reuse its bound terrain. The standalone city uses the same district,
building style, mesh slot, street detail, and route settings as the racing city.

Add **ProceduralCircuitScene** for a seeded circuit with scenery and collision.
The city and circuit components share seed, appearance quality, foliage, and
regeneration controls through `ProceduralScene`. They use the scene's lighting.
For a vehicle alone, use **ProceduralVehicle**. Use **ProceduralRaceScene** on a
vehicle actor for the integrated racing setup with driving, audio, and effects.
Existing race scenes continue to use their saved component and settings.

Generated scenery appears in a **Procedural scenery** group under the generating
actor. Regeneration replaces that group. Press **Regenerate** after
reopening a standalone flat city or circuit to rebuild it from its saved
settings. World scenery and its collision keep their seeded world positions
when the owner moves. Newly generated editable terrain and road-source actors
are saved under the generating actor and retain hand edits between builds.
When the city reuses an existing source, the source and terrain keep their
place in the hierarchy. The Lua open-city game saves new road and terrain
sources under its script actor so it can replace the car between sessions.

## Building styles and mesh slots

In the `ProceduralCityScene` Inspector, select **City Building Style** and
press **Regenerate**. In `ProceduralRaceScene`, also enable **Open City**.
The style controls the built-in façade modules and roof shapes used on the
city's building masses.

| Index | Style | Capture/CLI name | Built-in geometry |
| --- | --- | --- | --- |
| 0 | District Mix | `district` | Seeded building families distributed by district |
| 1 | Masonry Residential | `masonry` | Recessed punched windows, corner piers, hipped roofs, and chimneys |
| 2 | Civic Stone | `civic` | Deep stone surrounds, horizontal courses, and raised roof lanterns |
| 3 | Modern Office | `modern` | Glazing ribbons, narrow mullions, parapets, and rooftop service equipment |
| 4 | European Terrace | `terrace` | Projecting bay windows, framed entrances, canopies, and chimneys |
| 5 | Parisian Mansard | `parisian` | Mansard roof, dormers, cornices, and balconies |
| 6 | Warehouse Loft | `warehouse` | Brick pilasters, industrial glazing, loading entrance, and sawtooth skylight roof |
| 7 | Mediterranean | `mediterranean` | Plaster façades, shutters, hipped roof, and entrance pergola canopy |
| 8 | Art Deco | `artdeco` | Vertical piers, entrance fins, and stepped roof crown |
| 9 | Timber Gabled | `timber` | Timber framing and braces, gabled roof, and porch canopy |

Choose **District Mix** for variation within one city or a named preset for a
consistent architectural family. Existing saved style indices 0–3 retain their
names. The new styles use built-in meshes and need no imported architecture
assets. Appearance quality and distance control the finer façade details;
roof silhouettes remain visible in the far LOD. Changes retain the same
building masses, terrain support, and collision footprint.

Small details such as balcony bars, shutter slats, and raised window frames
share a fixed allowance across the city. Larger cities also space window bays
and mansard dormers more widely, with more detail retained on street-facing
elevations. Above 256 building lots, the fine-detail allowance is halved and
shared façade, paving, and soil textures use 256-pixel maps. Vehicle and
carriageway textures retain their own quality settings. Window sizes, storey
heights, and the main roof silhouettes remain unchanged. These limits keep
larger scenes within the existing memory budget.

Assign **City Wall Mesh**, **City Window Mesh**, **City Entrance Mesh**,
**City Corner Mesh**, or **City Roof Mesh** independently. Use **Browse** to
select a project mesh asset or enter a project-relative or absolute mesh path.
Clear a slot to restore the preset geometry. The style and slot selections
save with the scene. `RacingGameOpenCity` also accepts the style index and mesh
paths in its Script Inspector, where **Generate** applies them.

Use static triangle meshes with +Y up and +Z facing out on façades. The
generator fits the mesh bounds to the corresponding panel, aperture, corner,
or roof. A window module supplies the frame and uses the city's recessed glazing
behind it. Modules inherit the city's material for that part. The placement
budget shares custom geometry across buildings, starting at street-facing bays
on each building. Remaining placements use the built-in geometry.
Detailed appearance levels use the façade overrides in the near LOD. Preview
accepts corner and roof overrides. **City Building Kit Status** reports invalid
files, skipped roles, and placement limits. Failed placements use the preset.

For a native matched comparison, use **facade** with the same city seed and
layout. This example applies a sample arched window mesh to Civic Stone.

```powershell
& .\Tools\powershell\CaptureProceduralRacing.ps1 -Configuration Debug -CityTerrainRoads `
    -CityBlocks 6 -CityLayout grid -CityBuildingStyle civic -Views facade -Seconds 2 `
    -CityBuildingMeshes @{ window = '.\docs\examples\city-window-arch.obj' }
```

[Arched window mesh example](examples/city-window-arch.obj)

To compare the new built-in styles, clear the mesh slots and capture a named
preset without mesh overrides. The **building** view frames the façade and
roof of a representative low-rise residential parcel from above; **facade**
inspects street-level details. Each view keeps its parcel and camera fixed across
styles; the two views can select different parcels. The sample accepts the same names with
`--city-building-style`, for example `--city-building-style parisian`.

```powershell
& .\Tools\powershell\CaptureProceduralRacing.ps1 -Configuration Debug -CityTerrainRoads `
    -CityBlocks 6 -CityLayout grid -CityBuildingStyle parisian -Views building -Seconds 2
```

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
& .\Tools\powershell\CaptureProceduralRacing.ps1 -Configuration Debug -Scene city `
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
Flat natural-layout records add `.lsystem-v4` before `.records`, keeping grid and
historical natural-layout records independent. Authored terrain routes retain
session-only records because their geometry can change during editing.
City size and route are Inspector/session options; seed, quality and laps persist.

This implementation is a bounded procedural city with solo racing and exploration.
Traffic, opponents, interiors and streamed world sectors are not implemented.

## Editable splines, terrain and connected transport

[The authoring plan](WPPROCEDURAL_CITY_AUTHORING_PLAN.md) documents the native
**Window > City Roads** tool: bind a selected terrain, click/drag Bezier knots and
handles, click to select a road, insert a knot without changing the curve, edit
road fields, and undo/redo committed edits.
The scene's `CityRoadAuthoring` component owns canonical source and a persistent
terrain binding. Generated render and collision meshes are disposable caches.

With terrain roads active, **City Street Detail** adjusts the density of added
street fixtures on fitted sidewalks. The default is 100. Set it to 0 to hide these
fixtures. Press **Generate** to preview the new setting. Native captures accept
`--city-street-detail` or `-CityStreetDetail` with their terrain-road options.
Use `--view frontage` or `-Views frontage` for a pavement-height look along a
roundabout approach. The camera uses the road geometry, so density changes
can be compared from the same spot.

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
& .\Tools\powershell\CaptureProceduralRacing.ps1 -Configuration Debug -CityTerrainRoads `
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
& .\Tools\powershell\CaptureProceduralRacing.ps1 -Configuration Debug -Scene city `
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
