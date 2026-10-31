# Pinned runtime packages and durable UUIDs

`resource::ResourceSystem` exports a source-free resource directory through
`writeRuntimePackage(directory, roots, bindings, error)`. This reuses the existing
compilation index, immutable WPRS artifacts and runtime manifest loader. It does
not introduce a second catalog or runtime database.

Compile the requested roots with `CompilationOptions::packagedBuild = true` first.
Every install dependency must have current successful compilation evidence in the
same authoring session. The package operation uses the selected package variant;
an Editor-mode selection is rejected. Failed compiles revoke export evidence as
before. The exporter holds the resource operation fence through validation, copying
and publication, preventing another compile, mode switch or shutdown on that service.

```cpp
using namespace workphone::resource;
CompilationOptions options;
options.packagedBuild = true;
auto report = resources.compile(rootId, options);
if (!report.succeeded()) { /* Report diagnostics and stop. */ }

Array<RuntimeResourceBinding> bindings = { { authoredUUID, rootId } };
if (!resources.writeRuntimePackage(packageDirectory, {rootId}, bindings, error)) {
    // Report error; no complete package was published at packageDirectory.
}
```

The destination must be new and separate from both source and cooked storage. The
exporter creates an exclusive sibling `.wp-package-*` staging directory, writes
`runtime.wprm`, copies only the pinned install closure using its relative artifact
paths, and validates copied identity, compiler version, hashes, size, dependencies
and dependency generation pins. It then renames the directory exclusively. Existing
destinations, including empty directories, are never replaced. Windows uses
`MoveFileExW` without replace-existing, Linux uses `renameat2(RENAME_NOREPLACE)`, and
macOS uses `renamex_np(RENAME_EXCL)`. Other platforms or filesystems without an
exclusive rename implementation fail publication.

Normal failures clean up only the request-owned staging directory. A process crash
can leave that directory behind; orphan recovery and retention are still open.
Parent directories created for a failed request may remain empty. This establishes
complete-directory visibility during normal execution, not machine/power-loss
durability, cryptographic authenticity or protection against hostile filesystem
replacement. No executable, plugin or redistributable dependency is included.

## UUID manifest contract

The existing three-argument `writeRuntimeManifest` still writes manifest payload
version 2. The overload with a nonempty `Array<RuntimeResourceBinding>` writes
version 3. The WPRS outer container remains unchanged. Versions 1 and 2 continue
to load with an empty UUID map; there is no UUID-to-path guessing.

Version 3 appends a little-endian 64-bit binding count followed by length-prefixed
UUID and ResourceID strings to the version-2 payload. Entries are sorted by
canonical lowercase UUID. UUID input accepts exactly the dashed 36-character hex
form, case-insensitively, and rejects nil. UUIDs and bound ResourceIDs are each
unique. Every binding must identify a resource already in the pinned install
closure. The map is bounded by the existing 65,536-resource and 16-MiB manifest
limits; malformed maps fail before a mount becomes initialized. Unbound closure
members remain accessible by ResourceID. A partial map is explicitly supported;
export does not certify complete authored-reference coverage.

`CatalogResourceAdapter::runtimeBinding(request, binding, error)` supplies the
validated authored UUID and exact ResourceID from a current catalog request.
Failure clears the binding. This captures detached evidence; the package caller
must own catalog-session admission throughout collection and export. The resource
operation fence does not prevent independent catalog changes. Bundled media and
project catalogs must be mapped explicitly with distinct logical source identities;
UUID/path collisions fail rather than applying implicit mount precedence. The
checked catalog export below enforces per-file project shipping eligibility;
full saved-reference coverage and mounted-media closure integration remain open.

```cpp
ResourceSystem runtime(nullptr, nullptr);
RuntimeResourceConfig config;
config.compiledRoot = relocatedPackageDirectory;
config.manifestPath = relocatedManifestPath; // runtime.wprm in that directory
config.target = expectedTarget;
if (runtime.initializeRuntime(config, error)) {
    auto resource = runtime.loadUUID(authoredUUID, error);
}
```

