#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawMaterialManager, MaterialManager );

    ClawMaterialManager::ClawMaterialManager()
    {
        static const String className = "CMaterialManager";
        setName( className );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( stateManager );
        WP_ASSERT( graphicsSystem );
        WP_ASSERT( factoryManager );

        auto stateContext = stateManager->addStateContext();
        stateContext->setOwner( this );
        stateContext->setTaskId( graphicsSystem->getStateTask() );
        setStateContext( stateContext );

        auto stateListener = factoryManager->make_ptr<StateListener>();
        stateListener->setOwner( this );
        setStateListener( stateListener );
        stateContext->addStateListener( stateListener );
    }

    ClawMaterialManager::~ClawMaterialManager() = default;

    void ClawMaterialManager::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        MaterialManager::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void ClawMaterialManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );
            destroyStateContext();
            MaterialManager::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IMaterial> ClawMaterialManager::cloneMaterial( SmartPtr<IMaterial> material,
                                                            const String &clonedMaterialName )
    {
        try
        {
            if( !material )
            {
                return nullptr;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto clonedMaterial = factoryManager->make_ptr<ClawMaterial>();
            clonedMaterial->setResourceManager( this );
            clonedMaterial->setParentPrototype( material );
            clonedMaterial->fromData( material->toData() );
            clonedMaterial->setName( clonedMaterialName );
            clonedMaterial->load( nullptr );

            m_materials.emplace_back( clonedMaterial );
            return clonedMaterial;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IMaterial> ClawMaterialManager::cloneMaterial( const String &name,
                                                            const String &clonedMaterialName )
    {
        auto material = workphone::dynamic_pointer_cast<IMaterial>( getByName( name ) );
        return cloneMaterial( material, clonedMaterialName );
    }

    SmartPtr<IResource> ClawMaterialManager::create( const String &name )
    {
        return create( StringUtil::getUUID(), name );
    }

    SmartPtr<IResource> ClawMaterialManager::create( const String &uuid, const String &name )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto material = factoryManager->make_ptr<ClawMaterial>();
            WP_ASSERT( material );
            material->setResourceManager( this );
            material->setName( name );
            material->setFilePath( name );

            if( auto handle = material->getHandle() )
            {
                handle->setUUID( uuid );
            }

            FileInfo fileInfo;
            if( fileSystem->findFileInfo( name, fileInfo ) )
            {
                material->setFileSystemId( fileInfo.fileId );
            }

            m_materials.emplace_back( material );
            return material;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void ClawMaterialManager::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = nullptr;
        }
    }
}  // namespace workphone::render
