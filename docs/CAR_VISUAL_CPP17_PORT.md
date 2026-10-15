# CarVisual C++17 integration

The user-supplied `Claude-for-speed-main/src/vehicle/CarVisual.js` has been ported into the native C++17 vehicle path. It assembles imported body and wheel meshes; it does not procedurally create the car body. The procedural car and aircraft presets remain available.

## Editor setup

On a ProceduralVehicle component:

1. Select a car **Physics Preset**, normally **Road Sport** or **GT**.
2. Set **Car Chassis Model** to the body GLB and **Car Wheel Model** to its separate wheel GLB. Leave the chassis path empty to use the procedural generator.
3. Set **Primary Colour** for body paint. Optionally set **Car Livery Texture**.
4. Adjust **Car Model Faces +Z** and **Car Model Yaw Correction** (radians) if an asset uses different forward axes.
5. **Car Player Lights** enables the two native headlight spotlights. **Car Police Lights** enables the roof strobes. **Car Rival Shadows** controls shadow casting for non-player models.

For example, the supplied project contains `public/cars/bmw_m4.glb` and `public/cars/bmw_m4_wheel.glb`. Use paths in your local asset folder; the original JavaScript project is not required at runtime. Assets remain external and are not copied into this repository.

Compressed GLBs are decoded by native C++ into `.FBCache/CarVisual`. The cache uses source content identity and a converter version. The input is never overwritten. The decoder supports EXT_meshopt_compression attribute/index streams and octahedral, quaternion, and exponential filters; quantized primitive attributes become floats for the importer. Scene transforms are retained. Embedded PNG/JPEG/WebP bytes are exposed as image sidecars beside the converted GLB because the native Assimp material path skips embedded texture references. Source WebP images still require the active texture codec to support WebP.

## Native API and ownership

- `Workphone/Vehicle/CarVisual.hpp` and `CarVisual.cpp`: renderer-independent C++17 presentation core, bounds/normalization, paint classification, projected livery UV1, wheel steering/spin/mirroring, brake lights, beam/glow textures, nitrous/backfire, police strobes, and visibility.
- `Workphone/Vehicle/CarVisualScene.hpp` and `CarVisualScene.cpp`: native mesh/material/texture resources, transient child actors, two player spotlights, and cleanup. Imported mesh and material prototypes are never modified.
- `Workphone/Vehicle/CarVisualAssets.hpp` and `CarVisualAssets.cpp`: native GLB decoder/cache, using meshoptimizer v0.22 and the existing nlohmann JSON header.

`ProceduralVehicle::stepFixed` queues wheel/body/brake presentation from the existing simulation systems. `ProceduralVehicle::update` applies it on the application task. The owning actor keeps its world/physics transform; the adapter applies local heave, pitch, roll, wheel suspension, steering and spin. Clients can call `syncCarVisual(frame, seconds)` to drive presentation directly, including nitrous and backfire. The standalone `CarVisual::sync` and `CarVisualScene::sync` also accept an optional camera for flame/strobe billboarding.

Original UV0 is retained and a separate UV1 is generated. The native material adapter requests UV set 1 for livery. Actual multi-UV material selection, imported normal/roughness maps, spotlights, transparent/additive effects and shadow behaviour require verification in the active render backend. Headless contracts and offline mesh renders do not verify those GPU effects.

The adapter marks generated actors DontSave and owns their actors, cloned materials, generated mesh resources, lights, and effect textures. Rebuilding stages a replacement before releasing the previous visual. Missing/invalid models produce a generation error rather than silently substituting an unrelated car. Imported car visuals are rejected for helicopter and warbird physics presets.

## Validation

Standalone C++17 contracts:

```powershell
cmake -S Tests/cpp/CarVisual -B project_x64/carvisual-tests
cmake --build project_x64/carvisual-tests --config Debug
ctest --test-dir project_x64/carvisual-tests -C Debug --output-on-failure
```

Contracts cover helper geometry/outlier bounds, immutable prototypes, wheel centring/scale and front/rear placement, paint/glass classification, UV0 preservation, livery tinting, blur/brake/nitrous/strobe state, shared effect textures, malformed inputs, staged rebuilds, compressed GLB preparation, and source/cache preservation. An Editor property regression was added for persistence of imported-model settings; it has not been executed in the Editor suite.

The native Debug Workphone target and the standalone and root-integrated CarVisual test targets build successfully. Eight supplied chassis/wheel pairs (BMW M4, Lamborghini Cen/Hu, Ferrari Spy, Corvette, Supra, Police Ford and Police Mustang) passed native GLB decoding and C++17 presentation assembly using CPU mesh fixtures. All sixteen converted files also imported successfully through the existing WPAssimp DLL's Assimp C API. These checks do not exercise the complete Workphone scene adapter, GPU materials, or interactive Editor behaviour; the optional geometry preview is also an offline check.

## Provenance

- User-provided source: `G:/Claude-for-speed-main/Claude-for-speed-main/src/vehicle/CarVisual.js`.
- Source SHA-256: `f17ae9a652ea98e3035b9db0733d1ad496c4d7a424c07ffc70d493e31a4ce178`.
- No LICENSE/COPYING file was present in that local source tree during inspection.
- Mesh decoder: [meshoptimizer v0.22](https://github.com/zeux/meshoptimizer/tree/v0.22), MIT. Its upstream codec sources and licence are retained in `Dependencies/meshoptimizer`.
- JavaScript's conflicting front-axis comments are resolved consistently to Workphone's **-Z forward** for chassis, steered axle, headlights, tail lights and exhaust. Flame variation is seeded by time and vehicle seed for reproducible native presentation.
