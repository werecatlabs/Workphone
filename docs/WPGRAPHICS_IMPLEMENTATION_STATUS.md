# WPGraphics implementation status

## Verification increment — 8 October 2026

Rebuilt the selected graphics, native, SQLite catalog and ResourceSystem targets on `brodex` at `22e542cca`, with the local verification changes described below. **Debug: 13 passed, 1 unavailable. RelWithDebInfo: 13 passed, 1 unavailable.** All mandatory tests executed; the unavailable test is `WorkphoneGraphics.mesh_import_assets`, whose six external Ogre/OgreNext mesh files are absent. Neither release gate is certified.

The toolchain was Visual Studio Community 2026, MSVC **19.51.36260.0**, Windows SDK **10.0.26100.0**, and CMake/CTest **4.4.4**. The three GPU test executables recorded the actual DX11 device: **NVIDIA GeForce RTX 3090**, feature level **11.0**, driver **32.0.16.1692**. The DX11 target fixture reported the legacy-discard presentation path; this run does not certify flip presentation or representative frame performance.

Changes since the catalog increment at `597c6e5c5` retain schema-v1 migration and add long-text UI/serialization, editor-camera behavior, visibility, winding/capture changes, directional shadow regressions, core state/container refactors, and procedural circuit/open-city racing support. This increment validates the selected WPGraphics contracts; the complete racing/editor/unit suites were not run.

Completed verification work:

- Added `wpgraphics-baseline` configure/build/test presets and `Tools/BuildWPGraphicsBaseline.ps1`, which builds Debug and RelWithDebInfo with required SQLite/ResourceSystem support. The runner selects installed CMake explicitly and normalizes environment-variable names only in the build child process to avoid MSBuild's duplicate `PATH`/`Path` failure.
- Fixed the minimal graphics build's missing `imgui.lib`: WPGraphics now links the `imgui` CMake target so its dependency is built automatically.
- Updated two stale test fixtures to match the current implementation: explicitly load/unload the native Claw light and use the renderer's current front-face winding for the PBR triangle. Existing lighting/PBR assertions remain intact.
- Expanded `ValidateClawGraphics.ps1` to require all 14 registered tests, including catalog and ResourceSystem coverage. It writes configuration-specific JUnit and JSON evidence containing revision/local-diff identity, toolchain, adapter/driver, source/fixture and executable hashes, actual outcomes and coverage limits. GPU output includes selected-device evidence and diagnostic microbenchmark timings.
- Restored `.github/workflows/graphics-contracts.yml` and removed `.github` from ignore rules. Its native and full baseline jobs cover Debug and RelWithDebInfo. The hosted jobs explicitly use Visual Studio 2022, matching the [Windows 2022 runner inventory](https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md); local validation used Visual Studio 2026. Remote execution remains unverified.
- Verified that missing required registrations fail preflight and `-RequireExternalAssets` fails for the unavailable mesh fixtures. Default development validation reports that gap explicitly; it does not certify a release.

Local evidence is in the ignored `cmake-build-debug-vs2026-readiness` build directory: `baseline-configure.log`, `baseline-Debug-build.log`, `baseline-RelWithDebInfo-build.log`, and `claw-graphics-<configuration>-results.xml` / `claw-graphics-<configuration>-evidence.json`. Strict external-asset results are retained separately as `strict-external-assets-results.xml` / `strict-external-assets-evidence.json`. Generated mesh serialization, native deformation/particles, GPU PBR/shadows/deformation/particles, UI/text/camera, SQLite CRUD/migration and ResourceSystem compilation contracts pass. Imported animation/effects, catalog-to-GPU integration, packaged consumers, remote CI and representative performance/recovery/soak evidence remain open.

The next implementation increment is canonical catalog paths/entry kinds and lifecycle policy, then UUID/source-to-ResourceID mapping and a cooked material/texture reaching DX11. Batch A's local verification deliverable is complete; external-media release coverage and remote CI confirmation remain outstanding.

## Planning review before the rebuild — 8 October 2026

Source reviewed at `d2e81cdb4`. The catalog repairs/migration, explicit CPU skinning and basic DX11 particles remain present. Recent source also adds long-text UI/serialization, visibility, editor-camera-during-Play and sample capture/winding improvements.

A current `project_x64` Debug CTest attempt selected 14 graphics/catalog/resource targets: **14 unavailable, 0 executed**, because their expected executables were missing. This is a build-artifact/verification gap, not evidence of source assertion failures. No rebuild, performance measurement or GPU capture was performed for this planning request. `.github` and its previously documented workflows are absent from this checkout; restore/version CI before marking that work complete.

The revised [production plan](WPGRAPHICS_PRODUCTION_PLAN.md) starts with a rebuilt traceable baseline, then canonical catalog identity and a cooked material/texture reaching DX11, followed by imported animation and particle scene integration. The implementation and test results below are historical evidence, with their original configurations; neither release gate is certified.

## Historical implementation evidence

Catalog increment validated on 6 October 2026, Windows x64, Visual Studio 2026/MSVC 19.51, CMake 4.4.4, RelWithDebInfo, starting from `d5c859612` on `brodex`.

Current local evidence: the required SQLite catalog target and its Workphone/WPSQLite dependencies were built, and the catalog contracts passed without graphics or external media. The two isolated skinning/particle reference tests also passed. The catalog preset was configured with the existing verification build directory overridden to `cmake-build-debug-vs2026-readiness`.

