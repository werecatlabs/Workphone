#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CResourceGroupManager.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSystemOgre.hpp>
#include <WPGraphicsOgre/Addons/ResourceGroupHelper.hpp>
#include <WPGraphicsOgre/Core/WPOgreDataStream.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreFontManager.h>
#include <OgreOverlayManager.h>

namespace workphone
{
    namespace render
    {
        const u32 CResourceGroupManager::RGMID_UNLOADRESOURCEGROUP =
            0;  // = StateManagerTBB::getNewId();

        CResourceGroupManager::CResourceGroupManager()
        {
            /*StateManagerPtr& stateManager = fb::IApplicationManager::instance()->getStateManager();
            m_stateListener = new StateListenerAdapter<CResourceGroupManager>(this);
            m_stateListener->setTaskNotifyId(TI_GraphicsTaskId);
            stateManager->addStateListener(m_stateListener);
            stateManager->registerObserver(this, m_stateListener);*/
        }

        CResourceGroupManager::~CResourceGroupManager()
        {
            /*if(m_stateListener)
            {
                StateManagerPtr& stateManager = fb::IApplicationManager::instance()->getStateManager();
                m_stateListener->setListener(NULL);
                stateManager->removeStateListener(m_stateListener);
                stateManager->unregisterObserver(this, m_stateListener);
                WP_SAFE_REMOVE(m_stateListener);
            }*/
        }

        void CResourceGroupManager::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                WP_ASSERT( Thread::getTaskFlag( Thread::Render_Flag ) );
                WP_ASSERT( getLoadingState() != LoadingState::Loaded );

                setLoadingState( LoadingState::Loading );
                WP_LOG( "Resource loading started." );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
                WP_ASSERT( resourceGroupManager );

                auto renderMediaPath = applicationManager->getRenderMediaPath();
                auto fileSystem = applicationManager->getFileSystem();
                fileSystem->addFolder( renderMediaPath, true );

                auto shaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();

                m_resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();
                m_resourceGroupHelper = new ResourceGroupHelper;

                auto useRTSS = false;
                if( useRTSS )
                {
                    auto shaderLibPath = String( "../../../../Media/RTShaderLib/HLSL" );
                    // auto shaderCachePath = String(".ShaderCache");
                    auto shaderCachePath = String( "./" );
                    // if( Ogre::RTShader::ShaderGenerator::initialize() )
                    {
                        // Add the shader libs resource location. a sample shader lib can be found in
                        // Samples\Media\RTShaderLib
                        Ogre::ResourceGroupManager::getSingleton().addResourceLocation( shaderLibPath.c_str(),
                                                                                        "FileSystem" );
                        // Set shader cache path.
                        if( shaderGenerator )
                        {
                            shaderGenerator->setShaderCachePath( shaderCachePath.c_str() );
                        }
                    }
                }

                static const String defaultGrp = "General";

                // load scripts
                // parseScripts( Ogre::GpuProgramManager::getSingletonPtr(), ".cg", defaultGrp );
                //parseScripts( Ogre::GpuProgramManager::getSingletonPtr(), ".hlsl", defaultGrp );
                //parseScripts( Ogre::GpuProgramManager::getSingletonPtr(), ".glsl", defaultGrp );

                parseScripts( Ogre::MaterialManager::getSingletonPtr(), ".program", defaultGrp );
                parseScripts( Ogre::MaterialManager::getSingletonPtr(), ".material", defaultGrp );
                parseScripts( Ogre::CompositorManager::getSingletonPtr(), ".compositor", defaultGrp );
                // parseScripts( Ogre::FontManager::getSingletonPtr(), ".fontdef", defaultGrp );
                //parseScripts( Ogre::OverlayManager::getSingletonPtr(), ".overlay", defaultGrp );
                // parseScripts(Ogre::ParticleSystemManager::getSingletonPtr(), ".pu", defaultGrp);

                // Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                //     "../../../../Media/Terrain", "FileSystem" );
                // Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                //     "../../../../Media/RTShaderLib/Cg", "FileSystem" );
                // Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                //     "../../../../Media/RTShaderLib/HLSL", "FileSystem" );
                // Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                //     "../../../../Media/RTShaderLib/GLSL", "FileSystem" );
                // Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                //     "../../../../Media/ShadowVolume", "FileSystem" );

#if 0
                auto groups = resourceGroupManager->getResourceGroups();
                for( const auto &group : groups )
                {
                    resourceGroupManager->initialiseResourceGroup( group );
                    applicationManager->addLoadProgress( 1 );
                    Thread::yield();
                }
#else
                resourceGroupManager->initialiseAllResourceGroups();
                applicationManager->addLoadProgress( 1 );
#endif

