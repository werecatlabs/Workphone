# Windows DDS texture profiles

The existing GraphicsResourceCompiler cooks texres descriptors for six explicit
Windows DX11 targets. Select the same target on ResourceSystemConfig, the cooked
material consumer and RuntimeResourceConfig. Existing arbitrary portable targets
continue to use the BGRA mip payload.

| Target | GPU format | Alpha |
| --- | --- | --- |
| windows-dx11-dds-bgra8-v1 | B8G8R8A8_UNORM | Straight |
| windows-dx11-dds-bgra8-srgb-v1 | B8G8R8A8_UNORM_SRGB | Straight |
| windows-dx11-dds-bc1-v1 | BC1_UNORM | Opaque |
| windows-dx11-dds-bc1-srgb-v1 | BC1_UNORM_SRGB | Opaque |
| windows-dx11-dds-bc3-v1 | BC3_UNORM | Straight |
| windows-dx11-dds-bc3-srgb-v1 | BC3_UNORM_SRGB | Straight |

Texture descriptors retain their version-1 fields. A target can override only the
settings it needs to change.

```json
{
  "version": 1,
  "source": "data://textures/paint.png",
  "mipFilter": "colour",
  "atlasColumns": 1,
  "alphaCutoff": 0.5,
  "targetOverrides": {
    "pc": { "compression": "bc3", "maxTextureSize": 2048 },
    "windows-dx11-dds-bc3-srgb-v1": { "mipFilter": "cutout", "alphaCutoff": 0.45 }
  }
}
```

`targetOverrides` accepts at most 16 exact target names with up to 64 lowercase
ASCII letters, digits, periods, underscores or hyphens each. Each entry can set
`mipFilter`, `atlasColumns`, `alphaCutoff`, `compression` or `maxTextureSize`.
An omitted field inherits its descriptor default. A target override can set
`compression` to `inherit` or `maxTextureSize` to `0` to use the existing
target/project policy even when the descriptor supplies an explicit default.
The compiler rejects unknown or duplicate fields. The source path stays at the
descriptor root and is shared by every target.

The selected target is part of the existing compilation variant and source
fingerprint. The descriptor, including target overrides, is a captured input to
the BuildKey. Profiles carry a version suffix which must change when encoder
output or options change. Readers reject incompatible compiled versions explicitly.
Asset UUIDs and authored descriptor paths stay stable when profiles change.

The compiler first uses the existing bounded source decoding and semantic mip
generator, then encodes native DDS bytes. SRGB profiles accept colour, cutout or
none mip semantics. Compressed normal maps are rejected; use BGRA8 until an
appropriate normal-map compression profile is implemented. BC1 rejects any
nonopaque alpha, including generated mip alpha. BC3 retains alpha. BC profiles
require base dimensions divisible by four for DX11; BGRA8 supports odd dimensions.
An explicit maximum size or project texture quality policy can discard top mips.
Block mip tails retain their minimum one-block storage.

The first-party encoder v1 uses integer bounding colour endpoints and nearest
palette selection, with deterministic edge replication and alpha interpolation.
It introduces no external encoder dependency or license. Image quality and
throughput still require acceptance against production art. BC7 encoding requires
a registered provider. The built-in encoder covers the six profiles above.
BC5/BC6H, arrays, cubes, volumes and channel packing remain outside this workflow.

Authored .dds sources can pass through byte-for-byte when their DX10 header,
format, alpha mode and exact mip chain agree with the target and descriptor.
Passthrough does not regenerate mips or reinterpret semantic settings. Legacy
DXT headers, mismatched formats and unsupported topology are rejected. Source
files retain the compiler's 64 MiB encoded-input limit; cooked texture payloads
retain the 256 MiB allocation budget.

DDS bytes are embedded in the versioned WPDX typed texture payload inside the
normal immutable resource artifact. There is no loose sidecar which can escape
the manifest's generation pins. Validation checks magic, header sizes, DX10
resource dimension, single array element, supported DXGI format, flags, pitch,
dimensions, mip count, aggregate limits and exact payload length. Profile and
descriptor validation runs again before native installation. See Microsoft's
[DDS programming guide](https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide)
for the container layout; supported input is deliberately narrower than DDS.

Native installation checks device format support and supplies exact encoded
subresources to an immutable DX11 texture. SRGB formats receive SRGB views. Failed
validation/device creation preserves the previous texture. Retained encoded bytes
support unload/load and device reupload without source decoding. Recook after
changing descriptor settings to publish a new artifact.

## Editor target preview

Select a `.texres` file in the Windows Claw/DX11 Editor Assets panel. The inspector
shows the descriptor defaults and the saved settings for the selected target.
Choose `pc`, one of the six DDS targets, or a custom package target. Edit the mip
semantic, atlas columns, alpha cutoff, compression and maximum size for that
target.

**Build preview** runs the texture cooker on the draft in the shared background
worker pool. The result shows the actual format, dimensions, mip count, encoded
bytes and estimated resident bytes. Choose a mip and view colour, an individual
channel or compression error. The error view compares sampled channel values
against the corresponding uncompressed mip and reports mean absolute error on a
0–255 scale. Results are checked against the selected project, descriptor and
settings before display.

**Save target override** writes the changed fields for the selected target.
**Clear target override** returns that target to the descriptor defaults.
**Reopen descriptor** reloads a file edited outside the inspector. Save checks the
original document and replaces it atomically. Other targets, the source and the
asset identity are preserved. Previewing leaves the authored descriptor and cooked
cache untouched.

Preview work is limited to 16 MiB each of encoded and decoded input and a display
sample of at most 256 × 256 pixels. The image preview covers 2D BGRA8, BC1 and BC3.
An authored DDS can show its encoded channels but has no original image for error
comparison. An authored BC7 DDS can report metadata. External encoding providers,
cube-face selection, channel packing and bulk profile editing need separate
authoring and acceptance work. Interactive Editor acceptance is still pending.

The cooked-resource contract tests exercise all six profiles through the actual
catalog/compiler/material path. They validate deterministic output, malformed DDS,
odd-dimension block accounting, mip tails, opaque-alpha rejection, exact GPU mip
readback, linear compressed rendering, last-good replacement, source-absent retained
reupload and manifest loading without source images or compiler services. The
tested device was Microsoft Basic Render Driver. This is not representative GPU,
image-quality, performance or relocated read-only shipping-package certification.

Run the restored regression gate after building its targets:

```powershell
ctest --test-dir project_x64 -C RelWithDebInfo -R 'WorkphoneAssets.catalog_contracts|WPGraphics.cooked_material_resource|WPLua.runtime_contracts|^WPResourceTests$|WPResource.writer_lease|WPResource.index_migration' --output-on-failure
```
