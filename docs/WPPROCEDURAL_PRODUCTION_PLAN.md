# WPProcedural production and racing visual quality plan

Reviewed 12 October 2026 at `dafc62e4c`. Status: implementation in progress, not visual certification. The [implementation status](WPPROCEDURAL_IMPLEMENTATION_STATUS.md) records the first runtime, sample, export and authoring increment with its evidence limits. Packages below remain the complete programme; no comparison win is claimed.

The goal is a beautiful, coherent, drivable procedural racing world with production authoring and packaging. Procedural generation must serve art direction: plausible proportions, composition, restrained variation and convincing materials matter more than the number of generators. A small excellent circuit is the first deliverable; city scale follows after that slice passes.

This document owns the cross-system programme and racing showcase. Existing [terrain/procedural review](WPGRAPHICS_TERRAIN_PROCEDURAL_REVIEW.md), [foliage plan](WPGRAPHICS_FOLIAGE_PRODUCTION_PLAN.md), [foliage workflow](FOLIAGE_WORKFLOW.md), [terrain workflow](TERRAIN_WORKFLOW.md), [graphics plan](WPGRAPHICS_PRODUCTION_PLAN.md), [graphics optimization plan](WPGRAPHICS_OPTIMIZATION_PLAN.md), [asset production plan](WPRESOURCE_ASSET_PRODUCTION_PLAN.md), [animation plan](WPANIMATION_PRODUCTION_PLAN.md), [physics plan](WPPHYSICS_PRODUCTION_PLAN.md) and [audio plan](WPAUDIO_PRODUCTION_PLAN.md) retain their detailed contracts and release gates. This plan adds no exemption from those gates.

## Reference and definition of success

