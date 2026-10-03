#pragma once
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <vector>
#include <Workphone/Scene/Components/LODGroup.hpp>

namespace workphone::advanced
{
    struct CircuitSample
    {
        Vector3F position, right;
        float distance = 0;
    };
    struct Circuit
    {
        std::vector<CircuitSample> samples;
        float length = 0;
        size_t nearest( const Vector3F &position ) const;
    };
    struct SceneAssets
    {
        procedural::GeneratedVehicle vehicle;
        Circuit circuit;
        std::array<SmartPtr<scene::IGameActor>, 4> wheels;
        SmartPtr<scene::IGameActor> body, shadow;
        std::vector<SmartPtr<render::ITexture>> textures;
        std::vector<SmartPtr<render::IMaterial>> materials;
        std::vector<SmartPtr<IMeshResource>> meshes;
        Array<SmartPtr<scene::LODGroup>> treeLODs;
        u64 triangles = 0;
        u64 textureBytes = 0;
    };
    Circuit generateCircuit( u32 seed );
    void validateCircuit();
    void buildScene( SceneAssets &assets, SmartPtr<scene::IGameActor> vehicle, u32 seed,
                     procedural::VehicleAppearanceQuality quality );
    void configurePhysics( const SceneAssets &assets, SmartPtr<scene::IGameActor> actor );
}  // namespace workphone::advanced
