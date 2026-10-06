# WPGraphics production readiness and feature implementation plan

Date: 6 October 2026  
Status: Proposed implementation roadmap, based on repository inspection.  
Scope: WPGraphics/Claw, its native WorkphoneGraphics dependencies, and the engine/editor integration needed to ship it.  
User priorities: explicitly validate animation and particle systems; include water rendering in the feature roadmap.

## 1. Intended outcome

Deliver a renderer that can ship a Workphone game and reliably drive the editor: static and animated geometry, production materials and lighting, particles, terrain, UI, and a documented water feature tier. Each advertised feature must have a complete asset-to-screen path, predictable failure behavior, automated coverage, and measured performance.

Use two release gates:

- **R1 — production core:** Windows x64/DX11, cooked assets, static and skeletal meshes, basic animation graphs and IK integration, CPU particle simulation with batched GPU rendering, essential lighting/shadows, GPU HDR presentation, terrain, UI, diagnostics, and release packaging.
- **R2 — comprehensive feature release:** R1 plus richer animation, particle authoring and effects, water for lakes/pools/rivers, reflection probes, scalable lighting, temporal effects, streaming, and the broader feature set below.

DX12 and additional platforms receive their own certification gates. They do not inherit production status from DX11. Advanced ocean simulation, ray tracing, and virtualized geometry are subsequent optional work.

This plan does not certify the current build. Source and test code were inspected; builds, tests, GPU captures, and benchmarks were not run for this planning task. An implementation or test file establishes a starting point, not successful runtime behavior.

## 2. Evidence and current gaps

Paths below are relative to the repository root. Observations are limited to the inspected paths.

| Area | Evidence | Implementation implication |
|---|---|---|
| Architecture | [WPGraphics build](../Engine/cpp/Project/WPGraphics/CMakeLists.txt) links `WorkphoneGraphics`; [native build](../Engine/c/Project/WorkphoneGraphics/CMakeLists.txt) requires C90 without extensions | Keep C++17 integration and the native C90 contract; improve the existing layers |
| Backends | `ClawRendererDX11.cpp`, `ClawRendererSoftware.cpp`, `ClawRendererDX12.cpp`; engine-wide README also lists other backends | Publish a WPGraphics-specific support matrix; engine-wide support is not Claw certification |
| Post-processing | [ClawGraphicsPipeline](../Engine/cpp/Source/WPGraphics/ClawGraphicsPipeline.cpp) creates CPU effect buffers; [ClawHammerSystem](../Engine/cpp/Source/WPGraphics/ClawHammerSystem.cpp) restricts capture/presentation to the software default-window path | Implement a GPU pipeline for DX11; existing TAA/GTAO/SSR/etc. APIs do not establish GPU integration |
| DX11 materials | `ClawRendererDX11.cpp` has material/channel mapping, mesh caches, and sky environment caches; native DX11 has material and statistics APIs | Extend and test the existing material path; audit cache ownership, invalidation, and bounded growth |
| DX12 | [DX12 source](../Engine/cpp/Source/WPGraphics/ClawRendererDX12.cpp) uses an offscreen target, quad-oriented draw paths, and waits for the GPU at frame end; its header returns a null native renderer for UI integration | Treat as experimental; complete mesh/material drawing, presentation, UI, and per-draw resource safety before certification |
| Animation | [ClawMesh](../Engine/cpp/Source/WPGraphics/ClawMesh.cpp) delegates animation/controller/skeleton methods to `GraphicsMesh`; base vertex-processing hook is empty; native skeleton header contains bone identity storage | Trace import through final deformation. Implement missing controller, palette, vertex attributes, and skinning integration instead of assuming skeletal rendering exists |
| Animation tests | [Graph tests](../Tests/cpp/UnitTests/AnimationGraphTests.cpp) and [IK tests](../Tests/cpp/UnitTests/AnimationIKTests.cpp) exercise time/events/graph sampling and solvers | Retain these tests; add pose evaluation, import, skinning, rendered output, and scene/component tests |
| Particles | [CParticleSystem](../Engine/cpp/Source/WPGraphics/Particle/CParticleSystem.cpp) has empty load/unload and technique lookup; [BillboardRenderer](../Engine/cpp/Source/WPGraphics/Particle/Renderers/BillboardRenderer.cpp) update is commented out; manager contains empty methods | Establish one authoritative simulation/rendering path and repair lifecycle and visible rendering |
| Particle tests | [ParticleSystemTests](../Tests/cpp/UnitTests/ParticleSystemTests.cpp) can return early in headless/unavailable-backend cases; [component tests](../Tests/cpp/UnitTests/ComponentTestsParticleSystem.cpp) use a test renderer | Make skipped/unavailable cases visible in CI; add real Claw rendering tests and deterministic simulation tests |
| Water | [IGraphicsWater](../Engine/cpp/Include/Workphone/Interface/Graphics/IGraphicsWater.hpp) exists; searched water/ocean paths found no concrete implementation | Add a Claw water object and rendering passes; reuse the interface where applicable |
| Resources | [ResourceSystem.md](../Engine/cpp/Project/Workphone/ResourceSystem.md) describes versioned compilation, dependency tracking, atomic publication, runtime validation, and caching | Integrate graphics compilers and GPU upload/residency with this system rather than creating another database/cache |
| Tests | [C++ registration](../Tests/cpp/CMakeLists.txt) includes Claw text, DX11 target, UI destruction, and cubemap/PBR targets; C tests cover renderer/shader/pipeline contracts | Preserve regression coverage and extend it to complete frames and failure/recovery paths |
| Editor | `AnimationEditor.lua`, `ParticleEditor.lua`, `MaterialEditor.lua`, and `ShaderEditor.lua` exist | Extend existing authoring tools and bind native capabilities |
| Build/distribution | Root presets cover Android/iOS; WPGraphics build uses recursive source globbing and platform library paths; no root `.github` directory was present | Add reproducible graphics presets, explicit platform sources/dependencies, CI, install/export rules, and an external consumer test |

