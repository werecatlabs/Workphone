#pragma once
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <array>
#include <atomic>
#include <vector>

namespace workphone::scene::race
{
    struct CircuitSample
    {
        Vector3F position, right;
        float distance = 0;
    };
    struct WPCore_API Circuit
    {
        std::vector<CircuitSample> samples;
        float length = 0;
        size_t nearest( const Vector3F &position ) const;
    };
    struct SceneAssets
    {
        procedural::GeneratedVehicle vehicle;
        Circuit circuit;
        std::array<SmartPtr<IGameActor>, 4> wheels;
        SmartPtr<IGameActor> body, shadow, reflectionActor, ground;
        SmartPtr<render::ITexture> reflectionTexture;
        std::vector<SmartPtr<render::IMaterial>> vehicleMaterials, materials;
        std::vector<SmartPtr<render::ITexture>> textures;
        std::vector<SmartPtr<IMeshResource>> meshes;
        std::vector<SmartPtr<IGameActor>> actors;
        Array<SmartPtr<LODGroup>> treeLODs;
        SmartPtr<render::ISky> sky;
        SmartPtr<render::IGraphicsLight> sun;
        String resourcePrefix;
        u64 triangles = 0, textureBytes = 0;
    };
    WPCore_API Circuit generateCircuit( u32 seed );
    WPCore_API void validateCircuit();
    WPCore_API bool validateReflection( const SceneAssets &assets );
    WPCore_API void buildScene( SceneAssets &assets, SmartPtr<IGameActor> vehicle, u32 seed,
                                procedural::VehicleAppearanceQuality quality );
    WPCore_API void configurePhysics( const SceneAssets &assets, SmartPtr<IGameActor> vehicle );
}  // namespace workphone::scene::race

namespace workphone::scene
{
    class CarController;

    /// Seeded circuit, scenery and articulated Grand Prix vehicle. Attach to the vehicle actor.
    /// Inputs are published by the game; forces, reset and suspension run on the physics task.
    class WPCore_API ProceduralRaceScene : public Component
    {
    public:
        ProceduralRaceScene();
        ~ProceduralRaceScene() override;
        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        void postUpdate() override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        bool regenerate();
        void clearGeneratedScene();
        void configurePhysics();
        void setControls( f32 throttle, f32 brake, f32 steering );
        void usePlayerControls();
        void reset();
        void performReset();
        void updateWheelVisuals();
        void updateShadow();
        void setView( const Vector3F &position, f32 fovRadians, f32 nearClip, f32 lodBias );

        u32 getSeed() const
        {
            return m_seed;
        }
        void setSeed( u32 seed )
        {
            m_seed = seed;
        }
        s32 getQuality() const
        {
            return static_cast<s32>( m_quality );
        }
        void setQuality( s32 quality );
        bool isGenerated() const
        {
            return !m_assets.circuit.samples.empty();
        }
        bool isPhysicsConfigured() const
        {
            return m_physicsConfigured;
        }
        String getGenerationError() const
        {
            return m_generationError;
        }
        const race::SceneAssets &getAssets() const
        {
            return m_assets;
        }
        u32 getCircuitSampleCount() const
        {
            return static_cast<u32>( m_assets.circuit.samples.size() );
        }
        f32 getCircuitLength() const
        {
            return m_assets.circuit.length;
        }
        u32 nearestCircuitSample( const Vector3F &position ) const;
        Vector3F getCircuitPosition( u32 index ) const;
        Vector3F getCircuitRight( u32 index ) const;
        f32 getWheelbase() const;
        f32 getMaxSteeringAngle() const;
        SmartPtr<IGameActor> getWheelActor( u32 index ) const;
        SmartPtr<IGameActor> getBodyActor() const
        {
            return m_assets.body;
        }
        SmartPtr<CarController> getCarController() const;
        bool validateReflection() const;
        WP_CLASS_REGISTER_DECL;

    private:
        void applyAerodynamics();
        void updateSurfaceGrip();
        race::SceneAssets m_assets;
        u32 m_seed = 7;
        procedural::VehicleAppearanceQuality m_quality = procedural::VehicleAppearanceQuality::High;
        String m_generationError;
        bool m_physicsConfigured = false;
        f32 m_surfaceGrip = 1;
        std::atomic<bool> m_resetRequested{ false };
    };
}  // namespace workphone::scene
