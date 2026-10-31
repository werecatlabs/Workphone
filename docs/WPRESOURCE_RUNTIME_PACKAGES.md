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
enabled. It registers graphics (`texres`/`matres`), Lua, terrain, foliage, native
mesh/skin, skeleton/clip, transform-only scene/prefab and bounded UI compilers,
plus PCM audio when enabled. The supported subsets are described below.
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

Version 2 adds the required `buildProfile` object with exactly `name`, `toolchain`
and `settings`. The identities must be nonempty; `settings` is an object of up to
64 string values (empty values are allowed). Duplicate keys and embedded NULs are
rejected. The CLI flags `--build-profile <name> --toolchain <identity>` select this
version; optional `--build-settings '{"quality":"high"}'` supplies settings.
Saving without these flags preserves version 1 and its existing cache namespace.
Settings identify effective compiler inputs; format-specific options still belong
in the existing resource recipes.

On Windows, profiled builds derive an additional toolchain identity from the
loaded executable and DLLs beside it, plus the loaded Workphone, WPGraphics,
WPLua, WPAudio, WPSQLite and Direct3D compiler modules. Their filenames, sizes and content hashes
participate; installation paths do not. Read handles deny writes/deletion through
compilation and export, and a changed module inventory rejects publication.
The saved toolchain label remains unchanged and must identify external tools or
inputs outside this module inventory. Labels are limited to 4000 bytes to leave
room for the derived identity. Automatic module capture currently requires
Windows. These hashes identify cache inputs; they do not authenticate binaries.

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

The checked package tool registers the existing terrain (`terrain`) and foliage
(`folworld`, `folpage`, `folspec`) compilers alongside graphics descriptors, Lua
and PCM audio when enabled. Catalog mappings must name each installed typed
dependency, including `texres` layer/leaf textures. Runtime consumers use the
existing terrain worker decode and foliage shallow-directory/page streaming
paths. `WorkphoneAssets.typed_package_families` exports through the real catalog
CLI, removes sources, catalog and cooking cache, then launches separate native
consumers against read-only packages. It checks terrain drawing and foliage
scheduled page reads, resident instances, draw calls and sampled texture pixels.
This bounded fixture does not certify every Editor project asset family or GPU.

### PackageEditor and owned background jobs

PackageEditor loads and authors the supported JSON profile settings: catalog,
cooked storage, output directory, target, root UUIDs, explicit type mappings,
build profile, toolchain label and build-settings JSON. Empty build identities
with `{}` settings retain version 1; both identities are required for version 2.
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

## Profile-aware build identity

Native callers can set `ResourceSystemConfig::buildProfile` before initialization.
It captures a profile name, toolchain identity and sorted effective settings. A
nonempty profile requires both identities; fields are bounded to 4096 bytes and
settings to 64 entries. Length framing prevents delimiter aliases. Initialization
copies the profile; compilers receive it in both dependency scanning and cooking.
The toolchain identity must identify the actual cooker/encoder revision (for
example its binary digest); the caller resolves that identity and effective
settings. This API does not discover external tools or implement format options.

Index lookup includes the full canonical profile key. SQLite schema 3 remains
compatible: the internal target column stores target, a NUL separator and the key
for profiled records, using explicit-length bindings. Unprofiled targets contain
no separator and retain their old namespace. Treat this column as an opaque
index key, not a display label. Compiler name/version, target, build mode, profile,
captured source inputs and dependency generations participate in the generation
hash. Artifacts keep the existing compact directory layout for Windows path limits.
Changing compiler-name hashing causes a one-time authoring recook; existing pinned
packages remain readable. All DLL consumers must be rebuilt with matching headers.
PackageEditor and version-2 package JSON persist these inputs through the shared
reader/writer and detached job snapshot. Version-1 package JSON continues to
select the unprofiled namespace. Authored source revision fences, external-tool
capture and importer-specific interruption certification remain separate gates.

## Scheduled runtime payload reads

`ResourcePayloadStream` combines the read-only `ResourceSystem::loadPayload` path
with the existing `WorldStreamingScheduler`. Creation owns a private immutable
runtime mount, preventing queued work from retargeting a project reopen. Owner
threads replace demand and take results; worker callbacks perform verified reads
without constructing scene, graphics or audio objects. Each demand names a resource,
job key, priority and maximum payload size. Scheduler admission bounds pending,
active, ready, working and result payload reservations. UUIDs resolve through that
same private mount, without a catalog lookup. Pending identities are limited to
1024 bytes and charged to pending admission. Returned resources alias the result
reservation, including after stream destruction, until the last consumer
releases them. Invalid/oversized demand is rejected; missing or corrupt pinned
resources return errors. Cancellation is checked before and after a read, and
stale generations are suppressed by the shared scheduler. It cannot interrupt a
blocked filesystem call. Limits cover payload reservations, not total process RSS.
Reads are shallow and do not enter the complete-closure cache or retain child
payloads. Owner threads choose which typed installer receives each result.
This is whole-payload scheduling, not mesh LOD, texture mip, audio seeking or
arbitrary chunk-range streaming.

