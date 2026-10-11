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

Historical limitation at the end of increment 2: the compilation index kept one
active record per logical ID. Increment 3 below replaces that target/mode indexing.
The complete BuildKey/profile/toolchain schema remains open. Artifact fingerprints retain the
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

## Increment 3: variant-aware compilation records and loading

The compilation database now uses schema **3**, with `(resource_id, target,
packaged_build)` primary keys and matching composite dependency foreign keys.
Records and edges commit together; deletion cascades only within the selected
variant. Explicit queries never fall back to another target or mode, and
`compileAffected` walks the selected variant's reverse graph.

The ResourceSystem uses explicit variants for compilation, authoring loads and
manifest export. Authoring starts in Editor mode; the last successful root compile
or wholly successful nonempty affected batch selects the mode for subsequent
loads/exports. A failed build preserves that selection. Each load/export closure
uses one mode throughout; cache and current-session publication evidence keys
include mode. Failed builds revoke publication evidence for affected resources
and their dependents, while last-good committed artifacts remain loadable.
Publication invalidation conservatively clears both modes through the aggregate
graph; it does not delete their persisted records.
Authoring loads also verify the mode-qualified artifact namespace and committed
compiler/source/payload fingerprints before installing newly read containers.

Compatibility: schema 0–2 compilation indexes are derived data and are invalidated
transactionally, requiring recooking. This migration does not change authored
catalog tables or remove existing artifact files/manifests. Databases with a newer
schema are rejected. The old unqualified `getRecord` returns a sole variant and
rejects ambiguity; unqualified dependency queries aggregate variants and removal
removes every variant of that logical resource. Legacy commits use a reserved
empty-target namespace. The interface adds explicit variant methods, so database
implementations and linked consumers must be rebuilt together.

Regression coverage includes three coexisting target/mode records; transaction
rollback preserving the previous record and edges; persistence after reopening;
scoped forward/reverse queries and cascading removal; ambiguous legacy reads;
two ResourceSystems using independent SQLite connections; Editor/package
UpToDate reuse and restart reuse; variant-specific affected builds; retained
instances across mode switches; one-mode install closures and runtime manifest
export; and failed-build publication revocation/selection preservation.
An independent Python fixture runner exercises native schema-1/2 migration,
preservation of an authored sentinel table and rejection of a newer schema.
Its first native run exposed an active `PRAGMA user_version` cursor preventing
replacement of existing tables. The migration now finalizes that cursor before
starting schema replacement. The runner explicitly closes Python connections,
including on failure, so Windows can release and clean up fixture files.
CTest registers `WPResource.index_migration` when a Python interpreter is found.

The rebuilt `RelWithDebInfo` contracts passed in targeted CTest runs:

| Suite | Time |
|---|---|
| WorkphoneAssets.catalog_contracts | 3.97 s |
| WPResourceTests | 0.90 s |
| WPResource.index_migration | 0.47 s |
| WPGraphics.cooked_material_resource | 1.54 s |
| WPLua.runtime_contracts | 0.70 s |

`RelWithDebInfo` builds succeeded for `WPResourceTests`, `WPAssetCatalogTests`,
`WPGraphicsCookedResourceTests`, `LuaRuntimeTests` and `Editor`, including their
required dependencies. Build log: `project_x64/resource-variants-build.log`.
`git diff --check` passed. These tests exercise the updated native service/database and cooked
consumer fixtures; they do not certify interactive Editor workflows, a standalone
shipping package, representative hardware, Debug or process-kill/power-loss safety.

This completes target/mode separation within RES-013, not its full BuildKey
contract. Settings/profile/architecture/backend/toolchain variants, strong digests,
immutable input snapshots, cross-process writer admission, atomic compatible-set
publication, package/Editor controls, DDS cooking and native compressed texture
uploads remain open. No interactive Editor or shipping-package acceptance is
claimed by these source changes.

## Increment 4: complete-request index publication and writer admission (2026-10-11)

