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

Prepared or undoing records represent interrupted operations. They are inspected
here, but only committed records enable undo. Reopening the catalog runs the
existing interruption recovery service. Conflicts retain their content and journal
for explicit repair; the UI does not guess which conflicting copy should win.

Pages and actions are bound to the current catalog session. Switching projects,
closing/reopening the database or destroying the previous catalog expires the
selection. Read-only media mounts cannot enable restore. Journal reads do not
create operation tables in catalogs that have never performed an operation.
Malformed journal records fail the page without returning partial rows.

This adds inspection and guarded undo to the existing quarantine/recovery service.
Automatic retention, purge/garbage collection, chronological audit timestamps,
full-project reference indexing and an interactive conflict-resolution wizard
remain open. Required saved-document users now guard delete and copy undo; see
[reference-aware operation coverage](WPRESOURCE_ASSET_REFERENCES.md). The UI has compiled and the underlying journal/session/undo contracts
have passed; interactive restore/save/reopen acceptance has not been performed.

Creation undo now journals a guarded quarantine operation; creation redo restores
its retained bytes and original catalog UUIDs. The command expires on project
session changes. Project Clean retains authored metadata/lighting/imported scenes
and performs catalog maintenance; it does not purge asset generations.
