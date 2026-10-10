# WorkphonePhysics narrowphase optimization review

9 October 2026. Review of native C collision generation, scene contact consumption, mesh acceleration, and static-body integration. The original review did not change production code. The approved implementation is recorded in the [10 October follow-up](#implemented-follow-up-10-october-2026) below.

Reviewed source: `a6494f69b3295b7726e8aeec1bf71f3cdfdedb95`. Existing local changes to `ProceduralRaceSceneBuilder.cpp` were inspected and preserved. Its generated-collider findings describe the current working copy, rather than an audited running Editor scene.

## Main findings, in priority order

| Priority | Finding and evidence | Proposed change |
|---|---|---|
| High | [Mesh candidate overflow](../Engine/c/Source/WorkphonePhysics/workphone_physics_narrowphase.c#L283): every primitive/mesh test has a 1,024-index stack buffer. Above that capacity, the code finishes the BVH query and then scans every original triangle. A 32,768-triangle fixture jumped from 0.75 to 20.93 ms/pair at this boundary. | Stream candidates through a visitor or use reusable, checked growable scratch storage. Keep complete results and allocation-failure fallback; increasing the fixed limit only moves the cliff. |
| High | [Scene manifold cache](../Engine/c/Source/WorkphonePhysics/workphone_physics_scene.c#L1671): lookup scans all 1,024 slots and identifies only the two bodies. Successful generation repeats the lookup, and insertion may scan again. Default `ALWAYS` still incurs this bookkeeping while recomputing geometry. `FIXED`/`DISTANCE` can reuse another child shape's manifold, and refresh checks ignore orientation and geometry changes. | First correct shape-pair identity and invalidation. Use a canonical body/shape key with lifetime generations and a hash table/free list. Avoid cache lookup/write under `ALWAYS` when reuse is disabled; clear/repopulate when changing strategy. Retire pairs not seen in the current collision epoch. |
| High | [Mesh candidate shape](../Engine/c/Source/WorkphonePhysics/workphone_physics_narrowphase.c#L1277): a box queries using its enclosing sphere; a capsule uses `half_height + radius`. These bounds are loose for long vehicles, barriers, and capsules. In the threshold fixture the sphere selected 968/1,032 triangles, whereas a tight box AABB selected 96/104. | Keep sphere queries for spheres. Add conservative mesh-local AABB queries for boxes and swept-segment/capsule bounds. Use OBB/capsule node tests only if their rejection savings exceed the extra traversal cost. |
| High | [Per-triangle transforms](../Engine/c/Source/WorkphonePhysics/workphone_physics_narrowphase.c#L241): each candidate re-fetches mesh data, recomposes the shape pose, and normalizes quaternions during all three vertex rotations. Box/mesh immediately rotates those vertices into box space again. | Prepare poses once per pair. Test sphere/capsule contacts in mesh local space; use a single mesh-to-box transform for box/mesh. Transform accepted contact points/normals back to world space. Cache static prepared geometry with explicit mutation invalidation. |
| Medium | [Compound shape-pair enumeration](../Engine/c/Source/WorkphonePhysics/workphone_physics_scene.c#L1745): actor-level overlap leads to every enabled/filter-compatible child combination being tested. The scene discards individual shape AABBs after building the body's union bounds. Widely spaced children can produce many unnecessary exact tests. | Retain prepared child bounds and reject separated child pairs before cache lookup/geometric dispatch, expanded conservatively for contact tolerance. For very large compounds, evaluate a child hierarchy. Refresh bounds after pose changes from earlier contacts. |
| Medium/high | [Capsule/box](../Engine/c/Source/WorkphonePhysics/workphone_physics_narrowphase.c#L1113): a fixed 32-iteration ternary search performs 64 segment-point/AABB distance evaluations before the final evaluation, including for misses. The configured narrowphase iteration limit does not control it. | Replace with an analytic or bounded piecewise segment/AABB distance calculation in box space. Preserve or explicitly improve the embedded-capsule depenetration rule; test tangent, parallel, zero-length, and endpoint cases. |
| Medium | [Box/box SAT](../Engine/c/Source/WorkphonePhysics/workphone_physics_narrowphase.c#L883) and [triangle/box SAT](../Engine/c/Source/WorkphonePhysics/workphone_physics_narrowphase.c#L533) normalize every usable axis and repeatedly calculate projections. Box/box also rebuilds both bases on each call. | Use relative rotation/absolute rotation matrices for box/box, direct coordinate bounds for triangle/box face axes, and unnormalized separating tests where appropriate. Scale tolerance by axis length and preserve normalized penetration-depth comparisons and near-parallel handling. |
| Medium | [Duplicate mesh acceleration](../Engine/c/Source/WorkphonePhysics/workphone_physics_triangle_mesh.c#L432): creation/refit builds a flat BVH for raycasts and a separate pointer tree for collision candidates. The latter copies triangle vertices and [recomputes leaf triangle AABBs](../Engine/c/Source/WorkphoneCollision/workphone_collision_aabbtree.c#L332) during queries. | Extend one contiguous mesh BVH to support ray, sphere, and AABB traversal, retaining public wrappers. Store cooked triangle bounds/indices and optionally edges/normals. Refit bounds without rebuilding topology when appropriate. Evaluate immutable cooked-data sharing for repeated mesh instances. |
| Lower | [Contact material combination](../Engine/c/Source/WorkphonePhysics/workphone_physics_narrowphase.c#L610) repeats material getters, friction square roots, and restitution combination for each accepted/replacement contact. Dispatch is a long chain of type comparisons. | Compute material combination once per shape pair when its first contact is accepted. Consider a dispatch table for coverage and maintainability after measuring dispatch cost; it is unlikely to be the first bottleneck. |

The scene calls `wp_narrowphase_test_pair` directly inside `solve_broadphase_pair`. Optimizing only `wp_narrowphase_process_pairs` would not improve scene simulation. Several `workphone_physics_collision_*_tests` files are standalone utility APIs or wrappers; they are not the active primitive contact dispatch.

## Correctness issues that limit caching and optimization

The body-only cache issue was reproduced with one dynamic body carrying two radius-0.5 spheres, at local Y=0 and Y=10, above a static plane. Starting at body Y=0.4, with gravity zero, only the lower sphere touches. After one step:

- `ALWAYS`: Y=0.479200.
- `FIXED`, frequency 100 and separation threshold 1,000 to isolate reuse: Y=0.558400.

The second child reused the first child's contact and applied an additional correction. The default `ALWAYS` strategy regenerates contacts, so this particular reuse error is confined to optional reuse strategies. Linear scans and overwriting of a body's different child manifolds affect default-path bookkeeping too. A separated pair that stops appearing in the broadphase is not aged out by the current cache; stale entries can occupy slots indefinitely until another explicit invalidation occurs.

Persistent contact reuse must refresh local anchors/normals against current poses, discard broken contacts, and regenerate when features cease to be valid. Replaying old world-space contacts for a fixed number of frames is insufficient. The existing bounds revision is useful for detecting changes, but persistent contacts need geometry/filter/material validity distinguished from ordinary pose motion. The approach is consistent with [PhysX's explanation of persistent contact manifold refresh](https://nvidia-omniverse.github.io/PhysX/physx/5.4.1/docs/AdvancedCollisionDetection.html#persistent-contact-manifold-pcm); its performance claims are not Workphone measurements.

The separate public batch API has two additional reproduced issues: 46 mutually overlapping spheres generate 1,035 distinct pairs but return only 1,024 manifolds, and a following empty batch leaves the old 1,024 count visible. Fix checked growth/streaming and empty-input reset when consolidating that API. These limits do not truncate scene simulation, which streams single-pair tests.

The `GJK_EPA`, `SAT`, and `HYBRID` enum is stored but does not select different geometric implementations. `max_iterations` is also stored and never consulted by the active kernels. Changing those settings currently cannot improve performance; the supported primitive cases use their dedicated tests, and dynamic mesh/mesh remains intentionally unsupported.

## Static actors: what already works and what remains

Static is an explicit body type: `WORKPHONE_RIGIDBODY_STATIC`, distinct from dynamic and kinematic. The engine path is:

1. [Rigidbody component](../Engine/cpp/Source/Workphone/Scene/Components/Rigidbody.cpp#L500) checks `actor->isStatic()` and chooses `addRigidStatic` or `addRigidDynamic`. A static-flag change recreates the native body.
2. [WPPhysicsRigidStatic3](../Engine/cpp/Source/WPPhysics/WPPhysicsRigidStatic3.cpp#L30) creates a native static body. Stored mass alone does not determine whether this body is static.
3. [Generated trackside colliders](../Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp#L190), city building colliders, and city boundaries explicitly call `setStatic(true)` before adding their Rigidbody component.

Native integration exits for static bodies. Motion-substep estimation skips them. Bounds are reused while their revisions remain unchanged. Broadphase movable flags derive from dynamic inverse mass, so static/static, static/kinematic, and kinematic/kinematic response pairs are pruned; dynamic/static and dynamic/kinematic remain eligible. Sleeping dynamics remain eligible so moved static geometry can wake them. Static/static narrowphase tests are therefore already avoided in normal scene simulation.

Remaining static-heavy costs:

- [Simulation](../Engine/c/Source/WorkphonePhysics/workphone_physics_scene.c#L1988) still walks all actors every substep and calls the integration function before it exits for each static actor.
- Bounds synchronization still checks every actor's revision/state and configures every proxy per substep, even when the static tree is unchanged.
- [Pair traversal](../Engine/c/Source/WorkphonePhysics/workphone_physics_broadphase.c#L573) recomputes descendant ranks and starts a query for each enabled leaf, including static leaves. All-static queries terminate quickly; they still perform traversal setup and bookkeeping.
- [End-of-step bookkeeping](../Engine/c/Source/WorkphonePhysics/workphone_physics_scene.c#L1800) reads accumulated force/acceleration/torque before checking for a dynamic body, resets sleep time, and clears force/torque for static actors.
- [C++ transform publication](../Engine/cpp/Source/WPPhysics/WPPhysicsScene3.cpp#L404) copies the entire actor list, including static shared pointers, and fetches native objects before skipping statics. The snapshot protects callback-driven removal and must remain safe after optimization.
- Static shape poses, bases, and triangle transforms are still recomputed during dynamic/static narrowphase tests. Static classification alone does not eliminate this work.

An active-body list plus a dirty-static list could reduce these costs. It needs notifications for body type, mass/response eligibility, enable changes, pose/shape edits, removal, and wake/sleep transitions. Current bounds revisions do not notify the scene of every eligibility change, so skipping all polling without adding that contract would miss updates. An all-immovable-root early return is a smaller first improvement.

For very large tracks, evaluate separate static and moving trees using shared tree code. Query dynamic bodies against both, deduplicate dynamic/dynamic pairs, and retain static geometry for raycasts and wake-up contacts. Preserve canonical pair consumption order: simply dropping static query origins from the current rank-based algorithm can lose dynamic/static pairs, and reordering contacts changes the immediate solver's behavior.

No running Editor actor counts or live scene properties were inspected. The source proves how reviewed generated actors are marked; it does not prove every loaded/imported object in the user's current scene is static. Add diagnostic counts for static/dynamic/kinematic bodies, dirty static updates, and candidate shape types to make such mistakes visible.

## Measured baseline

Windows x64, MSVC RelWithDebInfo native library, Intel Core i5-1135G7. Harness compiled with `/O2 /Ob1 /MD /std:c17`, linked to freshly checked native libraries. Five timed samples per fixture, median reported; fixed poses, setup excluded, no contact solving in single-pair measurements. Primitive fixtures run 300,000 calls/sample; the small mesh runs 3,000; threshold fixtures run 100. Host load changes absolute timings.

| Fixture | Median microseconds/pair |
|---|---:|
| Sphere/sphere overlap | 0.2377 |
| Sphere/box overlap | 0.4319 |
| Rotated box/box overlap | 1.1466 |
| Rotated capsule/box overlap | 3.3936 |
| Capsule/box miss | 2.8381 |
| Small box / 32,768-triangle mesh | 12.1461 |
| Long box / mesh, 968 sphere candidates | 749.4230 |
| Slightly longer box / same mesh, 1,032 sphere candidates | 20,927.1380 |

Mesh fixture: a 128x128 grid of unit cells, two triangles per cell, horizontal at Y=0. The box is centered at (0,0.25,0), with half-height/half-width 0.5. Half-lengths are chosen so the query sphere radii including tolerance are 12 and 12.125. Tight box AABBs overlap 96 and 104 triangle AABBs respectively, independently counted from original vertices. The latter sphere query exceeds the 1,024-index capacity and causes all 32,768 triangles to be tested: approximately a 28x timing jump. Both tests report contacts; this is a performance cliff, not lost candidates.

Initial mesh cooking, including both current trees, took 66.96 ms in this run. That is setup evidence, not per-frame collision cost. A previous run gave 78.25 ms cooking and 0.85/22.97 ms at the capacity boundary; the cliff persisted despite host timing variation.

The review harness and raw output are saved at `C:/Users/z_des/AppData/Local/Temp/workphone-narrowphase-review-1791572732322/` (`review.c`, `build.cmd`, `results.txt`). From that directory, `cmd /c build.cmd` rebuilds/runs the fixtures. These temporary artifacts are supplementary; the fixture definitions and measured conclusions are recorded here.

## Implementation sequence and acceptance checks

1. **Establish focused narrowphase tests and counters.** Add per-shape-pair call/time counts, cache hits/misses/probes, mesh BVH nodes/candidates/tested triangles, overflow fallbacks, and contact generation/reduction counts. Keep timing benchmarks separate from correctness assertions. Move reproduced cache and batch-capacity cases into tests before modifying behavior. Cover misses, tangency, deep overlap, local transforms, degenerate triangles, and both pair directions.
2. **Correct and simplify manifold caching.** Fix canonical shape-pair keys, reverse-pair orientation, lifetime invalidation, current-pose refresh, and stale-entry retirement. Avoid default `ALWAYS` bookkeeping that cannot reuse anything. Use checked growth/hash lookup rather than fixed linear scans, and consolidate the separate collision-cache key handling. Gates: compound contacts remain distinct; rotation-only/shape edits cannot reuse stale geometry; removal and mode changes cannot revive stale contacts; substeps use an appropriate collision epoch.
3. **Remove the mesh candidate cliff and improve query bounds.** Introduce stateless visitor queries with radius/bounds passed as arguments, using one contiguous mesh BVH for ray/sphere/AABB queries. Retain original triangle identity and complete results. Use tighter box/capsule bounds. Gate: candidate counts crossing 1,024 do not trigger a full scan when acceleration exists; compare accelerated contact results with an independent full scan, including local transforms and invalid-index/fallback cases. Raycast normals and triangle/material identity must remain correct.
4. **Prepare and reuse shape geometry.** Compute normalized poses, inverse/relative transforms, child bounds, OBB bases, capsule endpoints, and material combination once where valid. Cull separated child pairs before expensive work. Cache static prepared data and refresh after mutations. Check revisions before each pair if earlier contacts can move a body; a once-per-frame world-pose cache would change current behavior. Gates: moved statics wake sleepers, static edits appear immediately, solver corrections and local rotations do not reuse stale poses, contact tolerance is preserved, and plane normal conventions remain unchanged.
5. **Replace expensive scalar algorithms.** Implement bounded analytic segment/box distance, then matrix-based box SAT and reduced-work triangle/box SAT. Use a retained reference implementation plus independent analytic fixtures/randomized checks. Gate: contact existence, normal direction, depth/tolerance, degeneracy handling, and swapped-pair behavior agree within defined numerical tolerances. Capsule embedded cases need their own depenetration specification. Measure hits and misses separately.
6. **Reduce static-only scene overhead.** Start with an all-immovable-root fast exit and avoid static force/sleep calculations while preserving accumulator-clearing semantics. Add state-change notifications before active/dirty lists, dynamic-only publication snapshots, or separate static/moving trees. Gates: static/dynamic/kinematic transitions, mass changes, enable/filter changes, add/remove, callback removal, moved statics, and actor-order regressions pass. Benchmark all-static, one vehicle plus a large track, and sleeping-heavy scenes.
7. **Prototype SIMD where data is regular.** Start with four independent triangle bounds/distance tests within one mesh pair, or homogeneous primitive batches with structure-of-arrays scratch storage. Keep scalar tails/reference paths and deterministic contact reduction. Cross-pair batching must preserve immediate solver ordering or explicitly be a separately validated pipeline change. Enable only after measured end-to-end improvement.

Steps 2 and 3 address the most urgent structural problems. Prepared transforms and capsule/box distance are strong next targets. Static overhead should be measured separately from geometry kernels so a speedup is attributed to the correct subsystem.

## Oriented bounding boxes

Implementation follow-up: the scene now uses cached local body OBBs as a selective six-face-axis rejection filter after AABB traversal. Geometry/pose invalidation, live solver poses, conservative fallbacks and measured results are documented in the [broadphase plan](WPPHYSICS_BROADPHASE_PLAN.md#implemented-continuation-tighter-oriented-body-bounds). The mesh triangle-query and exact narrowphase changes are now implemented as described in the follow-up below.

OBBs provide a tighter bound for long objects rotated away from the coordinate axes. As a geometric illustration, a 20-by-2 rectangle rotated 45 degrees has an approximately 15.56-by-15.56 enclosing AABB: 242 square units of footprint versus 40 for the OBB. This is a fit comparison, not a measured collision speedup. Spheres should retain sphere queries; a box is not a tighter representation of a spherical object.

The current narrowphase already uses oriented boxes for exact box/box SAT and transforms mesh triangles into box space for exact triangle/box tests. The missing opportunity is earlier candidate rejection: box/mesh still selects triangles using an enclosing sphere, and scene broadphase uses world AABBs. Repeating the existing box/box OBB SAT as an extra pretest would duplicate the exact collision test.

Recommended application:

1. Retain the shared AABB scene tree for cheap coarse rejection and motion updates.
2. Prepare each box's center, orthonormal basis, and half extents once where its pose revision permits reuse. Apply conservative contact tolerance.
3. Transform the query OBB into mesh local space. Use its enclosing local AABB for cheap node rejection, then evaluate selective OBB-versus-node-AABB rejection where rotations/aspect ratios make the coarse bound loose. Measure the added SAT cost against saved triangle work.
4. Continue using exact triangle/box contact generation on surviving candidates. For compounds, apply bounds to individual children rather than assuming the actor's union bound is one exact box.
5. Benchmark short/aligned boxes, long boxes at multiple angles, sloped roads, diagonal barriers, and dense contact regions. Compare sphere, local AABB, and local AABB plus OBB rejection using both candidate counts and total pair/scene time.

The earlier 968-versus-96 candidate comparison used an aligned box AABB, not a newly implemented OBB query. Rotated OBB-query performance remains unmeasured. Static mesh-node OBBs could be fitted during cooking, but a full OBB hierarchy adds storage, traversal-test, fitting, and refit costs; it should follow profiling of the selective query approach. Changing every dynamic-tree node to an OBB is not the recommended first implementation.

The exact box/box test can require 15 separating axes, as reflected in the current implementation and [Bullet's box collision source](https://github.com/bulletphysics/bullet3/blob/master/src/BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp). Cached bases and relative-matrix tests reduce preparation work, but the tighter fit must still justify the extra comparisons relative to cheap AABB tests.

## SIMD and build choices

An x64 build makes SSE2 available on the reviewed Windows target; it does not automatically vectorize this branch-heavy narrowphase into four independent collisions. SIMD is most promising after reducing candidate count and repeated preparation. Vectorizing a 32,768-triangle fallback is less valuable than avoiding that fallback.

Use a scalar reference and optional SSE2 kernel first. Preserve the 12-byte public `wp_vec3f` layout; do not load 16 bytes from it. Batch in private aligned/structure-of-arrays storage. Avoid a global AVX2 requirement; add runtime-dispatched wider kernels only if representative workloads justify them. Approximate reciprocal square roots or global fast-math settings need explicit tolerance/degeneracy validation, rather than being enabled as a blanket optimization.

The native build currently uses `/Ob1`. A separate measured `/Ob2` or link-time-optimization experiment may reduce small cross-file getter/math call overhead, but it does not address the algorithmic issues above. Treat compiler tuning as a benchmark variant with complete numerical checks, not a substitute for candidate reduction.

Algorithm references for implementation evaluation: [Geometric Tools segment/canonical-box distance](https://github.com/davideberly/GeometricTools/blob/master/GTE/Mathematics/DistSegment3CanonicalBox3.h) and [Bullet's relative-matrix box SAT implementation](https://github.com/bulletphysics/bullet3/blob/master/src/BulletCollision/CollisionDispatch/btBoxBoxDetector.cpp). These support the proposed algorithms, not a predicted Workphone speedup.

## Validation performed for this review

Fresh native dependency/target build checks completed for `WorkphonePhysicsCollisionTests`, `WorkphonePhysicsMaterialTests`, `WorkphonePhysicsBroadphaseTests`, and `PhysicsSceneCapacityTests`. All four focused CTests passed: collision, material, broadphase, and scene capacity. The temporary review harness additionally reproduced the mesh overflow cliff, optional compound-cache error, public manifold truncation, and empty-batch stale count.

The broader C++ UnitTests suite, full engine suite, sanitizer run, and interactive Editor validation were not executed for the original narrowphase review. Existing tests do not cover all reviewed cache/capacity failures, so their passing result does not certify those paths. Narrowphase kernel/cache changes remain proposals; the subsequent scene OBB-filter implementation has its separate validation and synthetic timing evidence in the broadphase plan linked above.

## Implemented follow-up, 10 October 2026

The seven implementation stages are integrated into the native scene path and the standalone narrowphase API. The original review and baseline above describe the previous implementation; source line links above refer to that reviewed version.

| Stage | Implemented behavior |
|---|---|
| Tests and counters | Dedicated `WorkphonePhysicsNarrowphaseTests` and `WorkphonePhysicsNarrowphaseBenchmarks` CMake targets. Shape-pair calls and optional nanosecond timing, preparation reuse, BVH traversal/candidates/exact triangles, full-scan fallbacks, generated/retained contacts, scene cache activity, compound rejection, actor types and dirty-static counters. |
| Contact cache | Scene and public utility share one checked growable hash implementation. Keys include both bodies and both shapes and accept either direction. Scene payloads check lifetime IDs and body/shape revisions, reverse anchors/normals when needed, refresh materials, and retire unseen contacts every collision substep. `ALWAYS` bypasses lookup, insertion and allocation. |
| Mesh queries | One flat BVH serves raycasts and stateless sphere/AABB/optional oriented visitor queries. The duplicate pointer collision tree and 1,024-candidate scratch arrays are removed. Original triangle IDs are retained. Missing acceleration scans valid triangles completely; malformed indices are skipped. Unchanged index topology refits in place; changed indices/validity rebuild. |
| Prepared geometry | Shapes cache normalized world poses, inverse rotation, axes, dimensions, capsule endpoints and world child bounds, checking revisions at consumption. Compound child bounds reject separated children before cache lookup/exact testing, with contact tolerance and roundoff allowance. Sphere/capsule mesh kernels run in mesh space; boxes transform directly from mesh to box space. Only accepted contacts are transformed to world coordinates. Materials combine once per contacting pair. |
| Scalar algorithms | Capsule/box uses a bounded piecewise quadratic segment/AABB minimum. Box/box uses relative-matrix SAT; triangle/box uses direct face bounds and unnormalized separating axes with scaled tolerance. Degenerate triangles use segment/point closest-distance fallbacks. Mesh-contact merging uses the mesh anchor consistently in both pair directions. |
| Static overhead | All-immovable roots return before rank/traversal setup. Static integration calls and unnecessary force/sleep calculations are skipped while accumulators still clear. Transform publication copies only non-static actors under the actor-array lock, releases that lock before callbacks, and checks scene membership/native state before publication. |
| SIMD prototype | Optional SSE2 batches four cached triangle bounds at mesh leaves, with safe component gathers, scalar tails, preserved visit order, and a compile-time scalar fallback. SIMD remains disabled by default because measured contact-pair savings were negligible or negative. |

See [prepared geometry](../Engine/c/Source/WorkphonePhysics/workphone_physics_geometry.c), [shared cache](../Engine/c/Source/WorkphonePhysics/workphone_physics_collision_cache.c), [mesh traversal](../Engine/c/Source/WorkphonePhysics/workphone_physics_triangle_mesh.c), and [contact kernels](../Engine/c/Source/WorkphonePhysics/workphone_physics_narrowphase.c).

### Reuse and depenetration rules

Optional `FIXED` and `DISTANCE` reuse is deliberately conservative: world-space contacts are reused only while both poses and geometries are unchanged. Any revision/lifetime change regenerates exact contacts regardless of the requested interval. Uncooked borrowed mesh arrays never reuse scene manifolds. Material values refresh even on cache hits. This fixes stale contacts without pretending to provide a full persistent-contact-manifold system; local-anchor feature tracking across moving poses remains future work.

An embedded capsule now chooses the smallest of six box-face translations that eject the **whole capsule**, including both segment endpoints and radius. The old closest-point rule could select an interior segment point and underestimate the correction. A centered radius-0.25 capsule with half-height 2 inside a unit-half-extent box gets a 1.25 lateral correction, rather than an arbitrary axial correction.

Selective mesh OBB rejection is enabled by default for boxes whose mesh-local AABB volume exceeds the oriented box volume by 1.5x. It adds the three box face-axis tests after the mesh AABB test; it conservatively retains edge-axis false positives. Spheres keep sphere traversal and capsules use endpoint-expanded AABBs. The shared body AABB tree and its existing selective body OBB filter remain in place.

### Measurements

Windows x64 MSVC RelWithDebInfo, same CPU and fixed-pose fixture definitions as the review. A fresh baseline was measured before these changes; optimized values below are medians of five samples from the committed benchmark. Setup is excluded from pair timings. Absolute timings vary with host load; these are fixture measurements, not Editor frame-rate claims.

| Fixture | Fresh baseline, us/pair | Optimized, us/pair | Approximate gain |
|---|---:|---:|---:|
| Sphere/sphere hit | 0.2658 | 0.1206 | 2.2x |
| Sphere/box hit | 0.4633 | 0.2924 | 1.6x |
| Rotated box/box hit | 1.2489 | 0.3176 | 3.9x |
| Rotated capsule/box hit | 3.3176 | 0.6726 | 4.9x |
| Capsule/box miss | 3.0813 | 0.4227 | 7.3x |
| Small box / 32,768-triangle mesh | 13.7533 | 8.6052 | 1.6x |
| Long box / mesh, previously 968 sphere candidates | 802.2420 | 91.7530 | 8.7x |
| Slightly longer box, previously 1,032 sphere candidates | 22,257.9590 | 99.9090 | 223x |

The two threshold fixtures now test 96 and 104 triangles with no overflow/full-scan fallback. A separate wide-box regression streams over 1,024 actual candidates without falling back. Cooking 32,768 triangles fell from 76.382 ms to 37.221 ms after removing the duplicate tree.

For the rotated long box, local AABB traversal tested 648 triangles in 257.944 us; the selective OBB filter tested 104 in 84.419 us, approximately 3.1x faster. Scalar and SSE2 AABB traversal measured 257.944/257.136 us for this pair, and 7,787/7,924 us for the dense 7,688-candidate case. Those SIMD differences do not justify enabling it by default.

Static measurements use 4,096 isolated box actors, 200 steps/sample, after warm-up. The baseline was rebuilt from the prior source with the same optimized compiler options. No collision contacts occur in these fixtures.

A final repeat after rebuilding gave 0.305 us for box/box, 0.647 us for capsule/box, 95.955 us for the former overflow fixture, and 238.770/77.579 us for the rotated long box with AABB/OBB traversal. SSE2 varied from a slight regression to a 3.4% dense-pair gain across runs; it remains opt-in.

| Scene fixture | Baseline us/step | Optimized us/step |
|---|---:|---:|
| All static | 260.412 | 178.753 |
| One dynamic plus 4,095 static | 391.303 | 320.517 |
| 256 sleeping dynamic plus 3,840 static | 470.942 | 359.406 |

These scenes still poll actor revisions/eligibility. Active/dirty membership lists and separate static/moving trees remain conditional follow-ups requiring a mutation-notification contract and representative track profiling. Immutable shared cooked mesh data, compound child hierarchies, a dispatch table, wider SIMD and cross-pair batching likewise remain evaluation items, as proposed in the review; they are not required for the implemented structural fixes.

### Validation and reproduction

- Fresh x64 native build plus the `WPPhysics` C++ DLL and `UnitTests` executable build/link succeeded.
- Seven focused CTests passed: narrowphase, collision, broadphase, oriented bounds, material, scene capacity and rotated vehicle inertia.
- The new narrowphase suite covers 6,000 hash entries and payload compaction; 1,035 batch manifolds and scene cache entries; empty/invalid batches; compound identity/culling; reverse cache orientation with live restitution; rotation/local-shape/dimension edits, removal/recreation, disable, mass/type transitions, stale retirement and mode changes.
- 12,000 seeded cases compare matrix SAT, triangle SAT and segment/box distance to retained pre-change scalar kernels. Independent dense segment samples and analytic endpoint, parallel, zero-length, tolerance and embedded fixtures supplement those comparisons.
- Mesh tests compare original-triangle bounds to independent scalar queries, nested/reentrant traversal, scalar/SSE2 contact ordering, accelerated/full-scan contact existence and maximum depth for sphere/capsule/box pairs with local transforms, both pair directions, OBB filtering, more than 1,024 candidates, vertex/index refits, invalid/degenerate triangles and ray identity/normal.
- MSVC AddressSanitizer native runs passed with SSE2 available and with `WP_PHYSICS_DISABLE_SIMD`. This is memory-access validation, not a leak-certification run.
- Focused wrapper tests `physics_dynamic_body_steps_and_publishes_transform` and `physics_transform_publication_preserves_callback_removal` passed all 22 assertions, including skipped static publication and listener-driven removal.

Build/run from the repository root:

```powershell
cmake --build project_x64 --target WorkphonePhysicsNarrowphaseTests WorkphonePhysicsNarrowphaseBenchmarks --config RelWithDebInfo --parallel 2
ctest --test-dir project_x64 -C RelWithDebInfo --output-on-failure -R 'WorkphonePhysics\.(narrowphase|collision|broadphase|oriented_bounds|material)|WPPhysics\.(scene_capacity|rotated_vehicle_inertia)'
.\bin\windows\v145\x64\MD\RelWithDebInfo\WorkphonePhysicsNarrowphaseBenchmarks.exe
```

Scene counters are available through `wp_physics_scene_get_broadphase_stats` and `wp_physics_scene_get_narrowphase_stats`; expensive per-pair timing is opt-in through `wp_physics_scene_set_narrowphase_timing_enabled`. Standalone narrowphase exposes corresponding controls for timing, forced full scans, SSE2 and selective mesh OBB filtering. Scene SSE2/mesh-OBB controls support profiling without changing solver order.

Public statistics layouts grew: rebuild native clients and plugins together. Existing ray/sphere query wrappers and the void batch API remain available; checked batch allocation failure clears the result and returns failure. The complete engine test suite, leak certification, non-x64 execution and manual Editor/vehicle-track validation were not run.
