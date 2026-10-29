# Terrain GT0 completion and validation

Implementation follow-up: 29 October 2026. This tracks bounded Windows x64/DX11
terrain. [The capability matrix](TERRAIN_WORKFLOW.md#capability-and-evidence-matrix)
defines implemented features and bounds; the original terrain/procedural audit is
historical. Automated passes alone do not certify GT0 or GT1.

## Reconciled baseline

TERR-01 through TERR-10 already have connected implementations: version-three
source/migration, shared triangulation/holes/queries/collision, four-layer PBR,
viewport editing and persistence, tiled LOD and bounded asynchronous geometry,
ground-consumer inputs, erosion/recipes/grading and cooked resources. Historical
missing-feature rows in the parent plans must not drive reimplementation.

The bounded core retains source/control data in memory and full-resolution
synchronous collision capped at 524,288 solid triangles. Disk-paged source and
collision, floating-origin rebasing, automatic terrain-city cut/fill and atomic
terrain/city replacement remain GT1/cross-system work. The four-layer limit,
PGM/raw height codecs and supported-transform policy are explicit capabilities.
PNG/TIFF/EXR, pressure/mirror modifiers and distance-resampled strokes remain
unsupported in this GT0 tier.

## Validation and package changes

Fresh validation exposed a full-source-budget replacement failure. Superseded
staging now retires before cancellation allocates bookkeeping. If a second
reservation for the same immutable source cannot leave enough job headroom,
renderer requests share their existing admitted source lease with stream work.
Ledger/class/size checks and retained-result lifetime contracts cover that path;
failed requests do not commit demand/device state. The original full-budget
fixture and five-second deadline remain unchanged.

Mesh allocation contracts now count the scheduler's admitted lease/control-block
metadata while separately checking exact skin payload sizes and capacities.
Cooked-terrain retirement checks destroy their still-live terrain/camera/target
owners before comparing source/result/GPU reservations to the original baseline.

The existing graphics runner now has `-TerrainOnly`. It requires CPU, native
physics, actual Lua editor workflow, DX11 production, cooked-resource,
representative performance and 1,000-cycle regeneration suites. Reports use the
`terrain-gt0-<configuration>` prefix, separate from full graphics-baseline reports.
Executable hashes are compared before/after execution; a changed binary fails
validation. Automated coverage and release certification remain separate fields.

`ValidateTerrainPackage.ps1` uses the existing cooked-resource executable in two
different processes. The author produces the original four-layer fixture and its
thirteen pinned textures. Only the install closure, executable and resolved PE
dependencies enter the relocated install. The consumer starts there with a
restricted DLL search PATH and constructs `ResourceSystem(nullptr, nullptr)`
without initializing the authoring fixture, compiler registry, compilation DB,
catalog or SQLite plugin.

The consumer checks actual four-layer pixels, verified tile metadata, corrupt and
wrong-target replacement retention, cache-handle release and renderer recreation.
It retains a PPM capture. Separate negative processes check a missing manifest and
a corrupt installed payload. The report includes installed file SHA-256 hashes,
subprocess logs/exit codes and actual DX11 device output. Existing output
directories are rejected rather than replacing earlier evidence.

This is a small fixture without a vehicle/physics scene. It proves a source-free
runtime process and failure handling, not representative gameplay, clean-machine
deployment, interactive usability or commercial visual acceptance. The executable
links authoring libraries for its separate export mode; consume never creates
their services. OS/CRT prerequisites remain provisioned on the validation host.

## Gate register

| Gate | Accountable role | Required evidence | Status |
|---|---|---|---|
| Baseline correctness | Terrain/QA | Fresh Debug and RelWithDebInfo CPU/Lua/physics/GPU/cooked results and candidate identity | Passed: 7/7 each, no skips or source/EXE/DLL drift |
| Bounded performance | Terrain/QA | Erosion/flythrough/teleport and 1,000-cycle regeneration measurements and retirement | Passed both configurations; local fixtures, not hardware frame-time acceptance |
| Relocated consumer | Resource/rendering | Separate source-free consumer, complete closure, failure handling and recreation | Passed both configurations, including ZIP extraction/re-execution |
| Editor workflow | Tools/QA | Scene viewport strokes, cancel/focus/selection/DPI, undo/redo, save/restart/reload | Two native contracts passed; live mouse walkthrough blocked by capture timeouts |
| Representative hardware | Rendering/QA | Supported physical GPU/device matrix and performance | Unavailable here: Basic Display Adapter and DX11 WARP only |
| Representative scene | Terrain/physics/art | Layers, holes, tile borders, shadows, multiple views, vehicle/contact probes and one active revision | Local automated pass in both configurations; native rays, driving and captures passed in copied/extracted gameplay packages |
| Visual acceptance | Art/independent reviewer | Camera sweeps/traversal without cracks, unacceptable LOD changes, shadow errors or road seams | Captures inspected for terrain/hole/vehicle output; independent quality acceptance remains open |
| Shipping candidate | Build/QA | Representative source-free scene on clean machine, prerequisites/notices, final configurations and archive hashes | Both local gameplay ZIPs passed; clean-machine, prerequisite and complete license audit gates remain open |
| Shared graphics-release longevity | Rendering/QA | Parent production plan's eight-hour mixed scene, reload/recovery, memory pressure and stable post-warm-up resources | Open; separate from the bounded terrain fixtures |

Roles identify responsibilities, not staffing assignments. GT0 requires every
mandatory bounded-core gate on the same candidate. The eight-hour mixed-scene row
belongs to the parent graphics release; a 1,000-cycle terrain fixture does not
satisfy it. GT1 and whole-renderer R1/R2 certification remain separate.

## Final evidence, 29 October 2026

This is the earlier fixture candidate. The representative gameplay follow-up below
records the newer implementation and its reconciled build/test/package identity.

The local [evidence index](../artifacts/terrain/gt0-20261029/gt0-evidence-index.json)
rechecks both configurations' executable/DLL hashes after all runs. Both packages
and test reports match source fingerprint
`158EE79CBEE674FA62DB734F972EA8BF85C1E78F1D9EBEC2E7DA7E8E117AB959`.
Both aggregate builds, including Editor, completed. Toolchain: CMake 4.4.2,
Visual Studio 18 Insiders/MSVC 19.52.37025.0, Windows SDK 10.0.26100.0.

- [Debug automated evidence](../artifacts/terrain/gt0-20261029/terrain-gt0-Debug-evidence.json): seven passed, zero unavailable.
- [RelWithDebInfo automated evidence](../artifacts/terrain/gt0-20261029/terrain-gt0-RelWithDebInfo-evidence.json): seven passed, zero unavailable.
- [Debug Editor contracts](../artifacts/terrain/gt0-20261029/accepted-Editor-Debug.xml): `city_road_viewport_authoring_test` and `terrain_city_source_anchor_roundtrip_test` passed in 25.41 seconds. These dispatch native rays and reconstruct scene data in-process; they do not prove mouse input or an interactive process restart.
- [Debug package evidence](../artifacts/terrain/gt0-20261029/package-Debug-04/package-evidence.json) and [RelWithDebInfo package evidence](../artifacts/terrain/gt0-20261029/package-RelWithDebInfo-01/package-evidence.json): separate author/consumer processes, both negative asset cases, unchanged installed assets and extracted ZIP execution passed.
- [Interactive attempt](../artifacts/terrain/gt0-20261029/interactive-attempt.json): Editor launched; capture failed twice after selecting/reselecting its returned window. No unobserved mouse actions were attempted.

| Local fixture | Debug | RelWithDebInfo |
|---|---:|---:|
| Thermal erosion: 1025x1025 source, radius-128 region, 64 iterations | 4625.239 ms | 1887.793 ms |
| Hydraulic erosion: same source/region/iterations | 11190.553 ms | 4076.072 ms |
| Erosion cancellation | 18.164 ms | 0.278 ms |
| Cold CPU stream: 256 tiles | 593.975 ms | 467.846 ms |
| Flythrough stop P95 | 60.510 ms | 32.650 ms |
| 1,000-cycle regeneration P95 | 248.510 ms | 69.751 ms |

Both regeneration runs returned to the warmed source-reservation baseline, and
both CPU streams stayed within 8 MiB of ready-result reservations. These are local
fixture measurements, not representative hardware rendering/performance or
eight-hour soak certification.

Final archives: [Debug fixture ZIP](../artifacts/terrain/gt0-20261029/package-Debug-04/WorkphoneTerrainFixture-Debug.zip)
and [RelWithDebInfo fixture ZIP](../artifacts/terrain/gt0-20261029/package-RelWithDebInfo-01/WorkphoneTerrainFixture-RelWithDebInfo.zip).
Hashes and per-file manifests are in the package reports. Earlier failed and
unstable-source runs remain in the evidence directory; they are not the accepted
candidate. Concurrent edits invalidated earlier results; the final release build
waited for competing shared-output builds to finish.

These are scoped terrain passes. Broader graphics/external-media coverage was
not recertified. `validationPassed` and `packagePassed` are true for the recorded
scopes; `gt0Certified` and `releaseCertified` remain false because the hardware,
live interaction, representative hardware/visual and clean-machine shipping gates
above have not passed.

## Representative gameplay evidence, 29 October 2026

The [final gameplay evidence index](../artifacts/terrain/gt0-20261029/gt0-gameplay-evidence-index.json)
reconciles both builds, fourteen automated terrain tests, fourteen package
subprocesses, four native captures and every ZIP entry. Current source and runtime
bytes were rechecked after execution. All accepted scopes match source fingerprint
`E496F8DB19E574F15DEE546D42A3C9600A0113FA2CA45764C5C16D92D2810A1C`.
No source, executable or runtime-library drift was observed in these final passes.
Other work was allowed to continue; no shared-work freeze was required.

- [Debug tests](../artifacts/terrain/gt0-20261029/gameplay-Debug-evidence-final.json) and [RelWithDebInfo tests](../artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-evidence-final.json): 7/7 each, no skips, including native physics, Lua, production rendering, cooked resources, performance and 1,000-cycle regeneration.
- [Debug gameplay package](../artifacts/terrain/gt0-20261029/gameplay-Debug-package-04/gameplay-package-evidence.json) and [RelWithDebInfo gameplay package](../artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-01/gameplay-package-evidence.json): separate author, copied consumer, captures, negative inputs and ZIP-extracted consumer all passed. Every subprocess completed within its unchanged 600-second packaging watchdog.
- Both driving runs per configuration passed settle, acceleration, steering, braking/reset and settling after reset. Four-second acceleration reached **21.72 m/s** against the unchanged **greater-than-20 m/s** gate. Native terrain query/render/physics probes agree; the hole misses all three and no fallback collision plane is present.
- Each install contains the manifest and fifteen cooked containers, its executable/plugin import closure, launcher and twelve original notices. No source assets or authoring database are shipped. Installed assets and the final install inventory remain unchanged after native diagnostic logs are collected.
- The negative logs explicitly report a missing `runtime.wprm` and `Compiled resource payload hash mismatch`. The finalizer verifies these precise causes, beyond the package harness's broader failure regex.

| Configuration | Seed / quality | Pinned vehicle hash | Archive |
|---|---|---|---|
| Debug | 7 / Preview | `368d4a2c35c68eb7` | [Gameplay ZIP](../artifacts/terrain/gt0-20261029/gameplay-Debug-package-04/WorkphoneTerrainGameplay-Debug.zip) |
| RelWithDebInfo | 7 / Preview | `6d6594b18f8e98d7` | [Gameplay ZIP](../artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-01/WorkphoneTerrainGameplay-RelWithDebInfo.zip) |

Both [terrain overview](../artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-01/terrain-gameplay-track.bmp)
and [car view](../artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-01/terrain-gameplay-car.bmp)
were captured separately at 640x360 in each configuration. The exercised adapter
was **Microsoft Basic Render Driver/WARP**, feature level 11.0, driver
`10.0.19041.7802`, with no dedicated video memory. Some final suites overlapped
local package execution; their timings are loaded-host diagnostics, not hardware
performance acceptance.

The resource mount is read-only, but the application transiently writes
`Application.log` in its working directory before the harness collects it.
An installation with filesystem writes denied was not tested. Vehicle generation
is pinned to each bundled configuration; cross-configuration bitwise output and
package interchangeability are not certified. Native captures establish rendered
output, not independent visual approval or live Editor interaction. Physical GPU
coverage/recovery, live viewport authoring and persistence, clean-machine
deployment, complete license audit and the parent graphics eight-hour soak remain
open. **GT0 and release certification remain false.**

## Reproduction

### Representative terrain gameplay package

Gameplay integration exposed three additional boundaries. Cooked artifact
hard-link publication and author scratch cleanup now use Windows extended-length
IO paths. Terrain readiness retries no longer repeatedly stop render/physics tasks
while waiting for those tasks to publish the deferred generation. Shared car
initialization now applies the documented road grip of 1.25, matching later surface
transitions; the existing acceleration, stability and phase-duration checks are
unchanged. Package-mode UI contexts disable automatic layout/log persistence so
validation can check the installation remains unchanged.

`SampleVehicleAdvanced --terrain-gameplay` extends `ProceduralRaceScene` with a
129x129 heightfield spanning 512 metres. Four layers, shared tile borders,
off-corridor hills and a true hole use native terrain rendering and collision;
the existing car, controller, wheel presentation and five-phase driving smoke
remain the gameplay path. The scene does not add a fallback collision plane.

`--export-terrain-gameplay <fresh-directory>` cooks a gameplay root, terrain and
thirteen textures into a pinned runtime closure. The vehicle tier is a versioned
procedural recipe: the runtime regenerates the existing Grand Prix vehicle and
rejects a mismatch with its stored full geometry/material/physics content hash.
Each archive pins this recipe to its bundled configuration; bitwise-identical
vehicle generation across Debug and optimized builds is not part of this tier.
This tier does not serialize baked vehicle mesh resources or use a saved scene's
transient generated mesh names as shipping assets.

`--terrain-gameplay-package <assets-directory>` mounts only that cooked closure.
It creates no compiler registry or compilation database, skips Lua/audio services
and the application's default-font asset, and uses an explicit package filesystem
root. Packaged UI contexts disable automatic layout/log file persistence. The executable
still imports authoring libraries needed by its separate export mode. Runtime
absence of those services does not imply their DLLs are absent.

Build `SampleVehicleAdvanced` and its graphics/physics/procedural plugins, then
run the package harness against the same configuration:

```powershell
./Tools/ValidateTerrainGameplayPackage.ps1 -BuildDirectory project_x64 -Configuration Debug
```

The harness separates actual driving from track/car captures, checks native
render/query/collision agreement and hole rejection, tests missing/corrupt cooked
inputs, preserves notices, hashes the install, extracts its ZIP and reruns driving.
It rejects source or runtime-binary drift and leaves diagnostic evidence in a
fresh output directory. Its 600-second process watchdog is a packaging diagnostic;
the existing city, terrain performance and soak thresholds remain unchanged.
Relocation on this host does not establish clean-machine, hardware, live Editor
interaction, independent visual acceptance or whole-renderer release certification.

Build both configurations, including Editor and package:

```powershell
./Tools/BuildWPGraphicsBaseline.ps1 -BuildDirectory project_terrain_gt0 `
    -Configuration Debug,RelWithDebInfo -IncludeEditor -TerrainOnly `
    -IncludeTerrainPackage -IncludeTerrainGameplayPackage
```

For an existing configured build, build `WPTerrainValidation`, then run:

```powershell
./Tools/ValidateClawGraphics.ps1 -BuildDirectory project_x64 -Configuration Debug -TerrainOnly
./Tools/ValidateTerrainPackage.ps1 -BuildDirectory project_x64 -Configuration Debug
```

Repeat with `RelWithDebInfo`. Run the full baseline without `-TerrainOnly` for
cross-system coverage; a focused pass cannot certify external-media fixtures or
unrelated graphics features. Avoid concurrent builds into the same configuration
and output paths. Preserve failure evidence and repair the actual boundary
without weakening fixtures, thresholds or timeouts.

Inside an extracted fixture ZIP, run from its root:

```powershell
./bin/WPGraphicsCookedResourceTests.exe --terrain-package-consume assets terrain.ppm
```

Omit the capture argument for a read-only installation. This is a focused terrain
consumer fixture, not a packaged representative vehicle scene or the Editor.
