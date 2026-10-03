#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSceneOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CSceneNodeOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsMeshOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CLightOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CParticleSystem.hpp>
#include <WPGraphicsOgre/Wrapper/CBillboardSetOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CAnimationStateController.hpp>
#include <WPGraphicsOgre/Wrapper/CAnimationTextureControl.hpp>
#include <WPGraphicsOgre/Wrapper/CCameraOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CDynamicLines.hpp>
#include <WPGraphicsOgre/Wrapper/CTerrainOgre.hpp>
#include <WPGraphicsOgre/Addons/OgreUtil.hpp>
#include <WPGraphicsOgre/Wrapper/CDecalCursor.hpp>
#include <WPGraphicsOgre/Addons/LightShafts.hpp>
#include <WPGraphicsOgre/Wrapper/CInstanceManager.hpp>
#include <WPGraphicsOgre/Wrapper/CInstancedObject.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CDynamicMesh.hpp>
#include <WPGraphicsOgre/Wrapper/CWindowOgre.hpp>
#include <WPGraphicsOgre/Addons/DynamicMesh.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreOverlaySystem.h>
#include <OgreSceneManager.h>
#include <OgreShaderGenerator.h>
#include <Ogre.h>
#include <tinyxml.h>

#ifdef WP_OGRE_USE_MESH_SPLITTERS
#    include <DestructibleMeshSplitter.h>
#    include <OgreMeshExtractor.h>
#    include <VertexMerger.h>
#endif

namespace workphone
{
    namespace render
    {

        WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsSceneOgre, IGraphicsScene );
        WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsSceneOgre::MaterialSharedListener,
                                   IEventListener );
        WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsSceneOgre::SceneManagerStateListener,
                                   IStateListener );
        WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsSceneOgre::SkyboxStateListener,
                                   IStateListener );
        WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsSceneOgre::RootSceneNode,
                                   CSceneNodeOgre );

        u32 CGraphicsSceneOgre::m_nextGeneratedNameExt = 0;

        CGraphicsSceneOgre::CGraphicsSceneOgre() :
            m_pRaySceneQuery( nullptr ),
            nextRenderQueueUpdate( 0 ),
            m_isClearing( false )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            auto rootSceneNode = factoryManager->make_ptr<RootSceneNode>();
            rootSceneNode->setCreator( this );
            m_rootSceneNode = rootSceneNode;

            // m_registeredSceneNodes.reserve(500);
            // m_registeredGfxObjects.reserve(500);

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto stateManager = applicationManager->getStateManager();
            auto threadPool = applicationManager->getThreadPool();

            //m_frameListener = fb::make_ptr<SceneManagerFrameListener>( this );
            //graphicsSystem->addFrameListener( m_frameListener );

            m_animationNamePrefix = StringUtil::EmptyString;
            m_animationNameSuffix = StringUtil::EmptyString;

            m_state = workphone::make_ptr<State>();

            auto stateContext = stateManager->addStateContext();
            m_stateContext = stateContext;
            stateContext->addState( m_state );

            auto stateData = workphone::make_ptr<GraphicsSceneState>();
            m_state->setData( stateData );

            auto sceneManagerStateListener = factoryManager->make_ptr<SceneManagerStateListener>();
            sceneManagerStateListener->setOwner( this );
            m_stateListener = sceneManagerStateListener;
            stateContext->addStateListener( sceneManagerStateListener );

            m_state->setStateContext( stateContext );

            auto renderTask = graphicsSystem->getRenderTask();
            // m_state->setTaskId(renderTask);

            m_materialSharedListener = workphone::make_ptr<MaterialSharedListener>();
            m_materialSharedListener->setOwner( this );

            m_skyboxMaterialListener = workphone::make_ptr<SkyboxStateListener>();
            m_skyboxMaterialListener->setOwner( this );
        }

        CGraphicsSceneOgre::CGraphicsSceneOgre( Ogre::SceneManager *sceneManager ) :
            m_pRaySceneQuery( nullptr ),
            nextRenderQueueUpdate( 0 ),
            m_isClearing( false )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            m_sceneManager = sceneManager;

            auto rootSceneNode = factoryManager->make_ptr<RootSceneNode>();
            rootSceneNode->setCreator( this );

            m_rootSceneNode = rootSceneNode;
            m_rootSceneNode->load( nullptr );

            // m_registeredSceneNodes.reserve(500);
            // m_registeredGfxObjects.reserve(500);

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            //m_frameListener = fb::make_ptr<SceneManagerFrameListener>( this );
            //graphicsSystem->addFrameListener( m_frameListener );

            m_animationNamePrefix = StringUtil::EmptyString;
            m_animationNameSuffix = StringUtil::EmptyString;

            m_state = workphone::make_ptr<State>();

            auto stateManager = applicationManager->getStateManager();
            auto threadPool = applicationManager->getThreadPool();

            auto stateContext = stateManager->addStateContext();
            m_stateContext = stateContext;
            stateContext->addState( m_state );

            auto stateData = workphone::make_ptr<GraphicsSceneState>();
            m_state->setData( stateData );

            auto sceneManagerStateListener = factoryManager->make_ptr<SceneManagerStateListener>();
            sceneManagerStateListener->setOwner( this );
            m_stateListener = sceneManagerStateListener;
            stateContext->addStateListener( sceneManagerStateListener );

            m_state->setStateContext( stateContext );

            if( threadPool )
            {
                auto numThreads = threadPool->getNumThreads();
                if( numThreads == 0 )
                {
                    stateContext->setTaskId( TaskId::Primary );
                }
                else
                {
                    stateContext->setTaskId( TaskId::Render );
                }
            }
            else
            {
                stateContext->setTaskId( TaskId::Primary );
            }

            m_materialSharedListener = workphone::make_ptr<MaterialSharedListener>();
            m_materialSharedListener->setOwner( this );

            m_skyboxMaterialListener = workphone::make_ptr<SkyboxStateListener>();
            m_skyboxMaterialListener->setOwner( this );
        }

        CGraphicsSceneOgre::~CGraphicsSceneOgre()
        {
            unload( nullptr );
        }

        void CGraphicsSceneOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                WP_ASSERT( Thread::getTaskFlag( Thread::Render_Flag ) );

                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();

                auto factoryManager = workphone::make_ptr<FactoryManager>();
                WP_ASSERT( factoryManager );

                setFactoryManager( factoryManager );

                FactoryUtil::addFactory<CSceneNodeOgre>( factoryManager );
                FactoryUtil::addFactory<CTerrainOgre>( factoryManager );
                FactoryUtil::addFactory<CWindowOgre>( factoryManager );

                auto root = Ogre::Root::getSingletonPtr();

                WP_ASSERT( root );

                auto name = getName();

                auto type = String( "DefaultSceneManager" );
                auto sceneMgr = root->createSceneManager( type.c_str(), name.c_str() );
                WP_ASSERT( sceneMgr );

                auto overlaySystem = Ogre::OverlaySystem::getSingletonPtr();
                WP_ASSERT( overlaySystem );

                sceneMgr->addRenderQueueListener( overlaySystem );

                sceneMgr->setAmbientLight( Ogre::ColourValue::White * 0.5f );

                m_sceneManager = sceneMgr;

                auto rootNode = sceneMgr->getRootSceneNode();
                auto rootSceneNode = workphone::static_pointer_cast<CSceneNodeOgre>( m_rootSceneNode );
                rootSceneNode->setSceneNode( rootNode );

                auto shaderGenerator = Ogre::RTShader::ShaderGenerator::getSingletonPtr();
                if( shaderGenerator )
                {
                    shaderGenerator->addSceneManager( sceneMgr );
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CGraphicsSceneOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &loadingState = getLoadingState();
                if( loadingState != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    auto stateManager = applicationManager->getStateManager();
                    WP_ASSERT( stateManager );

                    auto graphicsSystem = applicationManager->getGraphicsSystem();
                    WP_ASSERT( graphicsSystem );

                    if( m_materialSharedListener )
                    {
                        if( auto skyboxMaterial = getSkyboxMaterial() )
                        {
                            skyboxMaterial->removeObjectListener( m_materialSharedListener.get() );
                        }

                        m_materialSharedListener->unload( data );
                        m_materialSharedListener = nullptr;
                    }

                    if( m_skyboxMaterialListener )
                    {
                        if( auto skyboxMaterial = getSkyboxMaterial() )
                        {
                            if( auto stateContext = skyboxMaterial->getStateContext() )
                            {
                                stateContext->removeStateListener( m_skyboxMaterialListener );
                            }
                        }

                        m_skyboxMaterialListener->setOwner( nullptr );
                        m_skyboxMaterialListener = nullptr;
                    }

                    m_skyboxMaterial = nullptr;

                    for( auto obj : m_graphicsObjects )
                    {
                        obj->unload( nullptr );
                    }

                    m_graphicsObjects.clear();

                    auto skies = m_skies.snapshot();
                    for( auto sky : skies )
                    {
                        graphicsSystem->unloadObject( sky );
                    }

                    m_skies.clear();

                    auto rootSceneNode =
                        workphone::static_pointer_cast<CSceneNodeOgre>( m_rootSceneNode );
                    if( rootSceneNode )
                    {
                        rootSceneNode->removeChildren();
                    }

                    m_registeredGfxObjects.clear();

                    //WP_ASSERT( getSceneNodes() && getSceneNodes()->empty() );

                    auto sceneNodes = m_sceneNodes.snapshot();
                    for( auto sceneNode : sceneNodes )
                    {
                        sceneNode->detachAllObjects();
                    }

                    for( auto sceneNode : sceneNodes )
                    {
                        sceneNode->unload( nullptr );
                    }

                    if( rootSceneNode )
                    {
                        rootSceneNode->removeChildren();
                    }

                    m_sceneNodes.clear();

                    if( rootSceneNode )
                    {
                        rootSceneNode->unload( nullptr );
                        rootSceneNode = nullptr;
                        m_rootSceneNode = nullptr;
                    }

                    if( m_sceneManager )
                    {
                        m_sceneManager->clearScene();
                        m_sceneManager = nullptr;
                    }

                    //graphicsSystem->removeFrameListener( m_frameListener );

                    if( auto stateContext = getStateContext() )
                    {
                        if( auto stateListener = getStateListener() )
                        {
                            stateContext->removeStateListener( stateListener );
                        }

                        stateManager->removeStateContext( stateContext );

                        stateContext->unload( nullptr );
                        setStateContext( nullptr );
                    }

                    if( auto stateListener = getStateListener() )
                    {
                        stateListener->unload( nullptr );
                        setStateListener( nullptr );
                    }

                    GraphicsScene::unload( data );

                    setLoadingState( LoadingState::Unloaded );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CGraphicsSceneOgre::update()
        {
            auto activeCamera = getActiveCamera();
            if( !activeCamera )
            {
                for( auto object : m_graphicsObjects )
                {
                    if( object )
                    {
                        if( object->isDerived<IGraphicsCamera>() )
                        {
                            auto camera = workphone::static_pointer_cast<IGraphicsCamera>( object );
                            camera->setVisible( true );
                            setActiveCamera( activeCamera );
                        }
                    }
                }
            }

            for( auto terrain : m_terrains )
            {
                if( terrain )
                {
                    terrain->update();
                }
            }
        }

        SmartPtr<IMaterial> CGraphicsSceneOgre::getSkyboxMaterial() const
        {
            return m_skyboxMaterial;
        }

        void CGraphicsSceneOgre::setSkyboxMaterial( SmartPtr<IMaterial> skyboxMaterial )
        {
            m_skyboxMaterial = skyboxMaterial;
        }

        void CGraphicsSceneOgre::lock()
        {
            m_mutex.lock();
        }

        bool CGraphicsSceneOgre::try_lock()
        {
            return m_mutex.try_lock();
        }

        void CGraphicsSceneOgre::unlock()
        {
            m_mutex.unlock();
        }

        SmartPtr<ISharedObject> CGraphicsSceneOgre::addGraphicsObject( const String &name,
                                                                       const String &type )
        {
            if( type == ( "Billboard" ) )
            {
                //return addBillboardSet( name );
            }
            if( type == ( "Camera" ) )
            {
                //return addCamera( name );
            }
            if( type == ( "DynamicLines" ) )
            {
                auto dynamicLines = SmartPtr<CDynamicLines>( new CDynamicLines );
                dynamicLines->initialise();
                return dynamicLines;
            }
            if( type == ( "DynamicMesh" ) )
            {
                // return addDynamicMesh(name);
            }
            else if( type == ( "Light" ) )
            {
                //return addLight( name );
            }
            else if( type == ( "ParticleSystem" ) )
            {
                // return addParticleSystem(name);
            }
            else
            {
                WP_EXCEPTION( "Could not create graphics object." );
            }

            return nullptr;
        }

        SmartPtr<ISharedObject> CGraphicsSceneOgre::addGraphicsObject( const String &type )
        {
            String name = getUniqueName( type );
            return addGraphicsObject( name, type );
        }

        SmartPtr<ISharedObject> CGraphicsSceneOgre::addGraphicsObjectByTypeId( hash_type id )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();

            auto object = SmartPtr<IGraphicsObject>();

            if( id == IDynamicMesh::typeInfo() )
            {
                object = workphone::make_ptr<CDynamicMesh>();
            }
            else if( id == IParticleSystem::typeInfo() )
            {
                //object = workphone::make_ptr<PUParticleSystem>();
                //return fb::make_ptr<CParticleSystem>();
            }
            else if( id == IBillboardSet::typeInfo() )
            {
                //return lioncat::make_ptr<CBillboardSetOgre>();
            }
            else if( id == IGraphicsCamera::typeInfo() )
            {
                object = workphone::make_ptr<CCameraOgre>();
            }
            else if( id == IGraphicsLight::typeInfo() )
            {
                object = workphone::make_ptr<CLightOgre>();
            }
            else if( id == IDynamicLines::typeInfo() )
            {
                object = workphone::make_ptr<CDynamicLines>();
            }
            else
            {
                return GraphicsScene::addGraphicsObjectByTypeId( id );
            }

            if( object )
            {
                object->setCreator( this );
                graphicsSystem->loadObject( object );
            }

            return object;
        }

        void CGraphicsSceneOgre::clear()
        {
            try
            {
                ScopedLock lock( this );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                m_isClearing = true;

                m_registeredGfxObjects.clear();

                auto graphicsObjects = m_graphicsObjects.snapshot();
                for( auto object : graphicsObjects )
                {
                    if( object )
                    {
                        graphicsSystem->unloadObject( object );
                    }
                }

                m_graphicsObjects.clear();

                auto skies = m_skies.snapshot();
                for( auto sky : skies )
                {
                    if( sky )
                    {
                        graphicsSystem->unloadObject( sky );
                    }
                }

                m_skies.clear();

                auto sceneNodes = m_sceneNodes.snapshot();
                for( auto sceneNode : sceneNodes )
                {
                    sceneNode->detachAllObjects();
                }

                m_rootSceneNode->removeChildren();

                m_sceneNodes.clear();

                if( m_sceneManager )
                {
                    m_sceneManager->clearScene();
                }

                m_isClearing = false;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        bool CGraphicsSceneOgre::hasAnimation( const String &animationName )
        {
            ScopedLock lock( this );
            return m_sceneManager->hasAnimation( animationName.c_str() );
        }

        bool CGraphicsSceneOgre::destroyAnimation( const String &animationName )
        {
            ScopedLock lock( this );

            if( m_sceneManager->hasAnimation( animationName.c_str() ) )
            {
                m_sceneManager->destroyAnimation( animationName.c_str() );
                return true;
            }

            return false;
        }

        bool CGraphicsSceneOgre::validateGfxObjName( const String &name ) const
        {
            if( name.length() == 0 )
            {
                return false;
            }

            for( auto object : m_graphicsObjects )
            {
                if( object->getName() == name )
                {
                    return false;
                }
            }

            return true;
        }

        SmartPtr<IGraphicsSceneNode> CGraphicsSceneOgre::getRootSceneNode() const
        {
            return m_rootSceneNode;
        }

        SmartPtr<IGraphicsMesh> CGraphicsSceneOgre::addMesh( const String &name, const String &meshName )
        {
            try
            {
#ifndef _FINAL_
                validateGfxObjName( name );
#endif

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto graphicsObject = factoryManager->make_ptr<CGraphicsMeshOgre>();
                WP_ASSERT( graphicsObject );

                auto handle = graphicsObject->getHandle();
                WP_ASSERT( handle );

                graphicsObject->setName( name );

                graphicsObject->setCreator( this );
                graphicsObject->setMeshName( meshName );

                _addGraphicsObject( graphicsObject );

                graphicsSystem->loadObject( graphicsObject );

                return graphicsObject;
            }
            catch( Ogre::Exception &e )
            {
                WP_LOG_ERROR( e.getFullDescription().c_str() );
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IGraphicsMesh> CGraphicsSceneOgre::addMesh( const String &meshName )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto graphicsObject = factoryManager->make_ptr<CGraphicsMeshOgre>();
                WP_ASSERT( graphicsObject );

                auto name = getUniqueName( "Mesh" );
                graphicsObject->setName( name );

                graphicsObject->setCreator( this );
                graphicsObject->setMeshName( meshName );

                _addGraphicsObject( graphicsObject );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                graphicsSystem->loadObject( graphicsObject );

                return graphicsObject;
            }
            catch( Ogre::Exception &e )
            {
                WP_LOG_ERROR( e.getFullDescription().c_str() );
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        SmartPtr<IGraphicsMesh> CGraphicsSceneOgre::getMesh( const String &name ) const
        {
            for( auto &graphicsObject : m_graphicsObjects )
            {
                auto handleName = graphicsObject->getName();
                if( handleName == name )
                {
                    return graphicsObject;
                }
            }

            return nullptr;
        }

        SmartPtr<IParticleSystem> CGraphicsSceneOgre::addParticleSystem( const String &name,
                                                                         const String &templateName )
        {
#ifndef _FINAL_
            validateGfxObjName( name );
#endif

#if WP_OGRE_USE_PARTICLE_UNIVERSE
            SmartPtr<PUParticleSystem> particleSystem;  // (new PUParticleSystem(this), true);
            particleSystem->setName( name );
            particleSystem->setTemplateName( templateName );
            // m_graphicsObjects[name] = particleSystem.get();

            //return particleSystem;
#endif

            return nullptr;
        }

        SmartPtr<IParticleSystem> CGraphicsSceneOgre::getParticleSystem( const String &name ) const
        {
            // GraphicsObjects::const_iterator it = m_graphicsObjects.find(name);
            // if( it != m_graphicsObjects.end() )
            //{
            //	return it->second;
            // }

            return nullptr;
        }

        SmartPtr<IAnimationStateController> CGraphicsSceneOgre::createAnimationStateController()
        {
            SmartPtr<IAnimationStateController>
                animStateCtrl;  // (new CAnimationStateController(this), true);
            return animStateCtrl;
        }

        SmartPtr<IAnimationTextureControl> CGraphicsSceneOgre::createAnimationTextureCtrl(
            SmartPtr<IMaterialTexture> textureUnit, bool clone, const String &clonedMaterialName )
        {
            Ogre::TextureUnitState *textureUnitState = nullptr;
            // textureUnit->_getObject((void**)&textureUnitState);

            auto textureCtrl = new CAnimationTextureControl;
            textureCtrl->initialise( textureUnitState );

            SmartPtr<IAnimationTextureControl> animTextureCtrl;  // (textureCtrl, true);
            return animTextureCtrl;
        }

        bool CGraphicsSceneOgre::removeGraphicsObject( SmartPtr<ISharedObject> graphicsObject )
        {
            if( m_isClearing )
            {
                return false;
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            if( graphicsObject )
            {
                auto it =
                    std::find( m_graphicsObjects.begin(), m_graphicsObjects.end(), graphicsObject );
                if( it != m_graphicsObjects.end() )
                {
                    graphicsSystem->unloadObject( graphicsObject );
                    m_graphicsObjects.erase( it );
                    return true;
                }

                return GraphicsScene::removeGraphicsObject( graphicsObject );
            }

            return false;
        }

        Array<SmartPtr<IGraphicsObject>> CGraphicsSceneOgre::getGraphicsObjects() const
        {
            return m_graphicsObjects.snapshot();
        }

        void CGraphicsSceneOgre::setSkyBox( bool enable, SmartPtr<IMaterial> material, f32 distance,
                                            bool drawFirst )
        {
            try
            {
                if( auto stateContext = getStateContext() )
                {
                    if( auto state = stateContext->invalidateStateData<GraphicsSceneState>() )
                    {
                        state->enableSkybox = enable;
                    }
                }

                auto skyboxMaterial = getSkyboxMaterial();
                if( skyboxMaterial != material )
                {
                    if( skyboxMaterial )
                    {
                        skyboxMaterial->removeObjectListener( m_materialSharedListener.get() );

                        if( auto stateContext = skyboxMaterial->getStateContext() )
                        {
                            stateContext->removeStateListener( m_skyboxMaterialListener );
                        }
                    }

                    setSkyboxMaterial( material );

                    if( material )
                    {
                        material->addObjectListener( m_materialSharedListener );

                        if( auto stateContext = material->getStateContext() )
                        {
                            stateContext->addStateListener( m_skyboxMaterialListener );
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void CGraphicsSceneOgre::setFog( u32 fogMode, const ColourF &colour, f32 expDensity,
                                         f32 linearStart, f32 linearEnd )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );
            m_sceneManager->setFog( static_cast<Ogre::FogMode>( fogMode ),
                                    Ogre::ColourValue( colour.r, colour.g, colour.b, colour.a ),
                                    expDensity, linearStart, linearEnd );
        }

        void CGraphicsSceneOgre::registerForUpdates( SmartPtr<IGraphicsObject> gfxObject )
        {
            ScopedLock lock( this );
            // m_registeredGfxObjects.push_back(gfxObject.get());
        }

        bool CGraphicsSceneOgre::unregisteredForUpdates( SmartPtr<IGraphicsObject> gfxObject )
        {
            ScopedLock lock( this );
            // return m_registeredGfxObjects.erase_element(gfxObject.get());

            return false;
        }

        bool CGraphicsSceneOgre::unregisteredForUpdates( IGraphicsObject *gfxObject )
        {
            ScopedLock lock( this );
            // return m_registeredGfxObjects.erase_element(gfxObject);

            return false;
        }

        Ogre::SceneManager *CGraphicsSceneOgre::getSceneManager() const
        {
            return m_sceneManager;
        }

        void CGraphicsSceneOgre::_getObject( void **ppObject ) const
        {
            *ppObject = m_sceneManager;
        }

        void CGraphicsSceneOgre::clearQueues()
        {
            EntityDeleteQueue entityDeleteQueue;

            {
                ScopedLock lock( this );
                entityDeleteQueue = m_entityDeleteQueue;
                m_entityDeleteQueue.clear();
            }

            SceneNodeDeleteQueue sceneNodeDeleteQueue;

            {
                ScopedLock lock( this );
                sceneNodeDeleteQueue = m_sceneNodeDeleteQueue;
                m_sceneNodeDeleteQueue.clear();
            }

            ParticleSysDeleteQueue particleSysDeleteQueue;

            {
                ScopedLock lock( this );
                particleSysDeleteQueue = m_particleSysDeleteQueue;
                m_particleSysDeleteQueue.clear();
            }

            for( u32 i = 0; i < entityDeleteQueue.size(); ++i )
            {
                Ogre::Entity *entity = entityDeleteQueue[i];
                m_sceneManager->destroyEntity( entity );
            }

            for( u32 i = 0; i < sceneNodeDeleteQueue.size(); ++i )
            {
                Ogre::SceneNode *sceneNode = sceneNodeDeleteQueue[i];
                m_sceneManager->destroySceneNode( sceneNode->getName() );
            }

            for( u32 i = 0; i < particleSysDeleteQueue.size(); ++i )
            {
                Ogre::ParticleSystem *particleSystem = particleSysDeleteQueue[i];
                m_sceneManager->destroyParticleSystem( particleSystem );
            }
        }

        String CGraphicsSceneOgre::getUniqueName( const String &baseName ) const
        {
            return baseName + StringUtil::toString( m_nextGeneratedNameExt++ );
        }

        SmartPtr<IGraphicsTerrain> CGraphicsSceneOgre::createTerrain()
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto terrain = factoryManager->make_ptr<CTerrainOgre>();
            WP_ASSERT( terrain );

            terrain->setSceneManager( this );

            ScopedLock lock( this );
            m_terrains.push_back( terrain );

            graphicsSystem->loadObject( terrain );

            return terrain;
        }

        void CGraphicsSceneOgre::destroyTerrain( SmartPtr<IGraphicsTerrain> terrain )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            if( terrain )
            {
                graphicsSystem->unloadObject( terrain );

                ScopedLock lock( this );
                m_terrains.erase( std::remove( m_terrains.begin(), m_terrains.end(), terrain ),
                                  m_terrains.end() );
            }
        }

        SmartPtr<IDecalCursor> CGraphicsSceneOgre::addDecalCursor( const String &terrainMaterial,
                                                                   const String &decalTextureName,
                                                                   const Vector2F &size )
        {
            auto decalCursor = SmartPtr<CDecalCursor>( new CDecalCursor );
            // decalCursor->initialise(this, terrainMaterial, decalTextureName, size);
            return decalCursor;
        }

        // raycast from a point in to the scene.
        // returns success or failure.
        // on success the point is returned in the result.
        bool CGraphicsSceneOgre::castRay( const Ray3F &ray, Vector3F &result )
        {
            // create the ray to test
            Vector3F start = ray.getOrigin();
            Vector3F normal = ray.getDirection();

            Ogre::Ray ogreRay( Ogre::Vector3( start.X(), start.Y(), start.Z() ),
                               Ogre::Vector3( normal.X(), normal.Y(), normal.Z() ) );

            if( !m_pRaySceneQuery )
            {
                // create the ray scene query object
                m_pRaySceneQuery = m_sceneManager->createRayQuery(
                    Ogre::Ray(), Ogre::SceneManager::WORLD_GEOMETRY_TYPE_MASK );
                if( nullptr == m_pRaySceneQuery )
                {
                    WP_LOG_INFO( "Failed to create Ogre::RaySceneQuery instance" );
                    return ( false );
                }

                m_pRaySceneQuery->setSortByDistance( true );
            }

            // check we are initialised
            if( m_pRaySceneQuery != nullptr )
            {
                // create a query object
                m_pRaySceneQuery->setRay( ogreRay );

                // execute the query, returns a vector of hits
                if( m_pRaySceneQuery->execute().size() <= 0 )
                {
                    // raycast did not hit an objects bounding box
                    return ( false );
                }
            }
            else
            {
                WP_LOG_INFO( "Cannot raycast without RaySceneQuery instance" );
                return ( false );
            }

            // at this point we have raycast to a series of different objects bounding boxes.
            // we need to test these different objects to see which is the first polygon hit.
            // there are some minor optimizations (distance based) that mean we wont have to
            // check all of the objects most of the time, but the worst case scenario is that
            // we need to test every triangle of every object.
            Ogre::Real closest_distance = -1.0f;
            Ogre::Vector3 closest_result;
            Ogre::RaySceneQueryResult &query_result = m_pRaySceneQuery->getLastResults();
            for( size_t qr_idx = 0; qr_idx < query_result.size(); qr_idx++ )
            {
                // stop checking if we have found a raycast hit that is closer
                // than all remaining entities
                if( ( closest_distance >= 0.0f ) &&
                    ( closest_distance < query_result[qr_idx].distance ) )
                {
                    break;
                }

                // only check this result if its a hit against an entity
                if( ( query_result[qr_idx].movable != nullptr ) &&
                    ( query_result[qr_idx].movable->getMovableType().compare( "Entity" ) == 0 ) )
                {
                    // get the entity to check
                    auto pentity = static_cast<Ogre::Entity *>( query_result[qr_idx].movable );

                    // mesh data to retrieve
                    size_t vertex_count;
                    size_t index_count;
                    Ogre::Vector3 *vertices;
                    unsigned long *indices;

                    // get the mesh information
                    getMeshInformation( pentity->getMesh(), vertex_count, vertices, index_count, indices,
                                        pentity->getParentNode()->_getDerivedPosition(),
                                        pentity->getParentNode()->_getDerivedOrientation(),
                                        pentity->getParentNode()->_getDerivedScale() );

                    // test for hitting individual triangles on the mesh
                    bool new_closest_found = false;
                    for( int i = 0; i < static_cast<int>( index_count ); i += 3 )
                    {
                        // check for a hit against this triangle
                        std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(
                            ogreRay, vertices[indices[i]], vertices[indices[i + 1]],
                            vertices[indices[i + 2]], true, false );

                        // if it was a hit check if its the closest
                        if( hit.first )
                        {
                            if( ( closest_distance < 0.0f ) || ( hit.second < closest_distance ) )
                            {
                                // this is the closest so far, save it off
                                closest_distance = hit.second;
                                new_closest_found = true;
                            }
                        }
                    }

                    // free the verticies and indicies memory
                    delete[] vertices;
                    delete[] indices;

                    // if we found a new closest raycast for this object, update the
                    // closest_result before moving on to the next object.
                    if( new_closest_found )
                    {
                        closest_result = ogreRay.getPoint( closest_distance );
                    }
                }
            }

            // return the result
            if( closest_distance >= 0.0f )
            {
                // raycast success
                result = Vector3F( closest_result.x, closest_result.y, closest_result.z );
                return ( true );
            }
            // raycast failed
            return ( false );
        }

        // Get the mesh information for the given mesh.
        // Code found in Wiki: www.ogre3d.org/wiki/index.php/RetrieveVertexData
        void CGraphicsSceneOgre::getMeshInformation( const Ogre::MeshPtr mesh, size_t &vertex_count,
                                                     Ogre::Vector3 *&vertices, size_t &index_count,
                                                     unsigned long *&indices,
                                                     const Ogre::Vector3 &position,
                                                     const Ogre::Quaternion &orient,
                                                     const Ogre::Vector3 &scale )
        {
            bool added_shared = false;
            size_t current_offset = 0;
            size_t shared_offset = 0;
            size_t next_offset = 0;
            size_t index_offset = 0;

            vertex_count = index_count = 0;

            // Calculate how many vertices and indices we're going to need
            for( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i )
            {
                Ogre::SubMesh *submesh = mesh->getSubMesh( i );

                // We only need to add the shared vertices once
                if( submesh->useSharedVertices )
                {
                    if( !added_shared )
                    {
                        vertex_count += mesh->sharedVertexData->vertexCount;
                        added_shared = true;
                    }
                }
                else
                {
                    vertex_count += submesh->vertexData->vertexCount;
                }

                // Add the indices
                index_count += submesh->indexData->indexCount;
            }

            // Allocate space for the vertices and indices
            vertices = new Ogre::Vector3[vertex_count];
            indices = new unsigned long[index_count];

            added_shared = false;

            // Run through the submeshes again, adding the data into the arrays
            for( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i )
            {
                Ogre::SubMesh *submesh = mesh->getSubMesh( i );

                Ogre::VertexData *vertex_data =
                    submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

                if( ( !submesh->useSharedVertices ) || ( submesh->useSharedVertices && !added_shared ) )
                {
                    if( submesh->useSharedVertices )
                    {
                        added_shared = true;
                        shared_offset = current_offset;
                    }

                    const Ogre::VertexElement *posElem =
                        vertex_data->vertexDeclaration->findElementBySemantic( Ogre::VES_POSITION );

                    Ogre::HardwareVertexBufferSharedPtr vbuf =
                        vertex_data->vertexBufferBinding->getBuffer( posElem->getSource() );

                    auto vertex = static_cast<unsigned char *>(
                        vbuf->lock( Ogre::HardwareBuffer::HBL_READ_ONLY ) );

                    // There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
                    //  as second argument. So make it float, to avoid trouble when Ogre::Real will
                    //  be comiled/typedefed as double:
                    //      Ogre::Real* pReal;
                    float *pReal;

                    for( size_t j = 0; j < vertex_data->vertexCount;
                         ++j, vertex += vbuf->getVertexSize() )
                    {
                        posElem->baseVertexPointerToElement( vertex, &pReal );

                        Ogre::Vector3 pt( pReal[0], pReal[1], pReal[2] );

                        vertices[current_offset + j] = ( orient * ( pt * scale ) ) + position;
                    }

                    vbuf->unlock();
                    next_offset += vertex_data->vertexCount;
                }

                Ogre::IndexData *index_data = submesh->indexData;
                size_t numTris = index_data->indexCount / 3;
                Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;
                if( ibuf.isNull() )
                    continue;  // need to check if index buffer is valid (which will be not if the mesh
                // doesn't have triangles like a pointcloud)

                bool use32bitindexes = ( ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT );

                auto pLong =
                    static_cast<unsigned long *>( ibuf->lock( Ogre::HardwareBuffer::HBL_READ_ONLY ) );
                auto pShort = reinterpret_cast<unsigned short *>( pLong );

                size_t offset = ( submesh->useSharedVertices ) ? shared_offset : current_offset;
                size_t index_start = index_data->indexStart;
                size_t last_index = numTris * 3 + index_start;

                if( use32bitindexes )
                    for( size_t k = index_start; k < last_index; ++k )
                    {
                        indices[index_offset++] = pLong[k] + static_cast<unsigned long>( offset );
                    }

                else
                    for( size_t k = index_start; k < last_index; ++k )
                    {
                        indices[index_offset++] = static_cast<unsigned long>( pShort[k] ) +
                                                  static_cast<unsigned long>( offset );
                    }

                ibuf->unlock();
                current_offset = next_offset;
            }
        }

        void CGraphicsSceneOgre::getMeshInformation( const Ogre::Entity *entity, size_t &vertex_count,
                                                     Ogre::Vector3 *&vertices, size_t &index_count,
                                                     unsigned long *&indices,
                                                     const Ogre::Vector3 &position,
                                                     const Ogre::Quaternion &orient,
                                                     const Ogre::Vector3 &scale )
        {
            bool added_shared = false;
            size_t current_offset = 0;
            size_t shared_offset = 0;
            size_t next_offset = 0;
            size_t index_offset = 0;
            vertex_count = index_count = 0;

            Ogre::MeshPtr mesh = entity->getMesh();

            bool useSoftwareBlendingVertices = entity->hasSkeleton();

            if( useSoftwareBlendingVertices )
            {
                // entity->_updateAnimation();
            }

            // Calculate how many vertices and indices we're going to need
            for( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i )
            {
                Ogre::SubMesh *submesh = mesh->getSubMesh( i );

                // We only need to add the shared vertices once
                if( submesh->useSharedVertices )
                {
                    if( !added_shared )
                    {
                        vertex_count += mesh->sharedVertexData->vertexCount;
                        added_shared = true;
                    }
                }
                else
                {
                    vertex_count += submesh->vertexData->vertexCount;
                }

                // Add the indices
                index_count += submesh->indexData->indexCount;
            }

            // Allocate space for the vertices and indices
            vertices = new Ogre::Vector3[vertex_count];
            indices = new unsigned long[index_count];

            added_shared = false;

            // Run through the submeshes again, adding the data into the arrays
            for( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i )
            {
                Ogre::SubMesh *submesh = mesh->getSubMesh( i );

                // GET VERTEXDATA

                // Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData :
                // submesh->vertexData;
                Ogre::VertexData *vertex_data;

                // When there is animation:
                if( useSoftwareBlendingVertices )
                    vertex_data = submesh->useSharedVertices
                                      ? entity->_getSkelAnimVertexData()
                                      : entity->getSubEntity( i )->_getSkelAnimVertexData();
                else
                    vertex_data =
                        submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

                if( ( !submesh->useSharedVertices ) || ( submesh->useSharedVertices && !added_shared ) )
                {
                    if( submesh->useSharedVertices )
                    {
                        added_shared = true;
                        shared_offset = current_offset;
                    }

                    const Ogre::VertexElement *posElem =
                        vertex_data->vertexDeclaration->findElementBySemantic( Ogre::VES_POSITION );

                    Ogre::HardwareVertexBufferSharedPtr vbuf =
                        vertex_data->vertexBufferBinding->getBuffer( posElem->getSource() );

                    auto vertex = static_cast<unsigned char *>(
                        vbuf->lock( Ogre::HardwareBuffer::HBL_READ_ONLY ) );

                    // There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
                    //  as second argument. So make it float, to avoid trouble when Ogre::Real will
                    //  be comiled/typedefed as double:
                    //      Ogre::Real* pReal;
                    float *pReal;

                    for( size_t j = 0; j < vertex_data->vertexCount;
                         ++j, vertex += vbuf->getVertexSize() )
                    {
                        posElem->baseVertexPointerToElement( vertex, &pReal );

                        Ogre::Vector3 pt( pReal[0], pReal[1], pReal[2] );

                        vertices[current_offset + j] = ( orient * ( pt * scale ) ) + position;
                    }

                    vbuf->unlock();
                    next_offset += vertex_data->vertexCount;
                }

                Ogre::IndexData *index_data = submesh->indexData;
                size_t numTris = index_data->indexCount / 3;
                Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

                bool use32bitindexes = ( ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT );

                auto pLong =
                    static_cast<unsigned long *>( ibuf->lock( Ogre::HardwareBuffer::HBL_READ_ONLY ) );
                auto pShort = reinterpret_cast<unsigned short *>( pLong );

                size_t offset = ( submesh->useSharedVertices ) ? shared_offset : current_offset;
                size_t index_start = index_data->indexStart;
                size_t last_index = numTris * 3 + index_start;

                if( use32bitindexes )
                    for( size_t k = index_start; k < last_index; ++k )
                    {
                        indices[index_offset++] = pLong[k] + static_cast<unsigned long>( offset );
                    }

                else
                    for( size_t k = index_start; k < last_index; ++k )
                    {
                        indices[index_offset++] = static_cast<unsigned long>( pShort[k] ) +
                                                  static_cast<unsigned long>( offset );
                    }

                ibuf->unlock();
                current_offset = next_offset;
            }
        }

        unsigned int getMeshByIndex( const Array<unsigned int> &viIndexOffsets,
                                     unsigned int iIndexIndex )
        {
            // subset search
            auto iRangeMin = 0;
            auto iRangeMax = static_cast<s32>( viIndexOffsets.size() ) - 2;
            while( iRangeMin != iRangeMax )
            {
                auto iSamplePoint = ( iRangeMax + iRangeMin ) >> 1;
                if( iIndexIndex < viIndexOffsets[iSamplePoint + 1] )
                    iRangeMax = iSamplePoint;
                else  //>=
                    iRangeMin = iSamplePoint + 1;
            }
            return iRangeMax;
        }

        Array<SmartPtr<IGraphicsMesh>> CGraphicsSceneOgre::splitMesh(
            SmartPtr<IGraphicsMesh> mesh, const SmartPtr<Properties> &properties )
        {
            Array<SmartPtr<IGraphicsMesh>> meshes;

#ifdef WP_OGRE_USE_MESH_SPLITTERS
            Ogre::Entity *entity = nullptr;
            mesh->_getObject( (void **)&entity );

            Ogre::MeshPtr inMesh = entity->getMesh();

            std::string strSourceName = inMesh->getName();
            std::string strDestFolder = "../media/meshsplitoutput/";

            float fMaxSize = 1.0f;
            float fRoughness = 0.0f;
            float fResolution = 256;
            bool bSmooth = true;
            u32 nRecoveryAttempts = 100;
            bool bCutSurface = true;
            String strCutMaterial = "BaseWhiteMaterial";

            if( properties )
            {
                if( !properties->getPropertyValue( "maxSize", fMaxSize ) )
                    LOG_MESSAGE( "Graphics", "Warning: no \"maxSize\" property found. " );

                if( !properties->getPropertyValue( "roughness", fRoughness ) )
                    LOG_MESSAGE( "Graphics", "Warning: no \"roughness\" property found. " );

                if( !properties->getPropertyValue( "recoveryAttempts", nRecoveryAttempts ) )
                    LOG_MESSAGE( "Graphics", "Warning: no \"recoveryAttempts\" property found. " );

                if( !properties->getPropertyValue( "cutMaterial", strCutMaterial ) )
                    LOG_MESSAGE( "Graphics", "Warning: no \"cutMaterial\" property found. " );
            }

            Array<Ogre::MeshPtr> vSplinters = DestructibleMeshSplitter::SplitMesh(
                inMesh, fMaxSize, fRoughness, fResolution, bSmooth, nRecoveryAttempts, bCutSurface,
                strCutMaterial.c_str() );

            // find out which fragments are connected
            Array<Ogre::Vector3> vOverallVertices;
            Array<unsigned int> viOverallVertexOffsets, viOverallMatchingVertices;

            Ogre::MeshSerializer *pSerializer = new Ogre::MeshSerializer();
            Array<Ogre::String> vstrFiles;

            for( unsigned int iSplinter = 0; iSplinter < vSplinters.size(); iSplinter++ )
            {
                // write fragment to disk
                Ogre::String strFileName = strSourceName + Ogre::String( "_" ) +
                                           Ogre::StringConverter::toString( iSplinter ) +
                                           Ogre::String( ".mesh" );
                printf( "writing mesh fragment %s\n", strFileName.c_str() );
                pSerializer->exportMesh( vSplinters[iSplinter].get(), strDestFolder + strFileName );
                vstrFiles.push_back( strFileName );

                Array<Ogre::Vector3> vVertices;
                Array<unsigned int> viSubmeshVertexOffsets, viIndices, viSubmeshIndexOffsets,
                    viMatchingVertices;

                OgreMeshExtractor::Extract( vSplinters[iSplinter], vVertices, viSubmeshVertexOffsets,
                                            viIndices, viSubmeshIndexOffsets );

                DestructibleMeshSplitter::unloadMesh( vSplinters[iSplinter] );

                viOverallVertexOffsets.push_back( vOverallVertices.size() );
                vOverallVertices.insert( vOverallVertices.end(), vVertices.begin(), vVertices.end() );
            }
            delete pSerializer;

            viOverallMatchingVertices = VertexMerger::GetMatchingIndices( vOverallVertices );
            viOverallVertexOffsets.push_back( vOverallVertices.size() );

            Array<std::map<unsigned int, std::set<int>>> vmConnections( vSplinters.size() );
            for( unsigned int iMesh = 0; iMesh < vSplinters.size(); iMesh++ )
            {
                for( unsigned int iVert = viOverallVertexOffsets[iMesh];
                     iVert < viOverallVertexOffsets[iMesh + 1]; iVert++ )
                {
                    // get sub-mesh
                    int iMatchingVert = viOverallMatchingVertices[iVert];
                    unsigned int iOtherMesh = getMeshByIndex( viOverallVertexOffsets, iMatchingVert );
                    if( iOtherMesh != iMesh )
                    {
                        std::map<unsigned int, std::set<int>>::iterator it;
                        if( ( it = vmConnections[iMesh].find( iOtherMesh ) ) !=
                            vmConnections[iMesh].end() )
                            it->second.insert( iMatchingVert );
                        else
                        {
                            std::set<int> mySet;
                            mySet.insert( iMatchingVert );
                            vmConnections[iMesh][iOtherMesh] = mySet;
                        }
                    }
                }
            }
            // write XML output
            std::string strXMLFile = strDestFolder + strSourceName + Ogre::String( ".xml" );
            printf( "writing XML file to %s\n", strXMLFile.c_str() );
            std::ofstream ofs( strXMLFile.c_str() );
            ofs << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n";
            ofs << "<mesh_fragments>\n";
            for( unsigned int iFragment = 0; iFragment < vmConnections.size(); iFragment++ )
            {
                ofs << "\t<mesh name=\"" << vstrFiles[iFragment].c_str() << "\">\n";
                //;
                for( std::map<unsigned int, std::set<int>>::iterator it =
                         vmConnections[iFragment].begin();
                     it != vmConnections[iFragment].end(); it++ )
                {
                    ofs << "\t\t<connection target=\"" << vstrFiles[it->first].c_str() << "\">\n";
                    const std::set<int> &siVertices = it->second;

                    Ogre::Vector3 vMin( 1.0e+10f, 1.0e+10f, 1.0e+10f ),
                        vMax( -1.0e+10f, -1.0e+10f, -1.0e+10f );
                    for( std::set<int>::const_iterator itVert = siVertices.begin();
                         itVert != siVertices.end(); itVert++ )
                    {
                        Ogre::Vector3 v = vOverallVertices[*itVert];
                        vMin.makeFloor( v );
                        vMax.makeCeil( v );
                    }
                    float fSize = ( vMax - vMin ).length();
                    ofs << "\t\t\t<rel_force value=\"" << fSize * fSize << "\"/>\n";
                    ofs << "\t\t\t<rel_torque value=\"" << fSize * fSize * fSize << "\"/>\n";
                    Ogre::Vector3 vJointCenter = vMin + ( vMax - vMin ) * .5f;
                    ofs << "\t\t\t<joint_center x=\"" << vJointCenter.x << "\" y=\"" << vJointCenter.y
                        << "\" z=\"" << vJointCenter.z << "\"/>\n";

                    ofs << "\t\t</connection>\n";
                }
                ofs << "\t</mesh>\n";
            }
            ofs << "</mesh_fragments>";
            ofs.close();

            for( size_t i = 0; i < vSplinters.size(); ++i )
            {
                Ogre::MeshPtr mesh_ = vSplinters[i];
                mesh_->load();

                Ogre::Entity *entity_ = m_sceneManager->createEntity( mesh_ );

                String name = entity_->getName().c_str();
                SmartPtr<CGraphicsMeshOgre> graphicsObject( new CGraphicsMeshOgre( this ), this );
                graphicsObject->initialise( entity );
                graphicsObject->getHandle()->setName( name );
                m_graphicsObjects[name] = graphicsObject.get();
                meshes.push_back( graphicsObject );
            }
#endif

            return meshes;
        }

        void CGraphicsSceneOgre::addExistingGraphicsObject( SmartPtr<IGraphicsObject> graphicsObject )
        {
            m_graphicsObjects.push_back( graphicsObject );
        }

        void CGraphicsSceneOgre::addExistingSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
        {
            m_sceneNodes.push_back( sceneNode );
        }

        SmartPtr<IInstanceManager> CGraphicsSceneOgre::createInstanceManager(
            const String &customName, const String &meshName, const String &groupName, u32 technique,
            u32 numInstancesPerBatch, u16 flags /*=0*/, u16 subMeshIdx /*=0 */ )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::InstanceManager *mgr = m_sceneManager->createInstanceManager(
                customName.c_str(), meshName.c_str(),
                Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                static_cast<Ogre::InstanceManager::InstancingTechnique>( technique ), 80,
                Ogre::IM_USE16BIT, subMeshIdx );

            SmartPtr<CInstanceManager> instanceManager;  // (new CInstanceManager, true);
            m_instanceManagers.push_back( instanceManager );
            instanceManager->setInstanceManager( mgr );
            return instanceManager;
        }

        SmartPtr<IInstancedObject> CGraphicsSceneOgre::createInstancedObject( const String &materialName,
                                                                              const String &managerName )
        {
            SmartPtr<CInstancedObject>
                instancedObject;  // (new CInstancedObject(this, materialName, managerName), true);
            _addGraphicsObject( instancedObject );
            return instancedObject;
        }

        void CGraphicsSceneOgre::destroyInstancedObject( SmartPtr<IInstancedObject> instancedObject )
        {
        }

        void CGraphicsSceneOgre::_addGraphicsObject( SmartPtr<IGraphicsObject> graphicsObject )
        {
            //SpinRWMutex::ScopedLock lock( GraphicsObjectMutex );
            m_graphicsObjects.push_back( graphicsObject );
        }

        Array<SmartPtr<IGraphicsObject>> CGraphicsSceneOgre::_getGraphicsObjects() const
        {
            //SpinRWMutex::ScopedLock lock( GraphicsObjectMutex );
            return m_graphicsObjects.snapshot();
        }

        SmartPtr<IGraphicsTerrain> CGraphicsSceneOgre::getTerrain( u32 id ) const
        {
            return m_terrains[id];
        }

        void CGraphicsSceneOgre::setAnimationNameSuffix( const String &animationNameSuffix )
        {
            m_animationNameSuffix = animationNameSuffix;
        }

        String CGraphicsSceneOgre::getAnimationNameSuffix() const
        {
            return m_animationNameSuffix;
        }

        void CGraphicsSceneOgre::setAnimationNamePrefix( const String &animationNamePrefix )
        {
            m_animationNamePrefix = animationNamePrefix;
        }

        String CGraphicsSceneOgre::getAnimationNamePrefix() const
        {
            return m_animationNamePrefix;
        }

        CGraphicsSceneOgre::SceneManagerStateListener::SceneManagerStateListener()
        {
        }

        CGraphicsSceneOgre::SceneManagerStateListener::~SceneManagerStateListener()
        {
            m_owner = nullptr;
        }

        bool CGraphicsSceneOgre::SceneManagerStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto owner = getOwner() )
            {
                Ogre::SceneManager *smgr = nullptr;
                owner->_getObject( (void **)&smgr );

                if( smgr )
                {
                    try
                    {
                        WP_ASSERT( Thread::getTaskFlag( Thread::Render_Flag ) );

                        auto stateData = state->getData();

                        if( stateData->isDerived<AmbientLightStateData>() )
                        {
                            auto ambientLightState =
                                workphone::static_pointer_cast<AmbientLightStateData>( stateData );

                            auto ambientLight =
                                OgreUtil::convertToOgre( ambientLightState->ambientColour );
                            if( !OgreUtil::equals( ambientLight, smgr->getAmbientLight() ) )
                            {
                                smgr->setAmbientLight( ambientLight );
                            }
                        }
                        else if( stateData->isDerived<GraphicsSceneState>() )
                        {
                            auto sceneManagerState =
                                workphone::static_pointer_cast<GraphicsSceneState>( stateData );

                            if( sceneManagerState->enableSkybox )
                            {
                                auto skyboxMaterial = owner->getSkyboxMaterial();
                                //WP_ASSERT(skyboxMaterial);

                                if( auto pMaterial =
                                        workphone::static_pointer_cast<CMaterialOgre>( skyboxMaterial ) )
                                {
                                    auto ogreMaterial = pMaterial->getMaterial();
                                    //WP_ASSERT(ogreMaterial);

                                    if( ogreMaterial )
                                    {
                                        ogreMaterial->load();

                                        auto materialName = ogreMaterial->getName();
                                        smgr->setSkyBox( true, materialName );
                                    }
                                }
                            }
                            else
                            {
                                smgr->setSkyBox( false, "" );
                            }

                            auto enableShadows = sceneManagerState->enableShadows;
                            if( enableShadows )
                            {
                                smgr->setShadowTechnique( Ogre::SHADOWTYPE_NONE );
                                smgr->setShadowFarDistance( 100.0f );
                            }
                            else
                            {
                                smgr->setShadowTechnique( Ogre::SHADOWTYPE_NONE );
                            }
                        }

                        return true;
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }
            }

            return false;
        }

        bool CGraphicsSceneOgre::SceneManagerStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                if( message->isDerived<StateMessageSkyBox>() )
                {
                    auto messageSkyBox = workphone::static_pointer_cast<StateMessageSkyBox>( message );
                    auto enable = messageSkyBox->getEnable();
                    auto material = messageSkyBox->getMaterial();

                    owner->setSkyBox( enable, material );
                }
            }

            return false;
        }

        SmartPtr<CGraphicsSceneOgre> CGraphicsSceneOgre::SceneManagerStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        void CGraphicsSceneOgre::SceneManagerStateListener::setOwner(
            SmartPtr<CGraphicsSceneOgre> owner )
        {
            m_owner = owner;
        }

        CGraphicsSceneOgre::RootSceneNode::RootSceneNode() = default;

        CGraphicsSceneOgre::RootSceneNode::~RootSceneNode()
        {
            unload( nullptr );
            destroyStateContext();
        }

        void CGraphicsSceneOgre::RootSceneNode::load( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Loaded );
        }

        void CGraphicsSceneOgre::RootSceneNode::unload( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Unloading );
            m_sceneNode = nullptr;
            setLoadingState( LoadingState::Unloaded );
        }

        CGraphicsSceneOgre::MaterialSharedListener::MaterialSharedListener() = default;

        CGraphicsSceneOgre::MaterialSharedListener::~MaterialSharedListener() = default;

        void CGraphicsSceneOgre::MaterialSharedListener::unload( SmartPtr<ISharedObject> data )
        {
            m_owner = nullptr;
        }

        Parameter CGraphicsSceneOgre::MaterialSharedListener::handleEvent(
            EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
            SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
        {
            return Parameter();
        }

        void CGraphicsSceneOgre::MaterialSharedListener::loadingStateChanged(
            ISharedObject *sharedObject, LoadingState oldState, LoadingState newState )
        {
            if( newState == LoadingState::Loaded )
            {
                if( auto owner = getOwner() )
                {
                    owner->setSkyBox( owner->getEnableSkybox(), owner->m_skyboxMaterial );
                }
            }
        }

        bool CGraphicsSceneOgre::MaterialSharedListener::destroy( void *ptr )
        {
            return false;
        }

        SmartPtr<CGraphicsSceneOgre> CGraphicsSceneOgre::MaterialSharedListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        void CGraphicsSceneOgre::MaterialSharedListener::setOwner( SmartPtr<CGraphicsSceneOgre> owner )
        {
            m_owner = owner;
        }

        CGraphicsSceneOgre::SkyboxStateListener::SkyboxStateListener()
        {
        }

        CGraphicsSceneOgre::SkyboxStateListener::~SkyboxStateListener()
        {
            m_owner = nullptr;
        }

        bool CGraphicsSceneOgre::SkyboxStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool CGraphicsSceneOgre::SkyboxStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto owner = getOwner() )
            {
                if( auto stateContext = owner->getStateContext() )
                {
                    stateContext->setDirty( true );
                }
            }

            return false;
        }

        SmartPtr<CGraphicsSceneOgre> CGraphicsSceneOgre::SkyboxStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        void CGraphicsSceneOgre::SkyboxStateListener::setOwner( SmartPtr<CGraphicsSceneOgre> owner )
        {
            m_owner = owner;
        }
    }  // end namespace render
}  // namespace workphone
