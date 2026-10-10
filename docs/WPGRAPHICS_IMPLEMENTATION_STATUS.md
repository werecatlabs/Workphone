# WPGraphics implementation status

## Terrain data and Editor workflow increment — 10 October 2026

Starts from `brodex` at `ccb890ed6`. The merge preserves the cooked-material
increment and adds Lua runtime, reload, debugger and tooling changes. The remote
`brodex` SHA was verified over HTTPS after the machine's SSH authentication failed.

This implements the first connected terrain increment from
[the terrain plan](WPGRAPHICS_TERRAIN_PROCEDURAL_REVIEW.md): TERR-01 foundations,
the query/picking/CPU mesh portion of TERR-02, and basic TERR-04 authoring.
It does **not** complete GT0, R1, or the whole terrain/procedural programme.

- Immutable rectangular `TerrainData` snapshots carry dimensions, spacing, explicit
  sample-zero origin, height scale and row-major samples. Revisions reject stale
  commits; validation bounds allocations and rejects invalid/nonfinite or collapsed
  geometry. Maximum source size is 4,194,304 samples, with at most 8,193 per axis.
  Zero and negative height scales are supported. Object transforms support finite
  translation, positive nonuniform scale and normalized yaw rotation; pitch/roll
  and singular transforms are rejected without replacing the valid placement.
- Queries, exact triangle rays, CPU meshes and Claw native geometry share the same
  cell diagonal and height units. Queries retain snapshots without copying all
  samples. Legacy setters preserve the old `-dimensions/2` sample-zero anchor;
  newly generated assets use `-(dimensions-1)/2`. Origin is saved explicitly.
  Claw renders the full rectangular grid without the old 257-sample decimation.
  Native mesh construction retains the previous mesh if preparation fails.
- `TerrainSystem` owns actual samples independently of renderer lifetime.
  Version-one `workphone.terrain` JSON saves dimensions, spacing, origin, scale and
  all samples; malformed files leave the current revision intact. Scene `toData`
  carries this payload while normal inspector properties remain editable.
  Sample files use terrain-local coordinates; actor placement belongs to the scene.
  Legacy scenes without samples migrate to a flat grid with their original anchor.
  Renderer attachment/recreation restores an actor's existing placement; invalid
  actor placement rejects attachment before changing the retained runtime.
- Existing Lua TerrainEditor exposes local X/Z brush centres and real raise,
  lower, smooth and flatten actions. Each action makes one native undo command.
  A patch is limited to 65,536 changed samples and the existing managers retain
  at most 100 commands; their cursor/eviction/redo-branch handling is repaired.
  MT history blocks undo/redo while commands are pending or running. Explicit
  removal, clear and unload cancel unstarted jobs; history eviction preserves
  requested operations. Running commands release the history lock, and unload
  defers their cleanup until completion. Undo/redo validates metadata and
  affected sample values, preserving unrelated edits and rejecting conflicts.
  Under the existing command API, a rejected undo
  still advances the history cursor and logs the failure.
- Save, Reload and height-data export operate on actual versioned sample files,
  with same-directory temporary-file replacement. Export JSON also writes actual
  samples, so it cannot overwrite a terrain file with settings-only recipes.
  The separate Export Recipe Settings action appends `.recipe.json` to that path.
  Unimplemented terrain operations report unavailable.

Editor use: select a TerrainSystem, generate a height preset or import a saved
sample file, set **Brush Centre X/Z**, radius and strength, then use a supported
brush action. Raise/lower strength is a terrain-local height delta; smooth/flatten
strength is a 0–1 blend. Use the existing Edit undo/redo commands, set an output
file, and Save/Reload. Viewport drag strokes, tablet/mirror modifiers, image/raw
height import, mesh export, stamp/layer paint, erosion and collision/navigation
builds remain unavailable in this increment.