## 3. Scope and engineering rules

1. **Proposed first supported configuration:** Windows x64, DX11, C++17 wrappers, C90 native modules, SDR output. Select supported OS versions, adapter feature levels, toolchain, and driver versions during M0; record them rather than relying on the old README.
2. **Software renderer:** a bounded fallback and headless/reference path. Certify a documented subset and fail explicitly for unsupported features. Do not promise GPU-quality parity or GPU frame rates.
3. **Keep existing engine contracts stable.** Prefer internal services, concrete implementation settings, and additive native APIs. Avoid new virtual methods in shared `Workphone/Interface` headers during this pass; document any eventual ABI migration separately.
4. **Editor work follows existing conventions:** extend Lua authoring UI; use C++ for native rendering and thin bindings. Respect [editor conventions](../Tools/cpp/Editor/docs/CONVENTIONS.md) and coordinate with the existing editor upgrade plan.
5. **One owner per responsibility:** Workphone owns animation graph/IK evaluation; WPGraphics consumes final poses and handles deformation/rendering. Define whether particle simulation lives in Claw or native C during M0; both layers must not independently advance the same effect.
6. **Explicit capabilities:** feature availability must describe implemented rendering behavior and limits. Unsupported requests produce a diagnostic or a documented fallback, never a successful no-op. Until full switching exists, expose renderer selection as a restart-required configuration.
7. **Separate simulation and presentation:** immutable, frame-stamped snapshots cross worker/render boundaries. GPU creation/destruction runs on the owning render context. Cancel and drain jobs before scene/device teardown.
8. **GPU residency:** normal GPU frames retain color/depth/normal/velocity data on the GPU. Use asynchronous readback for tests, screenshots, and diagnostics; avoid routine full-frame CPU round trips.
9. **Feature definition of done:** runtime integration, asset validation, serialization, authoring controls where relevant, tests, profiling, fallback behavior, documentation, and a sample scene.

## 4. Feature coverage

P0 is required for R1. P1 is required for the documented R2 feature tier. P2 is an extension with its own acceptance gate. Existing code must be reused where sound; “required” does not mean every capability must be written from scratch.

| Feature family | P0: production core | P1: comprehensive release | P2: extensions |
|---|---|---|---|
| Device/window | Adapter selection, resize/minimize, offscreen targets, vsync, recovery | Multiple windows/views, frame pacing, optional MSAA | HDR display output and specialist presentation |
| Geometry | Indexed static/dynamic meshes, submeshes, robust attributes, bounds | Instancing, LOD, occlusion, indirect draws where justified | GPU-driven submission, virtualized geometry |
| Animation | Import, pose evaluation, CPU reference/GPU skinning, clips, transitions, root motion, events, basic IK | Blend spaces, additive layers/masks, animation LOD, sockets, morphs, retargeting | Advanced warping and crowd systems |
| Materials | Unlit and metallic/roughness PBR, normals, packed channels, alpha modes, instances | Clearcoat, detail maps, anisotropy, decals, material quality variants | Subsurface/transmission models |
| Textures/shaders | Mips, sRGB/linear correctness, samplers, offline shader compilation, fallback shaders | Compression, arrays/cubemaps, streaming, safe hot reload | Virtual texturing and specialist codecs |
| Lighting | Directional/point/spot lights, bounded light lists, IBL, directional shadows | Forward+ light lists, local shadows, probes, baked lightmaps | Advanced GI, ray-traced lighting |
| Image pipeline | GPU HDR intermediates, tone mapping, basic AA, optional bloom/exposure | TAA, GTAO, SSR, contact shadows, DOF, motion blur, dynamic resolution | Vendor upscalers, advanced temporal reconstruction |
| Particles | Deterministic CPU simulation, emitter lifecycle, curves, pooling, batched billboards, alpha/additive | Soft particles, flipbooks, trails/ribbons, mesh particles, collisions, subemitters, lighting | GPU simulation and very large effects |
| Water | Architectural hooks and required buffers | Lakes/pools/rivers, waves, depth/absorption, Fresnel, foam, reflection/refraction | Ocean spectrum, underwater volumes, caustics, wakes |
| Terrain/vegetation | Existing terrain correctness, layer blending, stable bounds/LOD | Chunk streaming, foliage instancing/wind, impostors | Virtual terrain materials |
| Sky/environment | Existing sky/cubemap path, cached IBL | Atmosphere, time of day, fog, environment transitions | Volumetric clouds/weather |
| UI/text | Claw UI and ImGui, clipping, text/glyph lifecycle, DPI | Localization/complex text integration, multi-viewport behavior | Specialist text rendering |
| Tool integration | Material, animation and particle preview; picking/debug views | Water controls, effect diagnostics, render graph/profiler views | Specialized capture/inspection tools |
| Resources | Cook/load/version validation, bounded uploads, failure-safe reload | Residency budgets, prefetch, eviction, dependency reload | Large-world streaming specialization |
| Operations | Diagnostics, test matrix, clean packaging, symbols, migration notes | Backend parity reports, performance dashboards | Additional independently certified platforms |

