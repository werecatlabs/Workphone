# Procedural city surface textures

Six CC0 surface sets from [Poly Haven](https://polyhaven.com), containing 24
unmodified 1024 x 1024 texture maps. Encoded images total **37,168,076 bytes**
(37.17 MB). The runtime can select a smaller decoded texture size independently
of these source images.

| Use | Official asset | Tile size in current API | Authors |
| --- | --- | --- | --- |
| Road asphalt | [Clean Asphalt](https://polyhaven.com/a/clean_asphalt) | 2.1 x 2.1 m | Dimitrios Savva |
| Sidewalk slabs | [Concrete Pavement](https://polyhaven.com/a/concrete_pavement) | 1.8 x 1.8 m | Charlotte Baglioni |
| Brick facade | [Brick Wall 001](https://polyhaven.com/a/brick_wall_001) | 3 x 3 m | Rob Tuytel, Dimitrios Savva |
| Fine plaster facade | [White Plaster 02](https://polyhaven.com/a/white_plaster_02) | 1 x 1 m | Rob Tuytel |
| Concrete facade | [Concrete Wall 009](https://polyhaven.com/a/concrete_wall_009) | 1.805 x 1.805 m | Charlotte Baglioni |
| Grass ground | [Grass Ground](https://polyhaven.com/a/grass_ground) | 2.51 x 2.51 m | Charlotte Baglioni |

`city-surfaces.json` pins canonical download URLs, byte counts, upstream MD5,
local SHA-256, measured pixel dimensions, authors and acquisition time. Physical
tile sizes use the API's current `dimensions` field in millimeters, divided by
1000 and rounded to six decimal places. The original millimeter values are also
retained. See the [official API schema](https://github.com/Poly-Haven/Public-API/blob/master/swagger.yml).
Older free-form `scale` fields can disagree with current dimensions; for example,
Brick Wall 001 historically says 1.5 x 1.5. An artistic UV-scale override should
remain separate from this provenance metadata.

Workphone uses a 1.5 m tile for Brick Wall 001 to give the five brick pitches
across the image a plausible architectural size. Other surfaces use the sizes
above. The runtime preserves source colour, converts OpenGL normal green to the
engine convention, and filters colour in linear light, normals as vectors and
roughness by its fourth moment. Maps are shared by material family, with 512 px
defaults and 256 px city preview maps. Missing or invalid sets retain the
procedural fallback; no network request occurs during scene generation.

New terrain imports the converted grass albedo, normal and ORM maps into the
active project's `Assets/Landscape/CitySurfaces` using immutable, content-addressed
PNG/`.texres` assets. These canonical `data://` references survive saving and
terrain package export. Existing terrain appearance is preserved. Standalone
SampleVehicleAdvanced uses `.FBCache/ProceduralCity` beneath its working folder
when no project is open.

Each stem has `_diff_1k.jpg` (sRGB color), `_rough_1k.jpg` and `_ao_1k.jpg`
(linear scalar maps), and `_nor_gl_1k.png` (linear OpenGL +Y tangent normal).
Grass alone uses the provider's `_nor_gl_1k.jpg` to keep the complete pack below
40 MB. All surfaces are nonmetallic: metallic is constant zero. No displacement
or precombined channel maps are included. Brick's local `_diff` basename differs
from the provider's `_diffuse` basename; its image bytes are unchanged.

Restore or verify from the repository root on Windows:

```powershell
.\Tools\powershell\FetchCitySurfaceTextures.ps1
.\Tools\powershell\FetchCitySurfaceTextures.ps1 -VerifyOnly
```

Existing valid files are reused. Verification is offline. An invalid existing
file is retained unless `-Force` is supplied; replacements are staged and verified
before publication. `-DestinationDirectory` permits an isolated restore without
changing the shipped textures. The manifest stays pinned when upstream assets
change; a checksum mismatch requires an intentional provenance update.

The image files are [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/),
as stated by [Poly Haven's asset license](https://polyhaven.com/license). The full
legal text is in `LICENSE-CC0.txt`. This license applies to these downloaded
images, not to Workphone source code. Attribution is retained here as provenance;
Poly Haven does not endorse Workphone. The acquisition used the official API with
the identifying `WorkphoneCitySurfaceFetcher/1.0` user agent. The restore script
uses only the pinned asset URLs and does not depend on a live metadata API.
