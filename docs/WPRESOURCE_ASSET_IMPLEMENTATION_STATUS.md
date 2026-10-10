# Resource and asset implementation ledger

Scope: implementation started on 2026-10-10 against the production plan in
[WPRESOURCE_ASSET_PRODUCTION_PLAN.md](WPRESOURCE_ASSET_PRODUCTION_PLAN.md).
This ledger records the integrity and cooking increments. The full plan and
production release gates remain open.

## Implemented in this increment

| Area | Result | Remaining scope |
| --- | --- | --- |
| Authored database durability | Catalog initialization verifies full synchronization; maintenance retains durable journaling and synchronization. Legacy DML returns affected rows or failure. | Busy/retry policy, storage exhaustion, encryption-aware backup policy and dependency upgrade. |
| Connection ownership | All SQLite open/close/query/DML paths share a recursive connection guard. Catalog transactions hold that guard across begin/commit/rollback. Results are detached while the guard is held. | Broader lock-order audit and multi-process mutation admission. |
| Persisted invalidation | Resource insert/update/delete triggers advance a transactional revision. Entry snapshot checks detect raw SQL and independent-connection changes, including updates that retain identity. Rolled-back changes retain the previous revision. | Per-entry revisions, watcher reconciliation and atomic publication to consumers. |
| Backup | SQLite online backup produces an independent snapshot and publishes it without overwriting existing destinations. Catalog exposes the API. | Editor controls, restore workflow, integrity checks, encryption and retention policy. |
| Legacy file loading | Exact canonical catalog lookup replaces substring fallback and create-on-load behavior for file paths. UUID queries use bound parameters; absent graphics/audio managers return failure. Reference database scans use a separate connection. | Structured diagnostic outcomes, authored numeric-ID mapping and bounded domain traversal. |
| Native paths | Compiled payload I/O, runtime manifests, Lua compiler inputs and compilation database directories use native UTF-8 filesystem conversion. | Audit remaining importers, third-party loaders and long paths. |
| Asset operations | One catalog service journals move/copy/delete, stages files on disk, updates descendant catalog rows transactionally, preserves move identities and creates new copy identities. Delete/undo preserves binary content and empty directories. Undo rejects changed bytes, identity conflicts and occupied destinations. | Sidecar/settings-cache migration, reference rewriting, notifications to loaded consumers, quotas/retention and asynchronous progress/cancellation. |
| Recovery | Catalog reopen recovers prepared operations and interrupted undo to the previous committed filesystem state. Conflicts preserve content and retain pending records. New file operations attempt recovery first and stop on unresolved records. | Recovery UI, process-kill/power-loss certification and external-writer coordination. |
| Editor integration | Project Assets copy/move and RemoveResourceCmd delete/undo use the shared service and report errors. | Copy/move command-stack undo integration, database Editor, pickers, inspectors and full interactive acceptance. |

## Operation contract

The project catalog remains version 2. Additive tables `wp_asset_catalog_changes`,
`wp_asset_operations` and `wp_asset_operation_entries` store revision and journal
state. Existing resource rows retain their UUIDs and canonical paths.
The backup API snapshots the database only; project backups must also retain
source assets, metadata and operation quarantine directories.

Each file operation records an identity snapshot and byte/tree fingerprint before
changing files. It stages content under `.workphone/asset-operations/<id>/payload`
and commits catalog changes after successful filesystem publication. An operation
that returns failure can have a pending journal record and staged/published files;
callers invoke recovery, and opening the catalog invokes recovery automatically.
No recovery path recursively deletes content. Copy undo also retains its bytes in
quarantine. This intentionally consumes disk until a retention/GC policy is built.

Operations reject out-of-project paths, reserved storage, links/reparse points in
asset trees, existing destinations and trees above 100,000 entries. Native Windows
renames use exclusive publication with write-through. The current fingerprint is
FNV-64 for accidental-change detection, not an authenticated content digest.
Canonical path auditing is not a security boundary against hostile concurrent
filesystem replacement. Filesystem/database power-loss atomicity is not certified.