Validation on 10 October used CMake/CTest 4.4.4, Visual Studio Community 2026,
MSVC 19.51.36260, Windows SDK 10.0.26100 and NVIDIA RTX 3090 DX11 hardware.
The full graphics/catalog/resource baseline passed in both Debug and
RelWithDebInfo: **16 passed, 1 unavailable** in each configuration. The sole
unavailable test remains `WorkphoneGraphics.mesh_import_assets`, whose six
external Ogre/OgreNext mesh fixtures are absent.

The added mandatory targets are `WPGraphics.terrain_contracts` and
`WPGraphics.terrain_lua_workflow`; the existing DX11 production target also checks
terrain edits and full-resolution geometry. Native coverage includes actor
reattachment, JSON/scene round trips, invalid/stale edits and queued history
completion/cancellation/unload. The Lua target loads the real Editor script and
exercises sculpt, undo/redo, save/reload and unsupported-operation errors.

JUnit reports and source/binary/device evidence are in
`cmake-build-debug-vs2026-readiness/claw-graphics-{Debug,RelWithDebInfo}-results.xml`
and the corresponding `-evidence.json` files. `git diff --check` passed. These
local runs do not constitute an interactive Editor acceptance run, packaging
validation or release certification.

Remaining release work includes physics publication/contact evidence, tiled LOD,
four-layer cooked PBR, worker cancellation and shared render/physics revision
publication, streaming, package reload, and an interactive Editor acceptance run.
GPU buffer creation remains lazy in the existing renderer: native mesh staging
does not yet guarantee atomic source/native/GPU replacement after a DX11 allocation
failure. This increment supplies no new large-map performance certification.

The DX11 fixture also exposed a pre-existing rotated-camera issue in
`Engine/c/Source/WorkphoneCore/workphone_matrix.c`: `wp_mat4f_look_at` places its
rotation basis in columns while its translation uses row dot products. The
terrain pixel check explicitly validates the resulting eye-space depths and
does not certify camera placement. Correcting that shared camera path and its
other renderer consumers remains separate work.

## Cooked material/texture consumer increment — 10 October 2026

Implementation starts from `brodex` at `7107f0a39`. The prior catalog/path/adapter
increment is retained in `7809b208c`. Subsequent branch updates add scene/texture/
actor scheduling changes, state-driven lights, tyre effects, importer/dependency
updates and physics narrowphase/BVH/collision-cache work. The docs add shared
resource and animation plans and expand terrain, procedural and foliage scope.
This increment follows the graphics plan's next executable cooked-asset task;
the complete physics, vehicle and Editor suites remain outside its validation.

Implemented:

- Registered `GraphicsResourceCompiler` supplies versioned `texres` and `matres`
  JSON descriptors through the existing compiler registry. Textures cook bounded
  top-down BGRA mip chains with the existing semantic filters. Materials declare
  one texture as compile/install dependency and pin its identity, source/payload
  hashes and compiler version. Typed readers validate lengths, versions, finite
  values, mip layout, integrity, dependency coherence and target/build mode.
- `ClawMaterialResource` resolves a catalog UUID through the existing adapter,
  stages an entire material/texture bundle on its render owner thread and swaps
  it only after validation. The final catalog token check and pointer swap share
  the catalog lock. Compile/decode/upload, compiler callbacks and retired-handle
  destruction stay outside that lock. Publisher identity, monotonically ordered
  requests and a pinned DX11 device reject stale or foreign candidates. Failed
  replacement preserves the last working bundle; explicit consumer unload
  invalidates outstanding candidates. Device changes suppress `current()` until
  a compatible replacement is installed.
- `ClawTexture::uploadCookedMips` stages exact authored mip bytes, retains them
  through unload/reload and device recreation, and rejects invalid or failed
  uploads while preserving the old compatible view. Legacy pixel/property edits
  deliberately leave cooked mode. Cached views now compare actual device identity
  as well as the native renderer wrapper.
- Repaired two regressions from the recent changes: the render owner must apply
  an equal authored transform when that transform is still queued for the native
  scene node; particle traversal must retain a ConcurrentArray snapshot.
- Material passes now retire their own state records and managed contexts when
  rebinding or unloading, releasing textures retained by that state. Supplied
  contexts and pre-existing records remain owned by their caller. Lifecycle
  contracts check texture destruction and managed-context counts.
