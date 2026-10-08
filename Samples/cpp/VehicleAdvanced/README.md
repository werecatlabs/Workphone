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

```powershell
./SampleVehicleAdvanced.exe --validate-circuit
./SampleVehicleAdvanced.exe --smoke-test --quality low
./SampleVehicleAdvanced.exe --track-smoke-test --quality low
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

## Reusable tree LOD and generated imposters

Trees are grouped into 64-metre patches, each with a scene `LODGroup`. LOD0
contains trunk and canopy renderers; LOD1 contains three cutout cards per tree
(two crossed side views and an overhead view). A shared 384 x 256 RGBA atlas is
baked from the same procedural pine mesh at startup using the engine's
`generateMeshImposters` API. The depth-tested CPU bake preserves the silhouette
and visible surface colours, with transparent gutters between views. The atlas
adds 393,216 bytes (0.375 MiB). Each distant tree uses six triangles, compared
with sixty stored triangles in the near mesh. Both representations stay allocated.

The reusable `LODSystem` snapshots bounds, thresholds and camera data into flat
arrays, selects levels in worker jobs and applies renderer visibility on the scene
thread. Trees have no individual update callbacks. Other generated meshes can use
the same components by adding renderer sets to a `LODGroup` in descending screen
height order. `setSize`, `setLocalReferencePoint`, `setHysteresis`, `setForcedLOD`
and `setCullBelowLastLOD` control selection. Automatic scene-camera discovery is
the default; applications with a separate rendering camera can publish a plain
`LODSystem::View` through `setViewOverride`, or restore discovery with
`clearViewOverride`. VehicleAdvanced publishes its actual follow/capture camera.

Tree patches use a representative seven-metre tree size, a screen height threshold
of 0.085 and 15% hysteresis. At a 45-degree vertical field of view, an initially
selected patch switches around 100 metres from its centre; hysteresis retains
meshes out to roughly 117 metres and restores them inside roughly 86 metres.
Distances change with camera field of view and LOD bias. Selection uses the patch
centre, so individual trees within a patch can be closer or farther away. The last
level stays visible at all distances. Transitions are discrete, without blending.

`MeshImposterTests` checks deterministic baking, front-surface depth selection,
transparent borders, rejected invalid input and tree thresholds in both directions.
The existing `LODSystemTests` covers perspective/orthographic projection, forced
levels, culling and hysteresis. Captures also check that each tree patch has exactly
one visible level and that the trunk and canopy agree. Both test executables and
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

Verified on Windows x64 / v145 using WPGraphics (DX11), WPPhysics,
WPVehiclePhysics and WPProcedural. Both Debug and RelWithDebInfo build with the
original and advanced samples present. Procedural service and rotated-inertia
regression tests pass in both configurations; the 100-seed circuit check passes.
After the detailed wheel revision, the Debug five-phase driving check and visual
steering comparison pass. RelWithDebInfo also passes the steering comparison, but
its broader driving check intermittently exceeds the suspension-height limit
during turning or braking; this remains a simulation stability issue.
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

The road and runoff are level and share one continuous static contact plane;
visual millimetre offsets prevent coplanar rendering artifacts. Trees, hills,
pits and guardrails are decorative. Track elevation, barrier collisions and
moving-platform wheel contacts are not part of this sample.

Generated RGBA maps are uploaded in memory. Albedo is sRGB; normal/ORM channels
are linear. ORM is split explicitly into AO, roughness and metalness slots. The
sample includes a soft ground contact shadow, daylight and a procedural sky.
Full shadow maps, bloom, engine audio and particle effects remain renderer/audio
follow-ups; graphics toggles alone are not evidence that those effects render.
