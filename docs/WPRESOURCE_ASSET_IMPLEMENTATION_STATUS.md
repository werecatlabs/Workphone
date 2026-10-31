# Resource and asset implementation ledger

Scope: implementation started on 2026-10-10 against the production plan in
[WPRESOURCE_ASSET_PRODUCTION_PLAN.md](WPRESOURCE_ASSET_PRODUCTION_PLAN.md).
This ledger records the integrity and cooking increments. The full plan and
production release gates remain open.

Foliage integration on 11 October adds compact target/resource cache directory
keys to keep Windows artifact paths bounded for long logical IDs and generated
species. Full identities and generation hashes remain validated in each header.
The earlier reversible directory encoding remains accepted for existing indexes
and installed manifests; package/container formats are unchanged. The resource
smoke fixture covers long subresource IDs, case distinctions, alias separation,
and pre-compact cache paths in authoring reuse and installed runtime manifests.

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

## Implementation increment 7: bounded catalog browser and Editor inspection

11 October 2026. This increment repairs the authored catalog browser portion of
**RES-046 / RV-23**. Reference assignment, domain editing and the other release
gates remain open.

AssetDatabaseManager::queryEntries returns detached identity DTOs instead of
constructing resources/directors. It provides literal, case-sensitive path/UUID
substring search, exact type filtering, scene exclusion and UUID-ordered keyset
pagination. Page sizes are constrained to 1–256; filter text and continuation
identities are bounded and reject embedded NULs. All SQL values are bound.
Literal matching uses escaped GLOB patterns supported by the bundled SQLite;
asterisks, question marks and brackets in user input remain literal.
Continuations carry their original filters, catalog instance and generation;
changed filters, catalog switches, close/reopen and external committed writes
invalidate them. Failed queries return an explicit error with no partial rows.
Connection serialization and a persisted-revision check around materialization
detect writes through the shared connection or another SQLite connection.
Each returned row is validated against canonical path/kind rules.

AssetCatalogBrowser is the shared, UI-independent navigation/selection model.
It retains the catalog weakly, clears detached rows on a project mismatch and
validates displayed generation evidence before selection or previous-page
navigation (including a return to page one). Refresh resets page history.
The native contracts exercise the same model used by the Editor.

AssetDatabaseWindow now resolves the active project's catalog, or an explicitly
injected catalog, without opening a database from its getter. The active-service
path rejects a catalog whose captured root does not match the active project,
including the interval before project service rebinding. It displays
bounded pages, literal search and exact type controls, previous/next navigation,
catalog errors and read-only UUID/path/type/kind details. Opening the window
refreshes its project binding. Editing filters immediately invalidates the
displayed rows; **Search / Refresh** explicitly applies them so typing does not
issue a database scan per character. Selection revalidates the active catalog
and DTO before inspection. Callbacks use weak owners, are disconnected before
UI removal, and no worker job touches live controls.

The formerly disabled configured-model/component tabs and their unreachable
legacy SQL were removed from this catalog window. Those tables belong to the
separate domain database Editor, whose authored editing/clone/backup/undo work
remains **RES-045**. This window offers inspection; it does not edit catalog
identity directly.

Regression coverage uses the real SQLite backend and includes 239 mixed
identities traversed across 17-row pages without omissions or duplicates, literal
percent/underscore/apostrophe/SQL-looking filters, exact type and scene filtering,
invalid limits/NULs, foreign and stale cursors, external writes, close/reopen,
navigation and selection after a project switch, stale previous-page navigation,
invalid selection indices, explicit clear and release of a weakly held catalog.

The Windows RelWithDebInfo build passed for WPResourceTests, WPAssetCatalogTests,
WPGraphicsCookedResourceTests, LuaRuntimeTests and Editor, with the final source
and rebuilt dependencies. All six resource CTest suites passed:

| Suite | Time |
|---|---|
| WorkphoneAssets.catalog_contracts | 6.59 s |
| WPGraphics.cooked_material_resource | 1.76 s |
| WPLua.runtime_contracts | 0.80 s |
| WPResourceTests | 1.84 s |
| WPResource.writer_lease | 3.52 s |
| WPResource.index_migration | 0.47 s |

Total CTest wall time: 15.12 s. git diff --check passed.
Build logs: project_x64/resource-catalog-browser-build.log (initial build),
project_x64/resource-catalog-browser-rebuild.log (SQLite compatibility repair),
project_x64/resource-catalog-browser-validation-build.log (final complete target
set and latest Editor source).

Interactive acceptance still required: open two projects with overlapping asset
UUIDs, open **Asset Database**, search/filter/page/select, switch projects while
the window remains visible, and verify stale selection rejection and refreshed
details. Close/reopen the panel and project repeatedly. No interactive UI or
shipping/hardware acceptance is implied by the headless model tests.

Bounds apply to returned rows and input text, not database scan time. Literal
substring searches can scan the catalog; indexed search, asynchronous query
publication/cancellation and representative large-project latency budgets remain
open. At this historical increment, the resource reference picker still required its
separate worker/lifetime, type-checking and undo repairs; a repaired read-only browser does not close those
requirements. DDS cooking/uploads, packaging and production certification also
remain open.

## Increment 8: bundled media mounts, reference commands and domain read isolation (11 October 2026)

The Editor creates a separate derived media catalog when absent, mounts it using
an OS-enforced SQLite read-only connection, and exposes a project/media switch in
the catalog browser and reference picker. The explicit WorkphoneMediaCatalog
builder classifies bundled sources, assigns deterministic path-based UUIDv5
identities, records Editor-only versus Runtime-tree eligibility, and publishes
without overwriting unrelated project/domain databases. Rebuilds preserve stable
path identities. Scanning skips private storage, links/reparse points and enforces
entry/depth budgets. Database reads no longer require a project for absolute paths.
See [bundled media ownership and workflow](WPRESOURCE_BUILTIN_MEDIA.md).

ResourceDatabase retains the media mount separately from the writable project
catalog. UUID reads resolve against the owning catalog and root, reject ambiguous
cross-catalog UUIDs, and do not adopt media rows into the project. Project-relative
paths remain project-scoped. Material, texture, font, mesh, sound and script UUID
routes use their existing resource managers/types; metadata-only script resolution
is not evidence of execution. Unmounting media stops new media UUID resolution.

The bounded reference picker no longer queues raw window pointers on worker jobs.
It captures weak targets, verifies selection/session/type evidence, resolves only
the chosen runtime resource, and submits a real ModifyPropertyCmd group. Public
property family labels and concrete registered type names resolve consistently.
Edits preserve metadata and unrelated properties, mark supported owners dirty and
support undo/redo, with preflight of every target and best-effort rollback for
setter rejection. The command captures both the project session and selected
project/media mount. Project switches, close/reopen, unmounting, independent edits
and destroyed targets cannot apply stale edits. Opening a reference button no
longer writes the button value into a referenced asset; imported file selections
resolve to their resource before editing. Save remains explicit.

Legacy settings/state lookups and updates now bind keys and values. Updates hold
the connection guard and use savepoints inside caller-owned transactions. Missing
queries return defaults, ambiguous authored keys are retained for explicit repair,
and failed mutations roll back. Reference database attachment and resource-map
lookups bind values too. These compatibility methods still expose their legacy
void/default-return APIs; a complete structured-result repository remains open.

Domain asset reads use a separate read-only backend and never initialize catalog
schema in AssetDatabase.db. Root lookup uses an explicit UUID column where present
or the legacy text-ID layout. Numeric descendants retain their row IDs. All
attributes are read under one snapshot; duplicate UUIDs/attributes, missing rows,
cycles and byte/count/depth overflow fail without publishing partial children.
Numeric-only domain roots require an authored UUID mapping; this increment does
not invent identities or migrate that database. Missing databases are not created.