The shared compilation database exposes `commitCompilations`: every changed record
and dependency edge in a request commits in one SQLite transaction. Invalid or
duplicate variant records are rejected; an error in any member rolls back the
whole set. Single-resource commit APIs delegate to this transaction path. Schema
3 and existing artifact/manifest formats remain unchanged.

The cooker accumulates candidate records while building immutable artifacts.
It publishes the index only after all requested roots and their dependencies
succeed. `compileAffected` treats its requested roots as one set. On a compiler
or index failure, no candidate record becomes active, successful-looking sibling
reports are changed to failure, old cache/install objects remain valid, and
publication eligibility for the attempted closure is revoked. A successful
transaction clears the runtime cache and enables publication of the checked set.
Unreferenced candidate files are retained for future recovery/GC work.

Authoring now holds an OS writer lease for the canonical cooked root and target
through shutdown. Both build modes share that admission scope. A second session
for the same target fails before opening its index; other targets can coexist.
Windows uses a non-inheritable file descriptor with exclusive sharing; POSIX uses
nonblocking `flock`. Process exit releases ownership. Lock files persist and must
not be unlinked to implement stale-lock recovery: ownership follows open handles.
Runtime-only mounts require no lease and continue reading pinned immutable paths.
Manifest export cannot overwrite reserved writer-lease storage.

Compatibility: custom compilation database paths must canonicalize inside the
cooked root, outside artifacts, staging and lease storage. External indexes are
rejected without opening them, preventing another cooked root from sharing a
target's index while bypassing root admission. Existing external-index users must
relocate their derived index or recook with the default path. Direct compilation
database clients are responsible for their own writer admission; this lease
contract covers ResourceSystem authoring sessions. Rebuild linked consumers for
the added batch database interface.

Regression coverage includes a child cooking before parent failure; last-good
closure loading after cache eviction; whole-closure commit failure and recovery;
retained old install objects; multi-root affected-build failure and recovery;
multi-record transaction rollback of records and edges after reopening; duplicate
batch rejection; same-process writer rejection; and independent-process writer
contention, canonical-root aliases, distinct targets, normal shutdown, forced
process termination and initialization failure. External/shared-across-root index
paths are rejected. Python-backed CTest fixtures use the rebuilt native service.

The rebuilt Windows `RelWithDebInfo` contracts passed in targeted CTest runs:

| Suite | Time |
|---|---|
| WPResourceTests | 1.95 s |
| WPResource.writer_lease | 3.46 s |
| WPResource.index_migration | 0.51 s |
| WorkphoneAssets.catalog_contracts | 3.57 s |
| WPGraphics.cooked_material_resource | 1.67 s |
| WPLua.runtime_contracts | 0.68 s |

`RelWithDebInfo` builds succeeded for `WPResourceTests`, `WPAssetCatalogTests`,
`WPGraphicsCookedResourceTests`, `LuaRuntimeTests` and `Editor`, including required
dependencies. Build log: `project_x64/resource-closure-build.log`.
`git diff --check` passed.
Windows and POSIX lease implementations are present; the POSIX path was not
compiled or exercised on this Windows host. No interactive Editor acceptance,
standalone shipping-package launch, Debug build or hardware performance
certification was performed.

This implements complete-request index atomicity and participating-writer
admission. It does not certify power-loss recovery, immutable source snapshots,
strong digests, generation journals/recovery/GC, graph-wide owner-thread consumer
swaps, or arbitrary external writers changing source/artifact/index files. The
parent records outside the requested set are not rebuilt by `compile(child)`;
callers use `compileAffected` to include reverse dependents. Persistent per-parent
dependency-artifact pinning and compatible-set installation remain separate gates.
The process termination fixture tests release of an idle writer lease, not a crash
in the middle of publication. DDS and the remaining Editor/release gates stay open.

## Increment 5: persistent dependency artifact pins (11 October 2026)

