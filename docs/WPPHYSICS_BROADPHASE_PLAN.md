# WorkphonePhysics broadphase review and implementation plan

Date: 9 October 2026. Target: Windows x64/MSVC. The shared AABB tree, revision-based bounds caching, traversal pruning, and optional SSE2 path are implemented. Remaining work is listed below.

## Review findings

The previous simulation path in `workphone_physics_scene.c` computed actor bounds once per collision substep and then visited every actor pair. Adaptive stepping can repeat this up to 64 times per simulation call. At 4,096 actors, enumeration alone visits 8,386,560 pairs per substep, before shape filtering and narrowphase work.

`workphone_physics_broadphase.c` contained a second all-pairs implementation. Its SAP/DBVT/MBP selection only stored a value, and its fixed arrays limited it to 512 proxies and 2,048 cached pairs. Dense overlaps could silently truncate results. Optimizing that file alone would not have improved scene simulation.

The scene also contained a grid contact path, grid allocation routines, and octree/BVH node definitions. Scene setup and the partition setter never created partition data, so selecting these modes continued to execute the all-pairs path. If activated, the grid path would recompute bounds inside candidate loops and process a body pair repeatedly when it shared several cells.

The broadphase and solver headers separately declared incompatible `wp_broadphase_type` enums. They now use one declaration.

## Implemented stage: one persistent AABB tree

- The native scene owns the same broadphase implementation used by the public C broadphase API. Separate scene/grid pair loops, grid allocation routines, unused spatial node definitions, and the scene's duplicate AABB overlap helper were removed. One contact consumer preserves existing shape filtering, narrowphase dispatch, and resolution.
- Nodes use index links in growable pooled storage. Stable proxy handles avoid searching for a body during each scene update. Surface-area insertion and height rotations maintain the hierarchy; freed nodes are reused.
- Tree bounds have a 0.1-unit margin. Small movements update exact bounds without reinsertion. Large movements and substantial shape shrinkage reinsert the leaf. Exact leaf overlap tests prevent the margin from changing the candidate set.
- Every substep synchronizes actor state, including static and sleeping actors. Bounds are rebuilt only when geometry or transforms change, with a conservative exception for uncooked borrowed mesh arrays described below.
- Aggregate enabled/movable flags prune disabled branches and immovable-versus-immovable branches. Sleeping dynamics remain movable for collision purposes. Existing body/shape category rules and constraint exclusions remain in the contact consumer.
- Pair traversal emits each pair once, in actor-index order. Pair order remains stable through tree rotations and node reuse. The scene streams pairs without allocating quadratic pair storage. The public pair cache grows dynamically and offers a checked allocation-failure result that exposes no partial cache.
- Tree storage is allocated when an actor is registered. Failed registration returns failure rather than silently omitting a registered actor during simulation. Clear/removal also invalidate cached contacts.
- BVH is the scene default. Legacy NONE/GRID/OCTREE selections remain accepted as documented aliases for the shared tree, rather than retaining duplicate algorithms. Public SAP/MBP/ABP selections likewise use the shared tree; the requested selection is retained as metadata.

The canonical enum values are SAP=0, DBVT=1, MBP=2, ABP=3. This retains the former broadphase-header values; clients compiled against the former solver-only MBP=1/ABP=2 values must rebuild and migrate any externally persisted numeric values.

## Implemented continuation: cached bounds and optional SSE2

- Each body has a bounds revision. Body transforms, shape attachment/removal, shape dimensions, local transforms, shape enable changes, mesh replacement, and cooked-mesh refits invalidate it. Static and sleeping bodies participate. Reassigning an identical body position/orientation does not invalidate it.
- Each scene caches the revision and validity of each actor's proxy. Revisions are captured before contact resolution so solver position corrections force a rebuild on the next substep. Body enable state, mass/type, filters, and triggers remain live; changes that do not affect geometry do not require a bounds rebuild.
- Cooked meshes already cache local bounds and require `wp_triangle_mesh_refit_aabb` after borrowed vertex/index edits. Refit now also invalidates the owning body's bounds. Uncooked mesh arrays retain per-substep recomputation because external writes cannot be observed. Their caching needs an explicit mutation contract before it can be safe.
- Ordered leaves are cached until membership, enable state, or requested order changes. Internal nodes carry the maximum descendant rank, pruning branches whose pairs have already been emitted. Final pairs retain canonical actor order, and hit sorting is skipped for zero/one hit.
- Optional SSE2 traversal compares four independent node boxes together. It gathers individual coordinates rather than reading 16 bytes from a 12-byte `wp_vec3f`. Tails and dense query regions use the shared scalar traversal. Both paths use the same exact leaf test, ordering, and contact consumer.
- Scalar traversal remains the default because SIMD measurements vary by workload. Enable the optional path with `wp_physics_scene_set_broadphase_simd_enabled(scene, 1)` or `wp_broadphase_set_simd_enabled(bp, 1)`. Each returns the actual enabled state; unsupported targets return zero. Defining `WP_PHYSICS_DISABLE_SIMD` when compiling the native library excludes the SSE2 implementation.
- `wp_physics_scene_get_broadphase_stats` reports bounds rebuilds/reuses, emitted candidate pairs, and collision substeps from the latest successful simulation call. These counters establish whether caching is effective without timing assertions in correctness tests.