                if( useRTSS )
                {
                    auto defaultMaterialScheme = Ogre::MaterialManager::DEFAULT_SCHEME_NAME;
                    auto defaultShaderScheme = Ogre::RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME;

                    // Create shader based technique from the default technique of the given material.
                    auto materialManager = Ogre::MaterialManager::getSingletonPtr();
                    auto materials = materialManager->getResourceIterator();
                    for( auto it : materials )
                    {
                        auto resource = it.second;
                        auto material = resource.dynamicCast<Ogre::Material>();

                        if( material->getName() == "F40_fbx#3" )
                        {
                            int stop = 0;
                            stop = 0;
                        }

                        auto numTechniques = material->getNumTechniques();
                        if( numTechniques == 0 )
                        {
                            auto technique = material->createTechnique();
                            WP_ASSERT( technique->getNumPasses() > 0 );
                        }

                        if( shaderGenerator )
                        {
                            shaderGenerator->createShaderBasedTechnique(
                                *material, defaultMaterialScheme, defaultShaderScheme, false );
                        }
                    }
                }

                WP_LOG( "Resource loading complete." );

                setLoadingState( LoadingState::Loaded );

                if( auto stateContext = getStateContext() )
                {
                    stateContext->triggerEvent( EventType::Loading, (hash_type)LoadingState::Loaded,
                                                Array<Parameter>(), this, this, nullptr );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_ERROR( "Resource loading error." );
                WP_LOG_EXCEPTION( e );
            }
        }

