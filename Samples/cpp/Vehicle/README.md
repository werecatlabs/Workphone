# Vehicle sample

Build `SampleVehicle` and run it from its binary directory so the shared plugin
configuration and `Bin/Media` resources can be found. On the configured Windows
build:

```powershell
cmake --build project_x64 --target SampleVehicle --config RelWithDebInfo
Set-Location Bin/windows/v145/x64/MD/RelWithDebInfo
./SampleVehicle.exe
```

- W or Up: throttle
- S or Down: brake
- A/D or Left/Right: steer
- R: reset the chassis and wheels at the starting position
- Mouse wheel: zoom the follow camera
- Escape: quit

The overlay shows speed, vertical velocity, controls, position, chassis mesh
offset, wheel torque, suspension extension and the wheel's ground ray. The sample
starts physics automatically and loads `WPVehiclePhysics` if the shared sample
configuration does not already include it. Round wheel meshes use the simulated
wheel radius and place the axle according to suspension travel.

Run `./SampleVehicle.exe --smoke-test` for an automated settling, acceleration,
turning, braking and reset check at full throttle. It verifies the chassis and
wheel transforms after each rendered frame, including the native mesh owner
matrices on Claw, and checks height variation and vertical velocity throughout
each phase after the initial drop. It exits with code 0 on success and 1 on failure.
Use `--plugins <config-file>` to select a different plugin configuration, for
example to compare WPPhysics and PhysX. Use only one physics backend in each file.