- Added `WPGraphics.cooked_material_resource` with generated image/descriptors,
  real SQLite/catalog/cook/load, exact GPU mip readback and visible `renderMesh`
  output. Format failures, reload ordering, catalog lifecycle, owner-thread and
  device recreation have dedicated contracts. The existing production test adds
  queued-transform and cooked-texture regressions.
- The baseline now requires 15 tests and hashes the typed compiler/consumer
  sources. The deliberately deleted CI workflow remains absent; the local runner
  records its availability instead of failing to hash a nonexistent file.

Validation: **Debug: 14 passed, 1 unavailable. RelWithDebInfo: 14 passed,
1 unavailable.** The sole unavailable test is `WorkphoneGraphics.mesh_import_assets`,
which still requires the six absent external Ogre/OgreNext mesh fixtures. All
mandatory contracts execute, including actual catalog symlink containment. The
new cooked-resource test uses generated fixtures and has no unavailable cases.

Both configurations used Visual Studio Community 2026, MSVC **19.51**, Windows
SDK **10.0.26100.0**, CMake/CTest **4.4.4**, and DX11 on **NVIDIA GeForce RTX 3090**,
feature level **11.0**, driver **32.0.16.1692**. Configuration-specific JUnit and
JSON evidence is in `cmake-build-debug-vs2026-readiness/claw-graphics-<configuration>-results.xml`
and `claw-graphics-<configuration>-evidence.json`. The JSON records the source
revision/diff, compiler, device, test sources and executable hashes, and the
absent CI workflow. To rebuild and validate both configurations:

```powershell
./Tools/BuildWPGraphicsBaseline.ps1 -BuildDirectory cmake-build-debug-vs2026-readiness
```

See [cooked resource authoring and ownership](WPGRAPHICS_COOKED_RESOURCES.md) for
formats, service setup, exact limits and lifecycle requirements. This advances
DB-07/08/10, GD and shared RES-010/019/021/022 for one synchronous graphics family;
it does not complete those broad gates. Source-free packaged loading, immutable
cook generations/metadata commit, isolated variants, cross-process writers,
input snapshots, asynchronous jobs/watchers and Editor preview adoption remain
open. In particular, ResourceSystem's existing output-before-metadata-commit
failure window can change disk output even while this consumer retains its old
GPU bundle. No R1/R2 or release certification is claimed.

## Catalog identity and resource adapter increment — 8 October 2026

Implementation starts from `brodex` at `22553f942`, following the verification increment committed as `f531e9140`. **Debug: 13 passed, 1 unavailable. RelWithDebInfo: 13 passed, 1 unavailable.** The unavailable test requires the six missing external Ogre/OgreNext mesh fixtures. Catalog coverage includes actual symlink containment and Windows 8.3 root aliases in both configurations, with no internal unavailable cases. Neither R1 nor R2 is certified.

Both configurations used Visual Studio Community 2026, MSVC **19.51**, Windows SDK **10.0.26100.0**, CMake/CTest **4.4.4**, and DX11 on **NVIDIA GeForce RTX 3090**, feature level **11.0**, driver **32.0.16.1692**. Current evidence is in `cmake-build-debug-vs2026-readiness/claw-graphics-<configuration>-results.xml` and `claw-graphics-<configuration>-evidence.json`. Earlier sections retain their original revision context.

The intervening branch changes retain the catalog and baseline tooling. They add DX11 scene fog, semantic colour/data/normal/cutout/roughness mip generation and GPU upload, texture mip properties, serialized LOD detail bounds, a synchronous small-batch LOD path, core container/listener changes, and expanded procedural vehicle scenery, physics capacity and handling tests. The new fog/mip tests exercise native GPU output; the complete vehicle/editor suites are outside this increment's selected graphics scope.

Implemented in this increment, validated in Debug and RelWithDebInfo:

