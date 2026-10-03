#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Director.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/FileInfo.hpp>

namespace workphone
{
    const String Director::versionStr = String( "version" );
    const String Director::objectTypeStr = String( "objectType" );

    WP_CLASS_REGISTER_DERIVED( workphone, Director, Resource<IBuildDirector> );

    Director::Director() = default;

    Director::~Director() = default;

    void Director::load( SmartPtr<ISharedObject> data )
    {
        Resource<IBuildDirector>::load( data );
    }

    void Director::unload( SmartPtr<ISharedObject> data )
    {
        Resource<IBuildDirector>::unload( data );
    }

    void Director::saveToFile( const String &filePath )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto data = workphone::static_pointer_cast<Properties>( getProperties() );
            WP_ASSERT( data );

            auto dataStr = DataUtil::toString( data.get(), true );
            WP_ASSERT( !StringUtil::isNullOrEmpty( dataStr ) );

            auto filePath = getFilePath();
            if( !StringUtil::isNullOrEmpty( filePath ) )
            {
                fileSystem->writeAllText( filePath, dataStr );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Director::loadFromFile( const String &filePath )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto dataStr = fileSystem->readAllText( filePath );

            auto properties = workphone::make_ptr<Properties>();
            DataUtil::parse( dataStr, properties.get() );

            setProperties( properties );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Director::save()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto data = workphone::static_pointer_cast<Properties>( getProperties() );
            if( data )
            {
                auto dataStr = DataUtil::toString( data.get(), true );
                WP_ASSERT( !StringUtil::isNullOrEmpty( dataStr ) );

                auto fileId = getFileSystemId();
                // WP_ASSERT( fileId != 0 );

                FileInfo fileInfo;
                if( fileSystem->findFileInfo( fileId, fileInfo ) )
                {
                    auto filePath = fileInfo.filePath.c_str();
                    if( !StringUtil::isNullOrEmpty( filePath ) )
                    {
                        fileSystem->writeAllText( filePath, dataStr );
                    }
                }
                else
                {
                    auto filePath = getFilePath();
                    if( !StringUtil::isNullOrEmpty( filePath ) )
                    {
                        fileSystem->writeAllText( filePath, dataStr );
                    }
                }
            }
            else
            {
                WP_LOG_ERROR( "Failed to save data is null." );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Director::toData() const -> SmartPtr<ISharedObject>
    {
        auto properties = getProperties();
        return properties;
    }

    void Director::fromData( SmartPtr<ISharedObject> data )
    {
        setProperties( data );
    }

    auto Director::getProperties() const -> SmartPtr<Properties>
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto properties = factoryManager->make_ptr<Properties>();
        setupHeaderProperties( properties );
        getButtons( properties );
        return properties;
    }

    void Director::setProperties( SmartPtr<Properties> properties )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        setButtons( properties );
    }

    void Director::setupHeaderProperties( SmartPtr<Properties> properties ) const
    {
        auto typeManager = TypeManager::instance();
        auto type = this->getTypeInfo();
        auto typeName = String( typeManager->getName( type ) );

        auto version = String( "1.0.0" );

        properties->setProperty( Director::versionStr, version );
        properties->setProperty( Director::objectTypeStr, typeName );
    }

    auto Director::getParent() const -> SmartPtr<IBuildDirector>
    {
        return m_parent;
    }

    void Director::setParent( SmartPtr<IBuildDirector> parent )
    {
        m_parent = parent;
    }

    void Director::addChild( SmartPtr<IBuildDirector> child )
    {
        m_children.push_back( child );
    }

    void Director::removeChild( SmartPtr<IBuildDirector> child )
    {
        auto it = std::find( m_children.begin(), m_children.end(), child );
        if( it != m_children.end() )
        {
            m_children.erase( it );
        }
    }

    void Director::removeChildren()
    {
        m_children.clear();
    }

    auto Director::findChild( const String &name ) -> SmartPtr<IBuildDirector>
    {
        for( auto &child : m_children )
        {
            if( child->getName() == name )
            {
                return child;
            }
        }

        return nullptr;
    }

    auto Director::getChildren() const -> Array<SmartPtr<IBuildDirector>>
    {
        return m_children.snapshot();
    }

    void Director::getButtons( SmartPtr<Properties> properties ) const
    {
        properties->setButtonPressed( "Save" );
        properties->setButtonPressed( "Import" );
    }

    void Director::setButtons( SmartPtr<Properties> properties )
    {
        if( properties->isButtonPressed( "Save" ) )
        {
            save();
        }

        if( properties->isButtonPressed( "Import" ) )
        {
            import();
        }
    }

}  // namespace workphone