Validation used rebuilt Windows RelWithDebInfo Workphone, SQLite, catalog-test
and Editor targets. Initial execution exposed an absolute-path startup restriction
and a stale SQLite factory object compiled with the previous class size. Both were
corrected. Normal-heap debugger reproduction also identified a destroyed target
access through the legacy intrusive WeakPtr. Asset browsing, picker targets and
reference commands now use owner-thread ObjectObserver lifetime tokens invalidated
before destruction; these tokens do not retain the object or follow reused storage.
This is scoped protection, not a replacement of every legacy weak pointer or a
concurrent weak-to-strong promotion mechanism.

The media inventory contains 251 assets (7 fonts, 4 materials, 2 meshes, 9 sounds,
4 textures and 225 scripts), all Editor-only. SQLite integrity_check reports ok;
an explicit rebuild retained all 251 path-to-UUID identities. Reference checks
cover regenerated compatible type metadata, invalid type IDs, destroyed targets,
recycled object storage, grouped undo/redo and session expiry. Refreshing picker
rows cannot recapture targets from an earlier project session, and missing source
files are rejected before runtime resource construction.

The six focused checks were run directly: catalog contracts, cooked graphics
resources, Lua runtime contracts, resource contracts, writer leases and compilation
index migration. Catalog contracts passed five consecutive normal-heap runs.
Logs are in project_x64/resource-observer-pass-*.log and
project_x64/resource-final-*.log. The Editor build log is
project_x64/resource-observer-build.log. Concurrent CMake edits disabled catalog
and cooked-resource registration; these results are direct execution evidence,
not a successful root CTest run at Increment 8. Increment 9 restores test
registration and records the fresh automated regression results below. Interactive Editor save/reopen
and representative hardware acceptance have not been executed.

At the end of Increment 8, remaining boundaries included source-format
validation/cooking, package closure and
eligibility enforcement, UUID runtime maps, asynchronous indexed-scale browsing,
persisted reference graph updates, authored metadata migration, standalone domain
editing, DDS profiles/cooking/uploads, actual save/reopen and interactive/hardware
release acceptance remain open. Trusted property setters are not a transactional
sandbox; failed rollback is reported and can require manual repair. The broader
production plan is not certified by these focused contracts.

## Increment 9: Windows DDS profiles and restored resource release tests (11 October 2026)

The existing texture compiler now supports explicit versioned Windows DX11 DDS
profiles for BGRA8, BC1 and BC3 in linear/SRGB forms. Native DDS bytes live inside
immutable typed resource artifacts, with target/mode indexing and pinned material
closures. The bounded parser validates layout, pitch and full mip payloads before
native installation; authored DX10 DDS passthrough validates the exact profile.
The deterministic first-party encoder needs no external tool. Graphics compiler
versions are bumped to 2, requiring explicit recooking of previous artifacts.

Claw installs encoded subresources directly after device capability checks,
retains last-good views on failure and retains DDS bytes for source-free reupload.
The existing Editor property panel exposes read-only native texture profile,
container, format, mip count and byte estimates. Catalog and cooked-resource test
registration is restored; WP_BUILD_TESTS enables root CTest independently of
WP_EDITOR_TESTS. The rebuilt Editor and all six focused CTest suites passed
(17.05 seconds total). All six DDS GPU formats passed exact mip readback; linear
BGRA8/BC1/BC3 passed visible production draw checks. Manifest loading succeeded
with original images absent and compiler services unset. The device was Microsoft
Basic Render Driver, driver 10.0.19041.7548, feature level 11.0. Build evidence is
in project_x64/resource-dds-final-build.log and
project_x64/resource-dds-validation-build.log; regression evidence is in
project_x64/resource-dds-final-tests.log. Actual interactive Editor save/reopen
and representative hardware acceptance remain unverified.

See [DDS profile support and validation](WPRESOURCE_WINDOWS_DDS.md). RES-050 through RES-052
have a bounded 2D implementation; RES-053 has inspection data but still needs
profile authoring/preview, and RES-054 has focused contracts but still needs
representative hardware, quality/performance and relocated package acceptance.
This increment does not close the remaining catalog metadata, reference graph,
retention/recovery UI, domain editing or asynchronous scale work.

## Increment 10: Editor operation history and quarantine restore (11 October 2026)

The Asset Database window exposes paginated journal inspection and guarded undo
for committed move/copy/delete operations. Deleted assets can be restored from
quarantine with original UUIDs. The view shows operation state, root, source and
destination; malformed records fail without partial publication. Project-session
fences reject stale navigation and restore actions. Bundled media remains read-only.
Queries neither construct assets nor create journal schema. The window callback
also uses the destruction-safe owner-thread observer introduced in Increment 8.

Journal contracts cover empty history, bounded pages/continuations, invalid
limits/cursors, committed-record selection, session expiry and the existing
byte-exact undo/conflict/interruption behavior. Catalog contracts passed after
rebuilding WPAssetCatalogTests and Editor. Logs: project_x64/resource-recovery-build.log
and project_x64/resource-recovery-tests.log. See [recovery workflow and boundaries](WPRESOURCE_ASSET_RECOVERY.md).
The final combined six-suite CTest gate passed in 17.19 seconds;
project_x64/resource-production-final-tests.log records that run. git diff --check passed.
Interactive acceptance, retention/purge, reference-aware deletion and a conflict
resolution wizard remain open; compiled UI and service contracts do not prove
interactive save/reopen behavior.

## Increment 11: Durable saved references and guarded asset operations (11 October 2026)

A separately versioned graph persists typed project/media references with document
revision, property provenance and required/optional policy. Production scene saves
capture the serialized tree before file IO, publish pending records, validate the
installed bytes and complete the graph. Graph writes roll back atomically; failed
writes, external edits and expired/superseded completions cannot publish ready
records. Script reference properties now declare resource metadata explicitly.

Bounded indexed forward/reverse queries return detached edges and project
missing/type diagnostics without resource construction. Catalog/reference revision
triggers fence continuations. The Asset Database details view displays saved users
for project and bundled-media assets. Required surviving users block quarantine
with actionable source/property diagnostics. Folder-internal users are quarantined
together; restore reactivates their edges. Moves/undo relocate document graph
ownership. Copy undo cannot remove a newly required copy. Legacy removal/clear
routes also check required users; quarantined graph identities cannot be replaced
by a new save. Unsupported/incomplete graph schemas fail without migration.

Editor creation undo now uses the same guarded quarantine service. Redo restores
retained bytes and UUIDs, with project-session expiry; it does not generate another
asset. Project Clean no longer deletes authored .meta/.lighting/.unity files; it
performs catalog maintenance and refreshes the filesystem view. The headless
command path handles the absence of a job queue without dereferencing it.

Rebuilt WPAssetCatalogTests and Editor passed the extended graph/scene-save and
actual creation/clean command contracts. The six-suite catalog, cooked graphics,
Lua, resource, writer-lease and migration gate passed in 17.81 seconds. Logs:
project_x64/resource-reference-final-build.log and
project_x64/resource-reference-final-tests.log. Production empty-scene saves cover
JSON/XML/binary serialization, directory/extension handling, pending-write
prevention, graph-failure rollback and scratch exclusion. The tests also exercise
creation undo refusing a required user, byte/UUID-preserving redo, session expiry
and authored-file retention during Project Clean. git diff --check passed.

See [reference graph coverage and limits](WPRESOURCE_ASSET_REFERENCES.md).
The implementation remains partial for RES-009/046: full-project legacy indexing,
material/prefab/domain saves, unsaved changes, durable authored UUID ownership,
compile/install/platform dependency policy, cycles and affected rebuild remain
open. The save marker detects incomplete file writes; it is not atomic document
replacement or a power-loss recovery guarantee. Synchronous validation budgets
bound work but do not certify large-project UI latency. Interactive populated-scene
save/reopen and representative package/hardware acceptance remain unverified.

## Increment 12: Staged authoring saves and document reindex (11 October 2026)

The shared authored save service stages and flushes complete bytes through an
exclusive sibling temporary file before replacement. Project saves persist a
pending graph and fence it across installation/completion. Preparation failures
retain the prior file/graph; replacement failures retain prior complete bytes;
completion failures retain new complete bytes with the pending safety fence.
Owned staging files are removed on normal completion/failure. Media/private
storage writes are rejected, and cache scratch/exports stay outside the graph.
This does not certify machine/power-loss durability or retain previous-document
history. Process-crash orphan cleanup and publication replay remain open.