Copying `.fbscene`, `.fbscenebin`, `.fbscenexml`, `.prefab`, `.fbprefab` and `.resource`
files is rejected until format-aware remappers are registered. Other copies receive
new catalog identities; internal authored references are not rewritten. Case-only
renames, cross-volume moves and simultaneous file operations from multiple Editor
processes are not supported by this increment.

## Validation

Rebuilt on Windows/MSVC v145 in `project_x64`, `RelWithDebInfo`. Four focused
CTest suites passed: `WorkphoneAssets.catalog_contracts` (3.68 s),
`WPGraphics.cooked_material_resource` (1.50 s), `WPResourceTests` (0.37 s)
and `WPLua.runtime_contracts` (0.68 s). The first three ran together against the
corrected Workphone DLL; Lua ran after its target rebuilt. These are focused
contract results, including a graphics test, not representative hardware or
interactive Editor certification. The `Editor` target and required dependencies
built and linked successfully, including the modified Project Assets and delete
command sources. `git diff --check` passed (line-ending conversion warnings only).

Added real SQLite tests cover full-sync maintenance,
raw and independent-connection invalidation, rollback, transaction exclusion,
affected-row/failure reporting and independent backup/no-overwrite. Added operation
tests cover all byte values, empty directories, move/copy identity, conflict-aware
undo and reopen recovery after an injected catalog failure following filesystem
publication, plus a simulated interruption between undo filesystem and catalog
updates. Backup coverage includes committed WAL content. Runtime fixtures now run
under a Unicode temporary root.

Commands:

```powershell
cmake --build project_x64 --config RelWithDebInfo --target WPResourceTests WPAssetCatalogTests WPGraphicsCookedResourceTests LuaRuntimeTests Editor --parallel 8
ctest --test-dir project_x64 -C RelWithDebInfo -R '^(WPResourceTests|WorkphoneAssets\.catalog_contracts|WPGraphics\.cooked_material_resource|WPLua\.runtime_contracts)$' --output-on-failure
git diff --check
```

Build logs are retained in `project_x64/resource-implementation-build.log` and
`project_x64/resource-implementation-build-followup.log`. The initial catalog test
compile caught a const lambda capture; its first run caught the missing revision
refresh in snapshot capture. Both were corrected before the passing runs.
No Debug build, interactive Editor walkthrough, standalone asset database Editor,
process-kill/power-loss run, real package launch or performance benchmark was run.

## Increment 2: immutable cooked artifact publication

Implemented and validated after the initial integrity increment, against source
baseline `dd298fe59114c5cb94142f797a8ca35ffd23ffcb` plus the working-tree changes.

- Cooker artifacts now live under `artifacts/v2/<encoded-target>/<editor-or-package>/<encoded-resource>/<input-output-fingerprints>.wprs`.
  The reversible byte encoding distinguishes case on Windows, escapes separators,
  avoids reserved device names/trailing dots and bounds each directory component.
  It distinguishes parent/subresource IDs that formerly flattened to one filename.
- Files publish exclusively from a staged container. A compilation-index commit
  failure leaves the previous record and its artifact intact; the new artifact is
  an unreferenced candidate. Existing generations are retained, including damaged
  files when a repair is published at a fresh path. GC/retention is not implemented.
- Runtime manifest payload v2 records each artifact's relative path as well as
  identity, version and hashes. Runtime v1 reading remains supported for existing
  source-free packages. New manifests pin exact immutable generations, so recooking
  or changing target/mode does not change an already-exported package's inputs.
  Manifest exports cannot overwrite reserved artifact/staging files.
- Authoring loads use committed artifact paths. Material compilation consumes
  pinned dependency artifacts supplied in CompileContext rather than reconstructing
  flattened texture filenames. CompileContext.outputPath is provisional staging
  information; compilers write the supplied stream, and CompilationReport.outputPath
  is the final published artifact.
