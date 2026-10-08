# SampleVehicleAdvanced graphics and LOD review

Reviewed 8 October 2026 in `G:/Workphone`, using the current source and the existing RelWithDebInfo executable. This is an implementation plan; rendering code has not been changed.

The biggest improvements should come from stable texture filtering and antialiasing, richer trackside materials and scenery, coherent lighting, and fewer vegetation draw submissions. Keep the existing PBR, reflection, procedural scene and LOD abstractions. A larger texture budget or more triangles alone will not address the most visible weaknesses.

## Evidence from the current sample

Three fresh seed-7, high-quality DX11 captures at 1280 x 720 passed the sample's reflection, triangle-winding and tree-level visibility checks:

| View | Near tree patches | Imposter patches | Reported mean frame interval |
| --- | ---: | ---: | ---: |
| [Car](G:/Workphone/Samples/cpp/VehicleAdvanced/review/car-high.png) | 6 | 38 | 45.82 ms |
| [Corner](G:/Workphone/Samples/cpp/VehicleAdvanced/review/corner-high.png) | 6 | 38 | 39.02 ms |
| [Track](G:/Workphone/Samples/cpp/VehicleAdvanced/review/track-high.png) | 0 | 44 | 30.27 ms |

Each run generated 35,018 stored triangles, 165 scene meshes and 39 texture resources; the builder reported 36,061,152 texture bytes. These totals include alternative LOD representations and the builder's texture accounting, rather than measuring visible triangles or total GPU memory. The short frame-interval measurements include the application and its scheduling; they are not isolated GPU timings or a repeatable performance benchmark.

The captures show noisy fine detail on carbon and asphalt, jagged silhouettes and markings, large uniform grass/gravel areas, strongly geometric pines and conical hills, and limited visual depth at the horizon. Existing shadows are visible. Still images do not establish how much shimmer, ghosting or LOD popping occurs during driving.

`MeshImposterTests.exe` passed. `UnitTests.exe --run_test=LODSystemTests` passed all three selected cases and 13 assertions, confirmed in [the XML report](G:/Workphone/Samples/cpp/VehicleAdvanced/review/lod-tests.xml). Those tests cover baking and selection helpers; they do not validate moving-camera transitions or the full renderer. Existing binaries were used without a rebuild. The startup configuration also reports an unavailable legacy `WPOISInput.dll`; the captures and selected checks nevertheless completed successfully.

### Findings that determine the plan

| Finding | Consequence and source |
| --- | --- |
| Ordinary DX11 textures have one mip level. | Filtering cannot progressively remove distant texture detail. [Texture creation](G:/Workphone/Engine/c/Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.c:891) and [ClawTexture upload](G:/Workphone/Engine/cpp/Source/WPGraphics/ClawTexture.cpp:204). Cubemap roughness levels already exist and should be retained. |
| The graphics pipeline's CPU frame exchange is restricted to the software renderer. | TAA, AO, bloom and exposure settings do not establish those effects in these DX11 captures. [Pipeline presentation](G:/Workphone/Engine/cpp/Source/WPGraphics/ClawHammerSystem.cpp:714). |
| DX11 shades and tone-maps directly in the material pixel shader. | A true HDR post-processing path needs a linear intermediate target and one final display transform. [Current shader](G:/Workphone/Engine/c/Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.c:247). |
| DX11 already renders one directional shadow map. | Improve its distribution and cost. The current path is one camera-fitted map, with a 200-metre fit cap and 3 x 3 PCF, rather than multiple cascades. [Scene shadow pass](G:/Workphone/Engine/cpp/Source/WPGraphics/ClawScene.cpp:641), [map fitting](G:/Workphone/Engine/cpp/Source/WPGraphics/ClawRendererDX11.cpp:851). |
| The builder configures fog, but the reviewed DX11 material path does not consume fog parameters. | Atmospheric depth requires a renderer connection. [Fog request](G:/Workphone/Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp:810), [DX11 material shader](G:/Workphone/Engine/c/Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.c:127). |
| Tree patches are 64 metres wide, but selection uses a fixed seven-metre representative tree at the patch centre. | Nearby large trees can lose detail when their patch centre is far away. Actual tree heights vary from four to eleven metres. [Tree construction and LOD authoring](G:/Workphone/Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp:496). |
| Tree imposters use three fixed cards and baked lighting through emission. | Near and far trees use different lighting models; side/top intersections and silhouette changes need visual validation. [Card geometry](G:/Workphone/Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp:211), [material setup](G:/Workphone/Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp:481). |
| LOD changes toggle complete renderer sets; fade width is reserved. | Hysteresis avoids repeated switching, but cannot hide the transition itself. [LODGroup application](G:/Workphone/Engine/cpp/Source/Workphone/Scene/Components/LODGroup.cpp:437), [level definition](G:/Workphone/Engine/cpp/Include/Workphone/Scene/Components/LODGroup.hpp:25). |
| The vehicle generator produces three LODs, but the builder uploads one selected by the quality preset. | Vehicle detail never changes with camera coverage. [Generated thresholds](G:/Workphone/Engine/cpp/Source/WPProcedural/WPVehicleGeometry.cpp:412), [builder selection](G:/Workphone/Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp:309). |

