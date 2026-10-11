# Bundled media asset catalog

Bundled fonts, textures, materials, scripts, audio and mesh files belong to the
installation's media catalog. Project assets remain in the project's writable
catalog. The authored configured-actor/domain database remains AssetDatabase.db;
it is a different database with different schema ownership.

The default bundled catalog is bin/Media/.workphone/media-assets.db in this
checkout. It uses the normal catalog v2 identity schema plus ownership marker
wp_media_catalog and eligibility table wp_media_entries. Never point the builder
at the project catalog or AssetDatabase.db. Existing databases without the exact
media ownership marker are rejected without migration or replacement.

## Creation and rebuilding

Build the WorkphoneMediaCatalog target, then run:

```powershell
& .\bin\windows\v145\x64\MD\RelWithDebInfo\WorkphoneMediaCatalog.exe .\bin\Media
```

An optional second argument specifies a different database destination. Exit 0
means publication succeeded; exit 1 reports a build failure; exit 2 reports invalid
arguments. Prebuild this derived database when assembling an installation whose
media directory will be read-only. Editor startup creates it if missing and then
opens it with SQLite's OS-enforced read-only mode. An existing catalog is mounted
without rescanning; rerun the builder after installing, changing or removing media.
A failed mount is logged and project assets remain usable.

UUIDs are deterministic UUIDv5 values of the catalog's fixed namespace and
canonical media-relative path key. Rebuilds, payload edits and installation-root
relocation preserve identities. Renaming a media path changes its UUID; redirects
and migration of existing references are not implemented. Windows canonical keys
fold ASCII case under the existing catalog policy; cross-platform identity policy
still requires certification. Database row order is deterministic by canonical key.

The scanner recognizes .ttf/.otf/.font, common raster images and DDS/KTX/KTX2,
.mat/.material, .lua, common audio files and .fbmeshbin. This is an inventory, not
validation, import, cooking, GPU upload or playback certification for those
formats. Unsupported files are skipped. Private .workphone and .git trees, links
and Windows reparse points are skipped. Limits are 100000 recognized assets,
500000 visited entries and 64 directory levels. Inventory failure preserves the
previous committed catalog. New databases publish exclusively from a sibling
candidate; existing owned databases update in one SQLite transaction. Source
payloads are never written. Process-kill/power-loss certification remains open.

## Editor and resource references

Open Asset Database and choose Show bundled media to search, filter, page and
inspect its UUIDs. The catalog browser and resolver retain a separate media mount.
Project-relative path lookup continues to mean project assets; UUID lookup can
resolve media through its own source root. A UUID present in both catalogs is
rejected rather than silently shadowed. No media rows are imported into a project.

Resource reference pickers provide the same project/media switch. Compatible
selected assets resolve through their runtime interfaces before assignment.
Assignments preserve the UUID in the owning property, mark supported owners dirty
and support grouped undo/redo. Save the owning document to persist the edit. A
changed project session, changed media mount, removed target, stale initial
selection or independent property edit rejects the operation. Media source files
and catalog rows remain read-only. General scene/material save/reopen and live
Editor interaction remain acceptance gates.

## Packaging policy

Assets under Media/Runtime are explicitly marked runtime-eligible; every other
bundled asset defaults to editor-only. Eligibility is metadata, not evidence that
an asset is cooked or included in a game package. Shipping must include only the
referenced eligible closure, reject Editor-only dependencies, map UUIDs into the
source-free runtime manifest and validate applicable licenses. That package
integration is still open; do not copy the entire Editor media tree into a game.
