#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreMaterialManager.h>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CMaterialManagerOgreNext, MaterialManager );

    CMaterialManagerOgreNext::CMaterialManagerOgreNext()
    {
        static const String className = "CMaterialManagerOgreNext";
        setName( className );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = applicationManager->getFactoryManager();

        auto stateContext = stateManager->addStateContext();
        stateContext->setOwner( this );
        stateContext->setTaskId( graphicsSystem->getStateTask() );
        setStateContext( stateContext );

        auto stateListener = factoryManager->make_ptr<StateListener>();
        stateListener->setOwner( this );
        setStateListener( stateListener );
        stateContext->addStateListener( stateListener );
    }

    CMaterialManagerOgreNext::~CMaterialManagerOgreNext() = default;

    void CMaterialManagerOgreNext::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_materials.reserve( 1024 );
        MaterialManager::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void CMaterialManagerOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            destroyStateContext();

            MaterialManager::unload( data );
            m_materials.clear();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto CMaterialManagerOgreNext::cloneMaterial( SmartPtr<IMaterial> material,
                                                  const String &clonedMaterialName )
        -> SmartPtr<IMaterial>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto newMaterial = factoryManager->make_ptr<CMaterialOgreNext>();
            newMaterial->setResourceManager( this );

            newMaterial->setParentPrototype( material );

            auto data = material->toData();
            newMaterial->fromData( data );

            auto handle = newMaterial->getHandle();
            newMaterial->setName( clonedMaterialName );

            newMaterial->load( nullptr );

            m_materials.emplace_back( newMaterial );

            return newMaterial;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto CMaterialManagerOgreNext::cloneMaterial( const String &name, const String &clonedMaterialName )
        -> SmartPtr<IMaterial>
    {
        try
        {
            auto materialResource = getByName( name );
            if( materialResource )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManagerPtr();
                WP_ASSERT( factoryManager );

                auto material = factoryManager->make_ptr<CMaterialOgreNext>();
                material->setResourceManager( this );

                auto data = materialResource->toData();
                material->fromData( data );

                auto handle = material->getHandle();
                material->setName( name );

                m_materials.emplace_back( material );

                return material;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto CMaterialManagerOgreNext::create( const String &name ) -> SmartPtr<IResource>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto material = factoryManager->make_ptr<CMaterialOgreNext>( this );
            WP_ASSERT( material );

            material->setName( name );

            WP_ASSERT( material->getHandle() );
            if( auto handle = material->getHandle() )
            {
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

            std::sort( m_materials.begin(), m_materials.end(),
                       []( SmartPtr<IMaterial> &a, SmartPtr<IMaterial> &b ) {
                           return a < b;  // compare by memory address
                       } );

            return material;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto CMaterialManagerOgreNext::create( const String &uuid, const String &name )
        -> SmartPtr<IResource>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto material = factoryManager->make_ptr<CMaterialOgreNext>( this );
            WP_ASSERT( material );

            WP_ASSERT( material->getHandle() );

            if( auto handle = material->getHandle() )
            {
                material->setName( name );
                handle->setUUID( uuid );
            }

            material->setFilePath( name );

            FileInfo fileInfo;
            if( fileSystem->findFileInfo( name, fileInfo ) )
            {
                material->setFileSystemId( fileInfo.fileId );
            }

            m_materials.emplace_back( material );

            std::sort( m_materials.begin(), m_materials.end(),
                       []( SmartPtr<IMaterial> &a, SmartPtr<IMaterial> &b ) {
                           return a < b;  // compare by memory address
                       } );

            return material;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void CMaterialManagerOgreNext::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

}  // namespace workphone::render
