# WPGraphics implementation status

## CPU-skinned OBB orientation refits - 17 October 2026

`ClawMesh::applySkinningPalette` now copies the previous OBB before replacing
vertices, then calls `wp_graphics_mesh_refit_bounds`. The native C90 path rebuilds
the current CPU AABB and projects every current vertex onto the previous box's
orientation, recalculating both centre and extents. Old extents never limit a new
pose. Finite, nondegenerate, nearly orthogonal directions are orthonormalized
before projection; missing, invalid or degenerate directions fall back to a full
PCA fit. Nonfinite vertices leave unknown bounds, preserving conservative culling.

Warm deformations avoid covariance accumulation and eigenvector solving. This
performs no lazy drawing work and allocates no heap scratch for refitting. Explicit bounds
updates retain the existing mesh-generation/stale-cull rejection contract and
refresh the attached native object's AABB. Each mesh exposes copied, cumulative
`pca_solves` and `orientation_refits` counters; clones own their own counters and
bounds. Ordinary authored-geometry rebuilds still request a new full fit.

Native tests move, rotate and stretch an eight-vertex mesh through 64 poses,
checking enclosure, changed centre/extents, retained orientation, one full solve
and 64 refits. They also cover invalid-direction fallback, invalid-vertex bounds
and stale prepared results. Active Claw animation contracts check initial full
fit, warm refit and independent clone refits after source retirement alongside
their DX11 pixel evidence.

Validation: all seven selected full Debug contracts pass, including active Claw
and DX11 target rendering on Microsoft Basic Render Driver/WARP. All six isolated
contracts pass in Debug and RelWithDebInfo; `git diff --check` passes. Logs:
`project_x64/{build,test}_refit_debug.log` and
`project_graphics_contracts/{build,test}_refit_{debug,release}.log`.

The retained orientation can be looser than a new PCA fit after a large shape
change. `wp_graphics_mesh_compute_aabb` remains available to refresh it explicitly.
The refit API explicitly replaces authored bounds, like that existing rebuild;
it does not change the requirement for expanded bounds around GPU deformation.
Representative animated-scene timings and an adaptive orientation-refresh policy
remain open; fewer covariance solves alone do not establish a frame-time speedup.

## Worker-batch shared transform prefixes - 17 October 2026

The next `PERF-07` / `SCENE-03` increment reuses composed ancestors within each
transform batch. Owner capture still deduplicates local matrices globally. It
additionally resolves integer parent/cache slots for each unique node in a batch,
then discards all borrowed node references before submission. A worker composes a
prefix on its first root-to-leaf visit and reuses that exact matrix for later
outputs. Each slot belongs to one batch; there are no cross-task dependencies,
shared cache writes or worker reads of live scene nodes.

Root identity multiplication and parent-to-child order are preserved. Null owners
still publish identity matrices. Scoped completion, cancellation without output
publication, rejected-dispatch inline execution, and the 257-node hierarchy limit
are unchanged. The original composition path is retained when there is no
within-batch reuse or the optional cache cannot fit/allocate within the 64 MiB
capture limit. A failed base capture still preserves the previous result.

Copied job statistics add `matrix_multiply_count` and `batch_count`; counts describe
the accepted plan and become completed work after this job drains. The existing
path-reference count gives the unshared multiplication cost. Derived cull views
retain those shared-publication statistics and must not count them as fresh work.

The 1,024-output fixture with 257 distinct local matrices and batch size 17 now
performs **362 matrix multiplications instead of 2,048** across 61 batches. Retained
job-array storage rises from 100,808 to 128,808 bytes in that fixture. New tests
compare every matrix bit-for-bit with an independent uncached root-to-leaf
reference across four batch sizes, inline execution and three workers, using two
roots, diverging branches, duplicate/null/ancestor outputs, rotations and reflected
or singular scales. Existing retirement, cancellation, depth and OBB/sphere tests
remain part of the native suite. A 350,000-output fixture verifies that an optional
cache exceeding the capture budget falls back without rejecting the valid base
snapshot or changing its matrices.

Validation: all seven selected full Debug contracts pass, including active Claw
and DX11 target rendering on Microsoft Basic Render Driver/WARP; all six isolated
contracts pass in Debug and RelWithDebInfo. `git diff --check` passes. Logs:
`project_x64/{build,test}_prefix_debug.log` and
`project_graphics_contracts/{build,test}_prefix_{debug,release}.log`.

This removes repeated matrix arithmetic within batches, not repeated path walks
or all owner-side capture/sort work. Cross-batch prefix sharing, automatic reuse
across viewport passes and retained allocation scratch remain open. The extra
capture sort and cache storage need representative profiling before claiming a
whole-frame speedup.

## Fitted OBB and sphere culling - 17 October 2026

