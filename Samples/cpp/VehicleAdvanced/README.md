# VehicleAdvanced

An independent procedural Grand Prix driving sample for WPGraphics, WPPhysics,
WPVehiclePhysics and WPProcedural. The original `SampleVehicle` remains available.

The [Lua Script component port](Lua.md) runs this scene inside the Editor.
Both versions share the reusable `scene::ProceduralRaceScene` game component.

The sample generates a detailed open-wheel car, livery and PBR maps, a seeded closed
circuit, continuous road and kerbs, runoff, barriers, garages, a start gantry,
batched trees and distant hills. Four wheel actors steer, rotate and move with
suspension. Broad slick tyres surround recessed spoke rims with raised lips and
centre hubs; brake discs follow their wheels. Visual steering is matched to the
brush tyre contact frame. WPVehiclePhysics owns chassis motion; the procedural dynamics service
is not advanced as a second simulation.

## Build and run

From the repository root, with the existing Windows x64 configuration:

```powershell
cmake --build project_x64 --config RelWithDebInfo --target SampleVehicleAdvanced
Set-Location Bin/windows/v145/x64/MD/RelWithDebInfo
./SampleVehicleAdvanced.exe --seed 7 --quality high
```

Run from the binary directory so the shared plugin configuration and `Bin/Media`
resources are available. WPProcedural is linked and registered explicitly;
WPVehiclePhysics is loaded when absent from the shared configuration. `--plugins
PATH` selects another configuration, with exactly one physics backend.

- W / Up: throttle
- S / Down: brake
- A / D / Left / Right: steer, with reduced steering lock at speed
- R: reset chassis, wheels, velocities, controller and lap progress
- Mouse wheel: chase-camera zoom
- Escape: quit

The HUD shows speed, gear, engine RPM, current lap, last/best times, seed and
circuit length. Quarter-lap checkpoints must be visited on the road before a lap
can complete. Brake input brightens the rear rain light. Leaving the kerbs reduces tyre grip
on gravel and grass.

`--seed N` reproduces the track and vehicle appearance. `--quality low|medium|high`
sets texture budgets and scenery density: low uses the middle vehicle LOD and
256-pixel map caps; medium uses the detailed mesh and 512-pixel caps; high uses
1024-pixel caps. Repeated scenery is batched into a few meshes, with shared
materials and textures. Startup logs report generation time, triangles and
uploaded texture bytes.

The scene includes a **Vehicle Reflection Cubemap** actor with a `Cubemap` component
in Custom mode. Its generated outdoor environment combines the procedural sky
with the start straight and grass in the lower hemisphere. `ClawCubemapTexture`
converts those six images into a linear cube texture with GGX-filtered roughness
levels. The paint, carbon, wheel rims and other vehicle materials bind this texture
in `PBSM_REFLECTION`; scenery continues to use the sky environment. This probe is
a generated static environment, not a realtime capture of track objects.

In the Editor, select the cubemap actor or its `Cubemap` component and scroll
the Properties pane to **Cubemap Texture Preview**. The six labelled thumbnails
show the source faces before roughness filtering. The preview refreshes when the
probe's texture changes; missing textures and unsupported face previews have an
explicit status message.

The sky images use top-to-bottom UV orientation. Material cubemap radiance is
independent of diffuse ambient brightness, and normal variance filtering reduces
sparkling specular highlights on the carbon weave. Capture runs also validate the
cubemap actor, native texture, every vehicle material binding and area-weighted
triangle winding against each generated mesh section's normals before reporting
success.

## Checks and captures

For a generated terrain city, `--city-terrain-roads --city-layout lsystem
--city-terrain-relief 1 --view hillside` selects a sustained road climb. Relief
accepts 0–2 and affects newly created terrain; regenerating a retained city
preserves its edited terrain. The hillside camera is cached for the capture and
its road ID, selected rise, eye and target are written to the evidence JSON.
Terrain cities on Claw automatically include paged park, street and hillside
vegetation. `--foliage-world` selects paged trees for circuits and cities without
terrain roads.

Paged trees and shrubs use detailed geometry, reduced geometry, then a
two-triangle imposter selected from 25 captured directions (eight azimuths at
three elevations, plus overhead). The generated albedo, normal and material
atlases support the existing lighting and isolated cutout mip filtering. Far
shadows retain reduced mesh geometry. Grass uses three progressively smaller
clump meshes with stable blade roots, stable density subsets and distance fade.
The render camera controls these levels, including its LOD bias and orthographic
direction; each camera retains independent transition history.

