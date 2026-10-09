# WPPhysics production implementation plan

Date: 9 October 2026. Reviewed checkout: `059bfa908`, with existing local changes. Status: proposed implementation plan; runtime implementation is not part of this document change.

## 1. Objective and release scope

Make the existing WPPhysics backend a reliable, supported physics system for shipped Workphone applications. Retain the native `WorkphonePhysics` simulation layer and the C++ `WPPhysics` integration layer, replacing incomplete behavior within them where necessary. Start with Windows x64/MSVC and C++17 integration; other toolchains and platforms need their own certification.

Production readiness means correct simulation within published limits, safe lifetime and concurrency behavior, useful authoring tools, measurable performance, reproducible builds, and evidence from runtime tests. Class names, property round trips, and successful linking do not establish a working feature.

Two releases keep the work deliverable:

- **R1: production core.** Static, dynamic and properly targeted kinematic bodies; primitive/convex/compound collision; static mesh and heightfield collision; stable contacts and fixed/D6 joints; an explicit kinematic constraint mode; raycasts, overlaps and sweeps; triggers/events; a baseline character controller; 2D core; collision cooking; Editor/Lua integration; diagnostics, benchmarks and certification.
- **R2: comprehensive gameplay physics.** Richer joints, ragdolls and animation interaction, advanced character movement, stronger vehicle integration, buoyancy/force fields, large-world streaming, replay/state restoration, richer 2D joints and tools, and measured parallel execution.
- **Extensions with separate gates.** Reduced-coordinate articulations, ropes/deformables/cloth, destruction/fracture, fluids, GPU simulation, and cross-platform deterministic lockstep. Keep capability hooks for these, but do not make every specialist simulation system a prerequisite for shipping rigid-body physics.

The user's requested **kinematic constraint mode is required in R1**, alongside improved kinematic rigid bodies. Section 5 defines its behavior and its distinction from a force-driven joint.

## 2. Evidence from the current checkout

This is a targeted source review, not a complete numerical or concurrency audit. Observations below identify work to verify and implement, rather than claiming that every unreviewed path is absent.

| Area | Reviewed evidence | Implication |
|---|---|---|
| Layering | [C++ build](../Engine/cpp/Project/WPPhysics/CMakeLists.txt) links native `WorkphonePhysics`; [native build](../Engine/c/Project/WorkphonePhysics/CMakeLists.txt) links `WorkphoneCollision` | Keep native simulation authoritative and wrappers responsible for engine integration |
| Time stepping | [Scene wrapper](../Engine/cpp/Source/WPPhysics/WPPhysicsScene3.cpp) reads frame delta and clamps it to 1/30 second; [native scene](../Engine/c/Source/WorkphonePhysics/workphone_physics_scene.c) clamps input to 0.25 second and computes up to 64 adaptive substeps | Establish one fixed-tick owner, a documented catch-up policy, and separate internal collision substeps |
| Kinematic bodies | [RigidDynamic3](../Engine/cpp/Source/WPPhysics/WPPhysicsRigidDynamic3.cpp) stores a target and immediately calls `setTransform()`; native integration also integrates non-static bodies from velocity and applies damping | Implement pending target consumption and derived velocities at the simulation boundary; avoid target motion being damped or advanced twice |
| 2D targets | [Scene2](../Engine/cpp/Source/WPPhysics/WPPhysicsScene2.cpp) sets kinematic bodies to target positions before simulation | Apply the same tick/target semantics in 2D; test angular motion and hidden-axis invariants |
| Shape collision | [Narrowphase](../Engine/c/Source/WorkphonePhysics/workphone_physics_narrowphase.c) implements sphere/box/capsule/plane pairs and primitive-to-mesh paths with triangle-tree candidates | Preserve these paths and regressions; complete and certify a pair-by-pair support matrix, including convex shapes |
| Mesh scope | Narrowphase explicitly excludes dynamic mesh-vs-mesh response | Use convex/compound representations for moving concave objects; keep arbitrary triangle meshes static in R1 |
| Contacts | Native scene contains direct contact resolution and a manifold cache keyed by body pair; cached validity records translation/frame information | Add shape-pair/feature identity, rotation and geometry revisions, safe invalidation and persistent impulse data |
| Broadphase | Native scene has an all-pairs AABB-filtered path and an optional grid branch; [broadphase API](../Engine/c/Source/WorkphonePhysics/workphone_physics_broadphase.c) is separate | Connect one measured production broadphase; prove that selecting an algorithm actually changes execution |
| Solver settings | [Solver configuration](../Engine/c/Source/WorkphonePhysics/workphone_physics_solver.c) stores iterations, timestep, CCD and broadphase settings; reviewed scene execution does not reference that configuration API | Connect settings to actual simulation or report unsupported values; remove misleading successful no-ops |
| Constraints | [Native constraint API](../Engine/c/Include/WorkphonePhysics/workphone_physics_constraint.h) exposes fixed/D6 axes and drives. Native scene solves linear limits/drives and corrects orientation for fixed joints or when all angular axes are locked | Implement independent twist/swing constraints, angular limits/drives, anchor Jacobians, inertia-aware impulses and stable iteration |
| Joint force semantics | Current angular correction distributes by inverse mass; break checks estimate force/torque from positional error and timestep; drive clamp limits velocity change | Replace these with dimensionally correct effective mass/inertia, accumulated impulse limits and solver reaction measurements |
| Duplicate adapters | Manager3 defines private native fixed/D6 adapter classes; [WPPhysicsConstraintD6](../Engine/cpp/Source/WPPhysics/WPPhysicsConstraintD6.cpp) also implements a native adapter | Consolidate construction and behavior before adding mode/drive features; verify all factories and registration paths |
| Events | Reviewed scene skips physical response for trigger shapes, but exposes no contact/trigger event queue in that execution path | Add pair lifecycle tracking and buffered begin/stay/end/break events, including non-responsive pairs |
| Character integration | [Manager3](../Engine/cpp/Source/WPPhysics/WPPhysicsManager3.cpp) creates [CapsuleController](../Engine/cpp/Source/Workphone/Physics/CapsuleController.cpp); its load method contains placeholder native-representation setup | Require shape sweep/depenetration evidence before certifying character movement |
| Engine integration | Scene3 publishes transforms and snapshots actors around callbacks; [Constraint component](../Engine/cpp/Source/Workphone/Scene/Components/Constraint.cpp) owns property and lifecycle integration | Retain useful behavior; formalize mutation, event delivery, state ownership and save/load contracts |
| Tests | [C collision tests](../Tests/c/WorkphonePhysicsCollisionTests.c), [C material tests](../Tests/c/WorkphonePhysicsMaterialTests.c), [native regressions](../Tests/cpp/UnitTests/WPPhysicsNativeCollisionTests.cpp), and physics/component/mesh/terrain tests exist | Extend existing regression coverage; add isolated mandatory tests without application/backend skip paths |