The existing README records an older regression from about 25 ms before tree patches to about 36–38 ms afterwards. That is historical application timing, not proof of today's GPU bottleneck. It is a strong reason to measure draw submissions before increasing patch counts. The current renderer already exposes draw/triangle counts, material and transform uploads, state bindings, GPU queries, presentation wait and p95 frame intervals through [DX11 statistics](G:/Workphone/Engine/c/Include/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h:116).

## Recommended delivery order

| Stage | Deliverable | Completion condition |
| --- | --- | --- |
| 1 | Repeatable visual and timing baseline; mipmaps and sampling improvements. | Less distant noise, preserved foliage coverage, and measured filtering cost. |
| 2 | Track materials, atmosphere and lighting coherence; practical DX11 antialiasing. | Car, corner and driving views visibly improve without exceeding the agreed frame budget. |
| 3 | Vegetation submission prototype and accurate LOD selection. | Smaller selection units improve nearby silhouettes while draw and frame costs improve or remain within budget. |
| 4 | LOD transitions, better foliage representations and shadow detail. | Moving-camera sweeps show stable brightness, silhouettes and shadows. |
| 5 | Runtime vehicle/scenery LOD, richer environment and GPU post-processing. | Quality presets scale consistently and each added effect has verified output and measured cost. |

Stages 2 and 3 offer complementary gains: scenery improves the image, and submission improvements create room for it. GPU HDR/TAA/AO is substantial backend work and should have its own milestones.

## Graphics improvements

### 1. Establish a useful baseline

Extend the capture harness with a HUD toggle, forced LOD, a fixed camera route and a benchmark duration. Use a stable scene and camera trace so physics variation does not obscure rendering comparisons. Start with seed 7; add two other seeds and test low/medium/high at 720p and 1080p. Warm up until resource upload completes, then collect 30–60 seconds of data.

Reuse the existing DX11 counters. Report CPU time with presentation wait identified separately, GPU time with valid sample count, mean/p95 frame intervals, draws and triangles per frame, LOD transitions, generation time and resident resource memory. Existing draw totals include shadow and UI passes: label them and add per-pass counters when needed. A 60 FPS / 16.67 ms goal can be adopted for named reference hardware after this baseline, rather than promised from the current short captures.

### 2. Fix sampling before raising texture resolution

Add full mip chains for procedural 2D textures. Give uploads an explicit albedo/emission versus normal/data role. Generate colour mips in linear space; renormalize normal mips and preserve normal variance in roughness filtering. Preserve alpha-test coverage for vegetation at the current cutoff, dilate colour into transparent borders, and provide atlas gutters that remain safe at lower mip levels. A simple average of transparent-black atlas pixels will create fringes and shrinking trees.

Use trilinear filtering and the existing configurable anisotropic sampler for asphalt and other grazing surfaces. Check actual material sampler settings; anisotropic support is already present. Budget the added mip storage, typically roughly one third for a full chain of a large square texture. Keep the shader's existing normal-variance specular filtering, then tune carbon frequency/normal strength and asphalt detail using moving footage.

Implement the colour-space contract consistently: using sRGB texture views must replace the corresponding manual shader decode, to avoid decoding twice. Ordinary colour mips can use a correctly configured GPU generation path; normal and alpha-coverage mips need additional processing. Microsoft's [GenerateMips contract](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-generatemips) specifies the required resource flags and supported formats.

### 3. Make the ground and surroundings read as a circuit

The [track builder](G:/Workphone/Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp:363) textures asphalt but gives grass, gravel, kerbs and hills largely constant materials. Add grass and gravel albedo/normal/roughness maps through the existing procedural services. Combine metre-scaled detail with broad colour variation so the meadow does not become a visibly tiled texture. Add track-edge blending, restrained tyre rubber along braking zones, asphalt repairs and kerb wear. Begin with masks or batched strips; avoid an actor/draw per decal.

Replace the [conical hills](G:/Workphone/Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp:537) with a modest continuous landscape mesh, smoother silhouettes and height/slope material variation. Keep it decorative initially so graphics work does not require new driving collision. Improve pit facades, signs, braking boards and the gantry with shared materials and a few deliberate landmarks. Near-track visual detail matters more than multiplying distant trees.

### 4. Align the sky, sun, atmosphere and reflections