- Schema version 2 adds explicit file/scene entry kinds and a canonical file-path key. File paths are stored relative to a captured project root with `/` separators; lookup keys fold ASCII case on Windows. Windows 8.3 root aliases are accepted and normalized to the canonical root. Equivalent spellings resolve one identity, and paths outside the root, invalid UTF-8, ambiguous Windows paths and unresolved/outside-root links are rejected. Missing source files remain representable. The root is explicitly configurable while closed; otherwise it is captured from the application project or database directory. Relative storage supports project relocation.
- Transactional upgrades from unversioned/version-1 catalogs preserve row IDs and UUIDs, retain original row snapshots (`wp_asset_catalog_backup_v0` / `wp_asset_catalog_backup_v1` as applicable), and fail closed on canonical collisions, reserved backup names or unsupported metadata. Migration also rejects custom triggers whose stored table-name casing differs from the actual legacy table: bundled SQLite 3.7.9 can silently omit those triggers while reloading the altered schema. The original database is preserved for explicit repair; an exact-case abort trigger covers rollback after schema alteration. Existing version-2 rows are revalidated. These are in-database snapshots, not independent backups.
- Detached entry snapshots carry catalog-instance and generation identity. Successful mutations and lifecycle changes invalidate outstanding snapshots; failed/conflicting/idempotent changes do not fabricate new identities. Open/close, narrow/wide loading, switching and root changes use the same catalog lifecycle. Shared virtual interfaces remain unchanged.
- `CatalogResourceAdapter` resolves catalog UUIDs to existing ResourceIDs and delegates compilation/loading to ResourceSystem. Its caller must bind the exact ResourceSystem `sourceRoot`, matching the catalog root, and rebuild the adapter if the service is reconfigured. Explicit catalog-type mappings validate the source extension without changing its type; canonical-key comparison permits ResourceID's lowercase-extension normalization on Windows. Compiler availability, loaded identity/type and compiler version are checked. Compilation hashes, dependencies and cooked containers remain owned by ResourceSystem and its existing database.
- ClawTexture retains authored mip settings through unload/reload, validates atlas metadata, and stages replacement views. A rejected mip candidate preserves the previous same-device GPU view; a successful candidate replaces it. Added wrapper-level regression coverage checks actual GPU pixels, metadata rejection, replacement, caching, reload and unload.
- Fixed standalone inclusion of `ClawHammerSystem.hpp` by using the equivalent engine `f32` declaration instead of depending on a prior native `wp_f32` declaration. Validation evidence now hashes the new contract helper headers and catalog/path/adapter implementation files, so reports identify the code exercised by these additions.

This advances **DB-01/03/04/05/06/07** in part. Catalog/adapter contracts cover canonical aliases, kind separation, migration rollback, relocation, lifecycle snapshots, type/root/version errors and stale requests around compilation. Adapter checks before and after ResourceSystem work reject stale returned results; they do not make compilation or publication atomic with catalog mutations. The existing ResourceSystem compiler can still persist an old output before the adapter detects staleness. No catalog-to-GPU publication transaction, graphics material/texture compiler, subasset identity, packaged runtime manifest or end-to-end cooked graphics draw is delivered here. General mutation/unload races, cancellation, cross-process coordination and durability remain open.

The next executable task is a catalog-resolved cooked material plus texture reaching a DX11 draw, using the adapter with staged render-thread publication and a final generation guard. Failed compilation/upload must preserve the last visible asset across rename/reload/unload. That evidence remains required for **GD** and delivery batch B. Remote CI and external-media release coverage also remain outstanding.

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

At this verification revision, the next implementation increment was canonical catalog paths/entry kinds and lifecycle policy, then UUID/source-to-ResourceID mapping and a cooked material/texture reaching DX11. The current increment above advances the identity/adapter portion. Batch A's historical local verification deliverable is complete; external-media release coverage and remote CI confirmation remain outstanding.

## Planning review before the rebuild — 8 October 2026

Source reviewed at `d2e81cdb4`. The catalog repairs/migration, explicit CPU skinning and basic DX11 particles remain present. Recent source also adds long-text UI/serialization, visibility, editor-camera-during-Play and sample capture/winding improvements.

