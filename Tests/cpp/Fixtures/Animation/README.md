# Analytic animation fixture

`analytic_channels.gltf` is original test data created for Workphone and dedicated to the public domain under CC0-1.0. It contains one unskinned triangle and two animated hierarchy nodes. It does not certify mesh skin import.

All times below are seconds; all transforms are absolute local, right-handed TRS.

- Root translation keys at 0 and 2 seconds: `(2,3,4)` and `(6,3,4)`.
- Root Y rotation at 0, 1, 2 seconds: 0, 90, 180 degrees. The rotation key at 1 requires interpolation of the independent translation channel to `(4,3,4)`.
- Child has bind translation `(0,2,0)` and unit scale. It has only rotation animation: 0 to 90 degrees around Z over two seconds. Missing translation/scale must retain the node bind values.
- At 0.5 seconds Root is `(3,3,4)`, rotation 45 degrees around Y; Child rotation is 22.5 degrees around Z.

The current legacy import accepts linear TRS/shortest spherical rotations and constant boundaries. STEP/CUBICSPLINE, duplicate/non-finite/negative/collapsed key times, ambiguous node names and discontinuous default-pose boundaries are rejected with diagnostics. Zero tick rate uses the existing 25 ticks/second fallback. Typed cooking, skinned glTF/FBX support and runtime renderer publication remain separate gates.

## Skinned character fixture

`analytic_skin.gltf` is original Workphone test data dedicated to the public domain under CC0-1.0. It contains an indexed quad and one two-joint skin in an embedded buffer. Root's bind transform is the identity. Child's local bind translation is `(0,1,0)`, so its inverse bind translates by `(0,-1,0)`. The mesh node has an identity transform.

The four XY vertices are `(0,0)`, `(2,0)`, `(0,2)` and `(2,2)`, with Z zero. Their Root weights are `1`, `0`, `0.25` and `0.6`. Child contributes the remaining weight. The two-second `Lift` clip translates Root from X zero to two and rotates Child from zero to 90 degrees around Z. At the endpoint, weighted vertices are `(2,0)`, `(3,3)`, `(1.25,1.25)` and `(2.8,2.4)`.

The import contract cooks the mesh, rig with its clip and skin descriptor, mounts the package after removing the authoring copies, and compares both interpolated and endpoint positions on separate native meshes. It also checks that a mismatched inverse bind leaves the preceding import output intact.
