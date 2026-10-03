#pragma once
#include <Workphone/Scene/Components/ProceduralMeshComponent.hpp>
#include <Workphone/Interface/Procedural/IVehicleGenerator.hpp>
#include <Workphone/Interface/Procedural/IVehicleDynamics.hpp>
#include <Workphone/Interface/Procedural/IVehicleDamage.hpp>
#include <Workphone/Interface/Procedural/IVehiclePresentation.hpp>
#include <Workphone/Interface/Procedural/IVehicleEffects.hpp>
namespace workphone::scene
{
    /// Editor authoring component. Generates a Mesh and MeshRenderer on the same actor.
    /// Simulation is explicitly fixed-stepped by the client with world surface samples.
    class WPCore_API ProceduralVehicle : public ProceduralMeshComponent
    {
    public:
        ProceduralVehicle();
        ~ProceduralVehicle() override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        const procedural::VehicleGenerationConfig &getConfig() const
        {
            return m_config;
        }
        void setConfig( const procedural::VehicleGenerationConfig &config )
        {
            m_config = config;
            requestRegeneration();
        }
        void setGenerator( SmartPtr<procedural::IVehicleGenerator> generator )
        {
            m_generator = generator;
            requestRegeneration();
        }
        const procedural::GeneratedVehicle &getGeneratedVehicle() const
        {
            return m_vehicle;
        }
        bool stepFixed( const procedural::VehicleControlInput &input,
                        const std::array<procedural::VehicleSurfaceSample, 4> &surfaces,
                        double deltaSeconds );
        procedural::VehicleDamageEvent registerImpact( const procedural::VehicleDamageImpact &impact );
        const procedural::VehicleDynamicsState &getDynamicsState() const
        {
            return m_dynamicsState;
        }
        const procedural::VehiclePresentationState &getPresentationState() const
        {
            return m_presentationState;
        }
        const std::vector<procedural::VehicleEffectEvent> &getEffects() const
        {
            return m_events;
        }
        WP_CLASS_REGISTER_DECL;

    protected:
        SmartPtr<IMesh> buildMesh() override;
        void applyGeneratedMaterials() override;

    private:
        procedural::VehicleGenerationConfig m_config;
        procedural::GeneratedVehicle m_vehicle;
        u32 m_lod = 0;
        Array<SmartPtr<Material>> m_generatedMaterials;
        String m_generatedMaterialIds;
        SmartPtr<procedural::IVehicleGenerator> m_generator;
        SmartPtr<procedural::IVehicleDynamics> m_dynamics;
        SmartPtr<procedural::IVehicleDamage> m_damage;
        SmartPtr<procedural::IVehiclePresentation> m_presentation;
        SmartPtr<procedural::IVehicleEffects> m_effects;
        procedural::VehicleDynamicsState m_dynamicsState;
        procedural::VehicleDynamicsTelemetry m_telemetry;
        procedural::VehicleDamageState m_damageState;
        procedural::VehiclePresentationState m_presentationState;
        procedural::VehicleEffectsState m_effectsState;
        std::vector<procedural::VehicleEffectEvent> m_events;
    };
}  // namespace workphone::scene
