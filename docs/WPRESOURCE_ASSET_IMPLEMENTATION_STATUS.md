# Resource and asset implementation ledger

Scope: implementation started on 2026-10-10 against the production plan in
[WPRESOURCE_ASSET_PRODUCTION_PLAN.md](WPRESOURCE_ASSET_PRODUCTION_PLAN.md).
This ledger records an initial integrity increment. The full plan and production
release gates remain open.

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

## Next implementation gates

1. Finish the first integrity slice: authored numeric-ID lookup/traversal, immutable
   metadata migration, reference-aware operations, retention and recovery UI.
2. Fix cooked output ownership: collision-free BuildKey namespaces and immutable
   build generations with atomic index publication and last-good recovery.
3. Persist typed dependency/reference edges and implement reverse queries,
   dependency invalidation and required-missing/cycle diagnostics.
4. Integrate typed runtime consumers and owner-thread publication throughout the
   Editor and shipping sample; complete asynchronous session-safe jobs.
5. Implement the asset database Editor, inspectors, preview/pickers, packaging
   controls and the declared format-family matrix.
6. Execute the plan's representative Editor, source-free package, failure,
   concurrency, performance and release gates. Passing focused tests alone does
   not establish production readiness.
