# Lua sample

`bin/Media/Scripts/Lua/Game/Core/SampleVehicleAdvanced.lua` is the Script component
version of this sample. Rebuild Workphone, WPGraphics, WPLuaBind and Editor before using it.
The editor plugin configuration must include WPProcedural and WPVehiclePhysics.

The Lua and native samples share `ProceduralRaceScene`: procedural car/track assets,
physics tuning, roadside box collision, LOD, wheel visuals, shadows, audio mixing,
contact sampling, particles and skid decals. The engine component owns their lifetime;
regeneration and actor destruction release them. Presentation updates run on the render
task; no per-frame Lua particle or decal calls are needed.

**Audio Enabled** and **Effects Enabled** default to true and can be changed live in
script properties. They are independent of Smoke Test, which is a driving test.
The Claw backend supplies smoke, dust, impact sparks and bounded skid marks, using
quality-dependent budgets. Other graphics backends continue to run the sample but
report effects as unavailable. Pause stops emission and fades audio; Reset clears
particles and marks. Audio uses separate voices per vehicle, so deleting one sample
does not stop another vehicle's sound.

The Workphone build copies the existing WAV pack and its credits into
`bin/Media/Audio/VehicleAdvanced`, where the engine audio loader resolves it for both
Editor and the native application. A working audio output device and WPAudio plugin
are required for sound. Presentation failures are logged without stopping generation.

Lua can call `raceScene:setAudioEnabled(bool)`, `setEffectsEnabled(bool)`,
`getAudioEnabled()`, `getEffectsEnabled()`, `isAudioAvailable()`, `isEffectsAvailable()`,
`getParticleCount()` and `getSkidDecalCount()`. Availability becomes current after the
first render update. Existing seed, quality, generation, collision and LOD APIs remain
the shared entry points. C++ consumers can also use the engine's `VehicleAudio` and
`VehicleVisualEffects` helpers; the latter selects a registered graphics implementation
through `IVehicleVisualEffects`, keeping Workphone independent of WPGraphics.

Create an empty actor in the Editor, add a **Script** component, and set its
`className` to `SampleVehicleAdvanced` and enable **updateInPlayMode**.
Press **Generate** in the script properties
or enter play mode. The script creates its own vehicle and follow camera. Quality
values are 0 (Preview), 1 (Standard), 2 (High) and 3 (Cinematic); the default seed is 7.

Drive with W/Up, brake with S/Down, steer with A/D or Left/Right, reset with R, and
zoom with the mouse wheel. Escape exits the application. The HUD reports speed,
gear, RPM, lap times, circuit length and off-track state. A lap requires all three
quarter-circuit checkpoints in order while on the road. Smoke Test exercises
settling, acceleration, steering, braking and reset; Track Smoke Test drives a lap.
These options exit the hosting application when the test finishes.
Smoke results are written to `VehicleAdvancedLuaSmoke.log` in the application's
working directory, including each phase's height and speed measurements.

**Performance Test** waits ten seconds after generation, then measures application
update intervals for thirty seconds and writes `VehicleAdvancedLuaPerformance.log`
in the application's working directory before exiting. It includes mean and p95
intervals and task profiler averages; application update rate is not rendered FPS.
Keep the seed, quality, viewport and machine load fixed when comparing runs.

The report also includes completed-Present intervals, render-pass CPU time,
Present wait, delayed GPU timestamp/disjoint samples and draw/upload counters.
These measurements cover offscreen, window and UI passes together. Render-pass
CPU time includes Present; subtract the separately reported Present time to
estimate the remaining render-pass elapsed time. It does not measure application
work outside those passes. The interval mean covers the measurement period; p95
uses the last 8192 intervals (enough for the standard thirty-second test).
The existing task profiles still use rolling averages and cannot be added into
a frame breakdown. GPU queries are polled without flushing or waiting. The
application requests a snapshot from the render thread and waits for publication
without taking the graphics update lock. Timings and counters use separate
property fields to respect the engine's 255-character value limit.

