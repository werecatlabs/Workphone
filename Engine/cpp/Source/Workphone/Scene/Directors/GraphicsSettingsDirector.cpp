#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/GraphicsSettingsDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, GraphicsSettingsDirector, Director );

    GraphicsSettingsDirector::GraphicsSettingsDirector() :
        m_pipeline( workphone::make_ptr<render::GraphicsPipeline>() )
    {
        m_pipeline->setQualityLevel( render::QualityLevel::High );
    }

    GraphicsSettingsDirector::~GraphicsSettingsDirector() = default;

    SmartPtr<Properties> GraphicsSettingsDirector::getProperties() const
    {
        auto properties = m_pipeline->getProperties();
        properties->setProperty( "Applies to", "Standalone game runtime", true );
        properties->setPropertyAsEnum( "Render API", m_renderApi, { "Software", "DirectX 11" } );
        return properties;
    }

    void GraphicsSettingsDirector::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
            return;
        s32 api = m_renderApi;
        if( properties->getPropertyValue( "Render API", api ) && api >= 0 && api <= 1 )
            m_renderApi = api;
        m_pipeline->setProperties( properties );
    }

    render::IGraphicsSystem::RenderApi GraphicsSettingsDirector::getRenderApi() const
    {
        using Api = render::IGraphicsSystem::RenderApi;
        return m_renderApi == 0 ? Api::Software : Api::DX11;
    }
}  // namespace workphone::scene