The named commercial reference is **EA SPORTS F1 25: 2026 Season Pack / 2026 Season Edition**. EA identifies this as its 2026 season release: [official product page](https://www.ea.com/games/f1/f1-25), [official launch announcement](https://news.ea.com/press-releases/press-releases-details/2026/Experience-the-Definitive-New-Era-of-Formula-1-EA-Sports-F1-25-2026-Season-Pack-Available-Now/default.aspx). These sources establish the reference identity, not its measured image quality or our parity. Record the actual reference build and settings when comparisons are made.

“Perfect” is not a finite engineering acceptance criterion. Use repeated measured improvement and independent visual review, with a declared scope. Passing a blind preference study supports a claim about the tested footage, hardware and conditions; it cannot certify every scene or every viewer. No side-by-side evaluation has been performed for this document.

Two acceptance tracks are required:

- **Racing visual slice:** one deliberately composed circuit sector with a hero vehicle, pit/garage frontage, road materials, safety furniture, terrain and vegetation. Dry daylight first; wet overcast, sunset and night follow. Static images and driving footage must both pass.
- **Procedural production library:** reproducible recipes, working services, robust geometry/materials, Editor save/reopen/undo, cancellation, coherent collision, streaming, cooking and source-free runtime packages. A beautiful fixed screenshot cannot satisfy this track.

## Current foundation and source gaps

Historical reviews describe older revisions. Current source and newer workflow documents take precedence; do not turn old findings into a new claim of missing functionality.

The [12 October implementation status](WPGRAPHICS_IMPLEMENTATION_STATUS.md#native-frame-and-geometry-ownership--12-october-2026) is the latest retained visual baseline: native geometry/frame ownership and four procedural tree variants advanced, but the final sample still failed its AAA visual review. Its WARP run reported 207.971 ms mean and 221.204 ms P95 over 29 measured frames; this is neither representative hardware performance nor the 120-second acceptance run proposed below. The documented art inventory lacks production racing assets, and native GPU HDR/TAA/SSR remains open. These are recorded earlier results, not tests rerun for this plan.

| Area | Foundation observed | Gap / next verification |
|---|---|---|
| Services | [ProceduralServices.cpp](../Engine/cpp/Source/WPProcedural/ProceduralServices.cpp) registers road, sky, texture and vehicle service implementations | Audit each advertised operation through actual consumer output; registration is not completeness |
| Generic mesh service | [MeshGeneratorDefault.cpp](../Engine/cpp/Source/WPProcedural/MeshGeneratorDefault.cpp) still returns empty road/terrain/object meshes and ignores quality/LOD setters | Implement adapters to working generators, validated settings, diagnostics and explicit unsupported results |
| Model geometry | [ProceduralModel.cpp](../Engine/cpp/Source/WPProcedural/ProceduralModel.cpp) has an empty `computeBoundingBox` | Define a retained bounds contract and validate finite vertices, indices, winding, seams and tangents before publication |
| Texture export | [ProceduralTextureBindings.cpp](../Tools/cpp/Editor/src/procedural/ProceduralTextureBindings.cpp) writes P6 bytes regardless of the format argument | Implement encoders with alpha/precision/semantic handling or reject unsupported formats; decode exported artifacts in tests |
| Terrain | [Current workflow](TERRAIN_WORKFLOW.md) documents the connected authoring/runtime path beyond the original audit | Preserve current terrain work; prove road grading, shoulder seams and render/query/collision revision agreement in racing scenes |
| Foliage | [Current workflow](FOLIAGE_WORKFLOW.md) describes authoring, deterministic scatter, page streaming, per-view LOD, instanced bins, wind and textured impostors | Integrate that path into the sample; improve artistic species quality and assess hard LOD switches in motion. Imported meshes initially filling all LOD slots need actual reductions |
| Racing scene | [ProceduralRaceSceneBuilder.cpp](../Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp), [OpenCityLayout.hpp](../Engine/cpp/Include/Workphone/Interface/Procedural/OpenCityLayout.hpp), [open-city guide](RacingGameOpenCity.md) provide reuse points | Verify composition, junctions, terrain fit, collision and route continuity; do not build another racing lifecycle |
| Prototype appearance | The racing builder uses small generated circuit maps, merged tree patches, box-based city facades and synthetic environment reflection content | Replace through P2–P6 with calibrated material detail, current foliage-world integration, architectural kits and verified environment lighting; retain recent tree improvements as baseline |
| Legacy world orchestration | [CWorldGenerator.cpp](../Engine/cpp/Source/WPProcedural/CWorldGenerator.cpp) has road/city/vegetation TODO branches and returns early when terrain generation is disabled; [CCollisionManager.cpp](../Engine/cpp/Source/WPProcedural/CCollisionManager.cpp) ray methods return false | Declare supported entry points, implement independent generation switches and chunk placement, and route collision through working engine services before advertising world generation |
| Showcase | [SampleVehicleAdvanced.cpp](../Samples/cpp/VehicleAdvanced/SampleVehicleAdvanced.cpp) already supports seed, quality, captures, clean HUD, LOD overrides, benchmark, resolution and smoke modes | Extend this harness with recorded replay, stable camera tracks, weather/material inspection and evidence output rather than a separate demo |
| Coverage | [Service tests](../Tests/cpp/ProceduralServices/ServiceTests.cpp), [client contracts](../Tests/cpp/ProceduralServices/ClientContracts.cpp), [foliage tests](../Tests/cpp/FoliageContractsTests.cpp), [sample Lua tests](../Tests/lua/SampleVehicleAdvancedTests.lua) exist | Inventory registrations and executed assertions; build matching configurations and distinguish skipped dependencies from passes |

## Architecture contracts

1. WPProcedural owns renderer-neutral generation recipes and CPU candidates. Workphone owns scene/component lifecycle; WPGraphics owns GPU publication; physics owns collision publication. Reuse existing job systems, terrain snapshots, foliage streaming, `LODGroup`/`LODSystem`, catalog and ResourceSystem. Extend existing interfaces before creating parallel services.
2. A recipe contains schema/generator version, stable seed, metres/unit convention, normalized parameters, input UUID/content hashes, output profile and dependencies. Stable traversal and domain-separated random streams prevent changing foliage from reshuffling buildings. Declare same-platform reproducibility and cross-platform tolerance separately.
3. Generated outputs contain meshes/submeshes, bounds, UVs/tangents, semantic material maps, collision, LODs, sockets and provenance. Validate before commit; diagnose recipe, chunk and operation. Empty valid output must be distinguishable from failure.
4. Workers prepare immutable candidates with work/memory limits and cancellation. Scene/project/device generations guard publication. Failed generation or upload retains the last complete visible asset. Cancel/drain on unload; never publish into a reopened scene from an obsolete job.
5. Rendering and collision use a coherent published revision for gameplay. Pending edits may have a labelled preview, but cannot quietly replace road visuals while old collision remains active. Stable generated IDs support selection and sparse artist overrides through regeneration.
6. Catalog/ResourceSystem own durable identities, dependency graphs, cooking and package manifests. Preserve subasset IDs on rebake; avoid creating durable resources for every preview. Ship cooked worlds without generator DLLs when generation is not a runtime feature.
7. Shared streaming policy budgets terrain, city, foliage, textures, collision and uploads. Cell-local coordinates and an explicit large-world origin policy prevent precision drift. Multi-view visibility, shadow demand and teleport recovery must be tested; one camera cannot globally hide another camera's assets.

## Art direction and feature coverage

Start with a coherent temperate motorsport venue: restrained colours, physically plausible roughness, layered roadside vegetation and a readable circuit silhouette. Establish a reference board, measured asset dimensions, palette, sun/exposure and material library before randomizing assets. Use procedural assembly of high-quality authored modules where appropriate; noise alone does not create believable cars or architecture.

| Domain | Required implementation | Visual acceptance |
|---|---|---|
| Circuit and roads | Spline centreline with arc-length sampling, stable frames, banking/camber, elevation constraints, variable widths, junction grading, bridges/tunnels, kerbs, runoff, pit entry and lane transitions | No gaps, flipped normals, stretched markings or wheel penetration; continuous shadows and terrain shoulders at race speed |
| Road materials | Calibrated asphalt aggregate, macro wear, rubber line, repairs, drains, paint, kerb chips, seam decals; distance-aware detail and semantic mips | Grounded closeups and stable distant detail; no tiled checker pattern or shimmering paint |
| City structure | Road hierarchy, blocks/parcels, setbacks, elevation-aware lots, zoning, landmarks, skyline rules, plazas, service access and navigation graph | Plausible streets and sightlines; varied silhouettes with a consistent architectural language |
| Buildings | Modular facade grammar, floor/door scale, corners, roofs, parapets, windows/interior proxies, signage, gutters, HVAC; shared material families and LODs | No floating modules, intersecting doors, repeated noise facades or bright leaking interiors |
| Hero vehicle | Curated body surfaces, bevels, panel gaps, wings, cockpit, driver, brake/suspension/wheel details, correct pivots and sockets; silhouette-preserving LODs | Convincing at cockpit and close chase distance; wheels contact the surface and suspension/steering follow physics |
| Vehicle appearance | Layered paint/clearcoat where supported, carbon fibre with correct scale, rubber, glass, metals, decals/liveries, heat/wear/dirt masks | Controlled highlights, legible decals and plausible reflections; inspect under neutral and scene lighting |
| Motion and FX | Stable camera and vehicle interpolation, wheel blur policy, exhaust/heat effects, brake glow, skids, sparks, dust and tyre spray driven by contact/surface state | Effects originate at correct contact points; no floating trails, transparent sorting failures or camera judder |
| Vegetation | Art-directed tree species, branch silhouette, bark/leaf maps, ground cover, biome/exclusion rules, terrain grounding, wind and interaction using current foliage services | Convincing canopy/understory, stable shadows and distant coverage; no repeated identical pine walls or obvious billboard turns |
| Terrain and geology | Shared height/layer data, erosion/drainage masks, rocks, cuttings, retaining walls, road grading, shoulders and terrain LOD | Natural broad forms and convincing road integration; no seams, hovering rocks or obvious texture bands |
| Props and venue life | Barriers/fences, marshal posts, gantries, pits, grandstands, seating, lights, cables, service vehicles and clutter grammars; density/exclusion rules | Functionally placed furniture, plausible scale and contact shadows; no noise-based prop spraying |
| People and traffic | Budgeted animated spectators, marshals, drivers and background traffic with placement/navigation constraints | No identical synchronized crowd motion, unsafe road placement or static foreground cardboard |
| Lighting and sky | Consistent sun/sky/IBL/exposure, calibrated PBR, stable shadows, local lights, probes/reflections, fog and atmosphere | Car and world share lighting; no crushed cockpit, clipped sky, detached shadows or flickering reflections |
| Weather and water | Coherent wetness masks, puddles/drainage, wet road reflections, rain/spray/visibility, river/lake shorelines; integrate planned water tier | Wet surfaces remain material-dependent; puddles respect topography and no uniform mirror asphalt |
| Image quality | Stable AA/temporal history, mip/anisotropic filtering, depth precision, restrained bloom/DOF/motion blur, colour grading and reflection fallbacks | Moving foliage/fences/decals stay stable; no excessive blur hiding geometry or aliasing |
| Audio and interaction | Existing audio/presentation ownership, surface transitions, occlusion/reverb tier, distant ambience and gameplay collision | Physical contact and audible/visible response agree; actual output-device playback tested separately |

No universal texel-density or triangle count makes an asset AAA. Select budgets from camera distance, screen coverage and measured hardware cost. Record physical texture scale and near/mid/far quality for each asset family; approve material swatches before multiplying them across a city.

## Ordered implementation packages

Each package closes with reviewable outputs, focused contracts and real sample evidence. Timing is deliberately not estimated until P0 measures the baseline and identifies hardware/content staffing. Renderer, tools, art/content and QA ownership are roles, not assumed available people.

Backlog mapping: P1/P2/P9 elaborate the existing review's PROC packages and GP gates; P3/P8 consume its TERR packages and GT gates; P5 consumes foliage GF gates. P6/P7 depend on the parent graphics gates. Keep those original identifiers in implementation PRs alongside P0–P10 to avoid two competing completion registers.

| Package / dependency | Concrete work and owners | Exit evidence |
|---|---|---|
| P0: baseline and look target | QA/rendering/content: fresh sample build, fixed seed, four existing camera views, dry driving sequence, adapter/settings manifest, frame and residency counters; inventory services and art assets | Reproducible captures, known-defect register, agreed reference framing and baseline budgets; no parity claim |
| P1: reliable procedural outputs; P0 | Procedural/tools/resources: repair generic mesh service, bounds and validation; supported-operation diagnostics; recipe versioning; texture encoder correctness; deterministic IDs | Nonempty valid representative outputs, malformed-input rejection, replay determinism, last-good retention and cancellation tests |
| P2: material-to-screen; P1 | Resources/rendering/content: generated albedo/normal/ORM to existing cook/material path; tangent conventions, mips, colour spaces, texture scale, source-free load | Same generated material visible in swatch, Editor, sample and cooked package; failed rebake retains old visible material |
| P3: hero road sector; P1/P2 | Procedural/terrain/physics/content: closed circuit sector, camber/kerbs/shoulders, grading, barriers and garage composition with common collision revision | Close road inspection plus full driving run with no seams, wheel penetration or collision divergence |
| P4: hero vehicle; P2/P3 | Vehicle/rendering/animation/content: curated procedural assembly, body/wheel/cockpit materials, sockets, driver tier, LODs and physics-correct presentation | Car orbit, cockpit/chase and steering/braking replay; dry silhouette and highlight review passes |
| P5: vegetation and venue; P2/P3 | Foliage/content/tools: sample integration with current foliage world, species art pass, trees/grass, props, crowds tier, skyline and facade modules | Near/far and forced-LOD captures, driving transitions, per-pass draw reduction and stable multi-view shadows |
| P6: lighting and temporal quality; P2–P5 | Rendering/content: exposure/IBL/shadow calibration, reflection strategy, AA/filtering/history policy and grading | Dry daylight slice passes static and moving review at target performance; no critical image defects |
| P7: weather/night; P6 | Rendering/FX/audio/content: wetness/puddles, spray, rain, fog, lights and water tier; contact-driven effects | Matched wet, sunset and night replays; stable reflections/visibility and bounded FX cost |
| P8: city/world scale; P3/P5/P6 | World/tools/physics: editable road/rail/river splines, road hierarchy/parcels, terrain grading, authored overrides, explicit road/rail bridges and crossings, streaming budgets, traffic tier and origin handling | Signed-coordinate and boundary seed corpus, saved hand edits, matched terrain/render/collision publication, river/road/rail clearance and bridge approach probes, long high-speed route, teleport/reload and memory plateau evidence |
| P9: shipping workflow; starts P1, closes after P8 | Tools/resources/QA: native commands behind existing Lua editors, undo/redo, save/reopen, invalid recipe recovery, baking, source-free packages and presets | A fresh project authors, saves, reopens, cooks and runs the same world on another machine with complete dependency manifest |
| P10: independent comparison; P6 then P7/P9 | QA/art review: preregistered blind still/video study, regression audit and performance verification | Report actual wins/ties/losses, confidence and unresolved defects; repeat the weakest failed package until acceptance |

Deliver P0–P6 before broadening the city. P7–P10 remain required for the full requested scope. CSG, UV tools, texture graphs, erosion, biome tools and editable rule stacks from the existing comprehensive plans remain library requirements even if the initial hero sector can ship without them. Destruction, interiors, snow, oceans and specialized simulation need explicit feature tiers and their own measurable gates before advertising support.

## First executable backlog

1. **PR-PROC-01: evidence harness.** Extend the existing sample review path with a JSON evidence manifest and fixed-tick recorded replay. Record commit/local diff, binary/assets hashes, seed, recipe versions, camera, time/weather, internal/output resolution, AA/upscale settings, adapter/driver, timings and counters. Capture failure must return failure. Existing options below remain usable unchanged.
2. **PR-PROC-02: generic mesh contracts.** Audit callers of `MeshGeneratorDefault`, implement working road/terrain/object adapters and retained settings, and test invalid/empty/oversized outputs. Do not quietly concatenate incompatible materials or invent unsupported LOD success.
3. **PR-PROC-03: geometry and export correctness.** Define/update bounds, tangents and seam validation; implement supported texture export and round-trip decoding with semantic maps. Add cancellation and last-good behavior around candidate publication.
4. **PR-PROC-04: one generated PBR road material end to end.** Generate, bake, catalog-resolve, cook and draw it through existing infrastructure, then load its package without source assets. Verify material parameters with neutral lighting and actual GPU pixels.
5. **PR-PROC-05: sample foliage integration.** Replace the sample-specific pine presentation through an explicit selectable preset using the existing foliage authoring/world path; preserve its old preset as a measured baseline. Compare coverage, all-pass draw calls, overdraw, memory and LOD stability.
6. **PR-PROC-06: road/vehicle hero slice.** Approve the road section and hero car separately, then together in the same lighting. Record comparison failures before adding more random content.
7. **PR-PROC-07: active DX11 lighting pipeline.** The current DX11 path applies fixed Reinhard/gamma in native PNTC shaders and bypasses the software-only HDR post bridge. A float reflection source alone does not close this package. First select the showcase's owned sun explicitly and record its active direction/power. Then implement linear HDR scene targets and final display transfer through the native C89 frame/pipeline ownership path, using the established narrow backend adapter. Lit, unlit, sky, transparent and particle passes must agree on linear scene output; compose display UI after the single tone map. Reuse the existing AgX/exposure policy and native frame delta rather than adding sample-owned post sequencing. Verify linear values above one, reference transfer swatches, exactly-once tone mapping, resize/device recreation and failed-allocation retention on the actual DX11 path. Record whether HDR post is active in each sample manifest. Temporal AA, diffuse environment irradiance, material clearcoat response and hardware performance remain separate exit requirements.

The [implementation status](WPPROCEDURAL_IMPLEMENTATION_STATUS.md) tracks delivered portions of these changes and their remaining acceptance work. An implemented portion does not close an entire package automatically.

## City authoring, terrain, rivers, bridges and rail

The requested city workflow now includes drawing and moving road splines by hand, retaining edits through regeneration, fitting the city to a selected terrain, editable rivers, road/rail bridges, train tracks and roundabouts. See [the source/Editor/terrain design](WPPROCEDURAL_CITY_AUTHORING_PLAN.md) and [transport, water and crossing design](WPPROCEDURAL_CITY_TRANSPORT_WATER_PLAN.md). These extend the same P3/P8/P9 ownership and acceptance gates rather than introducing a second world-generation lifecycle.

The bounded runtime currently supports a flat grid or seeded L-system street graph. The new spline source codec and point-sampled terrain fitting are CPU foundations, not an Editor drawing tool or a terrain-conforming city renderer. Rivers, bridge geometry/collision and rail generation remain design work. Compile/test/capture evidence for the existing city must not be reported as those features working.

## SampleVehicleAdvanced review workflow

The actual target is `SampleVehicleAdvanced` (with the final “d”). Reuse native [sample](../Samples/cpp/VehicleAdvanced/SampleVehicleAdvanced.cpp), [Lua sample](../bin/Media/Scripts/Lua/Game/Core/SampleVehicleAdvanced.lua), `ProceduralRaceScene` and `RacingGameFull`/`RacingGameOpenCity` lifecycle. Keep effects/audio ownership shared and avoid per-frame duplicate Lua systems.

Existing options, verified from source: `--seed`, `--quality low|medium|high`, `--plugins`, `--capture`, `--view follow|car|track|corner`, `--no-hud`, `--resolution`, `--force-lod auto|0|1`, `--force-vehicle-lod auto|0|1|2`, `--benchmark`, `--orbit`, and separate vehicle/track/collision/audio/effects smoke modes. Smoke and capture modes must be separate runs. Capture currently means a rendered frame, not proof of a drivable or visually accepted scene.

Example from a shell already configured with the correct working directory and matching plugins:

```powershell
& 'G:\Workphone\bin\windows\v145\x64\MD\RelWithDebInfo\SampleVehicleAdvanced.exe' --seed 42 --quality high --capture 'G:\Workphone\artifacts\procedural\baseline-car.bmp' --view car --no-hud --resolution 1920x1080 --no-audio
```

Create the output directory first; rebuild the executable and verify plugin/backend configuration before collecting evidence. This command is documented, not executed in this review. Add proposed showcase presets (dry circuit, wet circuit, sunset, night, city boulevard, vegetation stress), replay/camera-track selection and material debug views through the existing sample options. Proposed options must not appear as working commands until implemented.

## Performance and stability gates

P0 records the actual GPU/CPU, driver, backend and configuration; no retained binary or historical RTX capture certifies current source. Proposed initial baseline is 1920×1080 native, DX11, high quality, 60 Hz on the declared target machine. A separate 2560×1440 profile is a later measured target. Report internal resolution, upscaling, dynamic resolution, RT and quality differences in every reference comparison.

- After warmup, run the same 120-second replay three times: proposed P95 CPU frame and GPU frame each ≤16.67 ms, P99 presented interval ≤33.3 ms and no gameplay streaming stall over 50 ms. Report all samples, medians and worst run; averages alone do not pass.
- Profile generation, scene update, submission, GPU passes, shadow/colour/reflection draws, upload bytes, triangles, foliage visibility, resident pages, texture memory and collider counts separately. Provisional starting caps are 1 ms/frame streaming publication on CPU and 16 MiB/frame uploads; P0 must validate or revise them with evidence before implementation budgets are locked.
- Set absolute CPU/GPU residency limits from measured target capacity with documented headroom. During a 20-minute loop including regeneration and teleports, residency must plateau within the cap after eviction settles. Unbounded growth, crashes, NaNs, stale publication, device errors or silent missing assets fail.
- Force near/far LODs and compare silhouettes, screen coverage and pass counts. A lower triangle count alone does not prove fewer calls or better frame time. Inspect alpha overdraw, hard transitions, fence shimmer and distant road markings in motion.
- Run focused CPU generation/serialization contracts, GPU material/foliage contracts, Lua/component lifecycle tests, then representative packaged driving and Editor workflows in matching Debug and RelWithDebInfo configurations. Missing plugins/assets/devices are unavailable evidence, not a passing release test.

## Blind comparison and iteration loop

1. Obtain actual gameplay captures of the declared EA reference build and the current Workphone build. Match scene category, field of view, camera height, speed, weather, time of day, resolution, frame rate and compression as closely as possible. Keep a settings/deviation sheet. Use original Workphone content; do not require copied commercial assets.
2. Freeze an evaluation set before tuning: cockpit acceleration, chase corner, car orbit, road/kerb closeup, pit straight, vegetation pan, city street, wet braking and night pass. Mark categories the reference does not cover as separate Workphone reviews rather than fabricated matched comparisons.
3. Present randomized A/B pairs with no HUD, filenames or engine labels; randomize left/right and video order. Preserve full frames and equal encoding. Report that recognizable cars/venues can reveal identity, so blinding is imperfect. Use held-out seeds/cameras to expose overfitting.
4. Collect independent ratings for overall preference, vehicle fidelity, materials, lighting, environment composition, temporal stability and effects. Allow ties and record reasons. A proposed minimum panel is 20 independent reviewers with both experienced artists and regular racing players; the implementing agent is not the independent panel.
5. Proposed success criterion: Workphone is preferred on at least 60% of non-tied primary votes and the participant-clustered 95% confidence interval is above 50%, with no category showing a material recurring defect. Predeclare tie handling, primary metric and sample size; repeated ratings by one reviewer are not independent participants. An inconclusive interval requires more evidence, not a win claim.
6. Review full-resolution footage with a defect log: severity, reproducible seed/view/time, owner, suspected cause, screenshot/video and regression case. Fix the largest visible failures, rerun relevant contracts and replay, then repeat blinded evaluation on a held-out set. Track quality and performance together; an attractive still at unusable frame time fails.
7. End an iteration only with recorded pass/fail evidence. Ship the scoped slice when both visual and production gates pass; keep unimplemented domains explicitly open. If reference footage, target hardware or reviewers are unavailable, finish the implementation work possible and report the comparison gate as unverified.

## Editor, packaging and release checklist

- Existing procedural model/texture/terrain editors expose supported native operations with progress, cancellation and actionable failures. Rule/graph output selection is explicit, cycles/errors diagnosed, and evaluation is deterministic.
- Brush strokes and rule changes are native undo transactions; save/reopen preserves seeds, input references, species, manual edits and selected generated IDs. Regeneration does not erase artist overrides or duplicate colliders/resources.
- Bake outputs contain validated geometry, material maps, collision, LODs and provenance. Package manifests include all texture/mesh/species/animation dependencies and version checks. Missing/obsolete dependencies fail clearly.
- Fresh-project and second-machine tests exercise author → preview → save → reopen → bake → cook → package → drive. Include reset, unload/reload, cancellation, failed generation, failed upload and device recovery at supported tiers.
- Publish source/build identity, hardware/settings, contract results, actual images/videos, performance distributions, blind-review report and unresolved defects. Keep library contracts, Editor acceptance, package acceptance and visual comparison as distinct outcomes.

## Completion record for this change

Delivered planning: source-informed production plan, ordered implementation backlog, sample reuse strategy, visual feature coverage, proposed performance targets and an independent comparison protocol. Runtime implementation has begun; see the [implementation status](WPPROCEDURAL_IMPLEMENTATION_STATUS.md) for actual code changes and verification. The complete programme and independent visual gates remain open.