Scene saves use the shared service. Material direct/save/manager routes and actor/
resource prefab saves now adopt it. Previously empty prefab resource/save routes
now persist stored Properties. Empty material texture slots and their serialization
no longer require graphics startup. Texture UUID and path fallback use different
fields; their previous shared key overwrote the UUID with the path. The loader
retains compatibility with path-only legacy material texture fields. Unimplemented
USD scene serialization now fails explicitly rather than writing a placeholder.

JSON and XML preserve property type/read-only/attribute metadata. Versioned JSON
retains ordinary name/value fields alongside the flat legacy representation. The
Editor exposes selected-document reindex for typed JSON, versioned XML and portable
binary. Reindex verifies installed bytes, retains project/media ownership and
replaces the graph transactionally without rewriting the document. Ambiguous
legacy text requires an owning-serializer resave. Malformed, duplicate, unsupported,
private/outside-project and over-budget inputs fail without replacing the old graph.
This is individual document repair; project-wide discovery and legacy migration
remain open.

Current-source builds passed for WPAssetCatalogTests, Editor, WorkphoneMediaCatalog,
WPGraphicsCookedResourceTests, LuaRuntimeTests and WPResourceTests. The six-suite
catalog/cooked-graphics/Lua/resource/writer-lease/migration CTest gate passed in
19.38 seconds. Logs: project_x64/resource-document-final-build.log and
project_x64/resource-document-final-tests.log. git diff --check passed.

The expanded contracts exercise actual material direct/save/manager routes,
texture UUID/type persistence, prefab actor/resource/save routes, JSON/XML metadata
round trips, binary/XML/JSON reindexing, read-only/media/private/path containment,
byte-exact installation, locked destination preservation, pre/post-publication SQL
faults, pending repair without rewriting source bytes, future graph/XML schemas,
duplicate/malformed text and staging-file cleanup. Editor controls are compiled;
interactive populated-scene/material/prefab save/reopen and large-project latency
remain unverified. Full-project legacy migration, remaining domain authoring,
compile/install policy, recovery/retention and shipping adoption remain open.

## Increment 13: Project document discovery and transactional reindex (11 October 2026)

The asset catalog now exposes a detached project document inventory and a batch
reference rebuild. Discovery includes .fbscene/.fbscenebin/.fbscenexml, .mat/
.material, .prefab/.fbprefab and .resource, plus every active indexed document
regardless of extension. It distinguishes verifiable typed documents from legacy,
malformed, missing, quarantined-path, linked and unsupported documents. Private
.workphone and VCS roots, configured cache scratch and the mounted media root
are excluded. Existing indexed owners in excluded domains remain explicit blocked
records rather than being silently dropped. Links/reparse points are reported
without traversal. Read-only catalogs can inspect; only writable catalogs can apply.

Preview performs no graph/schema writes and retains no resource instances. Its
private snapshot binds apply to the catalog session/revision, project/resource
service binding, cache/media scope, media session/revision and document membership/
fingerprints. Editing public display rows cannot authorize extra writes. Apply
revalidates the inventory, replaces every verifiable graph inside one database
transaction, then rechecks the inventory before commit. Any validation/SQL failure
rolls back the whole batch and returns zero published graphs. Source files are never
rewritten. Blocked owners retain their previous graph/revision and remain visible;
a successful batch with blocked rows is explicitly incomplete coverage.

The Editor Asset Database adds Scan project document references and Reindex
verified project documents, with 100-row pages and per-document diagnostics.
Successful rebuild retains the report and disables reuse until another scan.
Project activation checks now also cover explicitly supplied window catalogs.
Legacy JSON/XML requires an owning-editor resave; automatic migration cannot
infer missing reference types safely and remains open.

Limits are 10,000 candidates, 100,000 visited filesystem entries, directory depth
64, 64 MiB per document and 256 MiB read budget per scan pass, with lower caller
limits supported. Exceeding a global limit yields no applicable partial preview.
Apply performs multiple bounded read/verification passes. It holds the catalog
write transaction synchronously; asynchronous jobs/cancellation and large-project
latency gates remain open. SQLite cannot exclude external filesystem writers:
this does not establish an atomic filesystem snapshot or close the documented
external-writer/durability release gates. Unknown unindexed format families,
unsaved edits, remaining serializers and compile/install dependency policy still
need explicit adoption; the scan is not a package-closure certificate.

Current-source WPAssetCatalogTests, Editor, WorkphoneMediaCatalog,
WPGraphicsCookedResourceTests, LuaRuntimeTests and WPResourceTests builds passed.
The six-suite catalog/cooked-graphics/Lua/resource/writer-lease/index-migration
CTest gate passed (6/6, 20.53 seconds). After adding the Windows junction fallback,
WPAssetCatalogTests rebuilt and its catalog suite passed again (11.13 seconds).
The fallback exercised actual reparse-point traversal rejection; the initial
symlink-only attempt lacked the required Windows privilege. No Editor interaction
or representative hardware/package certification was performed. git diff --check
passed. Logs: project_x64/resource-inventory-final-build.log,
project_x64/resource-inventory-final-tests.log,
project_x64/resource-inventory-junction-build.log and
project_x64/resource-inventory-junction-tests.log.

Expanded contracts cover side-effect-free preview, new/known-extension and existing
nonstandard-extension discovery, XML/binary parsing, global limits, whole-batch
rollback after a later SQL fault, blocked-owner graph retention, private snapshot
authority, unchanged source timestamps, missing targets, stale file bytes/membership,
project/cache/media changes, unknown graph schemas, read-only inspection, reopened
sessions and Windows junction rejection. Automatic legacy migration, asynchronous
scan jobs, remaining format families and complete project/package coverage remain
open alongside the release gates below.

## Increment 14: Pinned directory packages and runtime UUID maps (12 October 2026)

Runtime manifest payload version 3 adds a bounded, deterministic authored UUID map
to the existing exact install closure. Version 1/2 packages remain readable; the
original export overload still writes version 2. Explicit bindings normalize UUID
case, reject nil/malformed/duplicate identities, reject multiple UUIDs aliasing one
ResourceID and reject resources outside the exported closure. Invalid export retains
the last-good manifest; invalid input fails before runtime mount initialization.

The concrete ResourceSystem exposes runtime-only UUID resolution and loading.
Resolution/load share one operation fence across mount lifetime, use the existing
complete-closure limits/cache and retain caller-owned generations across shutdown.
No authoring catalog, source path guessing, compiler registry or SQLite is needed.
CatalogResourceAdapter can capture an explicitly validated UUID binding from a
current request. A stale request clears the result. The binding remains detached
evidence: callers must own catalog-session admission across collection/export.

writeRuntimePackage stages runtime.wprm and only its verified package-mode install
closure in a new sibling directory. Copied container identity, hashes, sizes,
dependencies and persistent artifact pins are validated before exclusive directory
publication. Existing destinations, including empty directories, and source/cooked
storage overlap fail. Normal failures remove the request-owned staging directory.
Original artifact/index/lease/private files are not copied wholesale. Export holds
the resource operation fence through copying and publication.

See [runtime package API, compatibility and boundaries](WPRESOURCE_RUNTIME_PACKAGES.md).
This adds the UUID/directory-export subset of RES-018/028/030. It does not close
per-asset shipping eligibility, full authored-reference coverage, PackageEditor/CLI
profiles, executable/plugin packaging or real shipping sample adoption. Independent
catalog writers remain outside the resource operation fence. Process-crash staging
recovery, quotas/cancellation, machine/power-loss durability, stronger authenticated
digests, asynchronous residency and multi-mount policy remain open. Windows native
validation is recorded below after the final source rebuild; Linux/macOS exclusive
rename branches are source-only in this increment.

Current-source Windows/MSVC v145 RelWithDebInfo builds passed for WPResourceTests,
WPAssetCatalogTests, WPGraphicsCookedResourceTests, LuaRuntimeTests and Editor, with
their required dependencies. The six-suite CTest gate passed **6/6 in 29.66 seconds**:

