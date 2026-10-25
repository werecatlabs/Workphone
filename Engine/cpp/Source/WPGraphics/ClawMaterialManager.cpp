#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawMaterialManager.hpp>
#include <WPGraphics/ClawMaterial.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/Path.hpp>

namespace workphone::render
{
    namespace
    {
        SmartPtr<IGraphicsSystem> materialGraphicsOwner()
        {
            auto app = core::IApplicationManager::instancePtr();
            return app ? app->getGraphicsSystem() : nullptr;
        }
    }
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
        ScopedLock graphicsOwner( materialGraphicsOwner() );
        setLoadingState( LoadingState::Loading );
        MaterialManager::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void ClawMaterialManager::unload( SmartPtr<ISharedObject> data )
    {
        ScopedLock graphicsOwner( materialGraphicsOwner() );
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
        ScopedLock graphicsOwner( materialGraphicsOwner() );
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
            if(auto source=dynamic_cast<ClawMaterial *>(material.get())) source->copyRuntimeShaderTo(*clonedMaterial);

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
        ScopedLock graphicsOwner( materialGraphicsOwner() );
        try
        {
            if( !uuid.empty() ) if( auto existing = getById( uuid ) ) return existing;
            if( !name.empty() ) if( auto existing = getByName( name ) ) return existing;
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

    SmartPtr<IResource> ClawMaterialManager::loadFromFile( const String &filePath )
    {
        ScopedLock graphicsOwner( materialGraphicsOwner() );
        // Keep one manager owner per identity. The base implementation appended
        // the object twice and parsed again in load(), preventing retirement.
        SmartPtr<IMaterial> material;
        {
            ScopedLock<decltype( m_materials )> lock( &m_materials );
            for( const auto &entry : m_materials )
                if( entry && entry->getFilePath() == filePath )
                {
                    material = entry;
                    break;
                }
        }
        if( !material )
        {
            material = dynamic_pointer_cast<IMaterial>( create( filePath ) );
            if( material ) material->setName( Path::getFileNameWithoutExtension( filePath ) );
        }
        if( material && !material->isLoaded() ) material->load( nullptr );
        return material;
    }

    u32 ClawMaterialManager::releaseUnusedResources( u32 maximumCandidates )
    {
        auto application = core::IApplicationManager::instancePtr();
        auto graphics = application ? application->getGraphicsSystem() : nullptr;
        if( !graphics || !maximumCandidates ) return 0;
        ScopedLock graphicsLock( graphics );
        const auto count = m_materials.size();
        const auto candidates = std::min<size_t>( maximumCandidates, count );
        u32 released = 0;
        for( size_t i = 0; i < candidates; ++i )
        {
            SmartPtr<IMaterial> candidate;
            {
                ScopedLock<decltype( m_materials )> lock( &m_materials );
                if( m_materials.empty() ) break;
                m_evictionCursor %= m_materials.size();
                candidate = m_materials[m_evictionCursor++];
            }
            if( auto material = dynamic_cast<ClawMaterial *>( candidate.get() ) )
                if( material->releaseUnusedResources() ) ++released;
        }
        return released;
    }

    void ClawMaterialManager::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = nullptr;
        }
    }
}  // namespace workphone::render
