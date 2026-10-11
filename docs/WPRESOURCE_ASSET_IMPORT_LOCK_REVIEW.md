# Asset import and reimport lock fixes

Reviewed and updated 2026-10-11. Applies to the legacy Editor/resource database
import workflow, the asset catalog and its SQLite backend.

## Import scheduling and maintenance

Import requests now enter one FIFO work queue. Only its reserved drainer executes
operations; admission never waits for worker jobs. Each folder/batch imports its
files inline before releasing the queue to the next request. No import producer
queues children and then waits for them. The process-wide queue intentionally
serializes import maintenance across resource-database instances as well.

The Editor queues Import and Reimport through AssetImportJob. Requests capture
the original resource database, catalog lifetime, project and cache paths.
The engine queue also checks project/catalog/session state before running a
request and between files. It retains captured paths rather than redirecting
pending work through the next project's ambient configuration. A cancelled pump
releases its reservation so future imports can proceed. Already running native
file operations remain synchronous; shutdown/project changes are checked between
operations rather than interrupting arbitrary native calls.

Reimport no longer deletes every file in the cache directory. Cache maintenance
runs in the same queue and removes only the established generated .fbmeshbin and
.pxtrianglemesh payload types, confined to the captured cache root. The live
catalog and its -wal/-shm/-journal sidecars are explicitly excluded, even when the
catalog uses a nonstandard filename. Source files, unknown files, manifests and
other database files survive. Unknown/generated formats are deliberately retained
until they have an explicit ownership and regeneration policy.

Catalog cleaning resolves file existence against the catalog's captured source
root, considers only file rows, and releases its SELECT statement before starting
row-deletion transactions. Scene-domain entries are not treated as missing files.

## Resource cleanup

Per-file background imports no longer invoke global runtime resource eviction.
Explicit cleanup requested from a worker is deferred to the primary job queue.
On that owning thread, unload-only cleanup selects uniquely owned candidates
under m_mutex, releases it, rechecks ownership, and invokes unload callbacks.
Removal detaches uniquely owned candidates under the mutex and runs callbacks
and final releases afterward. Resources acquired in the meantime are retained
rather than unloaded by that cleanup pass. Instance-pointer snapshots are also
protected by m_mutex. Resource interfaces still require their established owner
thread rules; this does not permit arbitrary concurrent native resource use.

## SQLite contention

Writable SQLiteDatabase connections now explicitly use a 1000 ms busy timeout
for both loadFromFile overloads instead of CppSQLite3's inherited 60 seconds.
A busy/locked mutation logs a retry diagnostic and returns failure; it does not
retry indefinitely or leave a new transaction open. Read-only media and the
separate resource-compilation database retain their existing timeout policies.

The reviewed catalog transaction lock order remains manager mutex followed by
SQLite connection mutex. Both are recursive for same-thread nested operations.
The serial import queue never holds its own mutex while calling import code,
resource lifecycle callbacks, database operations or captured-owner destructors.

## Validation

- RelWithDebInfo Workphone, WPSQLite, WPAssetCatalogTests and Editor builds passed.
- WorkphoneAssets.catalog_contracts and WPLua.runtime_contracts passed.
- New deterministic concurrency tests verify immediate admission during an active
  operation, FIFO completion and reentrant submission after the current batch.
- Real-SQLite import fixtures verify retained-job progress, single-worker and
  primary-only progress, asynchronous pumping, cancellation recovery, stale
  project rejection, queued reimport, generated-cache cleanup, live catalog and
  source preservation, and stable UUIDs.
- A competing real SQLite writer verifies prompt timeout and successful subsequent
  transactions. A cleanup callback starts a second thread that acquires the
  resource-database mutex, proving the callback is invoked outside that mutex.
- git diff --check passed.

Interactive Editor stress testing with GPU/audio imports, shutdown and project
switches is still an acceptance gate. No live certification is claimed by these
focused fixtures.
