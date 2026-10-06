# WPGraphics implementation status

Validated on 6 October 2026, Windows x64, MSVC 19.51, RelWithDebInfo.

Reviewed again at source revision `8a767dda5`. Existing graphics test binaries were rerun: **11 passed, 1 skipped**; isolated native contracts: **2 passed**. The isolated tests duplicate two contracts in the full suite. This review did not rebuild the engine, run database tests, measure performance or verify remote CI.

The production roadmap is only partially implemented. This stage supplies executable contracts and initial animation deformation and particle rendering paths; it does not satisfy the R1 or R2 release gates.

## Implemented in this stage

- Dependency-free C90 contract build and CI workflow for skinning and particle simulation.
- CPU reference skinning with four influences, up to 256 joints, weight normalization, validated affine transforms, and transactional rejection of invalid input. Supported transforms are rigid or uniformly scaled; nonuniform scale and shear are rejected.
- Explicit ClawMesh skinning data and palette APIs that update native positions and normals, invalidate cached GPU geometry, and recompute bounds. The caller supplies the palette on the owning rendering thread.
- Seeded, bounded CPU particle simulation at 120 Hz, with gravity, lifetime, size ranges, color interpolation, duration, pause, stop, drain, and bounded prewarm. No allocation occurs per simulation step.
- Claw particle lifecycle ownership, synchronized snapshots, diagnostics for unsupported templates, and dropped-particle counts.
- DX11 camera-facing particle batches with alpha/additive blending, depth testing without depth writes, and sorting within each effect. Scene preparation advances simulation independently of individual view draws.
- Backend capability inventory that retains experimental/unavailable status and explicitly reports `productionCertified = false`.
- Generated mesh serialization coverage independent of optional external media, plus a strict graphics validation runner.
- GPU readback regression proving particle pixels and translated CPU-skinned mesh pixels, alongside particle lifecycle checks.

The GPU fixture calls deformation and particle drawing directly. It does not yet establish imported/controller-driven animation, automatic scene scheduling, multi-camera correctness or catalog-to-cook-to-render behavior. Particle scene preparation is implemented as the intended update owner; once-per-frame behavior remains to be validated across all callers.

## Run validation

From the repository root in PowerShell:

```powershell
cmake --preset claw-contracts
cmake --build --preset claw-contracts
ctest --preset claw-contracts

cmake --preset claw-windows
cmake --build --preset claw-windows
Tools/ValidateClawGraphics.ps1 -BuildDirectory project_claw_windows
```

Use `-RequireExternalAssets` when validating a release environment with the media fixtures installed. The isolated contract build was built and tested; the fresh full-engine preset was configured. Full-engine compilation and GPU validation were performed in the existing `project_x64` build tree.

Latest full graphics result: **11 passed, 1 skipped**. The skipped external mesh import test requires missing `Bin/Media/Ogre/models` assets. The two isolated native contract tests also passed. CI configuration has been added but has not been verified by a remote run.

## Remaining release work

AssetDatabaseManager is now explicitly included in the [production plan](WPGRAPHICS_PRODUCTION_PLAN.md), under M1A and DB-01 through DB-10. Its current resource catalog and ResourceDirector caches should be retained, while durable UUIDs, scoped scene deletion, bound SQL values, transactional CRUD, schema migration, path canonicalization, cache synchronization/generations and narrow/wide database switching are hardened. An adapter must connect UUID/source metadata to ResourceSystem's path-based ResourceIDs and compilation metadata without duplicating that system. Database findings are source-review evidence; fixes and runtime verification remain open.

Animation remains experimental: asset import, skeleton/clip ownership, animation controllers and graphs, per-instance poses, automatic scene integration, GPU skinning, morph targets, IK, root motion, animated culling, and animation editor workflows are not completed.

Particles remain experimental: template/resource loading, emitter shapes, multiple emitters, curves, bursts, collisions, trails, flipbooks, globally sorted transparency, soft particles, lighting, robust editor authoring, scalability policies, and performance budgets remain. Timed pause is explicitly unsupported. Effects currently use a single point emitter.

Water rendering is unavailable. The water asset/component, mesh generation, shaders, reflection/refraction passes, depth/shoreline effects, editor integration, and validation scenes from the roadmap remain to be implemented.

The roadmap's GPU pass graph, HDR presentation, shadows and lighting completion, device recovery, runtime backend transitions, resource stress testing, performance certification, and release packaging also remain open. Next: required-backend catalog identity/mutation tests and repairs, catalog-to-compiled-material rendering, then imported animation and particle scene integration. See [the revised plan](WPGRAPHICS_PRODUCTION_PLAN.md) for dependencies and acceptance gates. Do not treat passing this stage's tests as a production certification.
