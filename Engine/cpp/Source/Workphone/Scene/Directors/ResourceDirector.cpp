#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ResourceDirector, Director );

    const String ResourceDirector::resourcePathStr = String( "resourcePath" );
    const String ResourceDirector::resourceUUIDStr = String( "resourceUUID" );
    const String ResourceDirector::saveStr = String( "Save" );
    const String ResourceDirector::importStr = String( "Import" );

    ResourceDirector::ResourceDirector() = default;

    ResourceDirector::~ResourceDirector() = default;

    void ResourceDirector::load( SmartPtr<ISharedObject> data )
    {
        Director::load( data );
    }

    void ResourceDirector::unload( SmartPtr<ISharedObject> data )
    {
        Director::unload( data );
    }

    void ResourceDirector::import()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return;
        }

        auto resourceDatabase = applicationManager->getResourceDatabase();
        if( !resourceDatabase )
        {
            return;
        }

        auto resourcePath = getResourcePath();
        if( resourcePath.empty() )
        {
            return;
        }

        // An explicit reimport must rebuild cached data using the saved options.
        resourceDatabase->importFile( resourcePath, true );
    }

    String ResourceDirector::getResourcePath() const
    {
        return m_resourcePath;
    }

    void ResourceDirector::setResourcePath( const String &resourcePath )
    {
        m_resourcePath = resourcePath;
    }

    void ResourceDirector::setResourceUUID( const String &resourceUUID )
    {
        m_resourceUUID = resourceUUID;
    }

    String ResourceDirector::getResourceUUID() const
    {
        return m_resourceUUID;
    }

    void ResourceDirector::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        auto resourcePath = getResourcePath();
        auto resourceUUID = getResourceUUID();
        properties->getPropertyValue( resourcePathStr, resourcePath );
        properties->getPropertyValue( resourceUUIDStr, resourceUUID );
        setResourcePath( resourcePath );
        setResourceUUID( resourceUUID );

        // Director dispatches the buttons once, after the resource path is current.
        Director::setProperties( properties );
    }

    SmartPtr<Properties> ResourceDirector::getProperties() const
    {
        auto properties = Director::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( resourcePathStr, getResourcePath() );
        properties->setProperty( resourceUUIDStr, getResourceUUID() );
        properties->setButtonPressed( saveStr, false );
        properties->setButtonPressed( importStr, false );

        return properties;
    }
}  // namespace workphone::scene