The renderer retains unchanged material constants and DX11 shader, texture,
sampler and geometry bindings between compatible draws. Native mesh objects
carry a borrowed wrapper submission context, cleared on unbind, so scene passes
no longer rebuild a mesh lookup map. Lights have a separate scene membership list.
UI/external-context and render-target transitions invalidate the binding cache.

Legacy presentation remains the default. For diagnostics, set
`WORKPHONE_DX11_FLIP_SWAP_CHAIN=1` before launching to request two-buffer flip
discard with a one-frame queue limit. Unsupported or failed requests fall back
to legacy; `WORKPHONE_DX11_LEGACY_SWAP_CHAIN=1` overrides the request. VSync is
preserved, and tearing is used only when supported and VSync is disabled.
The initial Editor flip comparison reduced cadence from about 60 to 40 FPS, so
it is opt-in pending a frame-pacing investigation. A 64-metre circuit-section
prototype also exceeded the Editor EventJob pool during generation; it was
removed, leaving circuit geometry and collision unchanged.

The final legacy Editor comparison (seed 7, High, stationary follow camera) ran
at 16.678 ms mean / 17.616 ms p95 between completed Presents, with 1.745 ms GPU
time and no geometry creations after warmup. It remains near 60 rendered FPS;
the small timing difference from the instrumented baseline is not a demonstrated
whole-frame speedup. Repeated-draw tests verify fewer material uploads and state
bindings, and the native full-lap test and inspected car/track captures pass.
The native acceleration smoke still fails its suspension-height variation limit
(about 0.185 m versus 0.15 m); that physics check remains unresolved.

The generated environment cubemap is static. Its six faces are filtered once on
first use and the GPU texture is reused for subsequent material draws. Replacing
its faces explicitly invalidates that cache; it does not capture the scene every
frame. Unchanged probe settings are also reused between updates.

The renderer also caches each sky's filtered cubemap separately. The Editor's
default sky and the generated circuit sky can both be visible; a shared cache
previously switched between their faces and repeated GGX filtering every frame.
Each sky now retains its filtered texture until its source faces change, and
unloaded skies release their cached GPU resources.

An Editor comparison on 2026-10-05 used seed 7, High quality, the stationary
vehicle follow camera and the same window size. Average scene drawing decreased
from 179.883 ms to 2.193 ms, and the render task decreased from 199.907 ms to
33.281 ms. These are profiler elapsed times, including presentation waits in the
render task, rather than an FPS measurement. The profiler now separates scene
drawing, graphics preparation, Editor UI, UI submission and presentation.

## Reusable component

CarController owns keyboard/gamepad input and is the single publisher of throttle,
brake and steering channels. W/A/S/D and the arrow keys are read together, so
opposing steering keys cancel and releasing one alias does not cancel a held
alias. Speed-dependent steering limiting is applied to player input there.
The samples retain reset/camera commands and smoke-test/AI policy only.
`ProceduralRaceScene::setControls()` delegates an exclusive programmatic override
to CarController; `usePlayerControls()` returns to keyboard/gamepad input.
Input events cannot overwrite an active override. Pausing or leaving play mode
publishes zero channels instead of leaving the last driving command active.

`scene::ProceduralRaceScene` owns the seeded vehicle mesh hierarchy, textures,
closed circuit, collision plane, trackside scenery, sky, reflection probe and
contact shadow. Both this C++ sample and the Lua sample use it. Attach it to the
vehicle actor, set its seed and quality, and call `regenerate()` while stopped.
It adds the required CollisionBox, Rigidbody and CarController siblings. Publish
input through `setControls(throttle, brake, steering)` and request reset with
`reset()`. The component registers physics and render callbacks and releases its
generated actors and resources on unload. Circuit queries and vehicle parameters
are exposed to Lua for custom race rules and AI. `setView()` supplies the camera
parameters used by tree LOD selection.

The C++ command-line frame capture and native render-matrix validation harness
remain in the executable. The Lua component uses the editor camera viewport and
application-update smoke checks; it does not create another ApplicationManager or
load plugins into the running editor.

## Script checks

From the repository root:

```powershell
./bin/windows/v145/x64/MD/RelWithDebInfo/luavm.exe Tests/lua/SampleVehicleAdvancedTests.lua
```
