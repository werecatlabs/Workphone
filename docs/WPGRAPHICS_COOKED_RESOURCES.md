# Cooked graphics resources (format version 1)

This increment provides a synchronous catalog-to-DX11 material consumer on the
existing ResourceSystem. It is available with `WP_BUILD_RESOURCE_SYSTEM`; the DX11
consumer also requires Windows and `WP_BUILD_RENDERER_DX11`. It does not replace
the Editor's legacy import/preview path.

## Authoring

Register `workphone::render::GraphicsResourceCompiler` in the existing resource
compiler registry. Its outputs are `texres` and `matres`, compiler version 1.
Descriptors are strict UTF-8 JSON objects, at most 64 KiB. All fields shown below
are required; unknown and duplicate fields are rejected.

`colour.texres`:

```json
{"version":1,"source":"data://colour.bmp","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5}
```

`surface.matres`:

```json
{"version":1,"baseColour":[1,1,1,1],"metalness":0,"roughness":1,"baseColourTexture":"data://colour.texres"}
```

Paths belong to the bound source root. The texture source is a compile dependency;
the material's texture is both a compile and install dependency. Subresources are
rejected. This first material format supplies one base-colour texture and scalar
metalness/roughness, with dielectric F0 fixed at 0.04; other material channels are
future format work.

Mip filters are `none`, `colour`, `data`, `normal`, `cutout`, or `roughness`, using
the existing semantic mip generator. Atlas columns must divide the base width;
the chain ends before a level would mix columns. Alpha cutoff is strictly between
0 and 1. Material numbers must be finite and within [0,1].

Encoded images are limited to 64 MiB. They must support FreeImage header inspection
without pixel allocation and be integer colour images of at most 32 bits per pixel. Dimensions are limited to
16384 per axis; the full BGRA mip chain is limited to 256 MiB. This is a per-asset
bound, not a runtime aggregate residency budget. The compiler inspects dimensions
before decoding, generates mips offline, and stores top-down BGRA bytes.

## Install and publication

Create `ClawMaterialResource` on the render owner thread, passing the catalog,
initialized ResourceSystem, its exact source root, explicit catalog-type mappings,
active ClawRendererDX11, expected target and packaged-build mode. Recreate this
owner if the ResourceSystem configuration changes. Application graphics, state,
factory and timer services must be initialized.

`reload(uuid, error)` resolves the catalog entry, compiles its closure, loads and
validates the typed graph, stages new material state and uploads the exact cooked
mips, then publishes the complete bundle. `stage` and `publish` expose those two
steps separately for controlled installation. Neither step falls back to raw
source texture loading. Compilation is synchronous authoring work; this API is
not an asynchronous runtime streaming scheduler.

Retain `current()` through the draw, bind its `material()` to the ClawMesh, and
call `ClawRendererDX11::renderMesh`. Treat published objects as immutable. All
owner methods and the final release of candidate/current handles belong on the
constructing render thread. `unload(error)` clears the owner and invalidates
pending candidates; callers must detach their mesh/material references when
unloading. Already retained handles pin their resources until released.

Material payloads pin the exact texture ResourceID, source hash, payload hash and
compiler version. The decoder validates the dependency before installation and
checks target/build mode consistency. A retained DX11 device pins device identity
through renderer recreation. Publication rejects obsolete request tickets,
another publisher's candidates, unavailable/replaced devices and stale catalog
snapshots (including rename, delete, close/reopen or project switch).

Compilation, decoding, GPU upload and material construction occur outside the
catalog lock. The final snapshot check and no-throw bundle pointer swap share the
catalog's recursive lock; the retired handle is released after unlocking. Failure
leaves the previous bundle untouched. Catalog changes do not automatically clear
an already retained working bundle; an explicit consumer unload releases it.

## Container and remaining scope

The existing CompiledResourceIO container owns identity, type, compiler version,
hashes and dependency records. Typed payload version 1 uses little-endian integer
fields and IEEE-754 float32 values, with length-prefixed strings and mip data.
Texture/material magic values are `WPTX`/`WPMT`; typed decoders reject incompatible
versions, invalid dimensions/scalars, inconsistent lengths, trailing bytes,
integrity failures and mismatched dependencies. FNV hashes identify incremental
build content; they are not authentication.

Use separate compiled roots and compilation databases for each target/build mode.
Embedding the target/mode permits rejection of a wrong artifact; it does not fix
the ResourceSystem's shared variant namespace. Its output-before-metadata-commit
failure window also remains open: rejected compilation can leave changed files
on disk while the consumer retains the previous GPU bundle. This increment does
not certify cross-process writers, crash recovery, source changes during a cook,
automatic watcher/reload integration, source-free read-only packages, imported
mesh/animation assets or aggregate memory budgets. Invalid mip chains and missing
upload devices are exercised; injected driver allocation failure and crash
recovery are not certified by these tests.

`WPGraphics.cooked_material_resource` exercises the real catalog/compiler/loader
and DX11 draw with programmatically generated tiny image fixtures. The existing
production-render test also checks exact cooked mip upload and scene transform
regressions. Run the baseline build/validation scripts for both Debug and
RelWithDebInfo; consult the implementation status for actual results.