### Baseline actually executed

On 9 October 2026:

```powershell
ctest --test-dir project_x64 -C RelWithDebInfo --output-on-failure -R '^WorkphonePhysics\.'
```

Result: **2/2 registered targets passed**, `WorkphonePhysics.collision` and `WorkphonePhysics.material`, in 1.16 seconds total. These used existing executables without rebuilding. Source/executable correspondence was not established. The C++ UnitTests suite, fresh compilation/linking, memory checks, benchmarks and interactive Editor scenes were not run for this planning change. The two targets cover multiple checks, but do not establish production readiness.

## 3. Architectural decisions

1. **One simulation owner.** Native C owns bodies, contacts, joint solving and queries. C++ owns engine handles, components, scheduling, resources and event translation. Editor and Lua submit commands to the same runtime services.
2. **One clock.** Scene scheduling owns an accumulator and a configurable fixed tick, initially 60 Hz. Vehicles may request a documented 120 Hz profile. Internal solver/CCD substeps do not advance gameplay callbacks or consume a target multiple times.
3. **One mutation boundary.** Create/destroy, shape replacement, mode changes and filter edits apply between ticks. Native pointers do not outlive owning handles. Use stable identifiers with generations for queued work and event references.
4. **One result boundary.** Publish completed, tick-stamped transforms, query data and events. Rendering interpolates completed states. User callbacks run outside simulation locks and cannot invalidate an active solver traversal.
5. **One configuration contract.** Use validated scene/body/joint descriptors and a capability report. A requested setting either affects execution, returns a clear unsupported/error result, or selects a named documented fallback.
6. **Preserve compatibility deliberately.** Keep existing C function signatures and C++ virtual tables where practical. Introduce additive native functions and an optional versioned control interface for new constraint features. A shared-interface ABI change requires a version bump and rebuilding all consumers.
7. **Consolidate adapters.** Choose the public WPPhysics constraint classes as the canonical implementation after verifying their lifecycle, then route manager factories through them and retire duplicate private adapters.
8. **Be explicit about language support.** Current native scene code uses C99-style declarations/loops; the native physics target does not itself require strict C90. M0 must select and enforce the actual supported C dialect rather than inheriting a graphics-module assumption.
9. **Keep modules focused.** WPVehiclePhysics owns tire/powertrain behavior, animation owns pose evaluation, and WPGraphics owns rendering. WPPhysics supplies collisions, forces, joints, authoritative state and debug geometry.
10. **Publish numerical limits.** Define units, gravity, shape dimensions, mass ratios, maximum motion per tick, coordinate range, tolerances and capacity budgets. Unsupported dynamic concave meshes fail validation or require explicit convex cooking.

Target tick pipeline:

```text
fixed-tick scheduler
  -> validate/apply queued mutations
  -> consume body/joint targets and build kinematic trajectories
  -> update broadphase and build unique shape pairs
  -> narrowphase + persistent manifold refresh + CCD
  -> build islands and contact/joint rows
  -> iterate velocity/position constraints and integrate
  -> update sleep state and pair lifecycle
  -> publish immutable state/events/statistics
  -> engine callbacks and presentation interpolation
```

The exact integration/CCD ordering is finalized in M3/M4 against regression tests. Broadphase bounds must include the intended swept motion.

## 4. Feature coverage

Every required feature includes runtime behavior, validated serialization, appropriate Editor/Lua exposure, tests, diagnostics, documentation and a sample. Existing working code is reused.