## 5. Proposed rendering architecture

Preserve `ClawHammerSystem` as orchestration and existing wrappers as engine-facing objects. Introduce internal implementation units with names finalized in M0:

- **Device/context service:** capabilities, device generation, adapter/window state, native handles, device recovery, and GPU diagnostics.
- **Resource registry:** typed handles with generations, ownership, upload queue, per-device caches, residency accounting, and deferred destruction. Replace raw-pointer cache identity where it can survive deletion or address reuse.
- **Render scene snapshot:** transforms, current/previous poses, materials, lights, bounds, particle draw data, and water surfaces frozen for a frame/view.
- **Pass graph:** explicit resource reads/writes, formats, dimensions, clear/load behavior, lifetimes, execution order, and per-view history. Validate conflicting bindings; pool compatible transient resources.
- **Draw lists:** depth/shadow/opaque/alpha-test/water/transparent/UI lists with pass-appropriate sorting and bounded material variants.
- **Graphics asset compilers:** mesh/skeleton/clip/material/shader/texture/particle/water settings plugged into the existing resource compiler registry.

Start with explicit DX11 passes; avoid building a generic multi-API framework before the first complete frame. Reuse CPU effects as references where useful. DX12 can adopt these contracts after DX11 is proven.

Proposed frame flow:

```mermaid
flowchart TD
    A[Simulation: animation and particles] --> B[Immutable scene and view snapshot]
    B --> C[Visibility, bounds and draw lists]
    C --> D[Skinning, shadows and optional depth prepass]
    D --> E[Opaque HDR shading, depth, normals and velocity]
    E --> F[AO, lighting resolve and reflection inputs]
    F --> G[Water and transparent particles]
    G --> H[Temporal resolve and post processing]
    H --> I[Tone mapping, UI and presentation]
```

Detailed ordering is validated in M5/M7: AO/reflections must affect the correct lighting term; refraction samples a pre-water scene copy; planar reflection views suppress recursive water rendering; transparency has an explicit temporal policy; display UI is composed after tone mapping. Each camera/render texture owns its own history and exposure policy.

## 6. Milestones and work packages

Effort ranges below are provisional **engineer-weeks**, including implementation and feature-specific tests. They are not elapsed-time promises. M0 replaces them with measured estimates and a hardware/content baseline. QA hardware access and technical-art fixtures are dependencies throughout.

| ID | Deliverable | Depends on | Effort | Gate |
|---|---|---|---|---|
| M0 | Baseline, capability contract, fixtures, build/test inventory | — | 2–3 | G0 |
| M1 | Device, lifecycle, resource and threading hardening | M0 | 5–8 | G1 |
| M2 | Animation asset-to-screen implementation and validation | M0; GPU integration uses M1 | 6–10 | GA |
| M3 | Particle simulation, visible rendering and lifecycle | M0; GPU integration uses M1 | 5–8 | GP |
| M4 | Materials, textures, shaders, lighting and geometry | M1; coordinates with M2/M3 | 6–10 | G4 |
| M5 | GPU pass graph, shadows and image pipeline | M1, M4; temporal deformation uses M2 | 7–12 | G5/R1 |
| M6 | Scene scale, terrain, streaming, UI and tools | M1–M5 as relevant | 5–8 | G6 |
| M7 | Water and richer effects/animation feature tier | M2, M3, M5; streaming integration M6 | 5–8 | GW/R2 |
| M8 | Final certification, documentation and distribution | Applicable release packages | 3–5 | GR |
| M9 | DX12 certification; other backends separately scoped | Stable R1 contracts | 6–12 per DX12 track | GB |

