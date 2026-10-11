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

## Next implementation gates

User-requested scope extension: production-plan packages **RES-050–054** add
platform texture profiles, Windows DDS cooking, native compressed uploads,
Editor target previews and package certification. Increment 9 implements the
bounded 2D compiler/upload subset. Complete the
remaining profile authoring, quality, topology and shipping-package gates using
the same immutable resource pipeline.

1. Finish the first integrity slice: authored numeric-root UUID mapping, immutable
   metadata migration, full-project reference coverage, retention and conflict resolution UI.
2. Extend target/mode indexing to full profile/toolchain BuildKeys, authored
   revision fences and generation recovery/retention on top of captured inputs,
   complete-request index transactions and immutable artifacts. Certify
   interrupted publication.
3. Extend the saved-document reference graph across remaining domain authoring routes
   and legacy migration/remaining-format discovery; persist
   typed compile/install/platform dependency modes, invalidation and cycle policy.
   Indexed forward/reverse queries and project missing/type diagnostics now exist.
4. Integrate typed runtime consumers and owner-thread publication throughout the
   Editor and shipping sample; complete asynchronous session-safe jobs.
5. Extend the bounded catalog inspection window with asynchronous scale gates;
   implement domain editing, previews, full reference coverage, packaging controls and
   the declared format-family matrix.
6. Execute the plan's representative Editor, source-free package, failure,
   concurrency, performance and release gates. Passing focused tests alone does
   not establish production readiness.