## Sphere and SIMD decisions

| Option | Assessment for this implementation |
|---|---|
| Sphere tests instead of AABBs | Potentially useful for compact, nearly spherical objects. Long vehicles, barriers, terrain, and compound bodies generally have loose bounding spheres. Squared-distance testing avoids a square root per pair, but still has arithmetic and data-loading costs. |
| Sphere tree | Can reduce pair enumeration, but needs the same insertion, refitting, balancing, and motion handling as an AABB tree. It is not the preferred first hierarchy for this mixture of shapes. Benchmark a representative sphere-heavy workload before maintaining a second implementation. |
| Sphere then AABB | A sphere enclosing an existing AABB cannot reject any pair whose AABBs overlap, so it adds no candidate-quality improvement. It might reject distant pairs before the AABB test, but the AABB test already exits cheaply on a separating axis. An independently computed tight shape sphere can reject some AABB false positives; reserve that experiment for expensive narrowphase candidates. |
| SIMD/SSE | An optional four-box SSE2 implementation is now available and checked against the scalar/all-pairs oracle. Synthetic results show gains for some sparse and mixed-size workloads, but small regressions elsewhere. Keep scalar as the default until representative scenes justify enabling it. |
| AVX2 | Optional later path, compiled separately and selected only when CPU/OS support is available. Do not require AVX2 globally just because the default build is x64. |

Sphere bounds must enclose every enabled child shape, including local offsets, mesh geometry, and any motion envelope used by the caller. Infinite planes need separate handling. Using only a body's origin and one child radius can miss collisions.

## Remaining implementation order

1. **Profile representative scenes.** Record bounds construction, tree updates, traversal, candidate sorting, narrowphase, and solver time separately. Include small scenes, dense stacks, a large static track with moving vehicles, sleeping-heavy scenes, mixed sizes, rotation-only motion, and teleports. The tree still has quadratic output when most objects overlap; no hierarchy removes that output cost.
2. **Define raw-mesh mutation ownership if needed.** Dirty revisions and cooked-mesh invalidation are implemented. An explicit refit/edit API for uncooked borrowed geometry would permit caching its local bounds while retaining external-edit correctness. Keep the current conservative refresh until callers can honor that contract.
3. **Improve layout if profiling supports it.** The optional SSE2 prototype is implemented. Next compare compact traversal bounds/links separated from leaf metadata, four-child nodes, or structure-of-arrays bounds against both current paths. Include packing, update, sorting, and end-to-end costs; preserve inclusive touching and the public vector layout.
4. **Try selective shape-sphere rejection.** Use cached conservative local spheres transformed correctly into world space; compare rejection rate and total narrowphase savings with the extra update/test cost. Enable only for workloads showing an end-to-end improvement. Do not add a global sphere prepass by default.
5. **Reuse the tree for scene queries.** The public broadphase AABB query already traverses the shared tree. Scene raycasts still use their existing shape/query path. Integrating them requires bounds synchronization on query, closest-hit traversal, current filter rules, and triangle/material identity tests.

These stages complement the broader [WPPhysics production plan](WPPHYSICS_PRODUCTION_PLAN.md). Contact-cache shape identity, full solver settings integration, CCD, and trigger-event delivery remain separate work.

## Validation and measurement

The x64 RelWithDebInfo build passed for native physics and the new test executable. The C++ `WPPhysics` target also compiled and linked against the updated native library; that target was built with project-reference rebuilding disabled after building the focused native dependencies.