Authoring cooks now emit WPRS container version 2. Each install dependency carries
its immutable, cooked-root-relative artifact path and source/payload fingerprints.
These pins are stored inside the parent's container, so reopening the service or
evicting its cache does not make an older parent resolve a newly committed child.
A direct load of that child still selects its latest committed variant. Recooking
the parent advances its complete pinned closure.

Cook identity includes the container format and install-artifact paths. Old
version-one outputs therefore recook into distinct immutable generations without
overwriting existing packages. Readers continue accepting version-one containers
and legacy manifests; authoring loads reject unpinned version-one parents with
install dependencies and request a recook. Existing callers of the container
writer default to version one unless they explicitly provide version-two pins.
The compilation database remains schema 3 and runtime manifests remain payload
version 2: their existing per-resource paths and fingerprints constrain all parent
pins during runtime loading and manifest export. Rebuild linked consumers for the
extended compiled-header structure.

Runtime caches identify exact artifact generations. Separate load requests can
retain both old and current instances of a logical child. Within one closure,
conflicting generations for the same logical ID fail explicitly; missing pinned
files never fall back to the latest index entry. Failed closures publish no
partial cache entries. Existing closure count, depth and payload limits still
apply, including cached nodes and shared subtrees. Authoring pins must belong to
the selected target/mode namespace; runtime pins must agree with manifest paths
and child fingerprints. Container readers/writers reject malformed paths,
incomplete pins, duplicate install IDs, unsupported versions and truncated pins.

Coverage adds persistent old-parent loading after a child-only rebuild and service
restart; simultaneous old/current child caching; parent recook advancement;
missing-pinned-child recovery; incompatible diamond rejection and retry; pin path
and fingerprint mismatch against a manifest; malformed/truncated pin records;
and genuine version-one container/package compatibility.

The rebuilt Windows `RelWithDebInfo` contracts passed in targeted CTest runs:

| Suite | Time |
|---|---|
| WPResourceTests | 1.47 s |
| WPResource.writer_lease | 3.33 s |
| WPResource.index_migration | 0.49 s |
| WorkphoneAssets.catalog_contracts | 3.29 s |
| WPGraphics.cooked_material_resource | 1.62 s |
| WPLua.runtime_contracts | 0.75 s |

The new legacy authoring fixture initially failed because its manually specified
path omitted target-key encoding. It now derives its path from a committed
artifact; the rebuilt resource suite passed. The first two Python-backed suites
passed before that fixture-only correction and were not unnecessarily repeated.
The resource, catalog, cooked graphics and Lua test targets and `Editor` rebuilt
successfully, including required dependencies. `git diff --check` passed.
Logs: `project_x64/resource-pinning-build.log` and
`project_x64/resource-pinning-fixture-build.log`.

This closes the per-parent pinning gap recorded under increment 4. It does not
automatically rebuild reverse dependents after `compile(child)`, certify typed
consumer swaps on owner threads, or implement immutable source snapshots, strong
digests, generation recovery/GC, full profile/toolchain BuildKeys, DDS cooking or
the remaining Editor workflows. Container metadata is not cryptographically
authenticated. Interactive Editor, shipping application, hardware/performance
and interrupted-publication acceptance remain separate release gates.

## Increment 6: captured build inputs and optional-input invalidation (11 October 2026)

Each `compile` or `compileAffected` request now owns an isolated input tree under
`.staging`. A logical input is copied once and shared throughout that request.
Dependency scanners receive a captured descriptor. Compilation receives captured
`sourcePath` and `sourceRoot` paths, so reads cannot observe transient edits to
the original descriptor or raw dependencies. Copies stream with a combined
1 GiB byte limit and 65,536-input limit, verify their source fingerprint before
acceptance, and become read-only. A final whole-request check compares both the
captured files and current logical source paths/bytes before the complete index
transaction and session evidence publication. Failure retains the prior index
and installed objects; unused immutable artifact candidates may remain.