`--foliage-lod auto|0|1|2` controls paged foliage independently of the car and
legacy tree actors. `--foliage-lod-smoke-test` captures all three forced levels
and automatic selection from one fixed camera, checks decreasing submitted
triangles and unchanged source population, then restores the requested setting.
For a moving-camera review, use `--foliage-motion-capture --orbit --benchmark 10`
with a terrain city and a capture path. It saves six frames along the active
camera's outward path plus a CSV of camera positions and native LOD counts.
The capture harness exposes `-FoliageLOD` and `-FoliageLODSmokeTest`. These controls
cover the paged population; merged ornamental planting meshes retain their
existing scene/streaming lifetimes.

The city scene component exposes Tree, Shrub and Grass LOD0/LOD1 Distance in
metres in the Inspector. LOD0 marks the detailed-to-reduced transition and LOD1
marks the reduced-to-far transition. Edit the values and press **Regenerate**;
the defaults are 65/260 m, 20/100 m and 8/35 m respectively. Camera LOD bias
scales the effective distances. Foliage assets can also specify an optional
`lodDistances` pair; older assets retain projected-size selection.

City foliage and building impostor resolution are separate Inspector options:
Auto, 64, 128 or 256 pixels per direction. Auto uses 64 pixels in Preview and
128 otherwise. Press **Regenerate** after changing either setting. A 256-pixel
city can need a higher **City Source Memory MiB** budget.

With `--view verge`, the sample looks across an open road end from pavement
level. Use `-Views verge` in the capture harness with `-CityTerrainRoads`.
The evidence records the selected endpoint camera and counts uploaded planting
and soil-to-grass transition triangles separately.

On DX11, the terrain-city sample requests 4× scene multisampling at high and
cinematic quality and 2× at medium. Use `--scene-aa none|msaa2|msaa4` or the
capture harness `-SceneAA` option for fixed-camera comparisons. Capture
evidence reports the scene depth target's actual sample count and the final
resolved image is saved at the requested resolution.

The capture harness accepts `-Views road-edit-before,road-edit-after` with
`-CityTerrainRoads`. Both views apply the same live spline edit and check exact
terrain and road restoration. The first records Undo and the second records
Redo from the same camera. The evidence also checks native collision at a point
covered only by the edited road and records its terrain height in both stages.
The fixture also verifies restoration of the vacated grade after moving and
deleting the road, then replays the exact saved grade. The capture evidence
includes native verge geometry readiness.

Terrain-city evidence includes retained heightfield relief, centreline and edge
grades, half-crossfalls, and the terrain fitter's changed-sample/cut/fill report.
Up to 96 distributed carriageway probes and eight probes per roundabout compare
the runtime surface with terrain and native collision. Roundabout cap probes
also check surface-plane continuity and terrain support. These are bounded
samples alongside the existing full geometry validation, not an exhaustive
visual or hardware certification.

```powershell
./SampleVehicleAdvanced.exe --validate-circuit
./SampleVehicleAdvanced.exe --smoke-test --quality low
./SampleVehicleAdvanced.exe --track-smoke-test --quality low
./SampleVehicleAdvanced.exe --collision-smoke-test --quality low
./SampleVehicleAdvanced.exe --audio-smoke-test --quality low
./SampleVehicleAdvanced.exe --effects-smoke-test --quality low --no-audio
./SampleVehicleAdvanced.exe --capture car.bmp --view car --quality high
./SampleVehicleAdvanced.exe --capture track.bmp --view track --quality high
./SampleVehicleAdvanced.exe --capture corner.bmp --view corner --quality high
./SampleVehicleAdvanced.exe --capture follow.bmp --view follow --quality high
./SampleVehicle.exe --smoke-test
./ProceduralServiceTests.exe
./VehicleInertiaTests.exe
```

The circuit check covers 100 seeds, repeatability, uniform sampling, loop closure
and spawn alignment. The short smoke test checks settling, acceleration, turning,
braking, reset, chassis height, native render transforms and agreement between
the visible front-wheel steering and the tyre contact frame. The full-circuit
check uses throttle, brakes and steering through the physics controller and
requires a complete checkpointed lap. Checks exit with 0 on success and 1 on
failure. Captures save the rendered DX11 frame to a 32-bit BMP after startup,
then exit; captures on other renderers are currently unsupported.

Procedural service tests cover generated geometry, physics, appearance, factory
registration/unregistration, the paint/contrast regression and indexed tyre
triangle coverage on all four wheels at every LOD. The inertia test
checks world-space torque on a body with unequal principal moments in two poses.