All four focused CTest tests passed: `WorkphonePhysics.broadphase`, `WorkphonePhysics.collision`, `WorkphonePhysics.material`, and `WPPhysics.scene_capacity`. The broadphase test runs with scalar and SSE2 selected, comparing exact pairs and queries with an independent all-pairs oracle across 700 objects and 50 rounds of movement, removal/reinsertion, enable changes, and movable changes. Additional fixtures cover dense cache growth, inclusive touching, finite plane bounds, small motion within fat bounds, custom pair order, clear/reuse, every legacy scene selection, body/shape disabling, local-shape edits, masks, triggers, and moved-static/sleeping contact behavior. Cache fixtures verify reuse and invalidation, rotation-only contacts, live mass changes, cooked mesh refits, uncooked vertex edits, shape destruction, and solver corrections across substeps.

Additional x64 AddressSanitizer builds instrumenting broadphase, scene, rigidbody, collision-shape, triangle-mesh, and test sources passed without reported errors, both with SSE2 available and with `WP_PHYSICS_DISABLE_SIMD`. This is not a full engine leak check. The full engine test suite and interactive Editor behavior were not validated in this change.

The optional benchmark target measures five fresh runs and reports the median. Scalar/SSE2 execution order alternates between repeats, and emitted pair counts must agree. The baseline below is the previous shared-tree library saved before this continuation, measured with the same benchmark source. Times are milliseconds per pass; setup is excluded.

| Candidate-generation workload | Bodies | Previous tree | Current scalar | Optional SSE2 |
|---|---:|---:|---:|---:|
| Small separated | 32 | 0.0023 | 0.0014 | 0.0014 |
| Sparse separated | 4,096 | 1.1123 | 1.0018 | 0.9393 |
| Dense all-overlap | 512 | 6.0203 | 4.8856 | 4.9923 |
| Mixed sizes | 2,048 | 1.7756 | 2.0570 | 1.4408 |
| Static-heavy | 4,096 | 0.5705 | 0.5243 | 0.5450 |
| All moving | 4,096 | 2.7741 | 2.7877 | 2.5629 |
| Teleports | 2,048 | 4.0244 | 4.3209 | 4.1996 |

All-moving updates translate every box by 0.03 units per pass. Teleports alternate large displacements. Candidate timings exclude shape/world-bound calculation, narrowphase, and solving. They show the tradeoffs: rank pruning and cached ordering help several cases, but mixed sizes and teleports regress in the scalar run; SSE2 is not a uniform improvement.

The separate full-scene benchmark includes simulation, bounds synchronization, integration, and contact traversal for 4,096 separated boxes, after warmup, over 80 steps per run. With scalar traversal, all-static simulation fell from 2.3579 to 0.2288 ms/step; a scene with 1% moving bodies fell from 2.3172 to 0.6158 ms/step. These scenes have no overlapping contacts and demonstrate caching benefits, not a game-frame speedup guarantee. Host load affects timings; profile actual vehicle/track and dense-contact scenes before selecting defaults.

Reproduce correctness with:

```powershell
cmake --build project_x64 --target WorkphonePhysicsBroadphaseTests WorkphonePhysicsCollisionTests WorkphonePhysicsMaterialTests PhysicsSceneCapacityTests --config RelWithDebInfo --parallel 2
ctest --test-dir project_x64 -C RelWithDebInfo -R 'WorkphonePhysics\.(broadphase|collision|material)|WPPhysics.scene_capacity' --output-on-failure
```

Run the optional synthetic timing comparison with:

```powershell
cmake --build project_x64 --target WorkphonePhysicsBroadphaseBenchmarks --config RelWithDebInfo --parallel 2
./bin/windows/v145/x64/MD/RelWithDebInfo/WorkphonePhysicsBroadphaseBenchmarks.exe
```

Primary references: [Microsoft x64 instruction-set defaults and feature checks](https://learn.microsoft.com/en-us/cpp/build/reference/arch-x64?view=msvc-170), [Box2D dynamic-tree overview](https://box2d.org/documentation/group__tree.html), and [fat-AABB update behavior](https://box2d.org/doc_version_2_4/classb2_dynamic_tree.html). Box2D is a 2D example of the hierarchy/update strategy, not evidence of a measured Workphone 3D speedup.
