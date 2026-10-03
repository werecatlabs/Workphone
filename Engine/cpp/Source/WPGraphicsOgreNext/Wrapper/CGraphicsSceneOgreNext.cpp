#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CInstanceManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CInstancedObjectOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSceneNodeOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsMeshOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCubemapOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CLightOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CAnimationStateController.hpp>
#include <WPGraphicsOgreNext/Wrapper/CAnimationTextureControl.hpp>
#include <WPGraphicsOgreNext/Wrapper/CBillboardSetOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCameraOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDecalCursor.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDynamicMesh.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDynamicLines.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTerrainOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CViewportOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSkybox6SidedOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSkyboxCubeOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CWindowOgreNext.hpp>
#include <WPGraphicsOgreNext/MeshLoader.hpp>
#include <WPGraphicsOgreNext/OgreUtil.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNextTypes.hpp>
#include <WPGraphicsOgreNext/CompositorManager.hpp>
#include <WPGraphicsOgreNext/Compositor.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#include <Workphone/Workphone.hpp>

#ifdef WP_OGRE_USE_MESH_SPLITTERS
#    include "DestructibleMeshSplitter.h"
#    include "OgreMeshExtractor.h"
#    include "VertexMerger.h"
#endif

#include <Ogre.h>
#include <OgreImage2.h>
#include <OgreItem.h>
#include <OgreMeshManager.h>
#include <OgreMeshManager2.h>
#include <OgreMesh2.h>
#include <OgreSceneManager.h>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreRenderable.h>

