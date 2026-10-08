# Assisted Grand Prix handling

The sample now uses a forgiving, F1 2012-inspired handling setup. This is a
tuning reference, not a reproduction of Codemasters' proprietary model. The
generated modern Grand Prix geometry, mass, engine and aero remain the basis.
Both the C++ sample and the Lua/Editor procedural race scene use the same setup.

## What caused the easy spins

Transient wheelspin consumed rear lateral grip before the tyre solver limited
the longitudinal impulse. That limit now runs before combined-slip saturation.
The brush curve also dropped abruptly from peak to sliding friction; its
transition is now continuous. Traction control reserves grip for cornering and
progressively reduces drive torque as measured wheelspin rises. It does not
rewrite wheel speed or the chassis transform.

Brush wheels previously received a generic 8,000 Nm brake torque from the
drivetrain, bypassing the configured axle balance. Service braking now uses each
wheel's configured torque and optional ABS. Handbraking remains separate. The
implicit suspension response also retains more of the configured spring and
damper response, preventing the large heave seen in the previous driving check.

## Setup and tuning

The shared setup lives in `race::configurePhysics` in
`Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp`.

| Setting | Sample value | Effect |
| --- | --- | --- |
| Traction Control | true on wheels | Reduces power-induced rear wheelspin; sacrifices some acceleration near the grip limit. |
| Anti Lock Brakes | true on wheels | Releases service-brake torque before locking above 3 m/s; allows a complete stop at low speed. |
| Steering Rate | 90 road-wheel degrees/s | Ramps steering and countersteering instead of snapping between keyboard inputs. Zero disables the backend filter. |
| Steering Acceleration | 14 m/s² backend ceiling | Limits steering angle at speed. The existing player input path also applies its more conservative 8 m/s² limit. |
| Steering Wheelbase | generated wheelbase | Keeps the speed limit consistent with the vehicle dimensions. |
| Road Grip | 1.25 multiplier | Preserved across road/runoff/grass transitions, rather than silently replaced on returning to the road. |
| Rear grip / lateral stiffness | +6% / +10% | Adds a small rear grip reserve and favours progressive understeer. |
| Sliding friction | 94% of axle peak | Gives a more recoverable slide. |
| Brake torque | generated front 3,150 / rear 2,280 Nm | Restores the configured front brake bias. |

The native assists default to off for other vehicles. Native wheel properties
`Traction Control` and `Anti Lock Brakes` round-trip and survive reset. To change
the sample's defaults, edit its shared setup so regeneration and Play transitions
reapply the desired values. For a wheel-focused setup, start by reducing steering
filtering; disable traction control only after testing throttle modulation.

## Validation, Windows x64 RelWithDebInfo

- Built WPVehiclePhysics, SampleVehicleAdvanced, VehicleHandlingTests and UnitTests.
- VehicleHandlingTests passed steering timing at 30/60/120 Hz, torque/grip budgets,
  slip feedback, forward/reverse ABS and tyre saturation continuity checks.
- Four focused native vehicle cases passed all 48 assertions, including assist
  defaults, property restoration and reset.
- The native rotated-inertia regression check passed.
- Both seed 7 / low and seed 42 / high graphics passed all five driving phases:
  settle, accelerate, full-input turn, brake/reset and settle again. The existing
  suspension thresholds were retained; the acceleration check now requires
  more than 20 m/s, and body slip and yaw are monitored above 5 m/s.
- Four-second straight acceleration ended at 21.68 and 21.64 m/s respectively.
  Full-throttle turning peaked at 2.42° and 2.34° body slip. Maximum suspension
  height excursion across driving phases was about 6.2 mm, versus approximately
  198 mm in the earlier failing acceleration run.

These are automated driving and render-pose checks. Subjective keyboard,
gamepad and steering-wheel feel, race-distance handling and exact F1 2012
performance have not been verified. Logs are under `review/handling-*.log`.

Build the physics plugin explicitly; building the sample alone does not rebuild
its dynamically loaded WPVehiclePhysics DLL:

```powershell
cmake --build project_x64 --config RelWithDebInfo --target WPVehiclePhysics SampleVehicleAdvanced VehicleHandlingTests UnitTests --parallel 2
```