- Source content is checked before dependency scanning and again before publication;
  compiler version, raw dependencies and pinned resource artifacts are rechecked
  after compilation. Detected changes fail without committing the candidate.

Compatibility: recook legacy authoring caches to populate the new artifact paths;
old loose files are retained. Consumers/exporters must use report/index paths or
manifest v2 mappings. ResourceID.compiledRelativePath remains the legacy-layout
helper used by v1 runtime mounts, not the current cooker output location. The WPRS
outer container format is unchanged; the manifest's inner payload is now version 2.
The DDS extension (RES-050–054) remains planned; future native DDS texture bytes fit
inside the same target-specific immutable WPRS artifacts.

Limitations: the compilation index still keeps one active record per logical ID,
so switching target/mode may require rechecking/rebuilding its active record.
All variant artifacts and already-exported manifests coexist, but multi-variant
index queries/BuildKey schemas remain open. Artifact fingerprints retain the
existing FNV-64 scheme and are not authenticated digests. Final input rechecks do
not replace immutable source snapshots against arbitrary external writes.
Whole dependency-closure commit, owner-thread compatible-set installation,
cross-process scheduling, long-path certification, GC and power-loss durability
remain release gates. This increment establishes per-artifact/index failure safety,
not complete production certification.

New regression coverage includes flattened subasset aliases, Windows case-distinct
artifact files, injected index-commit
failure followed by cache eviction/load, retained earlier artifacts, Editor/package
and target coexistence, loading an old pinned manifest after those builds, and
compiler-time source mutation. The relocated Unicode read-only runtime fixture
copies committed paths and tests the v2 manifest. Existing graphics and unit fixtures
now use report paths/pinned dependencies.

The first rebuilt catalog/runtime run failed with temporary container names nested
alongside deep artifact paths. Keeping payloads and container assembly in the short
`.staging` directory resolved those failures. General long-path certification remains
open. Final `RelWithDebInfo` builds succeeded for `WPResourceTests`,
`WPAssetCatalogTests`, `WPGraphicsCookedResourceTests`, `LuaRuntimeTests` and `Editor`,
including their required dependencies. A final incremental rebuild included all
regression fixture edits. The consolidated CTest run passed **4/4**:

| Suite | Time |
|---|---|
| WorkphoneAssets.catalog_contracts | 3.14 s |
| WPGraphics.cooked_material_resource | 1.36 s |
| WPLua.runtime_contracts | 0.59 s |
| WPResourceTests | 0.61 s |

Build logs: `project_x64/resource-generations-build.log` and
`project_x64/resource-generations-final-build.log`. `git diff --check` passed.
The Boost unit fixture was adapted to the new report paths but its separate
monolithic unit target was not built/run. No interactive Editor acceptance,
independent shipping package launch, representative hardware benchmark, Debug
build or process-kill/power-loss certification was performed in this increment.

## Next implementation gates

User-requested scope extension: production-plan packages **RES-050–054** add
platform texture profiles, Windows DDS cooking, native compressed uploads,
Editor target previews and package certification. These are planned, not yet
implemented. Keep their identity/settings/BuildKey decisions integrated with the
existing texture compiler and the next cooking consistency milestone.

1. Finish the first integrity slice: authored numeric-ID lookup/traversal, immutable
   metadata migration, reference-aware operations, retention and recovery UI.
2. Complete variant-aware BuildKey indexing, whole-closure generation publication,
   source snapshots, recovery and retention on top of the immutable artifacts.
3. Persist typed dependency/reference edges and implement reverse queries,
   dependency invalidation and required-missing/cycle diagnostics.
4. Integrate typed runtime consumers and owner-thread publication throughout the
   Editor and shipping sample; complete asynchronous session-safe jobs.
5. Implement the asset database Editor, inspectors, preview/pickers, packaging
   controls and the declared format-family matrix.
6. Execute the plan's representative Editor, source-free package, failure,
   concurrency, performance and release gates. Passing focused tests alone does
   not establish production readiness.
