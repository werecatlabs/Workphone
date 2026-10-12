# WPProcedural implementation status

## Racing foundations and visual slice — 12 October 2026

This implements parts of P0–P5 and P9 from the [production plan](WPPROCEDURAL_PRODUCTION_PLAN.md). It does not complete P0–P10 or certify AAA quality. Changes reuse the existing procedural services, racing component, terrain foliage attachment and resource compiler.

### Runtime and authoring changes

- `MeshGeneratorDefault` now generates terrain and road meshes, supports the corresponding object adapters, retains validated settings and publishes progress/history. Budgets, malformed dimensions, nonfinite data and unsupported options fail explicitly. Failed requests retain the previous generated mesh. LOD generation, simplification, tangents, arbitrary object conversion and collision generation remain unsupported by this adapter.
- `ProceduralMesh` now computes retained bounds, validates geometry indices/finite positions, preserves UVs and masks during append, and handles self-append. Invalid append candidates do not alter geometry. Public direct vertex edits still require explicit bounds recomputation.
- Fixed vehicle sidepod loft indices that referenced the central body instead of the appended pod vertices. Added closed pods, an open cockpit, interior/rails, raised airbox, cambered wings, pylons/endplates, mirrors and diffuser details. Wheel/axle physics metadata remains shared with the existing vehicle generator.
- Circuit and city reuse generated asphalt maps; city facades have generated concrete maps, roof/parapet/plinth modules and roof service details. Pit garages have an apron, roof caps, fascia, glazing, mullions and door slats. This is an incremental procedural art pass, not a production architectural kit.
- Optional `Foliage World` selection uses the existing terrain-attached foliage stream and Claw renderer: four seeded species with real reduced mesh LODs, alpha-cutout mips, wind, circuit exclusions/city-park placement and trunk colliders. Legacy patch trees remain available. Scene teardown clears/unregisters the foliage attachment before destroying generated textures. Runtime texture resolution now finds generated manager-owned atlases.
- Native and Lua samples share the component's foliage selection. Lua saves it through the sample's properties. Native quality names now include `cinematic`, matching the existing four-value quality enum; the name itself makes no visual/performance guarantee.
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

Verification is being completed for this increment; final results are recorded below after execution. Standalone export/recipe tests and independent image decoding are separate from the native service/GPU/sample runs. Structural validation reports do not establish image quality or the performance gate.

The earlier capture set at `artifacts/procedural/implementation-baseline-02` passed capture/native-frame/winding/LOD checks on Microsoft Basic Render Driver/WARP. Car/corner mean intervals were 192.541/207.309 ms with P95 216.277/218.981 ms; the runs contain only 29 measured renderer frames each. That set predates the completed hero geometry and optional foliage path. It fails the proposed frame target and remains visibly prototype quality. It is not hardware certification.

The existing cooked material schema binds base colour only. Generated normal/ORM/scalar texture cooking and package loading can be verified independently, but this does not establish a cooked multi-channel PBR draw. The old `Tools/cpp/Editor/src/procedural/ProceduralTextureBindings` source is dormant, excluded from the Editor target and uses outdated texture APIs; its export call was updated to the shared encoder but native Editor generation/export integration remains unavailable. Mocked Lua editor tests do not remove that gap.

Open work includes general recipe identity/versioning and async/cancellable last-good scene publication; full generic geometry/CSG/UV tools; native texture-editor binding; cooked PBR channel binding; road grading/elevation/junction production work; architectural kits and traffic/crowd/driver content; weather/night/water; GPU temporal quality and reflection work; sustained hardware performance/residency tests; full interactive Editor and source-free racing package acceptance; camera/replay recording; and independent blind comparison. No commercial-reference preference result is claimed.
