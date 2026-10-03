#include <EditorPCH.hpp>
#include "jobs/GenerateLighting.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    GenerateLighting::GenerateLighting() = default;
    GenerateLighting::~GenerateLighting() = default;

    void GenerateLighting::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto application = applicationManager->getApplication();

            auto lightActor = application->createDirectionalLight();
            setLightActor( lightActor );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void GenerateLighting::setLightActor( SmartPtr<scene::IGameActor> lightActor )
    {
        m_lightActor = lightActor;
    }

    SmartPtr<scene::IGameActor> GenerateLighting::getLightActor() const
    {
        return m_lightActor;
    }
}  // namespace workphone::editor