| Suite | Time |
|---|---|
| WorkphoneAssets.catalog_contracts | 14.87 s |
| WPGraphics.cooked_material_resource | 7.19 s |
| WPLua.runtime_contracts | 0.92 s |
| WPResourceTests | 2.35 s |
| WPResource.writer_lease | 3.52 s |
| WPResource.index_migration | 0.70 s |

The contracts cover deterministic UUID order/case normalization, invalid/aliased/
outside-closure identities, empty and malformed maps, old manifest compatibility,
concurrent UUID/path cache identity, aggregate closure limits, retained generations,
package-mode admission, exact file membership, occupied/overlapping destinations,
corrupt inputs, failed-export staging cleanup and last-good manifest preservation.
The relocated export loads by authored UUID with every exported file read-only and
original source/cooked directories removed. Real catalog contracts cover binding
capture, stale-request rejection and UUID-preserving rename beside an old package.

The first build overlapped later header edits and used stale PCH declarations; an
early retry also hit that build's PCH lock. The final builds ran sequentially after
the original process completed. The first rebuilt catalog run exposed unused
state-manager assertions in Material::unload during the existing headless material
save contracts. Removing those unused dependencies repaired cleanup; the final
six-suite run includes the existing material/prefab save and reference contracts.

Build logs: project_x64/resource-package-final-build.log and
project_x64/resource-package-validation-build.log. Regression log:
project_x64/resource-package-final-tests.log. git diff --check passed. No Debug,
interactive Editor acceptance, executable shipping-package launch, Linux/macOS
runtime run, representative hardware, process-kill or power-loss certification was
performed. The complete production plan and the remaining gates below stay open.

## Increment 15: Catalog-admitted package export and shipping policy (12 October 2026)

`CatalogResourceAdapter::writeRuntimePackage` now admits the complete package-mode
install closure against the current project catalog. Every member must have one
durable UUID, an explicit supported type mapping and runtime shipping eligibility.
Stale roots, uncataloged dependencies, ambiguous paths, unsupported types,
subresources and mismatched actual resource-service source roots reject export.
The previous destination and previously exported packages remain intact on failure.

`ResourceSystem::writeRuntimePackageChecked` obtains the actual pinned closure
before calling the resolver under its operation fence. It requires complete UUID
coverage and a retained admission token, then verifies that the rebound manifest
still describes the same artifacts and generations. Catalog admission acquires
locks in resource -> catalog -> SQLite connection order and retains a
`BEGIN IMMEDIATE` transaction through artifact copying and exclusive publication.
This excludes independent SQLite writers and other users of the same connection
through publication. A writable project catalog is required; read-only catalogs,
bundled-media closure integration and unmapped subresources fail explicitly.

Project shipping policy is persisted separately in version-1 package policy tables.
Unconfigured project files default to runtime eligibility without inspection
creating schema. Explicit runtime, Editor-only and excluded values are supported.
Setters require current file DTOs and a writable project session. Revision triggers
invalidate snapshots after policy writes, including independent bound SQL writes.
Unknown versions, incomplete layouts and missing revision triggers fail without
automatic repair. Bundled media inspection reads its inventory-owned usage and
rejects project setters.

The Asset Database window exposes guarded shipping-policy controls for project
files. Copy inherits explicit policy in the existing operation transaction; move
retains UUID policy. Quarantine/delete retains policy by UUID so undo/restore cannot
silently turn an Editor-only asset into runtime content. Policy/history retention
and purge remain open. Unsupported policy schema blocks copy before file staging.

The low-level explicit-binding ResourceSystem export remains available and does
not itself enforce catalog policy. Existing PackageEditor/CLI callers have not
been migrated to the checked adapter route. This increment therefore supplies the
eligibility/complete-mapping API subset of RES-028/030, not shipping certification.
It performs synchronous bounded closure validation; asynchronous jobs, scale,
multi-mount integration and interactive Editor acceptance remain open.

Merged-source validation exposed stale CMake source discovery: the present
`AssetImportQueue.cpp` was absent from the generated project. Workphone source
globs now use `CONFIGURE_DEPENDS`, and regeneration includes newly merged sources.
The empty, header-implemented `AssetReferenceChange.cpp` now includes the required
PCH. `AssetImportQueue` now exports its public API from Workphone.dll. The catalog
concurrency test wraps the factory-created real backend through public interfaces
instead of subclassing the unexported concrete SQLite implementation.

The rebuilt catalog/resource/writer-lease/index-migration suites passed **4/4 in
31.76 seconds**. The complete six-suite CTest gate passed **6/6 in 34.13 seconds**:

| Suite | Time |
|---|---|
| WorkphoneAssets.catalog_contracts | 19.99 s |
| WPGraphics.cooked_material_resource | 6.79 s |
| WPLua.runtime_contracts | 0.88 s |
| WPResourceTests | 2.54 s |
| WPResource.writer_lease | 3.34 s |
| WPResource.index_migration | 0.45 s |

New contracts cover policy persistence/defaults, stale sessions, independent SQL
revision changes, failed-write rollback, read-only/media ownership, future/missing
schema tracking, policy-preserving copy/move/quarantine/restore, complete transitive
UUID mapping, ineligible/uncataloged/unmapped dependency rejection, actual source
root binding, same-connection/independent writer exclusion through publication,
resolver failure/exception cleanup, admission lifetime and occupied destinations.

WPResourceTests, WPAssetCatalogTests, WPGraphicsCookedResourceTests and
LuaRuntimeTests were rebuilt with current Windows/MSVC v145 RelWithDebInfo sources.
The Editor controls also required mutable SmartPtr loop handles to call
`setEnabled`; the final current-source Editor build passed. Regression log:
project_x64/resource-admission-tests.log. Build log:
project_x64/resource-admission-validation-build.log (test targets and dependencies)
and project_x64/resource-admission-editor-build.log (successful final Editor build).
git diff --check passed. No interactive Editor run,
Debug build, process-kill/power-loss test, platform-port execution or real shipping
application certification was performed. The remaining production gates stay open.

## Increment 16: Checked resource-package CLI (18 October 2026)

`WorkphoneResourcePackage` connects the existing Lua and graphics compilers to
catalog-admitted directory export. Explicit catalog type mappings and root UUIDs
select inputs; package-mode compilation precedes complete-closure UUID and shipping
policy admission. Missing catalog paths are rejected without database creation.
Windows uses Unicode arguments. Existing output directories remain untouched.
See [command-line usage and limitations](WPRESOURCE_RUNTIME_PACKAGES.md).

The new Python/CTest integration fixture invokes the built executable against an
independent SQLite catalog under a Unicode source path. It covers exact directory
membership, occupied-output byte preservation, missing UUID/catalog rejection,
Lua syntax failure, unsupported policy schema, Editor-only/excluded transitive
dependencies, restored runtime eligibility and uncataloged dependencies.
Python SQLite connections are explicitly closed before Windows fixture cleanup.

The first broader catalog run exposed an unattached-technique null dereference in
`MaterialTechnique::addPass` during existing authored material save contracts.
The method now updates its parent material only when attached; assembling an
authored technique before material ownership is valid. The debugger identified
this production method before the guard was added.

This delivers a synchronous CLI subset of RES-028/030. Multi-root cooks commit
each successful root normally; a later root failure retains earlier derived cache
records but never publishes partial output. Saved profiles, PackageEditor adoption,
asynchronous cancellation, remaining typed families, executable/plugin packaging
and actual shipping application acceptance remain open. Opening a supported legacy
catalog may invoke its existing migration path.