#include <algorithm>
#include <limits>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, CGraphicsSceneOgreNext, GraphicsScene );

    u32 CGraphicsSceneOgreNext::m_nextGeneratedNameExt = 0;

    CGraphicsSceneOgreNext::CGraphicsSceneOgreNext()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = WPGraphicsOgreNext::getFactoryManager();
            WP_ASSERT( factoryManager );

            m_animationNamePrefix = StringUtil::EmptyString;
            m_animationNameSuffix = StringUtil::EmptyString;

            createStateContext();

            m_rootSceneNode = factoryManager->make_ptr<RootSceneNode>( this );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    CGraphicsSceneOgreNext::CGraphicsSceneOgreNext( Ogre::SceneManager *sceneManager )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = WPGraphicsOgreNext::getFactoryManager();
            WP_ASSERT( factoryManager );

            m_sceneManager = sceneManager;

            m_animationNamePrefix = StringUtil::EmptyString;
            m_animationNameSuffix = StringUtil::EmptyString;

            createStateContext();

            m_rootSceneNode = factoryManager->make_ptr<RootSceneNode>( this );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    CGraphicsSceneOgreNext::~CGraphicsSceneOgreNext()
    {
        if( m_rootSceneNode )
        {
            m_rootSceneNode->unload( nullptr );
            m_rootSceneNode = nullptr;
        }
    }

    void CGraphicsSceneOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            m_skies.reserve( 4 );

            GraphicsScene::load( data );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = (CGraphicsSystemOgreNext *)applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = WPGraphicsOgreNext::getFactoryManager();
            WP_ASSERT( factoryManager );

            setFactoryManager( factoryManager );

            WP_ASSERT( Thread::getTaskFlag( Thread::Render_Flag ) );

            auto root = Ogre::Root::getSingletonPtr();
            WP_ASSERT( root );

            auto name = getName();

            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );

            // auto id = StringUtil::getHash(name);

            auto numThreads = 0;
            auto sceneMgr = root->createSceneManager( Ogre::ST_GENERIC, numThreads, name.c_str() );
            WP_ASSERT( sceneMgr );

            auto overlaySystem = graphicsSystem->getOverlaySystem();
            auto overlayManager = Ogre::v1::OverlayManager::getSingletonPtr();

            WP_ASSERT( overlaySystem );
            WP_ASSERT( overlayManager );

            auto sceneFactoryManager = workphone::make_ptr<FactoryManager>();
            sceneFactoryManager->load( nullptr );
            setFactoryManager( sceneFactoryManager );

            FactoryUtil::addFactory<CCameraOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CGraphicsMeshOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CGraphicsSceneOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CGraphicsMeshOgreNext::MeshMaterialEventListener>(
                sceneFactoryManager );
            FactoryUtil::addFactory<CLightOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CMaterialOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CMaterialTextureOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CMaterialTextureOgreNext::TextureListener>( sceneFactoryManager );
            FactoryUtil::addFactory<CTextureOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CParticleSystemOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CSceneNodeOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CDynamicMesh>( sceneFactoryManager );
            FactoryUtil::addFactory<CSkybox6SidedOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CSkyboxCubeOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CTerrainOgreNext>( sceneFactoryManager );
            FactoryUtil::addFactory<CWindowOgreNext>( sceneFactoryManager );

            sceneFactoryManager->setPoolSizeByType<CCameraOgreNext>( 8 );
            sceneFactoryManager->setPoolSizeByType<CGraphicsMeshOgreNext>( 1024 );
            sceneFactoryManager->setPoolSizeByType<CGraphicsMeshOgreNext::MeshMaterialEventListener>(
                8 );
            sceneFactoryManager->setPoolSizeByType<CLightOgreNext>( 32 );
            sceneFactoryManager->setPoolSizeByType<CParticleSystemOgreNext>( 8 );
            sceneFactoryManager->setPoolSizeByType<CSceneNodeOgreNext>( 4096 );
            sceneFactoryManager->setPoolSizeByType<CSkyboxCubeOgreNext>( 8 );
            sceneFactoryManager->setPoolSizeByType<CTerrainOgreNext>( 4 );
            sceneFactoryManager->setPoolSizeByType<CWindowOgreNext>( 1 );

            sceneMgr->addRenderQueueListener( overlaySystem );
            auto sceneManagerRenderQueue = sceneMgr->getRenderQueue();
            WP_ASSERT( sceneManagerRenderQueue );

            sceneManagerRenderQueue->setSortRenderQueue( overlayManager->mDefaultRenderQueueId,
                                                         Ogre::RenderQueue::StableSort );

            m_sceneManager = sceneMgr;

            //m_sceneManager->setAmbientLight( Ogre::ColourValue::White * 0.4f,
            //                                 Ogre::ColourValue::White * 0.4f,
            //                                 Ogre::Vector3::UNIT_Y );

            m_sceneManager->setAmbientLight( Ogre::ColourValue::White * 0.0f,
                                             Ogre::ColourValue::White * 0.0f, Ogre::Vector3::UNIT_Y,
                                             1.0f );

            auto rootNode = sceneMgr->getRootSceneNode();
            WP_ASSERT( rootNode );

            auto pRootSceneNode = getRootSceneNode();
            WP_ASSERT( pRootSceneNode );

            auto rootSceneNode = workphone::static_pointer_cast<CSceneNodeOgreNext>( pRootSceneNode );
            WP_ASSERT( rootSceneNode );
            rootSceneNode->setupNode( rootNode );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CGraphicsSceneOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( !isLoaded() )
            {
                return;
            }

            // State data owns graphics resources. Release those references before the
            // corresponding camera/material factories begin destroying scene objects.
            if( auto stateContext = getStateContext() )
            {
                if( auto stateData = stateContext->getStateDataById<GraphicsSceneState>( getId() ) )
                {
                    stateData->activeCamera = nullptr;
                    stateData->skyboxMaterial = nullptr;
                    stateData->skyboxTexture = nullptr;
                }
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto stateManager = applicationManager->getStateManager();

            for( auto sky : m_skies )
            {
                if( sky )
                {
                    sky->unload( nullptr );
                }
            }

            m_skies.clear();

            if( auto stateContext = getStateContext() )
            {
                if( auto stateListener = getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                    stateListener->unload( nullptr );
                    setStateListener( nullptr );
                }
            }

            for( auto &obj : m_graphicsObjects )
            {
                obj->unload( nullptr );
            }

            m_graphicsObjects.clear();
            m_cubemaps.clear();

            if( m_rootSceneNode )
            {
                m_rootSceneNode->removeChildren();
            }

            m_registeredSceneNodes.clear();
            m_registeredGfxObjects.clear();

            for( auto &sceneNode : m_sceneNodes )
            {
                sceneNode->detachAllObjects();
            }

            for( auto &sceneNode : m_sceneNodes )
            {
                sceneNode->unload( nullptr );
            }

            m_sceneNodes.clear();

            if( m_rootSceneNode )
            {
                m_rootSceneNode->removeChildren();
            }

            if( m_rootSceneNode )
            {
                m_rootSceneNode->unload( nullptr );
                m_rootSceneNode = nullptr;
            }

            if( m_sceneManager )
            {
                m_sceneManager->clearScene( true );
                m_sceneManager = nullptr;
            }

            destroyStateContext();

            GraphicsScene::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CGraphicsSceneOgreNext::update()
    {
        auto graphicsObjects = m_graphicsObjects.snapshot();
        for( auto &object : graphicsObjects )
        {
            if( object )
            {
                if( object->isDerived<IGraphicsCamera>() )
                {
                    auto camera = workphone::static_pointer_cast<render::IGraphicsCamera>( object );
                    if( object->isVisible() )
                    {
                        setActiveCamera( camera );
                    }
                }
            }
        }

        auto particleSystems = m_particleSystems.snapshot();
        for( auto &particleSystem : particleSystems )
        {
            if( particleSystem )
            {
                particleSystem->update();
            }
        }

        auto terrains = m_terrains.snapshot();
        for( auto &terrain : terrains )
        {
            if( terrain )
            {
                terrain->update();
            }
        }

        auto skies = m_skies.snapshot();
        for( auto &sky : skies )
        {
            if( sky )
            {
                sky->update();
            }
        }

        refreshAutomaticCubemaps();
    }

    void CGraphicsSceneOgreNext::registerCubemap( CCubemapOgreNext *cubemap )
    {
        if( !cubemap )
        {
            return;
        }

        if( std::find( m_cubemaps.begin(), m_cubemaps.end(), cubemap ) == m_cubemaps.end() )
        {
            m_cubemaps.push_back( cubemap );
            refreshAutomaticCubemaps();
        }
    }

    void CGraphicsSceneOgreNext::unregisterCubemap( CCubemapOgreNext *cubemap )
    {
        if( !cubemap )
        {
            return;
        }

        auto it = std::remove( m_cubemaps.begin(), m_cubemaps.end(), cubemap );
        if( it != m_cubemaps.end() )
        {
            m_cubemaps.erase( it, m_cubemaps.end() );
            refreshAutomaticCubemaps();
        }
    }

    CCubemapOgreNext *CGraphicsSceneOgreNext::findBestCubemap( const Vector3F &position,
                                                               u32 visibilityMask ) const
    {
        auto bestScore = -std::numeric_limits<f32>::max();
        CCubemapOgreNext *bestCubemap = nullptr;

        for( auto cubemap : m_cubemaps )
        {
            if( !cubemap )
            {
                continue;
            }

            const auto cubemapMask = cubemap->getVisibilityMask();
            if( visibilityMask != 0u && cubemapMask != 0u && ( cubemapMask & visibilityMask ) == 0u )
            {
                continue;
            }

            const auto score = cubemap->getInfluenceScore( position );
            if( score > bestScore )
            {
                bestScore = score;
                bestCubemap = cubemap;
            }
        }

        return bestCubemap;
    }

    void CGraphicsSceneOgreNext::refreshAutomaticCubemaps()
    {
        auto graphicsObjects = m_graphicsObjects.snapshot();
        for( auto &object : graphicsObjects )
        {
            auto mesh = workphone::dynamic_pointer_cast<CGraphicsMeshOgreNext>( object );
            if( mesh )
            {
                mesh->updateAutomaticCubemap();
            }
        }
    }

    void CGraphicsSceneOgreNext::postUpdate()
    {
        // find active camera
        auto cameras = m_cameras.snapshot();
        for( auto &camera : cameras )
        {
            if( camera && camera->isVisible() )
            {
                setActiveCamera( camera );
                break;
            }
        }

        auto terrains = m_terrains.snapshot();
        for( auto &terrain : terrains )
        {
            if( terrain )
            {
                terrain->postUpdate();
            }
        }
    }

#ifdef _DEBUG
    s32 CGraphicsSceneOgreNext::addReference()
    {
        return ISharedObject::addReference();
    }

    bool CGraphicsSceneOgreNext::removeReference()
    {
        return ISharedObject::removeReference();
    }
#endif

    void CGraphicsSceneOgreNext::clear()
    {
        try
        {
            m_isClearing = true;

            m_registeredSceneNodes.clear();
            m_registeredGfxObjects.clear();

            auto sceneNodes = m_sceneNodes.snapshot();
            for( auto &sceneNode : sceneNodes )
            {
                if( sceneNode )
                {
                    sceneNode->unload( nullptr );
                }
            }

            if( auto rootNode = getRootSceneNode() )
            {
                rootNode->removeChildren();
            }

            m_sceneNodes.clear();
            m_cubemaps.clear();

            if( m_sceneManager )
            {
                m_sceneManager->setSky( false, Ogre::SceneManager::SkyCubemap, {},
                                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                m_sceneManager->clearScene( true );
            }

            m_isClearing = false;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto CGraphicsSceneOgreNext::hasAnimation( const String &animationName ) -> bool
    {
        if( m_sceneManager )
        {
            return m_sceneManager->hasAnimation( animationName.c_str() );
        }

        return false;
    }

    auto CGraphicsSceneOgreNext::destroyAnimation( const String &animationName ) -> bool
    {
        if( m_sceneManager->hasAnimation( animationName.c_str() ) )
        {
            m_sceneManager->destroyAnimation( animationName.c_str() );
            return true;
        }

        return false;
    }

    SmartPtr<ISharedObject> CGraphicsSceneOgreNext::addGraphicsObjectByTypeId( hash_type id )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        WP_ASSERT( graphicsSystem );

        if( auto factoryManager = getFactoryManagerPtr() )
        {
            if( id == IGraphicsCamera::typeInfo() )
            {
                auto camera = factoryManager->make_ptr<CCameraOgreNext>( this );
                graphicsSystem->loadObject( camera, true );
                m_cameras.push_back( camera );
                return camera;
            }
            else if( id == IGraphicsLight::typeInfo() )
            {
                auto light = factoryManager->make_ptr<CLightOgreNext>( this );
                graphicsSystem->loadObject( light, true );
                m_graphicsObjects.push_back( light );
                return light;
            }
            else if( id == IGraphicsMesh::typeInfo() )
            {
                auto mesh = factoryManager->make_ptr<CGraphicsMeshOgreNext>( this );
                m_graphicsObjects.push_back( mesh );
                return mesh;
            }
            else if( id == IParticleSystem::typeInfo() )
            {
                auto particleSystem = factoryManager->make_ptr<CParticleSystemOgreNext>( this );
                m_particleSystems.push_back( particleSystem );
                return particleSystem;
            }
            else if( id == IDynamicMesh::typeInfo() )
            {
                auto dynamicMesh = factoryManager->make_ptr<CDynamicMesh>( this );
                graphicsSystem->loadObject( dynamicMesh, true );
                m_graphicsObjects.push_back( dynamicMesh );
                return dynamicMesh;
            }
            else if( id == ISkybox::typeInfo() || id == ISkyboxCube::typeInfo() )
            {
                auto sky = factoryManager->make_ptr<CSkyboxCubeOgreNext>();
                sky->setScene( this );
                graphicsSystem->loadObject( sky, true );
                m_skies.push_back( sky );
                return sky;
            }

            auto object = factoryManager->make_object<ISharedObject>( static_cast<u32>( id ) );
            if( !object )
            {
                WP_LOG_ERROR( "Could not create graphics object." );
                return nullptr;
            }

            if( object->isDerived<IGraphicsObject>() )
            {
                auto graphicsObject = workphone::static_pointer_cast<IGraphicsObject>( object );
                graphicsObject->setCreator( this );

                graphicsSystem->loadObject( graphicsObject, true );

                if( graphicsObject->isDerived<IParticleSystem>() )
                {
                    m_particleSystems.push_back( graphicsObject );
                }
                else
                {
                    m_graphicsObjects.push_back( graphicsObject );
                }

                return graphicsObject;
            }
            else if( object->isDerived<IGraphicsTerrain>() )
            {
                auto terrain = workphone::static_pointer_cast<IGraphicsTerrain>( object );
                terrain->setSceneManager( this );
                m_terrains.push_back( terrain );
                invalidateSceneState();

                graphicsSystem->loadObject( object, true );
                return terrain;
            }
            else
            {
                graphicsSystem->loadObject( object, true );
                m_skies.push_back( workphone::static_pointer_cast<ISky>( object ) );
            }

            return object;
        }

        return nullptr;
    }

    SmartPtr<IGraphicsSceneNode> CGraphicsSceneOgreNext::addSceneNode( const String &name )
    {
        auto factoryManager = getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        if( factoryManager )
        {
            if( auto sceneNode = factoryManager->make_ptr<CSceneNodeOgreNext>( this ) )
            {
                sceneNode->setName( name );
                m_sceneNodes.push_back( sceneNode );

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
                WP_ASSERT( graphicsSystem );

                graphicsSystem->loadObject( sceneNode );
                return sceneNode;
            }
        }

        return nullptr;
    }

    auto CGraphicsSceneOgreNext::validateGfxObjName( const String &name ) const -> bool
    {
        if( name.length() == 0 )
        {
            return false;
        }

        for( auto object : m_graphicsObjects )
        {
            if( object->getNamePtr() == name )
            {
                return false;
            }
        }

        return true;
    }

    auto CGraphicsSceneOgreNext::addCamera( const String &name ) -> SmartPtr<IGraphicsCamera>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( name ) );
            WP_ASSERT( validateGfxObjName( name ) );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = WPGraphicsOgreNext::getFactoryManager();
            WP_ASSERT( factoryManager );

            auto camera = factoryManager->make_ptr<CCameraOgreNext>();
            camera->setCreator( this );

            auto handle = camera->getHandle();
            WP_ASSERT( handle );

            camera->setName( name );

            m_graphicsObjects.emplace_back( camera );

            graphicsSystem->loadObject( camera );

            if( !m_defaultCamera )
            {
                m_defaultCamera = camera;
            }

            return camera;
        }
        catch( Ogre::Exception &e )
        {
            WP_LOG_ERROR( e.getFullDescription().c_str() );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto CGraphicsSceneOgreNext::getCamera( const String &name ) -> SmartPtr<IGraphicsCamera>
    {
        for( auto object : m_graphicsObjects )
        {
            if( object->getName() == name )
            {
                return object;
            }
        }

        return nullptr;
    }

    auto CGraphicsSceneOgreNext::getCamera() const -> SmartPtr<IGraphicsCamera>
    {
        return m_defaultCamera;
    }

    auto CGraphicsSceneOgreNext::hasCamera( const String &name ) -> bool
    {
        for( auto object : m_graphicsObjects )
        {
            if( object->getNamePtr() == name )
            {
                return true;
            }
        }

        return false;
    }

    auto CGraphicsSceneOgreNext::addLight( const String &name ) -> SmartPtr<IGraphicsLight>
    {
        WP_ASSERT( validateGfxObjName( name ) );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        auto factoryManager = graphicsSystem->getFactoryManagerPtr();

        auto light = factoryManager->make_ptr<CLightOgreNext>( this );
        light->setName( name );

        m_graphicsObjects.emplace_back( light );

        graphicsSystem->loadObject( light );

        return light;
    }

    auto CGraphicsSceneOgreNext::getLight( const String &name ) const -> SmartPtr<IGraphicsLight>
    {
        for( auto object : m_graphicsObjects )
        {
            if( object->getNamePtr() == name )
            {
                return object;
            }
        }

        return nullptr;
    }

    auto CGraphicsSceneOgreNext::addMesh( const String &name, const String &meshName )
        -> SmartPtr<IGraphicsMesh>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            auto factoryManager = WPGraphicsOgreNext::getFactoryManager();

            WP_ASSERT( validateGfxObjName( name ) );

            auto graphicsObject = factoryManager->make_ptr<CGraphicsMeshOgreNext>( this );

            graphicsObject->setName( name );
            graphicsObject->setMeshName( meshName );

            m_graphicsObjects.push_back( graphicsObject );

            graphicsSystem->loadObject( graphicsObject );

            return graphicsObject;
        }
        catch( Ogre::Exception &e )
        {
            WP_LOG_INFO( e.getFullDescription().c_str() );
        }

        return nullptr;
    }

    auto CGraphicsSceneOgreNext::addMesh( const String &meshName ) -> SmartPtr<IGraphicsMesh>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = WPGraphicsOgreNext::getFactoryManager();

            auto name = getUniqueName( "Mesh" );
            WP_ASSERT( validateGfxObjName( name ) );

            auto graphicsObject = factoryManager->make_ptr<CGraphicsMeshOgreNext>();
            graphicsObject->setCreator( this );
            graphicsObject->setName( name );
            graphicsObject->setMeshName( meshName );

            m_graphicsObjects.push_back( graphicsObject );
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

    auto CGraphicsSceneOgreNext::createV1Mesh( const String &meshName ) -> SmartPtr<IGraphicsMesh>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto factoryManager = WPGraphicsOgreNext::getFactoryManager();

            auto meshExtention = Path::getFileExtension( meshName );
            static const auto engineMeshExtention = String( ".fbmeshbin" );
            if( meshExtention == engineMeshExtention )
            {
                auto resourceGroupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

                auto meshManagerV2 = Ogre::MeshManager::getSingletonPtr();
                auto meshResult = meshManagerV2->createOrRetrieve( meshName.c_str(), resourceGroupName );
                if( meshResult.second )
                {
                    auto newMesh = meshResult.first.dynamicCast<Ogre::Mesh>();

                    auto name = getUniqueName( "Mesh" );
                    auto entity = m_sceneManager->createItem( newMesh );

                    SmartPtr<CGraphicsMeshOgreNext> graphicsObject( new CGraphicsMeshOgreNext( this ) );
                    graphicsObject->setItem( entity );

                    m_graphicsObjects.push_back( graphicsObject );
                    return graphicsObject;
                }
                auto newMesh = meshManagerV2->createManual( meshName.c_str(), resourceGroupName );
                MeshLoader::loadFBMesh( newMesh, meshName );

                auto name = getUniqueName( "Mesh" );
                auto entity = m_sceneManager->createItem( newMesh );

                SmartPtr<CGraphicsMeshOgreNext> graphicsObject( new CGraphicsMeshOgreNext( this ) );
                graphicsObject->setItem( entity );

                m_graphicsObjects.push_back( graphicsObject );
                return graphicsObject;
            }
            auto meshManagerV1 = Ogre::v1::MeshManager::getSingletonPtr();
            auto resourceGroupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

            auto meshResult = meshManagerV1->createOrRetrieve( meshName.c_str(), resourceGroupName );
            if( meshResult.second )
            {
                auto planeMeshV1 = meshResult.first.dynamicCast<Ogre::v1::Mesh>();

                auto meshManagerV2 = Ogre::MeshManager::getSingletonPtr();
                auto planeMesh = meshManagerV2->createManual( meshName.c_str(), resourceGroupName );
                planeMesh->importV1( planeMeshV1.get(), true, true, true );

                auto name = getUniqueName( "Mesh" );
                auto entity = m_sceneManager->createItem( planeMesh );
                entity->setDatablock( "Marble" );

                SmartPtr<CGraphicsMeshOgreNext> graphicsObject( new CGraphicsMeshOgreNext( this ) );
                // graphicsObject->initialise(entity);
                graphicsObject->setItem( entity );

                // String entityName = entity->getName().c_str();
                // graphicsObject->getHandle()->setName(entityName);
                m_graphicsObjects.push_back( graphicsObject );
                return graphicsObject;
            }

            auto meshManagerV2 = Ogre::MeshManager::getSingletonPtr();
            auto mesh = meshManagerV2->load( meshName.c_str(), resourceGroupName );

            auto name = getUniqueName( "Mesh" );
            auto entity = m_sceneManager->createItem( mesh );

            SmartPtr<CGraphicsMeshOgreNext> graphicsObject( new CGraphicsMeshOgreNext( this ) );
            graphicsObject->setItem( entity );

            // String entityName = entity->getName().c_str();
            // graphicsObject->getHandle()->setName(entityName);
            m_graphicsObjects.push_back( graphicsObject );
            return graphicsObject;
        }
        catch( Ogre::Exception &e )
        {
            WP_LOG_ERROR( e.getFullDescription().c_str() );
        }

        return nullptr;
    }

    auto CGraphicsSceneOgreNext::getMesh( const String &name ) const -> SmartPtr<IGraphicsMesh>
    {
        const auto graphicsObjects = getGraphicsObjects();
        for( auto object : graphicsObjects )
        {
            if( object->isDerived<IGraphicsMesh>() )
            {
                if( object->getName() == name )
                {
                    return object;
                }
            }
        }

        return nullptr;
    }

    auto CGraphicsSceneOgreNext::addParticleSystem( const String &name, const String &templateName )
        -> SmartPtr<IParticleSystem>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = WPGraphicsOgreNext::getFactoryManager();

        ScopedLock lock( graphicsSystem );

        WP_ASSERT( validateGfxObjName( name ) );

        auto particleSystem = factoryManager->make_ptr<CParticleSystemOgreNext>( this );
        particleSystem->setName( name );
        particleSystem->setTemplateName( templateName );

        graphicsSystem->loadObject( particleSystem, true );
        m_particleSystems.emplace_back( particleSystem );

        return particleSystem;
    }

    auto CGraphicsSceneOgreNext::getParticleSystem( const String &name ) const
        -> SmartPtr<IParticleSystem>
    {
        const auto particleSystems = m_particleSystems.snapshot();
        for( auto object : particleSystems )
        {
            if( object->getName() == name )
            {
                return object;
            }
        }

        return nullptr;
    }

    auto CGraphicsSceneOgreNext::createAnimationStateController() -> SmartPtr<IAnimationStateController>
    {
        SmartPtr<IAnimationStateController>
            animStateCtrl;  // (new CAnimationStateController(this), true);
        return animStateCtrl;
    }

    auto CGraphicsSceneOgreNext::createAnimationTextureCtrl( SmartPtr<IMaterialTexture> textureUnit,
                                                             bool clone,
                                                             const String &clonedMaterialName )
        -> SmartPtr<IAnimationTextureControl>
    {
        Ogre::TextureUnitState *textureUnitState = nullptr;
        // textureUnit->_getObject((void**)&textureUnitState);

        auto textureCtrl = new CAnimationTextureControl;
        textureCtrl->initialise( textureUnitState );

        SmartPtr<IAnimationTextureControl> animTextureCtrl;  // (textureCtrl, true);
        return animTextureCtrl;
    }

    void CGraphicsSceneOgreNext::setSkyBox( bool enable, SmartPtr<IMaterial> material, f32 distance,
                                            bool drawFirst )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = WPGraphicsOgreNext::getFactoryManager();
            WP_ASSERT( factoryManager );

            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            if( isThreadSafe() )
            {
                ScopedLock lock( this );

                if( enable )
                {
                    if( m_sceneManager )
                    {
                        if( material )
                        {
                            auto pMaterial =
                                workphone::static_pointer_cast<CMaterialOgreNext>( material );

                            auto texName = String();

                            if( auto cubeTexture = pMaterial->getCubeTexture() )
                            {
                                texName = cubeTexture->getName();
                            }

                            if( !StringUtil::isNullOrEmpty( texName ) )
                            {
                                m_sceneManager->setSky(
                                    enable, Ogre::SceneManager::SkyCubemap, texName.c_str(),
                                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                            }
                        }
                    }
                }
                else
                {
                    m_sceneManager->setSky( enable, Ogre::SceneManager::SkyCubemap, "",
                                            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageSkyBox>();
                WP_ASSERT( message );

                message->setSender( this );
                message->setEnable( enable );
                message->setMaterial( material );

                auto stateContext = getStateContext();
                if( stateContext )
                {
                    stateContext->addMessage( stateTask, message );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CGraphicsSceneOgreNext::setFog( u32 fogMode, const ColourF &colour, f32 expDensity,
                                         f32 linearStart, f32 linearEnd )
    {
        ScopedLock lock( this );

        if( m_sceneManager )
        {
            auto fogColour = Ogre::ColourValue( colour.r, colour.g, colour.b, colour.a );
            auto ogreFogMode = static_cast<Ogre::FogMode>( fogMode );

            m_sceneManager->setFog( ogreFogMode, fogColour, expDensity, linearStart, linearEnd );
        }
    }

    void CGraphicsSceneOgreNext::registerSceneNodeForUpdates( SmartPtr<IGraphicsSceneNode> sceneNode )
    {
        m_registeredSceneNodes.push_back( sceneNode );
    }

    auto CGraphicsSceneOgreNext::unregisteredForUpdates( SmartPtr<IGraphicsSceneNode> sceneNode ) -> bool
    {
        if( sceneNode )
        {
            m_registeredSceneNodes.erase(
                std::remove( m_registeredSceneNodes.begin(), m_registeredSceneNodes.end(), sceneNode ),
                m_registeredSceneNodes.end() );

            return true;
        }

        return false;
    }

    auto CGraphicsSceneOgreNext::unregisteredForUpdates( IGraphicsSceneNode *sceneNode ) -> bool
    {
        if( sceneNode )
        {
            m_registeredSceneNodes.erase(
                std::remove( m_registeredSceneNodes.begin(), m_registeredSceneNodes.end(), sceneNode ),
                m_registeredSceneNodes.end() );

            return true;
        }

        return false;
    }

    void CGraphicsSceneOgreNext::registerForUpdates( SmartPtr<IGraphicsObject> gfxObject )
    {
        ScopedLock lock( this );
        // m_registeredGfxObjects.push_back(gfxObject.get());
    }

    auto CGraphicsSceneOgreNext::unregisteredForUpdates( SmartPtr<IGraphicsObject> gfxObject ) -> bool
    {
        ScopedLock lock( this );
        // return m_registeredGfxObjects.erase_element(gfxObject.get());

        return false;
    }

    auto CGraphicsSceneOgreNext::unregisteredForUpdates( IGraphicsObject *gfxObject ) -> bool
    {
        ScopedLock lock( this );
        // return m_registeredGfxObjects.erase_element(gfxObject);

        return false;
    }

    auto CGraphicsSceneOgreNext::getSceneManager() const -> Ogre::SceneManager *
    {
        return m_sceneManager;
    }

    void CGraphicsSceneOgreNext::_getObject( void **ppObject ) const
    {
        *ppObject = m_sceneManager;
    }

    void CGraphicsSceneOgreNext::clearQueues()
    {
        ScopedLock lock( this );

        for( auto entity : m_entityDeleteQueue )
        {
            //m_sceneManager->destroyItem( entity );
        }

        for( auto sceneNode : m_sceneNodeDeleteQueue )
        {
            m_sceneManager->destroySceneNode( sceneNode );
        }

        for( auto particleSystem : m_particleSysDeleteQueue )
        {
            m_sceneManager->destroyParticleSystem( particleSystem );
        }

        m_entityDeleteQueue.clear();
        m_sceneNodeDeleteQueue.clear();
        m_particleSysDeleteQueue.clear();
    }

    auto CGraphicsSceneOgreNext::getUniqueName( const String &baseName ) const -> String
    {
        return baseName + StringUtil::toString( m_nextGeneratedNameExt++ );
    }

    auto CGraphicsSceneOgreNext::createTerrain() -> SmartPtr<IGraphicsTerrain>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto factoryManager = WPGraphicsOgreNext::getFactoryManager();

        ScopedLock lock( graphicsSystem );

        auto terrain = factoryManager->make_ptr<CTerrainOgreNext>();
        WP_ASSERT( terrain );

        terrain->setSceneManager( this );
        graphicsSystem->loadObject( terrain );

        m_terrains.emplace_back( terrain );

        // Terrain contributes external compositor channels. Publish a scene state change so
        // camera compositors reconcile their terrain subscriptions and rebuild their workspaces.
        invalidateSceneState();

        return terrain;
    }

    void CGraphicsSceneOgreNext::destroyTerrain( SmartPtr<IGraphicsTerrain> terrain )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        ScopedLock lock( graphicsSystem );

        graphicsSystem->unloadObject( terrain );

        m_terrains.erase( std::remove( m_terrains.begin(), m_terrains.end(), terrain ),
                          m_terrains.end() );

        invalidateSceneState();
    }

    auto CGraphicsSceneOgreNext::addDecalCursor( const String &terrainMaterial,
                                                 const String &decalTextureName, const Vector2F &size )
        -> SmartPtr<IDecalCursor>
    {
        SmartPtr<CDecalCursor> decalCursor;  // (new CDecalCursor, true);
        // decalCursor->initialise(this, terrainMaterial, decalTextureName, size);
        return decalCursor;
    }

    // raycast from a point in to the scene.
    // returns success or failure.
    // on success the point is returned in the result.
    auto CGraphicsSceneOgreNext::castRay( const Ray3F &ray, Vector3F &result ) -> bool
    {
        // create the ray to test
        Vector3F start = ray.getOrigin();
        Vector3F normal = ray.getDirection();

        Ogre::Ray ogreRay( Ogre::Vector3( start.X(), start.Y(), start.Z() ),
                           Ogre::Vector3( normal.X(), normal.Y(), normal.Z() ) );

        if( !m_pRaySceneQuery )
        {
            // create the ray scene query object
            // m_pRaySceneQuery = m_sceneManager->createRayQuery(Ogre::Ray(),
            // Ogre::SceneManager::WORLD_GEOMETRY_TYPE_MASK); if (NULL == m_pRaySceneQuery)
            //{
            //	LOG_MESSAGE("SceneManager", "Failed to create Ogre::RaySceneQuery instance");
            //	return (false);
            //}

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
        for( auto &qr_idx : query_result )
        {
            // stop checking if we have found a raycast hit that is closer
            // than all remaining entities
            if( ( closest_distance >= 0.0f ) && ( closest_distance < qr_idx.distance ) )
            {
                break;
            }

            // only check this result if its a hit against an entity
            // if ((query_result[qr_idx].movable != NULL) &&
            //	(query_result[qr_idx].movable->getMovableType().compare("Entity") == 0))
            //{
            //	// get the entity to check
            //	Ogre::Entity *pentity = static_cast<Ogre::Entity*>(query_result[qr_idx].movable);

            //	// mesh data to retrieve
            //	size_t vertex_count;
            //	size_t index_count;
            //	Ogre::Vector3 *vertices;
            //	unsigned long *indices;

            //	// get the mesh information
            //	GetMeshInformation(pentity->getMesh(), vertex_count, vertices, index_count, indices,
            //		pentity->getParentNode()->_getDerivedPosition(),
            //		pentity->getParentNode()->_getDerivedOrientation(),
            //		pentity->getParentNode()->_getDerivedScale());

            //	// test for hitting individual triangles on the mesh
            //	bool new_closest_found = false;
            //	for (int i = 0; i < static_cast<int>(index_count); i += 3)
            //	{
            //		// check for a hit against this triangle
            //		std::pair<bool, Ogre::Real> hit = Ogre::Math::intersects(ogreRay,
            // vertices[indices[i]], 			vertices[indices[i+1]], vertices[indices[i+2]], true,
            // false);

            //		// if it was a hit check if its the closest
            //		if (hit.first)
            //		{
            //			if ((closest_distance < 0.0f) ||
            //				(hit.second < closest_distance))
            //			{
            //				// this is the closest so far, save it off
            //				closest_distance = hit.second;
            //				new_closest_found = true;
            //			}
            //		}
            //	}

            //	// free the verticies and indicies memory
            //	delete[] vertices;
            //	delete[] indices;

            //	// if we found a new closest raycast for this object, update the
            //	// closest_result before moving on to the next object.
            //	if (new_closest_found)
            //	{
            //		closest_result = ogreRay.getPoint(closest_distance);
            //	}
            //}
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

    auto CGraphicsSceneOgreNext::getSceneNodes() const
        -> const ConcurrentArray<SmartPtr<IGraphicsSceneNode>> &
    {
        return m_sceneNodes;
    }

    void CGraphicsSceneOgreNext::setSceneNodes(
        const ConcurrentArray<SmartPtr<IGraphicsSceneNode>> &sceneNodes )
    {
        m_sceneNodes = sceneNodes;
    }

    auto CGraphicsSceneOgreNext::getRegisteredSceneNodes() const
        -> const ConcurrentArray<SmartPtr<IGraphicsSceneNode>> &
    {
        return m_registeredSceneNodes;
    }

    void CGraphicsSceneOgreNext::setRegisteredSceneNodes(
        const ConcurrentArray<SmartPtr<IGraphicsSceneNode>> &sceneNodes )
    {
        m_registeredSceneNodes = sceneNodes;
    }

    auto getMeshByIndex( const Array<unsigned int> &viIndexOffsets, unsigned int iIndexIndex ) -> u32
    {
        // subset search
        int iRangeMin = 0;
        int iRangeMax = static_cast<s32>( viIndexOffsets.size() ) - 2;
        while( iRangeMin != iRangeMax )
        {
            int iSamplePoint = ( iRangeMax + iRangeMin ) >> 1;
            if( iIndexIndex < viIndexOffsets[iSamplePoint + 1] )
            {
                iRangeMax = iSamplePoint;
            }
            else  //>=
            {
                iRangeMin = iSamplePoint + 1;
            }
        }

        return iRangeMax;
    }

    auto CGraphicsSceneOgreNext::splitMesh( SmartPtr<IGraphicsMesh> mesh,
                                            const SmartPtr<Properties> &properties )
        -> Array<SmartPtr<IGraphicsMesh>>
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
                WP_LOG_INFO( "Warning: no \"maxSize\" property found. " );

            if( !properties->getPropertyValue( "roughness", fRoughness ) )
                WP_LOG_INFO( "Warning: no \"roughness\" property found. " );

            if( !properties->getPropertyValue( "recoveryAttempts", nRecoveryAttempts ) )
                WP_LOG_INFO( "Warning: no \"recoveryAttempts\" property found. " );

            if( !properties->getPropertyValue( "cutMaterial", strCutMaterial ) )
                WP_LOG_INFO( "Warning: no \"cutMaterial\" property found. " );
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
                    if( ( it = vmConnections[iMesh].find( iOtherMesh ) ) != vmConnections[iMesh].end() )
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
            for( std::map<unsigned int, std::set<int>>::iterator it = vmConnections[iFragment].begin();
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
            mesh_->load( nullptr );

            Ogre::Entity *entity_ = m_sceneManager->createEntity( mesh_ );

            String name = entity_->getName().c_str();
            SmartPtr<CGraphicsMeshOgreNext> graphicsObject( new CGraphicsMeshOgreNext( this ), this );
            graphicsObject->initialise( entity );
            graphicsObject->getHandle()->setName( name );
            m_graphicsObjects[name] = graphicsObject.get();
            meshes.push_back( graphicsObject );
        }
#endif

        return meshes;
    }

    void CGraphicsSceneOgreNext::addExistingGraphicsObject( SmartPtr<IGraphicsObject> graphicsObject )
    {
        m_graphicsObjects.push_back( graphicsObject );
    }

    void CGraphicsSceneOgreNext::addExistingSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
    {
        m_sceneNodes.push_back( sceneNode );
    }

    auto CGraphicsSceneOgreNext::createInstanceManager( const String &customName, const String &meshName,
                                                        const String &groupName, u32 technique,
                                                        u32 numInstancesPerBatch, u16 flags /*=0*/,
                                                        u16 subMeshIdx /*=0 */ )
        -> SmartPtr<IInstanceManager>
    {
        auto instanceManager = workphone::make_ptr<CInstanceManagerOgreNext>(
            customName, meshName, groupName, technique, numInstancesPerBatch, flags, subMeshIdx );
        m_instanceManagers.push_back( instanceManager );

        if( !StringUtil::isNullOrEmpty( meshName ) )
        {
            try
            {
                const auto &resourceGroup =
                    StringUtil::isNullOrEmpty( groupName )
                        ? Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME
                        : groupName;
                Ogre::MeshManager::getSingletonPtr()->load( meshName.c_str(), resourceGroup.c_str() );
            }
            catch( Ogre::Exception &e )
            {
                WP_LOG_ERROR( "CGraphicsSceneOgreNext::createInstanceManager - unable to load mesh '" +
                              meshName + "': " + e.getFullDescription() );
            }
        }

        return instanceManager;
    }

    auto CGraphicsSceneOgreNext::createInstancedObject( const String &materialName,
                                                        const String &managerName )
        -> SmartPtr<IInstancedObject>
    {
        SmartPtr<CInstanceManagerOgreNext> manager;
        for( const auto &instanceManager : m_instanceManagers )
        {
            auto ogreNextManager =
                workphone::dynamic_pointer_cast<CInstanceManagerOgreNext>( instanceManager );
            if( ogreNextManager && ogreNextManager->getCustomName() == managerName )
            {
                manager = ogreNextManager;
                break;
            }
        }

        if( !manager && !m_instanceManagers.empty() )
        {
            manager =
                workphone::dynamic_pointer_cast<CInstanceManagerOgreNext>( m_instanceManagers.front() );
        }

        if( !manager )
        {
            WP_LOG_ERROR( "CGraphicsSceneOgreNext::createInstancedObject - no instance manager named '" +
                          managerName + "'." );
            return nullptr;
        }

        auto instancedObject =
            workphone::make_ptr<CInstancedObjectOgreNext>( this, manager, materialName, managerName );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
        {
            graphicsSystem->loadObject( instancedObject );
        }

        addExistingGraphicsObject( SmartPtr<IGraphicsObject>( instancedObject ) );
        return instancedObject;
    }

    void CGraphicsSceneOgreNext::destroyInstancedObject( SmartPtr<IInstancedObject> instancedObject )
    {
        if( !instancedObject )
        {
            return;
        }

        m_graphicsObjects.erase( std::remove( m_graphicsObjects.begin(), m_graphicsObjects.end(),
                                              SmartPtr<IGraphicsObject>( instancedObject ) ),
                                 m_graphicsObjects.end() );

        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
            {
                graphicsSystem->unloadObject( instancedObject );
                return;
            }
        }

        instancedObject->unload( nullptr );
    }

    auto CGraphicsSceneOgreNext::getTerrain( u32 id ) const -> SmartPtr<IGraphicsTerrain>
    {
        return m_terrains[id];
    }

    void CGraphicsSceneOgreNext::setAnimationNameSuffix( const String &suffix )
    {
        m_animationNameSuffix = suffix;
    }

    auto CGraphicsSceneOgreNext::getAnimationNameSuffix() const -> String
    {
        return m_animationNameSuffix;
    }

    void CGraphicsSceneOgreNext::setAnimationNamePrefix( const String &prefix )
    {
        m_animationNamePrefix = prefix;
    }

    auto CGraphicsSceneOgreNext::getAnimationNamePrefix() const -> String
    {
        return m_animationNamePrefix;
    }

    void CGraphicsSceneOgreNext::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

    auto CGraphicsSceneOgreNext::getFactoryManager() const -> SmartPtr<IFactoryManager>
    {
        return m_factoryManager;
    }

    CGraphicsSceneOgreNext::RootSceneNode::~RootSceneNode()
    {
        unload( nullptr );
    }

    void CGraphicsSceneOgreNext::RootSceneNode::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loaded );
    }

    void CGraphicsSceneOgreNext::RootSceneNode::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        m_sceneNode = nullptr;
        CSceneNodeOgreNext::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    CGraphicsSceneOgreNext::RootSceneNode::RootSceneNode( CGraphicsSceneOgreNext *scene ) :
        CSceneNodeOgreNext( scene )
    {
    }

    CGraphicsSceneOgreNext::RootSceneNode::RootSceneNode() = default;

    bool CGraphicsSceneOgreNext::handleStateChanged( SmartPtr<IState> &state )
    {
        if( GraphicsScene::handleStateChanged( state ) )
        {
            return true;
        }

        if( state->getOwnerPtr() == this )
        {
            Ogre::SceneManager *smgr = nullptr;
            _getObject( reinterpret_cast<void **>( &smgr ) );

            if( smgr )
            {
                try
                {
                    if( auto stateData = state->getData() )
                    {
                        if( stateData->isDerived<GraphicsSceneState>() )
                        {
                            auto sceneManagerState = SafeReadPtr<GraphicsSceneState>( stateData );
                            if( sceneManagerState )
                            {
                                // if (sceneManagerState->getEnableSkybox())
                                //{
                                //	auto skyboxMaterialName = sceneManagerState->getSkyboxMaterialName();
                                //	smgr->setSkyBox(true, skyboxMaterialName);
                                // }
                                // else
                                //{
                                //	smgr->setSkyBox(false, "");
                                // }

                                //auto ambientLight =
                                //    OgreUtil::convertToOgre( sceneManagerState->ambientColour );
                                //if( !OgreUtil::equals( ambientLight, smgr->getAmbientLight() ) )
                                {
                                    //smgr->setAmbientLight( ambientLight, ambientLight, -Ogre::Vector3::UNIT_Y );
                                }

                                //smgr->setAmbientLight( ambientLight * 0.995f, ambientLight * 0.5f,
                                //                      Ogre::Vector3::UNIT_Y, 1.0f );
                            }

                            return true;
                        }
                        else if( stateData->isDerived<AmbientLightStateData>() )
                        {
                            auto ambientLightState = SafeReadPtr<AmbientLightStateData>( stateData );

                            auto upperHemisphere =
                                OgreUtil::convertToOgre( ambientLightState->upperHemisphere );
                            auto lowerHemisphere =
                                OgreUtil::convertToOgre( ambientLightState->lowerHemisphere );
                            auto hemisphereDir =
                                OgreUtil::convertToOgre( ambientLightState->hemisphereDir );
                            auto envMapScale = ambientLightState->envmapScale;

                            smgr->setAmbientLight( upperHemisphere, lowerHemisphere, hemisphereDir,
                                                   envMapScale );

                            return true;
                        }
                    }
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }
        }

        return false;
    }

    bool CGraphicsSceneOgreNext::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( GraphicsScene::handleStateMessage( message ) )
        {
            return true;
        }

        if( message )
        {
            if( message->getSender() == this )
            {
                if( message->isExactly<StateMessageSkyBox>() )
                {
                    auto pMessage = workphone::static_pointer_cast<StateMessageSkyBox>( message );
                    auto enabled = pMessage->getEnable();
                    auto material = pMessage->getMaterial();

                    setSkyBox( enabled, material );

                    return true;
                }
            }
        }

        return false;
    }

    void CGraphicsSceneOgreNext::createStateContext()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = WPGraphicsOgreNext::getFactoryManager();
        WP_ASSERT( factoryManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto stateContext = stateManager->addStateContext();
        WP_ASSERT( stateContext );

        stateContext->setOwner( this );
        setStateContext( stateContext );
        stateContext->setTaskId( TaskId::Render );

        auto sceneNodeContext = stateManager->addStateContext();
        WP_ASSERT( sceneNodeContext );

        sceneNodeContext->setOwner( this );
        setSceneNodeContext( sceneNodeContext );
        sceneNodeContext->setTaskId( TaskId::Render );

        auto stateListener = factoryManager->make_ptr<StateListener>();
        stateListener->setOwner( this );
        setStateListener( stateListener );

        Array<u32> graphicsObjectContextIds;
        graphicsObjectContextIds.reserve( 12 );

        graphicsObjectContextIds.push_back( CCameraOgreNext::typeInfo() );
        graphicsObjectContextIds.push_back( CGraphicsMeshOgreNext::typeInfo() );
        graphicsObjectContextIds.push_back( CLightOgreNext::typeInfo() );
        graphicsObjectContextIds.push_back( CParticleSystemOgreNext::typeInfo() );

        for( auto &id : graphicsObjectContextIds )
        {
            auto graphicsObjectContext = stateManager->addStateContext();
            WP_ASSERT( graphicsObjectContext );

            graphicsObjectContext->setOwner( this );
            setGraphicsObjectContext( id, graphicsObjectContext );
            graphicsObjectContext->setTaskId( TaskId::Render );

            graphicsObjectContext->addStateListener( stateListener );
        }

        stateContext->addStateListener( stateListener );
        sceneNodeContext->addStateListener( stateListener );

        auto state = factoryManager->make_ptr<State>();
        state->setId( getId() );
        state->setOwner( this );
        stateContext->addState( state );

        auto graphicsSceneState = factoryManager->make_ptr<GraphicsSceneState>();
        state->setData( graphicsSceneState );

        auto ambientLightState = factoryManager->make_ptr<State>();
        ambientLightState->setId( getId() );
        ambientLightState->setOwner( this );
        stateContext->addState( ambientLightState );

        auto ambientLightData = factoryManager->make_ptr<AmbientLightStateData>();
        ambientLightState->setData( ambientLightData );
    }

    void CGraphicsSceneOgreNext::destroyStateContext()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        if( !stateManager )
        {
            WP_LOG( "StateManager null" );
        }

        if( auto stateContext = getSceneNodeContext() )
        {
            if( stateContext->isLoaded() )
            {
                if( auto stateListener = getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                }

                if( stateManager )
                {
                    stateManager->removeStateContext( stateContext );
                }

                stateContext->setOwner( nullptr );
            }

            setSceneNodeContext( nullptr );
        }

        for( auto &[id, stateContextPtr] : m_graphicsObjectContexts )
        {
            if( auto stateContext = stateContextPtr.lock() )
            {
                if( auto stateListener = getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                }

                if( stateManager )
                {
                    stateManager->removeStateContext( stateContext );
                }

                stateContext->setOwner( nullptr );
            }
        }

        if( auto stateContext = getStateContext() )
        {
            if( auto stateListener = getStateListener() )
            {
                stateContext->removeStateListener( stateListener );
            }

            if( stateManager )
            {
                stateManager->removeStateContext( stateContext );
            }

            stateContext->setOwner( nullptr );
            setStateContext( nullptr );
        }

        if( auto stateListener = getStateListener() )
        {
            stateListener->unload( nullptr );
            setStateListener( nullptr );
        }
    }

}  // namespace workphone::render