R1 uses P0 subsets of these work packages and an initial M8 certification. R2 completes P1 subsets and repeats certification for newly enabled features. Broad serial effort for M0–M8 is 44–72 engineer-weeks; sequencing, reuse and staffing determine elapsed time. Track DX12/platform expansion separately. Do not schedule water or cosmetic effects ahead of resolving animation/particle blockers.

### M0 — establish an honest baseline

- **BASE-01:** Inventory backend selection, feature flags, API behavior, source registration, tests and skipped paths. Publish `supported / experimental / unavailable` capabilities and limits for each backend.
- **BASE-02:** Add Windows Claw presets for Debug and RelWithDebInfo, then static/shared coverage as packaging requires. Remove accidental dependencies on unrelated tools, proprietary directories and developer-local paths from the minimal graphics build.
- **BASE-03:** Run existing C renderer/shader/pipeline tests and C++ Claw tests. Capture actual failures and skips; inspect UnitTests filters and ensure animation/particle tests execute on Claw.
- **BASE-04:** Create licensed, versioned fixtures: material spheres, target/viewport grid, skinned two-bone strip, humanoid clips, particle effects, terrain, and water test geometry. Store source and cooked outputs with fixture hashes.
- **BASE-05:** Measure startup, CPU/GPU frame time, uploads, allocation counts, VRAM and skipped tests. Record adapter/driver/configuration and content complexity.

**G0:** Repeatable configure/build/test instructions work on a clean machine; known failures, skipped coverage, capability limits and budgets are documented. No feature gets promoted based on an API flag alone.

### M1 — safe lifecycle and platform behavior

- **CORE-01:** Audit load/unload/destruction, partial initialization rollback, reference cycles, native handle ownership and render-thread affinity across meshes, textures, targets, UI and scenes.
- **CORE-02:** Make caches device-owned, versioned and bounded. Invalidate on resource mutation, unload, device replacement and address reuse; preserve the existing repeated-draw regressions.
- **CORE-03:** Handle device removal/reset from presentation and resize, rebuild device resources from retained/cooked data, and propagate structured errors. Bound recovery retries and fall back or stop with a useful diagnostic.
- **CORE-04:** Validate dimensions, pitches, counts, formats and checked allocation arithmetic before native allocation/upload. Reject corrupted and unsupported assets without partial state publication.
- **CORE-05:** Define worker/render scheduling and frame snapshot ownership. Join/cancel work before teardown; prevent stale callbacks from publishing into a replacement scene or device.
- **CORE-06:** Support resize storms, zero-size/minimized windows, DPI changes, render-target switches and adapter selection. Distinguish configured backend from active backend.
- **CORE-07:** Add debug-layer messages, resource names, GPU markers, allocation/upload counters and actionable failure logs. Wire a regression runner to fail on relevant graphics validation errors.

