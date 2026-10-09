# WorkphonePhysics broadphase review and implementation plan

Date: 9 October 2026. Target: Windows x64/MSVC. The shared AABB-tree stage below is implemented; the later optimization stages remain proposals.

## Review findings

The previous simulation path in `workphone_physics_scene.c` computed actor bounds once per collision substep and then visited every actor pair. Adaptive stepping can repeat this up to 64 times per simulation call. At 4,096 actors, enumeration alone visits 8,386,560 pairs per substep, before shape filtering and narrowphase work.

`workphone_physics_broadphase.c` contained a second all-pairs implementation. Its SAP/DBVT/MBP selection only stored a value, and its fixed arrays limited it to 512 proxies and 2,048 cached pairs. Dense overlaps could silently truncate results. Optimizing that file alone would not have improved scene simulation.

The scene also contained a grid contact path, grid allocation routines, and octree/BVH node definitions. Scene setup and the partition setter never created partition data, so selecting these modes continued to execute the all-pairs path. If activated, the grid path would recompute bounds inside candidate loops and process a body pair repeatedly when it shared several cells.

The broadphase and solver headers separately declared incompatible `wp_broadphase_type` enums. They now use one declaration.

## Implemented stage: one persistent AABB tree

- The native scene owns the same broadphase implementation used by the public C broadphase API. Separate scene/grid pair loops, grid allocation routines, unused spatial node definitions, and the scene's duplicate AABB overlap helper were removed. One contact consumer preserves existing shape filtering, narrowphase dispatch, and resolution.
- Nodes use index links in growable pooled storage. Stable proxy handles avoid searching for a body during each scene update. Surface-area insertion and height rotations maintain the hierarchy; freed nodes are reused.
- Tree bounds have a 0.1-unit margin. Small movements update exact bounds without reinsertion. Large movements and substantial shape shrinkage reinsert the leaf. Exact leaf overlap tests prevent the margin from changing the candidate set.
- Every substep refreshes actor bounds, including static and sleeping actors. This preserves detection of externally moved statics, edits to shapes, and wake-up contacts. Skipping these updates requires explicit revision tracking first.
- Aggregate enabled/movable flags prune disabled branches and immovable-versus-immovable branches. Sleeping dynamics remain movable for collision purposes. Existing body/shape category rules and constraint exclusions remain in the contact consumer.
- Pair traversal emits each pair once, in actor-index order. Pair order remains stable through tree rotations and node reuse. The scene streams pairs without allocating quadratic pair storage. The public pair cache grows dynamically and offers a checked allocation-failure result that exposes no partial cache.
- Tree storage is allocated when an actor is registered. Failed registration returns failure rather than silently omitting a registered actor during simulation. Clear/removal also invalidate cached contacts.
- BVH is the scene default. Legacy NONE/GRID/OCTREE selections remain accepted as documented aliases for the shared tree, rather than retaining duplicate algorithms. Public SAP/MBP/ABP selections likewise use the shared tree; the requested selection is retained as metadata.

The canonical enum values are SAP=0, DBVT=1, MBP=2, ABP=3. This retains the former broadphase-header values; clients compiled against the former solver-only MBP=1/ABP=2 values must rebuild and migrate any externally persisted numeric values.

## Sphere and SIMD decisions

| Option | Assessment for this implementation |
|---|---|
| Sphere tests instead of AABBs | Potentially useful for compact, nearly spherical objects. Long vehicles, barriers, terrain, and compound bodies generally have loose bounding spheres. Squared-distance testing avoids a square root per pair, but still has arithmetic and data-loading costs. |
| Sphere tree | Can reduce pair enumeration, but needs the same insertion, refitting, balancing, and motion handling as an AABB tree. It is not the preferred first hierarchy for this mixture of shapes. Benchmark a representative sphere-heavy workload before maintaining a second implementation. |
| Sphere then AABB | A sphere enclosing an existing AABB cannot reject any pair whose AABBs overlap, so it adds no candidate-quality improvement. It might reject distant pairs before the AABB test, but the AABB test already exits cheaply on a separating axis. An independently computed tight shape sphere can reject some AABB false positives; reserve that experiment for expensive narrowphase candidates. |
| SIMD/SSE | Useful when several candidate bounds can be tested together. x64/MSVC defaults to SSE2, but scalar floating-point instructions do not automatically mean four pairs are processed in parallel. Benchmark batched comparisons against the scalar tree before introducing intrinsics. |
| AVX2 | Optional later path, compiled separately and selected only when CPU/OS support is available. Do not require AVX2 globally just because the default build is x64. |

