#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ResourceDirector, Director );

    const String ResourceDirector::resourcePathStr = String( "resourcePath" );
    const String ResourceDirector::resourceUUIDStr = String( "resourceUUID" );
    const String ResourceDirector::saveStr = String( "Save" );
    const String ResourceDirector::importStr = String( "Import" );
    const String ResourceDirector::importStatusStr = String( "Import Status" );

    ResourceDirector::ResourceDirector() = default;

    ResourceDirector::~ResourceDirector() = default;

    void ResourceDirector::load( SmartPtr<ISharedObject> data )
    {
        Director::load( data );
    }

    void ResourceDirector::unload( SmartPtr<ISharedObject> data )
    {
        SourceBudgetString retired;
        {
            SpinRWMutex::ScopedLock lock( m_importStatusMutex );
            ++m_importRequest;
            m_importStatus.swap( retired );
        }
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
        const auto request = beginImport();
        if( auto database = dynamic_pointer_cast<ResourceDatabase>( resourceDatabase ) )
        {
            database->importFile( resourcePath, true, this, request );
        }
        else
        {
            resourceDatabase->importFile( resourcePath, true );
            setImportStatus( request, "Import requested; this resource database does not report completion." );
        }
    }

    String ResourceDirector::getImportStatus() const
    {
        SpinRWMutex::ScopedLock lock( m_importStatusMutex, false );
        return m_importStatus;
    }

    u64 ResourceDirector::beginImport()
    {
        SourceBudgetString status{ String( "Import queued." ) };
        SpinRWMutex::ScopedLock lock( m_importStatusMutex );
        m_importStatus.swap( status );
        return ++m_importRequest;
    }

    void ResourceDirector::setImportStatus( u64 request, const String &status )
    {
        {
            SpinRWMutex::ScopedLock lock( m_importStatusMutex, false );
            if( request != m_importRequest ) return;
        }
        SourceBudgetString replacement{ status };
        SpinRWMutex::ScopedLock lock( m_importStatusMutex );
        if( request == m_importRequest ) m_importStatus.swap( replacement );
    }

    SmartPtr<ISharedObject> ResourceDirector::toData() const
    {
        auto properties = getProperties();
        if( properties ) properties->removeProperty( importStatusStr );
        return properties;
    }

    String ResourceDirector::getResourcePath() const
    {
        return m_resourcePath;
    }

    void ResourceDirector::setResourcePath( const String &resourcePath )
    {
        if( m_resourcePath != resourcePath )
        {
            SourceBudgetString retired;
            {
                SpinRWMutex::ScopedLock lock( m_importStatusMutex );
                ++m_importRequest;
                m_importStatus.swap( retired );
            }
        }
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
        const auto status = getImportStatus();
        properties->setProperty( importStatusStr, status, true );
        auto &statusProperty = properties->getPropertyObject( importStatusStr );
        // Attribute values have fixed capacity; the full diagnostic lives in
        // the property string and is read directly by the hover tooltip.
        statusProperty.setAttribute( "description", "Hover to read the complete import status." );
        statusProperty.setAttribute( "valueTooltip", "true" );

        return properties;
    }
}  // namespace workphone::scene
