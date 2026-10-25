# WPProcedural implementation status

## Generated cities with editable terrain roads — 25 October 2026

`ProceduralRaceScene` now promotes grid and L-system streets into the existing
`CityRoadAuthoring` source when **City Terrain Roads** is enabled. The source and
terrain are persistent scene anchors; generated render/collision actors are
disposable and excluded from scene serialization. Regeneration preserves manual
road edits and terrain rebinding. Application updates rebuild derived city data
after source revisions, explicit road rebuilds and terrain snapshot/placement
changes. Failed or missing bindings report an error and remove stale derived
collision. The scene cannot silently return to a second flat ground world.

The converter assigns deterministic IDs, merges collinear chains, selects bounded
roundabouts and fits the original lap through their directed circulation routes.
Edited sources must still support that lap continuously, including terrain contact
heights and island exclusion. Unsupported laps become unavailable in native and
Lua racing flows. They are not replaced by an invented route.

Buildings use exact full-parcel terrain-cell coverage and road/junction clearance.
Holes, excessive relief or unsupported parcels are culled; retained foundations,
building collision, parks and existing foliage placement use the same fitted
heights. Source roads own generated asphalt, kerb, turf and paint materials with
semantic texture/mip bindings. Flat street overlays are omitted in terrain mode.

The first fresh street capture exposed missing pedestrian edges and grass-only
splitter tops. The next pass adds concrete splitter rims and fitted 2.4 m footways
beside streets, approaches and roundabout edges. Footways sit 15 cm above terrain,
retain exact cell ridges/holes, omit crossing mouths and unsupported pieces, and
share their rendered top/kerb geometry with one concave collision mesh. Spatial
admission and batched fitting bound candidate, vertex and validation work.

Native iteration also found a redundant startup regeneration: terrain integration
serial notifications advanced the authored source revision. Serial-only updates
now acknowledge the unchanged snapshot/pose; actual terrain recooks preserve
the authored token. Focused tests cover both paths and source detach/reattach.

The native sample and capture script accept `--city-terrain-roads` and
`-CityTerrainRoads`. Evidence checks actual source/terrain revisions, fitted lots,
native surface rays, driveable route coverage and uploaded road material bindings.
Collision smoke additionally exercises manual edit retention, regeneration,
terrain rebinding and a same-source-revision terrain refresh. That stronger
repeated-generation retention gate has not passed within its 180-second limit.

`RacingGameOpenCity` now persists/forwards the terrain mode and canonical source
UUID. Quality/seed/source changes reuse the native vehicle/camera/component in
terrain mode; recreation from serialized script properties rebinds the same
document. Flat mode retains its existing lifecycle. Terrain-city lap records are
session-local until a stable edited-route fingerprint is exposed; regeneration
and detected route changes reset them and the UI states the policy. The real Lua city/component suites pass with fixture
checks for retained knot IDs/widths, cache invalidation and source restoration.

The retained build-21 baseline built the Debug sample, Editor and focused
targets. Its city/Editor CTest run passed 12/12 in 138.56 seconds, including all
seed/size source profiles, maximum footway profiles, exact support/holes/grade,
eight connected roundabout pedestrian
corridors, oblique seams, allocation rollback, source revisions and save/reopen.
The existing production-render GPU suite passed in 26.74 seconds. Both real Lua
city/component fixture runs passed, including 50 component scripts. These Editor
fixtures prove native controls and saved anchors; recorded mouse/DPI interaction
and a complete restored driving scene remain separate acceptance gates.

The preserved build-21 native baseline uses six-block cities, high quality,
foliage, 1280x720, braked replay and the actual DX11 HDR path:

| Capture | Roads / roundabouts | Supported parcels / building masses | Result |
|---|---|---|---|
| [Grid street and overview](../artifacts/procedural/city-terrain-20261025-04/manifest.json) | 22 / 4 | 144 / 216 | Both native validations and clean process exits pass |
| [Natural pedestrian view](../artifacts/procedural/city-terrain-natural-20261025-05/manifest.json) | 70 / 4 | 119 / 143; 3 parcels culled | Native validation and clean process exit pass |

All three views verify four native road/ring/island/terrain rays, a separate
native hit on the single fitted footway collider, island exclusion, continuous
driveable route heights and 16 uploaded road PBR textures. Footway height errors
are at most 0.00000012 m. Source, active and applied road revisions agree. The
maximum-city corpus is CPU geometry/admission evidence; these native runs use
six blocks.

Fresh captures exposed and fixed a real renderer lifetime defect: the sky cache
held a legacy weak pointer that dereferenced freed sky storage during renderer
shutdown. It now uses the existing independent `ObjectObserver` lifetime token
and retires expired entries. The GPU regression physically frees six sky owners
across three renderer lifecycles before cache destruction. The failed capture
and exception stack remain in `city-terrain-20261025-03`. A separate hot cleanup
stack exposed global update-list sorting on every actor removal; stable erasure
now preserves survivor order, with a regression in the save/reopen fixture.

The images are visibly improved at road/kerb contact but remain prototype art:
repeated opaque glazing/massing, sparse ground content, limited street props,
simple lighting and visible aliasing still fail commercial visual acceptance.
The adapter is Microsoft Basic Render Driver/WARP. Grid street/overview mean/P95
are 207.61/234.37 ms and 258.42/442.61 ms; natural pedestrian mean/P95 are
598.16/977.98 ms, over 29 measured frames each. These short software-renderer
runs, including concurrent workspace activity, do not certify hardware speed.

Capture 04 records the verified build and source/binary hashes. Capture 05 uses
the same binary hashes; concurrent material/Lua work subsequently replaced shared
Debug DLLs. The later retention runs use a separately frozen binary set, not the
04/05 baseline, and do not certify unrelated concurrent changes.

| Retention evidence | Observed outcome |
|---|---|
| [06: original smoke](../artifacts/procedural/city-terrain-retention-20261025-06/manifest.json) | Timed out at 180.37 s; the original timing failure remains open |
| [07: extended diagnostic](../artifacts/procedural/city-terrain-retention-20261025-07-diagnostic/manifest.json) | Same frozen binaries/fixture, separate 600 s limit; access violation `0xc0000005` after 259.52 s |
| [08: CDB diagnostic](../artifacts/procedural/city-terrain-retention-20261025-08-debugger/outcome.json) | Same frozen binaries under launch-time debugging; fourth generation still progressing at the 600 s timeout, no reproduced exception stack or functional pass |