Windows/MSVC v145 RelWithDebInfo builds passed for `WorkphoneResourcePackage`,
`WPAssetCatalogTests` and `WPResourceTests`, including required dependencies.
The final focused CTest gate passed **5/5 in 23.66 seconds**: catalog contracts
(14.75 s), resource contracts (2.50 s), writer lease (3.37 s), index migration
(0.47 s) and package CLI (2.54 s). The existing cooked-graphics and Lua test
executables also passed against the rebuilt DLLs (**2/2 in 12.24 seconds**);
those two test targets were not separately rebuilt in this increment.
`git diff --check` passed. Logs: `project_x64/resource-package-cli-final-build.log`,
`project_x64/resource-package-cli-final-tests.log` and
`project_x64/resource-package-cli-consumer-tests.log`. The initial failed catalog
run and debugger evidence remain in the corresponding `resource-package-cli-tests`
and `resource-package-cli-catalog-stack` logs. No Debug build, interactive Editor
walkthrough, independent shipping application launch or process-kill/power-loss
certification was performed.

## Increment 17: Saved CLI profiles and source-free verification (18 October 2026)

The package CLI now saves version-1 JSON requests with `--save-profile` and builds
them with `--profile`. Relative paths resolve from the profile directory, allowing
relocation independently of the working directory. Profiles select explicit target,
source/catalog/cooked/output paths, catalog type mappings and root UUIDs. They feed
the existing compilation and checked catalog exporter; no alternative policy or
runtime store was introduced. Profile saving performs no cooking or catalog open.
Exclusive hard-link publication preserves an existing profile and requires a
supporting filesystem. Unsupported versions, unknown/duplicate/missing fields,
malformed UTF-8/JSON, NUL/trailing bytes, duplicate roots/mappings and bounded
size/count violations fail before authoring services open.

`--verify` uses the existing runtime-only mount and loads requested UUID install
closures without an authoring catalog, compilation database or compiler registry.
The command still depends on the CLI's DLLs. It validates payload loading, not
Lua execution, GPU installation, audible output or every unrequested manifest item.

The executable integration fixture now saves/reloads profiles, verifies exclusive
save/no-cook behavior, relocates a profile/project tree and verifies policy rejection
through the profile path. It then removes the relocated sources/catalog/cooked
cache and successfully loads both root and dependency UUIDs from read-only package
files in another process. It checks unchanged bytes/membership, absent authoring
storage, wrong target, missing UUID and corrupted-payload rejection. Rejected
profiles leave output and cooked directories absent; completed saves leave no
profile staging file.

Windows/MSVC v145 RelWithDebInfo `WorkphoneResourcePackage` and dependencies built
successfully. The expanded `WorkphoneAssets.package_cli` CTest passed; results are
in `project_x64/resource-package-profile-tests.log`, with build output in
`project_x64/resource-package-profile-build.log`. `git diff --check` passed.
Only this targeted integration suite was needed for these tool-only changes;
the earlier engine/consumer gate is recorded in increment 16. No Debug build,
interactive Editor acceptance or shipping consumer/application certification is
claimed. PackageEditor and shared native/UI request integration, asynchronous jobs,
full typed-family adoption and the remaining production gates stay open.

## Increment 18: PackageEditor integration and owned asynchronous builds (18 October 2026)

The versioned profile reader/writer now lives in Workphone as exported shared
functions. The CLI and native PackageManager use the same format rather than
maintaining separate profile stores. PackageEditor selects a saved profile and
offers Build, Cancel, status/diagnostics and the request log path. Unsupported
build-type/platform/compression/version setter calls were removed. The existing
Editor application update path polls the window while visible; workers never
hold or call Lua UI objects. Full in-window profile authoring remains open.

`IPackageManager` now exposes the resource-profile/job contract. `createPackage`
delegates to this route instead of the legacy filename-filtered archive builder.
Legacy texture/platform flag accessors remain for compatibility but do not alter
saved resource profiles. This public virtual-interface change requires matching
rebuilt native consumers and bindings.

Admission requires a running project, queue and worker pool. It reads the bounded
profile, verifies the actual source root against the active project, and captures
resolved request arguments before queueing. Overlapping builds and silently
rejected queue submissions fail explicitly. `ResourcePackageJob` rejects primary
thread execution, snapshots only its captured request, starts the deployed CLI
without a shell and waits only for that child. Restricted inherited handles carry
stdout/stderr and a cooperative cancellation event. A Windows Job Object owns
child lifetime. Result state and logs are detached from application/UI lifetime.

Project path changes and manager/window unload request cancellation. An unstarted
job stopped by queue shutdown finalizes as cancelled and cannot run later.
Cancellation checks run before authoring setup, between root compilations and
before export. In-progress compilation/export may finish; a committed output
remains success. Normal completion removes the immutable request snapshot and
retains its unique log. Process-crash orphan cleanup, retention quotas and
interruption inside a compiler/export remain open.

The Editor target depends on the package executable. Editor installation includes
that executable, its runtime dependency closure and the RapidJSON license used by
the shared profile parser. `BuildEditorPackage.ps1` now runs the real checked CLI
profile fixture from the relocated read-only installation with a system-only child
PATH, in addition to the existing Editor startup/negative-input checks.

New validation lanes are `WorkphoneAssets.package_editor_workflow` (production Lua
window with strict UI doubles) and `WorkphoneAssets.package_jobs` (native manager,
real Lua bindings, worker execution and actual child process). Native contracts
cover worker/queue admission, immutable queued requests, overlap, cancel/stop,
nonzero exits, retry, project switching, unload and snapshot/log lifecycle. The
job fixture also relocates a resulting package, removes authoring sources/catalog/
cooked data, makes package files read-only and runs an actual LuaManager consumer
with filesystem module search disabled. This is typed Lua execution/require
evidence, not scene/mesh/animation/audio/UI-family certification.

Current-source Windows/MSVC v145 RelWithDebInfo builds passed for the package tool,
native job/consumer host, catalog/resource/cooked-graphics/Lua test targets and
Editor, including affected dependencies. A final follow-up rebuild included the
queue-stop handling and corrected external-output native fixture. The nine-suite
CTest gate passed **9/9 in 41.27 seconds**: catalog (14.83 s), cooked graphics
(11.11 s), Lua runtime (0.81 s), resource (2.60 s), writer lease (3.40 s), index
migration (0.45 s), PackageEditor Lua workflow (0.01 s), native package jobs and
Lua consumer (1.39 s), and package CLI/profiles (6.41 s). `git diff --check` passed.
Logs: `project_x64/resource-package-editor-final-build.log`,
`project_x64/resource-package-editor-validation-build.log` and
`project_x64/resource-package-editor-final-tests.log`. The first native job run
correctly rejected a test output inside its source root; the fixture now exports
outside the project, retaining that overlap guard. The first build's local
`Handle` helper collided with the engine's type and was renamed before the final
passing rebuilds.

The first installed-package pass returned exit 0 despite a PackageEditor Lua
overload traceback: native `setEnabled` requires the child-propagation argument.
The window now passes both booleans, its strict UI double enforces that arity,
and installed-package validation rejects Lua tracebacks/overload errors even on
exit 0. The earlier `resource-editor-validation-20261018-c146ecad` candidate is
explicitly rejected/superseded by its `resource-package-review.json` annotation.
The final nine-suite gate above includes the corrected window and stricter test.

Final installed-package validation passed in
`project_graphics_sdk/resource-editor-validation-20261018-7af6c738/`.
`editor-package-evidence.json` records `validationPassed=true` and
`releaseCertified=false`. The candidate includes the runtime archive, separate
symbols archive, payload/module SHA-256 inventories and per-check logs. Checks
passed for relocated resource CLI/profile contracts, read-only Editor startup,
cross-volume startup, rejection of Debug harness packaging, and expected failures
for missing media/plugin inputs. Loaded modules were confined to the installation
and Windows, and the installed payload remained byte-for-byte unchanged. Final
driver log: `project_x64/resource-editor-install-final-validation.log`.

No Debug test build, interactive artist walkthrough, clean-machine validation,
completed license audit, full-family game launch, representative hardware capture,
process-kill/power-loss certification or scale/soak/performance acceptance was
performed. The installed candidate and typed Lua test do not substitute for those
remaining mandatory gates.
Full R1/R2 shipping certification remains open until the declared family matrix,
interactive workflows, recovery/soak/performance and representative hardware gates
in the production plan are satisfied.

## Increment 19: Supported profile authoring inside PackageEditor (18 October 2026)