**G1:** Resize/target-switch/repeated-load tests and failure injection complete without corruption, leaks or stale references. Recovery checks follow [Microsoft's Direct3D 11 device-removal guidance](https://learn.microsoft.com/en-us/windows/uwp/gaming/handling-device-lost-scenarios).

### M2 — animation: close the entire deformation path

Animation is an early blocking workstream. Existing graph samples and IK solver results must reach a correctly deformed mesh. Maintain a CPU reference implementation to diagnose GPU/import errors.

| ID | Implementation | Required evidence |
|---|---|---|
| ANIM-01 | Trace and complete importer → mesh skeleton → clip/controller → evaluated local/model pose → skin palette → render draw | One real imported character animates through the same cooked path used by a packaged application |
| ANIM-02 | Define joint ordering/remapping, inverse bind transforms, coordinate/unit conversion, matrix conventions, weights and influences; normalize valid weights and reject invalid indices/NaNs/cycles | Bind pose reproduces original vertices; shuffled joint indices and importer axis conversion fixtures behave correctly |
| ANIM-03 | Implement/validate translation/scale interpolation and shortest-path quaternion interpolation, missing/constant channels, loop/clamp, seek and reverse playback policies | Analytical expected poses at endpoints/midpoints; empty/one-key/zero-duration/duplicate-time cases handled explicitly |
| ANIM-04 | Bridge existing graphs to actual pose blending; clip playback, transitions, event delivery, pause/resume, root motion and basic IK | Idle/walk/run transitions deform correctly; root displacement is applied once; events survive wraps without duplicates |
| ANIM-05 | Implement skinned vertex attributes, per-instance joint palettes, CPU fallback and DX11 GPU skinning; handle normals/tangents and declared scale constraints | GPU positions/normals agree with CPU reference; two characters sharing assets retain independent poses |
| ANIM-06 | Use deformed geometry in depth and shadow passes; update conservative animated bounds; retain previous-frame poses for velocity | Limbs remain visible at frustum edges, shadows follow limbs, and skinned motion vectors are correct |
| ANIM-07 | Implement sockets/attachments and serialize skeleton/clip references; invalidate pose/palette caches safely on reload | Attached prop follows a bone; reload/unload while animation jobs run is safe |
| ANIM-08 | Complete P1 blend spaces, additive layers, masks, animation LOD, morph targets and retargeting; specify morph/skin order | Rendered reference poses and transition tests cover each advertised feature |

Specific tests and fixtures:

- A two-joint strip with analytically known bend; a rigid one-joint mesh; a humanoid; separate instances; skeletal LODs; morph targets.
- Imported hierarchy/inverse-bind validation, nonuniform scale policy, negative-scale policy, missing joints, bad weights and limit boundaries. Native header capacities are not a supported GPU bone budget; specify limits per backend and asset compiler.
- Unit tests for sampling/blending/root-motion/event math; component tests for update scheduling and graph/controller ownership; real Claw render tests for deformation, materials, shadows and velocity.
- Camera cuts, seeks, clip changes, teleport, LOD change and reload reset history appropriately. First-frame previous pose must equal current pose.
- CPU/GPU comparison via asynchronous buffer capture where available, with screen-space/image checks as a complementary oracle. Proposed position tolerance: `1e-4 × max(1, fixture extent)`; normal tolerance and scale policy are fixed in M0.
- Stress: 100 characters at the selected skeleton size, mixed clips, culling, pause/resume, rapid scene reload and repeated resource replacement. Benchmark graph/pose evaluation separately from skinning/draw cost.

**GA:** A packaged Claw sample imports and renders animated characters, independent poses, root motion, events, basic IK and animated shadows correctly. GPU integration tests run on the certified backend; graph/solver-only success cannot satisfy this gate. P1 features have additional explicit tests before R2 promotion.

### M3 — particles: complete simulation, drawing and ownership

First decide the authoritative path among `CParticleSystem`, techniques/jobs, inherited `ParticleSystem`, and native particle state. Repair the selected path; remove or clearly isolate obsolete entry points after checking consumers.

| ID | Implementation | Required evidence |
|---|---|---|
| FX-01 | Real load/unload, effect/technique/emitter creation, lookup, play/stop/pause/restart, and attachment behavior | A created effect loads, emits, draws and fully releases; component state reaches the renderer |
| FX-02 | Seeded RNG per instance, fixed simulation step, emission accumulators, bursts, lifetime/expiry and bounded catch-up/prewarm | Counts and particle state match analytical/seeded expectations for varying frame rates |
| FX-03 | Preallocated particle pools and draw buffers, bounded capacities, explicit overflow policy, safe reuse | No steady-state per-particle allocation; saturation is bounded and observable |
| FX-04 | Point/box/sphere/cone emitters, local/world space, velocity/gravity/drag, color/size/rotation curves and flipbooks | Local particles follow emitters as documented; world-space particles remain independent after emission |
| FX-05 | Batched/instanced camera-facing and velocity-aligned billboards, premultiplied/straight-alpha material rules, additive blending and sorting | Visible smoke/sparks match reference captures; alpha ordering and depth writes are correct |
| FX-06 | Simulation/render snapshots, bounds, culling/time policy, distance LOD, independent multi-camera draw orientation | Camera changes do not advance simulation twice; culling does not produce reentry explosions or frozen lifetime bugs |
| FX-07 | P1 soft particles, lighting/fog, mesh particles, trails/ribbons, collisions and subemitters with recursion budgets | Depth intersections soften; trails remain continuous; collision/subemitter behavior is bounded and reproducible |
| FX-08 | Versioned effects and safe resource reload; extend `ParticleEditor.lua` preview, curves, seed, playback, pool/overdraw stats | Editor and packaged application use the same effect asset and produce matching seeded results |

Required tests:

- Separate deterministic simulation tests from GPU rendering tests. Use independent analytical counts/trajectories, not only implementation-generated golden data.
- Boundary cases: zero/negative settings, invalid timestep, fractional emission, large delta time, finite/non-looping duration, expired particles, zero capacity and full pools.
- Stop-immediate versus stop-emitting-and-drain; pause versus timed pause; restart/prewarm; moving/scaled parent; disabled/hidden emitter; scene removal during pending jobs.
- Effects: fire, smoke, sparks, rain, a burst explosion and a ribbon. Compare reference captures for additive/alpha blending, flipbooks, sorting and depth intersection.
- Stress: 10,000 visible particles across multiple systems, then a 100,000-particle optional tier. Publish hardware/settings and simulation, submission, GPU and overdraw costs separately; particle count alone is insufficient.
- Run headless simulation in normal CI and real rendering on a GPU runner. Required GPU tests that cannot obtain their backend report skip/unavailable explicitly and block release certification.

**GP:** Seeded effects simulate and visibly render through Claw, respect lifecycle/space semantics, stay inside pool and frame budgets, and release cleanly. Settings serialization and mock component tests cannot satisfy this gate by themselves.

### M4 — assets, materials, geometry and lighting

- **ASSET-01:** Register graphics compilers with existing `ResourceSystem`; validate meshes, skeletons, clips, textures, materials, shaders and effects. Include target/format/compiler versions in derived outputs.
- **ASSET-02:** Compile shaders offline with reflection and bounded variant keys; validate bindings, vertex layouts and constant-buffer alignment; retain last good shader on failed development reload.
- **MAT-01:** Complete consistent metallic/roughness PBR, correct linear/sRGB treatment, normal/tangent conventions, packed-channel mapping, alpha-test/blend and material instances. Test against material sphere/texture fixtures.
- **TEX-01:** Audit mips, sampler states, row pitches, cubemap faces, compression and missing-texture fallback. Integrate incremental background decode and bounded render-thread upload.
- **GEO-01:** Complete topology/index-size/submesh/dynamic-buffer paths; validate cache invalidation after vertex/index changes; add instancing and material/mesh LOD in the P1 tier.
- **LIGHT-01:** Connect scene directional/point/spot lights to shading with documented limits, attenuation and unit conventions; complete IBL cache invalidation and environment transitions.
- **LIGHT-02:** Implement scalable Forward+ light lists, overflow handling, reflection probes and lightmap consumption for P1. Audit the existing lightmapper before promising baking; implement or explicitly defer unsupported authoring/bake paths.

**G4:** Cooked assets render the same material/light behavior in editor and runtime. Corrupt/reloaded inputs fail safely. Every material feature is tested in static, skinned and instanced paths where supported.

### M5 — GPU frame pipeline, shadows and effects

- **PIPE-01:** Implement the pass graph and per-view resource/history ownership; explicit color/depth/normal/velocity contracts, transient target pooling and resize-safe allocation.
- **PIPE-02:** Render linear HDR color on DX11 and tone-map once at presentation. Verify UI composition and offscreen render-texture color semantics.
- **SHADOW-01:** Real directional shadow maps/CSM, stable cascades, bias/filter controls, caster visibility and alpha-test/skinned caster support. P1 adds local-light shadow budgets and atlas management.
- **POST-01:** First complete tone mapping/basic AA/bloom/exposure. Port AO, TAA, SSR, contact shadows, DOF and motion blur as separate tested passes; API availability follows each completed pass.
- **TEMP-01:** Camera jitter, static/skinned motion vectors, per-view history, disocclusion handling and reset on cut/resize/teleport/reload. Validate temporal behavior over sequences, not single screenshots.
- **QUALITY-01:** Low/medium/high presets with documented feature/cost differences; P1 dynamic resolution and upscaling policy. Use fallbacks when required inputs/capabilities are unavailable.

**G5 / R1 pipeline gate:** GPU HDR frames reach the window and render textures without normal-frame CPU readback. Essential shadows and P0 effects are visually validated, and animation/particles compose correctly. Each P1 effect remains experimental until its temporal/image/performance tests pass.

### M6 — scene scale, terrain, UI and authoring

- **SCENE-01:** Audit per-camera visibility jobs and snapshot completeness; introduce frame completion barriers or immutable visibility results. Measure frustum culling, sorting and scene traversal with representative content.
- **WORLD-01:** Validate terrain geometry, blend layers, normals, bounds and seams; add terrain LOD/chunk streaming and foliage instancing/wind/impostors for P1.
- **STREAM-01:** Budget CPU/GPU memory, uploads and concurrent work; prioritize visible assets, retain safe placeholders, cancel requests on unload and evict only unused resources.
- **UI-01:** Validate runtime UI/ImGui/text clipping, state restoration, font/glyph lifetimes, DPI, render textures, multiple views and device recovery.
- **TOOL-01:** Extend existing Lua editors; expose skeleton/palette/bounds diagnostics, particle pool/overdraw views, material reload errors, pass timing and texture residency.
- **TOOL-02:** Add picking, debug shading modes and inspectable capabilities/settings. Native implementation detail stays in diagnostics rather than ordinary user flows.

**G6:** Representative multi-view scenes, animated characters, particles, terrain and UI remain correct during streaming, reload and camera changes. Editor preview uses the runtime path rather than an unrelated renderer.

### M7 — water and the comprehensive feature tier

Water is a new implementation workstream. Start with lakes/pools/rivers so the first tier is bounded and useful. Introduce `ClawWater` internally (name provisional), implement applicable `IGraphicsWater` methods, and keep richer settings in a versioned concrete resource/settings object.

| ID | Implementation | Required evidence |
|---|---|---|
| WATER-01 | Water plane/mesh and river geometry, surface bounds, material settings, flow/normal textures, time and wave controls | Lake/pool/river sample assets cook, load, render and round-trip settings |
| WATER-02 | Fresnel, normal perturbation, depth reconstruction, absorption/scattering approximation, refraction with edge clamping | Correct shallow/deep appearance and silhouettes; samples use matching-view depth/color |
| WATER-03 | IBL/probe reflection fallback; budgeted planar reflection for suitable surfaces; optional SSR contribution | Missing/offscreen reflection information falls back gracefully; no recursive water reflection |
| WATER-04 | Shore/intersection foam, river flow maps, wave normals and basic analytic wave displacement | Stable shoreline, continuous river seams and conservative displaced bounds |
| WATER-05 | Pass order, transparent object/particle policy, fog/shadows, per-view reflection targets and quality settings | Objects above/below/intersecting water and particles crossing the surface compose predictably |
| WATER-06 | Extend Lua material/scene controls with water preview and reflection cost diagnostics | Editor/runtime match; multiple surfaces share a documented reflection budget |
| WATER-07 | Optional ocean spectrum/FFT, underwater volumes, caustics, shore simulation, wakes and spray integration | Separate P2 gate after core water is stable; explicitly separate visual waves from physics/buoyancy |

Core water acceptance includes camera grazing angles, reflection-camera clipping/handedness, near-plane intersections, overlapping water surfaces, resize, multi-camera views, depth precision and documented transparency limitations. Test a lake, shallow pool and moving river against repeatable images/sequences.

Expose wave height/normal sampling only through a deliberate contract if gameplay needs it; physics owns buoyancy. Splash emitters may consume interaction events and must respect particle budgets. Underwater cameras are outside the first water tier and receive explicit fallback behavior until WATER-07 is certified.

**GW / R2 water gate:** The three core water samples render correctly with defined reflections/refraction, foam and quality budgets. No ocean/underwater claim is made without its own implementation and tests. R2 also completes ANIM-08, FX-07/08 and other P1 feature gates.

### M8 — certification and shipping

- **SHIP-01:** CI: clean configure/build, C90/C++17 compilation, contract/unit tests, targeted static analysis and supported sanitizer configurations. GPU jobs produce image/sequence comparisons and capture diagnostics.
- **SHIP-02:** Separate test outcomes into passed/failed/skipped/unavailable; require the selected backend for certification. Pin assets, capture settings and tolerances; review golden-image updates rather than automatically accepting changes.
- **SHIP-03:** Add install/export targets, headers/libraries/dependency manifests, sample cooked content, license notices, symbols and an out-of-tree consumer that builds against the installed package.
- **SHIP-04:** Document supported hardware/features/limits, setup, resource formats, quality tiers, recovery, migration, troubleshooting and profiling.
- **SHIP-05:** Run long-duration mixed-scene soak, repeated scene/resource reload, device recovery, corrupted-asset and memory-pressure tests. Archive evidence per candidate/version/backend.

**GR:** Ship only after the applicable R1/R2 gates below pass. Release notes identify certified configurations and experimental features.

### M9 — backend expansion

For DX12, complete real geometry/material submission, swapchain/window/render-texture presentation and UI integration first. Add per-draw upload allocations and descriptors with fence-governed reuse, frame contexts, resource-state tracking, deferred release and a bounded memory budget. Repeated draws in one command list must preserve distinct transforms/materials; waiting at frame end alone is insufficient to validate per-draw writes.

Use [Microsoft's fence-based resource management guidance](https://learn.microsoft.com/en-us/windows/win32/direct3d12/fence-based-resource-management) and [resource-state synchronization guidance](https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12). Certify device/presentation/lifecycle, animation, particles, water and frame pipeline independently on DX12. Native bridges or internal backend-neutral draw interfaces must replace the current null UI renderer path.

**GB:** Backend-specific tests, images, recovery, packaging and measured budgets pass. Linux/macOS/mobile require a separate windowing/build/backend audit and estimate; an engine-wide build flag does not establish WPGraphics feature parity.

## 7. Test strategy and release acceptance

| Layer | Purpose | Examples |
|---|---|---|
| Unit/headless | Fast, deterministic correctness | Pose/weight math, particle trajectories/counts, resource validation, pass dependencies, settings serialization |
| Component/integration | Prove scheduling and ownership | Graph/controller/mesh bridge, effect lifecycle, reload/cancellation, scene teardown, upload queue |
| GPU contract | Catch native state/resource errors | Independent per-draw constants, target switching, bindings, formats, skinned buffers, device reset |
| Visual/sequence | Prove final appearance and time behavior | Materials, skinned shadows, motion vectors, smoke sorting, water clipping, temporal stability |
| Performance/soak | Prove predictable cost and longevity | Crowds, particle overdraw, streaming, terrain, multi-view UI, repeated reload and eight-hour scenes |
| Packaging | Prove someone else can use it | Clean installation, cooked-content sample, external consumer, no local dependency paths |

Preserve existing tests. Add focused executables for animation, particle simulation, particle rendering, GPU pipeline and water alongside the Claw tests, using the existing registration style. GPU tests may require a hardware runner; WARP/offscreen checks supplement hardware coverage where viable. Do not count a headless early return as evidence that a GPU feature works.

Proposed budgets, to calibrate in M0:

- 1080p medium reference scene: p95 GPU frame time ≤16.7 ms and p95 CPU render submission ≤4 ms on the declared baseline machine. Measure after warm-up; record p99 and streaming spikes separately.
- Scenes report geometry/material/light counts, bone counts, visible characters, particle screen coverage and active water reflection views. Test subsystem fixtures as well as one mixed scene.
- Animation target: 100 characters with a declared skeleton/clip complexity; particle target: 10,000 visible particles. Optional larger tiers need separate measured budgets.
- No avoidable full-frame synchronous GPU readback in normal rendering. No unbounded cache/pool growth or per-particle steady-state allocations.
- Eight-hour soak: no crash, no graphics validation errors, no monotonic unexplained memory growth after warm-up. Post-unload live resource counts return to documented baseline; allocator-reserved memory is reported separately.
- At least 1,000 repeated scene-load/unload and target-resize cycles; injected creation/upload/decode failures preserve valid state. Device recovery tests restore a usable scene or produce the specified bounded failure outcome.
- Certification covers selected AMD/NVIDIA/Intel adapters where available, with recorded drivers. The initial supported set is the machines actually validated, then broadened deliberately.

**R1 checklist:** G0, G1, GA, GP, P0 portions of G4/G5/G6, initial GR; licensed cooked sample; no unresolved crash/data corruption/resource leak; no mandatory feature tests skipped; documented performance and capabilities.

**R2 checklist:** R1 plus every advertised P1 feature gate, GW, expanded performance/visual/soak suite and renewed GR. If a P1 feature is deferred, adjust the published scope and retain its explicit backlog entry.

## 8. Risks and decisions to resolve in M0

| Risk/decision | Response |
|---|---|
| Animation/particle code exists but final rendering may be missing | Prioritize asset-to-screen fixtures and GA/GP; budget implementation work, not only tests |
| Multiple animation representations and particle update paths | Define authoritative data and ownership; adapt existing layers rather than updating twice |
| Bone/keyframe/influence limits differ between code and assets | Publish validated per-backend limits; compiler rejects overflow or performs documented partitioning |
| Shared interface ABI and other graphics implementations | Keep this pass internally additive; test compatibility; isolate any later versioned interface change |
| Software CPU pipeline mistaken for DX11 feature support | Separate capabilities; implement GPU passes and prove actual output |
| DX12 resources/texture handles interpreted across APIs | Require typed backend ownership and an explicit bridge; reject incompatible handles |
| Transparent particles, water and temporal effects interact | Specify pass order and history policy; maintain dedicated mixed-effect test scenes |
| Legacy dependency paths and missing GPU CI | Resolve clean build/packaging and hardware access before expanding the feature promise |
| Feature breadth outgrows staffing | Ship R1 first; complete R2 work in bounded packages; estimate P2 independently |

Owners to assign: graphics lead (contracts/gates), native rendering engineer (device/passes), animation engineer (pose/deformation), effects engineer (particles/water), technical artist (fixtures/reference scenes), and QA/build engineer (CI/certification). One person may hold several roles; staffing changes elapsed time and parallelism, not the acceptance criteria.

## 9. First implementation increments

Each increment should be independently reviewable and preserve the baseline tests.

1. `BASE-01/02/03`: capability/skip inventory, minimal Windows Claw presets and recorded test baseline.
2. `BASE-04`: two-bone animation and seeded particle fixtures plus real-backend test harnesses that fail clearly when required capabilities are absent.
3. `ANIM-01/02`: imported skeleton/clip tracing, validated bind transforms/weights and a CPU deformation reference.
4. `FX-01/02`: authoritative particle lifecycle/simulation path, deterministic seed/time policy and analytical simulation tests.
5. `CORE-01/02/05`: resource ownership, cache generations and safe scene/job teardown required by animated meshes/effects.
6. `ANIM-04/05`: graph-to-pose bridge and visibly correct DX11 skinning with independent instances.
7. `FX-03/05/06`: pooled simulation snapshots and real batched particle rendering, bounds and multi-camera correctness.
8. `PIPE-01/02` and `SHADOW-01`: GPU HDR frame with correct static/skinned shadows and transparent composition.
9. Complete remaining P0 asset/material/UI/recovery work; run R1 certification and package the mixed-scene sample.
10. Implement P1 features in measured packages, beginning with animation/effect completeness and WATER-01 through WATER-06; certify R2 before promoting its capabilities.

Completion records for every increment should include changed files, actual tests and skips, reference captures where relevant, measured costs, and remaining limitations. This document is the roadmap; the first executable task is the baseline and animation/particle validation harness.
