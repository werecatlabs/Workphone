# VehicleAdvanced implementation plan

## Intended result

Turn the copied sample into `SampleVehicleAdvanced`: a playable, procedurally generated open-wheel race car on a scenic closed asphalt circuit, rendered through WPGraphics. The scene should look deliberately composed: detailed bodywork and wheels, a generated livery, textured asphalt, contrasting kerbs, trackside barriers, grass, distant hills, a procedural sky, and coherent daylight.

Use a reproducible seed and a small configuration object for the car, circuit, scenery, and graphics quality. Generate the principal geometry and textures at startup using the existing procedural services. The finished sample should drive, steer, brake, reset, and complete a lap; it should also have an automated smoke test.

Recommended scope: a Grand Prix car and a mostly flat circuit with a long straight, a hairpin, and flowing corners. The current vehicle geometry generator supports this shape. GT and RoadSport presets exist in the public types, but their geometry is not currently implemented. Hills belong around the circuit initially; track elevation should follow only after contact and collision support are verified.

## What exists and what needs integration

The copied `CMakeLists.txt` still defines `SampleVehicle`, which conflicts with the original sample. The scene still uses a box chassis, simple round wheels, a large ground box, and straight road markings.

Reuse these existing systems:

- Public procedural services: `IVehicleGenerator`, `IRoadSystem`, `ITextureForge`, and `ISkyAtmosphere`, supplied by WPProcedural.
- Vehicle output: detailed material-labelled mesh sections, three LODs, generated appearance textures, and SI-unit physics configuration.
- Road output: independent surface, kerb, marking, dressing, and collision layers.
- Scene bridges: `ProceduralVehicle`, `ProceduralRoad`, and `ProceduralMeshComponent` already publish transient CPU meshes to the renderer.
- Existing Vehicle sample: plugin startup, input, WPVehiclePhysics integration, wheel transforms, follow camera, reset, and smoke-test structure.
- WPGraphics: transient mesh conversion, submesh materials, normal/roughness/metalness/AO maps, generated UV projections, sky rendering, and graphics pipeline settings.

Important gaps to address explicitly:

- The scene bridges use fallback material values; generated texture buffers still need upload and binding.
- Generated tyre/rim/brake sections currently combine wheels. Independent steering and rotation need stable wheel-part output.
- Road services build individual segments, not a complete smooth racing circuit. Joins, UV continuity, and collision continuity need a circuit builder.
- `ProceduralVehicle::stepFixed` exposes a separate dynamics model; it does not automatically move actors or instantiate a physics-backed car. Choose one simulation owner.
- Atmospheric parameters alone do not draw a sky. Graphics toggles alone do not prove that shadows or post-processing appear in the final frame.

## Implementation sequence

### 1. Make the advanced sample independent

- Rename the target, executable, application class, and copied guards to `SampleVehicleAdvanced`.
- Add an explicit WPProcedural dependency and a sample-specific plugin configuration using WPGraphics, WPPhysics, WPVehiclePhysics, and WPProcedural.
- Introduce `VehicleAdvancedConfig` with seed, quality, track dimensions, and car appearance settings.
- Keep the original sample working. Preserve its useful input, reset, camera, and smoke-test behavior while separating advanced scene construction.
- Update the README with the actual advanced build/run commands.

**Complete when:** both vehicle targets configure and build together, and the advanced executable starts with the required named procedural factories available.

### 2. Generate and display the car

- Generate a Grand Prix vehicle through the public `IVehicleGenerator` contract and validate its result before publishing resources.
- Convert mesh sections into renderer-neutral transient mesh resources, preserving normals, UVs, indices, and material identifiers. Reuse the existing scene bridge where practical.
- Extend geometry output to provide separate wheel assemblies with stable corner identifiers. Separate rotating tyre/rim parts from steering-only or fixed brake/suspension parts.
- Build a body actor plus four wheel hierarchies. Derive hubs, bounds, and dimensions from generated metadata rather than copied chassis offsets.
- Start with descriptor-based PBR materials so geometry and transforms can be checked before texture work.

**Complete when:** the car has recognisable bodywork, aero surfaces, cockpit, suspension, rims, and tyres; each wheel can steer and rotate independently without duplicated static wheels.

### 3. Connect generated geometry to vehicle physics

- Retain WPVehiclePhysics as the sole authoritative motion system for this sample, with the existing rigidbody/contact integration.
- Add a small adapter from `GeneratedVehicle.physics` into the supported chassis, drivetrain, tyre, and suspension properties. Identify missing setter support during this phase and add it with focused checks rather than silently substituting unrelated defaults.
- Use generated mass, centre of mass, wheel hubs, radius, track, and wheelbase consistently for physics and visuals. Explicitly convert units and axes where required.
- Drive visual wheel poses from simulated steering, spin, and suspension. Keep generated presentation pitch/roll cosmetic if used.
- Do not also advance `ProceduralVehicle::stepFixed` as another source of chassis movement.
- Reset chassis pose, velocity, controller state, wheels, and camera together.

**Complete when:** settling, acceleration, steering, braking, and reset pass on a simple test surface with correctly positioned wheels and stable chassis height.

### 4. Generate a continuous drivable circuit