## Runtime Script component UUIDs

Before loading a fresh `LuaManager`, call
`configureRuntimeScriptResources(RuntimeResourceConfig, error)`. It creates a
private source-free `ResourceSystem` mount with no catalog, compiler or index.
Normal `scene::Script` components already persist `scriptAssetUuid` and
`className`; their existing `loadScriptAsset` call now resolves the captured
package UUID and installs its compiled Lua/module closure before construction.
Missing and wrong-type UUIDs fail without catalog or loose-path fallback.

Runtime mode disables filesystem module search, `loadfile`, `dofile` and native
library loading. Loose-file/list loading, direct resource/state replacement,
authoring retarget and hot reload are rejected while the VM is active. Explicit
text/debug execution remains an API; this mode does not sandbox script code.
Unload retires the VM and releases the mount. Reusing that manager requires an
explicit runtime remount or an explicit authoring configuration while unloaded.
The mount caps individual payloads to the Lua source limit, closures to 256
resources and 64 MiB. Existing preload/source copies and Lua VM allocations are
not fully charged to the shared residency scheduler.
External debugger stop/unload signals cancellation before waiting for the Lua
executor lock, then cleans hooks/registry on the retired executor boundary.
Reentrant VM teardown from inside a Lua/native callback remains unsupported.

The native regression constructs and updates an actual Script component through
its UUID after removing source, catalog and compilation storage. It covers two
fresh VM lifetimes, missing/wrong-type rejection, a catalog spy, retained component
cleanup and preservation after rejected source/reload/retarget operations. Scene
compilation still rejects component-bearing graphs until their UUID dependencies
and pinned reference installation are integrated.

## Typed PCM audio

The package tool registers `audio::AudioResourceCompiler` when WPAudio is built.
Map the catalog's sound type to `wav`. Version 1 cooks validated, nonempty mono
or stereo PCM16 WAVE at 8–192 kHz, up to 64 MiB, into a normal pinned WPRS member.
Malformed/unsupported audio fails compilation. Raw WAVE bytes are retained for the
existing native decoder; no new catalog, codec cache or voice system is introduced.
`AudioResourceCompiler::validate` checks runtime type/version and WAVE structure.
On the sound control thread, `WPAudioSound::loadResource` takes a cooked resource,
uses the existing clip cache and voice lifecycle, and skips all filesystem fallback.
Unload an existing sound before changing assets. Device creation failure returns
false; successful cooking/validation does not establish audible playback.
The CLI fixture includes a Lua root with a WAV install dependency and removes
their authoring inputs/cook cache before UUID mounting. Other audio codecs, long
clip streaming, Editor sound authoring and the other typed families remain open.

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

## Native mesh geometry and typed materials

Map a catalog mesh type to `mesh` to cook portable WPM1 static geometry. Inputs
are limited to 64 MiB and validated with the existing budgeted native decoder.
Empty/invalid geometry and subasset selectors are rejected. An optional adjacent
`<file>.mesh.materials` file contains a JSON array of `matres` ResourceIDs in
submesh order, for example `["data://paint.matres","data://glass.matres"]`.
It is bounded to 64 KiB, 128 slots and 1024 bytes per ID; its slot count must match
the mesh. These materials are compile/install dependencies. Its bytes, including
absence, are captured with the request. Without a sidecar, all legacy native
material IDs must be zero. Ogre geometry import remains unsupported here.
The version-2 mesh envelope records target, package mode and ordered bindings;
the earlier version-1 unbound WPM1 payload remains readable.
`ClawMesh::loadResource` accepts a pinned runtime mesh on the owner thread,
validates/decodes/uploads and stages every pinned material before replacing
geometry and slots together through the Windows DX11 resource bridge. Other
builds report unavailable typed material installation. A failure preserves both
previous owners. Materials,
textures and programs remain retained with the mesh. It performs no source-path
fallback. The source-free native fixture renders two material regions and reads
their pixels; representative hardware and interactive Editor adoption remain open.

## Skeletons, clips and native skinning

Map the authored skeleton type to `skelres`. Its source uses the existing
`XMLSkeletonSerializer` schema. The bounded subset supports ordered contiguous
bone handles, parent hierarchy, position/quaternion transforms and linear clips
with strictly ordered translation/quaternion keys. Limits are 2 MiB of XML,
256 bones, 64 clips, 2048 tracks and 16384 keys. Linked rigs, base animations,
axis-angle keys, nonunit scale, DTDs and unknown fields fail explicitly.
`Animator::loadResource` installs a private skeleton and clips through the existing
importer after complete validation. Clear an existing animation graph before
using this entry point. Failed replacement retains the previous rig and clips.

Map a skin descriptor to `skinres`. Its JSON schema is:

```json
{"version":1,"mesh":"data://character.mesh","skeleton":"data://rig.skelres",
 "vertices":[{"joints":[0,0,0,0],"weights":[1,0,0,0]}]}
```