For 07, Windows Error Reporting identifies `Workphone.dll+0x14511c8`; following
the frozen DLL's export thunks resolves it to `FSM::getAutoChangeState+0x18`.
Source inspection found an FSM cached pointer into manager flag storage that can
move when the manager grows. The fix resolves the current slot through locked
manager access and prevents unloaded FSMs/deferred references from touching a
retired slot. The Editor regression forces storage relocation and checks live
flag values plus retired-queue behaviour. This diagnosis uses the recorded fault
address and source evidence; 08 did not capture a matching exception stack.

The FSM fix and a separate parcel exclusion correction reserving the full
2.45 m roadside pavement extent are awaiting the final combined test/native
validation. New parcel/cull counts must come from those captures. Build 22 has
completed, but subsequent focused rebuilds and the final city/Editor/FSM/GPU
matrix remain in progress. CDB launch may perturb allocation costs; 08 is not
performance evidence, and neither extended run closes the original 180-second gate.
Hardware performance, recorded Editor pointer/DPI acceptance and source-free
reconstruction remain open.

Remaining gates include ordinary junction reconstruction, automatic grading,
arbitrary edited-lap topology, transport/water integration, representative GPU
performance and independent commercial-quality visual comparison.

## Racing foundations and visual slice — 12 October 2026

This implements parts of P0–P5, P8 and P9 from the [production plan](WPPROCEDURAL_PRODUCTION_PLAN.md). It does not complete P0–P10 or certify AAA quality. Changes reuse the existing procedural services, racing component, terrain foliage attachment and resource compiler.

### Runtime and authoring changes

- `MeshGeneratorDefault` now generates terrain and road meshes, supports the corresponding object adapters, retains validated settings and publishes progress/history. Budgets, malformed dimensions, nonfinite data and unsupported options fail explicitly. Failed requests retain the previous generated mesh. LOD generation, simplification, tangents, arbitrary object conversion and collision generation remain unsupported by this adapter.
- `ProceduralMesh` now computes retained bounds, validates geometry indices/finite positions, preserves UVs and masks during append, and handles self-append. Invalid append candidates do not alter geometry. Public direct vertex edits still require explicit bounds recomputation.
- Fixed vehicle sidepod loft indices that referenced the central body instead of the appended pod vertices. Added closed pods, an open cockpit, interior/rails, raised airbox, cambered wings, pylons/endplates, mirrors and diffuser details. Wheel/axle physics metadata remains shared with the existing vehicle generator.
- Circuit and city reuse generated asphalt maps; city facades have generated concrete maps, roof/parapet/plinth modules and roof service details. Pit garages have an apron, roof caps, fascia, glazing, mullions and door slats. This is an incremental procedural art pass, not a production architectural kit.
- Optional `Foliage World` selection uses the existing terrain-attached foliage stream and Claw renderer: four seeded species with real reduced mesh LODs, a seeded 256px leaf-spray atlas with individual leaves/twigs/alpha gaps, alpha-cutout mips, wind, circuit exclusions/city-park placement and trunk colliders. Legacy patch trees remain available. Scene teardown clears/unregisters the foliage attachment before destroying generated textures. Runtime texture resolution now finds generated manager-owned atlases.
- Native and Lua samples share the component's foliage selection. Lua saves it through the sample's properties. Native quality names now include `cinematic`, matching the existing four-value quality enum; the name itself makes no visual/performance guarantee.
- Fixed graphics resource property updates clearing resource names: partial edits now retain the name, and explicit name edits apply correctly. The generated foliage atlas uses its registered identity and verifies the species retains its cutout binding. This repair was prompted by actual captures showing opaque white leaf cards despite healthy paging diagnostics.
- Shared CPU image encoders produce real PNG/TGA/PAM with straight alpha and opaque-only PPM. Unsupported formats and malformed sizes fail; colour/data values are not implicitly gamma-converted. Asphalt source export writes six TGA maps with semantic `.texres` descriptors, a current-tier `.matres` and a completion manifest into a fresh directory. Existing output directories are refused.
- Texture Editor supported chains now execute in stable creation order and publish only successful complete candidates. Versioned JSON recipe save/load validates detached plain data and retains the previous generated texture until a replacement succeeds. This is a linear supported chain, not a general texture DAG.

### Review harness

Native sample additions:

| Option | Behavior |
|---|---|
| `--evidence PATH.json` | Writes actual adapter/driver, capture-target dimensions, requested settings, completed native frame diagnostics, render timings/counters and structural validation result |
| `--replay PATH.csv` | Applies one control row per physics-task tick, in contiguous tick order, then holds the brakes at end |
| `--foliage-world` | Selects paged instanced vegetation instead of legacy merged tree patches |
| `--export-materials NEW_DIR` | Writes the generated asphalt source set; parent directory must already exist |

Replay header is `tick,throttle,brake,steering`. Ticks begin at zero with no gaps. Throttle/brake are in [0,1], steering in [-1,1]; values must be finite. The physics task targets 120 Hz, but existing physics uses measured/clamped deltas: input replay is not a fixed-step deterministic physics implementation or a cross-platform replay guarantee. Inputs start when the post-load physics configuration is ready. Timing, reset and camera-track recording remain future work.

`Tools/CaptureProceduralRacing.ps1` runs selected views into a fresh output directory and saves source/diff/binary/media/plugin hashes, exact arguments, logs, output image hashes and runtime reports. It defaults to silent audio and accepts `-FoliageWorld`, `-ExportMaterials`, `-Replay`, `-Views`, `-Seconds`, `-Resolution` and `-Configuration`. Build separately before capture. Asset hashes are a media inventory, not a runtime dependency trace. The script rejects reused output directories and reports capture/manifest/timeout failures.

