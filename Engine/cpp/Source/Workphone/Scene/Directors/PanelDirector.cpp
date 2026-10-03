#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/PanelDirector.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Scene/Directors/UiDialogDirector.hpp>
#include <Workphone/Scene/Directors/UiElementDirector.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, PanelDirector, Director );

    PanelDirector::PanelDirector() = default;

    PanelDirector::~PanelDirector() = default;

    SmartPtr<Properties> PanelDirector::getProperties() const
    {
        auto properties = Director::getProperties();
        properties->setPropertyAsType( "texture", m_texture );
        return properties;
    }

    void PanelDirector::setProperties( SmartPtr<Properties> properties )
    {
        properties->getPropertyAsType( "texture", m_texture );
    }

    void PanelDirector::setTexture( SmartPtr<render::ITexture> texture )
    {
        m_texture = texture;
    }

    SmartPtr<render::ITexture> PanelDirector::getTexture() const
    {
        return m_texture;
    }
}  // namespace workphone::scene
