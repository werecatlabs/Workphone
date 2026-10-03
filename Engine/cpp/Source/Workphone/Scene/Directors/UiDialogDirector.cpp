#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/UiDialogDirector.hpp>
#include <Workphone/Scene/Directors/ButtonDirector.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, UiDialogDirector, Director );

    UiDialogDirector::UiDialogDirector() = default;
    UiDialogDirector::~UiDialogDirector() = default;

    SmartPtr<Properties> UiDialogDirector::getProperties() const
    {
        auto properties = Director::getProperties();

        setupHeaderProperties( properties );
        getButtons( properties );

        properties->setPropertyAsType( "buttonDirector", m_buttonDirector );
        properties->setPropertyAsType( "tabButtonDirector", m_tabButtonDirector );
        properties->setProperty( "backgroundTexture", m_backgroundTexture );
        properties->setProperty( "title", m_title );
        return properties;
    }

    void UiDialogDirector::setProperties( SmartPtr<Properties> properties )
    {
        setButtons( properties );

        properties->getPropertyAsType( "buttonDirector", m_buttonDirector );
        properties->getPropertyAsType( "tabButtonDirector", m_tabButtonDirector );
        properties->getPropertyValue( "backgroundTexture", m_backgroundTexture );
        properties->getPropertyValue( "title", m_title );

        save();
    }

    void UiDialogDirector::setTabButtonDirector( SmartPtr<ButtonDirector> tabButtonDirector )
    {
        m_tabButtonDirector = tabButtonDirector;
    }

    SmartPtr<ButtonDirector> UiDialogDirector::getTabButtonDirector() const
    {
        return m_tabButtonDirector;
    }

    void UiDialogDirector::setButtonDirector( SmartPtr<ButtonDirector> buttonDirector )
    {
        m_buttonDirector = buttonDirector;
    }

    SmartPtr<ButtonDirector> UiDialogDirector::getButtonDirector() const
    {
        return m_buttonDirector;
    }

    void UiDialogDirector::setBackgroundTexture( SmartPtr<render::ITexture> backgroundTexture )
    {
        m_backgroundTexture = backgroundTexture;
    }

    SmartPtr<render::ITexture> UiDialogDirector::getBackgroundTexture() const
    {
        return m_backgroundTexture;
    }

    void UiDialogDirector::setTitle( const String &title )
    {
        m_title = title;
    }

    String UiDialogDirector::getTitle() const
    {
        return m_title;
    }

}  // namespace workphone::scene
