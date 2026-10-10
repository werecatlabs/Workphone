# Analytic animation fixture

`analytic_channels.gltf` is original test data created for Workphone and dedicated to the public domain under CC0-1.0. It contains one unskinned triangle and two animated hierarchy nodes. It does not certify mesh skin import.

All times below are seconds; all transforms are absolute local, right-handed TRS.

- Root translation keys at 0 and 2 seconds: `(2,3,4)` and `(6,3,4)`.
- Root Y rotation at 0, 1, 2 seconds: 0, 90, 180 degrees. The rotation key at 1 requires interpolation of the independent translation channel to `(4,3,4)`.
- Child has bind translation `(0,2,0)` and unit scale. It has only rotation animation: 0 to 90 degrees around Z over two seconds. Missing translation/scale must retain the node bind values.
- At 0.5 seconds Root is `(3,3,4)`, rotation 45 degrees around Y; Child rotation is 22.5 degrees around Z.

The current legacy import accepts linear TRS/shortest spherical rotations and constant boundaries. STEP/CUBICSPLINE, duplicate/non-finite/negative/collapsed key times, ambiguous node names and discontinuous default-pose boundaries are rejected with diagnostics. Zero tick rate uses the existing 25 ticks/second fallback. Typed cooking, skinned glTF/FBX support and runtime renderer publication remain separate gates.