The window now loads and edits catalog/cooked/output paths, target, root UUIDs and
explicit type mappings. The active source root is read-only. Save new profile
delegates to the shared native writer and preserves existing files; Load restores
the selected file. Unsaved edits disable Build, reopening restores a selected
saved profile, and project changes reset the draft. The default profile filename
is now `package.json` in the active project; older profiles remain loadable by path.
Unsupported compression/platform/version features are not exposed as controls.

The Lua bindings translate detached field tables to the existing profile request.
They validate active project ownership, string/list limits, exact field sets,
dense arrays, valid non-nil UUIDs and duplicate identities/mappings, then use the
same version-1 reader/writer as the CLI. Failures return structured errors without
opening a catalog or cooking. Actual asset existence, complete closure mapping and
shipping eligibility remain checked at build/export time. No new catalog/store or
alternate package format was introduced.

The production Lua window test covers edit/save/load, preserved existing profiles,
dirty-build prevention, reopened settings, project switching and asynchronous
controls. The native binding/job test authors and reopens a profile through real
LuaManager bindings, changes target/output/cooked settings, builds that profile,
then mounts its actual manifest using the edited target and loads its root UUID.
It also rejects duplicate roots/mappings, sparse arrays, unknown fields and a
different source project without creating the rejected files.

Windows/MSVC v145 RelWithDebInfo builds passed for the native host, package tool,
Editor and affected Lua libraries; LuaRuntimeTests was relinked against the updated
bindings. The full nine-suite resource gate passed **9/9 in 42.84 seconds**.
Logs: `project_x64/resource-package-profile-editor-final-build.log`,
`project_x64/resource-package-profile-editor-lua-build.log` and
`project_x64/resource-package-profile-editor-final-tests.log`.

Build recovery exposed a full G: drive after generating release symbol copies.
The previous WPLua PDB was preserved. Redundant task-owned extracted symbol trees
were moved intact to the C: task workspace; the original ZIPs/evidence remain on
G:. No source/user assets were removed. The first cross-volume validation attempt
under a long G: scratch path failed artifact copying; the shorter task scratch
root rerun is recorded below. General long-path certification remains open.

Installed-package validation passed with `validationPassed=true` and
`releaseCertified=false`. Evidence, runtime/symbol ZIPs and per-check logs are in
`C:/Users/casper_music/.codex/visualizations/2026/10/18/01a14ff8-ad4b-7281-81a5-8eaffeefa475/resource-profile-editor-validation-20261018-24602bad/`.
The task scratch root was `G:/Workphone/project_x64/tmp/24602bad` to bound native
artifact paths and retain a real cross-volume installation/workspace check.
Relocated CLI/profile contracts, read-only Editor startup, cross-volume startup,
Debug harness rejection and missing-media/plugin failures passed. Successful
startup logs contain no Lua overload errors or tracebacks. The installed payload
remained unchanged and loaded modules were confined to the installation/Windows.
Final driver log: `project_x64/resource-profile-editor-install-final-validation.log`.
`git diff --check` passed.
Interactive layout/artist acceptance, a full asset-family shipping sample,
hardware, recovery/power-loss and scale/soak certification remain open.

## Increment 20: Bounded PackageEditor catalog root picker (19 October 2026)

PackageEditor searches the active project's existing catalog with literal
path/UUID text and exact optional type filters. Up to 25 file rows are displayed;
larger result sets require refining the filters. Selection rechecks detached
path/type/revision/catalog-instance identity against the live catalog, rejects
project transitions and closed/reopened catalog sessions, and adds the UUID to
the draft only when its type has an explicit mapping. Duplicate roots and the
512-root limit are enforced. Catalog identities cross Lua as decimal strings to
preserve all 64 bits. Queries construct no assets and keep no second catalog
cache. Searches and selections are disabled while a package job is active.

The picker browses the active project catalog, not an arbitrary draft catalog
path. The existing checked build validates the saved profile against its selected
catalog and shipping policy. Source existence/compiler availability and full
install-closure admission are build checks, not picker guarantees. Closure
preview, richer root-list editing, asynchronous catalog scale gates and interactive
layout/artist acceptance remain open.

Native Lua contracts cover literal/type filtering, bounded results, malformed
filters, stale revisions, reopened catalogs and project isolation. Lua workflow
contracts cover mapping/duplicate guards, dirty drafts, filter invalidation,
project reset and queued UI events during builds. The Editor, package tool and
native fixture built successfully. All nine focused resource/catalog/graphics/
Lua/package suites passed in 41.18 seconds; see
`project_x64/resource-package-root-picker-final-tests.log`.
`git diff --check` passed.

Relocated package validation passed with `validationPassed=true` and
`releaseCertified=false`. Evidence and runtime/symbol ZIPs are in
`C:/Users/casper_music/.codex/visualizations/2026/10/18/01a14ff8-ad4b-7281-81a5-8eaffeefa475/resource-root-picker-validation-20261019/`;
the driver log is `project_x64/resource-package-root-picker-install.log`.
Relocated CLI/profile checks, read-only startup, cross-volume startup, Debug
harness rejection and missing-media/plugin failures passed. Interactive picker
acceptance, clean-machine validation, license audit and full-family shipping
certification remain open.

## Increment 21: Asynchronous checked install-closure previews (19 October 2026)

PackageEditor can preview a saved profile through the owned Windows package job.
The child compiles roots in package mode, reuses export's pinned manifest collector
and catalog/SQLite admission resolver, then returns UUID/ResourceID identities,
selected-root markers, payload sizes and direct install dependency edges. Missing,
unmapped, Editor-only or excluded members reject the entire request. This inspects
the cooked install closure rather than estimating it from authored references.

`CatalogResourceAdapter::previewRuntimePackage` delegates to
`ResourceSystem::previewRuntimePackageChecked`; the exporter and preview share the
same admission and closure validation path. Preview stops before artifact copying
and directory publication. Its temporary manifest is cleaned from cooked storage
on normal return. The profile's output directory is untouched, including when it
already exists. The CLI exposes
`--preview-profile <saved-profile> <new-report.json>` and publishes a complete JSON
report exclusively. Preview can change catalog/cooked storage and is labelled as
such in the UI. It is not a side-effect-free plan or a guarantee that a later build
will admit the same generation.

The Editor captures settings before queueing, excludes overlapping builds/previews,
uses the existing cooperative cancellation and project/unload lifecycle, and
displays results on its update thread. Reports are retained beside logs; quotas
and process-crash cleanup remain open. The UI reads at most 8 MiB once per completed
preview, shows up to 100 resources and three inclusion parents per member, and
displays the full report's path. Preview is limited to 4096 closure members. Draft edits,
root additions, profile load/save and project changes clear the displayed snapshot.
Payload byte totals are not installed-size or residency estimates. Subsequent
builds repeat validation.

The Editor, package tool, native job host and affected resource/catalog/Lua/graphics
test hosts built successfully; see
`project_x64/resource-package-preview-final-build.log`. All nine focused suites
passed in 43.85 seconds (`project_x64/resource-package-preview-final-tests.log`).
Coverage includes preview/report determinism, exact install membership and reasons,
occupied output, exclusive report publication, cooperative preview cancellation,
project isolation, malformed/missing reports, ineligible/uncataloged dependencies,
stale-result suppression, and the source-free packaged Lua consumer.

Final relocated installation validation passed with `validationPassed=true` and
`releaseCertified=false`. Evidence, runtime/symbol ZIPs and per-check logs are in
`C:/Users/casper_music/.codex/visualizations/2026/10/18/01a14ff8-ad4b-7281-81a5-8eaffeefa475/resource-preview-final-validation-20261019/`;
the driver log is `project_x64/resource-package-preview-final-install.log`.
Relocated CLI/profile/preview contracts, read-only Editor startup, cross-volume
startup, Debug harness rejection and missing-media/plugin failures passed.
The payload remained unchanged; loaded modules were confined to installation and
Windows paths. This final run supersedes the earlier preview candidate, which
preceded the final procedural DLL rebuild. `git diff --check` passed.
Interactive layout/artist acceptance, full-family shipping samples,
clean-machine/license audit, hardware and recovery/soak gates remain open.