`resolveRuntimeUUID` clears its output on failure. `loadUUID` resolves and loads
under the same mount lifetime fence, uses the same complete-closure cache and
request limits as `load(ResourceID)`, and retains immutable caller-owned objects
across shutdown. Missing, malformed and nil UUIDs fail with diagnostics. Lookups
require an initialized runtime mount and cannot consult a catalog, source tree,
compiler or SQLite database. A previously exported UUID remains pinned to its old
generation after an authoring rename or recook; a new binding can retain that UUID
while selecting the new logical path.

## Catalog-admitted export and shipping policy

Use `CatalogResourceAdapter::writeRuntimePackage(directory, roots, error)` with
current adapter requests for project packages that must enforce shipping policy.
It verifies the actual ResourceSystem source root and every pinned install
dependency against the project catalog and the adapter's registered type mappings.
Every member receives a UUID binding. Missing or ambiguous catalog identities,
unsupported types, subresources, stale root requests, Editor-only or excluded
dependencies reject the entire export with diagnostics.

The resource operation fence is acquired before catalog and SQLite connection
locks. A retained `BEGIN IMMEDIATE` admission excludes independent writers and
same-connection operations through exclusive directory publication; it rolls back
its read admission afterward. This route requires a writable project catalog.
Read-only and mounted bundled-media package integration are not supported here.

`getAssetPackagePolicy` inspects a file without creating schema. Project files
default to `AssetPackageUsage::Runtime`; `setAssetPackagePolicy` persists Runtime,
EditorOnly or Excluded from a current project file snapshot. The Asset Database
window exposes these controls. The separate version-1 policy schema and revision
triggers reject unsupported or incomplete layouts without repair. Policy writes
expire catalog snapshots, including writes through independent SQLite connections.
Media inspection uses the inventory's existing policy and rejects project setters.
Copy inherits policy atomically; move retains it; quarantine/delete retains UUID
policy for restore. Purging retained history/policy remains open.

The lower-level `ResourceSystem::writeRuntimePackageChecked` invokes a resolver
with the actual source root, target and pinned closure. A successful resolver must
return complete UUID bindings and a lifetime token retained until publication or
failure. Acquire external locks in resource -> catalog -> connection order; do not
reenter resource operations from the callback. A second manifest validation checks
that artifacts and generations did not change during admission. Resolver exceptions
fail export and clean request-owned staging. The raw explicit-binding export and
partial-map manifest format remain available; they do not enforce catalog policy.
PackageEditor and the command-line resource-directory slice below use checked
catalog admission through the same saved profile request.

## Command-line resource directory packaging

Build `WorkphoneResourcePackage` when Workphone, SQLite, WPGraphics and WPLua are
enabled. It registers the existing graphics (`texres`/`matres`) and Lua compilers.
Provide an existing writable project catalog, its actual source root, a cooked
root, a new destination, an explicit target and root asset UUIDs:

```powershell
WorkphoneResourcePackage.exe 'G:/Project' 'G:/Project/AssetDatabase.db' `
  'G:/Cooked/Project' 'G:/Packages/Project-v1' pc `
  --map script=lua --map texture=texres --map material=matres --root <asset-uuid>
```

Mappings are explicit catalog type names, not inferred from filenames. Use the
types actually stored in the project catalog; repeat `--map` and `--root` as
needed. Resolve all roots first, compile each root and its dependency closure
with `packagedBuild=true`, then publish through
`CatalogResourceAdapter::writeRuntimePackage`. Missing identities, unmapped or
ineligible dependencies, compilation errors and occupied destinations return
nonzero without publishing a new complete package. Windows arguments use the
wide-character entry point, preserving Unicode paths. Exit codes are 0 for
publication, 1 for an operation/validation failure and 2 for invalid argument count.