Scanners that read auxiliary input bytes use the new `CompileContext.snapshotInput`
hook before reading them. The hook automatically contributes raw dependency edges
and fingerprints. Optional absent inputs contribute an explicit absent marker and
an edge, so later creation, deletion and empty-file transitions invalidate reverse
dependents. During scanning, `sourceRoot` still identifies the authoring tree for
path validation; auxiliary byte reads must use the capture hook. The callback is
cleared before compilation. Standalone tools without this callback retain their
previous compiler behavior. Lua's optional `.lua.deps` scanner now uses the hook.
Rebuild linked compiler consumers for the extended context/dependency structures.

Build identity includes a capture-policy marker so existing pre-snapshot outputs
recook into new immutable generations. Temporary snapshot paths do not participate
in identity. WPRS version 2 and compilation schema 3 remain unchanged. Private
snapshot directories are cleaned on normal success, failure and up-to-date paths;
process-crash leftovers await the planned generation recovery/retention service.
Existing indexes gain absent-input edges when their roots are recooked; perform a
full recook before relying on optional-file creation events after upgrading.

Regression coverage includes restored descriptor/raw edits during compilation,
persistent raw edits retaining the previous record, optional input appearance
during a build, reverse invalidation after creation/deletion, empty versus absent
identity, shared input trees across multi-root requests, changes to an already
compiled root before batch publication, cleanup after failure and recovery, and
authoring files remaining untouched by a compiler write to its captured descriptor.
The real Lua contracts additionally cover optional sidecar creation/removal and
incremental compilation afterward.

The rebuilt Windows `RelWithDebInfo` contracts passed in targeted CTest runs:

| Suite | Time |
|---|---|
| WPResourceTests | 2.27 s |
| WPResource.writer_lease | 3.50 s |
| WPResource.index_migration | 1.05 s |
| WorkphoneAssets.catalog_contracts | 3.78 s |
| WPGraphics.cooked_material_resource | 2.35 s |
| WPLua.runtime_contracts | 1.05 s |

The four native test targets and `Editor` rebuilt successfully, including required
dependencies. `git diff --check` passed. No interactive Editor, standalone shipping
application, POSIX build or hardware/performance acceptance was performed.
Build log: `project_x64/resource-input-snapshot-build.log`.

Compilers remain trusted code; this is not a process sandbox. File capture is lazy,
not an atomic filesystem snapshot. External writes after the final validation
check still require the planned authored revision/publication coordination.
Cryptographic digests, interrupted-publication recovery, input-cache retention,
configurable budgets and representative performance certification remain open.
DDS profiles/cooking, the remaining asset database/Editor workflows, typed consumer
swaps and shipping-release acceptance remain separate production gates.

## Next implementation gates

User-requested scope extension: production-plan packages **RES-050–054** add
platform texture profiles, Windows DDS cooking, native compressed uploads,
Editor target previews and package certification. These are planned, not yet
implemented. Keep their identity/settings/BuildKey decisions integrated with the
existing texture compiler and the next cooking consistency milestone.

1. Finish the first integrity slice: authored numeric-ID lookup/traversal, immutable
   metadata migration, reference-aware operations, retention and recovery UI.
2. Extend target/mode indexing to full profile/toolchain BuildKeys, authored
   revision fences and generation recovery/retention on top of captured inputs,
   complete-request index transactions and immutable artifacts. Certify
   interrupted publication.
3. Persist typed dependency/reference edges and implement reverse queries,
   dependency invalidation and required-missing/cycle diagnostics.
4. Integrate typed runtime consumers and owner-thread publication throughout the
   Editor and shipping sample; complete asynchronous session-safe jobs.
5. Implement the asset database Editor, inspectors, preview/pickers, packaging
   controls and the declared format-family matrix.
6. Execute the plan's representative Editor, source-free package, failure,
   concurrency, performance and release gates. Passing focused tests alone does
   not establish production readiness.
