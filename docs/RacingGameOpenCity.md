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
park lots use the same deterministic CPU layout. Roads and intersections reuse
WPProcedural's IRoadSystem. Facades are batched by material; each building has a
static collision box. Road rendering uses a flat surface matching the existing
vehicle collision plane. City boundaries prevent driving off that plane.

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
- Native rendering and physics need a live Editor play session for visual validation.
