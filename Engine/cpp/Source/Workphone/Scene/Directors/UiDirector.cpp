#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/UiDirector.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Scene/Directors/ButtonDirector.hpp>
#include <Workphone/Scene/Directors/UiDialogDirector.hpp>
#include <Workphone/Scene/Directors/UiElementDirector.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, UiDirector, Director );

    const String UiDirector::defaultButtonDirectorStr = String( "defaultButtonDirector" );
    const String UiDirector::startDialogDirectorStr = String( "startDialogDirector" );
    const String UiDirector::defaultBackgroundTextureStr = String( "defaultBackgroundTexture" );
    const String UiDirector::hasCharacterSelectStr = String( "hasCharacterSelect" );
    const String UiDirector::hasVehicleSelectStr = String( "hasVehicleSelect" );
    const String UiDirector::hasSceneSelectStr = String( "hasSceneSelect" );
    const String UiDirector::hasWorkshopStr = String( "hasWorkshop" );

    UiDirector::UiDirector() = default;

    UiDirector::~UiDirector() = default;

    SmartPtr<Properties> UiDirector::getProperties() const
    {
        auto properties = Director::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setPropertyAsType( defaultButtonDirectorStr, getDefaultButtonDirector() );
        properties->setPropertyAsType( startDialogDirectorStr, getStartDialogDirector() );
        properties->setPropertyAsType( defaultBackgroundTextureStr, getDefaultBackgroundTexture() );

        properties->setProperty( hasCharacterSelectStr, hasCharacterSelect() );
        properties->setProperty( hasVehicleSelectStr, hasVehicleSelect() );
        properties->setProperty( hasSceneSelectStr, hasSceneSelect() );
        properties->setProperty( hasWorkshopStr, hasWorkshop() );

        return properties;
    }

    void UiDirector::setProperties( SmartPtr<Properties> properties )
    {
        Director::setProperties( properties );
        if( !properties )
        {
            return;
        }

        auto defaultButtonDirector = getDefaultButtonDirector();
        auto startDialogDirector = getStartDialogDirector();
        auto defaultBackgroundTexture = getDefaultBackgroundTexture();
        auto characterSelect = hasCharacterSelect();
        auto vehicleSelect = hasVehicleSelect();
        auto sceneSelect = hasSceneSelect();
        auto workshop = hasWorkshop();

        properties->getPropertyAsType( defaultButtonDirectorStr, defaultButtonDirector );
        properties->getPropertyAsType( startDialogDirectorStr, startDialogDirector );
        properties->getPropertyAsType( defaultBackgroundTextureStr, defaultBackgroundTexture );

        properties->getPropertyValue( hasCharacterSelectStr, characterSelect );
        properties->getPropertyValue( hasVehicleSelectStr, vehicleSelect );
        properties->getPropertyValue( hasSceneSelectStr, sceneSelect );
        properties->getPropertyValue( hasWorkshopStr, workshop );

        setDefaultButtonDirector( defaultButtonDirector );
        setStartDialogDirector( startDialogDirector );
        setDefaultBackgroundTexture( defaultBackgroundTexture );
        setHasCharacterSelect( characterSelect );
        setHasVehicleSelect( vehicleSelect );
        setHasSceneSelect( sceneSelect );
        setHasWorkshop( workshop );
    }

    SmartPtr<ButtonDirector> UiDirector::getDefaultButtonDirector() const
    {
        return m_defaultButtonDirector;
    }

    void UiDirector::setDefaultButtonDirector( SmartPtr<ButtonDirector> defaultButtonDirector )
    {
        m_defaultButtonDirector = defaultButtonDirector;
    }

    void UiDirector::setStartDialogDirector( SmartPtr<UiDialogDirector> startDialogDirector )
    {
        m_startDialogDirector = startDialogDirector;
    }

    SmartPtr<UiDialogDirector> UiDirector::getStartDialogDirector() const
    {
        return m_startDialogDirector;
    }

    void UiDirector::setDefaultBackgroundTexture( SmartPtr<render::ITexture> defaultBackgroundTexture )
    {
        m_defaultBackgroundTexture = defaultBackgroundTexture;
    }

    SmartPtr<render::ITexture> UiDirector::getDefaultBackgroundTexture() const
    {
        return m_defaultBackgroundTexture;
    }

    bool UiDirector::hasCharacterSelect() const
    {
        return m_hasCharacterSelect;
    }

    void UiDirector::setHasCharacterSelect( bool hasCharacterSelect )
    {
        m_hasCharacterSelect = hasCharacterSelect;
    }

    bool UiDirector::hasVehicleSelect() const
    {
        return m_hasVehicleSelect;
    }

    void UiDirector::setHasVehicleSelect( bool hasVehicleSelect )
    {
        m_hasVehicleSelect = hasVehicleSelect;
    }

    bool UiDirector::hasSceneSelect() const
    {
        return m_hasSceneSelect;
    }

    void UiDirector::setHasSceneSelect( bool hasSceneSelect )
    {
        m_hasSceneSelect = hasSceneSelect;
    }

    bool UiDirector::hasWorkshop() const
    {
        return m_hasWorkshop;
    }

    void UiDirector::setHasWorkshop( bool hasWorkshop )
    {
        m_hasWorkshop = hasWorkshop;
    }

}  // namespace workphone::scene
