#pragma once

#include <Workphone/Scene/Components/ProceduralScene.hpp>
#include <Workphone/Vehicle/VehicleAudio.h>
#include <Workphone/Vehicle/VehicleVisualEffects.h>
#include <memory>

namespace workphone::scene
{

    class CarController;
    class ProceduralCityConfiguration;

    /**
     * ProceduralRaceScene
     *
     * Component that manages a seeded, fully procedural race scene containing a
     * generated circuit, scenery and an articulated Grand Prix vehicle. Attach
     * this component to the main vehicle actor. Input commands are provided by
     * the game thread while physics-related updates (forces, resets, suspension)
     * are executed on the physics task.
     */
    class WPCore_API ProceduralRaceScene : public ProceduralScene
    {
    public:
        /** Construct an empty ProceduralRaceScene. */
        ProceduralRaceScene();

        /** Destructor; tears down generated assets if still present. */
        ~ProceduralRaceScene() override;

        /** Load component state from a shared data object. */
        void load( SmartPtr<ISharedObject> data ) override;

        /** Unload component state and release resources. */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** Per-frame update executed on the main/game thread. */
        void update() override;

        /** Post-update stage called after main update; used for deferred work. */
        void postUpdate() override;

        /** Retrieve editable component properties for inspector/editor. */
        SmartPtr<Properties> getProperties() const override;

        /** Configure physics for the generated scene and vehicle. */
        void configurePhysics();

        /** Set player input controls for the vehicle (throttle, brake, steering). */
        void setControls( f32 throttle, f32 brake, f32 steering );

        /** Switch to player-provided control input. */
        void usePlayerControls();

        /** Request a vehicle reset on the physics task. */
        void reset();

        /** Perform the actual reset operation (called on the appropriate task). */
        void performReset();

        /** Update wheel actor transforms/visuals to match physics. */
        void updateWheelVisuals();

        /** Update shadow actor parameters based on current scene and view. */
        void updateShadow();

        /**
         * Update view-dependent parameters for the generated scene such as LOD
         * bias, near clip and field-of-view for rendering.
         */
        void setView( const Vector3F &position, f32 fovRadians, f32 nearClip, f32 lodBias );

        /** Select a scene-owned native terrain ground for the next regeneration.
         * Null restores the existing circuit mode. The supplied immutable source
         * must contain the flat, hole-free driving corridor around the origin.
         */
        void setTerrainGameplaySource( render::TerrainSnapshot source );

        /// Let a host retain newly authored terrain and roads independently of its disposable car.
        void setCitySourceOwner( SmartPtr<IGameActor> owner );

        /** Query whether physics has been configured for the generated scene. */
        bool isPhysicsConfigured() const;
        bool isGenerated() const override;

        /** Vehicle geometry/parameters. */
        f32 getWheelbase() const;
        f32 getMaxSteeringAngle() const;

        /** Accessors for vehicle actors. */
        SmartPtr<IGameActor> getWheelActor( u32 index ) const;
        SmartPtr<IGameActor> getBodyActor() const;
        SmartPtr<CarController> getCarController() const;

        /** Validate reflection resources for the current generated assets. */
        bool validateReflection() const;

        // Shared presentation lifecycle for native and Lua hosts.
        void initializePresentation();

        void setAudioEnabled( bool enabled );
        bool getAudioEnabled() const;

        void setEffectsEnabled( bool enabled );
        bool getEffectsEnabled() const;

        void setAutomaticPresentation( bool enabled );

        bool isAudioAvailable() const;
        bool isEffectsAvailable() const;

        u32 getParticleCount() const;
        u32 getSkidDecalCount() const;

        bool isRoadSurface( const Vector3F &position ) const;

        advanced::VehicleAudioInput sampleVehicleAudio() const;
        advanced::VehicleEffectsFrame sampleVehicleEffects() const;
        advanced::VehicleAudio &getVehicleAudio();
        advanced::VehicleVisualEffects &getVehicleVisualEffects();

        WP_CLASS_REGISTER_DECL;

    private:
        void readGenerationProperties( Properties &properties ) override;
        bool prepareGeneration() override;
        void buildGeneratedScene() override;
        void generationFailed() override;
        void generationCleared() override;

        advanced::VehicleAudio m_vehicleAudio;
        advanced::VehicleVisualEffects m_vehicleVisualEffects;
        atomic_bool m_audioEnabled{ true }, m_effectsEnabled{ true }, m_automaticPresentation{ true };
        atomic_bool m_presentationResetRequested{ false };
        bool m_presentationInitialized = false, m_appliedAudioEnabled = false,
             m_appliedEffectsEnabled = false;
        float m_presentationAudioClock = 0;
        /** Apply aerodynamic forces to the vehicle based on current velocity. */
        void applyAerodynamics();

        /** Update surface grip coefficient from current scene / physics state. */
        void updateSurfaceGrip();

        std::unique_ptr<ProceduralCityConfiguration> m_city;
        WeakPtr<IGameActor> m_citySourceOwner;
        render::TerrainSnapshot m_terrainGameplaySource;

        /** Per-scene surface grip multiplier applied to vehicle physics. */
        f32 m_surfaceGrip = 1;

        /** True when physics has been configured for the generated scene. */
        bool m_physicsConfigured = false;

        /** Flag used to request a reset safely across threads/tasks. */
        atomic_bool m_resetRequested{ false };
    };
}  // namespace workphone::scene