```powershell
& 'C:\Program Files\CMake\bin\cmake.exe' -S . -B project_x64 -DWP_BUILD_PROCEDURAL_SERVICE_TESTS=ON
& 'C:\Program Files\CMake\bin\cmake.exe' --build project_x64 --config Debug --target SampleVehicleAdvanced ProceduralServiceTests VehicleReviewTests WPGraphicsCookedResourceTests
& .\Tools\CaptureProceduralRacing.ps1 -Configuration Debug -Views car,corner -FoliageWorld -ExportMaterials
```

### Evidence and remaining gates

Debug sample and focused native targets build. Six focused CTest checks passed after rebuilding the graphics-resource-name fix: procedural services/geometry/component properties (including the name regression), replay/evidence contracts, Lua texture recipes, native texture encoders, real vehicle Lua presentation bindings and cooked material/resource GPU contracts. Standalone Release export/recipe checks passed 2/2; independent Pillow decoding confirmed PNG/TGA/PAM pixels and alpha. The native generated asphalt test exercised six semantic map cooks, exact uploaded albedo mip bytes, a base-colour-only DX11 draw, failed-rebake retention and source-free package closure/reupload. Structural validation reports do not establish image quality or the performance gate.

The earlier capture set at `artifacts/procedural/implementation-baseline-02` passed capture/native-frame/winding/LOD checks on Microsoft Basic Render Driver/WARP. Car/corner mean intervals were 192.541/207.309 ms with P95 216.277/218.981 ms; the runs contain only 29 measured renderer frames each. That set predates the completed hero geometry and optional foliage path. It fails the proposed frame target and remains visibly prototype quality. It is not hardware certification.

`artifacts/procedural/implementation-final` contains successful car/corner captures with repaired hero geometry, working leaf cutout binding, paged foliage, source material export and replay input. Both reports pass native frame, winding, vehicle LOD and foliage readiness checks. Car/corner mean intervals are 132.443/117.185 ms and P95 143.434/128.823 ms over 29 measured renderer frames each, on the same software adapter. Replay consumed 58/120 rows in each run; this is input-application evidence, not proof of deterministic physics. Visual inspection confirmed the atlas repair but found oversized leaf silhouettes; the subsequent canopy capture tests the finer clustered atlas. These are short diagnostic runs and do not close hardware/performance or AAA visual gates.

The final `artifacts/procedural/implementation-canopy/car.bmp` capture was inspected after the clustered-atlas pass: the green leaf sprays and denser crowns render correctly, with no white opaque cards. Its report passes structural validation, with 44 resident foliage pages, 144 instances, no failed pages, and 58 replay ticks consumed. WARP mean/P95 are 128.794/141.021 ms over 29 measured frames. Capture runner exits successfully. The scene still fails the requested AAA visual target: lighting/terrain remain simple, branching/card repetition is apparent, ground treatment is sparse and vehicle surfaces/content need further art and renderer work. No commercial-reference comparison was performed.

### Cooked PBR material increment — 13 October 2026

The cooked material schema now accepts optional `normalTexture` and `ormTexture` references. Each dependency retains its exact source/payload/compiler generation; the decoder accepts legacy base-colour payloads and validates the extended semantic slots, unique install closure, target/build mode and linear Normal/Data filtering. GPU staging uploads and retains all three maps before publication. One ORM upload is shared across the renderer's explicit channel slots: red occlusion, green roughness and blue metalness. Generated asphalt source export now writes these bindings with unit material multipliers. The compiler version advances to invalidate stale material cooks.

The sample's sampler, texture-scale and vehicle material controls now use the existing explicit material-state setters. Their previous top-level property bags did not apply those editor-state fields. Clearcoat state can now be authored correctly, but its native shader support still requires separate inspection and verification.

The fresh Debug rebuild and both targeted CTest checks pass: `WPProcedural.services` and `WPGraphics.cooked_material_resource` (15.22 seconds for the GPU suite). The material test verifies exact GPU mip bytes for all three maps and real draw changes from normal, roughness(G), occlusion(R) and metalness(B). Invalid normal semantics and missing ORM sources retain the last visible material. The original base-colour compatibility checks remain passing. A stale test fixture expecting a zero metalness multiplier was corrected to match the exported unit multiplier.

`ClawMaterialResource::stageRuntime` shares the typed validation and GPU construction path with authoring, but publishes an immutable loaded generation without requiring a source catalog or compiler. The passing package contract constructs and draws a fresh three-map GPU material while the source directory is hidden, rather than relying only on the previously retained authoring material. It also rejects missing runtime resources and mismatched package modes. This proves the material-level source-free GPU path; it does not certify a source-free racing scene package. No full P2 completion is claimed: Editor/swatch/sample equivalence and visual acceptance remain to be proven.

### Colour transfer repair — 13 October 2026

A real mid-grey draw regression confirmed double colour decoding: tinted BGRA UNORM produced RGB 66/66/66 while identical BGRA sRGB produced 37/37/38. The native PNTC shader now uses the bound SRV's actual format to distinguish hardware sRGB decoding from encoded UNORM samples. Encoded albedo/emission uses the piecewise sRGB transfer; hardware-decoded samples are used directly. Colour and vertex tint decode independently. Alpha, normal and ORM remain data. The draw owns the transfer flags, and unchanged material submission preserves them to avoid forcing repeated constant-buffer uploads.

The rebuilt `WPGraphics.cooked_material_resource` suite passes in 14.04 seconds, including the prior multimap/source-free checks. With a 0.6 colour tint and the fixture's controlled lighting:

| Draw | BGRA UNORM | BGRA sRGB | BC1/BC3 sRGB |
|---|---|---|---|
| Mid-grey albedo RGB | 65/65/65 | 65/65/65 | 66/66/67 |
| Mid-grey emission RGB | 97/97/98 | 97/97/98 | 100/99/100 |

The compressed profiles remain within the declared four-code-value tolerance. Emission testing also exposed `Material::setTexture` requiring an authoring resource database for an already-created texture. That path now permits direct texture assignment without a database and no longer loads an unused `panel.png` resource; the source-free fixture exercises it. This repair covers ordinary/instanced PNTC albedo and emission. Terrain layer colour, environment maps, HDR/exposure and final output transfer still need their own calibration and regression evidence.