Successful earlier root compilations remain in the derived cache if a later root
fails; output publication is still all-or-nothing. This synchronous command does
not provide cancellation, executable/plugin distribution,
additional typed compilers. It may perform the catalog's
normal supported legacy schema migration on opening; retain project backups as for
Editor authoring. It rejects a missing catalog path rather than creating one.

### Saved CLI profiles and runtime verification

Prefix the positional request with `--save-profile <new-file.json>` to save it
without opening the catalog, cooking or exporting. The parent directory must
exist. Publication is exclusive and never replaces an existing profile. It uses
a staged file and same-directory hard link; filesystems without hard-link support
reject saving. Saving validates structure, not the current existence or eligibility
of asset UUIDs; the checked build validates those when the profile runs.

```powershell
WorkphoneResourcePackage.exe --save-profile 'G:/Project/shipping.json' `
  'G:/Project' 'G:/Project/AssetDatabase.db' 'G:/Project/Cooked' `
  'G:/Packages/Project-v1' pc --map script=lua --root <asset-uuid>
WorkphoneResourcePackage.exe --profile 'G:/Project/shipping.json'
WorkphoneResourcePackage.exe --verify 'G:/Packages/Project-v1' pc <asset-uuid>
```

Version 1 JSON contains exactly `version`, `sourceRoot`, `catalog`, `cookedRoot`,
`output`, `target`, `mappings` and `roots`. A mapping has exactly `catalogType` and
`extension`; roots are UUID strings. Paths resolve relative to the profile file,
independently of the process working directory. Saved paths are relative where
possible, allowing a project/profile tree to relocate. Absolute paths remain
supported. There are no command-line overrides when loading a profile.

The reader rejects unsupported versions, unknown/duplicate/missing fields,
malformed UTF-8/JSON, NULs, trailing content, duplicate root UUIDs (ignoring case),
duplicate catalog type mappings, files above 64 KiB, strings above 4096 bytes,
more than 128 mappings or 512 roots. Profiles use the same compilers and checked
exporter as positional requests; they cannot bypass shipping policy.

`--verify <directory> <target> <uuid> [uuid...]` mounts the existing runtime
manifest and loads every requested UUID with its pinned install closure. It uses
`ResourceSystem(nullptr, nullptr)` and never opens a catalog or compilation index.
It checks container identity, target and payload integrity through the existing
runtime loader. It verifies requested closures, not every unrequested manifest
entry. The CLI executable still requires its normal DLL dependencies. This does
not execute Lua or install graphics/audio resources into a live consumer.
No files are written by verification. Missing/corrupt content returns nonzero.

### PackageEditor and owned background jobs

PackageEditor loads and authors the supported JSON profile settings: catalog,
cooked storage, output directory, target, root UUIDs and explicit type mappings.
The active project source root is displayed read-only. Roots and mappings accept
semicolon-separated lists. Load restores the saved values; Save new profile
creates an unused file through the shared native writer and never replaces an
existing profile. Choose a new filename when editing an existing profile. The
profile parent directory must already exist. Unsaved edits disable Build, and
project changes reset the draft. The selected saved profile restores after
closing/reopening the window in the current manager session. The default file is
`package.json` relative to the active project.

Native read/save bindings return structured success/error results, validate the
active source root and bounded settings/lists, and reject malformed/nil/duplicate
root UUIDs, invalid/duplicate mappings, unknown fields and sparse arrays. They do
not open a catalog or cook content. Asset existence and shipping eligibility are
checked by the normal build/export route. Build, Cancel, job status and the
per-request log path use the owned background job described below. Unsupported
build-type/compression/platform/version controls remain absent.

The native `IPackageManager` resource-package API queues one `ResourcePackageJob`
at a time on the existing worker queue. It requires a running worker pool and
rejects execution on the primary thread. Before queueing it reads the bounded
profile and verifies that its source root is the current project. The detached
request captures resolved paths and settings; later profile edits cannot redirect
that job. The worker writes an immutable request snapshot under
`.workphone/package-jobs`, then launches the deployed `WorkphoneResourcePackage`
directly with Unicode arguments and a restricted inherited-handle list. No shell
or process-wide current-directory change is used.