## City building streaming

The city first generates its building geometry, then caches the detail mesh
payloads for eligible patches in a private directory under the project's
`.FBCache/ProceduralCityStreaming`. The generation owns this cache and removes
its files on teardown. Each eligible patch can use three screen-size LODs,
with detailed façades and roofs at LOD0, simplified building masses and roof
silhouettes at LOD1, and a baked, multi-view skyline imposter at LOD2. The
lowest available tier stays resident. Patches stay detailed when their distant
meshes lack required custom building geometry.

The active LOD camera controls residency of the higher-detail mesh payloads.
Moving or switching the camera updates both screen-size LOD selection and city
streaming. The C++ sample publishes its actual render-camera view; scene Camera
components are discovered automatically, including Editor camera overrides and
fallbacks. Offscreen render-texture cameras do not replace the primary view.
Losing the camera during a transition preserves existing residency until a valid
view returns. `ProceduralScene::setCityStreamingObservers` can explicitly replace
camera demand for simulation or tooling; clearing it resumes the active camera.
Buildings keep their actors, materials and collision. Roads, edited
terrain and the existing tree and paged foliage paths keep their own lifetimes.
The component saves `City Streaming` (enabled by default), `City Stream Load
Radius` (300 m), and `City Stream Retain Radius` (420 m). A larger retain radius
reduces reloads near the load boundary. Radii must be finite, with load greater
than zero and retain at least as large as load.

`City Source Memory MiB` (768 by default, range 128–4096) is a saved minimum for
the shared CPU source ledger. City generation raises the existing limit only
when needed. The ledger charges source bytes as resources are allocated, and
GPU resources use their own budget. Constant occlusion, roughness and metallic
channels in city materials use 1×1 scalar textures with the same values.
Channels with detail keep their original resolution and values.

Use `--city-streaming on|off` to select streaming in the sample and
`--city-source-memory-mib N` to override the minimum. The capture runner exposes
these as `-CityStreaming on|off` and `-CitySourceMemoryMiB N`. It defaults to
`off` for stable comparisons with earlier captures.

The Windows Claw streaming probe requires a generated city, streaming enabled,
and a capture. It first evicts initial detail and reloads from the near observers,
so the measured near and return states use the same load-radius conditions.
It then moves the streaming observer beyond the city and back while leaving the
capture camera in place. Actor and component identities are checked from startup
through this preparation and the measured round trip. The `cityStreamingSmoke` evidence records
settled near, evicted, returned and automatic-observer states, including native
mesh residency, imposter readiness and the shared source ledger. The sample
requires native detail eviction and reload and a decrease in source memory
during eviction before it resumes normal capture and collision validation.
The probe also moves the actual render camera away and back with explicit
streaming observers cleared, while a test-only kinematic hold keeps the vehicle
in the city. The Physics task applies the hold and restores the original dynamic
state, velocities and sleep state; capture success requires restoration. A separate
cold preparation makes that camera-only near/return comparison independent of
the earlier multi-observer demand. `cameraOnly` records camera/view agreement,
native-visible detail and imposter tier counts, residency, and measured vehicle drift
(at most 0.25 m). The original camera pose is restored before the capture.

Building imposters use 49 directional frames: 16 azimuths at three elevations,
plus an overhead frame. The active LOD camera selects a frame with angular
hysteresis. Each patch keeps one two-triangle quad, updating its transform and
material UV rectangle; its atlas stays resident while detail meshes stream out.
High-quality frames are 128 pixels with 2x supersampling and transparent gutters.
Orthographic Editor views use parallel camera rays. The quad sits in front of
the baked source bounds to avoid slicing elevated silhouettes into the ground;
its placement follows camera rays so its projected footprint stays fixed.
This is a distant representation with approximate depth. Detail LODs cover views
inside the source bounds; per-pixel depth, dynamic lighting and frame crossfades
are not provided by this sprite-sheet path.

The `imposter-front`, `imposter-diagonal`, `imposter-back` and `imposter-high`
review views use the same patch and a 300 m camera radius. They force the city
imposter tier on the distant target patch for inspection, first move from an
opposite bearing to verify
automatic frame selection, and export the actual atlas as a losslessly repacked
7-by-7 TGA next to each capture. Normal views retain automatic LOD selection.
These review views require a generated city with streaming enabled, and cannot
be combined with `--orbit` or the streaming smoke test.

