#pragma once

#include <Workphone/Scene/Components/ProceduralScene.hpp>

#include <memory>

namespace workphone::scene
{
    class ProceduralCityConfiguration;

    /// Builds city scenery on its owner and can bind the generated city to editable terrain roads.
    class WPCore_API ProceduralCityScene : public ProceduralScene
    {
    public:
        ProceduralCityScene();
        ~ProceduralCityScene() override;

        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;
        SmartPtr<Properties> getProperties() const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        void readGenerationProperties( Properties &properties ) override;
        bool prepareGeneration() override;
        void buildGeneratedScene() override;
        void generationFailed() override;

    private:
        std::unique_ptr<ProceduralCityConfiguration> m_city;
    };
}  // namespace workphone::scene
