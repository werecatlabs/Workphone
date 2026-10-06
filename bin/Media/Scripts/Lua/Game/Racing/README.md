# Workphone racing components

The racing scripts use Workphone actors, native vehicle/physics components and UI.
`Game/Core/RacingGameFull.lua` is the playable entry point: attach it as the class
of an actor's Script component, enable `updateInPlayMode`, and use **Generate** to
build in edit mode. Enter Play for circuit challenge or time trial, menus, countdown,
pause, retries, telemetry, records and results. Generation and retries reuse the
procedural vehicle sample. Ranking uses the same RaceView/RankManager components
available to other games.

## Component ownership and setup

Each helper takes its Script component in its constructor. Bind the native objects
or other Lua helpers explicitly before calling its methods. Owning game scripts
instantiate helpers and forward `update(dt)`; do not attach another Script to the
same actor that updates those same helpers a second time. A standalone attached
helper also needs a game/bootstrap script to supply its bindings. Constructors do
not search the scene or create implicit singletons.

`RacingComponent` provides finalization through idempotent `shutdown()` methods.
The game owner calls shutdown when it destroys its generated subtree. Helpers do
not destroy borrowed actors, vehicle controllers, cameras, sounds or labels. RaceUI
and MenuManager destroy only the UI roots they generated. UI callbacks queue work
until the next update to avoid destroying actors inside a native button callback.
Call UI and actor mutation on the scene task. Invalid configuration fails with a
specific error; expected save/network/asset failures return `false, reason`.

Distances are metres, velocities metres/second, angles radians, and times seconds.
Local **-Z is forward**, +Y is up, and positive car steering turns toward +X.

## APIs

| Area | Components and setup |
| --- | --- |
| Timing | `RaceManager:InitializeRace("race" or "timeTrial", laps)`, then `OnUpdate(dt, progressSample)`. Pause/resume preserve elapsed time. `TimeTrialConfig:apply(manager)` selects unlimited laps. `Statistics:updateSession` formats telemetry. |
| Routes | `WaypointCircuit:setProceduralScene(scene)` reuses native sample lookup. `SetWaypoints(vectorArray)` builds a closed authored circuit with true segment distances; consecutive duplicate points are rejected. `PathCreator:fromActors` and `sample(spacing)` provide authoring helpers. |
| Progress | `ProgressTracker:bind(actor, circuit)`, then `sample()` gives index/count, road offset and wrong-way/off-track flags. It tracks ordered lap progress for standings. `Laps` reuses the same quarter-checkpoint rules as RaceSession. |
| Gates | Configure `Checkpoint` or `DroneGate` with position, normal and half-extents. `crossed(previous, current)` tests forward swept-plane crossings within the aperture and rejects teleports over `maxStep`. `CheckpointContainer:setCheckpoints(orderedGates)` and `sample(racerId, position)` support separate per-racer state. Order the first gate at the start, intermediate gates afterward, and the final gate at the finish. Remove departing racers explicitly. |
| Grid | `SpawnpointContainer:generateGrid(circuit, count, spacing, halfWidth)` or `setPoints({{position, rotation}, ...})`; `GetStartTransform(rank)` returns copies. Grid transforms use circuit -Z heading. |
| Standings | Bind `RaceView` with a numeric ID, actor, Statistics and ProgressTracker. Register views in `RankManager`. `refresh()` sorts finished racers by total time, then active racers by lap progress, with IDs breaking ties. `RaceStandingsUI:bind(textComponent, rankManager)` displays rows. |
| Vehicle | `RacingCarController:bind(nativeCar, actor)` delegates validated input to native CarController. PlayerControl enables native keyboard/gamepad ownership. Wheels configures borrowed native WheelController components; native suspension owns rolling animation. |
| AI | `OpponentControl:bind(car, actor, body, circuit)`, set `targetSpeed`, optional Brakezone list, then enable/update. Look-ahead steering and braking use native CarController; shutdown brakes the car. It provides basic route-following AI, not collision avoidance or overtaking. |
| Mobile | `MobileControlManager:bind(car)`, enable, then supply touch IDs/controls to `setTouch` and release/cancel on pointer loss. It explicitly transfers input ownership; it does not create platform-specific touch widgets. |
| Bike/drone | RacingMotorbikeController delegates native driving and smooths a separately bound rider's visual lean. It needs a suitable native chassis; it does not implement two-wheel balance physics. RacingDroneController applies thrust/drag/torque to a bound Rigidbody only through `physicsUpdate()`, which the native physics owner must call once per physics step. Ordinary Lua Script update must not apply those forces. |
| Replay | ReplayManager records at 20 Hz, up to 12,000 frames (10 minutes), and stops at capacity. Playback interpolates position and normalized shortest-path rotation, supports pause and looping, and samples by binary search. GhostVehicle copies validated samples onto a visual actor with no Rigidbody/CarController. Supply visual actors for replay playback to avoid competing with live physics. Recordings are in memory; no replay file codec is supplied. |
| Cameras | PlayerCamera delegates follow/zoom to native VehicleCameraController. PlayerFPCamera, ReplayCamera and MinimapFollowTarget reuse TargetFollower. Bind a camera actor and target; CameraSwitcher selects among supplied native Camera components. |
| Presentation | WaypointArrow/RacerPointer orient visual actors toward targets. RacerName sanitizes text. TextAlpha fades a borrowed Text component; FramerateCounter averages over half-second windows. UIButton debounces activation and defers its callback. Skidmark stores a bounded segment ring and invokes a supplied ribbon renderer with `(segment, slot)`; the renderer owns GPU geometry. |
| Audio | Register already-loaded native ISound resources in SoundManager. It controls playback, location, music, volumes and mute. Spatial/2D mode is selected when loading the sound resource. One registered sound is retriggered rather than allocating new sound actors; supply resource instances for the polyphony your game needs. |
| Player data | `PlayerData:open(recordsPath)` loads lap records and a separate `.profile` file. `saveProfile()` persists credits, upgrade levels and claimed run IDs. RaceRewards requires a stable unique run ID and a validated finish result. VehicleUpgrades purchases catalog levels, rolls back on failed saves, and invokes supplied native tuning callbacks during `apply()` at race setup. These helpers do not trust network clients to grant currency. |