## Increment 22: Catalog and closure preview paging (19 October 2026)

PackageEditor now navigates the catalog in bounded 25-row pages instead of
requiring more restrictive filters for every result beyond the first page.
The additive Lua page query reuses the existing catalog cursor, binding it to
exact filters, revision and catalog instance. Decimal identity strings preserve
all 64 bits; malformed, overflowing, stale and mismatched cursor values fail
closed. The UI keeps cursor history for Previous/Next, clears it on filter or
project changes, and clears old rows/navigation when a query fails. Selection
still rechecks current detached identity before adding a root UUID.

Closure previews now show 25 resources per page with row ranges and Previous/Next
controls through the existing 4096-member limit. The native reader validates the
full bounded report on each explicit page request and returns only that page;
ordinary UI updates do not reread it. Negative/oversized/out-of-range requests
fail. Changed report paths, read failures, profile edits and project switches
clear navigation. Inclusion reasons and payload totals remain tied to the same
successful preview request. The legacy first-100-row read helper remains available.

The package tool, Editor, native job host and Lua host built successfully
(`project_x64/resource-package-paging-build.log`). All nine focused suites passed
in 55.03 seconds (`project_x64/resource-package-paging-tests.log`). The native
fixture builds a real 127-resource closure, checks all six pages for exact unique
membership, validates inclusion reasons and the partial last page, rejects
invalid page indices, and executes the exported package after deleting all Lua
sources and the catalog. A 30-row catalog fixture checks continuation membership,
filter mismatch, overflowing identities and catalog reopen rejection. Lua workflow
tests cover bidirectional navigation, reset/invalidations and report replacement.

Installed Editor validation passed with `validationPassed=true` and
`releaseCertified=false`. Evidence, runtime/symbol ZIPs and per-check logs are in
`C:/Users/casper_music/.codex/visualizations/2026/10/18/01a14ff8-ad4b-7281-81a5-8eaffeefa475/resource-paging-validation-20261019/`;
driver log: `project_x64/resource-package-paging-install.log`.
Relocated CLI/profile/preview contracts, read-only startup, cross-volume startup,
Debug harness rejection and missing-media/plugin failures passed. Startup logs
contain no Lua binding/overload tracebacks; installed payload bytes remained
unchanged and loaded modules were confined to installation/Windows paths.
`git diff --check` passed. Interactive acceptance, asynchronous catalog scale work,
report caching/retention and broader shipping certification remain open.

## Increment 23: Interrupted package request recovery (19 October 2026)

Windows package jobs now retain an exclusive, non-inherited delete-on-close lock
for the lifetime of each captured request. Another recovery pass skips unavailable
locks; normal completion cleans its profile before releasing the lock. Owner
death releases the OS handle, while the existing kill-on-close Job Object owns
the child lifetime. Cleanup now removes a snapshot only when this worker created
it successfully, preserving pre-existing files after a failed capture.

Before starting a new build/preview, the worker scans at most 4096 entries in the
project's owned package-job directory. Each new request exclusively writes and
flushes a versioned `.owner` marker before capturing its profile; recovery requires
an exact bounded marker match. Unmarked legacy requests and unknown/partial
markers are left in place, avoiding interference with older live producers.
Valid UUID-named captured profiles with ownership markers are
checked with the shared bounded profile parser and exact project-root binding,
then archived exclusively as `<uuid>.interrupted.json`. Recovery retains settings
and reports counts in the completion diagnostic. Invalid/foreign snapshots,
non-regular files, unavailable locks and archive conflicts are preserved with
diagnostics. Reparse/redirected storage is rejected. The standalone CLI
`--recover-jobs <project-root>` performs the same operation without opening a
catalog or cooking assets. Repeated recovery ignores already archived records.

Recovery never automatically retries, removes published outputs or reclaims
package/cooked staging trees. An interrupted request's publication outcome is
unknown; inspect the output before explicitly choosing a fresh destination.
Archived records retain the ownership marker as history; successful normal
cleanup removes only the worker's own marker.
Retention quotas, orphan artifact cleanup, compiler/export kill windows and
power-loss durability remain open.

The native process fixture deliberately blocks a child inside the production
job's ownership path, kills the owner, checks child exit and profile preservation,
recovers the record, and runs a fresh real export through a restarted worker.
It also checks automatic recovery counts, active-lock skipping, marker retention,
idempotent recovery, unowned-file protection and source-free package execution.
CLI fixtures cover legacy/unknown marker preservation, malformed/foreign profiles,
archive conflicts, unchanged logs/reports/catalog bytes and redirected storage
rejection when symlink creation is supported. This scope does not certify
interruption inside a compiler or publication.

The complete `WorkphoneEditorPackageBinaries` runtime set, tool, native host and
Lua host built successfully; see
`project_x64/resource-package-recovery-marked-build.log`. All nine focused suites
passed in 62.29 seconds (`project_x64/resource-package-recovery-final-tests.log`).
Installed validation passed with `validationPassed=true`, `releaseCertified=false`:
`C:/Users/casper_music/.codex/visualizations/2026/10/18/01a14ff8-ad4b-7281-81a5-8eaffeefa475/resource-recovery-final-validation-20261019/`.
The driver log is `project_x64/resource-package-recovery-final-install.log`.
Relocated CLI/profile/preview/recovery contracts, read-only Editor startup,
cross-volume startup, Debug harness rejection and missing-media/plugin failures
passed. Payload bytes remained unchanged, loaded modules stayed within installed/
Windows paths, and final installed runtime hashes match the built modules.
The initial `resource-recovery-validation-20261019` candidate is superseded and
marked with `recovery-review.json`: it preceded the durable ownership guard and
must not be used for legacy request recovery. `git diff --check` passed.
Interactive acceptance, clean-machine/license audit, full-family shipping,
compiler/export kill windows, artifact reclamation, retention and power-loss
certification remain open.

## Increment 24: Bounded package-child supervision (19 October 2026)

Windows package jobs now supervise child execution with a monotonic deadline and
cooperative cancellation grace period. Defaults are 30 minutes and 30 seconds.
The Editor captures validated `WP_RESOURCE_PACKAGE_MAX_RUN_MS` (1000–86400000)
and `WP_RESOURCE_PACKAGE_CANCEL_GRACE_MS` (1000–600000) overrides before queueing;
zero explicitly disables the corresponding limit. Invalid settings fail without
queueing. Captured fields are detached from later environment/UI changes.

Cancellation first signals the existing inherited event. Child exit 0 still wins
over a pending deadline; cooperative exit 3 distinguishes user cancellation from
a timeout. If the child does not exit within grace, its owned Job Object is
terminated and exit confirmation is bounded to five seconds. Forced stops,
unexpected exit codes and uncertain wait/status failures report `interrupted`
instead of asserting that publication failed or never occurred. Captured settings
and ownership evidence remain available for the existing recovery route. Outputs
are never deleted to make a retry possible. Watchdog timing starts at child
resume; prelaunch filesystem/recovery work is outside this deadline.

The complete Editor runtime set, package tool, native host and Lua host built
successfully (`project_x64/resource-package-watchdog-build.log`). All nine focused
suites passed in 63.76 seconds
(`project_x64/resource-package-watchdog-final-tests.log`). The native child tests
deadline escalation, user cancellation, cooperative timeout, success after a
cancellation signal, unexpected exit, child termination before job completion,
retained-record recovery and invalid environment settings before queueing.
Existing real CLI export, 127-member closure paging, source-free Lua consumption
and owner-kill recovery also passed. The test PID marker is reset between the
watchdog and owner-kill phases to prevent stale-PID fixture interference.
These establish supervisor behavior, not a crash inside a real compiler or the
publication window.