The fresh `artifacts/procedural/colour-transfer-20261013/car.bmp` was inspected after rebuilding. Capture runner and structural validation pass, with 58 replay inputs applied. The short WARP run reports 143.956 ms mean and 159.949 ms P95 over 29 frames. Ordinary generated UNORM materials retain their appearance with the corrected transfer; the controlled DDS tests establish the sRGB fix. The scene remains prototype quality, with flat venue/terrain treatment and visibly repeated foliage. This is a colour correctness increment, not an AAA visual acceptance result.

### Terrain and environment colour — 13 October 2026

A new production-terrain draw fixture reproduced double decoding with nonwhite tint: the same mid-grey texture gave red 37 through UNORM and 28 through sRGB. Terrain staging now derives each albedo layer's decode flag from its actual view format, independently of normal strength. Both planar and triplanar paths decode colour/tint consistently. The analytic fixture also exposed two-sided raster state flipping the terrain's upward lighting normal into the ground; heightfield lighting now preserves the authored surface normal. Snow tint follows the same sRGB decode convention.

`ClawCubemap` accepts encoded RGBA/BGRA8 UNORM/sRGB faces and linear RGBA16F/32F faces, at the selected 2D SRV mip, up to 2048 pixels per dimension. Float radiance is preserved above one through the GGX roughness levels; encoded byte faces use the piecewise sRGB transfer. Unsupported views, foreign devices, nonfinite/negative source radiance and nonfinite filtered output fail before replacing the previous cube. Context/device identity is checked, including the cached-view path.

Fresh Debug targets build. `WPGraphics.cooked_material_resource` passes in 14.20 seconds, including matching planar/triplanar UNORM/sRGB draws with nonwhite tint and the expected lighting result. `WPGraphics.cubemap_pbr` passes after the selected-mip test in 10.76 seconds: orientation, filtering, caching, PBR draw, encoded sRGB equivalence, half/full-float radiance (2/.5/.125 at smooth and rough levels), selected source mip and invalid-source retention. The racing sample still generates 8-bit reflection source faces; this increment proves native HDR cubemap input, not an HDR racing environment or final exposure/lighting calibration.

The rebuilt `artifacts/procedural/terrain-environment-20261013/car.bmp` was inspected. Capture/structural checks pass; WARP mean/P95 are 151.517/176.166 ms over 29 frames. This view remains visibly prototype quality. The sample's foliage carrier is all-hole terrain and its meadow is a separate mesh, so the dedicated production-terrain GPU fixture supplies terrain lighting evidence. The sample's native reflection input still traverses `ClawTextureManager::createManual`/`copyData`, which currently assumes 8-bit storage and ignores the requested manual format. A typed float upload path is required before claiming HDR radiance reaches the racing reflection probe.

### HDR sample bridge — 13 October 2026

`ITexture::copyLinearFloatData` adds an explicit, non-pure backend capability for RGBA32F linear data; unsupported backends return a diagnostic. Claw validates dimensions, exact pixel count, finite/nonnegative RGB and alpha in [0,1] before staging a candidate. It retains float pixels for GPU reupload and unload/load, rejects lossy BGRA8 snapshots, and clears obsolete representations when switching storage. GPU allocation/upload failure retains the previous view. This path stages one float mip; the cubemap prefilter owns the probe's roughness levels. The existing manual-format argument remains a separate unsupported-format issue.

The shared racing builder now supplies linear float reflection faces and retains an unclipped solar core; the visible sky remains LDR. Capture evidence reads back the actual current-device RGBA32F probe, reporting dimensions, mip count, finite values and peak radiance. A capture requires radiance above one to pass its new HDR validation. This is a deliberately authored sun/sky model, not measured photometric calibration or an HDR display/output pipeline.

The first Debug build passed both GPU suites (cubemap/float texture 12.36 seconds; cooked material/terrain 14.17 seconds). Tests verify exact values up to 12, malformed-input retention and unload/load reupload; linear float terrain colour also matches the encoded UNORM/sRGB paths. The new interface method has since been moved to the end of the existing virtual texture methods to preserve their ordering. The coordinated Debug rebuild of native targets, Lua bindings and Editor succeeds. The final cubemap/float-texture suite passes in 12.70 seconds and the material/terrain suite in 14.33 seconds. An Editor build is not interactive workflow acceptance.

`artifacts/procedural/hdr-sample-20261013/car.bmp` was inspected; the capture runner and structural/HDR checks pass. GPU readback confirms a current-device 128×128 RGBA32F probe with eight mips, finite values and peak linear radiance 16.5446. This establishes the float source → prefilter → sample probe path above the former 8-bit clamp. WARP mean/P95 are 156.207/166.644 ms over 29 frames, not a hardware performance pass. The scene remains prototype quality: the new dynamic range does not close exposure, material response, foliage shape or venue composition gates.

The rebuilt sample capture at `artifacts/procedural/pbr-runtime-20261013/car.bmp` was inspected. Native frame, geometry/LOD and foliage checks pass; the report records 58 applied replay ticks and no failed foliage pages. Mean/P95 intervals are 141.912/169.678 ms over 29 frames on Microsoft Basic Render Driver/WARP. The sample still uses its direct generated-material path; the separate GPU contract proves cooked/package rendering. The image remains prototype quality and does not demonstrate commercial-reference parity or a hardware performance pass.

## Broadleaf silhouette and venue structure

Native broadleaf generation now uses a segmented tapered leader, rising primary/secondary forks and densely filled crowns of small leaf sprays with varied pitch/yaw and rounded lighting normals. The three LODs retain stable cluster subsets and reduce topology. Bark/leaf material sections, authored wind and bounds across all LODs remain supported; grass and groundcover generation retain their previous topology. Default near/mid/far tree budgets are 1,376/492/204 triangles; maximum 128-branch near trees remain below 28,000 vertices and 14,000 triangles. These are geometry budgets, not performance acceptance.

The sample start gantry uses open galvanized chords and diagonal braces instead of solid dark columns. Existing simple collision envelopes remain in place. Non-preview distant terrain sampling increases from 32 to 96 divisions without changing the flat racing/foliage contact footprint.