Historical graphics evidence at `8a767dda5`: existing graphics test binaries reported **11 passed, 1 skipped**. The isolated tests duplicate two contracts in that full suite. GPU tests were not rebuilt/rerun for this catalog increment; performance, remote CI, complete engine/editor builds and clean-machine packaging remain unverified.

The production roadmap is only partially implemented. This stage supplies executable contracts and initial animation deformation and particle rendering paths; it does not satisfy the R1 or R2 release gates.

## Implemented in this stage

- Dependency-free C90 contract build for skinning and particle simulation; CI definitions are restored by the current verification increment, with remote execution still unverified.
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
- Catalog migration regressions for Unicode/apostrophe paths, a misleading nonunique UUID index, retained backup/reopen behavior, duplicate identities/paths, invalid legacy keys, embedded NULs, early/late migration failure and newer-version preservation. The dedicated catalog preset and restored full-baseline CI definition require the backend.
- Windows build repair: the media-path definition is scoped to its native-runtime consumer so paths with spaces do not break Boost.Context's MASM compilation.

The GPU fixture calls deformation and particle drawing directly. It does not yet establish imported/controller-driven animation, automatic scene scheduling, multi-camera correctness or catalog-to-cook-to-render behavior. Particle scene preparation is implemented as the intended update owner; once-per-frame behavior remains to be validated across all callers.

## Run validation

From the repository root with PowerShell 7, build and validate both configurations:

```powershell
pwsh -File Tools/BuildWPGraphicsBaseline.ps1
```

Use `-Configuration Debug` or `-Configuration RelWithDebInfo` for one configuration, `-BuildDirectory` for a different build tree, and `-CMakeExecutable` / `-Generator` for an explicit toolchain. The default configure preset uses Visual Studio 2026. Revalidate an existing baseline with `pwsh -File Tools/ValidateClawGraphics.ps1 -BuildDirectory project_wpgraphics_baseline -Configuration Debug`. Reports include catalog and ResourceSystem contracts as mandatory coverage.

The narrower catalog/native presets remain available:

```powershell
$baselineCMake = Join-Path $env:ProgramFiles 'CMake/bin/cmake.exe'
$baselineCTest = Join-Path $env:ProgramFiles 'CMake/bin/ctest.exe'
& $baselineCMake --preset asset-catalog
& $baselineCMake --build --preset asset-catalog
& $baselineCTest --preset asset-catalog

& $baselineCMake --preset claw-contracts
& $baselineCMake --build --preset claw-contracts
& $baselineCTest --preset claw-contracts
```

Use `-RequireExternalAssets` when validating a release environment with the media fixtures installed; missing media blocks that gate. The current baseline was configured/built through `BuildWPGraphicsBaseline.ps1` with `-BuildDirectory cmake-build-debug-vs2026-readiness`, then validated in both configurations. This is a selected-target local rebuild, not a clean-machine packaging check.

Historical catalog result: **1 CTest target passed**, exercising the CRUD and migration contracts above. Historical isolated native reference result: **2 passed**. Historical full graphics result: **11 passed, 1 skipped**. The current rebuilt baseline supersedes those results for the selected targets with **13 passed, 1 unavailable per configuration**. Remote CI remains unverified; its definitions are restored locally.

## Remaining release work

AssetDatabaseManager is included in the [production plan](WPGRAPHICS_PRODUCTION_PLAN.md), under M1A and DB-01 through DB-10. Its existing identity/mutation fixes now have required-backend runtime evidence, and versioned migration is implemented with retained row snapshots and rollback. Legacy conflicts require explicit repair; no duplicate identities are silently discarded. Path canonicalization and explicit asset kinds, cache generations/budgets, broader mutation/unload races, open/durability policy and read-only runtime manifests remain open. An adapter must connect UUID/source metadata to ResourceSystem's path-based ResourceIDs and compilation metadata without duplicating that system. GD remains unsatisfied until catalog-resolved cooked assets reach a DX11 draw and failure preserves the last good asset.

Animation remains experimental: asset import, skeleton/clip ownership, animation controllers and graphs, per-instance poses, automatic scene integration, GPU skinning, morph targets, IK, root motion, animated culling, and animation editor workflows are not completed.

Particles remain experimental: template/resource loading, emitter shapes, multiple emitters, curves, bursts, collisions, trails, flipbooks, globally sorted transparency, soft particles, lighting, robust editor authoring, scalability policies, and performance budgets remain. Timed pause is explicitly unsupported. Effects currently use a single point emitter.

Water rendering is unavailable. The water asset/component, mesh generation, shaders, reflection/refraction passes, depth/shoreline effects, editor integration, and validation scenes from the roadmap remain to be implemented.

The roadmap's GPU pass graph, HDR presentation, shadows and lighting completion, device recovery, runtime backend transitions, resource stress testing, performance certification, and release packaging also remain open. Next: catalog path/lifecycle hardening and the ResourceSystem bridge, catalog-to-compiled-material rendering, then imported animation and particle scene integration. See [the revised plan](WPGRAPHICS_PRODUCTION_PLAN.md) for dependencies and acceptance gates. Do not treat passing this stage's tests as a production certification.