- Create a closed centreline from a deterministic set of control points; smooth and resample it at approximately uniform arc length.
- Build a continuous road ribbon with consistent frames, normals, and distance-based UVs. Reuse road-service layers and texture generation; add the join/stitching adapter needed for curves and loop closure.
- Generate kerbs, runoff shoulders, lane/start markings, and barriers from the same centreline. Avoid urban sidewalks and crossings in the circuit preset.
- Construct static collision from the same surface positions as the visible road. Keep decorative dressing out of the driving collision unless intentional.
- Verify WPPhysics mesh collision and wheel queries first. If mesh contact support is incomplete, repair that path before introducing track elevation.
- Add asphalt, kerb, grass, and gravel surface classifications where the vehicle contact interface supports them.
- Derive spawn orientation, checkpoints, lap progress, and reset placement from the track data.

**Complete when:** the complete loop has no visible cracks, wheel-contact gaps, abrupt collision ridges, or start/finish seam; the car can drive a full lap and recover safely after leaving the road.

### 5. Upload procedural textures and finish materials

- Add a reusable generated-texture adapter using the public graphics interfaces, with a narrowly scoped WPGraphics bridge if in-memory upload is missing.
- Upload vehicle livery, carbon, rubber, metal detail, normal maps, and ORM buffers. Upload asphalt and other surface maps from `ITextureForge`.
- Honour albedo/livery sRGB encoding and linear normal/data maps without double gamma conversion.
- Explicitly map generated ORM channels: R = AO, G = roughness, B = metalness. Repack into existing material slots or add a tested channel-selection path; the current packed-mask choices must not be assumed to match this layout.
- Use mesh UVs for the livery, continuous UVs for asphalt, and world/object projection for suitable scenery. Set texture density in metres.
- Set appropriate paint, carbon, rubber, glass, metal, kerb, and asphalt properties. Start with supported PBR features; treat clearcoat/refraction as renderer work if the final frame does not actually implement them.
- Bound texture resolution and share material/texture resources across repeated objects.

**Complete when:** the generated maps visibly affect rendering, car graphics align with the body, asphalt density remains consistent through corners, and saved seed/quality settings reproduce the appearance.

### 6. Compose the environment and lighting

- Generate grass banks, distant hills, sparse trees, barriers, braking boards, a start gantry, and a modest pit area. Keep landmarks clear from the driving camera.
- Use a controlled palette: vivid car paint, dark asphalt, red/white kerbs, muted vegetation, and readable signage.
- Drive directional sunlight, ambient light, and fog from `ISkyAtmosphere`. Build a visible procedural sky mesh or texture from those results.
- Verify sunlight, grounded/contact shadows, exposure, antialiasing, and restrained bloom in captured WPGraphics frames. If an effect is disconnected, implement the required renderer connection before claiming it works.
- Introduce scenery density and LOD limits, shared meshes, and sensible draw distances.

**Complete when:** daylight and materials read coherently, the car feels grounded, the horizon is finished, and the scene remains visually clear at driving speed.

### 7. Finish the driving experience

- Tune a damped chase camera with speed-dependent look-ahead, sensible zoom, and track-aware clipping prevention.
- Add a compact HUD for speed, gear, lap/time, and reset controls; put detailed physics telemetry behind a debug toggle.
- Add braking lights and restrained skid/dust effects from supported simulation events. Add engine/audio feedback if the existing audio backend is available and the integration remains small.
- Expose `--seed`, `--quality`, and `--smoke-test`, plus deterministic screenshot capture positions for visual review.

**Complete when:** the sample feels like a coherent driving showcase and is understandable without reading its source.

### 8. Validate and document

- Build Debug and RelWithDebInfo with both the original and advanced samples present.
- Run existing vehicle/procedural service tests; add focused coverage for wheel-part transforms, circuit closure/index bounds, texture channel mapping, and generated collider alignment.
- Extend smoke testing to settling, acceleration, turning, braking, reset, and track contact across joints. Keep outcomes based on behavior, not only setter/getter round-trips.
- Capture car close-ups, the start straight, a flowing corner, and a wide circuit view. Review material seams, wheel alignment, sky, lighting, and camera framing.
- Measure generation time, peak memory, frame time, draw calls, and texture counts on the test machine. Set and document quality budgets from those measurements; do not promise a frame rate before profiling.
- Test repeated startup/shutdown and release generated GPU resources, meshes, actors, services, and plugins in dependency order.
- Document controls, seed/quality options, tested graphics/physics plugins, and any remaining limitations.

**Complete when:** the advanced sample launches reliably, passes behavioral checks, looks finished in the captured views, and releases its resources cleanly.

## Suggested sample structure

Keep `SampleVehicleAdvanced` responsible for application lifecycle and orchestration. Split construction and integration into small units such as `ProceduralVehicleBuilder`, `ProceduralTrackBuilder`, `ProceduralMaterialBuilder`, `VehiclePhysicsAdapter`, and `VehicleAdvancedEnvironment`. Keep generated resource ownership explicit. Use public Workphone procedural interfaces in the sample; put backend-specific fixes in their respective engine plugins.

## Delivery checkpoints

1. **Playable foundation:** independent target, generated car, animated wheels, working physics, simple closed circuit.
2. **Visual showcase:** uploaded procedural PBR maps, scenery, sky, verified lighting, composed camera views.
3. **Finished sample:** HUD, lap/reset behavior, quality settings, automated checks, visual captures, and documentation.

Complete the playable foundation before expanding graphics effects. The highest-risk integration work is wheel-part extraction, physics configuration, continuous track collision, and procedural texture upload.