| Feature family | R1 production core | R2 comprehensive release | Separate extension |
|---|---|---|---|
| Scene stepping | Fixed ticks, pause/single-step, explicit manual stepping, bounded catch-up, interpolation | Multiple independent scenes/time scales, recorded tick streams | Cross-machine lockstep certification |
| Rigid bodies | Static/dynamic/kinematic, mass/density/COM/inertia, force/impulse modes, damping, sleep/wake, axis locks | Better gyroscopic behavior and validated high-mass-ratio profiles | Specialist scientific integration |
| Shapes | Box, sphere, capsule, plane, convex hull, compound shapes, local poses | Additional cylinder/cone/ellipsoid and compound editing where justified | Signed-distance/voxel collision |
| Static world | Cooked triangle meshes, heightfields, material IDs, sidedness, scale validation | Tile streaming, terrain edits, origin shifting, collision LOD contracts | Moving deforming concave collision |
| Narrowphase | Certified shape pair matrix, robust normals, persistent manifolds, degenerate-input handling | Improved manifold quality and dense-world optimization | Specialist geometry kernels |
| Contacts/materials | Static/dynamic friction, restitution thresholds, combine rules, contact offsets, stable stacking | Rolling/torsional friction and anisotropy where required | Custom constitutive material models |
| Fast motion | Per-body CCD with defined supported pairs, swept bounds, bounded TOI handling | Broader angular/compound CCD and high-speed vehicle profiles | General exact continuous concave collision |
| Queries | Closest/any/all raycasts, sphere/box/capsule sweeps, overlaps, filters, stable hit identity, overflow reporting | Batched/asynchronous queries, convex sweeps and scene-query snapshots | GPU queries |
| Events/filtering | Contact and trigger enter/stay/exit, wake/sleep, joint break, layer matrix, query/simulation masks | Contact modification and richer event payloads | User-defined solver plugins |
| Joints | Fixed/D6, independent linear/twist/swing axes, soft/hard limits, angular/linear drives, break/reaction data | Hinge, slider, ball/socket, distance/spring, gear and pulley presets/rows | Reduced-coordinate articulations |
| Kinematic control | Tick targets, moving-platform velocities, kinematic constraint follower mode, mode transitions | Mixed prescribed/dynamic joint coordinates and richer trajectory controls | General closed-loop inverse kinematics |
| Characters | Capsule sweep/slide, grounding, step/slope limits, gravity/jump, moving platforms, bounded depenetration | Crouching, stairs/ledge cases, push policies, swimming/climbing integration | Crowd avoidance/navigation solver |
| Vehicles | Preserve current suspension rays, force-at-point, material traction and transform synchronization | Batched wheel queries, richer collision/material telemetry, trailer constraints and validated handling profiles | Vehicle-specific GPU simulation |
| Animation | Explicit body/animation transform ownership and safe handoff hooks | Ragdoll setup, active ragdolls, blend-in/out, hit reactions | Muscle/tissue simulation |
| 2D | Correct planar motion, circle/box/convex polygon and static edge/chain collision, queries, events and core joints | Full 2D motor/limit/preset coverage and authoring | Specialist 2D deformables |
| Environmental forces | Gravity overrides and force/torque utilities | Buoyancy volumes, fluid drag, wind/force fields and water-service integration | Fluid simulation |
| State/networking | Stable IDs, deterministic same-build test mode, command capture and diagnostics | Versioned snapshots, restore/replay, server authority and correction hooks | Bitwise cross-platform rollback guarantees |
| Assets/tools | Versioned collision cooking, last-good reload, colliders/joint gizmos, mode controls, undo/save/reopen | Hull decomposition tools, ragdoll editor, streaming visualization and replay tools | Runtime fracture authoring |
| Operations | Dedicated CI, native/wrapper/integration tests, fuzzing, memory checks, benchmarks and packaging | Performance dashboards, certified worker-count profiles and broader platforms | Independent new backend certification |

## 5. Kinematic bodies and kinematic constraints

### 5.1 Proposed control modes

Joint **control mode** is separate from joint **type** and per-axis **motion/limits**. Changing a D6 joint to kinematic control must not erase its authored joint frames or limits.

| Mode | Body behavior | Contact and joint response | Typical use |
|---|---|---|---|
| Dynamic | Native solver enforces the configured joint | Both dynamic bodies respond according to mass/inertia | Hinges, chains, suspension, freely moving mechanisms |
| Servo | Dynamic body follows pose/velocity targets through finite force/torque drives | Contacts and loads can prevent exact target tracking | Grabbed objects, powered doors, active ragdolls |
| Kinematic | A designated follower follows prescribed joint coordinates/poses | Follower motion has no physical recoil; dynamic objects react to its contact velocity | Animated doors, lifts, conveyors, scripted machinery |

Use an additive, versioned control descriptor containing mode, follower body, target space, pose/velocity target, active coordinates, speed/acceleration bounds, collision policy and release-velocity policy. Names such as `ConstraintControlMode` and `ConstraintControlDesc` are proposed API names, subject to the M0 compatibility audit.

For R1, kinematic constraint mode owns a **fully prescribed follower pose**. Enabling it explicitly acquires that body's kinematic motion ownership; validate and show this in the Editor. Preserve its mass/inertia and prior body settings for later release. Do not turn both endpoints kinematic or silently change unrelated attached bodies. Dynamic/servo mode retains dynamic response.

