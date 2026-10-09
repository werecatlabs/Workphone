# WorkphonePhysics narrowphase optimization review

9 October 2026. Review of native C collision generation, scene contact consumption, mesh acceleration, and static-body integration. No production code was changed for this review.

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

Implementation follow-up: the scene now uses cached local body OBBs as a selective six-face-axis rejection filter after AABB traversal. Geometry/pose invalidation, live solver poses, conservative fallbacks and measured results are documented in the [broadphase plan](WPPHYSICS_BROADPHASE_PLAN.md#implemented-continuation-tighter-oriented-body-bounds). The mesh triangle-query and exact narrowphase optimizations reviewed below remain proposed work.

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
