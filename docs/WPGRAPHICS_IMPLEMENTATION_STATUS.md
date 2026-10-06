# WPGraphics implementation status

Catalog increment validated on 6 October 2026, Windows x64, Visual Studio 2026/MSVC 19.51, CMake 4.4.4, RelWithDebInfo, starting from `d5c859612` on `brodex`.

Current local evidence: the required SQLite catalog target and its Workphone/WPSQLite dependencies were built, and the catalog contracts passed without graphics or external media. The two isolated skinning/particle reference tests also passed. The catalog preset was configured with the existing verification build directory overridden to `cmake-build-debug-vs2026-readiness`.

Historical graphics evidence at `8a767dda5`: existing graphics test binaries reported **11 passed, 1 skipped**. The isolated tests duplicate two contracts in that full suite. GPU tests were not rebuilt/rerun for this catalog increment; performance, remote CI, complete engine/editor builds and clean-machine packaging remain unverified.

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
- Required SQLite catalog coverage for existing bound CRUD, durable UUIDs, shared-scene deletion, failed rename rollback, detached lookups, concurrent reads and database switching.
- Catalog schema version 1 in its own metadata table, preserving unrelated `user_version` values. Valid unversioned catalogs retain their original rows/IDs and a one-time `wp_asset_catalog_backup_v0` row copy inside the same database. This is a migration snapshot, not an independent database-file backup.
- Schema guards reject invalid/oversized/NUL-containing values and enforce UUID/file-path uniqueness while allowing shared scene paths. Conflicting legacy data, reserved-name collisions and unsupported schema versions fail closed; the migration transaction rolls back schema and data changes.
- Catalog migration regressions for Unicode/apostrophe paths, a misleading nonunique UUID index, retained backup/reopen behavior, duplicate identities/paths, invalid legacy keys, embedded NULs, early/late migration failure and newer-version preservation. A dedicated catalog preset and CI job require the backend and fail on missing tests.
- Windows build repair: the media-path definition is scoped to its native-runtime consumer so paths with spaces do not break Boost.Context's MASM compilation.

The GPU fixture calls deformation and particle drawing directly. It does not yet establish imported/controller-driven animation, automatic scene scheduling, multi-camera correctness or catalog-to-cook-to-render behavior. Particle scene preparation is implemented as the intended update owner; once-per-frame behavior remains to be validated across all callers.

## Run validation

From the repository root in PowerShell:

```powershell
cmake --preset asset-catalog
cmake --build --preset asset-catalog
ctest --preset asset-catalog

cmake --preset claw-contracts
cmake --build --preset claw-contracts
ctest --preset claw-contracts

cmake --preset claw-windows
cmake --build --preset claw-windows
Tools/ValidateClawGraphics.ps1 -BuildDirectory project_claw_windows
```

Use `-RequireExternalAssets` when validating a release environment with the media fixtures installed. The isolated contract build was built and tested; the fresh full-engine preset was configured. Full-engine compilation and GPU validation were performed in the existing `project_x64` build tree.

Latest catalog result: **1 CTest target passed**, exercising the CRUD and migration contracts above. Latest isolated native reference result: **2 passed**. Historical full graphics result: **11 passed, 1 skipped**; the skipped external mesh import test requires missing `Bin/Media/Ogre/models` assets. The new catalog CI job has not been verified by a remote run.

## Remaining release work

AssetDatabaseManager is included in the [production plan](WPGRAPHICS_PRODUCTION_PLAN.md), under M1A and DB-01 through DB-10. Its existing identity/mutation fixes now have required-backend runtime evidence, and versioned migration is implemented with retained row snapshots and rollback. Legacy conflicts require explicit repair; no duplicate identities are silently discarded. Path canonicalization and explicit asset kinds, cache generations/budgets, broader mutation/unload races, open/durability policy and read-only runtime manifests remain open. An adapter must connect UUID/source metadata to ResourceSystem's path-based ResourceIDs and compilation metadata without duplicating that system. GD remains unsatisfied until catalog-resolved cooked assets reach a DX11 draw and failure preserves the last good asset.

Animation remains experimental: asset import, skeleton/clip ownership, animation controllers and graphs, per-instance poses, automatic scene integration, GPU skinning, morph targets, IK, root motion, animated culling, and animation editor workflows are not completed.

Particles remain experimental: template/resource loading, emitter shapes, multiple emitters, curves, bursts, collisions, trails, flipbooks, globally sorted transparency, soft particles, lighting, robust editor authoring, scalability policies, and performance budgets remain. Timed pause is explicitly unsupported. Effects currently use a single point emitter.

Water rendering is unavailable. The water asset/component, mesh generation, shaders, reflection/refraction passes, depth/shoreline effects, editor integration, and validation scenes from the roadmap remain to be implemented.

The roadmap's GPU pass graph, HDR presentation, shadows and lighting completion, device recovery, runtime backend transitions, resource stress testing, performance certification, and release packaging also remain open. Next: catalog path/lifecycle hardening and the ResourceSystem bridge, catalog-to-compiled-material rendering, then imported animation and particle scene integration. See [the revised plan](WPGRAPHICS_PRODUCTION_PLAN.md) for dependencies and acceptance gates. Do not treat passing this stage's tests as a production certification.
