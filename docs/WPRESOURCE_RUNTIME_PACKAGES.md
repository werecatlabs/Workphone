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
UUID/path collisions fail rather than applying implicit mount precedence. Per-asset
shipping eligibility and saved-reference graph policy remain separate open work.

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