`SampleVehicleAdvanced` and `ProceduralServiceTests` rebuilt successfully; `WPProcedural.services` passes (1.26 s). Both fresh `artifacts/procedural/venue-foliage-20261013` car/corner captures pass structural and HDR validation and were inspected. Fuller canopies and the open truss are visible, but the image remains prototype quality: crowns are still too regular, venue/ground treatment is sparse, vehicle design/materials need more work, and an angular sky band needs investigation. WARP mean/P95 intervals are 165.669/183.334 ms for car and 158.293/184.614 ms for corner over 29 frames each. Car geometry submission is about 230,030 triangles per frame, reflecting the added crown detail and terrain grid; this is not a hardware performance pass.

`WPFoliageAuthoringTests` is now an independently buildable CTest target, reusing the existing authoring contracts and adding seeded geometry repeatability, finite unit normals, valid/nondegenerate triangles, complete material sections, all-LOD bounds, strict LOD reduction, maximum branch budgets and unchanged grass topology checks.

Both registered tests pass after a fresh build: `WPGraphics.foliage_plant_geometry` (0.47 s) and `WPGraphics.foliage_authoring` (5.20 s). The first full authoring run exceeded its 60 s timeout because the population persistence/concurrency fixture repeatedly serialized full-detail generated trees. That fixture now uses a small validated species mesh; full-detail/default/maximum recipes retain their separate geometry checks. The population edits, persistence, cancellation, mask/exclusion and concurrent read/write checks remain in the authoring run.

The angular background band was isolated by temporarily omitting only the distant terrain mesh (`artifacts/procedural/sky-isolation-20261013`). The grid/index/upload inspection found no evident corruption. The authored terrain remained raised at its square outer boundary, producing straight fog-coloured skyline edges. The restored mesh now retains the flat inner 300 m footprint, uses a lower, finer ridge profile and returns smoothly to flat ground at radius 600 m, before any square boundary. Fully flat inner/outer cells are omitted; the high-quality ridge submits 10,136 triangles instead of 18,432. The diagnostic omission is removed from the final source and rebuilt sample.

The final `artifacts/procedural/ridge-foliage-20261013/car.bmp` was inspected: the former large diagonal wedge is replaced by a low continuous ridge, with the new foliage/truss visible. Capture, structural and HDR checks pass; the probe retains finite peak radiance 16.5446. The 29-frame WARP run records 168.601 ms mean / 183.257 ms P95 and 213,438 submitted triangles per frame. It remains a short software-renderer diagnostic, not representative hardware or AAA acceptance. Final rebuilt service/authoring/plant checks pass 3/3 (1.44/5.34/0.47 s); `git diff --check` passes. Capture source hashing now includes the general scene/foliage sources, graphics interfaces and native C renderer paths as well as the previous sample/backend roots; both plant generator and native DX11 renderer hashes are present in the final manifest.

## Hero vehicle surfaces and cockpit

The vehicle aero skins now share vertices along chord and span, removing hard normal breaks at every wing strip. Separate perimeter faces retain crisp leading/trailing/side boundaries. All three LODs use curved span profiles with restrained gull shape, taper and twist. Body, sidepod and airbox lofts use bounded shape-preserving Hermite interpolation at near/mid detail; coarse LOD retains the authored stations. Nose tip width increases from 50 to 150 mm. Sidepods have broader rolled decks and a raised shallow inlet with a painted lip, recessed carbon walls and backing instead of a flat dark rectangle over a painted cap. The airbox has the same genuine recess construction. Smooth shoulder shaping also removes the former cross-section discontinuity at the horizontal seam.

The halo is a shared swept curved tube with a centre support in all three LODs, including presets without fine details. The cockpit floor is lower with internal walls to give the opening depth. Wheel hubs, tracks, tyre dimensions and physics pivots are unchanged. The body tail and airbox stations now remain ordered at the supported short/long wheelbase extremes instead of folding backward along Z. No driver, cockpit controls, steering animation upgrade or modern regulation compliance is implied by this increment.

`ProceduralServiceTests` and `SampleVehicleAdvanced` rebuild successfully. Extended service checks cover default/2 m/5 m wheelbases, finite orthonormal vertex frames, complete/nondegenerate triangles, bounds, strict LOD reductions, shared curved wing surfaces, recessed inlet depth, retained tyre/hub/axle dimensions, halo retention and exact default recipe replay. The rebuilt services and sample review contracts pass 2/2 (1.79/0.03 s). Both `hero-surfaces-20261013/car.bmp` and the subsequent `hero-cockpit-20261013/car.bmp` were inspected; the second iteration makes the inlet shallower and retains the curved halo. Final capture/structural/HDR-probe checks pass. WARP mean/P95 are 169.595/201.671 ms over 29 frames, with 229,910 submitted triangles per frame. Geometry detail increases; no performance improvement or AAA art acceptance is claimed. Vehicle proportions/material response, driver content, tyre/rim art, venue composition, ground density and foliage variation still need further work.

A source audit confirms the DX11 sample does not currently use the C HDR post pipeline: `ClawHammerSystem::beginGraphicsPipelineFrame` admits the software renderer only. Native PNTC lighting applies a fixed Reinhard tone map and gamma before direct DX11 presentation. The float reflection probe establishes radiance preservation above one, not adjustable exposure, an HDR display path, or GPU post-processing acceptance. The previously suspected double post-tone-map does not occur on this active DX11 path.

## Explicit sky-matched sunlight

`ClawScene` now accepts a nonvirtual primary directional-light preference. It holds a weak reference, validates current scene membership/type/visibility during selection and falls back to the existing first-visible directional light. Invalid requests preserve the previous preference; hiding permits fallback, removal and scene clear prevent stale retained lights from remaining active. The rendering path uses this same selection helper. No `IGraphicsScene` virtual interface change is required.

The racing builder always owns a sun derived from the procedural sky. The standalone sample selects it after scene generation; host light parameters remain unchanged. Capture validation checks the selected light against the owned sun and records direction/power plus `lightingOutputPath=dx11-fixed-reinhard` and `hdrPostPipelineActive=false` so floating-point reflections are not mistaken for an active GPU HDR post chain.