Use one lighting configuration for the procedural sky, directional sun, ambient/sky lighting, fog and imposter generation. The current builder computes an atmospheric sun direction but configures the visible sun with fixed values, and can reuse an Editor light. Make the sample's intended lighting explicit while preserving predictable behavior when hosted in the Editor.

Connect fog parameters to DX11 and blend in linear space before the final display transform. Start with inexpensive distance/height fog; use it to separate the hills and establish a credible horizon. Retain the existing GGX-filtered reflection cube. Improve its trackside content or add a small set of static local probes before considering realtime captures. Test reflection seams and roughness levels independently of diffuse ambient brightness.

Car materials already author clearcoat parameters, but the reviewed DX11 material shader has no clearcoat lobe. Add a measured, energy-aware clearcoat implementation for paint if the close view still needs it. Preserve restrained tyre response and readable rims; more gloss is not a universal improvement.

### 5. Deliver antialiasing and post-processing in separate steps

A GPU fullscreen AA pass is a practical first milestone for jagged geometry and markings. MSAA is another option, but requires multisampled colour/depth targets, resolve support and corresponding capture changes; the current capture harness accepts single-sample UNORM targets only. Alpha-to-coverage should be considered only with a working multisample path. Post AA alone will not fix texture aliasing, so pair it with mipmaps.

For the longer-term GPU pipeline, render scene colour to a linear floating-point target, implement GPU exposure/tone mapping and composite the HUD afterwards. Add real depth/normal inputs for AO, then camera jitter, previous transforms and object/camera motion vectors for TAA. Wheels, camera resets, LOD swaps and disocclusion need explicit history handling. Keep all passes on the GPU; avoid per-frame CPU readback into the existing software effects. Add restrained bloom once HDR output works. SSR, depth of field and motion blur are later options with separate cost/quality decisions.

### 6. Refine the existing shadows

First tune the current near-car shadow fit, slope/normal bias and filter size. Then implement actual cascades for near detail and wider track coverage, with stable split ranges and texel snapping. Keep cascade count and distance configurable; the existing CSM setting alone does not supply multiple DX11 maps. Microsoft's [cascaded shadow map guidance](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps) describes distributing resolution across the camera frustum.

Cull shadow casters against light/cascade volumes while retaining offscreen casters that can affect visible receivers. Give foliage a stable simplified shadow representation instead of blindly casting its crossed/top imposter cards. Reassess the existing contact-shadow quad after real shadows improve, so the car is not darkened twice. Add screen-space contact shadows only after real GPU depth inputs exist.

## LOD system improvements

### 1. Separate spatial bounds, selection detail and submission

Keep `LODGroup` as authoring data and `LODSystem` as the selector. Preserve its immutable job snapshots, generation/revision guards, hysteresis and scene-thread application. Preserve the renderer's existing frustum culling.

Add conservative spatial bounds for culling and a separate detail metric for selection. A 64-metre patch radius and the size of one tree describe different things. Simply calling `recalculateBounds()` on a patch while keeping its threshold would retain its detailed mesh much farther away; it does not solve the current selection approximation.

Select trees individually in flat arrays, or in smaller adaptive clusters, using their actual size and camera coverage. Aggregate the results into shared render submissions rather than creating one actor/renderer per tree. A temporary conservative cluster rule can keep detail if any member requires it, but its extra geometry cost must be measured. Do not blindly shrink patches: that can amplify the documented renderer overhead.

### 2. Prototype shared vegetation draws before expanding foliage

Represent trees with shared prototype meshes and per-instance transforms, scale, tint and variant IDs. Build compact visible lists per material and LOD, and reuse persistent GPU buffers. Near trunks, near canopy and distant cards can each have a small number of submissions, independent of selection granularity.

The reviewed WPGraphics DX11 path uses `DrawIndexed`; generic instancing interfaces exist, but a concrete DX11 vegetation instancing implementation was not found. Treat this as renderer work. Extend the public graphics contract where suitable and implement a narrow DX11 path, using [DrawIndexedInstanced](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-drawindexedinstanced). Prototype it on existing pines first. If it is too large for the first delivery, compare a bounded number of material/LOD batches or larger distant merged clusters instead.

Acceptance depends on measured draw count, submission time, GPU time and alpha overdraw, not just triangle count. Both current representations remain resident; selection alone does not save memory. Shared meshes remove repeated geometry, while streaming/eviction is a separate later feature for much larger scenes.

### 3. Improve screen-space selection and camera ownership

The selector already accounts for field of view, orthographic projection and LOD bias. Keep that behavior. Its perspective calculation currently uses centre-to-camera distance and a sphere approximation. Add actual projected bounds or optional per-level geometric-error metadata for assets that need more accurate selection, especially at wide FOV and near the camera. Add viewport height when thresholds are expressed in pixels.