`SCENE-02` now uses mesh-local oriented bounds in both native job preparation and
the synchronous scene reference. Rebuilding a mesh AABB also fits an OBB from CPU
positions using covariance principal axes and projected extrema. The fit falls
back to local axes when their surface-area proxy is smaller. This is an approximate
fit, with no minimum-volume guarantee (see the distinction in the
[Open3D oriented-box documentation](https://www.open3d.org/docs/latest/cpp_api/classopen3d_1_1geometry_1_1_oriented_bounding_box.html)).
The previous transformed local AABB already avoided world-AABB inflation; the new
fit additionally tightens meshes whose geometry runs diagonally in local space.

The immutable candidate publication caches each transformed centre, three half
axes and an enclosing sphere. Frustum planes are normalized once per view. Sphere
rejection and full containment precede six-plane box support tests. Under parent
shear the transformed box is a parallelepiped, and its sphere encloses all eight
corners; negative and singular affine scales remain supported. Invalid/nonfinite
bounds or unsupported projective transforms retain candidates. These are
conservative six-plane tests, not full frustum/box separating-axis intersection.
Shadow views continue to retain off-camera casters and perform no camera-bound
tests. Derived camera/shadow views reuse the cached bounds without refitting.

Mesh APIs expose copied OBBs, explicit fitting, trusted authored OBBs and clearing.
Authored AABBs clear fitted OBBs and preserve their deformation margins through
vertex replacement. Replacement and dirty notifications invalidate computed
bounds and mesh generations; unknown bounds remain visible until rebuilt. Fitting
occurs at an explicit bounds rebuild, never lazily during drawing. Serialized
authored AABBs remain conservative; OBB serialization is not introduced. GPU
deformation still requires suitably expanded authored bounds. CPU skinning that
rebuilds bounds also pays the fit cost and needs representative profiling.

Per-view copied counters report sphere tests/rejections/full acceptance, box
tests/rejections, fitted versus fallback boxes and conservative candidates.
`wp_graphics_cull_job_set_sphere_test` provides an owner-side A/B switch, inherited
by derived views and immutable while a configured job is ready.

Native regression evidence includes fitted enclosure for diagonal, arbitrary 3D,
planar, linear and point clouds; transactional invalid-box rejection; NaN retention;
authored margins and stale-result invalidation. A diagonal rod is rejected by its
OBB while its transformed local AABB remains admitted. Its shadow caster survives
source-view retirement. In 256 reflected/singular/sheared hierarchy fixtures,
sphere-enabled and box-only lists agree; an independent clip-space vertex check
detects no rejection of a mesh containing a visible vertex. The fixture records
123 sphere rejections, 19 full accepts and 114 box tests. These operation counts
do not establish a frame-time speedup or a live hardware/Editor visual result.

Validation: all seven selected full Debug contracts pass, including DX11 target
and active Claw production rendering on Microsoft Basic Render Driver/WARP. All
six isolated contracts pass in both Debug and RelWithDebInfo; `git diff --check`
passes. Logs: `project_x64/{build,test}_obb_debug.log` and
`project_graphics_contracts/{build,test}_obb_{debug,release}.log`. DX12/Vulkan device
rendering was not exercised; the bounds implementation is backend-independent CPU
code consumed through the existing native scene/Claw job paths.

## Shared colour/shadow candidate publication - 17 October 2026

`wp_graphics_cull_result_create_view` derives a camera/main or conservative shadow
view from the complete immutable candidate publication. Bounds, world matrices,
object ordering keys and ancestor/resource revisions are held once by an internal
reference-counted snapshot. Each view owns independent camera matrices/masks,
visible indices and ordering. It includes candidates excluded by the source view,
so another camera can see them and off-camera shadow casters remain admitted.

Derivation checks scene membership and every captured object's mesh/ancestor
revisions before reuse. Camera-only changes may be sampled into a new view without
recapturing valid geometry; they still invalidate an old view that captured the
previous matrices/mask. Scene or geometry changes reject derivation. Reference
changes, validation and view destruction are owner-thread operations. Successful
views survive destruction of the source result and replacement/destruction of its
preparation job. Scene, cameras and resources remain borrowed; their existing
owner/lifetime contract is unchanged. Empty publications also support derived views.

`ClawScene::prepareRenderJobs` now prepares colour once and derives shadows from
that publication. A non-empty scene performs **one transform dispatch per pass**
instead of two; the shadow view performs zero. Native per-view dispatch counters
and `ClawScene::getPreparedTransformDispatchCount` expose actual preparation work.
The older transform-array counters describe the shared publication, so clients
must not sum them as fresh work for each derived view. Frame-end retirement and
RTT pre-bind/post-draw validation keep their existing ordering and output retention.

Validation: six selected full Debug contracts pass after normal project-reference
builds, and all six isolated contracts pass in Debug and RelWithDebInfo. New native
checks cover independent cameras/masks, off-camera casters, reference-list/matrix
parity, zero derived dispatches, source/job retirement, nested views, stale object/
ancestor/membership rejection and empty views. DX11 compares two cameras derived
from one publication against complete synchronous reference pixel buffers, renders
a retained view after source retirement, and verifies stale shared geometry leaves
RTT pixels intact. The active material-aware Claw RTT test asserts one dispatch for
its colour/shadow pass. Device: Microsoft Basic Render Driver/WARP, feature level
`b000`, driver `10.0.19041.7802`. `git diff --check` passes.

Logs: `project_x64/build_shared_views_debug.log`, `test_shared_views_debug.log`, and
`project_graphics_contracts/{build,test}_shared_views_verified_{debug,release}.log`.
This removes duplicate candidate capture/composition within a pass; it does not
establish a measured whole-frame speedup. Automatic reuse across separate active
viewport passes, shared-prefix worker composition, retained sort/capture scratch,
async scheduling and live Editor/hardware performance remain open.

## Native transform cache and shared capture - 16 October 2026

The next `PERF-07` / `SCENE-03` increment is integrated into the existing native
scene-node queries and transform-to-cull preparation. Local matrices are cached
by a separate local revision. World queries walk the ancestor chain on the owner
and rebuild only when the node revision, parent identity or parent's evaluated
world revision changes. This uses per-instance revisions, with no assumption that
separate DLLs share a global clock. Equal position/orientation/scale assignments
keep both caches and prepared cull results valid. Reparenting and parent destruction
invalidate world matrices while retaining unchanged local matrices.

Queries through 257 nodes use the cache. Deeper native queries preserve the
existing depth-limited recursive reference and never publish truncated transforms
into the cache. Transform jobs still reject such paths. Matrix queries now populate
caches even through const pointers and require the owner thread. Worker jobs keep
their existing immutable-input contract and never access node caches.

Transform capture sorts borrowed references on the owner, copies each unique
local matrix once, and replaces repeated ancestor matrices with integer indices.
Borrowed references are freed before worker submission. Workers retain their
original root-to-leaf multiplication order, private outputs and scoped completion.
The 64 MiB job-array/capture-scratch cap, cancellation and pre-draw fallback rules
remain in effect. New counters report unique local matrices, path references,
outputs and retained/peak job-array bytes; cull results copy these counters for
active-pass inspection. Node counters report local/world cache builds, excluding
hierarchy checks and the uncached deep-path fallback.

The deterministic shared-hierarchy fixture requests 1,024 outputs from 256 children
with one shared parent: 2,048 path references now use 257 copied local matrices.
The x64 fixture retains 100,808 bytes of job arrays. Unchanged second captures do
not rebuild any local matrices. Tests cover normalized rotation/nonuniform negative
scale against independently composed matrices, warm reuse, selective child edits,
ancestor edits, idempotent setters, parent replacement, parent
retirement, attached-camera motion and the depth-limit boundary. Existing job
capture/destruction/cancellation and cull-to-RTT pixel parity checks remain required.

This measures copy/build work and job-array storage, not whole-frame speedup.
Each node retains two matrices plus revision/counter metadata until destruction.
Sorting adds capture work, and workers still compose shared prefixes separately
for each requested output. Sharing one transform publication between colour/shadow
views, persistent scratch reuse, spatial candidates, full per-pass timing and
representative Editor/hardware benchmarks remain open.

Validation: normal project-reference Debug rebuilds pass for the native scene
contracts and Claw DX11 target/production targets. Six selected Debug contracts
pass, including the two-camera complete pixel comparison, stale-dependency output
retention and active material-aware Claw RTT path. All six isolated contracts pass
in both Debug and RelWithDebInfo. Device evidence remains Microsoft Basic Render
Driver/WARP (`b000`, driver `10.0.19041.7802`); live Editor and hardware performance
have not been certified. Logs: `project_x64/build_transform_reuse_verified_debug.log`,
`build_transform_reuse_final_contract_debug.log`, `test_transform_reuse_verified_debug.log`, and
`project_graphics_contracts/{build,test}_transform_reuse_verified_{debug,release}.log`.
`git diff --check` passes.

## Native transform and cull integration - 16 October 2026

`workphone_graphics_transform_job.h/.c` capture local-matrix ancestor chains on
the owner before submitting tasks. Workers compose only copied matrices into
private storage. `wait` publishes after a task-local completion group drains;
cancel/destroy drain without publishing. The pool is borrowed and optional
(NULL computes inline); failed task submission also computes inline. Capture
rejects excessive counts, snapshots over 64 MiB and paths deeper than 257 nodes.
Independent instances have independent completion, so unrelated pool work does
not delay a completed transform job. Node edits/destruction after dispatch are safe.

`workphone_graphics_cull_job.h/.c` use these matrices with copied bounds, masks,
queue/Z order and pass policy. Results own all candidate and complete ancestor-path revisions, frozen camera
and world matrices, and stable visible indices. Main views apply conservative
frustum tests; shadow views retain admitted off-camera casters. Unknown/inverted/
nonfinite bounds remain visible. Validation rejects scene membership/replacement,
object and mesh edits, ancestor changes, and camera matrix/mask changes before
consumption. Hidden candidates are validated too. Scene/object/node revisions and
mesh bounds mutation now participate in this contract. Native owner APIs must be
used for mutation; raw destruction of borrowed resources outside their owner is
not supported. Result destruction itself never dereferences borrowed objects.

`wp_graphics_scene_render` consumes the prepared result; the synchronous
`render_with_submit` remains its pre-draw allocation/capture failure fallback.
`ClawScene` consumes frozen matrices for DX11 material-aware colour draws,
conservative mesh shadows and software draws. The DX11 mesh snapshot entry point
uses the captured view/projection rather than querying the camera for each mesh.
`ClawHammerSystem` prepares results before begin/draw, checks them as the RTT
dependency and again before texture publication, then retires them at pass end.
Claw preparation failures fail the pass; no partially submitted pass is replayed.
Clear/unload/native-scene release discard prepared jobs. CPU capture/preparation
is synchronous in the active frame; optional pooled transforms are available
through `wp_graphics_scene_set_transform_pool`.

This is a correctness/integration tier. Ancestor chains are copied per object,
and main/shadow snapshots are prepared separately. Generation-cached/deduplicated
transforms, spatial acceleration, asynchronous frame scheduling, per-job profiling,
full immutable material/resource/pass snapshots, live Editor comparison and
representative hardware performance remain open. Terrain/foliage/particle systems
retain their existing independent preparation paths.

Validation: the selected full Debug build and six contracts pass:
`WorkphoneGraphics.scene_jobs`, `renderer_contract`, `native_frame`, `rtt_job`,
`WPGraphics.claw_dx11_targets` and `claw_production_render`. The six isolated
C90-configured contracts also pass in Debug and RelWithDebInfo. Native tests cover
parent/child composition, rotation and negative/nonuniform scale, shared paths,
node destruction after dispatch, null/empty inputs, oversized counts/batches,
depth limits, cancellation without output writes and completion independent of
unrelated work. Cull tests cover reference ordering, masks, two views, invalid
bounds, off-camera casters, hidden-candidate/mesh/bounds/camera/ancestor edits,
remove/recreate, scene clear after capture, cancellation and retirement from a
submission callback.

DX11 tests compare complete pixel buffers from two camera views against the
synchronous native scene reference, then reject stale-transform dependencies
without changing the last complete output. The active Claw frame contract also
uses a real material-aware mesh and rejects scene retirement after drawing.
Device evidence: Microsoft Basic Render Driver/WARP, feature level `b000`, driver
`10.0.19041.7802`. This proves selected headless/device contracts, not a live Editor
preview, hardware performance gain or whole-solution certification.

Reconfigure with `cmake --preset claw-contracts` and `cmake -S . -B project_x64`.
Build the isolated targets in Debug/RelWithDebInfo; build the full Debug targets
`WorkphoneGraphicsSceneJobTests`, `WorkphoneGraphicsRendererTests`,
`WorkphoneGraphicsFrameTests`, `WPGraphicsClawDX11TargetTests` and
`WPGraphicsClawProductionTests`, with normal project references enabled. The
replacement DX12 source initially failed to link because its Windows SDK C COM
helpers were disabled; a source-specific `COBJMACROS` CMake definition resolves
that build failure without editing its renderer implementation. This is not DX12
rendering certification.

Evidence logs: `project_x64/build_scene_jobs_verified_debug.log`,
`build_scene_jobs_final_order_debug.log`, `test_scene_jobs_verified_debug.log`, and
`project_graphics_contracts/{build,test}_scene_jobs_verified_{debug,release}.log`.
The historical RTT-only checkpoint below records the earlier validation scope.


## Native RTT job and DX11 execution - 16 October 2026

`workphone_graphics_rtt_job.h/.c` now implement an opaque, synchronous C90 job
with copied descriptors, optional retained contexts, dependency readiness,
pre/post generation validation, terminal status/result, cooperative cancellation,
and guarded reconfiguration/destruction. Finish is the non-cancellable
publication/restore boundary. GPU execution stays on the render owner; this is
not a worker immediate-context job or a queued completion/fence implementation.

The DX11 adapter accepts RGBA8-UNORM with the existing legacy LDR draw policy.
It uses a separate D3D11.1 context state and reusable per-target scratch colour
and depth buffers. Clear/load and draw operate on scratch; only a successful
post-draw check submits copies to the destination. Output bindings, viewport,
scissor, blend/shader bindings and native input caches are restored on failure
and success. Native constants are marked for re-upload by the next native draw;
this does not undo arbitrary modifications to callback-owned resource contents.
Scratch colour+depth is limited to 256 MiB per target, with no aggregate residency
certification. `OK` confirms ordered command submission, not GPU fence completion.

The adapter rejects stale renderer/target generations and extents, wrong-thread
execution, recursive RTT, active HDR/shadow scopes at entry, resize/Present during
RTT, and sampled destination hazards. Native texture/material bindings record a
sticky hazard so unbinding before job completion cannot hide a self-reference.
Scene/view validation and completed native cull results are checked through callbacks.
Claw now prepares transform/cull results before offscreen execution and retains
scene/camera references through post-draw validation (see the current slice above).

`ClawHammerSystem` executes offscreen scene/pipeline/UI work inside the native
job, catches C++ exceptions before returning through C, closes active pipeline
work on failure and restores the previous C++ target/viewport/camera. A failed
pass retains the previous complete texture. `ClawRenderTarget` also stages a new
allocation before retiring the previous one; successful replacement gets a new
native generation. The window frame path keeps its existing presentation policy.

Backend support: DX11 implements this first adapter and requires D3D11.1 context
state support. Native DX12 has no matching RTT target/job adapter; the native
Vulkan implementation still contains creation/draw/presentation stubs. All three
graphics APIs can render offscreen, but this implementation is currently DX11 only.

Validation: five selected Debug contracts pass: `WorkphoneGraphics.rtt_job`,
`WPGraphics.claw_dx11_targets`, `claw_text_contract`, `claw_production_render` and
`cubemap_pbr`. The isolated C90 RTT contract also passes in Debug and
RelWithDebInfo. The DX11 test compares the complete RTT pixel buffer with a
synchronous reference, checks blend factors/sample mask and PS/VS/CS bindings,
output/depth/viewport restoration, pending dependencies, stale renderer/target
and post-draw scene validation, wrong-thread/recursive execution, mid-pass resize
and Present rejection, persistent/transient sampling hazards, and failed-draw
pixel retention. Active Claw frame tests cover real job consumption, scene
exceptions, preparation failure before begin, previous-target restoration,
failed replacement retention and successful resize generation replacement.

Device evidence is Microsoft Basic Render Driver/WARP, feature level `b000`,
driver `10.0.19041.7802`. Logs are in `project_x64/graphics-rtt-debug-verified-tests.log`,
`graphics-rtt-debug-isolated-adapter-build.log` and
`graphics-rtt-contracts-{debug,release}-build.log`. The native DX11 library was
rebuilt from the modified source before the final adapter/test build. Another
chat is replacing the native DX12 source; the final selected C++ targets were
rebuilt with project references disabled against those already-built native
libraries. This is selected DX11 validation, not a full rebuild/certification of
the in-progress DX12 backend or the full solution.

Live Editor previews, queued scheduler integration,
scene-linear/HDR output, GPU completion/per-job profiling, real device-removal
recovery, long stress and hardware performance remain open.


## Resize rejection and active-pass safety - 16 October 2026

The native DX11 resize path rejects requests during active HDR/shadow passes,
whose saved output views must remain valid until resolve/restoration. A resize
to the current valid extent preserves existing targets. When DXGI rejects
`ResizeBuffers`, the native path recreates and rebinds the retained window
outputs. The C++ adapter uses the actual swap-chain dimensions for its viewport
instead of publishing the rejected requested extent, and retries on a later
window target setup.

Validation: all four selected Debug graphics targets rebuild and their CTest
contracts pass (`claw_dx11_targets`, `claw_text_contract`,
`claw_production_render`, `cubemap_pbr`). The target fixture retains a real
swap-chain back buffer to force native and wrapper ResizeBuffers failure,
checks native output rebinding, actual viewport dimensions, clear/readback and
successful retry, and checks same-extent target retention and HDR/shadow resize
rejection. Evidence is in `project_x64/graphics-resize-debug-final-build.log`
and `project_x64/graphics-resize-debug-tests.log`. Device: Microsoft Basic
Render Driver/WARP, feature level `b000`, driver `10.0.19041.7802`.

This validates resize rejection recovery on a live device. RelWithDebInfo,
device-removal recovery, failed rollback allocation, interactive Editor behavior
and hardware frame pacing remain unverified.


## Checked native presentation - 16 October 2026

DX11 presentation now has a success-reporting native entry point. It checks the
swap-chain window identity and rejects offscreen outputs and unfinished HDR or
shadow passes before calling DXGI. The existing void presentation API delegates
to the same implementation. Native diagnostics retain the last HRESULT and count
presentation attempts, occlusion and failures separately. Only `S_OK` completes
a measured frame; occlusion/failure breaks the interval sequence so a suspended
window cannot inflate the next successful frame interval.

`ClawRendererDX11::endRender()` finishes the native pass without invoking the
generic end-frame function, which already presents with VSync enabled. This
removes the second Present previously issued for a Claw window and keeps
offscreen/detached target completion from presenting the swap chain. The adapter
uses the checked path once for graphics windows, including window adapters. It handles a missing application/graphics system or
detached target and logs DXGI presentation failures. Its existing frame guard
prevents repeated `endRender()` calls from presenting twice. VSync remains an
engine setting passed through the adapter; Direct3D calls and result accounting
remain native.

Validation: fresh Debug builds of `WPGraphicsClawDX11TargetTests`,
`WPGraphicsClawRendererTests`, `WPGraphicsClawProductionTests` and
`ClawCubemapTests` succeed; their four selected CTest contracts pass. The target
test reports three attempts and three completed submissions for three window
frames, including repeated end calls, missing application state, foreign/missing
window rejection, offscreen wrapper completion, detached targets, unfinished
HDR/shadow rejection, resize and back-buffer readback. The broader test build
also required repairing a missing closing bracket in a foliage fixture include.

Evidence is in `project_x64/graphics-presentation-debug-final-build.log`,
`graphics-presentation-debug-target-final-build.log` and
`graphics-presentation-debug-tests.log`. The device is Microsoft Basic Render
Driver/WARP, feature level `b000`, driver `10.0.19041.7802`. This run returned
`S_OK`; occlusion and device-removal outcomes were not injected. RelWithDebInfo,
recovery, multi-window swap chains, interactive Editor acceptance and hardware
frame pacing remain unverified. A successful DXGI submission alone does not
prove a displayed or visually correct image.

## Bounded frame grabber capture — 16 October 2026

`IGraphicsWindow` now offers a success-reporting RGBA8 capture method. Backends
without readback support return false; `ClawWindow` forwards successful capture
from the current DX11 back buffer. `FrameGrabberStandard::update()` asks for the
default window, validates positive dimensions and an exact four-byte-per-pixel
allocation, and publishes only a successful capture. It no longer treats the
32-bit colour depth as 32 bytes per pixel. `addFrame()` and `popFrame()` now use
a three-frame FIFO, dropping the oldest frame when full, and forward to a video
stream only if one exists. The currently configured video stream factories
return null, so the grabber remains usable through `popFrame()` without them.

Validation: the Debug `ClawCubemapTests` build and `WPGraphics.cubemap_pbr` pass
with a focused grabber fixture that checks failed-capture suppression, exact
buffer size and the bounded FIFO. The Debug DX11 target contract also passes.
RelWithDebInfo validation follows separately. Automatic render-loop scheduling,
video encoding, and post-Present capture remain open.

## Native window back-buffer readback — 16 October 2026

`ClawWindow::copyContentsToMemory()` now uses a native DX11 staging copy of the
current swap-chain back buffer. It accepts `Auto` or `Back` before Present and
returns top-down, tightly packed RGBA8. A `Front` request is rejected because
the presented front buffer is not available through this swap-chain path. The
native API checks the expected window handle, texture format and dimensions,
and caller buffer capacity before writing pixels. Offscreen and window capture
share the same bounded row-copy implementation.

Validation: Debug and RelWithDebInfo `WPGraphicsClawDX11TargetTests` builds and
`WPGraphics.claw_dx11_targets` runs pass. The GPU test clears the window back
buffer, checks captured pixels and an output guard, and verifies that a wrong
window handle or undersized buffer causes no writes. This verifies capture
before Present on the local DX11 device. `FrameGrabberStandard` can now queue
successful captures; video streams and capture timing across Present remain open.

## Native offscreen target readback — 16 October 2026

`ClawRenderTarget::copyContentsToMemory()` now copies its allocated offscreen
colour surface through a native DX11 staging readback. The result is top-down,
tightly packed RGBA8 with four bytes per pixel. The native API checks target
format and dimensions and requires enough caller-owned output capacity before
copying, leaving undersized buffers untouched. Offscreen targets have one colour
surface, so `FrameBuffer` selection has no effect. `swapBuffers()` remains
unwired for this target type.

Validation: Debug and RelWithDebInfo `WPGraphicsClawDX11TargetTests` builds and
`WPGraphics.claw_dx11_targets` runs pass. The test clears an odd-width resized
target to a known colour, checks pixels across multiple rows and an output guard,
and confirms an undersized buffer stays untouched. This is a synchronous CPU
readback contract; window presentation and frame capture remain separate work.

## Cached cubemap device identity — 16 October 2026

`ClawCubemapTexture` checks that its cached filtered view belongs to the
current DX11 device before returning it. If the renderer's device changes,
the wrapper releases the old cube and prepares a replacement from its source
faces. The check also handles a renderer object whose address is reused across
device creation. A null device cannot return the previous view.

Validation: `WPGraphics.cubemap_pbr` creates a second DX11 device, confirms the
cached view is rejected there, then creates and reads a replacement cubemap on
that device. It also unloads and reloads the same `ClawRendererDX11` object
while a `ClawCubemapTexture` holds six ordinary `ClawTexture` faces. The wrapper
returns no view while unloaded, rebuilds on the new device, and verifies that
all six faces reupload and a sampled cubemap pixel is preserved. Debug and
RelWithDebInfo builds and focused test runs pass. Full device-loss recovery in
the Editor remains unverified.

## Native cubemap GPU ownership — 16 October 2026

`ClawCubemap` now holds one opaque native owner for its filtered DX11 view and
six retained source views. Native creation checks that all sources are present
or all absent and belong to the requested device, uploads the complete cube,
then retains their identities. Failed creation leaves the previous owner
visible; reset and replacement release the view and sources together. The
public `ClawCubemap` API and CPU GGX filter are unchanged.

Validation: Debug and RelWithDebInfo `ClawCubemapTests` build, and
`WPGraphics.cubemap_pbr` passes in both configurations. The added contract
releases the caller's source view, reads it through the native owner's retained
reference, and checks owner matching and destruction. Device-loss recovery and
long-running resource stress remain unverified.

## Native cubemap source-face readback — 16 October 2026

The DX11 staging texture, selected-mip copy and mapped-row handling used by
`ClawCubemap` now live in the native renderer. Its bounded readback API accepts
same-device 2D shader views in BGRA8, RGBA8, RGBA16F or RGBA32F form and returns
owned, tightly packed face bytes. C++ keeps the existing sRGB/half-float decode
and GGX filter. Failed readback or invalid radiance still leaves the previous
complete cubemap visible. The readback is synchronous and is used during cube
preparation, not on each frame.

Validation: Debug and RelWithDebInfo `ClawCubemapTests` build, and
`WPGraphics.cubemap_pbr` passes in both configurations. The contract checks
native row packing, rejection cleanup, supported face formats, orientation,
filtered levels and PBR pixel output. This does not measure preparation cost or
establish cubemap/device-loss recovery.

## Native filtered cubemap upload — 16 October 2026

`ClawCubemap` still reads and filters source faces in C++, but sends its 48
face/mip buffers to the native DX11 module for immutable RGBA32F cube creation.
The native upload validates every face, mip dimension and float count before
creating the texture and shader-resource view. Publication stays transactional:
an invalid candidate leaves the previous cube in place. Existing face caching
and studio fallback behavior remain in `ClawCubemap`.

Validation: Debug and RelWithDebInfo `ClawCubemapTests` build, and
`WPGraphics.cubemap_pbr` passes in both configurations,
including face orientation, filtered mips, PBR draw readback, reuse, incomplete
face rejection and the new native incomplete-mip rejection. This confirms the
selected DX11 contract only; device-loss recovery and broader lifetime stress
remain open.

## Native RGBA32F texture upload — 16 October 2026

The sampled linear-float texture path now calls a native DX11 upload API instead
of creating Direct3D textures and shader-resource views in `ClawTexture.cpp`.
The native API checks the device-supported dimensions, exact RGBA float count
and 256 MiB upload limit before creating an immutable `R32G32B32A32_FLOAT`
resource. The C++ wrapper retains its existing staged replacement and previous
visible texture on upload failure. The DX11 target contract checks invalid
inputs and reads back HDR pixel values from the uploaded resource.

Validation: the Debug `WPGraphicsClawDX11TargetTests` and `ClawCubemapTests`
targets build, and both `WPGraphics.claw_dx11_targets` and
`WPGraphics.cubemap_pbr` pass on the current local DX11 device. The build also
exposed two refactor-related issues: `FoliageAuthoring.cpp` lacked the local
`FoliageJsonBudget.hpp` declaration, and a dependencies-off build linked the
absent `Meshopt.lib`. The missing include and conditional CMake target link
are repaired. These are selected Debug contracts, not a full suite or
hardware performance result. This is one texture upload boundary increment; the other native
target/camera, resource lifetime, frame snapshot and presentation work below
remains open.

## Native frame and geometry ownership — 12 October 2026

This increment advances the C90 implementation/C++17 wrapper boundary. It does
**not** complete the graphics production plan or establish AAA visual parity.

- `wp_graphics_system_render()` now preserves legacy frame callbacks, matching
  `render_frame()`. The convenience function delegates to the same update/render
  implementation. Registered pass order, recursion guards and mutation rejection
  remain native. Completed-frame diagnostics expose update/render serials,
  completed/skipped target sequences and the clamped timestep.
  Completion counters describe callback dispatch, not successful GPU draws;
  visual output and backend failures require separate checks.
- `ClawHammerSystem` drives native `update()` then `render()`. It publishes pass
  descriptors and retained C++ contexts together, skips detached/minimized
  targets, and publishes a mutex-protected diagnostics snapshot for Editor and
  sample readers. Window activation retains the existing window contract.
- Native mesh code now converts all seven vertex formats to lit vertices,
  generates missing smooth normals and invalidates prepared data after topology,
  vertex or index edits. Atomic content generations distinguish address reuse
  across independently created worker meshes. Setters safely accept their own
  buffers as inputs and check allocation-size overflow.
- Native DX11 owns per-renderer hashed geometry caches and triangle-strip range
  conversion. WPGraphics retains material/scene adaptation while delegating
  vertex conversion, GPU geometry allocation, reuse and destruction to C.
  Native clients call `wp_renderer_dx11_forget_mesh()` on the render owner before
  destroying cached meshes for immediate GPU retirement; `ClawMesh` already
  follows that lifecycle. Returned geometry pointers are borrowed.
- VehicleAdvanced capture/benchmark reports native frame counters and requires
  a completed native target with one update per render. Warmup starts on the
  render task after loading, so procedural generation cannot consume the entire
  measurement interval before the first frame. Warmup and measurement each
  require at least 30 rendered sample updates as well as their elapsed-time
  threshold. Loading resets review state.
- VehicleAdvanced reuses the existing foliage plant generator for four stable
  branching tree variants. Each near tree uses 548 triangles; far impostors use
  the matching variant in a shared atlas. Existing patch batching, LOD and
  collision paths are retained.
- The isolated C90 contract preset now includes mesh conversion/serialization
  and the correctly registered frame target. Full builds use the real native
  scene implementation. Disabled wrapper/DX11 regression targets are restored.

Validation: fresh Debug builds of the modified native libraries, WPGraphics,
three wrapper/DX11 test executables and SampleVehicleAdvanced succeed. Seven
selected full-engine contracts pass: mesh, frame, renderer, shader, wrapper text,
DX11 targets and production rendering. Four isolated native contracts pass in
both Debug and RelWithDebInfo. New GPU checks compare complete offscreen pixel
buffers from cached/native and direct reference geometry, including indexed
lists, strips, odd-start submesh ranges, generated normals, packed colors and
nonindexed meshes under all cull modes. Shadow pixel tests also use native cached
geometry. Device evidence is Microsoft Basic Render Driver/WARP, feature level
`b000`, driver `10.0.19041.7548`; this is not hardware performance certification.
Local logs/XML are in `project_x64/graphics-native-frame-evidence/`.

The workspace art inventory, including ignored/cooked resources and the media
SQLite catalog, contains two bundled meshes and four textures, but no production
car, track, foliage, racing PBR sets or HDR environment suitable for the requested
comparison. Existing procedural geometry and generators are therefore reused;
no external art has been substituted.

Fresh final sample evidence is `canopy-car.bmp`, `canopy.stdout.log` and
`canopy.stderr.log` in that directory. Native diagnostics report 59 updates,
59 renders, one dispatched target sequence and no skipped sequences; generated
mesh winding validation passes for 121,826 triangles and the process exits 0.
The 29-frame measurement
reports mean 207.971 ms and p95 221.204 ms, so this run does not establish a
shipping frame budget. Startup emits three rejected particle-timestep warnings
and the audio device cannot open; effects/audio acceptance is not claimed.
The visual review rejected the first sparse canopy, then rebuilt and captured
denser overlapping boughs. The final crowns read more clearly as trees, but the
overall car, materials, lighting, track detail and aliasing still fail the
requested AAA visual gate. No blind commercial-reference test was performed.

The inspected baseline VehicleAdvanced capture remains visibly prototype quality:
simplified car/aero/wheel geometry, faceted repeating trees, sparse scenery,
coarse repeating asphalt and weak lighting/reflection cues. No blind comparison
or claim of superiority to the official
[F1 2026 season reference](https://www.ea.com/games/f1/f1-25) is supported.

Open architecture work includes native target/camera setup and presentation,
material/environment records, render snapshots, draw-list/shadow preparation,
texture/cubemap upload and lifetime management, and native GPU post-processing.
Direct3D implementation remains in several C++ texture, cubemap, terrain,
resource and renderer units. The existing software post-processing pipeline is
not a GPU HDR/TAA/SSR implementation. The broader production, asset, physics,
hardware performance, recovery, Editor, packaging and visual acceptance gates
remain independent and incomplete. Full C++ RelWithDebInfo validation is not
claimed by this increment.

## Paged foliage, authoring and cooked resources - 11 October 2026

Started on `brodex` at `d393187aa` and incorporates the subsequent remote commits
through `d9c5e4842` (tile-demand/matrix changes and typed material texture loading).
The previous terrain delivery is retained in `d525603fa`. Intervening work adds DDS profiles,
safer asset saves and reference handling, project reindexing, and bundled media
catalog mounting. This increment preserves those changes and restores disabled
graphics/terrain test registrations while retaining the new asset test sources.

The connected foliage path now includes:

- Validated species, signed cells and stable instance identities; asynchronous
  native paging through the shared scheduler, cancellation, multi-view demand and
  bounded source/result/work/GPU reservations.
- DX11 cross-cell instance groups, per-view culling and projected near/middle/far
  LOD with hysteresis, grass density subsets, textured billboards, wind,
  interaction bending and matching cutout/shadow deformation. The existing
  fixed-LOD instancing ABI remains compatible.
- Deterministic native plant/scatter recipes, terrain/biome/moisture masks and
  exclusions, manual edits and persistent IDs, paint/erase, selection and transforms,
  bounded undo/redo, source import/export, Lua controls and live diagnostics.
- Textured multi-view impostor baking with alpha, normal/ORM/depth output, gutters,
  cancellation and explicit CPU texture availability.
- Cooked `.folspec`, `.folpage` and `.folworld` resources with pinned dependencies,
  demand-loaded pages and source-independent installed runtime loading.
- Conservative extent metadata keeps off-centre, scaled and wind-deformed plants
  resident across page boundaries. Native transform validation precedes render and
  collision publication; invalid candidates preserve the previous generation.
- Compact resource cache keys and immutable foliage export paths avoid Windows
  path-length failures while existing cache entries remain readable.
- Nearby tree/bush collision and gameplay promotion/demotion/removal. Candidate
  collision bodies stage before publication; failures retain the previous source,
  pose and native bodies. Terrain render/physics publication shares the scene lock.

Validation completed on 11 October 2026 for the modified working tree based on
`d9c5e4842`, using CMake 4.4.4, Visual Studio 2026 / MSVC 19.51, Windows SDK
10.0.26100 and NVIDIA RTX 3090 DX11 hardware (driver 32.0.16.1692).
**Debug: 20 passed, 1 unavailable. RelWithDebInfo: 20 passed, 1 unavailable.**
The Editor builds in both configurations. The unavailable test is
`WorkphoneGraphics.mesh_import_assets`, whose six external Ogre/OgreNext mesh
fixtures are absent; it is not counted as a pass.

The foliage contracts exercise deterministic authoring, concurrent reads/edits,
1,000 paging cycles, cancellation and retained budgets, native collision rollback,
DX11 wind/alpha/shadow and extent-aware paging, invalid-transform publication
rejection, cooked offline loading and actual Lua paint/undo/save/load/bake actions.
Resource contracts also cover long logical IDs and pre-compact cache paths in
authoring and installed manifests. All 175 recorded source hashes match the final
tested code in each configuration. Local XML/JSON evidence is retained under
`cmake-build-debug-vs2026-readiness/claw-graphics-{configuration}-*` and records the
base revision, modified-file status, source/binary hashes and GPU device evidence.
This is a selected-target local build, not a clean-machine package certification.

Additional incoming-change checks pass in both configurations:
`WPLua.runtime_contracts` (including typed texture loading) and
`Tests/lua/MaterialEditorTextureDropTests.lua`. Their logs and Lua runtime XML
reports are retained under `cmake-build-debug-vs2026-readiness/foliage-extra-lua-*`.
These two checks are additional to the 20 passing baseline tests above.

See [Foliage workflow](FOLIAGE_WORKFLOW.md) and the
[plan implementation matrix](WPGRAPHICS_FOLIAGE_PRODUCTION_PLAN.md#paged-foliage-and-authoring-implementation---11-october-2026)
for supported tiers and bounds. GF0/GF1 certification still requires representative
forest frame-time/overdraw measurements, interactive Editor visual acceptance and
the eight-hour soak. Standalone cooked page consumers supply an authoritative
instance snapshot for collision; disk-paged collision is not automatic.

## Terrain runtime and expanded authoring — 11 October 2026

Continues on `brodex` at `ce2aecf5b`, matching the remote branch when checked.
The intervening branch work adds immutable resource manifests and input snapshots,
catalog browsing, asset-reference/lifetime fixes, SQLite integration, and audio
queue/cache changes. Previous terrain, foliage and animation implementations are
retained. The moved graphics test directory and commented terrain/graphics target
registrations have been repaired so validation actually builds those contracts.

The current terrain changes connect these paths:

- Version-three rectangular terrain assets contain four PBR layer descriptors,
  normalized control weights, wetness/snow masks and cell holes. Version-one and
  version-two sources migrate. CPU
  sampling, picking, render geometry, collision, navigation input and shoreline
  intersections share the same triangulation and hole policy.
- DX11 stages all layer maps, control data and resident tile geometry before
  publication. Source, runtime queries, collision and rendered generation advance
  together; failed preparation or a busy physics scene retains the previous
  complete runtime generation. Authoring has an explicit requested revision.
- Terrain appearance includes height blending from ORM alpha, triplanar projection,
  detail normals, seeded macro tint and masked wetness/snow. Persistent controls
  and mask brushes participate in undo; defaults retain earlier asset appearance.
- Terrain uses 64-cell tiles and four distance/error-selected LODs. Coarser interiors
  retain every shared border sample and source-wide normal, avoiding cracks
  between different LODs. Hole tiles remain exact. Camera/shadow demand, frustum
  prefetch, hysteresis, cancellation, bounded worker queues and GPU upload/residency
  budgets replace global nearest-sample reduction. Both shadow and colour passes
  consume one published generation.
- A shared world streaming scheduler replaces terrain-specific worker threads.
  Producers share bounded CPU preparation, priority/fairness and memory admission;
  tile GPU reservations follow actual buffers through staging, reuse and retirement.
- The existing Lua editor drives grouped/cancellable viewport strokes, sculpting,
  layer painting, holes, image/raw import and actual height/OBJ exports. Native
  inspector integration provides world-ray input and a sampled brush outline.
- Thermal/hydraulic erosion, terrace/noise, height/slope masks and road/river grading
  produce real data. Erosion reads authoritative neighbour halos and writes only
  the selected region. Ordered recipes retain a base, operation parameters and
  deterministic seed; their cancellable worker prepares a geometry/control-weight
  viewport overlay, then commits as one undo command with stale-source protection.
- Terrain-grounded foliage retains stable placement identities and resamples dirty
  ground, density/exclusion-mask and filtered layer-weight regions. Retained
  consumer bindings rebuild selected ground, navigation, shoreline and path inputs
  after an active terrain publication, preserving their previous complete bundle
  on failure. Explicit OBJ/JSON exports remain available. Cooked terrain and its
  texture closure load through ResourceSystem without an authoring database or compiler.

See [Terrain workflow](TERRAIN_WORKFLOW.md) for supported controls, formats,
transaction behavior and limits. This implementation does not certify the full
graphics R1/R2 programme. In particular, full-source collision has a 524,288
triangle budget; terrain CPU source/control data stay resident; adoption of the
shared scheduler by other world producers, disk-paged collision and the independent foliage/navigation/water
engines remain separate integration work. GT0/GT1 release evidence also requires
interactive visual acceptance and representative performance/package/soak runs.
The [capability matrix](TERRAIN_WORKFLOW.md#capability-and-evidence-matrix) maps
TERR-01 through TERR-10 to concrete code, tests and unsupported cases; the
[resource format reference](WPGRAPHICS_TERRAIN_RESOURCE_FORMAT.md) covers cooking
and migration. The appearance tier remains opaque four-layer PBR with optional
detail and surface effects; it does not introduce a weather simulation.

Validation completed on 11 October 2026 using CMake 4.4.4, Visual Studio 2026 /
MSVC 19.51, Windows SDK 10.0.26100 and the NVIDIA RTX 3090 DX11 device (driver
32.0.16.1692). **Debug: 19 passed, 1 unavailable. RelWithDebInfo: 19 passed,
1 unavailable.** The Editor builds in both configurations. The only unavailable
test is `WorkphoneGraphics.mesh_import_assets`, which requires six external
Ogre/OgreNext fixtures absent from this checkout. Both evidence reports record
`validationPassed: true`; coverage remains incomplete and release certification
is false. All 139 recorded source hashes match the final implementation.

The terrain CPU/scheduler, native physics, actual Lua workflow, DX11 production
and source-free cooked-resource suites pass in both configurations. Coverage
includes shared budget rejection/recovery and lease retirement, coherent
collision/render publication, all expanded material effects, thirteen pinned
texture dependencies, v1/v2/v3 migration, recipe save during preparation, and
undo followed by a different appearance/layer edit. The native Editor viewport
was built; an interactive Editor walkthrough was not performed.

Representative local fixture measurements from these runs:

| Fixture | Debug | RelWithDebInfo |
|---|---:|---:|
| CPU stream: 1025 × 1025 source, 256 LOD3 tiles | 487.517 ms | 479.969 ms |
| Thermal erosion: 129 × 129, 16 iterations, seed 97 | 75.156 ms | 29.479 ms |
| Hydraulic erosion: same dimensions/iterations/seed | 92.348 ms | 38.562 ms |
| GPU residency: 129 × 129, four tiles, cold preparation | 49.164 ms | 48.487 ms |
| Ten unchanged GPU preparations | 0.113 ms | 0.031 ms |

Both streaming runs peak at 8 MiB of reserved ready-result capacity. The four-tile
GPU fixture retains 76,176 geometry bytes and uploads zero geometry bytes during
its ten warm preparations. Erosion has a 13 MiB simulation-array ceiling at its
262,144-sample cap; this is not a measured whole-process peak. These are local
microfixtures, not a mixed-scene frame-time or soak acceptance result.

Reproduce with `Tools/BuildWPGraphicsBaseline.ps1 -Configuration Debug,RelWithDebInfo -IncludeEditor`.
The current local reports are
`cmake-build-debug-vs2026-readiness/claw-graphics-{Debug,RelWithDebInfo}-results.xml`
and the corresponding `-evidence.json` files. Build logs are
`baseline-{Debug,RelWithDebInfo}-build.log` in that directory. The workflow's
capability matrix retains the outstanding interactive, representative
performance/soak and shipping-package acceptance work.


## Foliage, animation and shared-resource foundations - 10 October 2026

Continues on `brodex` at `eead5c43d`, matching the remote branch when checked.
The previous terrain/Editor increment is preserved in `2a7413752`. Intervening
changes migrate audio to WorkphoneAudio, harden networking and its tests, add
implementation plans and remove generated build artifacts.

This delivery implements the first connected portions of the three production
plans, not their complete R1 release outcomes:

- **Foliage:** real DX11 indexed instancing, retained scene batches sharing one
  fixed-LOD mesh, explicit matrices/tints, section materials, cutout/shadow state
  and scene/camera/viewport masks. A reusable 4,096-instance GPU buffer bounds
  upload storage; populations split into actual draws. Replacement rejects stale
  ownership and retains snapshots through both passes. The synthetic two-section
  10,000-tree contract compares scalar, instanced and ideal merged coverage and
  counts (20,000, 6 and 2). This proves draw reduction; the recorded single-sample
  CPU submission timings do not establish a frame-time speedup or a full forest benchmark.
- **Animation:** both actual Assimp routes interpolate independent channels and
  preserve bind defaults, with transactional rejection of unsupported/malformed
  curves. Runtime rotations interpolate correctly and sparse tracks use their
  own time indices. Shared clips resolve the requested skeleton per application; editing the initial
  time-zero key now modifies the retained key.
  Claw clones share immutable bind vertices while owning native deformation output.
  New fixtures exercise the real glTF routes, independent skeletons and DX11 clone
  survival; existing graph and IK tests are now required baseline tests.
- **Resources:** a versioned WPRS manifest pins target and the exact install
  dependency closure. `ResourceSystem::initializeRuntime` mounts an existing
  cooked tree without source files, compiler registry, SQLite or writes. Requests
  validate identity, versions, hashes and bounded closure size/depth before cache
  publication; shared dependencies retain one live identity. Authoring recooks
  invalidate stale lookup generations while callers may retain older immutable
  objects. This is synchronous loading of immutable packages, not asynchronous
  residency or multi-process package publication.

Validation on 10 October 2026 passed the full required baseline in both Debug
and RelWithDebInfo: **18 passed, 1 unavailable** in each configuration, using
CMake/CTest 4.4.4, Visual Studio Community 2026, MSVC 19.51.36260, Windows SDK
10.0.26100 and NVIDIA RTX 3090 DX11 hardware (driver 32.0.16.1692). The sole
unavailable test is `WorkphoneGraphics.mesh_import_assets`; its six external
Ogre/OgreNext fixtures remain absent. This is not full release certification.
The new required baseline tests are
`WPAnimation.core_contracts` and `WPAnimation.import_contracts`; the existing
Claw production and resource smoke executables include the new GPU/runtime
contracts. Source hashes in the evidence report include the new fixtures and
production files. Focused importer builds now have proper CMake dependency edges
for their bundled libraries, including tinyusdz, instead of relying on prebuilt
`.lib` files. The importer export definition is now private to its DLL producer.

Reproduce with `Tools/BuildWPGraphicsBaseline.ps1 -Configuration Debug,RelWithDebInfo`.
The current run used `cmake-build-debug-vs2026-readiness`; per-configuration
`claw-graphics-*-results.xml` and `claw-graphics-*-evidence.json` record test results,
GPU/driver details and source/binary hashes. Interactive Editor authoring was not
exercised for this increment.

At that 10 October checkpoint, remaining work included foliage paging/culling,
near/mid/far LOD, grass/scatter/persistence and cooked species; typed animation
cooking and the automatic Animator-to-Claw bridge, normalized pose blending,
parented scene local/world ownership and Editor authoring; durable resource
identity, typed runtime installers, generation publication, async residency and
package tooling. The existing Lua Editor tools remain the authoring integration
point. See [foliage](WPGRAPHICS_FOLIAGE_PRODUCTION_PLAN.md),
[animation](WPANIMATION_PRODUCTION_PLAN.md) and
[resources](WPRESOURCE_ASSET_PRODUCTION_PLAN.md) for the precise contracts.

## Terrain data and Editor workflow increment — 10 October 2026

Starts from `brodex` at `ccb890ed6`. The merge preserves the cooked-material
increment and adds Lua runtime, reload, debugger and tooling changes. The remote
`brodex` SHA was verified over HTTPS after the machine's SSH authentication failed.

This implements the first connected terrain increment from
[the terrain plan](WPGRAPHICS_TERRAIN_PROCEDURAL_REVIEW.md): TERR-01 foundations,
the query/picking/CPU mesh portion of TERR-02, and basic TERR-04 authoring.
It does **not** complete GT0, R1, or the whole terrain/procedural programme.

- Immutable rectangular `TerrainData` snapshots carry dimensions, spacing, explicit
  sample-zero origin, height scale and row-major samples. Revisions reject stale
  commits; validation bounds allocations and rejects invalid/nonfinite or collapsed
  geometry. Maximum source size is 4,194,304 samples, with at most 8,193 per axis.
  Zero and negative height scales are supported. Object transforms support finite
  translation, positive nonuniform scale and normalized yaw rotation; pitch/roll
  and singular transforms are rejected without replacing the valid placement.
- Queries, exact triangle rays, CPU meshes and Claw native geometry share the same
  cell diagonal and height units. Queries retain snapshots without copying all
  samples. Legacy setters preserve the old `-dimensions/2` sample-zero anchor;
  newly generated assets use `-(dimensions-1)/2`. Origin is saved explicitly.
  Claw renders the full rectangular grid without the old 257-sample decimation.
  Native mesh construction retains the previous mesh if preparation fails.
- `TerrainSystem` owns actual samples independently of renderer lifetime.
  Version-one `workphone.terrain` JSON saves dimensions, spacing, origin, scale and
  all samples; malformed files leave the current revision intact. Scene `toData`
  carries this payload while normal inspector properties remain editable.
  Sample files use terrain-local coordinates; actor placement belongs to the scene.
  Legacy scenes without samples migrate to a flat grid with their original anchor.
  Renderer attachment/recreation restores an actor's existing placement; invalid
  actor placement rejects attachment before changing the retained runtime.
- Existing Lua TerrainEditor exposes local X/Z brush centres and real raise,
  lower, smooth and flatten actions. Each action makes one native undo command.
  A patch is limited to 65,536 changed samples and the existing managers retain
  at most 100 commands; their cursor/eviction/redo-branch handling is repaired.
  MT history blocks undo/redo while commands are pending or running. Explicit
  removal, clear and unload cancel unstarted jobs; history eviction preserves
  requested operations. Running commands release the history lock, and unload
  defers their cleanup until completion. Undo/redo validates metadata and
  affected sample values, preserving unrelated edits and rejecting conflicts.
  Under the existing command API, a rejected undo
  still advances the history cursor and logs the failure.
- Save, Reload and height-data export operate on actual versioned sample files,
  with same-directory temporary-file replacement. Export JSON also writes actual
  samples, so it cannot overwrite a terrain file with settings-only recipes.
  The separate Export Recipe Settings action appends `.recipe.json` to that path.
  Unimplemented terrain operations report unavailable.

Editor use: select a TerrainSystem, generate a height preset or import a saved
sample file, set **Brush Centre X/Z**, radius and strength, then use a supported
brush action. Raise/lower strength is a terrain-local height delta; smooth/flatten
strength is a 0–1 blend. Use the existing Edit undo/redo commands, set an output
file, and Save/Reload. Viewport drag strokes, tablet/mirror modifiers, image/raw
height import, mesh export, stamp/layer paint, erosion and collision/navigation
builds remain unavailable in this increment.

Validation on 10 October used CMake/CTest 4.4.4, Visual Studio Community 2026,
MSVC 19.51.36260, Windows SDK 10.0.26100 and NVIDIA RTX 3090 DX11 hardware.
The full graphics/catalog/resource baseline passed in both Debug and
RelWithDebInfo: **16 passed, 1 unavailable** in each configuration. The sole
unavailable test remains `WorkphoneGraphics.mesh_import_assets`, whose six
external Ogre/OgreNext mesh fixtures are absent.

The added mandatory targets are `WPGraphics.terrain_contracts` and
`WPGraphics.terrain_lua_workflow`; the existing DX11 production target also checks
terrain edits and full-resolution geometry. Native coverage includes actor
reattachment, JSON/scene round trips, invalid/stale edits and queued history
completion/cancellation/unload. The Lua target loads the real Editor script and
exercises sculpt, undo/redo, save/reload and unsupported-operation errors.

JUnit reports and source/binary/device evidence are in
`cmake-build-debug-vs2026-readiness/claw-graphics-{Debug,RelWithDebInfo}-results.xml`
and the corresponding `-evidence.json` files. `git diff --check` passed. These
local runs do not constitute an interactive Editor acceptance run, packaging
validation or release certification.

Remaining release work includes physics publication/contact evidence, tiled LOD,
four-layer cooked PBR, worker cancellation and shared render/physics revision
publication, streaming, package reload, and an interactive Editor acceptance run.
GPU buffer creation remains lazy in the existing renderer: native mesh staging
does not yet guarantee atomic source/native/GPU replacement after a DX11 allocation
failure. This increment supplies no new large-map performance certification.

The DX11 fixture also exposed a pre-existing rotated-camera issue in
`Engine/c/Source/WorkphoneCore/workphone_matrix.c`: `wp_mat4f_look_at` places its
rotation basis in columns while its translation uses row dot products. The
terrain pixel check explicitly validates the resulting eye-space depths and
does not certify camera placement. Correcting that shared camera path and its
other renderer consumers remains separate work.

## Cooked material/texture consumer increment — 10 October 2026

Implementation starts from `brodex` at `7107f0a39`. The prior catalog/path/adapter
increment is retained in `7809b208c`. Subsequent branch updates add scene/texture/
actor scheduling changes, state-driven lights, tyre effects, importer/dependency
updates and physics narrowphase/BVH/collision-cache work. The docs add shared
resource and animation plans and expand terrain, procedural and foliage scope.
This increment follows the graphics plan's next executable cooked-asset task;
the complete physics, vehicle and Editor suites remain outside its validation.

Implemented:

- Registered `GraphicsResourceCompiler` supplies versioned `texres` and `matres`
  JSON descriptors through the existing compiler registry. Textures cook bounded
  top-down BGRA mip chains with the existing semantic filters. Materials declare
  one texture as compile/install dependency and pin its identity, source/payload
  hashes and compiler version. Typed readers validate lengths, versions, finite
  values, mip layout, integrity, dependency coherence and target/build mode.
- `ClawMaterialResource` resolves a catalog UUID through the existing adapter,
  stages an entire material/texture bundle on its render owner thread and swaps
  it only after validation. The final catalog token check and pointer swap share
  the catalog lock. Compile/decode/upload, compiler callbacks and retired-handle
  destruction stay outside that lock. Publisher identity, monotonically ordered
  requests and a pinned DX11 device reject stale or foreign candidates. Failed
  replacement preserves the last working bundle; explicit consumer unload
  invalidates outstanding candidates. Device changes suppress `current()` until
  a compatible replacement is installed.
- `ClawTexture::uploadCookedMips` stages exact authored mip bytes, retains them
  through unload/reload and device recreation, and rejects invalid or failed
  uploads while preserving the old compatible view. Legacy pixel/property edits
  deliberately leave cooked mode. Cached views now compare actual device identity
  as well as the native renderer wrapper.
- Repaired two regressions from the recent changes: the render owner must apply
  an equal authored transform when that transform is still queued for the native
  scene node; particle traversal must retain a ConcurrentArray snapshot.
- Material passes now retire their own state records and managed contexts when
  rebinding or unloading, releasing textures retained by that state. Supplied
  contexts and pre-existing records remain owned by their caller. Lifecycle
  contracts check texture destruction and managed-context counts.
- Added `WPGraphics.cooked_material_resource` with generated image/descriptors,
  real SQLite/catalog/cook/load, exact GPU mip readback and visible `renderMesh`
  output. Format failures, reload ordering, catalog lifecycle, owner-thread and
  device recreation have dedicated contracts. The existing production test adds
  queued-transform and cooked-texture regressions.
- The baseline now requires 15 tests and hashes the typed compiler/consumer
  sources. The deliberately deleted CI workflow remains absent; the local runner
  records its availability instead of failing to hash a nonexistent file.

Validation: **Debug: 14 passed, 1 unavailable. RelWithDebInfo: 14 passed,
1 unavailable.** The sole unavailable test is `WorkphoneGraphics.mesh_import_assets`,
which still requires the six absent external Ogre/OgreNext mesh fixtures. All
mandatory contracts execute, including actual catalog symlink containment. The
new cooked-resource test uses generated fixtures and has no unavailable cases.

Both configurations used Visual Studio Community 2026, MSVC **19.51**, Windows
SDK **10.0.26100.0**, CMake/CTest **4.4.4**, and DX11 on **NVIDIA GeForce RTX 3090**,
feature level **11.0**, driver **32.0.16.1692**. Configuration-specific JUnit and
JSON evidence is in `cmake-build-debug-vs2026-readiness/claw-graphics-<configuration>-results.xml`
and `claw-graphics-<configuration>-evidence.json`. The JSON records the source
revision/diff, compiler, device, test sources and executable hashes, and the
absent CI workflow. To rebuild and validate both configurations:

```powershell
./Tools/BuildWPGraphicsBaseline.ps1 -BuildDirectory cmake-build-debug-vs2026-readiness
```

See [cooked resource authoring and ownership](WPGRAPHICS_COOKED_RESOURCES.md) for
formats, service setup, exact limits and lifecycle requirements. This advances
DB-07/08/10, GD and shared RES-010/019/021/022 for one synchronous graphics family;
it does not complete those broad gates. Source-free packaged loading, immutable
cook generations/metadata commit, isolated variants, cross-process writers,
input snapshots, asynchronous jobs/watchers and Editor preview adoption remain
open. In particular, ResourceSystem's existing output-before-metadata-commit
failure window can change disk output even while this consumer retains its old
GPU bundle. No R1/R2 or release certification is claimed.

## Catalog identity and resource adapter increment — 8 October 2026

Implementation starts from `brodex` at `22553f942`, following the verification increment committed as `f531e9140`. **Debug: 13 passed, 1 unavailable. RelWithDebInfo: 13 passed, 1 unavailable.** The unavailable test requires the six missing external Ogre/OgreNext mesh fixtures. Catalog coverage includes actual symlink containment and Windows 8.3 root aliases in both configurations, with no internal unavailable cases. Neither R1 nor R2 is certified.

Both configurations used Visual Studio Community 2026, MSVC **19.51**, Windows SDK **10.0.26100.0**, CMake/CTest **4.4.4**, and DX11 on **NVIDIA GeForce RTX 3090**, feature level **11.0**, driver **32.0.16.1692**. Current evidence is in `cmake-build-debug-vs2026-readiness/claw-graphics-<configuration>-results.xml` and `claw-graphics-<configuration>-evidence.json`. Earlier sections retain their original revision context.

The intervening branch changes retain the catalog and baseline tooling. They add DX11 scene fog, semantic colour/data/normal/cutout/roughness mip generation and GPU upload, texture mip properties, serialized LOD detail bounds, a synchronous small-batch LOD path, core container/listener changes, and expanded procedural vehicle scenery, physics capacity and handling tests. The new fog/mip tests exercise native GPU output; the complete vehicle/editor suites are outside this increment's selected graphics scope.

Implemented in this increment, validated in Debug and RelWithDebInfo:

- Schema version 2 adds explicit file/scene entry kinds and a canonical file-path key. File paths are stored relative to a captured project root with `/` separators; lookup keys fold ASCII case on Windows. Windows 8.3 root aliases are accepted and normalized to the canonical root. Equivalent spellings resolve one identity, and paths outside the root, invalid UTF-8, ambiguous Windows paths and unresolved/outside-root links are rejected. Missing source files remain representable. The root is explicitly configurable while closed; otherwise it is captured from the application project or database directory. Relative storage supports project relocation.
- Transactional upgrades from unversioned/version-1 catalogs preserve row IDs and UUIDs, retain original row snapshots (`wp_asset_catalog_backup_v0` / `wp_asset_catalog_backup_v1` as applicable), and fail closed on canonical collisions, reserved backup names or unsupported metadata. Migration also rejects custom triggers whose stored table-name casing differs from the actual legacy table: bundled SQLite 3.7.9 can silently omit those triggers while reloading the altered schema. The original database is preserved for explicit repair; an exact-case abort trigger covers rollback after schema alteration. Existing version-2 rows are revalidated. These are in-database snapshots, not independent backups.
- Detached entry snapshots carry catalog-instance and generation identity. Successful mutations and lifecycle changes invalidate outstanding snapshots; failed/conflicting/idempotent changes do not fabricate new identities. Open/close, narrow/wide loading, switching and root changes use the same catalog lifecycle. Shared virtual interfaces remain unchanged.
- `CatalogResourceAdapter` resolves catalog UUIDs to existing ResourceIDs and delegates compilation/loading to ResourceSystem. Its caller must bind the exact ResourceSystem `sourceRoot`, matching the catalog root, and rebuild the adapter if the service is reconfigured. Explicit catalog-type mappings validate the source extension without changing its type; canonical-key comparison permits ResourceID's lowercase-extension normalization on Windows. Compiler availability, loaded identity/type and compiler version are checked. Compilation hashes, dependencies and cooked containers remain owned by ResourceSystem and its existing database.
- ClawTexture retains authored mip settings through unload/reload, validates atlas metadata, and stages replacement views. A rejected mip candidate preserves the previous same-device GPU view; a successful candidate replaces it. Added wrapper-level regression coverage checks actual GPU pixels, metadata rejection, replacement, caching, reload and unload.
- Fixed standalone inclusion of `ClawHammerSystem.hpp` by using the equivalent engine `f32` declaration instead of depending on a prior native `wp_f32` declaration. Validation evidence now hashes the new contract helper headers and catalog/path/adapter implementation files, so reports identify the code exercised by these additions.

This advances **DB-01/03/04/05/06/07** in part. Catalog/adapter contracts cover canonical aliases, kind separation, migration rollback, relocation, lifecycle snapshots, type/root/version errors and stale requests around compilation. Adapter checks before and after ResourceSystem work reject stale returned results; they do not make compilation or publication atomic with catalog mutations. The existing ResourceSystem compiler can still persist an old output before the adapter detects staleness. No catalog-to-GPU publication transaction, graphics material/texture compiler, subasset identity, packaged runtime manifest or end-to-end cooked graphics draw is delivered here. General mutation/unload races, cancellation, cross-process coordination and durability remain open.

The next executable task is a catalog-resolved cooked material plus texture reaching a DX11 draw, using the adapter with staged render-thread publication and a final generation guard. Failed compilation/upload must preserve the last visible asset across rename/reload/unload. That evidence remains required for **GD** and delivery batch B. Remote CI and external-media release coverage also remain outstanding.

## Verification increment — 8 October 2026

Rebuilt the selected graphics, native, SQLite catalog and ResourceSystem targets on `brodex` at `22e542cca`, with the local verification changes described below. **Debug: 13 passed, 1 unavailable. RelWithDebInfo: 13 passed, 1 unavailable.** All mandatory tests executed; the unavailable test is `WorkphoneGraphics.mesh_import_assets`, whose six external Ogre/OgreNext mesh files are absent. Neither release gate is certified.

The toolchain was Visual Studio Community 2026, MSVC **19.51.36260.0**, Windows SDK **10.0.26100.0**, and CMake/CTest **4.4.4**. The three GPU test executables recorded the actual DX11 device: **NVIDIA GeForce RTX 3090**, feature level **11.0**, driver **32.0.16.1692**. The DX11 target fixture reported the legacy-discard presentation path; this run does not certify flip presentation or representative frame performance.

Changes since the catalog increment at `597c6e5c5` retain schema-v1 migration and add long-text UI/serialization, editor-camera behavior, visibility, winding/capture changes, directional shadow regressions, core state/container refactors, and procedural circuit/open-city racing support. This increment validates the selected WPGraphics contracts; the complete racing/editor/unit suites were not run.

Completed verification work:

- Added `wpgraphics-baseline` configure/build/test presets and `Tools/BuildWPGraphicsBaseline.ps1`, which builds Debug and RelWithDebInfo with required SQLite/ResourceSystem support. The runner selects installed CMake explicitly and normalizes environment-variable names only in the build child process to avoid MSBuild's duplicate `PATH`/`Path` failure.
- Fixed the minimal graphics build's missing `imgui.lib`: WPGraphics now links the `imgui` CMake target so its dependency is built automatically.
- Updated two stale test fixtures to match the current implementation: explicitly load/unload the native Claw light and use the renderer's current front-face winding for the PBR triangle. Existing lighting/PBR assertions remain intact.
- Expanded `ValidateClawGraphics.ps1` to require all 14 registered tests, including catalog and ResourceSystem coverage. It writes configuration-specific JUnit and JSON evidence containing revision/local-diff identity, toolchain, adapter/driver, source/fixture and executable hashes, actual outcomes and coverage limits. GPU output includes selected-device evidence and diagnostic microbenchmark timings.
- Restored `.github/workflows/graphics-contracts.yml` and removed `.github` from ignore rules. Its native and full baseline jobs cover Debug and RelWithDebInfo. The hosted jobs explicitly use Visual Studio 2022, matching the [Windows 2022 runner inventory](https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md); local validation used Visual Studio 2026. Remote execution remains unverified.
- Verified that missing required registrations fail preflight and `-RequireExternalAssets` fails for the unavailable mesh fixtures. Default development validation reports that gap explicitly; it does not certify a release.

Local evidence is in the ignored `cmake-build-debug-vs2026-readiness` build directory: `baseline-configure.log`, `baseline-Debug-build.log`, `baseline-RelWithDebInfo-build.log`, and `claw-graphics-<configuration>-results.xml` / `claw-graphics-<configuration>-evidence.json`. Strict external-asset results are retained separately as `strict-external-assets-results.xml` / `strict-external-assets-evidence.json`. Generated mesh serialization, native deformation/particles, GPU PBR/shadows/deformation/particles, UI/text/camera, SQLite CRUD/migration and ResourceSystem compilation contracts pass. Imported animation/effects, catalog-to-GPU integration, packaged consumers, remote CI and representative performance/recovery/soak evidence remain open.

At this verification revision, the next implementation increment was canonical catalog paths/entry kinds and lifecycle policy, then UUID/source-to-ResourceID mapping and a cooked material/texture reaching DX11. The current increment above advances the identity/adapter portion. Batch A's historical local verification deliverable is complete; external-media release coverage and remote CI confirmation remain outstanding.

## Planning review before the rebuild — 8 October 2026

Source reviewed at `d2e81cdb4`. The catalog repairs/migration, explicit CPU skinning and basic DX11 particles remain present. Recent source also adds long-text UI/serialization, visibility, editor-camera-during-Play and sample capture/winding improvements.

A current `project_x64` Debug CTest attempt selected 14 graphics/catalog/resource targets: **14 unavailable, 0 executed**, because their expected executables were missing. This is a build-artifact/verification gap, not evidence of source assertion failures. No rebuild, performance measurement or GPU capture was performed for this planning request. `.github` and its previously documented workflows are absent from this checkout; restore/version CI before marking that work complete.

The revised [production plan](WPGRAPHICS_PRODUCTION_PLAN.md) starts with a rebuilt traceable baseline, then canonical catalog identity and a cooked material/texture reaching DX11, followed by imported animation and particle scene integration. The implementation and test results below are historical evidence, with their original configurations; neither release gate is certified.

## Historical implementation evidence

Catalog increment validated on 6 October 2026, Windows x64, Visual Studio 2026/MSVC 19.51, CMake 4.4.4, RelWithDebInfo, starting from `d5c859612` on `brodex`.

Local evidence at that revision: the required SQLite catalog target and its Workphone/WPSQLite dependencies were built, and the catalog contracts passed without graphics or external media. The two isolated skinning/particle reference tests also passed. The catalog preset was configured with the existing verification build directory overridden to `cmake-build-debug-vs2026-readiness`.

Historical graphics evidence at `8a767dda5`: existing graphics test binaries reported **11 passed, 1 skipped**. The isolated tests duplicate two contracts in that full suite. GPU tests were not rebuilt/rerun for this catalog increment; performance, remote CI, complete engine/editor builds and clean-machine packaging remain unverified.

The production roadmap is only partially implemented. This stage supplies executable contracts and initial animation deformation and particle rendering paths; it does not satisfy the R1 or R2 release gates.

## Implemented in this stage

- Dependency-free C90 contract build for skinning and particle simulation; CI definitions are restored by the current verification increment, with remote execution still unverified.
- CPU reference skinning with four influences, up to 256 joints, weight normalization, validated affine transforms, and transactional rejection of invalid input. Supported transforms are rigid or uniformly scaled; nonuniform scale and shear are rejected.
- Explicit ClawMesh skinning data and palette APIs that update native positions and normals, invalidate cached GPU geometry, and recompute bounds. The caller supplies the palette on the owning rendering thread.
- Seeded, bounded CPU particle simulation at 120 Hz, with gravity, lifetime, size ranges, color interpolation, duration, pause, stop, drain, and bounded prewarm. No allocation occurs per simulation step.
- Claw particle lifecycle ownership, synchronized snapshots, diagnostics for unsupported templates, and dropped-particle counts.
- DX11 camera-facing particle batches with alpha/additive blending, depth testing without depth writes, and sorting within each effect. Scene preparation advances simulation independently of individual view draws.
- Backend capability inventory that retains experimental/unavailable status and explicitly reports `productionCertified = false`.
- Generated mesh serialization coverage independent of optional external media, plus a strict graphics validation runner.
- GPU readback regression proving particle pixels and translated CPU-skinned mesh pixels, alongside particle lifecycle checks.
- Required SQLite catalog coverage for existing bound CRUD, durable UUIDs, shared-scene deletion, failed rename rollback, detached lookups, concurrent reads and database switching.
- Catalog schema version 1 in its own metadata table, preserving unrelated `user_version` values. Valid unversioned catalogs retain their original rows/IDs and a one-time `wp_asset_catalog_backup_v0` row copy inside the same database. This is a migration snapshot, not an independent database-file backup.
- Schema guards reject invalid/oversized/NUL-containing values and enforce UUID/file-path uniqueness while allowing shared scene paths. Conflicting legacy data, reserved-name collisions and unsupported schema versions fail closed; the migration transaction rolls back schema and data changes.
- Catalog migration regressions for Unicode/apostrophe paths, a misleading nonunique UUID index, retained backup/reopen behavior, duplicate identities/paths, invalid legacy keys, embedded NULs, early/late migration failure and newer-version preservation. The dedicated catalog preset and restored full-baseline CI definition require the backend.
- Windows build repair: the media-path definition is scoped to its native-runtime consumer so paths with spaces do not break Boost.Context's MASM compilation.

The GPU fixture calls deformation and particle drawing directly. It does not yet establish imported/controller-driven animation, automatic scene scheduling, multi-camera correctness or catalog-to-cook-to-render behavior. Particle scene preparation is implemented as the intended update owner; once-per-frame behavior remains to be validated across all callers.

## Run validation

From the repository root with PowerShell 7, build and validate both configurations:

```powershell
pwsh -File Tools/BuildWPGraphicsBaseline.ps1
```

Use `-Configuration Debug` or `-Configuration RelWithDebInfo` for one configuration, `-BuildDirectory` for a different build tree, and `-CMakeExecutable` / `-Generator` for an explicit toolchain. The default configure preset uses Visual Studio 2026. Revalidate an existing baseline with `pwsh -File Tools/ValidateClawGraphics.ps1 -BuildDirectory project_wpgraphics_baseline -Configuration Debug`. Reports include catalog and ResourceSystem contracts as mandatory coverage.

The narrower catalog/native presets remain available:

```powershell
$baselineCMake = Join-Path $env:ProgramFiles 'CMake/bin/cmake.exe'
$baselineCTest = Join-Path $env:ProgramFiles 'CMake/bin/ctest.exe'
& $baselineCMake --preset asset-catalog
& $baselineCMake --build --preset asset-catalog
& $baselineCTest --preset asset-catalog

& $baselineCMake --preset claw-contracts
& $baselineCMake --build --preset claw-contracts
& $baselineCTest --preset claw-contracts
```

Use `-RequireExternalAssets` when validating a release environment with the media fixtures installed; missing media blocks that gate. The prior verification baseline was configured/built through `BuildWPGraphicsBaseline.ps1` with `-BuildDirectory cmake-build-debug-vs2026-readiness`, then validated in both configurations. This is a selected-target local rebuild, not a clean-machine packaging check. Current Debug and RelWithDebInfo validation passes.

Historical catalog result: **1 CTest target passed**, exercising the CRUD and migration contracts above. Historical isolated native reference result: **2 passed**. Historical full graphics result: **11 passed, 1 skipped**. The subsequent verification baseline reported **13 passed, 1 unavailable per configuration**. Current results are recorded at the top of this document. Remote CI remains unverified; its definitions are present.

## Remaining release work

AssetDatabaseManager is included in the [production plan](WPGRAPHICS_PRODUCTION_PLAN.md), under M1A and DB-01 through DB-10. Schema-v2 canonical file identity, explicit file/scene kinds, catalog-instance/generation snapshots and a ResourceSystem adapter are implemented and validated in Debug and RelWithDebInfo. Legacy conflicts still require explicit repair; no duplicate identities are silently discarded. Remaining work includes subassets, complete reimport/move reference handling, broader mutation/unload and cross-process races, durability/busy/cancellation policy, independent recovery, non-ASCII case/general short-name alias policy, and read-only runtime manifests. Captured Windows 8.3 root aliases are covered. Path resolution is an identity policy rather than a filesystem security boundary. No director cache is populated; bounded cache policy is required if caching returns. GD remains unsatisfied until catalog-resolved cooked assets reach a DX11 draw and failure preserves the last visible asset through coordinated catalog, compilation and GPU publication.

Animation remains experimental: asset import, skeleton/clip ownership, animation controllers and graphs, per-instance poses, automatic scene integration, GPU skinning, morph targets, IK, root motion, animated culling, and animation editor workflows are not completed.

Particles remain experimental: template/resource loading, emitter shapes, multiple emitters, curves, bursts, collisions, trails, flipbooks, globally sorted transparency, soft particles, lighting, robust editor authoring, scalability policies, and performance budgets remain. Timed pause is explicitly unsupported. Effects currently use a single point emitter.

Water rendering is unavailable. The water asset/component, mesh generation, shaders, reflection/refraction passes, depth/shoreline effects, editor integration, and validation scenes from the roadmap remain to be implemented.

The roadmap's GPU pass graph, HDR presentation, shadows and lighting completion, device recovery, runtime backend transitions, resource stress testing, performance certification, and release packaging also remain open. Next: deliver catalog-to-compiled-material/texture rendering with coordinated publication, then imported animation and particle scene integration. See [the revised plan](WPGRAPHICS_PRODUCTION_PLAN.md) for dependencies and acceptance gates. Do not treat passing this stage's tests as a production certification.