Sphere bounds must enclose every enabled child shape, including local offsets, mesh geometry, and any motion envelope used by the caller. Infinite planes need separate handling. Using only a body's origin and one child radius can miss collisions.

## Follow-up implementation order

1. **Profile representative scenes.** Record bounds construction, tree updates, traversal, candidate sorting, narrowphase, and solver time separately. Include small scenes, dense stacks, a large static track with moving vehicles, sleeping-heavy scenes, mixed sizes, rotation-only motion, and teleports. The tree still has quadratic output when most objects overlap; no hierarchy removes that output cost.
2. **Add reliable dirty revisions.** Track body transforms, shape membership, local transforms, dimensions, mesh revisions, enable/filter state, and solver position corrections. Cache static/unchanged bounds only after every mutation path participates. Cache raw-mesh local bounds instead of transforming every vertex per substep. Retain the moved-static/sleeping and edited-shape regressions.
3. **Improve layout and measure SSE2 batches.** Separate compact traversal bounds/links from leaf metadata. Consider four-child nodes or batched leaf candidates with structure-of-arrays min/max coordinates. Compare four candidates per SSE2 batch with scalar early exits, including packing and sorting costs. Keep the public 12-byte `wp_vec3f` layout; a 16-byte SIMD load must not read beyond it. Preserve inclusive touching and conservative bounds.
4. **Try selective shape-sphere rejection.** Use cached conservative local spheres transformed correctly into world space; compare rejection rate and total narrowphase savings with the extra update/test cost. Enable only for workloads showing an end-to-end improvement. Do not add a global sphere prepass by default.
5. **Reuse the tree for scene queries.** The public broadphase AABB query already traverses the shared tree. Scene raycasts still use their existing shape/query path. Integrating them requires bounds synchronization on query, closest-hit traversal, current filter rules, and triangle/material identity tests.

These stages complement the broader [WPPhysics production plan](WPPHYSICS_PRODUCTION_PLAN.md). Contact-cache shape identity, full solver settings integration, CCD, and trigger-event delivery remain separate work.

## Validation and measurement

The x64 RelWithDebInfo build passed for native physics and the new test executable. The C++ `WPPhysics` target also compiled and linked against the updated native library; that target was built with project-reference rebuilding disabled after building the focused native dependencies.

All four focused CTest tests passed: `WorkphonePhysics.broadphase`, `WorkphonePhysics.collision`, `WorkphonePhysics.material`, and `WPPhysics.scene_capacity`. The broadphase test compares exact pairs and queries with an independent all-pairs oracle across 700 objects and 50 rounds of movement, removal/reinsertion, enable changes, and movable changes. Additional fixtures cover dense cache growth, inclusive touching, finite plane bounds, small motion within fat bounds, custom pair order, clear/reuse, every legacy scene selection, body/shape disabling, local-shape edits, masks, triggers, and moved-static/sleeping contact behavior.

An additional x64 AddressSanitizer build instrumenting the modified broadphase and scene sources passed the focused broadphase tests without reported errors. This is not a full engine leak check. The full engine test suite and interactive Editor behavior were not validated in this change.

One synthetic run over 4,096 separated boxes and forty passes measured 72 ms for stationary tree traversal, 192 ms for all-moving tree update plus traversal, and 924 ms for all-pairs enumeration. All-moving updates translate every box by 0.03 units per pass. These timings exclude initial construction, shape/world-bound calculation, narrowphase, and solving; they are candidate-generation evidence, not a game-frame speedup guarantee. Host load and timer resolution affect the results.

Reproduce correctness with:

```powershell
cmake --build project_x64 --target WorkphonePhysicsBroadphaseTests WorkphonePhysicsCollisionTests WorkphonePhysicsMaterialTests PhysicsSceneCapacityTests --config RelWithDebInfo --parallel 2
ctest --test-dir project_x64 -C RelWithDebInfo -R 'WorkphonePhysics\.(broadphase|collision|material)|WPPhysics.scene_capacity' --output-on-failure
```

Run the optional synthetic timing comparison with:

```powershell
./bin/windows/v145/x64/MD/RelWithDebInfo/WorkphonePhysicsBroadphaseTests.exe --benchmark
```

Primary references: [Microsoft x64 instruction-set defaults and feature checks](https://learn.microsoft.com/en-us/cpp/build/reference/arch-x64?view=msvc-170), [Box2D dynamic-tree overview](https://box2d.org/documentation/group__tree.html), and [fat-AABB update behavior](https://box2d.org/doc_version_2_4/classb2_dynamic_tree.html). Box2D is a 2D example of the hierarchy/update strategy, not evidence of a measured Workphone 3D speedup.