The tree threshold of 0.085 corresponds to approximately 61 pixels at 720p and 92 at 1080p. Expose meaningful per-asset profiles instead of an unexplained single constant. Use pixel error for simplified meshes and silhouette/coverage criteria for imposters; calibrate the values with images rather than one universal distance. Test actual four-to-eleven-metre tree sizes and nonuniform actor scale.

Decouple publication of the render view from `treeLODs.front()`, so adding car/scenery groups still works when no trees exist. Retain the actual follow/capture camera override. Define behavior for the Editor, secondary views and probes: shared geometry should satisfy the highest relevant visible-view detail requirement, or use separate per-view draw lists. Shadow LOD should have its own policy.

### 4. Add real transitions and match representations

Implement `fadeTransitionWidth` with a transition state carrying source level, target level and weight. Use complementary dithered depth-writing coverage rather than transparent blending of entire meshes. Budget the short interval when both levels draw. Start with a narrow measured transition band; test stationary threshold boundaries and moving-camera sweeps. Without temporal AA, dither can be visible, so do not assume it improves every asset automatically.

Update the capture assertion: outside transitions exactly one level should be visible; during a supported transition two levels may be active with complementary weights. Apply a consistent transition policy to shadows, and invalidate or reject temporal history when representations change.

Match trunk placement, canopy outline, apparent brightness and shadow footprint before tuning fade duration. The existing baked-emission imposters cannot respond to changed lighting like near meshes. An initial improvement is a calibrated bake under the same lighting/display transform; the stronger solution is albedo/normal atlases lit by the common renderer.

### 5. Improve foliage quality in three useful ranges

Use a better near tree with branch/foliage structure, a cheaper silhouette-preserving middle representation, and a far imposter. Add a few seeded species/shape variations, tint and scale variation, and coherent clusters. Keep each range measurable; increasing near-tree detail should happen after submission costs are controlled.

For far trees, evaluate view-aware billboards and a modest set of azimuth/elevation views rather than drawing all three fixed cards. Keep an overhead solution for the track view. Extend the current hardcoded three-view atlas API only when this prototype demonstrates a worthwhile quality gain. Normal/depth atlases and octahedral view interpolation are later options. Validate grazing views, transparent gutters, alpha mip coverage and shadow shape.

Add a tiny-screen-size fade/cull policy for minor vegetation and props, with hysteresis. The current tree last level never culls. Coordinate culling with functioning fog, far clip and landmark visibility so the horizon does not disappear. Add wind only after all representations share compatible displacement and temporal motion handling.

### 6. Extend coverage and reduce selector overhead proportionately

Wire the generated vehicle LODs into renderer sets under the existing chassis/four wheel hierarchies. Preserve wheel hub offsets, steering, spin, material identity and silhouettes in every level. Keep high detail for the player's car during close views; one hero car offers less performance opportunity than repeated scenery. Add trackside props and barrier chunks next. Keep road continuity and markings intact; aggressive road simplification is low priority for the present scene size.

Cache static thresholds/bounds using authoring revisions and transform changes. Reuse batch storage only after jobs release it. Benchmark a synchronous fast path for small active sets; the current 44 tree groups fit within one default 64-item job. Update far groups less often where their projected error changes slowly, prioritizing near/transitioning groups. Attach a view epoch to asynchronous results so camera teleports/resets can reject obsolete selections without weakening the existing lifetime guards. Larger scenes can add spatial hierarchy traversal and distant cluster/HLOD representations after the current submission path is efficient.

## Validation and first implementation slice

For each implementation stage, retain the seed-7 captures and record changes using the same exposure, view and resolution. Add driving footage and a camera orbit through both LOD boundaries; inspect grass/road repetition, carbon shimmer, foliage brightness, shadow movement and popping. Test low/medium/high with both the standalone sample and the Lua/Editor port because construction lives in the shared `ProceduralRaceScene` builder.

Add focused behavior tests when features arrive: colour/data mip correctness and alpha coverage; actual bounds/scales; complementary fade weights; stale job results after removal, reset and view changes; multiple cameras; wheel transforms through a vehicle LOD swap; offscreen shadow casters. GPU effects require rendered-output checks as well as unit tests. Preserve the existing reflection/winding checks and driving smoke tests when changes touch shared vehicle hierarchies.

The first implementation slice should be **benchmark/capture controls, mipmaps with material-specific filtering, grass/gravel maps, and functioning DX11 fog**, followed by a **vegetation instancing/submission prototype with accurate selection sizes**. This addresses the most visible defects and the documented LOD cost issue before committing to a larger GPU HDR/TAA pipeline. Keep geometry generation, renderer quality and LOD profiles separately configurable so a quality preset can trade scenery, filtering, shadows and effects against a measured budget.