There must be one influence record per vertex in the referenced PNT/PNTC mesh,
with exactly four joints and four finite weights in [0,1], summing to one within
1e-5 before compiler normalization. Every joint slot, including zero-weight slots,
must exist in the referenced rig. Limits are 8 MiB of source,
65536 vertices, depth 8 and one million JSON tokens. Both children are pinned
compile/install dependencies; the mesh retains its own material dependencies.
`ClawMesh::loadResource` stages the full mesh, materials, bind vertices, rig and
native CPU bind pose before replacing the previous set. Existing skin deformation is reused
without reopening an authoring mesh. The native package fixture advances two
independent Animators, checks deformed vertices and reads posed/rest pixels after
source/catalog/cooking storage removal. Animation graphs, morphs, retargeting and
full model import remain separate feature gates.

## Transform-only scene and prefab graphs

Map the authored types to `fbscene` or `prefab`. The compiler validates the actual
Properties JSON schema and writes the existing `FBSCNBIN` binary representation.
The initial supported subset contains actor identity, local transforms and nested
actors. Components, external references, lighting and Editor camera state are
rejected until captured UUID dependency resolution and their typed installers
are integrated. Limits are 8 MiB of source, 4096 actors and depth 64, with canonical
nonnil UUIDs, finite transforms and unit quaternions.

`decodeActorGraphResource` retains a CPU admission lease in every decoded Properties
node, including nodes retained beyond the graph wrapper. `installCookedSceneGraph`
uses the existing `SceneLoadJob` append/rollback path and its captured load
generation; failed append restores the previously ready scene. Prefabs use the
existing `GamePrefab` and `createInstanceData` UUID remapping. Cloned nodes retain
their own lease. Editor contracts cover hierarchy/transforms, remapped identities,
rollback/readiness, stale generation rejection and escaped-node lifetimes. This
does not provide arbitrary component-bearing Editor scene shipping.

## Bounded native UI trees

Map an authored UI descriptor to `uires`. Version-1 JSON contains `version` and
`nodes`. Each flat node has `type` (`window`, `container`, `text` or `button`),
unique nonzero integer `id`, `parent` (-1 for the first root, otherwise an earlier
window/container index), `label`, two-number `position` and `size`, and boolean
`visible`/`enabled`. Text and buttons may additionally specify `textSize`.
Source/payload is limited to 64 KiB and 256 nodes. Unknown fields, scripts, images,
font references, invalid UTF-8, ImGui `##` label escapes, duplicate interactive
sibling labels, repeated window labels anywhere in the tree and colliding visible
external window labels fail explicitly.

The owner-thread installer creates hidden controls through the existing loaded
UI manager, applies native properties, verifies registration and parent membership,
then publishes the tree. Failed staging unregisters controls. Rejected publication
hides/detaches the instance, retaining registrations for retry until destruction.
Destruction unregisters the tree and releases its retained resource and
conservative CPU metadata lease.
Editor tests exercise the actual backend hierarchy, properties and cleanup;
interactive input, visual layout and project-switch publication remain open.

## Whole-payload residency and shared work

`ResourcePayloadStream::setResidencyLimits(entries, payloadBytes)` enables an
optional payload cache; `findResident(id)` returns its existing leased result.
Lookup updates eviction order. Quota eviction drops cache ownership while live
consumers retain scheduler result reservations. New accepted reads evict cached
ownership when result headroom is needed. Defaults disable caching. All cache
operations are owner-thread operations. The quota counts payload bytes and
entries. Resident requests are delivered through the scheduler without a new
payload reservation and retain the original physical lease. Matching resource
and size-limit requests within one demand batch share one physical read and fan
out logical job keys. Scheduler statistics count physical jobs. The stream retains
charged pending-delivery metadata until take/cancel. Cancellation clears aliases;
invalid demand preserves the previously accepted deliveries. Cached work runs
ahead of cold work, including equal maximum priorities, to avoid result-pressure
deadlock. Coalescing across concurrently active batches, complete object/header
accounting and chunk/LOD/mip/audio residency remain open work.

WPRS headers are capped at 8 MiB of encoded metadata and checked against actual
remaining file bytes before string/vector allocation. Container strings and all
ResourceIDs require valid UTF-8 without embedded NULs. This bounds parser input;
it does not establish full runtime metadata accounting or authenticated packages.

## Validation boundary

`WPResourceTests` covers deterministic v3 export, invalid/duplicate/aliased/outside-
closure mappings, legacy manifests, empty maps, concurrent UUID/path identity,
closure budgets, last-good manifest preservation, package-mode admission, exact
file membership, existing destinations, corrupt inputs, staging cleanup, relocation
with original source/cooked storage removed and read-only runtime byte preservation.
`WorkphoneAssets.catalog_contracts` covers the real catalog-to-package UUID bridge,
stale binding rejection and UUID-preserving authoring rename beside a pinned package.

This resource-directory API does not complete interactive closure-preview
acceptance, mixed-component scene and whole-Editor typed consumer adoption,
multi-mount precedence, complete asynchronous residency, full-family compiler interruption, interactive Editor acceptance
or representative hardware/full-family shipping application certification. The
native package-job fixture separately exercises a source-free Lua consumer; that
does not close the other typed-family or interactive release gates.
