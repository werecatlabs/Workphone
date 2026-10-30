# Terrain GT0 completion and validation

Implementation follow-up: 29–30 October 2026. This tracks bounded Windows x64/DX11
terrain. [The capability matrix](TERRAIN_WORKFLOW.md#capability-and-evidence-matrix)
defines implemented features and bounds; the original terrain/procedural audit is
historical. Automated passes alone do not certify GT0 or GT1.

The [current follow-up evidence index](../artifacts/terrain/gt0-20261029/gt0-editor-followup-evidence-index.json)
binds the repaired Editor, eighteen accepted tests, both fresh gameplay packages
and the prepared portable clean-Windows kit. The gate register retains the
remaining observed-Editor, guest, hardware and independent visual requirements.

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
representative performance and 1,000-cycle regeneration suites. The live follow-up
also makes scene-picking and Editor event-queue regressions mandatory, for nine
tests in the current terrain-only gate. Reports use the
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
| Baseline correctness | Terrain/QA | Fresh Debug and RelWithDebInfo CPU/Lua/physics/GPU/cooked results and candidate identity | Repaired `C4B9ADB4...` passed 9/9 each without source/runtime drift; Debug 229.45s, optimized 90.91s |
| Bounded performance | Terrain/QA | Erosion/flythrough/teleport and 1,000-cycle regeneration measurements and retirement | `C4B9ADB4...` fixtures passed both configurations; local measurements, not hardware frame-time acceptance |
| Relocated consumer | Resource/rendering | Separate source-free consumer, complete closure, failure handling and recreation | `C4B9ADB4...` fresh Debug and optimized package 05 copied/extracted runs passed; all fourteen subprocess checks reconciled |
| Editor workflow | Tools/QA | Scene viewport strokes, cancel/focus/selection/DPI, undo/redo, save/restart/reload | `C4B9ADB4...` live sculpt, native-menu history, release-outside cancel and authored distinct-process scene reload passed at 96 DPI; full Editor matrix remains open |
| Representative hardware | Rendering/QA | Supported physical GPU/device matrix and performance | Unavailable here: Basic Display Adapter and DX11 WARP only |
| Representative scene | Terrain/physics/art | Layers, holes, tile borders, shadows, multiple views, vehicle/contact probes and one active revision | `C4B9ADB4...` automated checks and both fresh packages passed; copied/extracted driving and four native captures reconciled |
| Visual acceptance | Art/independent reviewer | Camera sweeps/traversal without cracks, unacceptable LOD changes, shadow errors or road seams | Captures inspected for terrain/hole/vehicle output; independent quality acceptance remains open |
| Shipping candidate | Build/QA | Representative source-free scene on clean machine, prerequisites/notices, final configurations and archive hashes | Both `C4B9ADB4...` local packages passed; optimized kit 04 and portable ZIP prepared. Actual guest, prerequisites and full license audit remain open; Debug uses development CRTs |
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
subprocesses, four native captures and every ZIP entry. Source and runtime
bytes were rechecked at that candidate's completion. All accepted scopes match source fingerprint
`E496F8DB19E574F15DEE546D42A3C9600A0113FA2CA45764C5C16D92D2810A1C`.
No source, executable or runtime-library drift was observed in these final passes.
Other work was allowed to continue; no shared-work freeze was required. This is
retained candidate evidence: the subsequent Editor/input fixes below change the
workspace and must not inherit this candidate's all-suite or archive acceptance.

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

## Live Editor and offline guest follow-up, 30 October 2026

The current repaired build/test candidate is source
`C4B9ADB426748B6A6F7067F9136B731F50135839ACAE1C28BB97F576D4F599A3`.
The [release-outside repair report](../artifacts/terrain/gt0-20261029/editor-release-outside-fix-01/report.json)
records successful WPImGui rebuilds, WPGraphics relinks and Editor/regression
builds in both configurations. All nine mandatory tests passed in each:
**Debug 229.45s; RelWithDebInfo 90.91s**. Source and runtime hashes did not change
during either suite. These local WARP/CPU passes do not certify hardware or GT0.

The production terrain phase selector now checks forced cancellation and whether
the pointer is outside the exact scene image before considering mouse release.
A release that is the first sampled frame outside therefore cancels the active
stroke instead of committing it. The shared selector regression covers that
same-frame case; the real LuaManager/native terrain test checks restored samples
and **Cancelled terrain stroke.** status. Cancellation changes status only when
a stroke was active, so idle outside frames retain completed-stroke status.

[Live sessions 09/10](../artifacts/terrain/gt0-20261029/editor-live-09/session-evidence.json)
used actual Sky mouse/keyboard input in the native Editor at 96 DPI. Session 09
sculpted **312/4,225 samples**, with a maximum terrain-local height delta of
**0.349393129m**, on the 65x65 source with two-metre spacing and height scale one.
Native **Edit > Undo** and terrain **Save** reproduced the baseline bytes
(`9053215D...`); **Edit > Redo** and **Save** reproduced the painted bytes
(`2FB8B52D...20B0`). A subsequent active stroke released outside the image
reported cancellation in [frame 036](../artifacts/terrain/gt0-20261029/editor-live-09/captures/frame-036.bmp).
Its saved cancelled snapshot exactly matches the painted/redo payload.

The authored scene was saved through native **Save Scene As** to
[06-authored.fbscene](../artifacts/terrain/gt0-20261029/editor-live-09/snapshots/06-authored.fbscene).
The [session 09 launch](../artifacts/terrain/gt0-20261029/editor-live-09/launch.json)
and [exit](../artifacts/terrain/gt0-20261029/editor-live-09/exit.json) record PID
16924 exiting with code zero. A distinct
[session 10 launch](../artifacts/terrain/gt0-20261029/editor-live-10/launch.json),
PID 14248, used native **File > Open Scene** and terrain **Save** to produce
[07-restarted.terrain.json](../artifacts/terrain/gt0-20261029/editor-live-09/snapshots/07-restarted.terrain.json),
also exactly matching the painted bytes. Its
[exit record](../artifacts/terrain/gt0-20261029/editor-live-10/exit.json) reports
code zero. Both sessions retain the same source, Editor and runtime hashes.

The independent stock Windows PowerShell 5.1
[full content report](../artifacts/terrain/gt0-20261029/editor-final-content-review-01/terrain-authoring-evidence.json)
and [review verification](../artifacts/terrain/gt0-20261029/editor-final-content-review-01/review-verification.json)
confirm all seven stable artifacts, exact undo/redo/cancel/restart byte pairs and
the owned scene component's embedded payload. The content helper intentionally
keeps UI input and actual process-restart flags false; observed input/process
evidence is recorded in the live sessions separately.

This closes the observed sculpt/history/release-outside/save/restart path.
The session report accepts only this scoped live sequence;
`fullEditorWorkflowMatrixAccepted` and `interactiveEditorAuthoringAccepted`
remain false. Live Escape, focus-loss and selection
cancellation, other DPI settings, layer/hole authoring and continuous stroke
sampling were not established by these sessions. Physical-GPU and independent
visual acceptance remain open; **GT0 and release certification remain false**.

The current [optimized package 05 verification](../artifacts/terrain/gt0-20261029/package-refresh-05/final-verification.json)
and [gameplay package report](../artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-05/gameplay-package-evidence.json)
reconcile the same `C4B9ADB4...` source as the eighteen automated tests and the
separately observed live sequence above. All seven package subprocesses passed:
authoring, copied five-phase driving, two separate captures, both precise negative
causes and ZIP-extracted five-phase driving. Copied/extracted driving took
**407.06s/419.48s**, within the unchanged **600s** watchdog. The root-owned Editors
were stopped during these WARP runs; other host work continued, so timings remain
local diagnostics. Runtime inputs, installed assets and all 38 payload file hashes
were unchanged; the archive also contains four directory entries.

The [optimized gameplay ZIP](../artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-05/WorkphoneTerrainGameplay-RelWithDebInfo.zip)
SHA256 is `ADC4450CDE05A8006AE529FCDF1C8F38D5D30D8038CDFCF96D5374A18CFD0ED4`.
[Offline kit 04](../artifacts/terrain/gt0-20261029/clean-machine-kit-04/README.txt)
and its [preparation evidence](../artifacts/terrain/gt0-20261029/clean-machine-kit-04/kit-evidence.json)
pin that archive and inventory. Preparation passed under stock Windows PowerShell
5.1; manifest, guest-runner and Sandbox-configuration hashes match their pins.
The kit remains **prepared-only**: its guest evidence folder is empty, no guest
ran, and no prerequisite installer was supplied. Actual clean-Windows deployment,
prerequisite and license acceptance remain open.

The [portable offline validation kit](../artifacts/terrain/gt0-20261029/offline-kit-transfer-04/WorkphoneTerrainCleanWindowsValidationKit.zip)
bundles those prepared files, including the empty guest-evidence directory.
[Transfer verification](../artifacts/terrain/gt0-20261029/offline-kit-transfer-04/transfer-evidence.json)
checks all nine file streams against kit 04. Extract it into a fresh folder on an
already Sandbox-capable host and use the README's hash-pinned Launch/Collect
commands; Launch regenerates absolute mapping paths for that host. The transfer
archive does not add guest, prerequisite, hardware or visual acceptance.

The fresh [Debug package verification](../artifacts/terrain/gt0-20261029/package-refresh-Debug-05/final-verification.json)
also passed all seven subprocess checks on `C4B9ADB4...`, with copied/extracted
driving at **420.80s/417.71s** under the same 600s watchdog. Its
[gameplay ZIP](../artifacts/terrain/gt0-20261029/gameplay-Debug-package-05/WorkphoneTerrainGameplay-Debug.zip)
SHA256 is `AA8B77C7CB4E4217797096DFC3A2F992D8E4A57014DF76123F65BBEFF2EC458C`.
This is local diagnostic relocation using development CRT prerequisites, not a
Debug clean-machine or redistribution pass; kit 04 targets only the optimized
package.

[Combined package verification](../artifacts/terrain/gt0-20261029/package-refresh-final-05/combined-verification.json)
reconciles both current archives, fourteen subprocesses, eighteen baseline tests,
76 payload files and eight directory entries. The
[final follow-up index](../artifacts/terrain/gt0-20261029/gt0-editor-followup-evidence-index.json)
also rechecks both runtime configurations, all native authoring snapshots and
key captures against the same source. Earlier `E496F8DB...` Debug and
`6C5D806F...` package 04/kit 03 results remain historical evidence below.

### Retained ray-repair and earlier evidence

The preceding completed automated baseline and optimized package pass used source
`6C5D806FC3CD7CB0BE4331DFFA98B2E5A9D6836041CE14E2A6BCC60E69F10A58`.
[Debug](../artifacts/terrain/gt0-20261029/final-nine-test-baseline/Debug/terrain-gt0-Debug-evidence.json)
and [RelWithDebInfo](../artifacts/terrain/gt0-20261029/final-nine-test-baseline/RelWithDebInfo/terrain-gt0-RelWithDebInfo-evidence.json)
each passed all nine mandatory tests with complete coverage and no source,
test-executable or runtime-DLL drift. Elapsed suite times were 222.59s and 91.01s.
These runs were sequential with our Editor/package processes stopped; they remain
local WARP/CPU diagnostics. The underlying focused builds/relinks and failure
history are retained in
[final reconciliation](../artifacts/terrain/gt0-20261029/editor-ray-fix-01/final-reconciliation-02/report.json).

Subsequent live validation reproduced the release-outside cancellation defect
repaired in `C4B9ADB4...` above. These retained `6C5D806F...` results predate that
repair and do not validate its runtime bytes.

[Live session 07](../artifacts/terrain/gt0-20261029/editor-live-07/session-evidence.json)
used actual Sky input in the Scene render viewport. The corrected yellow brush
preview followed the pointer, and a focused drag reported a committed stroke.
Native UI Save recorded 313/4,225 changed samples with a maximum height delta of
0.34993118m. Native **Edit > Undo** followed by Save produced bytes identical to
the baseline; **Edit > Redo** followed by Save produced bytes identical to the
painted snapshot. The source is a 65x65 grid with two-metre spacing and height
scale one. This verifies a specific live sculpt/history path, not the full DPI,
focus, cancellation, layer/hole paint or independent visual matrix.

The owned Editor exited during resized-window panel navigation after package
startup had changed focus. Its saved snapshots and normal shutdown log are
retained. The continuation retained an authored native scene; the
[content precheck](../artifacts/terrain/gt0-20261029/editor-scene-content-precheck-01/content-precheck.json)
confirms its owned TerrainSystem embeds the exact 15,570-byte Redo payload.
That read-only comparison does not establish input or distinct-process scene
loading. The later sessions 09/10 above provide the separate observed cancellation
and authored restart evidence for the repaired candidate.

The live check uses actual Sky mouse/keyboard input and an opt-in native Editor
backbuffer observer because the Windows capture provider timed out after its
bounded retry. `--validation-capture-directory <existing-absolute-directory>`
accepts a fresh output token in `capture.request` and records only this Editor's
pixels, PID, HWND, frame sequence, timestamp, client bounds and DPI. It does not
inject input or edit terrain. Each capture keeps `interactiveInputVerified=false`;
input actions and saved content require separate reconciliation.

The first terrain-component selection stalled. Retained
[thread stacks](../artifacts/terrain/gt0-20261029/editor-live-03/selection-stacks.txt)
show Render holding graphics/texture ownership while waiting for Lua, and an
Application selection callback holding Lua while waiting for the texture manager.
Terrain callbacks now use the existing Application event queue and an explicit
cached viewport snapshot in ordinary Editor mode as well as debugger mode.
Selection/lifecycle generations discard queued rays from retired selections.
The rebuilt Editor subsequently opened the terrain panel without this stall.

The normal Inspector also clipped the final terrain tabs because two nested
windows forced a 780-pixel width. Both now fill their available width; the live
check reached **Tools/Debug** in a 520-pixel Inspector, imported the deterministic
65x65 fixture through **Reload**, and wrote a native schema-v3 baseline through
**Save**. A scene was saved through the native **Save Scene As** dialog and opened
in a distinct Editor process. These partial checks do not establish sculpt,
undo/redo, cancellation or authored restart persistence.

The next stroke exposed an additional callback boundary: LuaManager passes a
Parameters container and mutable results container, while terrain getters used
ordinary Lua returns and its brush method expected positional arguments. Terrain
now has explicit adapters for the existing ABI, preserving direct Lua methods and
the global invocation contract. The real LuaManager regression exercises returned
brush state and native terrain edits/cancellation/commit/undo/redo. Its execution
and the next live authoring result are recorded separately from the earlier
direct-Lua fixtures.

With those adapters active, live diagnostics confirmed the cached brush was
enabled with a 20m radius, but the pointer still produced no hit. The viewport
passed normalized UVs to the pixel-oriented `GraphicsCamera::getRay`, which divided
them again by camera screen dimensions. The tool path now reuses
`makeScenePickingRay` with actual image origin/size/mouse coordinates and the
camera matrices. Invalid input emits a finite cancel payload; ordinary city-tool
hover outside the image retains its existing draft semantics. The regression
projects known terrain points into displaced/scaled viewports and intersects the
ray through the same production adapter; a real Lua/native case checks rollback
on invalid projection. The active implementation lives in static WPImGui linked
into WPGraphics, so validation requires a WPImGui rebuild and WPGraphics relink.

The earlier [optimized package refresh 03](../artifacts/terrain/gt0-20261029/package-refresh-03/final-verification.json)
passed all seven subprocesses for retained source
`C5CA5176ACA930A81024D5BADDE47CB7092ED8149BBA7547BDE224199111FDB4`.
Copied/extracted driving took 509.15s/495.62s under the unchanged 600s limit during
concurrent WARP Editor work. Its archive SHA256 is
`24E03A14D5C6815EB75E241E156B95448ECA02A0D197F5AB6A851F4A836DF830`.
[Offline kit 02](../artifacts/terrain/gt0-20261029/clean-machine-kit-02/README.txt)
pins that archive and its 38 payload files. This pass predates the ray repair; neither
it nor the earlier all-suite runs certify the repaired runtime bytes.

The later [optimized package refresh 04](../artifacts/terrain/gt0-20261029/package-refresh-04/final-verification.json)
reconciles the `6C5D806F...` ray repair with both nine-test baseline reports and
the [optimized gameplay package](../artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-04/gameplay-package-evidence.json).
All seven package subprocesses passed, including copied and ZIP-extracted driving,
two separate captures and both precise negative causes. Driving took
456.36s/417.01s under the unchanged 600s watchdog during concurrent WARP Editor
work; these are loaded-host diagnostic timings. Its
[archive](../artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-04/WorkphoneTerrainGameplay-RelWithDebInfo.zip)
SHA256 is `30E9BBC821A600B692476134ED2D8B4CBFFC3D2F32D808CDC6D914B320102894`.
All 38 payload files match their inventory; the ZIP also contains four directory
entries. The Debug gameplay ZIP was not refreshed and remains historical
`E496F8DB...` evidence. Package 04 predates the release-outside repair and is
superseded for current optimized validation by package 05 above.

The user requested an offline kit instead of changing this host's disabled guest
features. [Prepared kit 03](../artifacts/terrain/gt0-20261029/clean-machine-kit-03/README.txt)
and its [preparation evidence](../artifacts/terrain/gt0-20261029/clean-machine-kit-03/kit-evidence.json)
pin package 04's archive and all 38 payload files. Its guest runner inventories
Windows/software/CRT state before the loader probe, retains the original driving
checks and 600-second watchdog, and checks ZIP-extracted execution, captures,
precise negative causes and installed hashes. Optional provisioning requires an
explicitly supplied, pinned Microsoft-signed x64 redistributable after the
unprovisioned probe; Prepare downloads or installs nothing.

The kit's Sandbox configuration maps only read-only payload and writable evidence,
with networking and vGPU disabled. Launch requires the Windows Sandbox feature
already enabled and no existing Sandbox session. Collect rejects prepared-only
evidence, a fresh user or a restricted PATH as clean-OS proof. On this host
Sandbox/Hyper-V/Containers are disabled and no usable guest runtime is installed.
No guest, installer, host
feature change or reboot occurred. Kit 03 remains prepared-only with no guest
output or supplied prerequisite. The earlier
[kit 01 status](../artifacts/terrain/gt0-20261029/clean-machine-kit-status.json)
is retained for its `E496F8DB...` archive and point-in-time environment audit;
it does not describe package 04 or kit 03.
Actual clean-machine, physical-GPU and independent visual acceptance remain open.

`Tools/ValidateTerrainAuthoringEvidence.ps1` compares separately saved native v3
baseline/paint/undo/redo/cancel/scene/restart artifacts, while keeping content
matching distinct from input, native-loader, process-restart and visual proof.
The prepared walkthrough and input are in
[editor-authoring-kit-01](../artifacts/terrain/gt0-20261029/editor-authoring-kit-01/README.md).

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
