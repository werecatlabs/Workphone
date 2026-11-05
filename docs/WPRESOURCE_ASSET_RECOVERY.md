# Asset operation history and recovery

Open the Editor's Asset Database window with a project active. Choose
**Operation history / Recovery** to inspect journaled move, copy and delete
operations. The view uses bounded pages of 100 records ordered by operation ID;
this is stable ordering, not chronological ordering. Previous/Next navigate pages.
Search/Refresh returns to the first page. Bundled media has no writable recovery
workflow.

Select a record to inspect its ID, state, source, destination and project root.
For a committed delete, **Restore quarantined asset** restores the exact bytes and
original catalog UUIDs. **Undo selected operation** reverses committed moves or
copies. The service rechecks journal state, catalog identity, root containment,
byte digests and destination conflicts immediately before mutation. Existing
files are never overwritten. Failure diagnostics remain visible in the details
panel. Save/reload affected documents and resources as appropriate after undo.

Prepared or undoing records represent interrupted operations. Select one and use
**Retry selected recovery** to run recovery for that record. Prepared operations
return to their pre-operation state. Interrupted undo returns to the last
committed operation state, where ordinary undo remains available. The selected
retry checks journal identity and verifies up to 256 MiB of content. Reopening
the catalog still runs the existing automatic recovery pass.

Move, copy and undo publish durable intent before changing asset files. The
filesystem phase rechecks that intent under SQLite writer admission and holds
admission through the final catalog commit. Selected and automatic recovery use
the same admission, so a second catalog connection cannot recover a live rename.

On Windows, an interrupted move or delete can expose **Review conflict
preservation** when the restore path has been recreated and the journaled bytes
are available at exactly one other location. The plan shows both paths and the
operation-owned holding location. **Confirm preserve and recover** rechecks the
catalog revision, journal, paths and both file fingerprints. It moves the
blocking file to `.workphone/asset-operations/<operation ID>/preserved-conflict`
using an exclusive rename, then restores the journaled file. The history row
shows the holding file after recovery. If journal publication fails, the two
files remain reachable and the pending record can be retried.

Preservation supports two ordinary files of at most 64 MiB each. A matching
duplicate, a changed catalog binding, an occupied holding path, directories and
redirected paths require manual inspection. The workflow keeps all existing
copies in those cases. Copy recovery is available through the selected retry.

Pages and actions are bound to the current catalog session. Switching projects,
closing/reopening the database or destroying the previous catalog expires the
selection. Read-only media mounts cannot enable restore. Journal reads do not
create operation tables in catalogs that have never performed an operation.
Malformed journal records fail the page without returning partial rows.

Automatic retention, purge/garbage collection, chronological audit timestamps,
full-project reference indexing and conflict handling for remaining cases stay
open. Required saved-document users now guard delete and copy undo. See
[reference-aware operation coverage](WPRESOURCE_ASSET_REFERENCES.md). The UI has compiled and the underlying journal/session/undo contracts
have passed for the earlier inspection and undo workflow. Native validation of
the new selected recovery contracts and interactive acceptance are pending.

Creation undo now journals a guarded quarantine operation; creation redo restores
its retained bytes and original catalog UUIDs. The command expires on project
session changes. Project Clean retains authored metadata/lighting/imported scenes
and performs catalog maintenance; it does not purge asset generations.