The registration tests exposed an existing initialization defect: `ClawLight` setters configured before its queued load had no native light/state storage and silently lost values. Scene light creation now initializes the CPU-only native C light before returning it, for both interface-ID and Claw class-ID paths. This introduces no GPU calls on the generation thread. Native light cloning likewise initializes before copying settings and now preserves position. Tests verify nondefault direction/colour/power before any render tick, clone configuration, normal fallback, preference, invalid foreign/generic/point lights, hidden/removed retained objects and clear.

The sample, procedural services, production renderer test and Editor build successfully. After fixture initialization/queue cleanup repairs, the final focused run passes 3/3: services 1.80 s, vehicle review 0.03 s, and `WPGraphics.claw_production_render` 6.17 s. The last suite includes existing GPU terrain/foliage/shadow contracts as well as the new CPU light-selection contracts. This is compiled Editor evidence, not an interactive Editor acceptance run. The production plan now makes active native DX11 linear scene output, one final tone map, display UI ordering and exposure/transfer/resize/device checks an explicit P6 backlog item.

The final `artifacts/procedural/hero-sun-20261013/car.bmp` was inspected and capture validation passes. Runtime evidence reports the selected sky-matched sun at power 1.8 and direction approximately `(0.006285, -0.761013, 0.648706)`; the changed shadow direction is visible. The output-path report confirms fixed Reinhard and no active HDR post pipeline. WARP mean/P95 are 167.355/189.251 ms over 29 frames, with 234,426 submitted triangles per frame. These short diagnostic results do not close hardware performance, driver content, advanced material response, temporal quality or independent visual comparison gates. The scene remains visibly prototype quality.

## Bounded city generation revision 2 — 24 October 2026

The shared `OpenCityLayout` now gives parcels stable signed-cell IDs, independent seed streams, coherent commercial/residential/civic districts, street frontage and parcel envelopes. Four parcels per block retain an 11 m road-centre setback and a 4 m central service gap. Route choice does not reshuffle the lots; expanding the grid preserves common parcel identities and geometry. One-to-three-tier building volumes are shared by the scene's rendered massing and its static collision boxes. Existing six-field lot aggregates remain valid, and new buffers retain source-budget accounting.

City façades use individual near glazing quads, floor bands, corner piers, entrance fascias and roof details. Quads replace the old six-face box per window. Near/far batches live in 128 m patches selected by the existing `LODGroup`/`LODSystem`, with hysteresis and a coarse tier retaining massing, roof silhouette and glazing ribbons. Facade materials share complete generated concrete PBR maps and use world-box projection; the old XZ projection collapsed vertical wall texture coordinates. The first visual review caught an incomplete low-level concrete bake with a metallic fallback; the corrected scene uses `bakeSurface(Concrete)` and requires albedo, normal and ORM outputs. A surrounding landscape surface provides context outside the bounded driving area.

`OpenCityStreetDetails` partitions flat 15 cm sidewalks, junction corners and plaza contact surfaces without the old slab overlap/height seams. The rendered pieces and compact collision union cover the same area. Interior block surfaces merge into one collider each, while outside pieces remain separate so road mouths stay open. Six/eight/ten-block cities use 112/164/224 pavement collision boxes instead of 400/676/1024. This repair follows a real ten-block run that exhausted the scene-root actor capacity; global capacities were not increased. Lamps and decorative bollards use bounded seeded placement with road/junction clearance; lamp posts have collision proxies. Lane markings sit 6 mm above the flat road surface and their winding is normalized upward. Park trees use parcel coordinates at turf grade, distribute across all parks under the existing candidate budget, and use 128 m city pages with a bounded overview visibility range. Circuit foliage policy is unchanged.

The native sample now accepts `--city --city-blocks 6|8|10 --city-route 0|1|2`. Street and city-overview cameras use cached layout dimensions, with a city-sized far clip. The capture runner accepts matching scene options and inventories the procedural interface headers as source inputs. City evidence checks route closure/alignment, footprint clearance, exact building-mass collision actor counts and one active level per city patch. The existing collision smoke mode supports city native shape/pose probes plus road and sidewalk height rays; it does not claim a complete driving lap.

The sample and focused tests rebuild successfully. The latest layout/street tests pass 2/2 (0.57/0.35 s), including parcel/mass nonoverlap, replay/expansion stability, connected routes, bounds, pavement-union equivalence and street furniture. The mocked Lua city workflow passes, and four invalid native CLI combinations exit with the required argument error. Before collider consolidation, the six-block native smoke passed with seed 7: 793 attached static boxes, 225 building masses, road Y=0 and sidewalk Y=0.15.

The first street/overview capture set (`city-scene-20261024-01`, seed 42, six blocks, high) passes native structural/HDR checks. It contains 144 parcels, 125 building lots, 19 parks, 216 building collision volumes and 36 city LOD groups. Street selects 29 near/7 far groups; overview selects 0 near/36 far. Its WARP mean/P95 intervals are 282.224/356.625 ms for street (29 frames) and 79.292/94.270 ms for overview (38 frames). These images were inspected and rejected as visual acceptance: the material issue above darkened façades, and the former tree range hid most parks in the overview. This is a retained diagnostic set, not the corrected art result or a hardware performance pass.

This advances the bounded city portion of P8. Curved/hierarchical road networks, graded or multi-level streets, streamed world sectors, authored architectural kits, interiors, traffic/pedestrians, full driving/Editor/package acceptance and independent visual comparison remain open.

After collision consolidation, the maximum-size native smoke exits successfully: seed 42, ten blocks, route 2, preview quality with foliage. It verifies 1,193 registered static boxes, including 439 building masses, with road Y=0 and sidewalk Y=0.15. The 2,880 m route is generated, but this test probes static shapes and grades rather than driving the route. The earlier failed maximum run remains in `procedural-city-max-physics.*.log`; the successful replacement is `procedural-city-max-compact.*.log`. The final compact street tests pass (0.35 s), alongside layout tests (0.57 s), and prove that collider consolidation preserves the original pavement/plaza union and open carriageways.

