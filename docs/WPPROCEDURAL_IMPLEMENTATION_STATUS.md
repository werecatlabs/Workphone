# WPProcedural implementation status

## Racing foundations and visual slice — 12 October 2026

This implements parts of P0–P5 and P9 from the [production plan](WPPROCEDURAL_PRODUCTION_PLAN.md). It does not complete P0–P10 or certify AAA quality. Changes reuse the existing procedural services, racing component, terrain foliage attachment and resource compiler.

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

Next colour-space audit: the native DX11 shader applies a `pow(...,2.2)` colour decode while sRGB DDS profiles use hardware sRGB views. A mid-grey render comparison is needed to determine and repair double decoding; existing saturated red/green fixtures do not establish calibrated colour equivalence. The current multi-map tests establish channel routing and effects, not this colour calibration gate.

The rebuilt sample capture at `artifacts/procedural/pbr-runtime-20261013/car.bmp` was inspected. Native frame, geometry/LOD and foliage checks pass; the report records 58 applied replay ticks and no failed foliage pages. Mean/P95 intervals are 141.912/169.678 ms over 29 frames on Microsoft Basic Render Driver/WARP. The sample still uses its direct generated-material path; the separate GPU contract proves cooked/package rendering. The image remains prototype quality and does not demonstrate commercial-reference parity or a hardware performance pass.

The old `Tools/cpp/Editor/src/procedural/ProceduralTextureBindings` source is dormant, excluded from the Editor target and uses outdated texture APIs; its export call was updated to the shared encoder but native Editor generation/export integration remains unavailable. Mocked Lua editor tests do not remove that gap.

Open work includes general recipe identity/versioning and async/cancellable last-good scene publication; full generic geometry/CSG/UV tools; native texture-editor binding; full P2 workflow acceptance; road grading/elevation/junction production work; architectural kits and traffic/crowd/driver content; weather/night/water; GPU temporal quality and reflection work; sustained hardware performance/residency tests; full interactive Editor and source-free racing package acceptance; camera/replay recording; and independent blind comparison. No commercial-reference preference result is claimed.
