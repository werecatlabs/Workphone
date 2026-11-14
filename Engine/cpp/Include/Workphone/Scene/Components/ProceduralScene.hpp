#pragma once

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/Components/ProceduralSceneGeneration.hpp>
#include <mutex>

namespace workphone::scene
{
    /// Common lifecycle for seeded scene generators. Add a concrete generator to an actor.
    class WPCore_API ProceduralScene : public Component
    {
    public:
        ProceduralScene();
        ~ProceduralScene() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        bool regenerate();
        void clearGeneratedScene();
        virtual bool isGenerated() const;
        String getGenerationError() const;
        const race::SceneAssets &getAssets() const;

        u32 getSeed() const;
        void setSeed( u32 seed );
        s32 getQuality() const;
        void setQuality( s32 quality );
        bool getFoliageWorldEnabled() const;
        void setFoliageWorldEnabled( bool enabled );
        String getMaterialExportDirectory() const;
        void setMaterialExportDirectory( const String &directory );

        u32 getCircuitSampleCount() const;
        f32 getCircuitLength() const;
        u32 nearestCircuitSample( const Vector3F &position ) const;
        Vector3F getCircuitPosition( u32 index ) const;
        Vector3F getCircuitRight( u32 index ) const;

        WP_CLASS_REGISTER_DECL;

    protected:
        virtual void readGenerationProperties( Properties &properties );
        virtual bool prepareGeneration();
        virtual void buildGeneratedScene() = 0;
        virtual void generationFailed();
        virtual void generationCleared();
        void ensurePhysicsScene();
        void activateGeneratedAssets();

        mutable std::recursive_mutex m_generationMutex;
        race::SceneAssets m_assets;
        u32 m_seed = 7;
        procedural::VehicleAppearanceQuality m_quality = procedural::VehicleAppearanceQuality::High;
        bool m_foliageWorldEnabled = false;
        String m_materialExportDirectory;
        String m_generationError;

    private:
        atomic_bool m_generated{ false };
        atomic_bool m_activationPending{ false };
    };
}  // namespace workphone::scene
