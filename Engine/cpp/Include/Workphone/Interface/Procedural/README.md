# Procedural services and editor components

Workphone owns the public `I*` services and their `*Types.hpp` value types. These headers do not include WPProcedural headers. WPProcedural supplies the default `C*` adapters and keeps its original `WP*` entry points, so existing source code continues to compile. Rebuild Workphone, WPProcedural, and client modules together because ownership of exported value-type operations has moved to Workphone.

Load the Workphone plugin and then WPProcedural as usual. WPProcedural registers the following named factories and removes them on unload:

- `IRoadSystem`, `ISkyAtmosphere`, `ITextureForge`
- `IVehicleAppearance`, `IVehicleDamage`, `IVehicleDynamics`, `IVehicleEffects`
- `IVehicleGenerator`, `IVehicleGeometry`, `IVehiclePhysics`, `IVehiclePresentation`

A client uses only the public contract:

```cpp
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>

auto generator = factories->createObjectFromType<workphone::procedural::IVehicleGenerator>(
    "IVehicleGenerator" );
if( generator )
{
    workphone::procedural::VehicleGenerationConfig config;
    auto vehicle = generator->generate( config );
    if( vehicle.isValid() )
    {
        // Consume renderer-neutral meshes, appearance maps, and SI physics data.
    }
}
```

Replace a named factory to supply another backend. Authoring components also provide explicit service setters for dependency injection. Service instances should remain alive while clients use them; the plugin binary must remain loaded until its instances have been released. Mutable seed/state setters are intended to be used before generation, not concurrently with calls on the same instance.

## Using the editor

Select an actor and use the normal Add Component dialog. Workphone registers four components:

- **ProceduralVehicle** exposes seed, physics preset, appearance quality, texture resolution, number, primary colour, wear, wetness, dimensions, detail settings, and LOD. It creates or reuses a Mesh and MeshRenderer on the actor. Generated material descriptors supply fallback PBR materials when there are no user-authored Material components. The current geometry author supports Grand Prix open-wheel vehicles; GT and Road Sport authoring requests produce a Generation Error, as the existing generator does.
- **ProceduralRoad** exposes endpoints in actor-local space, road class, surface, seed, sidewalks, kerbs, markings, and dressing. Each render layer becomes a submesh; collision geometry remains available through `getResult()` for the client's collision integration.
- **ProceduralSky** exposes time of day, day of year, latitude, turbidity, intensity, and weather. It can apply scene ambient light and fog and create or reuse a directional Light. Disable Apply Lighting or Apply Fog to compute parameters without applying those settings. This component computes atmospheric parameters; it does not add a sky-dome renderer.
- **ProceduralSurfaceTexture** exposes surface, seed, resolution, tiling, parallax, detail scale, and albedo colour-space settings. It bakes albedo, normal, ORM, and height buffers, available through `getResult()`. GPU texture upload and material map binding are renderer integration responsibilities; normal/ORM must be uploaded as linear data.

Changing properties schedules regeneration on the application task. The **Regenerate** button explicitly repeats generation; **Generation Error** reports unavailable services or invalid configuration. Mesh generation is deferred until siblings have been deserialized, uses registered transient CPU mesh resources, and creates a fresh resource name on regeneration so render backends reload the mesh. Failed generation keeps the previous published mesh. Generated resources and owned components are removed on unload; reused Mesh components retain their prior resources.

Vehicle appearance's generated texture maps are exposed through `getGeneratedVehicle()`. The built-in scene bridge uses the descriptors' fallback colours, roughness, metalness, opacity, and emission; it does not upload those maps or every advanced PBR lobe.

The Claw renderer converts the transient CPU mesh into its native position/normal/UV format, preserves submesh ranges, and reloads geometry while retaining the render object binding. Native texture upload and additional material-map support remain separate renderer integration work.

## Runtime vehicle integration

`ProceduralVehicle::stepFixed(input, surfaces, seconds)` advances dynamics, damage, presentation, and effects together. Supply four world/contact samples and call it at a fixed cadence. The component stages the state update so a failed step does not partially commit simulation state. `registerImpact()` handles collision damage and the optional speed/yaw response. Read `getDynamicsState()`, `getPresentationState()`, and `getEffects()` to apply actor/wheel poses and spawn renderer/audio effects. These data APIs do not automatically move the actor or create particle systems.

## Validation

The focused test target covers every abstract interface and named factory, deterministic generation, mesh index bounds, physics/damage/presentation/effects service calls, named editor dropdown round-trips, resolution bounds, detached components, and plugin factory removal. Its client translation unit includes no WPProcedural implementation headers.

When Claw is enabled, it also checks native vertex/index conversion, submesh index rebasing, geometry reload, retained render-object binding, and rejection of invalid indices without requiring a graphics device. These checks do not replace an interactive editor/rendering smoke test.

```powershell
cmake -S . -B project_x64 -DWP_BUILD_PROCEDURAL_SERVICE_TESTS=ON
cmake --build project_x64 --target ProceduralServiceTests --config Release --parallel 4
# Run ProceduralServiceTests.exe from the configured runtime output directory.
```