```powershell
.\Tools\powershell\CaptureProceduralRacing.ps1 -Scene city -CityLayout lsystem `
  -CityTerrainRoads -CityTerrainRelief 2 -CityStreaming on -Seconds 1 `
  -Views imposter-front,imposter-diagonal,imposter-back,imposter-high
```

```powershell
./SampleVehicleAdvanced.exe --city-terrain-roads --city-layout lsystem --view hillside `
    --city-streaming on --city-source-memory-mib 768 --city-streaming-smoke-test `
    --capture hillside.bmp --evidence hillside.json

# From the repository root
./Tools/powershell/CaptureProceduralRacing.ps1 -Scene city -CityTerrainRoads -CityLayout lsystem `
    -Views hillside -CityStreaming on -CitySourceMemoryMiB 768 -CityStreamingSmokeTest
```

## Reusable tree LOD and generated imposters

Trees are grouped into 64-metre patches, each with a scene `LODGroup`. LOD0
combines trunks and canopies into one opaque draw using a two-colour palette;
LOD1 contains three cutout cards per tree
(two crossed side views and an overhead view). A shared 384 x 256 RGBA atlas is
baked from the same procedural pine mesh at startup using the engine's
`generateMeshImposters` API. The depth-tested CPU bake preserves the silhouette
and visible surface colours, with transparent gutters between views. The atlas
adds 393,216 bytes (0.375 MiB). Each distant tree uses six triangles, compared
with sixty stored triangles in the near mesh. Both representations stay allocated.

The reusable `LODSystem` snapshots bounds, thresholds and camera data into flat
arrays, selects larger sets in worker jobs and applies renderer visibility on the scene
thread. Trees have no individual update callbacks. Other generated meshes can use
the same components by adding renderer sets to a `LODGroup` in descending screen
height order. `setSize`, `setLocalReferencePoint`, `setHysteresis`, `setForcedLOD`
and `setCullBelowLastLOD` control selection. Automatic scene-camera discovery is
the default; applications with a separate rendering camera can publish a plain
`LODSystem::View` through `setViewOverride`, or restore discovery with
`clearViewOverride`. VehicleAdvanced publishes its actual follow/capture camera.
`getView` exposes the same snapshot to city residency. Automatic discovery uses
the active Editor view when present, otherwise the active game camera with the
highest viewport Z order; equal priorities keep registration order. Authored
offscreen output cameras are excluded. Scene Camera users, including the Lua
racing samples, do not need to publish an override every frame.

Small sets up to the configured grain size calculate synchronously, avoiding job
dispatch and a frame of result latency. Tree patches use `setDetailBounds` with
each tree's actual position and four-to-eleven-metre height, a screen height
threshold of 0.085 and 15% hysteresis. A patch retains its near representation
while any member requires it. Nonuniform actor scale uses the largest absolute
axis conservatively. Distances change with camera field of view and LOD bias. The last
level stays visible at all distances. Transitions are discrete, without blending.

`MeshImposterTests` checks deterministic baking, front-surface depth selection,
transparent borders, rejected invalid input and tree thresholds in both directions.
The existing `LODSystemTests` covers perspective/orthographic projection, forced
levels, culling and hysteresis. Captures also check that each tree patch has exactly
one visible level. Both test executables and
the sample build and pass the focused checks in Debug and RelWithDebInfo. The
Debug low-quality close capture selected four mesh and 24 imposter patches.

```powershell
cmake --build project_x64 --config RelWithDebInfo --target SampleVehicleAdvanced MeshImposterTests
cd Bin/windows/v145/x64/MD/RelWithDebInfo
./MeshImposterTests.exe
./UnitTests.exe --run_test=LODSystemTests
./SampleVehicleAdvanced.exe --capture tree_lod_car.bmp --view car --quality high
./SampleVehicleAdvanced.exe --capture tree_lod_track.bmp --view track --quality high
```

## Verified configuration and measurements

The measurements below describe earlier revisions. See the implementation
report in `review/IMPLEMENTATION.md` for the graphics and LOD update.

Verified on Windows x64 / v145 using WPGraphics (DX11), WPPhysics,
WPVehiclePhysics and WPProcedural. Both Debug and RelWithDebInfo build with the
original and advanced samples present. Procedural service and rotated-inertia
regression tests pass in both configurations; the 100-seed circuit check passes.
The assisted handling revision passes the five-phase driving check in
RelWithDebInfo with seed 7 / low graphics and seed 42 / high graphics. The turn
checks body slip and yaw rate as well as suspension and render poses. With
trackside collisions enabled, the turn uses moderate steering to stay within
the barriers. See [HANDLING.md](HANDLING.md) for
the setup, tuning settings and measured results. Earlier Debug checks predate
this handling revision.
Before the wheel revision, the advanced sample completed a checkpointed lap under
physics control in approximately 154 seconds in RelWithDebInfo. Car, overhead
circuit, corner and chase-camera views were captured and reviewed, with the car
capture refreshed for the new wheel geometry.

Measurements before tree imposters, using seed 7 in RelWithDebInfo at the default
1280 x 720 window:

| Quality | Generated triangles | Uploaded texture buffers | Texture bytes | Generation time |
| --- | ---: | ---: | ---: | ---: |
| Low | 20,554 | 31 | 4,472,832 (4.27 MiB) | 0.92 s |
| High | 34,070 | 31 | 31,997,952 (30.52 MiB) | 2.18 s |

Before tree imposters, both presets used 35 scene meshes. Measured mean
frame intervals over the capture's final three seconds were approximately 25
ms. This is whole-application wall time including physics, rendering and pacing,
not isolated GPU time. Before the detailed wheel revision, low-quality full-lap process peak working set was roughly
265 MiB. Mesh count is not a draw-call measurement; GPU timing and draw-call
profiling remain unmeasured. These observations describe this machine and do not
guarantee a frame rate on other hardware.

After tree imposters, the high preset generates 35,018 triangles across both
levels and uploads 32 textures totaling 32,391,168 bytes. Seed 7 creates 44 tree
patches and 165 allocated scene meshes. The close capture selected six mesh
patches and 38 imposter patches; the overhead capture selected all 44 imposter
patches. Both passed the single-visible-level check. Mean frame intervals were
approximately 36 ms and 38 ms respectively, with generation around 3.9 seconds.
This reduces distant tree geometry, but the additional patch renderers currently
increase whole-application frame time relative to the earlier global tree batch.
These timings are not isolated GPU measurements.

## Current scope

The graphics update adds explicit mip filtering for generated textures: linear-light
colour, normalized normal vectors, linear data, GGX roughness moments and
alpha-coverage-preserving cutouts. Atlas views remain isolated through the mip
chain. Grass and gravel now have seeded surface maps and anisotropic sampling.
DX11 materials receive scene fog for exponential, squared-exponential and linear
modes; fog is applied before the existing display transform. Its horizon colour
matches the generated sky through that transform, and the generated sun follows
the sky's sun direction. The sky remains unfogged. The fixed high-altitude
`--view track` review disables fog so circuit details remain visible.

The car now uses the generated LOD renderer sets under the original chassis and
four wheel actors. High/medium use all three levels; low starts at the middle
level. Levels share materials and wheel hub transforms. The player car retains
high detail during close views, with discrete hysteretic changes at distance.

For repeatable comparisons, use the following controls from the executable directory:

```powershell
./SampleVehicleAdvanced.exe --quality high --seed 7 --view car --no-hud --resolution 1280x720 --capture car.bmp
./SampleVehicleAdvanced.exe --quality high --seed 7 --view track --no-hud --force-lod 0 --benchmark 10
./SampleVehicleAdvanced.exe --quality high --seed 7 --view track --no-hud --force-lod 1 --benchmark 10
./SampleVehicleAdvanced.exe --quality high --seed 7 --no-hud --benchmark 15 --orbit
```

`--force-lod auto|0|1` applies to legacy trees and remains the paged foliage
fallback when `--foliage-lod` is absent. `--force-vehicle-lod auto|0|1|2`
selects a vehicle level within the current preset; low has two levels and clamps
the index accordingly. Benchmark mode holds the car stationary,
waits five seconds for warmup, resets native statistics and measures the requested
seconds. Logs include frame-interval mean/p95, CPU frame time including Present,
Present time, GPU time/sample count, draws, submitted triangles, constant uploads
and state binds per frame. GPU samples are asynchronous and may trail the frame
count. Counts include shadows/offscreen work. Capture mode also verifies tree and
vehicle LOD exclusivity, reflections and winding. Orbit mode requires benchmark mode.

The road and runoff are level and share one continuous static contact plane;
visual millimetre offsets prevent coplanar rendering artifacts. Guardrail beams
and posts, pit garages and doors, gantry supports and overhead beam, and tree
trunks have generated static box collisions. Rail boxes follow each segment's
yaw; separate gantry boxes preserve clearance over the road. Colliders remain
active through tree LOD changes and are removed with their generated scene.
Hills and tree foliage are decorative; track elevation and moving-platform
wheel contacts are not part of this sample. The collision smoke check probes
the generated physics shapes, checks gantry clearance and drives into a garage.

Collision validation passed in RelWithDebInfo with seed 7 / low (452 static
boxes) and seed 42 / high with tree imposters forced on (543 boxes). Both garage
impact checks stopped the vehicle at the wall. Physics scene storage now grows
beyond the old 256-body limit; a 529-body regression checks registration, ray
queries, contacts, removal and reuse. Sorting only the modified update list
during actor registration reduced measured generation time from 52.7 to 4.6
seconds on low and from 80.8 to 8.1 seconds on high with these colliders enabled.
The assisted Grand Prix tyre grip is restored to 1.25, matching the on-road
surface multiplier. The intervening 0.7 setting reduced initial traction and
failed the acceleration check independently of audio. Seed 7 / low now reaches
27.9 m/s with audio/effects enabled and 28.0 m/s with both disabled after the
four-second acceleration phase (previously 15.6 and 15.8 m/s respectively).
Acceleration and turning checks pass; the complete driving smoke still fails
the later braking-phase slip/yaw stability check. Its thresholds are unchanged.

Vehicle audio is enabled by default. Six engine loops blend with drivetrain
RPM and throttle; rolling noise follows road speed, and tyre squeal follows
measured slip on grounded wheels. Gain changes are smoothed, tyre sounds fade
while airborne, and all loops fade while paused. This is a centred driver mix,
using generic racing recordings rather than an F1 2012 engine recording.
`--no-audio` disables the car sounds. A missing output device or asset disables
audio gracefully; the vehicle remains driveable.

CMake copies the bundled WAV files and [audio credits](Audio/CREDITS.md) into
`Audio/VehicleAdvanced` beside the executable. Engine recordings are CC0, tyre
squeal is CC-BY 3.0, and the generated rolling loop is CC0; retain the credits
when redistributing. Builds and playback do not download assets.

`VehicleAudioMixTests` checks the RPM crossfade, slip/contact gating, fades and
invalid telemetry. `--audio-smoke-test` runs real platform voices through idle,
redline/skid, airborne and paused gain checks. The RelWithDebInfo build and mix
tests passed locally. The platform check is blocked here because XAudio2 cannot
open an output device (`0x80070490`); audible playback is unverified.

Vehicle visual effects are enabled by default in the C++ sample. Grounded tyres
emit smoke when slipping on the track and dust when moving across the runoff.
Sharp velocity losses at speed produce small spark bursts. Sparks use a velocity
change heuristic, rather than contact reports from the physics backend.
`--no-effects` disables particles and skid decals.

Skid marks follow the wheel contact points and normals, using a single batched
surface mesh with feathered tread textures. They fade after 16 seconds and expire
after 20 seconds. Grip recovery, airborne wheels, pause and reset break trails;
reset also clears live particles and marks. These are road-surface decal strips,
rather than decals projected onto arbitrary scene geometry.

The existing Claw particle simulator and DX11 billboard renderer handle smoke,
dust and sparks. Textures are generated in memory; there are no additional asset
downloads. Low/medium/high quality caps each particle system at 64/192/384 live
particles and skid strips at 256/1024/2048. Explicit emission preserves old
particles in world space. Decal geometry uploads are limited to 30 Hz.

`VehicleDecalTests` checks trail continuity, teleport rejection, invalid contacts,
capacity, fade and reset. The native particle contracts also check explicit
emission, world-space trajectories, pause, capacity and invalid input.
`--effects-smoke-test` uses scripted effect telemetry while checking that real
vehicle ground-contact rays resolve. Its five phases exercise road smoke/skids,
airborne decay, off-road dust, pause and reset/resume. With
`--effects-capture PATH.bmp`, it saves the road capture and `PATH.bmp.dust.bmp`.
These fixtures check effects independently of the car's handling tune.

Generated RGBA maps are uploaded in memory. Albedo is sRGB; normal/ORM channels
are linear. ORM is split explicitly into AO, roughness and metalness slots. The
sample includes a soft ground contact shadow, daylight and a procedural sky.
DX11 has a single directional shadow map. Cascaded shadows and GPU bloom
remain follow-ups; graphics toggles alone are not
evidence that those effects render. GPU HDR/TAA/AO, dithered LOD transitions and
true instance buffers are subsequent roadmap stages. The current tree change
reduces near submissions per patch, while distant patches remain separate draws.