The corrected `artifacts/procedural/city-scene-20261024-02` street and overview captures both pass and the runner exits successfully. Both images were inspected. Façade colour/material families are readable and park foliage is now present across the overview. The overview has 19 resident pages and 220 tested/resident tree instances, with no pending or failed pages; its tree LOD counts are 0 near, 13 mid and 207 far. City building LODs remain 29 near/7 far at street level and 0 near/36 far in the overview. Native HDR output and city structural checks pass. Both new procedural headers are included in the source hash inventory.

On Microsoft Basic Render Driver/WARP at 1280×720, final street mean/P95 intervals are 221.145/226.078 ms over 29 frames (467,649 submitted triangles/frame); overview is 79.984/92.593 ms over 38 frames (407,637 triangles/frame). Different views, culling, shadows and tree coverage affect these counters; they are not an isolated LOD benchmark or representative hardware certification. The manifests explicitly record visual acceptance as not accepted. Internal review still finds flat dark glazing, repetitive box-tier architecture and four-parcel grids, sparse human-scale entrances/ground dressing, and a basic surrounding landscape. Calibrated glass/indirect lighting, authored district kits and collision-safe street/park detail are the next visual priorities. No commercial-reference comparison or AAA acceptance is claimed.

The old `Tools/cpp/Editor/src/procedural/ProceduralTextureBindings` source is dormant, excluded from the Editor target and uses outdated texture APIs; its export call was updated to the shared encoder but native Editor generation/export integration remains unavailable. Mocked Lua editor tests do not remove that gap.

Open work includes general recipe identity/versioning and async/cancellable last-good scene publication; full generic geometry/CSG/UV tools; native texture-editor binding; full P2 workflow acceptance; road grading/elevation/junction production work; architectural kits and traffic/crowd/driver content; weather/night/water; GPU temporal quality and reflection work; sustained hardware performance/residency tests; full interactive Editor and source-free racing package acceptance; camera/replay recording; and independent blind comparison. No commercial-reference preference result is claimed.

## City lighting, frontage packing and editable transport foundation — 24 October 2026

The city now has three district facade kits: masonry housing, pale mineral civic/podium surfaces and contemporary office ribbons. Near walls are segmented around actual recessed apertures, with reveals, sills, mullions, closed entrances, canopies, parapets and bounded roof equipment. The stepped occupied building volumes and collision remain shared. Window panes have restrained seeded tint/roughness variation and occasional privacy glass. This is opaque glazing with outdoor HDR specular response; transmission and furnished interiors are not implemented.

City-only ambient/sun balance is now neutral `(0.55,0.57,0.60)`/power 3, facade micro-AO is reduced, and city materials share the existing HDR outdoor probe. Mineral weathering is restrained instead of applying the forge's full contrast to large panels. City fog uses scene-linear radiance in the native filmic path. Circuit lighting policy is retained. Street dressing adds batched park paths, benches, planters, bins, utility covers and paving joints with bounded solid collision proxies. Fresh images exposed dotted black seams and corrugated asphalt: joints were raised to a 6 mm bias, widened and softened, and city asphalt now uses seeded isotropic aggregate over an 8 m tile.

`--city-layout grid|lsystem`, capture `-CityLayout`, the native `City Natural Roads` property and the Lua Inspector select the bounded natural layout. A two-iteration branching grammar snaps/splits its planar graph around three shared closed trunk routes. Limits remain 160 nodes/240 edges; route selection does not reshuffle topology or parcels. Natural sidewalks are conservative rotated prisms checked against every road; acute/short segments can omit footways. Continuous pedestrian junction paving is still open.

The first natural capture exposed a sparse 31-parcel settlement and dense short road forks. Wider branching angles, fewer growth anchors and frontage-based sampling now produce 122 parcels, 112 building lots and 10 parks for seed 42/size 6, on 83 nodes/101 edges. Both sides of actual roads receive bounded candidates; complete envelopes retain an 11 m setback, 0.75 m separation and city bounds. Natural layout preparation moved to `OpenCityLayout.cpp` so future packing changes do not require modifying its public implementation. Seed-corpus size-6/8/10 packing and route independence pass; generated graph indices remain derived identity, not editable source GUIDs.

The rebuilt sample and four focused CTests pass: layout 11.89 s, street descriptors 0.36 s, road graph 0.42 s and spline source 0.13 s. Tests include planarity/connectivity, replay, full-envelope clearance, packing and bounds. The real Lua city fixture passes, including switching layout/record namespaces; this is mocked native/UI workflow evidence. Four invalid CLI combinations exit 2. The revised maximum-size native collision smoke exits 0: seed 42, ten blocks, route 2, preview quality with foliage, 1,015 registered static boxes and 385 building masses, road Y=0, sidewalk Y=0.15, route 1,473.2 m. It checks shapes/poses/rays, not a complete driving lap. The earlier natural smoke (589 boxes/125 masses) predates the packing revision and is retained separately.

Fresh capture sets were inspected and used to revise source. `city-lighting-20261024-03` caught uniform glazing, pavement aliasing and asphalt ruts. `city-natural-20261024-04` caught sparse packing and the gray background slab. Final `city-detail-20261024-05` and `city-frontage-20261024-06` runners exit 0 and all views pass city, foliage and native HDR checks. Both new out-of-line layout/spline sources are present in the final source-hash inventory.

| Final set/view, 1280×720 Debug WARP | Frames | Mean/P95 interval ms | Submitted triangles/frame | Draws/frame |
|---|---:|---:|---:|---:|
| `city-detail-20261024-05/street` | 29 | 206.414 / 214.669 | 739,501 | 389 |
| `city-frontage-20261024-06/street` | 29 | 206.022 / 214.518 | 512,711 | 238 |
| `city-frontage-20261024-06/track` | 47 | 64.540 / 70.227 | 221,917 | 241 |

The final natural street selects 12 near/10 far building patches; overview selects 0 near/22 far. Both have ready foliage with 220 resident tree instances. These different views/layouts are not an isolated LOD or hardware performance benchmark. Review finds substantially clearer street frontages and neighbourhood packing, but opaque glass, repeated silhouettes, aliasing, awkward road forks, incomplete junction walks and untreated land-use gaps still read as a prototype. Manifests record visual acceptance as **not accepted**, with no commercial comparison or hardware certification.

