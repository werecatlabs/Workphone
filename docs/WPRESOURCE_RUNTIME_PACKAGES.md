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
PackageEditor adoption of the checked route remains open. The command-line
resource-directory slice below uses checked catalog admission.

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
PackageEditor controls or additional typed compilers. It may perform the catalog's
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

## Validation boundary

`WPResourceTests` covers deterministic v3 export, invalid/duplicate/aliased/outside-
closure mappings, legacy manifests, empty maps, concurrent UUID/path identity,
closure budgets, last-good manifest preservation, package-mode admission, exact
file membership, existing destinations, corrupt inputs, staging cleanup, relocation
with original source/cooked storage removed and read-only runtime byte preservation.
`WorkphoneAssets.catalog_contracts` covers the real catalog-to-package UUID bridge,
stale binding rejection and UUID-preserving authoring rename beside a pinned package.

This resource-directory API does not complete PackageEditor/CLI profiles, scene/
mesh/animation/audio/UI typed consumer adoption, multi-mount precedence, asynchronous
residency, executable packaging, process-kill recovery, interactive Editor acceptance
or representative hardware/source-free shipping application certification.
