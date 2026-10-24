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
(6, 8 or 10 per side), City Route (0-2), and Generate. Streets are 88-104 metres
apart according to seed. Dense central buildings, quieter outer districts and
park lots use the same deterministic CPU layout. Layout revision 2 gives parcels
stable IDs, street frontages and coherent commercial, residential and civic
districts. Building footprints retain an 11 metre road-centre setback and a
4 metre service gap through the block centre. Road and route coordinates remain
independent of appearance quality and route selection does not reshuffle parcels.

Roads and intersections reuse WPProcedural's IRoadSystem. A shared flat city
street profile partitions the sidewalks and junction corners at a 15 cm grade;
the same box descriptors build their rendered surfaces and collision. Interior
block plazas meet that grade. Building geometry and collision consume identical
one-to-three-tier mass descriptions. Facades are grouped into 128 metre LOD
patches: nearby buildings use individual glazing bays, frames, entrance fascias
and roof details, while distant buildings retain the same stepped massing and
simple glazing ribbons. Existing LODSystem selection and hysteresis manage the
patches. City boundaries remain inside the shared ground collision plane.

Street lamps and bollards use bounded deterministic placement with road and
junction clearance. Lamps have pole collision proxies; bollards are decorative
in this tier. With foliage enabled, the existing foliage world distributes its
bounded tree candidates across park parcels at the 16 cm turf surface, rather
than sampling only the first portion of the lot array.

## Native city showcase

`SampleVehicleAdvanced` retains its circuit default. Pass `--city`, optionally
`--city-blocks 6|8|10` and `--city-route 0|1|2`, to use the same city generation
properties as the Lua/Editor workflow. `--view street` inspects a street frontage;
`--view track` frames the city overview. City-only flags require `--city` and
invalid values are rejected before scene startup.

After building the Debug sample, capture fresh runtime evidence with:

```powershell
& .\Tools\CaptureProceduralRacing.ps1 -Configuration Debug -Scene city `
    -CityBlocks 6 -CityRoute 0 -Quality high -Views street,track `
    -FoliageWorld -NoEffects -Seconds 3
```

The capture manifest records the scene mode and city options. City evidence
checks route closure/street alignment, footprint clearance, the exact shared
building-mass collision count and one active level per city LOD patch. These
structural checks do not certify image quality, drivability or hardware speed.

Preferences use `RacingGameOpenCity.records` in the project directory. Lap records
use separate `RacingGameOpenCity.<blocks>.<route>.records` files, keyed by city seed.
Appearance quality does not change the street layout or lap record identity.
City size and route are Inspector/session options; seed, quality and laps persist.

This implementation is a bounded procedural city with solo racing and exploration.
Traffic, opponents, interiors and streamed world sectors are not implemented.

Validation:

- `luavm.exe Tests/lua/RacingGameOpenCityTests.lua` exercises the real Lua game
  against the existing native/UI fixture, including preview, Play, free drive,
  races, pause, route records, regeneration, time trial completion and cleanup.
- `Tests/cpp/OpenCityLayoutTests.cpp` is a standalone C++17 test of layout
  determinism, building clearance, route continuity, street alignment and bounds.
- `Tests/cpp/OpenCityStreetDetailsTests.cpp` checks the shared sidewalk/corner
  partition, flat grade, furniture clearance, bounds and deterministic placement.
- The native sample's `--collision-smoke-test` supports `--city`: it checks native
  static shape attachment/poses and short face probes, the exact building-mass
  count, a road ray at Y=0 and a sidewalk ray at Y=0.15. It exits after those
  checks; the circuit mode retains its separate garage-impact test. This city
  probe does not certify a complete driving lap or vehicle handling.
- Native sample captures and live Editor play sessions remain separate acceptance
  checks; a native build or CPU test does not establish either result.
