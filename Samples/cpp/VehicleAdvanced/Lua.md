# Lua sample

`bin/Media/Scripts/Lua/Game/Core/SampleVehicleAdvanced.lua` is the Script component
version of this sample. Rebuild Workphone, WPGraphics and WPLua before using it.
The editor plugin configuration must include WPProcedural and WPVehiclePhysics.

Create an empty actor in the Editor, add a **Script** component, and set its
`className` to `SampleVehicleAdvanced`. Press **Generate** in the script properties
or enter play mode. The script creates its own vehicle and follow camera. Quality
values are 0 (Preview), 1 (Standard), 2 (High) and 3 (Cinematic); the default seed is 7.

Drive with W/Up, brake with S/Down, steer with A/D or Left/Right, reset with R, and
zoom with the mouse wheel. Escape exits the application. The HUD reports speed,
gear, RPM, lap times, circuit length and off-track state. A lap requires all three
quarter-circuit checkpoints in order while on the road. Smoke Test exercises
settling, acceleration, steering, braking and reset; Track Smoke Test drives a lap.
These options exit the hosting application when the test finishes.

## Reusable component

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