Installed-package validation passed with `validationPassed=true` and
`releaseCertified=false`. Evidence, runtime/symbol ZIPs and per-check logs are in
`C:/Users/casper_music/.codex/visualizations/2026/10/18/01a14ff8-ad4b-7281-81a5-8eaffeefa475/resource-watchdog-validation-20261019/`;
driver log: `project_x64/resource-package-watchdog-install.log`.
Relocated CLI/profile/preview/recovery contracts, read-only Editor startup,
cross-volume startup, Debug harness rejection and missing-media/plugin failures
passed. The installed payload remained unchanged and loaded modules were confined
to installation/Windows paths. Final installed module hashes match the built
runtime set. `git diff --check` passed. Interactive acceptance, retention,
artifact reclamation, real compiler/export interruption, clean-machine/license
audit, power-loss and full shipping certification remain open.

## Increment 25: Owned job history and live diagnostic budgets (30 October 2026)

Known completed Windows package jobs now publish exclusive, project-bound receipts
with exact log/preview names, completion status/time and file size/fingerprints.
Interrupted outcomes retain their captured requests and never become eligible
history. Automatic retention reserves the current result and defaults to 64
requests / 64 MiB. Captured environment overrides and dry-run/apply CLI commands
are documented in `WPRESOURCE_RUNTIME_PACKAGES.md`. Legacy/unmanaged, changed,
foreign, redirected, locked and uncertain records are preserved with warnings.
Receipt-bound durable prune intents permit restart after partial deletion; cleanup
never recursively deletes a directory or touches authored/package output files.
The scan is bounded to 4096 entries and reports incomplete accounting; this is a
managed-history policy rather than a guarantee about total directory disk use.

Child stdout/stderr now passes through an owned pipe. The parent caps diagnostic
files at a captured 32 MiB default, retains a truncation footer within the cap,
requests cooperative cancellation on excess and uses the existing grace/watchdog
for an unresponsive writer. Draining has bounded work between supervision checks.
Successful publication still wins over a concurrent limit; forced termination
continues to report an unknown outcome. Jobs stay busy through receipt/prune work,
so another request cannot race the previous completion's ownership cleanup.

Native fixtures exercise continuous output followed by forced interruption,
quick successful output beyond the cap, explicit unlimited output, and invalid
captured limits. Independent CLI fixtures exercise count/byte retention, dry-run
byte preservation, modified/foreign/escaped members, unrelated producer metadata,
active/viewed-file locks, idempotence and resumed partial pruning. These fixtures
also run against the installed package tool. Cooking-cache/staging reclamation,
unmanaged history and asset-operation quarantine retention remain separate work.

## Increment 26: Resource publication process-kill contracts (30 October 2026)

The new `WPResource.publication_crash` fixture uses a standalone host, real
ResourceSystem/SQLite compilation index, checked admission and immutable exporter.
It kills the host during a deterministic test compiler's payload write, during
admission with staging present, and after destination publication before success
acknowledgement. No production fault hook is introduced. Each case verifies the
previous package bytes, source-free read-only runtime consumption after removing
authoring/cooked availability, writer/index reopen in a fresh process, explicit
new-destination export and existing-output preservation. The committed-but-
unacknowledged case verifies the new package as well. This closes these controlled
process-death contracts; real mesh/audio/animation/importer crashes, physical
power loss, orphan artifact cleanup and representative scale/soaks remain open.

## Editor shipping target and current release boundary (30 October 2026)

The user selected the native Editor project as the shipping target. Visible
software and DX11 probes used an isolated workspace and the previously validated
19 October package; both native-window capture paths timed out, including one
retry after activation. They establish no mouse/input or artist workflow pass.
Evidence is retained in
`C:/Users/casper_music/.codex/visualizations/2026/10/18/01a14ff8-ad4b-7281-81a5-8eaffeefa475/editor-asset-release-ui-20261030.json`.
This workstation exposes Microsoft Basic Display Adapter, driver
10.0.19041.3636, so representative hardware certification is unavailable here.

The source audit still finds cooked texture/material and Lua compiler adoption;
scene/prefab/mesh/animation/audio/UI typed package families are not certified by
those consumers. Full profile/toolchain BuildKeys, remaining authoring/reference
coverage, asynchronous residency, cache/variant/streaming and R2 domain workflows
remain implementation work. Clean-machine, legal/license, real hardware,
interactive Editor and full-family certification remain open. None of the focused
or installed checks changes those gates to passed.

### Validation evidence for increments 25–26

RelWithDebInfo rebuilt successfully and all ten release suites passed in 105.73
seconds (`project_x64/resource-package-release-final-opt-tests.log`). This includes
the new publication crash contract, native job/log supervision, CLI retention,
catalog, resource, graphics, Lua and PackageEditor workflow suites. Debug resource
and Editor targets also rebuilt. All ten Debug release suites passed. The initial
aggregate Editor run completed in 749.92 seconds with 73/95 cases passing and 22
Play-mode fixture construction failures
(`project_x64/resource-package-release-final-debug-tests.log`). Its private plugin
list still requested removed OIS input and omitted the audio provider now required
before resource initialization. The fixture now omits OIS and loads the configured
audio provider; production checks/assertions and timeouts are unchanged. Rebuilt
primary Play and worker-restored camera cases both passed (15.81 seconds total;
`project_x64/resource-package-release-editor-fixture-focused.log`). The final
aggregate rerun passed in 794.38 seconds within the unchanged 900-second timeout:
95/95 cases and 1,966,040/1,966,040 assertions passed. Driver/result:
`project_x64/resource-package-release-editor-final-all.log`; retained detailed
report: `project_x64/resource-package-release-editor-final-report.log`.
This verifies the automated native Editor suite, including the corrected lifecycle
fixture; it does not replace visible input, device or full-family package evidence.

The first installed attempt correctly failed because the current CMake cache had
`WP_INSTALL_EDITOR_PACKAGE=OFF`, leaving the Debug rejection gate unexercised. Its
failed evidence is retained in `resource-history-validation-20261030`. Packaging
was explicitly configured ON and the complete runtime target rebuilt. The script
now reports a disabled install configuration directly when `-SkipBuild` is used.

Fresh installed validation passed with `validationPassed=true` and
`releaseCertified=false`. Runtime/symbol archives, payload/module hashes and six
installer/CLI/startup checks are retained at
`C:/Users/casper_music/.codex/visualizations/2026/10/18/01a14ff8-ad4b-7281-81a5-8eaffeefa475/resource-history-final-validation-20261030/`.
The relocated CLI also passed the history safety/restart fixtures. Read-only and
cross-volume startup preserved installed bytes; missing required media/plugins
failed explicitly, and Debug installation was rejected. Loaded runtime hashes
match the matching current built modules. Driver log:
`project_x64/resource-package-release-final-installed-tests.log`.

## Next implementation gates

User-requested scope extension: production-plan packages **RES-050–054** add
platform texture profiles, Windows DDS cooking, native compressed uploads,
Editor target previews and package certification. Increment 9 implements the
bounded 2D compiler/upload subset. Complete the
remaining profile authoring, quality, topology and shipping-package gates using
the same immutable resource pipeline.

1. Finish the first integrity slice: authored numeric-root UUID mapping, immutable
   metadata migration, full-project reference coverage, asset-operation retention
   and conflict resolution UI.
2. Extend target/mode indexing to full profile/toolchain BuildKeys, authored
   revision fences and generation recovery/retention on top of captured inputs,
   complete-request index transactions and immutable artifacts. Certify
   importer-specific interruption and power-loss publication.
3. Extend the saved-document reference graph across remaining domain authoring routes
   and legacy migration/remaining-format discovery; persist
   typed compile/install/platform dependency modes, invalidation and cycle policy.
   Indexed forward/reverse queries and project missing/type diagnostics now exist.
4. Integrate the runtime UUID map and pinned directory export with typed consumers,
   richer root picking/closure inspection and owner-thread publication throughout the
   Editor and shipping sample. Extend the owned Windows package job to the
   remaining asynchronous requests/residency and platform requirements.
5. Extend the bounded catalog inspection window with asynchronous scale gates;
   implement domain editing, previews, full reference coverage, packaging controls and
   the declared format-family matrix.
6. Execute the plan's representative Editor, source-free package, failure,
   concurrency, performance and release gates. Passing focused tests alone does
   not establish production readiness.