R1 supports world/static/kinematic reference frames and acyclic kinematic chains. A fully specified D6 trajectory can combine locked coordinates with targeted coordinates inside limits. Unspecified free coordinates, conflicting owners, dynamic reference chains and loops are rejected with actionable diagnostics; use dynamic/servo control for these until R2's mixed-coordinate solver is certified. This makes the initial mode useful without claiming a general articulated inverse-kinematics solver.

### 5.2 Motion contract

1. `setKinematicTarget()` queues a target for the next fixed tick. The target getter returns the pending target; body transform getters report the last committed state until stepping completes. Preserve an explicit teleport operation for immediate placement at a safe mutation boundary.
2. Derive linear and angular velocity from previous and target poses over the fixed tick, with shortest-arc quaternion handling. Use center-of-mass motion consistently; contact-point velocity includes angular motion.
3. Interpolate the trajectory across internal substeps. Consume the target once per fixed tick. Do not apply gravity, forces, damping or an additional velocity integration to the prescribed trajectory.
4. With no new target, hold the committed pose and publish zero target velocity. A distinct velocity-control option may continue motion when explicitly selected.
5. Define joint targets in joint-local coordinates relative to actor A by default, with explicit world-space conversion. Include frame orientations and local anchor offsets when computing the follower pose.
6. Enforce joint limits before building a trajectory: clamp under an explicit policy or reject an invalid target. Report the accepted target and the applied result. Limits apply in all control modes.
7. For the initial kinematic follower, collision policy is **prescribed motion**: dynamic bodies are pushed; the follower is not stopped by collision impulses. Static/kinematic obstruction is reported and may prevent a safe path. An optional stop-at-obstruction mode is enabled only after matching shape sweeps, initial-overlap and angular-motion behavior pass certification.
8. Do not label a force-limited actuator kinematic. Finite force/torque limits belong to servo mode. A follower that must yield under load uses servo mode.
9. Apply mode changes atomically at a tick boundary. On kinematic-to-dynamic release, default to preserving the last measured trajectory velocity, with explicit zero/custom alternatives. Clear incompatible accumulated forces and cached impulses, validate overlaps, and wake affected islands.
10. Kinematic ownership is exclusive. Reject multiple simultaneous target owners, animation/physics transform feedback, cyclic follower chains and incompatible joints; diagnose the relevant bodies/constraints.
11. Define reaction reporting honestly. Dynamic/servo break thresholds use accumulated solver impulses divided by timestep. Kinematic blocked-path reports are distinct from a measured joint reaction; synthetic follower motion must not fabricate a physical break force.
12. Buffer blocked, mode-changed and target-rejected events. Publish them with tick/body/joint IDs outside the simulation lock.

