#pragma once
#include <Workphone/Vehicle/VehicleAudio.h>
#include <Workphone/Vehicle/VehicleVisualEffects.h>

#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/Interface/Procedural/OpenCityLayout.hpp>
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <array>
#include <mutex>
#include <vector>

namespace workphone::scene::race
{
    /**
     * A single sample point along a race circuit.
     *
     * position: world-space position of the sample.
     * right: world-space right direction at the sample (used for vehicle orientation).
     * distance: accumulated distance along the circuit up to this sample.
     */
    struct CircuitSample
    {
        Vector3F position, right;
        float distance = 0;
    };

    /**
     * Represents a procedural race circuit composed of ordered samples.
     *
     * The samples vector contains the track samples in order; length is the total
     * path length computed from the samples. nearest() returns the index of the
     * sample closest to a given world-space position.
     */
    struct WPCore_API Circuit
    {
        std::vector<CircuitSample> samples;
        float length = 0;

        /**
         * Find the index of the nearest circuit sample to the provided position.
         *
         * @param position World-space position to query.
         * @return Index into the samples vector of the nearest sample.
         */
        size_t nearest( const Vector3F &position ) const;
    };

    /**
     * Collection of assets that make up a generated race scene.
     *
     * This structure holds the generated vehicle data, the track circuit and all
     * render / physics actors, materials, textures and supporting scene resources
     * required for rendering and simulation. It is intended to be populated by
     * buildScene() and consumed by the scene component and physics configuration
     * code.
     */
    struct SceneAssets
    {
        bool openCity = false;
        int cityBlocks = 8, cityRoute = 0;
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
        SmartPtr<LODGroup> vehicleLOD;
        SmartPtr<render::ISky> sky;
        SmartPtr<render::IGraphicsLight> sun;
        String resourcePrefix;
        u64 triangles = 0, textureBytes = 0;
    };

    /**
     * Generate a deterministic circuit using the provided seed.
     *
     * @param seed Random seed for procedural generation.
     * @return Generated Circuit instance containing samples and length.
     */
    WPCore_API Circuit generateCircuit( u32 seed );

    /**
     * Run any internal validation checks for circuit generation. Intended for
     * debug/assertions; does not return a value.
     */
    WPCore_API void validateCircuit();

    /**
     * Validate that reflection rendering resources are correctly configured for
     * the provided scene assets (e.g. textures and actors present).
     *
     * @param assets Scene assets to validate.
     * @return true if reflection resources are valid and usable.
     */
    WPCore_API bool validateReflection( const SceneAssets &assets );

    /**
     * Build scene assets for a procedural race scene and attach them to the
     * provided vehicle actor. This creates renderable actors, materials,
     * textures and computes derived statistics such as triangle and texture
     * memory usage.
     *
     * @param assets Output container for created scene assets.
     * @param vehicle Vehicle actor to attach generated parts to.
     * @param seed Seed used for deterministic generation.
     * @param quality Visual quality level for generated vehicle appearance.
     */
    WPCore_API void buildScene( SceneAssets &assets, SmartPtr<IGameActor> vehicle, u32 seed,
                                procedural::VehicleAppearanceQuality quality );

    /**
     * Configure physics actors, colliders and joints for the generated scene
     * and associated vehicle. This function should be called after buildScene()
     * and before stepping physics simulation.
     *
     * @param assets Scene assets containing actors and meshes.
     * @param vehicle Vehicle actor to configure physics for.
     */
    WPCore_API void configurePhysics( const SceneAssets &assets, SmartPtr<IGameActor> vehicle );
}  // namespace workphone::scene::race

namespace workphone::scene
{
    class CarController;

    /**
     * ProceduralRaceScene
     *
     * Component that manages a seeded, fully procedural race scene containing a
     * generated circuit, scenery and an articulated Grand Prix vehicle. Attach
     * this component to the main vehicle actor. Input commands are provided by
     * the game thread while physics-related updates (forces, resets, suspension)
     * are executed on the physics task.
     */
    class WPCore_API ProceduralRaceScene : public Component
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

        /** Apply properties loaded from the editor or serialized state. */
        void setProperties( SmartPtr<Properties> properties ) override;

        /** Regenerate the procedural scene and vehicle using the current seed. */
        bool regenerate();

        /** Remove all generated actors and free associated resources. */
        void clearGeneratedScene();

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

        /** Get/Set the seed used for deterministic generation. */
        u32 getSeed() const;
        void setSeed( u32 seed );

        /** Get/Set visual quality for generated vehicle appearance. */
        s32 getQuality() const;
        void setQuality( s32 quality );

        /** Query whether a scene has been generated. */
        bool isGenerated() const;

        /** Query whether physics has been configured for the generated scene. */
        bool isPhysicsConfigured() const;

        /** If generation failed, return a human-readable error message. */
        String getGenerationError() const;

        /** Access the generated scene assets (read-only). */
        const race::SceneAssets &getAssets() const;

        /** Circuit query helpers. */
        u32 getCircuitSampleCount() const;
        f32 getCircuitLength() const;
        u32 nearestCircuitSample( const Vector3F &position ) const;
        Vector3F getCircuitPosition( u32 index ) const;
        Vector3F getCircuitRight( u32 index ) const;

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
        bool isRoadSurface( const Vector3F& position ) const;
        advanced::VehicleAudioInput sampleVehicleAudio() const;
        advanced::VehicleEffectsFrame sampleVehicleEffects() const;
        advanced::VehicleAudio& getVehicleAudio();
        advanced::VehicleVisualEffects& getVehicleVisualEffects();
        WP_CLASS_REGISTER_DECL;

    private:
     advanced::VehicleAudio m_vehicleAudio;
     advanced::VehicleVisualEffects m_vehicleVisualEffects;
     mutable std::recursive_mutex m_presentationMutex;
     atomic_bool m_audioEnabled{ true }, m_effectsEnabled{ true }, m_automaticPresentation{ true };
     atomic_bool m_presentationResetRequested{ false };
     bool m_presentationInitialized = false, m_appliedAudioEnabled = false,
          m_appliedEffectsEnabled = false;
     float m_presentationAudioClock = 0;
     /** Apply aerodynamic forces to the vehicle based on current velocity. */
     void applyAerodynamics();

     /** Update surface grip coefficient from current scene / physics state. */
     void updateSurfaceGrip();

     /** Generated scene assets owned by this component. */
     race::SceneAssets m_assets;

     /** Deterministic seed used for procedural generation (default = 7). */
     u32 m_seed = 7;

     /** Visual quality level used when building the vehicle appearance. */
     procedural::VehicleAppearanceQuality m_quality = procedural::VehicleAppearanceQuality::High;

     /** Generated city layout used for street surface queries. */
     procedural::OpenCityLayout m_cityLayout{};

     /** Select open-city generation instead of the standalone circuit. */
     bool m_openCity = false;

     /** City size and selected street route used for generation. */
     s32 m_cityBlocks = 8, m_cityRoute = 0;

     /** Last generation error message, empty when generation succeeded. */
     String m_generationError;

     /** Per-scene surface grip multiplier applied to vehicle physics. */
     f32 m_surfaceGrip = 1;

     /** True when physics has been configured for the generated scene. */
     bool m_physicsConfigured = false;

     /** Flag used to request a reset safely across threads/tasks. */
     atomic_bool m_resetRequested{ false };
    };
}  // namespace workphone::scene