A Windows Job Object owns the child lifetime and kills it if its owning handles
close during process teardown. Only the package child is awaited, never the global
job queue. Its exit status determines succeeded/failed/cancelled; stdout/stderr go
to a unique log. Completed requests remove their snapshot; logs are retained.
UI polling occurs on the Editor update path and never from the worker. Project
path changes, manager unload and PackageEditor unload request cancellation.

Cancellation starts cooperatively through an inherited event: the CLI checks
before authoring setup, between root compilations and before export. It returns
exit 3 when cancellation wins before export. Editor-owned Windows jobs now allow
30 minutes of child execution by default, followed by a 30-second cooperative
grace period. A user cancellation starts that grace period immediately. Limits
use the monotonic Windows clock and are captured before the job enters the queue.

`WP_RESOURCE_PACKAGE_MAX_RUN_MS` overrides the child deadline (1000–86400000 ms);
`WP_RESOURCE_PACKAGE_CANCEL_GRACE_MS` overrides the grace period (1000–600000 ms).
Either can be set to `0` to disable that respective limit. Invalid values reject
the request before queueing. Environment changes after queueing cannot alter the
captured execution policy. These are Editor job settings, not profile fields or
timeouts for a standalone CLI invocation. The deadline starts when the child is
resumed; it does not bound filesystem/recovery work before launch.

A child that finishes successfully before escalation remains success even if
cancellation was requested too late. A cooperative exit 3 reports `cancelled`, or
`timed out` when caused by the deadline. An unresponsive child is terminated via
its owned Windows Job Object after grace expires; the parent then allows at most
five seconds to confirm exit. Forced stops and unexpected child exit codes report
`interrupted`, retain the captured profile/ownership marker, and explicitly leave
the publication outcome unknown. No existing output is removed or replaced.
Inspect outputs and recover the retained record before explicitly retrying.
Interruption inside real compiler/export publication windows, package-staging
orphan cleanup and log retention quotas remain open. A queued cancelled job still needs
the queue to process it before its detached result becomes final.

The Editor build and install package include the tool beside `Editor.exe` with
its runtime dependency closure. The legacy texture/platform flag methods remain
for API compatibility, but do not change a saved resource profile. Resource profile
builds always include required eligible install dependencies. This window builds
resource directories; it does not package game executables or certify other typed
families or interactive workflows.

PackageEditor also searches the active project's catalog for file roots, using a
literal path/UUID filter and optional exact catalog type. It shows 25 matches per
page with Previous/Next navigation. Continuations capture the filters, catalog
instance and revision; stale or mismatched continuations fail and clear the old
page. Changing filters or projects clears navigation history. Clicking a match rechecks its path,
type, catalog instance and revision before adding its UUID. A reopened catalog or
project switch invalidates the old selection. The picker constructs no resources,
requires an explicit type mapping and suppresses duplicate roots. Adding a root
marks the draft unsaved; save to a new profile filename before building. Search
is disabled during a package job. This searches the active project catalog; a
different catalog entered in the profile is still checked by the build pipeline.
Selection alone does not certify source availability, compiler support, shipping
eligibility or install dependencies.

## Checked closure previews

Use **Preview saved closure** in PackageEditor, or run:

```text
WorkphoneResourcePackage --preview-profile shipping.json new-preview.json
```

Preview compiles the saved roots in package mode, then uses the same pinned
manifest collector and catalog admission resolver as export. It checks every
install member's UUID, type mapping, revision and shipping policy. The report
lists selected roots, ResourceIDs, UUIDs, payload byte counts and direct install
dependency edges. Compile-only inputs are not install members. An Editor-only,
excluded, missing or unmapped install member rejects the whole preview; no partial
report is published. The report filename must be new and its parent must exist.
Publishing the report uses an exclusive hard link and preserves existing files.

