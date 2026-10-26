#pragma once
#include <Workphone/Scene/Components/ProceduralMesh.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/Interface/Procedural/IVehicleDynamics.hpp>
#include <Workphone/Interface/Procedural/IVehicleDamage.hpp>
#include <Workphone/Interface/Procedural/IVehiclePresentation.hpp>
#include <Workphone/Interface/Procedural/IVehicleEffects.hpp>

namespace workphone::scene
{

    /**
     * @brief Component used for authoring procedurally-generated vehicles in the editor.
     *
     * This component generates a runtime mesh and renderer on the same actor. It separates
     * generation (IVehicleGenerator) from simulation (IVehicleDynamics), damage handling
     * (IVehicleDamage), presentation (IVehiclePresentation), and effects (IVehicleEffects).
     *
     * Car simulation is advanced by the client via fixed-step updates using world surface
     * samples. The helicopter and warbird presets generate static models and mass metadata;
     * rotor/propeller animation and flight dynamics require a separate aircraft controller.
     */
    class WPCore_API ProceduralVehicle : public ProceduralMesh
    {
    public:
        /**
         * @brief Construct a new ProceduralVehicle component.
         */
        ProceduralVehicle();

        /**
         * @brief Destroy the ProceduralVehicle component and release generated resources.
         */
        ~ProceduralVehicle() override;

        /**
         * @brief Retrieve serializable properties for editor UI and persistence.
         *
         * @return SmartPtr<Properties> Pointer to the properties object (may be null).
         */
        SmartPtr<Properties> getProperties() const override;

        /**
         * @brief Apply properties loaded from the editor or a scene file.
         *
         * @param properties Properties to apply to this component.
         */
        void setProperties( SmartPtr<Properties> properties ) override;

        /**
         * @brief Unload runtime resources associated with this component.
         *
         * @param data Optional shared data used during unload operations.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Get the current vehicle generation configuration.
         *
         * The configuration controls parameters passed to the vehicle generator when
         * creating geometry, materials, and other assets.
         *
         * @return const procedural::VehicleGenerationConfig& Reference to the config.
         */
        const procedural::VehicleGenerationConfig &getConfig() const;

        /**
         * @brief Set the vehicle generation configuration.
         *
         * Changing the config typically requires re-generating the vehicle assets.
         *
         * @param config New generation configuration to use.
         */
        void setConfig( const procedural::VehicleGenerationConfig &config );

        /**
         * @brief Assign a custom vehicle generator implementation.
         *
         * The generator is responsible for producing mesh data and GeneratedVehicle
         * metadata. Passing null will clear the current generator.
         *
         * @param generator Generator implementation to use.
         */
        void setGenerator( SmartPtr<procedural::IVehicleGenerator> generator );

        /**
         * @brief Access the most recently generated vehicle data.
         *
         * This returns the GeneratedVehicle produced by the active generator. The
         * returned reference is valid while the component holds the generated data.
         *
         * @return const procedural::GeneratedVehicle& Generated vehicle metadata.
         */
        const procedural::GeneratedVehicle &getGeneratedVehicle() const;

        /**
         * @brief Advance the vehicle simulation by a fixed time step.
         *
         * The caller provides control inputs and up to four surface samples (for
         * wheel contacts or terrain probing). The component updates internal
         * dynamics, presentation and effect states.
         *
         * @param input Control inputs (steering, throttle, brake, etc.).
         * @param surfaces Array of up to 4 surface samples describing contact geometry.
         * @param deltaSeconds Fixed timestep to advance, in seconds.
         * @return true if the simulation produced visible changes that require a render/update.
         */
        bool stepFixed( const procedural::VehicleControlInput &input,
                        const std::array<procedural::VehicleSurfaceSample, 4> &surfaces,
                        double deltaSeconds );

        /**
         * @brief Register an impact event to be processed by the damage system.
         *
         * This will update internal damage state and may produce a VehicleDamageEvent
         * describing resulting changes (visual damage, performance degradation, etc.).
         *
         * @param impact Impact description (location, normal, impulse, etc.).
         * @return procedural::VehicleDamageEvent Event produced by the damage handler.
         */
        procedural::VehicleDamageEvent registerImpact( const procedural::VehicleDamageImpact &impact );

        /**
         * @brief Get the current dynamics state (physics-related telemetry and pose).
         *
         * @return const procedural::VehicleDynamicsState& Dynamics state snapshot.
         */
        const procedural::VehicleDynamicsState &getDynamicsState() const;

        /**
         * @brief Get the current presentation state (transforms, animation state, etc.).
         *
         * @return const procedural::VehiclePresentationState& Presentation state snapshot.
         */
        const procedural::VehiclePresentationState &getPresentationState() const;

        /**
         * @brief Get accumulated effect events (audio, particles, decals, etc.) since last query.
         *
         * The returned vector contains VehicleEffectEvent entries generated by the
         * VehicleEffects implementation during simulation steps.
         *
         * @return Source-admitted effect-event inventory, including retained spare capacity.
         */
        const Array<procedural::VehicleEffectEvent> &getEffects() const;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Build or rebuild the runtime mesh from generated geometry.
         *
         * Overrides ProceduralMeshComponent::buildMesh to create an IMesh reflecting
         * the currently generated vehicle geometry.
         *
         * @return SmartPtr<IMesh> Constructed mesh (may be nullptr on failure).
         */
        SmartPtr<IMesh> buildMesh() override;

        /**
         * @brief Apply materials generated by the vehicle generator to the mesh renderer.
         *
         * This will map generatedMaterialIds / m_generatedMaterials onto the renderer
         * assigned to this component's actor.
         */
        void applyGeneratedMaterials() override;

        // String containing comma-separated generated material identifiers produced by the generator.
        String m_generatedMaterialIds;

        // Procedural systems used by this component. Any of these smart pointers may be null
        // if the corresponding subsystem is not active for a given vehicle instance.
        SmartPtr<procedural::IVehicleGenerator> m_generator;
        SmartPtr<procedural::IVehicleDynamics> m_dynamics;
        SmartPtr<procedural::IVehicleDamage> m_damage;
        SmartPtr<procedural::IVehiclePresentation> m_presentation;
        SmartPtr<procedural::IVehicleEffects> m_effects;

        // Runtime state produced/consumed by the subsystems during simulation.
        procedural::VehicleDynamicsState m_dynamicsState;
        procedural::VehicleDynamicsTelemetry m_telemetry;
        procedural::VehicleDamageState m_damageState;
        procedural::VehiclePresentationState m_presentationState;
        procedural::VehicleEffectsState m_effectsState;

        // Configuration and generated assets.
        procedural::VehicleGenerationConfig m_config;
        procedural::GeneratedVehicle m_vehicle;

        // Level-of-detail index used when generating or selecting meshes and materials.
        u32 m_lod = 0;

        // Materials produced by the generator and applied to the runtime renderer.
        Array<SmartPtr<Material>> m_generatedMaterials{ Allocator<SmartPtr<Material>>( sourceBudget ) };

        // Accumulated effect events produced by the last simulation step(s).
        Array<procedural::VehicleEffectEvent> m_events{ Allocator<procedural::VehicleEffectEvent>(
            sourceBudget ) };
        // Second admitted generation for transactional fixed-step output. Warm
        // updates reuse both buffers without admitting replacement storage.
        Array<procedural::VehicleEffectEvent> m_eventScratch{ Allocator<procedural::VehicleEffectEvent>(
            sourceBudget ) };
    };
}  // namespace workphone::scene
