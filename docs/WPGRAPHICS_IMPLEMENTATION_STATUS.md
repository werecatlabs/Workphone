# WPGraphics implementation status

Validated on 6 October 2026, Windows x64, MSVC 19.51, RelWithDebInfo.

The production roadmap is only partially implemented. This stage supplies executable contracts and initial animation deformation and particle rendering paths; it does not satisfy the R1 or R2 release gates.

## Implemented in this stage

- Dependency-free C90 contract build and CI workflow for skinning and particle simulation.
- CPU reference skinning with four influences, up to 256 joints, weight normalization, validated affine transforms, and transactional rejection of invalid input. Supported transforms are rigid or uniformly scaled; nonuniform scale and shear are rejected.
- Explicit ClawMesh skinning data and palette APIs that update native positions and normals, invalidate cached GPU geometry, and recompute bounds. The caller supplies the palette on the owning rendering thread.
- Seeded, bounded CPU particle simulation at 120 Hz, with gravity, lifetime, size ranges, color interpolation, duration, pause, stop, drain, and bounded prewarm. No allocation occurs per simulation step.
- Claw particle lifecycle ownership, synchronized snapshots, diagnostics for unsupported templates, and dropped-particle counts.
- DX11 camera-facing particle batches with alpha/additive blending, depth testing without depth writes, and sorting within each effect. Scene preparation advances simulation independently of individual view draws.
- Backend capability inventory that retains experimental/unavailable status and explicitly reports `productionCertified = false`.
- Generated mesh serialization coverage independent of optional external media, plus a strict graphics validation runner.
- GPU readback regression proving particle pixels and translated CPU-skinned mesh pixels, alongside particle lifecycle checks.

## Run validation

From the repository root in PowerShell:

```powershell
cmake --preset claw-contracts
cmake --build --preset claw-contracts
ctest --preset claw-contracts

cmake --preset claw-windows
cmake --build --preset claw-windows
Tools/ValidateClawGraphics.ps1 -BuildDirectory project_claw_windows
```

Use `-RequireExternalAssets` when validating a release environment with the media fixtures installed. The isolated contract build was built and tested; the fresh full-engine preset was configured. Full-engine compilation and GPU validation were performed in the existing `project_x64` build tree.

Latest full graphics result: **11 passed, 1 skipped**. The skipped external mesh import test requires missing `Bin/Media/Ogre/models` assets. The two isolated native contract tests also passed. CI configuration has been added but has not been verified by a remote run.

## Remaining release work

Animation remains experimental: asset import, skeleton/clip ownership, animation controllers and graphs, per-instance poses, automatic scene integration, GPU skinning, morph targets, IK, root motion, animated culling, and animation editor workflows are not completed.

Particles remain experimental: template/resource loading, emitter shapes, multiple emitters, curves, bursts, collisions, trails, flipbooks, globally sorted transparency, soft particles, lighting, robust editor authoring, scalability policies, and performance budgets remain. Timed pause is explicitly unsupported. Effects currently use a single point emitter.

Water rendering is unavailable. The water asset/component, mesh generation, shaders, reflection/refraction passes, depth/shoreline effects, editor integration, and validation scenes from the roadmap remain to be implemented.

The roadmap's GPU pass graph, HDR presentation, shadows and lighting completion, device recovery, runtime backend transitions, resource stress testing, performance certification, and release packaging also remain open. See `WPGRAPHICS_PRODUCTION_PLAN.md` for dependencies and acceptance gates. Do not treat passing this stage's tests as a production certification.