Preview updates the authoring catalog/cooking cache and temporarily writes an
owned manifest under cooked storage. It never creates, checks or replaces the
profile's package destination, so an existing output does not prevent preview.
Normal completion/failure removes the temporary manifest tree. A preview is a
snapshot of the admitted cooked generation, not permission to skip build checks;
the subsequent build compiles and validates again. Crash/power-loss cleanup and
durability certification remain open.

The Editor runs preview through the same owned child job, immutable captured
profile, single-request guard, cancellation event and project/unload lifecycle as
build. Successful reports remain beside job logs as `<request>.preview.json`;
owned log/report retention is described below. UI polling reads a successful report
on completion and again on explicit page navigation on the owner thread. It shows
25 members per page across at most 4096 resources, with Previous/Next controls,
the current row range, and up to three immediate inclusion parents per member.
Normal update polling never reparses the report. Replaced report paths or failed
reads clear preview navigation. The legacy `readResourcePackagePreview` Lua
helper still returns its first 100 rows; the window uses the additive
`readResourcePackagePreviewPage` helper. The full JSON report
retains every direct dependency edge and is limited to 8 MiB. Payload byte totals
exclude WPRS containers, the runtime manifest, filesystem overhead and compiler
inputs; they are not installed size or residency estimates. Editing settings,
loading/saving a profile, adding a root or switching projects clears the displayed
snapshot. Failed/cancelled requests expose diagnostics instead of old results.

## Interrupted package requests

On Windows, each executing package job owns an exclusive `<request>.json.lock`
handle until its captured profile has been cleaned up. The lock is created
exclusively, is not inherited by the child, and is deleted by Windows on handle
close, including owner process death. The existing kill-on-close Job Object owns
the package child. Worker cleanup removes a captured profile only after that
worker successfully created it; failed ownership/profile creation preserves an
existing file.

Before writing a captured profile, the worker exclusively creates and flushes
`<request>.owner` with the exact versioned `WORKPHONE_PACKAGE_REQUEST_V1` marker.
Normal cleanup removes its own marker only when profile cleanup succeeds (or
capture never succeeded). A process crash leaves the marker beside its profile.
Recovery requires an exact, bounded marker match before opening the profile;
unmarked legacy records and unknown/truncated/extended markers are preserved
with warnings. This avoids treating an older producer's unlocked live request
as abandoned. Archived records retain their ownership marker as history.

Before a new build or preview, the worker inspects up to 4096 entries in the
project's `.workphone/package-jobs` directory. A request filename must be a nonnil
dashed UUID followed by `.json`. The profile must pass the shared bounded parser
and resolve to the same project source root. Recovery takes its own exclusive
request lock; unavailable locks are skipped. Abandoned snapshots are renamed
exclusively to `<request>.interrupted.json` in the same directory. Invalid or
foreign-project snapshots, non-regular files and existing archive conflicts are
preserved with warnings. Redirected/reparse job-storage directories are rejected.
The worker's completion diagnostic reports recovery counts.

Inspect/recover records without opening a catalog or cooking assets with:

```text
WorkphoneResourcePackage --recover-jobs <project-root>
```

The CLI lists recovered paths, lock skips and warning counts. Repeating recovery
does not reprocess interrupted records. Logs, preview reports, package outputs,
cooking artifacts and package staging trees are untouched. Request records from
older producers have no ownership/live-lock protocol and are left in place for
manual inspection. An archived request has an unknown publication outcome. Recovery never retries
or declares a prior package successful/failed. Inspect outputs first, then save
an explicit profile with an unused destination before retrying.

This establishes snapshot recovery and child-lifetime behavior after a tested
owner-process kill. The separate publication test described below covers process
termination inside the real resource pipeline. Orphan cooking/staging artifact
reclamation, power-loss durability and hostile filesystem races remain open.

## Diagnostic budgets and completed-job history