## Persistence

DataLoader stores versioned, escaped scalar dictionaries and never executes file
contents. It limits files to 1 MiB, 4,096 entries and 64 KiB per string. RacingRecords
retains its plain lap-record format. Writes close a temporary file before replacing
the target, preserve a backup during replacement, and restore it on replacement
failure. Use one writer per player profile; concurrent processes must not share the
same file. If a crash leaves `.bak`, validate/recover that backup before further
saves; the loader refuses to overwrite it. Corrupt profiles are reported and kept.
Reward history is capped at 2,048 run IDs and upgrade levels at 100. Persistence
errors remain visible to the game instead of silently losing the existing save.

## Networking through Workphone

The Photon/PUN and unfinished Steam-specific Lua files have been removed. The Lua
layer does not import either SDK. Steamworks implementation belongs in the future
C++ backend behind `INetworkManager`, `IPacket` and native actor `NetworkView`.

`RacingNetworkTransport:bind(networkManager, sessionEpoch, authorizeSender)` uses
the existing Workphone interface: `createPacket`, reliable `sendPacket`, and
`sendPacketUnreliable`. The session epoch is a nonzero uint32 assigned by the native
session owner and shared during authenticated session setup. Bind before subscribing;
rebinding clears old subscriptions/sequences.

- `subscribe(racerId, callback)` receives validated racer metadata; `sendRacer(data)`
  sends standings/progress snapshots. RaceView provides `ToData` and `FromData`.
- `subscribeSession(sessionViewId, callback)` receives race state, countdown, pause
  resume state, laps and clocks; `sendSession(data)` sends reliable host state.
  RaceManagerView provides `ToData`, `receiveSession`, `receive` and `publish`.
- The native network listener must forward application packets to `receive(packet)`
  on the scene task. Native NetworkView separately replicates actor transforms.
  The Lua transport does not replace native actor ownership or transport lifecycle.
- `authorizeSender(systemAddress, objectId, messageType)` must authenticate the
  sender and enforce **host authority**, using native session/address ownership.
  No default-allow policy is supplied. RaceManagerView clients reject state on a
  host instance. Network participants are registered through authenticated session
  setup; snapshots never create arbitrary actors or participants.
- The wire envelope uses Workphone magic `0x574E4554`, version 1, application message
  types 64 (racer) and 65 (session), view ID, epoch and sequence. Packets are fixed at
  39 and 43 bytes. Exact size, enums, finite numeric ranges, epoch and increasing
  sequences are checked before callbacks. Unknown, stale, malformed or unauthorized
  packets return a failure without applying state. Sequence exhaustion requires a
  new epoch; host migration and lobby/session negotiation remain native responsibilities.

Example client bindings, after participant/session registration:

```lua
transport:bind(app:getNetworkManager(), nativeSessionEpoch, authorizeNativeHost)
managerView.isHost = false
managerView:bind(raceManager, rankManager, transport)
managerView:JoinRace(racerView)
transport:subscribe(racerView.id, function(data) managerView:receive(data) end)
transport:subscribeSession(managerView.id, function(data) managerView:receiveSession(data) end)
-- Native packet dispatch forwards packet ownership safely to transport:receive.
```

## Migration and validation

Old `Car_Controller`, `Motorbike_Controller`, and `Drone_Controller` scripts are now
RacingCarController, RacingMotorbikeController and RacingDroneController. The duplicate
top-level CheckPoint script was removed; use Checkpoint in Race/Others. The container
class now matches `CheckpointContainer.lua`. Update old scene Script class names.
There are no Unity GameObject, Time, JsonUtility, Photon or scene-singleton calls in
this directory. Small subclasses deliberately reuse shared behavior.

Run from the repository root:

```powershell
./bin/windows/v145/x64/MD/RelWithDebInfo/luavm.exe Tests/lua/RacingComponentsTests.lua
```

This suite loads every script listed in RacingScriptManifest, constructs every
component, and exercises the existing vehicle/full-game regressions plus routes,
gates, -Z AI steering, mobile input, physics force interfaces, replay timing,
ranking/ties, UI cleanup, failure handling, persistence rollback and wire packets.
Fixtures mirror the native binding signatures. This is Lua behavior validation;
native Editor rendering, actual chassis physics, sound assets, ribbon rendering,
and a real network backend still require integration checks on the target build.
