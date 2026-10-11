# Saved-document reference graph

Scene, material and prefab saves now record typed asset references in the project catalog.
The separate bundled-media catalog stays read-only; project documents own their
links to media UUIDs. The graph has its own version-1 schema, separate from the
catalog identity schema and compilation database.

Each edge stores its source document path, property location, target UUID,
project/media domain, declared type and required/optional policy. Property
locations use child indexes within the captured serialized tree. Resource metadata
supplies types; `scriptAssetUuid` is also recognized for compatibility. Actor and
component instance links are excluded from this asset graph. References default
to required; `referenceRequired=false` explicitly marks an optional link.

`saveAssetDocument` stages complete output through an exclusive sibling temporary
file, checks writes, flushes and closes it, then replaces the destination on the
same volume. `GameScene::saveScene`, material direct/manager/save routes,
`GamePrefabManager` actor/resource saves and `GamePrefab::save` use this service.
Parent directories are created and the installed path is returned only on success.
Mounted bundled media and private catalog storage cannot be overwritten.
Relative saves must stay within the captured project root; explicit absolute
exports can target a location outside it. Configured cache
scratch and exports outside the project do not own project graph records. Scene
USD saves fail explicitly because that serializer is still unimplemented.

For indexed documents, staging precedes a committed pending graph revision. A
catalog write transaction fences the pending revision across replacement and
installed-byte validation. Failed graph preparation leaves the old file and graph
intact. Failed replacement retains the old complete file and a pending graph;
failed completion retains the new complete file and a pending graph. Re-save or
reindex repairs the latter states. Superseded or expired sessions cannot complete.
Normal completion/failure attempts to remove owned temporary files; the focused
contracts verify cleanup for the exercised failure cases.

This is not a distributed filesystem/SQLite transaction or a certified power-loss
protocol. There is no retained prior-document history. A process crash can leave
an orphan sibling temporary file; automatic cleanup/replay and generation
retention remain open. Filesystem operations can race external writers, and
immutable authored UUID/revision ownership is not implemented. The fingerprint
is byte count plus FNV-1a for accidental-change detection, not authentication.

JSON serialization now retains property type, read-only state and attributes in
`__wp_properties_v1`, alongside the existing flat values and children. XML retains
attributes as `Attribute` children and marks each Properties group with
`PropertyMetadataVersion=1`. Binary Properties already retains this metadata.
The JSON reader still accepts legacy flat documents. Versioned JSON also preserves
ordinary fields named `name` and `value`. Reserved structural field-name collisions
fail serialization. Material texture slots persist UUID in `texture` and a separate
`texturePath` fallback; the previous shared key overwrote the UUID with the path.
Loading still accepts legacy path-only texture fields.

The Asset Database has **Reindex selected document references** for project files.
The service reads typed JSON, versioned XML or portable binary without constructing
resources or rewriting source bytes. It validates the captured bytes again and
replaces the graph in one transaction; failures preserve the previous graph.
Active-project reindex retains the mounted media domain and excludes cache scratch.
Unversioned JSON/XML must be opened and resaved through the owning serializer;
the service refuses to guess missing resource semantics. Malformed/duplicate JSON
keys, duplicate XML properties/attributes, unknown XML versions, unsupported
formats, encoded NULs, external/private paths and over-budget documents fail.
This is per-document repair, not a full-project discovery or legacy migration tool.

Select a project or bundled asset in the Editor's Asset Database window to see
its indexed saved-document users, required/optional policy, type and graph state.
Project references diagnose missing identities and type mismatches without
constructing resources. The UI shows up to 100 users and reports additional pages.
The service exposes forward/reverse keyset queries with up to 256 edges per page;
continuations expire on catalog/reference revisions or a changed query/session.

Journaled delete refuses required users in surviving indexed documents, reporting
the source path and property location. It rechecks before staging and again before
catalog commit. Pending, missing or externally changed indexed documents block
deletion because their previous graph cannot prove the absence of new users.
Deleting a folder can quarantine internal users and their targets together.
Quarantined edges leave the active graph; restore reactivates them. Document moves
and move undo relocate graph ownership transactionally. Copying an indexed authored
document requires a remapper. Copy undo refuses to remove a newly referenced copy.
Quarantined graph identities cannot be reused by a new save before restoration.
The legacy catalog removal and clear methods also check required references.

Undoing Editor asset creation also uses guarded quarantine. Redo restores the
retained file bytes and catalog UUIDs, and reopening/switching the project expires
the creation command. Project Clean retains authored .meta/.lighting/.unity files;
it maintains the catalog and refreshes the filesystem view. Derived-artifact
purging remains deferred until generation retention and live-pin policy exist.
Creation itself still needs exclusive file publication and complete domain
serialization/adoption before that workflow can be certified.

Capture limits are 64 MiB per document, depth 64, 10,000 nodes/edges and 100,000
properties. Delete validation bounds surviving document checks to 10,000 records
and 256 MiB of file reads. Exceeding a limit fails without staging a delete.
Validation currently runs synchronously; asynchronous scale work remains open.
Reindex text parsing additionally bounds structural depth to 128 and tokens/
elements/attributes to 300,000 before recursive conversion. All mutations use bound SQL and the connection-wide transaction guard. Unknown or
incomplete graph schemas fail without migration or partial query publication.

Coverage is deliberately explicit: only indexed saved documents are represented.
Legacy text documents must be saved through this path; remaining domain authoring
routes still need adoption. Unsaved edits, raw SQL mutations and unindexed files
are not a complete project reference inventory. Graph ownership currently follows
canonical document paths; immutable authored AssetUUID metadata migration remains
open. Compile/install/platform dependency modes, cycle policy, affected rebuild,
full-project reindexing and package closure are separate remaining work.

Focused contracts cover save fingerprints, pending/superseded sessions, graph
transaction failure, future schemas, optional/missing references, bounded queries,
required deletion, source moves, folder quarantine/restore, copy-undo conflicts,
media-domain separation, actual empty-scene JSON/XML/binary saves, creation
undo/redo and non-destructive project cleaning. Authored-document contracts also
exercise locked destinations, pre/post-publication SQL faults, source-preserving
reindex repair, property-metadata round trips and actual material/prefab saves. They do not
establish interactive populated-scene save/reopen or full-project coverage.