Windows jobs drain child stdout/stderr through a private pipe into the exclusive
job log. The default log limit is 32 MiB. Bytes beyond that limit are discarded,
the log receives a truncation footer within its existing size, and the supervisor
signals cancellation. An unresponsive child follows the same cancellation grace
and forced-interruption route as a timeout. Exit zero still wins if publication
already completed; a log limit never authorizes deleting package outputs. Pipe
draining has bounded work per watchdog poll, so continuous output cannot starve
deadline/cancellation checks. `WP_RESOURCE_PACKAGE_MAX_LOG_BYTES` accepts
1024–536870912 bytes; zero explicitly disables this limit.

After a known successful, failed, cancelled or timed-out completion, the worker
writes an exclusive `<request>.completed.json` receipt. It binds the project,
request UUID, completion time/status and exact size/fingerprint of the owned log
and successful preview report. Interrupted requests receive no receipt. Job
completion remains busy until receipt/cleanup/retention work finishes.

Automatic retention keeps up to 64 completed requests and 64 MiB of managed
history, reserving the just-completed request so the window can read its log and
report. Before queueing, the Editor captures validated overrides:

| Setting | Nonzero range | Default |
| --- | --- | --- |
| `WP_RESOURCE_PACKAGE_HISTORY_KEEP_COUNT` | 1–4096 | 64 |
| `WP_RESOURCE_PACKAGE_HISTORY_MAX_BYTES` | 1024–1073741824 | 67108864 |

Zero in either history setting disables automatic pruning. These settings and the
log budget are captured with the request; later environment changes do not alter
an executing job. Inspect or apply an explicit history policy with:

```text
WorkphoneResourcePackage --prune-jobs <project-root> <keep-count> <max-bytes>
WorkphoneResourcePackage --apply-prune-jobs <project-root> <keep-count> <max-bytes>
```

The first command is a dry run. Explicit zero count/bytes selects every eligible
completed bundle for removal. Neither command touches package outputs, authored
files, unmarked legacy logs, invalid/foreign receipts or interrupted evidence.
Pruning acquires the request lock, validates receipt identity and file bytes,
and refuses modified, redirected, open or unavailable files. It persists an exact
receipt-bound `.pruning` intent before deleting members; a later invocation can
resume an interrupted deletion without treating arbitrary missing files as owned.
Unknown intents are preserved. It scans at most 4096 directory entries and reports
incomplete accounting, lock skips and preservation warnings. The budget covers
valid receipted files, not the entire job directory: protected or unprunable data
can exceed it. Cooking caches and abandoned package staging trees are separate.

## Process-termination publication contracts

`WPResource.publication_crash` kills a standalone host in three controlled phases
of the real ResourceSystem: during a test compiler's payload write, during checked
admission with staging present, and after the destination is committed but before
the caller receives success. Each case preserves a previous package byte-for-byte,
removes source/cooked availability, verifies old and committed new packages through
the source-free runtime, then reopens the authoring pipeline in a fresh process
and performs an explicit export to a new destination. Existing destinations are
never overwritten. The compiler is a deterministic test family, so this proves
process-death publication/reopen contracts rather than real importer crash safety,
physical power-loss durability, staging reclamation or full-family certification.

## Validation boundary

`WPResourceTests` covers deterministic v3 export, invalid/duplicate/aliased/outside-
closure mappings, legacy manifests, empty maps, concurrent UUID/path identity,
closure budgets, last-good manifest preservation, package-mode admission, exact
file membership, existing destinations, corrupt inputs, staging cleanup, relocation
with original source/cooked storage removed and read-only runtime byte preservation.
`WorkphoneAssets.catalog_contracts` covers the real catalog-to-package UUID bridge,
stale binding rejection and UUID-preserving authoring rename beside a pinned package.

This resource-directory API does not complete interactive closure-preview
acceptance, scene/mesh/animation/audio/UI typed consumer adoption, multi-mount precedence, asynchronous
residency, full-family compiler interruption, interactive Editor acceptance
or representative hardware/full-family shipping application certification. The
native package-job fixture separately exercises a source-free Lua consumer; that
does not close the other typed-family or interactive release gates.