A current `project_x64` Debug CTest attempt selected 14 graphics/catalog/resource targets: **14 unavailable, 0 executed**, because their expected executables were missing. This is a build-artifact/verification gap, not evidence of source assertion failures. No rebuild, performance measurement or GPU capture was performed for this planning request. `.github` and its previously documented workflows are absent from this checkout; restore/version CI before marking that work complete.

The revised [production plan](WPGRAPHICS_PRODUCTION_PLAN.md) starts with a rebuilt traceable baseline, then canonical catalog identity and a cooked material/texture reaching DX11, followed by imported animation and particle scene integration. The implementation and test results below are historical evidence, with their original configurations; neither release gate is certified.

## Historical implementation evidence

Catalog increment validated on 6 October 2026, Windows x64, Visual Studio 2026/MSVC 19.51, CMake 4.4.4, RelWithDebInfo, starting from `d5c859612` on `brodex`.

Local evidence at that revision: the required SQLite catalog target and its Workphone/WPSQLite dependencies were built, and the catalog contracts passed without graphics or external media. The two isolated skinning/particle reference tests also passed. The catalog preset was configured with the existing verification build directory overridden to `cmake-build-debug-vs2026-readiness`.

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

Use `-RequireExternalAssets` when validating a release environment with the media fixtures installed; missing media blocks that gate. The prior verification baseline was configured/built through `BuildWPGraphicsBaseline.ps1` with `-BuildDirectory cmake-build-debug-vs2026-readiness`, then validated in both configurations. This is a selected-target local rebuild, not a clean-machine packaging check. Current Debug and RelWithDebInfo validation passes.

Historical catalog result: **1 CTest target passed**, exercising the CRUD and migration contracts above. Historical isolated native reference result: **2 passed**. Historical full graphics result: **11 passed, 1 skipped**. The subsequent verification baseline reported **13 passed, 1 unavailable per configuration**. Current results are recorded at the top of this document. Remote CI remains unverified; its definitions are present.

## Remaining release work

AssetDatabaseManager is included in the [production plan](WPGRAPHICS_PRODUCTION_PLAN.md), under M1A and DB-01 through DB-10. Schema-v2 canonical file identity, explicit file/scene kinds, catalog-instance/generation snapshots and a ResourceSystem adapter are implemented and validated in Debug and RelWithDebInfo. Legacy conflicts still require explicit repair; no duplicate identities are silently discarded. Remaining work includes subassets, complete reimport/move reference handling, broader mutation/unload and cross-process races, durability/busy/cancellation policy, independent recovery, non-ASCII case/general short-name alias policy, and read-only runtime manifests. Captured Windows 8.3 root aliases are covered. Path resolution is an identity policy rather than a filesystem security boundary. No director cache is populated; bounded cache policy is required if caching returns. GD remains unsatisfied until catalog-resolved cooked assets reach a DX11 draw and failure preserves the last visible asset through coordinated catalog, compilation and GPU publication.

Animation remains experimental: asset import, skeleton/clip ownership, animation controllers and graphs, per-instance poses, automatic scene integration, GPU skinning, morph targets, IK, root motion, animated culling, and animation editor workflows are not completed.

Particles remain experimental: template/resource loading, emitter shapes, multiple emitters, curves, bursts, collisions, trails, flipbooks, globally sorted transparency, soft particles, lighting, robust editor authoring, scalability policies, and performance budgets remain. Timed pause is explicitly unsupported. Effects currently use a single point emitter.

Water rendering is unavailable. The water asset/component, mesh generation, shaders, reflection/refraction passes, depth/shoreline effects, editor integration, and validation scenes from the roadmap remain to be implemented.

The roadmap's GPU pass graph, HDR presentation, shadows and lighting completion, device recovery, runtime backend transitions, resource stress testing, performance certification, and release packaging also remain open. Next: deliver catalog-to-compiled-material/texture rendering with coordinated publication, then imported animation and particle scene integration. See [the revised plan](WPGRAPHICS_PRODUCTION_PLAN.md) for dependencies and acceptance gates. Do not treat passing this stage's tests as a production certification.
