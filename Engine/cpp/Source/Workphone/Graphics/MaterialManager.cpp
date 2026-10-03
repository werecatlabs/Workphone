#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/MaterialManager.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, MaterialManager, IMaterialManager );
    WP_CLASS_REGISTER_DERIVED( workphone::render, MaterialManager::StateListener, IMaterialManager );

    MaterialManager::MaterialManager() = default;

    MaterialManager::~MaterialManager() = default;

    void MaterialManager::load( SmartPtr<ISharedObject> data )
    {
        m_materials.reserve( 1024 );
    }

    void MaterialManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto materials = m_materials.snapshot();
            for( auto &material : materials )
            {
                if( material )
                {
                    material->unload( nullptr );
                }
            }

            m_materials.clear();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialManager::cloneMaterial( SmartPtr<IMaterial> material, const String &clonedMaterialName )
        -> SmartPtr<IMaterial>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto newMaterial = factoryManager->make_ptr<Material>();

        newMaterial->setParentPrototype( material );

        auto data = material->toData();
        newMaterial->fromData( data );

        newMaterial->setName( clonedMaterialName );

        newMaterial->load( nullptr );

        m_materials.emplace_back( newMaterial );

        return newMaterial;
    }

    auto MaterialManager::cloneMaterial( const String &name, const String &clonedMaterialName )
        -> SmartPtr<IMaterial>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto materialResource = getByName( name );
        if( materialResource )
        {
            auto material = factoryManager->make_ptr<Material>();

            auto data = materialResource->toData();
            material->fromData( data );

            material->setName( name );

            m_materials.emplace_back( material );

            return material;
        }

        return nullptr;
    }

    auto MaterialManager::create( const String &name ) -> SmartPtr<IResource>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystemPtr();
        WP_ASSERT( fileSystem );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto material = factoryManager->make_ptr<Material>();
        WP_ASSERT( material );

        WP_ASSERT( material->getHandle() );
        if( auto handle = material->getHandle() )
        {
            material->setName( name );

            auto uuid = StringUtil::getUUID();
            handle->setUUID( uuid );
        }

        material->setFilePath( name );

        FileInfo fileInfo;
        if( fileSystem->findFileInfo( name, fileInfo ) )
        {
            material->setFileSystemId( fileInfo.fileId );
        }

        m_materials.emplace_back( material );

        return material;
    }

    auto MaterialManager::create( const String &uuid, const String &name ) -> SmartPtr<IResource>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystemPtr();
        WP_ASSERT( fileSystem );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto material = factoryManager->make_ptr<Material>();
        WP_ASSERT( material );

        WP_ASSERT( material->getHandle() );

        setName( name );

        if( auto handle = material->getHandle() )
        {
            handle->setUUID( uuid );
        }

        material->setFilePath( name );

        FileInfo fileInfo;
        if( fileSystem->findFileInfo( name, fileInfo ) )
        {
            material->setFileSystemId( fileInfo.fileId );
        }

        m_materials.emplace_back( material );

        return material;
    }

    auto MaterialManager::createOrRetrieve( const String &uuid, const String &path, const String &type )
        -> Pair<SmartPtr<IResource>, bool>
    {
        if( auto materialResource = getById( uuid ) )
        {
            return Pair<SmartPtr<IResource>, bool>( materialResource, false );
        }

        if( !StringUtil::isNullOrEmpty( uuid ) )
        {
            auto material = create( uuid, path );
            WP_ASSERT( material );

            return Pair<SmartPtr<IResource>, bool>( material, true );
        }

        auto material = create( path );
        WP_ASSERT( material );

        return Pair<SmartPtr<IResource>, bool>( material, true );
    }

    auto MaterialManager::createOrRetrieve( const String &path ) -> Pair<SmartPtr<IResource>, bool>
    {
        if( auto materialResource = getByName( path ) )
        {
            return Pair<SmartPtr<IResource>, bool>( materialResource, false );
        }

        auto material = create( path );
        WP_ASSERT( material );

        return Pair<SmartPtr<IResource>, bool>( material, true );
    }

    void MaterialManager::saveToFile( const String &filePath, SmartPtr<IResource> resource )
    {
        try
        {
            ScopedLock lock( this );

            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
            WP_ASSERT( resource );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto material = workphone::static_pointer_cast<IMaterial>( resource );
            auto pData = workphone::static_pointer_cast<Properties>( material->toData() );

            auto materialStr = DataUtil::toString( pData.get(), true );
            fileSystem->writeAllText( filePath, materialStr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MaterialManager::loadFromFile( const String &filePath ) -> SmartPtr<IResource>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto materialName = Path::getFileNameWithoutExtension( filePath );

            auto materials = m_materials.snapshot();
            for( auto material : materials )
            {
                if( material )
                {
                    auto currentMaterialName = material->getName();

                    if( materialName == currentMaterialName )
                    {
                        FileInfo fileInfo;
                        if( fileSystem->findFileInfo( filePath, fileInfo ) )
                        {
                            auto currentMaterialPath = String( fileInfo.filePath.c_str() );
                            auto currentFileId = material->getFileSystemId();

                            if( filePath == currentMaterialPath && fileInfo.fileId == currentFileId )
                            {
                                return material;
                            }
                        }
                    }
                }
            }

            auto ext = Path::getFileExtension( filePath );
            if( ext == ".mat" )
            {
                auto stream = fileSystem->open( filePath, true, false, false, false, false );
                if( !stream )
                {
                    stream = fileSystem->open( filePath, true, false, false, true, true );
                }

                if( stream )
                {
                    auto materialStr = stream->getAsString();

                    auto materialData = workphone::make_ptr<Properties>();
                    DataUtil::parse( materialStr, materialData.get() );

                    auto material = create( materialName );

                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( filePath, fileInfo ) )
                    {
                        auto fileId = fileInfo.fileId;
                        material->setFileSystemId( fileId );
                    }

                    material->setFilePath( filePath );

                    material->fromData( materialData );

                    if( !material->isLoaded() )
                    {
                        graphicsSystem->loadObject( material );
                    }

                    m_materials.emplace_back( material );

                    return material;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto MaterialManager::loadResource( const String &name ) -> SmartPtr<IResource>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

            if( auto materialResource = getByName( name ) )
            {
                materialResource->load( nullptr );
                return materialResource;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto MaterialManager::getByName( const String &name ) -> SmartPtr<IResource>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

            auto materials = m_materials.snapshot();
            for( auto &material : materials )
            {
                if( material )
                {
                    if( material->getName() == name )
                    {
                        return material;
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto MaterialManager::getById( const String &uuid ) -> SmartPtr<IResource>
    {
        try
        {
            auto materials = m_materials.snapshot();
            for( auto &material : materials )
            {
                if( auto handle = material->getHandle() )
                {
                    if( handle->getUUIDAsString() == uuid )
                    {
                        return material;
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void MaterialManager::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    void MaterialManager::destroyAll()
    {
        auto materials = m_materials.snapshot();
        for( auto &material : materials )
        {
            if( material )
            {
                material->unload( nullptr );
            }
        }

        m_materials.clear();
    }

    void MaterialManager::destroyResource( SmartPtr<IResource> resource )
    {
        m_materials.erase( std::remove( m_materials.begin(), m_materials.end(), resource ),
                           m_materials.end() );
    }

    SmartPtr<IResource> MaterialManager::cloneResource( SmartPtr<IResource> resource,
                                                        const String &clonedResourceName )
    {
        try
        {
            if( !resource )
            {
                WP_LOG_ERROR( "MaterialManager::cloneResource: resource is null." );
                return nullptr;
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            // Try to clone as IMaterial if possible
            auto material = workphone::static_pointer_cast<IMaterial>( resource );
            if( material )
            {
                auto newMaterial = factoryManager->make_ptr<Material>();
                newMaterial->setParentPrototype( material );
                auto data = material->toData();
                newMaterial->fromData( data );
                newMaterial->setName( clonedResourceName );
                newMaterial->load( nullptr );
                m_materials.emplace_back( newMaterial );
                return newMaterial;
            }

            WP_LOG_ERROR( "MaterialManager::cloneResource: failed to clone resource (unknown type)." );
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "MaterialManager::cloneResource: exception occurred." );
            WP_LOG_EXCEPTION( e );
        }
        return nullptr;
    }

    SmartPtr<IResource> MaterialManager::cloneResource( const String &name,
                                                        const String &clonedResourceName )
    {
        try
        {
            if( StringUtil::isNullOrEmpty( name ) )
            {
                WP_LOG_ERROR( "MaterialManager::cloneResource: name is null or empty." );
                return nullptr;
            }

            auto resource = getByName( name );
            if( !resource )
            {
                WP_LOG_ERROR( "MaterialManager::cloneResource: resource not found by name: " + name );
                return nullptr;
            }

            return cloneResource( resource, clonedResourceName );
        }
        catch( std::exception &e )
        {
            WP_LOG_ERROR( "MaterialManager::cloneResource (by name): exception occurred." );
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void MaterialManager::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    SmartPtr<IStateContext> MaterialManager::getStateContext() const
    {
        return m_stateContext;
    }

    bool MaterialManager::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        auto materials = m_materials.snapshot();
        for( auto &material : materials )
        {
            if( message->getSender() == material )
            {
                if( material )
                {
                    if( material->handleStateMessage( message ) )
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool MaterialManager::handleStateChanged( SmartPtr<IState> &state )
    {
        auto materials = m_materials.snapshot();
        for( auto &material : materials )
        {
            if( material )
            {
                if( material->handleStateChanged( state ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    void MaterialManager::StateListener::setOwner( SmartPtr<MaterialManager> owner )
    {
        m_owner = owner;
    }

    SmartPtr<MaterialManager> MaterialManager::StateListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    bool MaterialManager::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwnerPtr() )
        {
            return owner->handleStateChanged( state );
        }

        return false;
    }

    bool MaterialManager::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwnerPtr() )
        {
            return owner->handleStateMessage( message );
        }

        return false;
    }

    void MaterialManager::StateListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    MaterialManager::StateListener::StateListener() = default;

    MaterialManager::StateListener::~StateListener() = default;

}  // namespace workphone::render