        void CResourceGroupManager::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );

                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CResourceGroupManager::initialiseAllResourceGroups()
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_resourceGroupManager->initialiseAllResourceGroups();
        }

        void CResourceGroupManager::initialiseResourceGroup( const String &groupName )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_resourceGroupManager->initialiseResourceGroup( groupName.c_str() );
        }

        void CResourceGroupManager::unloadResourceGroup( const String &groupName )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_resourceGroupManager->unloadResourceGroup( groupName.c_str() );
        }

        void CResourceGroupManager::clearResourceGroup( const String &groupName )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_resourceGroupManager->clearResourceGroup( groupName.c_str() );
        }

        void CResourceGroupManager::destroyResourceGroup( const String &groupName )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_resourceGroupManager->destroyResourceGroup( groupName.c_str() );
        }

        void CResourceGroupManager::_getObject( void **ppObject ) const
        {
            *ppObject = m_resourceGroupManager;
        }

        void CResourceGroupManager::reloadResources( const String &groupName )
        {
            String messages;
            m_resourceGroupHelper->checkTimeAndReloadIfNeeded( groupName.c_str(), messages, true );
        }

        void CResourceGroupManager::parseScripts( const Array<String> &scripts )
        {
            parseScripts( Util::createSet( scripts ) );
        }

        void CResourceGroupManager::parseScripts( const Set<String> &scripts )
        {
            String defaultGrp = "General";

            for( auto fileName : scripts )
            {
                try
                {
                    String extension = Path::getFileExtension( fileName );

                    if( extension == ( ".program" ) )
                    {
                        parseScript( Ogre::MaterialManager::getSingletonPtr(), fileName, defaultGrp );
                    }
                    else if( extension == ( ".material" ) )
                    {
                        parseScript( Ogre::MaterialManager::getSingletonPtr(), fileName, defaultGrp );
                    }
                    else if( extension == ( ".compositor" ) )
                    {
                        parseScript( Ogre::CompositorManager::getSingletonPtr(), fileName, defaultGrp );
                    }
                    else if( extension == ( ".fontdef" ) )
                    {
                        parseScript( Ogre::FontManager::getSingletonPtr(), fileName, defaultGrp );
                    }
                    else if( extension == ( ".overlay" ) )
                    {
                        parseScript( Ogre::OverlayManager::getSingletonPtr(), fileName, defaultGrp );
                    }
                    else if( extension == ( ".pu" ) )
                    {
                        // parseScripts(Ogre::ParticleSystemManager::getSingletonPtr(), ".pu",
                        // defaultGrp);
                    }
                }
                catch( std::exception &e )
                {
                    WP_LOG_INFO( e.what() );
                    throw;
                }
                catch( ... )
                {
                    throw;
                }
            }
        }

        void CResourceGroupManager::parseScript( Ogre::ScriptLoader *loader, const String &fileName,
                                                 const String &group )
        {
            try
            {
                WP_ASSERT( loader );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                auto data = fileSystem->open( fileName );
                if( data )
                {
                    auto size = data->size();
                    auto buffer = OGRE_ALLOC_T( u8, size, Ogre::MEMCATEGORY_GENERAL );
                    data->read( buffer, size );

                    Ogre::DataStreamPtr dataStream(
                        new Ogre::MemoryDataStream( fileName.c_str(), buffer, size, true, false ) );
                    loader->parseScript( dataStream, group.c_str() );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_INFO( e.what() );
                throw;
            }
            catch( ... )
            {
                throw;
            }
        }

        void CResourceGroupManager::parseScripts( Ogre::ResourceManager *resMgr, const String &extension,
                                                  const String &group )
        {
            try
            {
                WP_ASSERT( resMgr );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                Array<String> scriptFileNames;
                fileSystem->getFileNamesWithExtension( extension, scriptFileNames );

                auto uniqueScriptFileNames = Util::createSet( scriptFileNames );
                for( auto fileName : uniqueScriptFileNames )
                {
                    auto data = fileSystem->open( fileName, true, false, false, false );
                    if( !data )
                    {
                        data = fileSystem->open( fileName, true, false, false, true );
                    }

                    WP_LOG( fileName );

                    if( StringUtil::contains( fileName, "pbr-vert.glsl" ) )
                    {
                        int a = 0;
                        a++;
                    }

                    if( data )
                    {
                        try
                        {
                            if( data->isOpen() )
                            {
                                auto dataStream = Ogre::DataStreamPtr( new WPOgreDataStream( data ) );
                                resMgr->parseScript( dataStream, group.c_str() );
                            }
                        }
                        catch( Ogre::Exception &e )
                        {
                            WP_LOG_EXCEPTION( e );
                        }
                        catch( std::exception &e )
                        {
                            WP_LOG_EXCEPTION( e );
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CResourceGroupManager::parseScripts( Ogre::ParticleSystemManager *resMgr,
                                                  const String &extension, const String &group )
        {
            try
            {
                WP_ASSERT( resMgr );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                Array<String> scriptFileNames;
                fileSystem->getFileNamesWithExtension( extension, scriptFileNames );

                auto uniqueScriptFileNames = Util::createSet( scriptFileNames );
                for( auto fileName : uniqueScriptFileNames )
                {
                    auto data = fileSystem->open( fileName );
                    if( data )
                    {
                        auto size = data->size();
                        auto buffer = OGRE_ALLOC_T( u8, size, Ogre::MEMCATEGORY_GENERAL );
                        data->read( buffer, size );

                        Ogre::DataStreamPtr dataStream(
                            new Ogre::MemoryDataStream( fileName.c_str(), buffer, size, true, false ) );
                        resMgr->parseScript( dataStream, group.c_str() );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CResourceGroupManager::parseScripts( Ogre::ScriptLoader *loader, const String &extension,
                                                  const String &group )
        {
            try
            {
                WP_ASSERT( loader );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                Array<String> scriptFileNames;
                fileSystem->getFileNamesWithExtension( extension, scriptFileNames );

                auto uniqueScriptFileNames = Util::createSet( scriptFileNames );
                for( auto fileName : uniqueScriptFileNames )
                {
                    auto data = fileSystem->open( fileName );
                    if( data )
                    {
                        auto size = data->size();
                        auto buffer = OGRE_ALLOC_T( u8, size, Ogre::MEMCATEGORY_GENERAL );
                        data->read( buffer, size );

                        Ogre::DataStreamPtr dataStream(
                            new Ogre::MemoryDataStream( fileName.c_str(), buffer, size, true, false ) );
                        loader->parseScript( dataStream, group.c_str() );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        SmartPtr<IStateContext> CResourceGroupManager::getStateContext() const
        {
            return m_stateContext;
        }

        void CResourceGroupManager::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

    }  // end namespace render
}  // namespace workphone