`CityRoadSplineSource` adds a strict version-1 GUID/JSON source and editable relative Bezier handles, bounded adaptive sampling, Fixed/Drape terrain callbacks, centre/edge grade/crossfall and local fold rejection. Invalid input and allocation denial retain previous outputs. RapidJSON scratch allocations use operation-owned SourceCpu tracking; a headroom denial sweep verifies exact baseline residency, including abandoned parse children. The [hand-editable example](examples/city-road-source-v1.cityroads.json) round-trips through the real codec test. Terrain fitting is point-sampled and cannot prove continuous hole/cell coverage. This CPU foundation does not yet drive Editor handles, scene regeneration, terrain carving or native road collision.

[The authoring plan](WPPROCEDURAL_CITY_AUTHORING_PLAN.md) specifies the existing Editor viewport/Undo/asset paths and last-good terrain/city publication work. [The transport and water plan](WPPROCEDURAL_CITY_TRANSPORT_WATER_PLAN.md) now covers editable rivers, road/rail bridges, train tracks and first-class roundabouts, including terrain support, lane-aware island-avoiding routes and 3D surface queries. Those city runtime features remain proposed. Native drawing/terrain integration, real Editor acceptance, source-free packaging, representative hardware and independent blind comparison remain open.

## Terrain road authoring and single-lane roundabouts — 24 October 2026

This increment implements the first native authoring tier described in the [city authoring plan](WPPROCEDURAL_CITY_AUTHORING_PLAN.md). **Window > City Roads** provides terrain binding, click/drag Bezier drawing, knot/handle editing, road fields, draft commit/cancel, Undo/Redo and scene saving. One scene-owned `CityRoadAuthoring` component retains canonical source and a persistent terrain-component UUID. Version 1 remains compatible; strict version 2 adds empty documents and typed roundabouts. Project asset/catalog/importer ownership is still planned, rather than a second editable copy of the scene source. The existing generated grid/L-system city retains its separate flat-support path.

`CityRoadTerrainMesh` clips the entire carriageway footprint against the actual terrain cell triangles. It supports translated, yawed and positively scaled terrain, rejects holes and unsupported coverage, and checks full-cell grades. Fixed profiles cannot float above the terrain. Roundabouts support three to six ports, a real annulus around a raised island/kerb, derived splitter refuges and directed entry/exit routes. Referenced splines must join the solved approach cross-section in position, width, tangent and height; a matching GUID alone cannot admit a disconnected road. The Editor places four connected approaches and conservatively rejects nonplanar junction support. Terrain cut/fill and swept-path approval are unfinished.

Native publication prepares mesh and physics inventories before swapping them under the existing render/physics locks. The same fitted road/island/kerb geometry drives rendering, native mesh collision and driveable-surface queries. Markings are clipped/projected onto the prepared road triangles with a 5 mm visual lift, including terrain ridges; they have no collider. Generated actors are excluded from scene serialization. Failed source edits retain the previous publication. If a changed terrain can no longer support the roads, the component removes stale road collision while preserving source intent and an error. This is safe deactivation, not fully atomic terrain/city replacement or proven GPU-upload failure recovery.

The Debug sample, Editor and four CPU targets build. All five integrated CTests pass in 20.13 seconds: source compatibility/identity, roundabout topology/routes, exact terrain fitting, component revision/lifetime behavior and native Editor viewport authoring. Coverage includes allocation denial and retained outputs, transformed terrain, between-sample holes/ridges, strict joins and terrain-refresh deactivation/recovery. The Editor fixture retains its publication adapter: it verifies Render-delivered controls, owner-task input, consecutive roads, handles after a 5 m terrain rise, multi-step undo/redo, rejected-history cursor preservation, atomic junction deletion, hide cancellation and scene serialization. It does not certify a live Editor session or native GPU publication. It caught the engine's distinct `Parameter` C-string/String storage: city phase producers now construct explicit `String` values.

Four fresh `SampleVehicleAdvanced --road-authoring-showcase` street/overview capture iterations were inspected. The first established native terrain and road collision; the second added markings, splitter refuges and stronger terrain relief; the third moves the close inspection camera above the junction; the fourth refreshes evidence after rebuilding the final graphics library. Final [street](../artifacts/procedural/city-road-authoring-20261024-04/street.bmp) and [overview](../artifacts/procedural/city-road-authoring-20261024-04/track.bmp) captures both pass native structural/HDR checks, four native road/ring/island/terrain ray probes, island exclusion, circulation direction and route/surface height agreement. The fixture contains four splines, one roundabout, 9,409 terrain samples and 24,561 runtime triangles. The final manifest inventories 1,235 sources, including the native ImGui path, and 11 binaries.

At 1280×720 on Microsoft Basic Render Driver/WARP, the final close view records 50 frames with mean/P95 intervals 60.507/87.944 ms; overview records 78 frames at 39.076/55.184 ms. These short software runs are diagnostic. The manifests explicitly record visual acceptance as **not accepted**: asphalt/terrain shading is simple, surroundings are empty and thin distant lines alias. No representative hardware result, full driving lap, commercial blind comparison or AAA parity is claimed. Rivers, bridges, rail, arbitrary junction editing, generated-city source conversion and production art remain open.

The Debug executable now opens the ordinary Editor when launched without arguments or with `--interactive`; explicit Boost arguments retain the CTest runner. Development startup resolves relative plugin/media paths from the executable directory. A live native window was exposed, but Windows capture failed with `FrameArrived timed out`/`window capture timed out`; a native accessibility read exposed title-bar controls only, and input reported `coordinate input geometry is unavailable`. The same limitation occurred with twelve and zero workers, so no threaded-render defect is established. Both owned instances were stopped and the ordinary worker policy restored. [The recorded attempt](../artifacts/procedural/city-road-authoring-editor-20261024-attempt/attempt.json) explicitly leaves mouse-driven draw/edit/undo/reopen acceptance unverified. The final control pass also gives newly drawn roads the same 25 mm terrain surface bias as the native showcase, preventing coplanar road/terrain surfaces.
