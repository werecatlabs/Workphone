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

Texture descriptors retain their version-1 fields:

```json
{"version":1,"source":"data://textures/paint.png","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5}
```

The selected target is part of the existing compilation variant and source
fingerprint. Profiles carry a version suffix which must change when encoder
output or options change. Graphics compiler versions are now 2 for texres/matres;
previous compiled version-1 artifacts must be recooked. Readers reject incompatible
versions explicitly. Asset UUIDs and authored descriptor paths do not change.

The compiler first uses the existing bounded source decoding and semantic mip
generator, then encodes native DDS bytes. SRGB profiles accept colour, cutout or
none mip semantics. Compressed normal maps are rejected; use BGRA8 until an
appropriate normal-map compression profile is implemented. BC1 rejects any
nonopaque alpha, including generated mip alpha. BC3 retains alpha. BC profiles
require base dimensions divisible by four for DX11; BGRA8 supports odd dimensions.
No implicit resizing or alpha loss occurs. Block mip tails retain their minimum
one-block storage.

The first-party encoder v1 uses integer bounding colour endpoints and nearest
palette selection, with deterministic edge replication and alpha interpolation.
It introduces no external encoder dependency or license. This is a deterministic
baseline encoder; image quality and throughput have not been certified against
production art or competing encoders. BC5/BC6H/BC7, arrays, cubes, volumes, channel
packing, resize policies and quality tiers are unsupported. These require new
explicit profiles and acceptance evidence.

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
support unload/load and device reupload without source decoding. Cooked DDS mip
settings require descriptor reimport. The existing Editor property panel shows
read-only cooked profile/container, DXGI format, mip count and encoded/resident
byte estimates. A target chooser, per-target authored overrides, channel/face
preview and compression-error UI remain open.

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