These semantics are consistent with the distinction between target-driven kinematic bodies and force-driven joints documented by [PhysX rigid-body dynamics](https://nvidia-omniverse.github.io/PhysX/physx/5.8.0/docs/RigidBodyDynamics.html) and [PhysX joints](https://nvidia-omniverse.github.io/PhysX/physx/5.4.0/docs/Joints.html). They are proposed Workphone contracts, not a promise to copy that backend.

### 5.3 Implementation work

- Native body API: pending target, previous/committed pose, derived velocities, target revision and explicit teleport/reset behavior.
- Native constraint API: control descriptor, selected follower and validated target/frame conversion; dedicated trajectory preparation before contact solving.
- Native solver: omit recoil on the fully kinematic follower while retaining its velocity in contact/joint relative-velocity equations; support dynamic/servo rows against kinematic anchors.
- Constraint graph: validate ownership and ordering, evaluate acyclic follower chains in a deterministic topological order, and reject unsupported graph structures.
- C++ adapter: consolidate duplicate joint implementations, forward descriptors without per-frame allocations, and preserve load/unload and body-removal semantics.
- Constraint component: serialize mode, target settings and policies; migrate existing constraints to dynamic mode without changing authored limits.
- Editor/Lua: expose target setters and mode controls, joint frame/trajectory gizmos, blocked diagnostics, undo/redo and sample mechanisms.
- R2 mixed-coordinate work: solve dynamic free coordinates while prescribing selected coordinates using a constrained effective-mass formulation; do not approximate this by setting the entire body's inverse mass to zero.

Required demonstrations: a lift carrying a dynamic crate; a rotating platform carrying a character; a scripted hinged door pushing a body; a servo door stopping under load; a world-anchored constraint; an acyclic mechanism; and a kinematic-to-dynamic release preserving velocity. Each requires numerical assertions and an interactive scene check.

## 6. Ordered implementation milestones

Effort is intentionally not assigned calendar dates before the numerical baseline. M0 produces estimates from measured gaps. Each milestone should be delivered as small reviewable changes with its own tests; a later milestone does not excuse an earlier failing gate.

### M0 — Establish the executable contract and test harness

**Work:** Inventory every public API against its actual native path; select supported C dialect/toolchain/build configurations; document units and initial numerical limits. Trace all factories, registration, solver configuration, broadphase selection and 2D paths. Consolidate adapter ownership design. Add dedicated physics test targets independent of graphics, audio and application singleton startup. Register individual scenarios or report their execution explicitly. Add version/capability reporting.

**Files:** Native/C++ physics project CMake files, `Tests/c`, `Tests/cpp/UnitTests`, a proposed `Tests/PhysicsProduction` directory, and a proposed physics CI workflow.

**Exit gate:** Fresh Debug and RelWithDebInfo builds of native physics, WPPhysics and required Workphone dependencies; CTests execute all mandatory scenarios without availability skips. Save source and executable hashes, toolchain, options, fixtures and test counts. Produce a feature status matrix: implemented/partial/unsupported/unverified.

### M1 — Fix lifetime, state ownership and time stepping

**Depends on:** M0.

**Work:** Add one fixed-tick accumulator and explicit manual-step path; validate inputs; bound catch-up and expose dropped/deferred time. Separate gameplay ticks, render frames and collision substeps. Define force accumulation across ticks/substeps and clearing on pause. Add stable handles/generations, deferred mutation queues, deterministic destruction order and safe scene teardown. Make shape/joint/body ownership explicit, including shared collision assets. Publish state/events outside locks. Connect scene configuration to actual execution.

**Exit gate:** Identical fixed-tick results under 30/60/144 Hz presentation schedules; documented hitch policy; no double integration or lost force. Repeated create/remove/clear/reload and callback-driven removal pass. Allocation-failure injection and stale queued commands leave valid state; memory checks find no physics-owned leaks or invalid accesses.

### M2 — Implement correct kinematic rigid-body targets

**Depends on:** M1.

**Work:** Implement section 5.2 target/teleport semantics in native C and both wrappers. Derive angular/linear contact velocities, interpolate substeps, handle sleeping contacts, and provide atomic dynamic/kinematic transitions. Reject conflicting animation/component updates. Retain regression coverage for vehicle transform publication.

**Exit gate:** Translating/rotating platforms push and carry dynamic bodies using the correct contact velocity; no-target hold is stationary; targets are consumed once. Results are stable across render schedules and internal substep counts. Release velocities, wake behavior, pauses and teleports pass tests.

### M3 — Complete collision geometry, broadphase and queries

**Depends on:** M1; uses M2 swept trajectories.

**Work:** Write the shape-pair support matrix and certify existing primitive/mesh paths. Add validated convex hull cooking and convex narrowphase, compound transforms, heightfield behavior and mesh sidedness/material IDs. Introduce stable shape/feature IDs and geometry revisions. Replace body-only contact cache identity with ordered shape pairs and consistent normal orientation. Recompute on rotation, scale, shape/filter changes, teleport and removal.

Connect a production broadphase, initially a measured dynamic AABB tree or SAP selected from benchmark results. Retain brute-force as a correctness oracle for small scenes. Deduplicate grid/partition pairs and handle huge/out-of-bounds objects. Implement unified closest/any/all raycasts, overlaps and sphere/box/capsule sweeps with body/shape/triangle/material identity, distances, normals, initial-overlap semantics, filters and capacity/overflow results.

**Exit gate:** Every advertised pair has separated/touching/penetrating/rotated/scaled cases and symmetry checks. Compound shapes never reuse another shape pair's manifold. Broadphase pairs match the oracle on randomized scenes, including movement/removal/filter changes. Query results agree with analytic fixtures and independently evaluated mesh triangles; dense candidate results are never silently truncated.

### M4 — Replace ad hoc resolution with a stable contact/joint solver

**Depends on:** M1–M3.

**Work:** Extract contact/joint assembly and solving from the monolithic scene into focused native modules. Implement sequential impulses/PGS with Jacobians, world-space inverse inertia, anchor lever arms, accumulated impulse clamping, warm starting, friction/restitution and bounded position correction. Use actual iteration settings. Build deterministic islands and island-level sleep/wake. Preserve useful existing narrowphase behavior while migrating the solver behind focused comparisons.

Implement fixed/D6 independent linear and twist/swing rows; asymmetric angular limits, soft limits, linear/angular pose and velocity drives, implicit spring behavior, force/acceleration semantics and measured break/reaction data. Limit motor impulse using force or torque times the **actual substep duration**, with a documented tick-wide budget so repeated iterations/substeps cannot multiply the allowed actuation. Resolve hard limits and contacts together; projection is an explicit bounded recovery option, not routine teleport solving.

Add per-body CCD using swept candidate bounds and a defined time-of-impact/speculative strategy. Publish supported pairs and remaining angular/mesh limitations. Keep adaptive substeps as a quality tool, not the sole guarantee against tunneling. Hitting an iteration/TOI budget must have a conservative, reported outcome.

**Exit gate:** Stable stacks, slopes, friction/restitution, offset-COM bodies, off-center impulses, joint chains and advertised mass-ratio cases pass duration-based tests. Independent twist/swing/linear settings change actual motion. Drive limits and joint break thresholds remain within tolerances across timesteps/substeps. Fast thin-wall and rotating-body cases pass the supported CCD matrix. Existing road/barrier, suspension-normal and moved-static wake regressions remain passing.

### M5 — Deliver kinematic constraint control and core gameplay integration

**Depends on:** M2–M4.

**Work:** Implement section 5 control modes, ownership graph and target preparation. Consolidate C++ adapters and expose versioned control through components/Lua. Add buffered contact/trigger/break/wake/sleep events with a documented ordering/removal policy. Generate sensor events independently of whether either endpoint receives impulses; include static/kinematic sensor interactions allowed by filtering.

Implement capsule sweep/slide, bounded depenetration, slope/step limits, grounding and moving-platform support. Verify WPVehiclePhysics uses force-at-point and query semantics consistently; keep current wheel/tire/powertrain code in its module. Complete planar 2D circles/boxes/convex polygons, edge/chain world collision, query/event behavior and fixed/revolute/prismatic baseline joints with one authoritative 2D path.

**Exit gate:** All section 5 demonstrations pass. Character tests cover walls, corners, stairs, steep slopes, ceilings, initial overlap and translating/rotating platforms. Triggers report exactly one begin/end lifecycle and do not apply contact impulses. Vehicle samples drive, brake and collide without transform feedback. 2D bodies retain zero hidden-axis motion and correct angular inertia.

### M6 — Ship collision assets and Editor workflows

**Depends on:** M3–M5; asset schema work can start after M0.

**Work:** Add versioned mesh/hull/heightfield cook formats with source hash, settings, units/scale, geometry validation, material mapping and cook-version identity. Reuse catalog/ResourceSystem ownership contracts rather than adding a parallel database. Cook asynchronously, validate the result, and publish immutable collision assets at tick boundaries with last-good fallback. Cancel stale scene/reimport work using generations.

Extend existing collider and Constraint components and Editor properties/gizmos. Include body mode, density/mass/COM, layers, trigger settings, joint frames/limits/drives/control mode, validation messages and profiler overlays. Support undo/redo, duplication, prefab migration, save/reopen, Play/Stop restoration and repeated reimport. Follow existing [Editor conventions](../Tools/cpp/Editor/docs/CONVENTIONS.md).

**Exit gate:** Author a mechanism and collision world in the Editor, save/reopen, run/stop and restore authored state. Reimport the same collision source at least three times with changed scale/settings; runtime queries and collision reflect each change. Failed cooking preserves the last valid asset. Reload/scene cancellation cannot publish stale geometry or leak handles.

### M7 — Bound memory and establish measured scalability

**Depends on:** M4–M6.

**Work:** Introduce scene reservations/budgets for bodies, shapes, pairs, manifolds, solver rows, events, query results and scratch storage. Remove steady-state allocations from stepping after reservation; document allocations for scene edits and deferred publication. Expose overflow and exhaustion results. Record per-phase times, counts, cache hit rates, broadphase false positives, solver residuals, CCD retries and dropped/deferred time.

Optimize from profiles. First certify single-thread execution; then parallelize independent islands and safe narrowphase batches using the engine job system. Use deterministic ordering in replay mode. Thread-count setters must control a real supported execution profile or return unsupported. Avoid parallelizing shared-body solver rows without an explicit scheduling design.

**Exit gate:** Benchmarks meet the budgets in section 8 with no lost pairs/events and no hidden fallback blowups. Advertised worker counts pass race/lifetime testing and repeated replay checks. Teardown cancels and drains outstanding jobs. Serial execution remains a supported correctness reference.

### M8 — Certify and package R1

**Depends on:** M0–M7.

**Work:** Run the complete matrix in section 7 on clean builds, then package headers/libraries/runtime dependencies, cooked fixtures, symbols and samples. Document API migration, known numerical limits and the capability matrix. Add a minimal C consumer and C++ consumer, validate plugin loading/unloading, and certify the Editor/runtime path.

**Exit gate:** Every required R1 scenario has current source/executable evidence in both configurations; no missing executable or unavailable backend counts as passing. No unresolved release-blocking crashes, data races, physics-owned leaks, silent capacity loss or unsupported successful no-ops. Record manual visual evidence separately from numerical and automated results.

### M9 — Implement the comprehensive R2 feature set

**Depends on:** R1 solver and ownership contracts; individual streams have these additional prerequisites.

| Stream | Implementation | Required evidence |
|---|---|---|
| Joint library | Hinge/slider/ball/distance/spring presets over tested rows; gear/pulley where new equations are required; richer limits/friction | Motors under load, ratios, limits, chains, loops within published solver limits |
| Mixed-coordinate kinematics | Prescribed selected joint coordinates with dynamic remaining DOFs; effective-mass changes, ownership and animation blending | Locked/prescribed/free combinations, contacts and handoff with no hidden full-body mass change |
| Ragdolls/animation | Skeleton-to-body mapping, collider authoring, joint limits, active drives, pose ownership and blend transitions | Drop/recover, animated contact, ragdoll handoff, save/reopen and stable pose publication |
| Characters | Crouch clearance, richer stairs/ledge handling, pushes and platform detach behavior; gameplay hooks for swimming/climbing | Parameterized obstacle suite, moving platforms, blocked resize and frame-rate independence |
| Vehicles | Batched wheel queries, per-surface traction, suspension telemetry, trailers and handling regression scenes | Braking/cornering/curbs/slopes at multiple tick profiles, query budgets and repeatable handling metrics |
| Environment | Buoyancy volumes and sample-based forces/torques, linear/angular drag, force fields and water-height/velocity adapter | Neutral float, displacement changes, off-center buoyancy torque and repeatable water sampling |
| Large worlds | Chunked static collision, origin shift, streaming lifetime, joint/query coordinate conversion | Actors crossing tile boundaries, moving origins, in-flight queries and stale cook cancellation |
| Replay/network hooks | Versioned full-state snapshots and command streams; server authority/correction APIs | Restore/replay includes sleep, joint/drive state, pending targets, caches or deterministic reconstruction and handle mappings |
| 2D completion | Rich motor/limit/spring presets, chain-edge adjacency behavior and tools | 2D-specific stacking, fast motion, joints and shape/query parity reports |
| Scale and tools | Certify additional worker profiles, batched queries, ragdoll/replay tools and production dashboards | Numerical equivalence within published contract, race tests and p99 budget evidence |

R2 ends with a new full certification pass. Serial R1 passing results do not certify multithreaded R2 execution, and a Windows result does not certify another platform.

## 7. Validation matrix and definition of done

Proposed tolerances below are starting acceptance targets for meter-scale fixtures. M0 must freeze them alongside shape/mass/coordinate ranges; tuning must be explained, not used to hide regressions.

| Suite | Required cases | Acceptance |
|---|---|---|
| API/lifetime | Null/invalid descriptors, stale handles, allocation failures, shared shapes, body/joint removal, scene clear/reload | Explicit results; no invalid access or leaked ownership; repeated teardown is safe |
| Time/forces | 30/60/144 Hz render schedules, pause/single-step, hitch/catch-up, force vs impulse, zero/NaN/infinite dt | Same fixed-tick command stream gives the same same-build result; time loss/debt is observable |
| Geometry | Every supported pair, transform/scale, winding, degenerate hull/triangles, compound offsets | Correct overlap and normal orientation; finite outputs; declared unsupported pairs fail clearly |
| Cache/broadphase | Rotation-only motion, multiple shape pairs, filter edits, remove/reuse IDs, world bounds | Matches brute-force candidate oracle; no stale manifold or duplicate physical resolution |
| Contacts | 10-box stack, slope rest, restitution drop, friction slide, offset COM, mass ratios up to an initially proposed 100:1 | Finite/stable for at least 60 simulated seconds; proposed rest penetration <= 1 cm and no unbounded drift/energy growth |
| Constraints | Fixed anchor, independent 6 axes, asymmetric angular limits, offset anchors, drives, world anchor, chains and break | Proposed ordinary-fixture error <= 5 mm/1 degree after settling; force/torque units and limits verified |
| Kinematic control | Hold, translation/rotation, chained follower, target rejection, ownership conflicts, mode release | Target reached within declared path policy; correct contact velocity; no double step or transform feedback |
| CCD | Thin walls, primitive/static mesh, dynamic pairs where supported, translation+rotation, multiple impacts | No pass-through for every advertised pair within published speed/size limits; budget exhaustion reported |
| Queries | Any/closest/all, inside start, zero length, rotated compounds, masks, mesh hit ID, overflow | Analytic/reference agreement; proposed simple-fixture distance error <= 1 mm; no silent hit loss |
| Events | Contact/trigger begin/stay/end, filtered pairs, removal, reload, break/wake/sleep, callback mutation | Exactly defined lifecycle, stable IDs and delivery order; no callback under simulation locks |
| Characters/vehicles | Steps/slopes/corners/platforms; suspension rays, braking and track barriers | Numerical assertions plus interactive sample evidence; preserve existing vehicle regressions |
| 2D | Circle/polygon/edge pairs, joints, fast motion, moving platforms, event/query parity | Planar invariants, 2D inertia and the published independent feature matrix pass |
| Assets/Editor | Reimport repeatedly, failed cook, scale changes, undo/redo, prefab, save/reopen, Play/Stop | Last-good state preserved; accepted edits reach runtime; authored state restores correctly |
| Concurrency | Mutations during stepping, queries, cancellation, scene switch, callbacks, worker-count changes | Documented synchronization; no race/use-after-free; deterministic mode passes its exact contract |
| Robustness | Seeded randomized scenes, malformed assets/descriptors, capacity exhaustion, long soaks | Finite state or explicit failure; reproducible seed/command log; no unbounded memory growth |
| Packaging | Clean C/C++ consumers, static/shared configurations if supported, plugin unload/reload | Complete compile/link/start/step/shutdown evidence with correctly versioned dependencies |

Test layers:

1. **Native isolated tests:** geometry, solver, constraints, queries, materials, stepping and failure injection; no renderer or application singleton dependency.
2. **C++ adapter tests:** real WPPhysics wrappers against native state, factories, interface conversion and ownership.
3. **Engine integration tests:** components, asynchronous scene lifecycle, animation/vehicle transform ownership, resources and Lua.
4. **Interactive demonstrations:** Editor scenes for platforms, mechanisms, stacking, characters, vehicles and terrain. Capture the relevant state/debug overlays and list the exact manual checks.
5. **Nightly stress:** long seeded runs, replay divergence detection, concurrency stress, memory and performance tracking.

Mandatory physics tests must fail if their required backend/fixture is missing. Optional platform features may skip only with a reported reason and without contributing to that feature's certification. Replace smoke-only checks with behavior assertions as each feature lands.

Use supported address/undefined-behavior/race tooling on the certified toolchains; a platform without a suitable detector needs an explicit alternative. Separate fresh build/link, automated behavior, leak/crash, performance and manual visual evidence in release reports.

## 8. Performance and memory certification

These are proposed starting workloads, not current capacity claims:

| Profile | Workload | Initial proposed budget |
|---|---|---|
| Gameplay core | 1,000 dynamic bodies, 10,000 static collider instances, 200 active joints, 500 representative queries per 60 Hz tick | Physics + required queries p95 <= 4 ms, p99 <= 6 ms on a named reference CPU |
| Mechanisms/characters | 100 kinematic movers, 100 characters, 500 joints, mixed trigger/contact traffic | Same frame budget with target/graph/query/event costs included |
| Vehicle scene | 16 four-wheel vehicles, cooked road/barriers/terrain, traffic props; 120 Hz profile | Proposed p95 <= 2 ms and p99 <= 3 ms per tick on the same reference CPU |
| Sleeping/streaming world | 10,000 mostly sleeping bodies, incoming/outgoing collision chunks and origin shifts | Near-zero sleeping solve work; bounded measured streaming and wake spikes |
| Capacity/soak | Declared maximum reserved pairs/rows/events/results plus adversarial dense overlap | Explicit overload outcome, no silent pair loss, crash or growing allocation backlog |

M0/M7 may revise workloads and budgets using product requirements and measured hardware, before certification. Record CPU, worker count, clock/power settings, build flags, scene hashes, warm-up, duration and p50/p95/p99/max. Count active shapes, contact points, rows and queries: actor count alone is misleading. Include dense and sparse scenes, mesh hotspots and worst-case target velocities.

Reserve configurable scene memory; require zero unplanned allocations during steady-state native stepping after warm-up and within reservation. Track C++ snapshot/event/query allocation separately and bound it. Exceeding a budget must return a status or enact an explicitly documented conservative fallback. No universal hard body limit or throughput claim should be published before these tests.

Fixed stepping helps reproducibility, but deterministic ordering and state reconstruction also matter. [Box2D simulation documentation](https://box2d.org/documentation/md_simulation.html) provides reference guidance for fixed steps and deterministic simulation ordering; WPPhysics must establish its own guarantees through tests.

## 9. Delivery dependencies and first implementation increment

Critical path:

```text
M0 baseline -> M1 ownership/clock -> M2 kinematic bodies
                           \-> M3 geometry/queries
M2 + M3 -> M4 solver/joints/CCD -> M5 constraint control/gameplay
M3 + M5 -> M6 assets/tools -> M7 measured scale -> M8 R1 certification
R1 contracts -> M9 comprehensive streams -> R2 certification
```

Asset schema and Editor designs can proceed after M0, but their runtime publication depends on M1. Kinematic targets can land before the solver replacement; production certification waits for contact, joint and gameplay gates. Do not schedule multicore solver optimization ahead of stable serial behavior.

The first implementation increment should be:

1. Add isolated native timestep/kinematic/constraint test executables and mandatory CTest registration.
2. Capture a fresh source-matched Debug/RelWithDebInfo baseline and API capability inventory.
3. Introduce validated scene stepping configuration and the fixed-clock boundary.
4. Replace immediate kinematic target placement with tick-consumed native targets and measured contact velocity.
5. Deliver a lift/crate and rotating-platform regression scene, plus explicit dynamic/kinematic release tests.
6. Follow with persistent shape-pair contacts and the joint solver foundation; implement the constraint control modes on that foundation.

For local builds, honor the configured compiler concurrency budget; this checkout currently uses `WP_MSVC_COMPILE_PROCESSES=1`. Avoid overlapping builds of the same targets. Rebuild Workphone and dependent modules when interface/DLL changes make artifacts stale. Never count an old executable's success as proof of a changed source build.

## 10. Principal risks and decision gates

| Risk | Mitigation and decision |
|---|---|
| Native numerical solver scope is larger than the wrapper API suggests | Prototype stacks, mass ratios, joint chains and force-limited drives in M4 before estimating R2. If gates fail, narrow the published support envelope or explicitly evaluate another backend; do not silently replace WPPhysics |
| Kinematic control conflicts with contacts, animation or multiple joints | Exclusive ownership, explicit follower designation, validated graph restrictions, separate servo mode and published collision policies |
| Fixed-clock migration changes vehicles or gameplay timing | Record current representative behavior, move all consumers to tick-stamped inputs, and maintain vehicle/transform regressions |
| Adapter duplication produces inconsistent feature behavior | One canonical adapter path with factory/registration tests before extending descriptors |
| Shared-interface changes break plugins and import libraries | Add versioned extensions first; rebuild consumers and package compatibility versions when ABI changes are unavoidable |
| Cooking/streaming publishes stale geometry | Immutable assets, generation checks, tick-boundary replacement, last-good fallback and repeated reimport tests |
| Broadphase/cache optimizations lose contacts | Brute-force oracle, stable shape/feature identity, revision invalidation, capacity reporting and adversarial fixtures |
| Feature names overstate completed behavior | Capability inventory tied to tests; unsupported modes fail clearly; release notes list supported pairs/profiles and limitations |
| Passing smoke tests hide missing integration | Require isolated numerical tests, real wrappers, gameplay scenes and separate manual evidence |

R1 ships only after its feature matrix and release gates pass. R2 ships after the additional gameplay features have equivalent evidence. Optional specialist extensions are tracked independently, with their own implementation plans and certification criteria.
