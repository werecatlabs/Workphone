#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/GraphicsScene.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Graphics/IAnimationStateController.hpp>
#include <Workphone/Interface/Graphics/IAnimationTextureControl.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Interface/Graphics/IBillboardSet.hpp>
#include <Workphone/Interface/Graphics/IInstanceManager.hpp>
#include <Workphone/Interface/Graphics/IInstancedObject.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IMaterialTexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/ISky.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsTerrain.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/State/States/AmbientLightStateData.hpp>
#include <Workphone/State/States/GraphicsSceneState.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsScene, SharedGraphicsObject<IGraphicsScene> );
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsScene::StateListener, IStateListener );

    u32 GraphicsScene::nodeCounter = 0;

    GraphicsScene::GraphicsScene()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

        auto name = String( "GraphicsScene" );
        setName( name );
        setId( StringUtil::getHash( name ) );
    }

    GraphicsScene::~GraphicsScene()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( applicationManager )
        {
            if( auto stateManager = applicationManager->getStateManagerPtr() )
            {
                if( auto stateContext = getStateContextPtr() )
                {
                    auto states = stateContext->getStates();
                    for( auto &state : states )
                    {
                        if( state && state->getOwnerPtr() == this )
                        {
                            state->unload( nullptr );
                            stateContext->removeState( state );
                        }
                    }

                    setStateContext( nullptr );
                }
            }
        }
    }

    void GraphicsScene::load( SmartPtr<ISharedObject> data )
    {
        SharedGraphicsObject<IGraphicsScene>::load( data );

        m_animations.reserve( 4 );
        m_cameras.reserve( 4 );
        m_registeredSceneNodes.reserve( 4 );
        m_registeredGfxObjects.reserve( 4 );
        m_sceneNodes.reserve( 128 );
        m_graphicsObjects.reserve( 128 );
        m_particleSystems.reserve( 4 );

        m_skies.reserve( 4 );
        m_terrains.reserve( 4 );
    }

    void GraphicsScene::unload( SmartPtr<ISharedObject> data )
    {
        std::fprintf( stderr, "TRACE base GraphicsScene unload start %p\n", this );
        // Clear all scene objects before unloading
        clear();
        std::fprintf( stderr, "TRACE base GraphicsScene clear done\n" );

        m_animations.clear();

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
        {
            auto cameras = m_cameras.snapshot();
            for( auto &camera : cameras )
            {
                if( camera )
                    graphicsSystem->unloadObject( camera );
            }

            auto sceneNodes = m_sceneNodes.snapshot();
            for( auto &sceneNode : sceneNodes )
            {
                if( sceneNode )
                    graphicsSystem->unloadObject( sceneNode );
            }

            auto terrains = m_terrains.snapshot();
            for( auto &terrain : terrains )
            {
                if( terrain )
                    graphicsSystem->unloadObject( terrain );
            }
        }

        m_cameras.clear();
        m_registeredSceneNodes.clear();
        m_registeredGfxObjects.clear();
        m_sceneNodes.clear();
        m_graphicsObjects.clear();
        m_particleSystems.clear();
        m_terrains.clear();
        m_defaultCamera = nullptr;
        m_rootSceneNode = nullptr;

        if( auto factoryManager = getFactoryManager() )
        {
            setFactoryManager( nullptr );
        }

        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( auto stateManager = applicationManager->getStateManagerPtr() )
            {
                std::fprintf( stderr, "TRACE base GraphicsScene state cleanup start\n" );
                auto stateListener = getStateListener();

                if( auto sceneNodeContext = getSceneNodeContext() )
                {
                    std::fprintf( stderr, "TRACE base GraphicsScene node context %p\n",
                                  sceneNodeContext.get() );
                    if( stateListener )
                    {
                        sceneNodeContext->removeStateListener( stateListener );
                    }
                    std::fprintf( stderr, "TRACE base GraphicsScene node listener removed\n" );

                    stateManager->removeStateContext( sceneNodeContext );
                    std::fprintf( stderr, "TRACE base GraphicsScene node context removed\n" );
                    sceneNodeContext->setOwner( nullptr );
                    std::fprintf( stderr, "TRACE base GraphicsScene node context owner cleared\n" );
                }
                std::fprintf( stderr, "TRACE base GraphicsScene scene-node context done\n" );

                Array<SmartPtr<IStateContext>> graphicsObjectContexts;
                for( auto &[id, weakContext] : m_graphicsObjectContexts )
                {
                    if( auto ctx = weakContext.lock() )
                    {
                        graphicsObjectContexts.push_back( ctx );
                    }
                }

                // Keep the contexts alive while releasing the weak index. WeakPtr uses
                // intrusive counters stored on the context itself, so its destructor must
                // run before removeStateContext can destroy the last strong reference.
                m_graphicsObjectContexts.clear();

                for( auto &ctx : graphicsObjectContexts )
                {
                    if( stateListener )
                        ctx->removeStateListener( stateListener );

                    stateManager->removeStateContext( ctx );
                    ctx->setOwner( nullptr );
                }
                std::fprintf( stderr, "TRACE base GraphicsScene object contexts done\n" );
            }
        }

        m_sceneNodeContext = nullptr;
        m_graphicsObjectContexts.clear();

        SharedGraphicsObject<IGraphicsScene>::unload( data );
        std::fprintf( stderr, "TRACE base GraphicsScene unload end\n" );
    }

    void GraphicsScene::update()
    {
        // Update all graphics objects
        for( auto &graphicsObject : m_graphicsObjects )
        {
            if( graphicsObject )
            {
                graphicsObject->update();
            }
        }

        // Update all particle systems
        for( auto &particleSystem : m_particleSystems )
        {
            if( particleSystem )
            {
                particleSystem->update();
            }
        }

        for( auto &sky : m_skies )
        {
            if( sky )
            {
                sky->update();
            }
        }
    }

    String GraphicsScene::getType() const
    {
        return m_type.load();
    }

    void GraphicsScene::setType( const String &type )
    {
        m_type = type;
    }

    ColourF GraphicsScene::getUpperHemisphere() const
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->getStateDataById<AmbientLightStateData>( getId() ) )
            {
                return stateData->upperHemisphere;
            }
        }

        return {};
    }

    void GraphicsScene::setUpperHemisphere( const ColourF &upperHemisphere )
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->invalidateStateDataById<AmbientLightStateData>( getId() ) )
            {
                stateData->upperHemisphere = upperHemisphere;
            }
        }
    }

    ColourF GraphicsScene::getLowerHemisphere() const
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->getStateDataById<AmbientLightStateData>( getId() ) )
            {
                return stateData->lowerHemisphere;
            }
        }

        return {};
    }

    void GraphicsScene::setLowerHemisphere( const ColourF &lowerHemisphere )
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->invalidateStateDataById<AmbientLightStateData>( getId() ) )
            {
                stateData->lowerHemisphere = lowerHemisphere;
            }
        }
    }

    Vector3<real_Num> GraphicsScene::getHemisphereDir() const
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto state = context->getStateDataById<AmbientLightStateData>( getId() ) )
            {
                return state->hemisphereDir;
            }
        }

        return Vector3<real_Num>::unit();
    }

    void GraphicsScene::setHemisphereDir( const Vector3<real_Num> &hemisphereDir )
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto state = context->invalidateStateDataById<AmbientLightStateData>( getId() ) )
            {
                state->hemisphereDir = hemisphereDir;
            }
        }
    }

    f32 GraphicsScene::getEnvmapScale() const
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto state = context->getStateDataById<AmbientLightStateData>( getId() ) )
            {
                return state->envmapScale;
            }
        }

        return 1.0f;
    }

    void GraphicsScene::setEnvmapScale( f32 envmapScale )
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto state = context->invalidateStateDataById<AmbientLightStateData>( getId() ) )
            {
                state->envmapScale = envmapScale;
            }
        }
    }

    void GraphicsScene::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsManager = applicationManager->getGraphicsSystemPtr();
        graphicsManager->unlock();
    }

    void GraphicsScene::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsManager = applicationManager->getGraphicsSystemPtr();
        graphicsManager->lock();
    }

    bool GraphicsScene::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsManager = applicationManager->getGraphicsSystemPtr();
        return graphicsManager->try_lock();
    }

    void GraphicsScene::clear()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

            m_isClearing = true;

            // Clear all graphics objects
            auto graphicsObjects = m_graphicsObjects.snapshot();
            for( auto &object : graphicsObjects )
            {
                if( object )
                {
                    graphicsSystem->unloadObject( object );
                }
            }

            m_graphicsObjects.clear();

            // Clear all particle systems
            auto particleSystems = m_particleSystems.snapshot();
            for( auto &system : particleSystems )
            {
                if( system )
                {
                    graphicsSystem->unloadObject( system );
                }
            }

            m_particleSystems.clear();

            // Clear all skies
            auto skies = m_skies.snapshot();
            for( auto &sky : skies )
            {
                if( sky )
                {
                    graphicsSystem->unloadObject( sky );
                }
            }

            m_skies.clear();

            // Reset active camera
            setActiveCamera( nullptr );

            if( auto context = getStateContextPtr() )
            {
                if( auto stateData = context->invalidateStateDataById<GraphicsSceneState>( getId() ) )
                {
                    stateData->activeCamera = nullptr;
                }
            }

            m_isClearing = false;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool GraphicsScene::hasAnimation( const String &animationName )
    {
        auto animations = m_animations.snapshot();
        for( auto &animation : animations )
        {
            if( animation )
            {
                if( animation->getName() == animationName )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool GraphicsScene::destroyAnimation( const String &animationName )
    {
        bool destroyed = false;

        auto it = std::remove_if( m_animations.begin(), m_animations.end(),
                                  [&animationName, &destroyed]( SmartPtr<IAnimation> &animation ) {
                                      if( animation && animation->getName() == animationName )
                                      {
                                          destroyed = true;
                                          return true;
                                      }

                                      return false;
                                  } );
        m_animations.erase( it, m_animations.end() );

        return destroyed;
    }

    SmartPtr<ISharedObject> GraphicsScene::addGraphicsObject( const String &name, const String &type )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

        auto factoryManager = graphicsSystem->getFactoryManagerPtr();
        auto object = factoryManager->make_object<IGraphicsObject>( type );
        if( object )
        {
            object->setName( name );
            object->setCreator( this );
            graphicsSystem->loadObject( object );

            if( object->isDerived<IParticleSystem>() )
            {
                m_particleSystems.push_back( object );
            }
            else
            {
                m_graphicsObjects.push_back( object );
            }

            return object;
        }

        return nullptr;
    }

    SmartPtr<ISharedObject> GraphicsScene::addGraphicsObject( const String &type )
    {
        return addGraphicsObject( type, type );
    }

    SmartPtr<ISharedObject> GraphicsScene::addGraphicsObjectByTypeId( hash_type id )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

        auto factoryManager = getFactoryManagerPtr();
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
            m_terrains.push_back( terrain );
            invalidateSceneState();
            return terrain;
        }
        else
        {
            graphicsSystem->loadObject( object, true );
            m_skies.push_back( workphone::static_pointer_cast<ISky>( object ) );
        }

        return object;
    }

    bool GraphicsScene::removeGraphicsObject( SmartPtr<ISharedObject> graphicsObject )
    {
        if( !graphicsObject || m_isClearing )
            return false;

        if( graphicsObject->isDerived<IGraphicsCamera>() )
        {
            if( getActiveCamera() == graphicsObject )
            {
                setActiveCamera( nullptr );

                if( auto context = getStateContextPtr() )
                {
                    if( auto stateData =
                            context->invalidateStateDataById<GraphicsSceneState>( getId() ) )
                    {
                        stateData->activeCamera = nullptr;
                    }
                }
            }
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

        WP_ASSERT( graphicsObject );

        graphicsSystem->unloadObject( graphicsObject );

        if( graphicsObject->isDerived<IParticleSystem>() )
        {
            m_particleSystems.erase(
                std::remove( m_particleSystems.begin(), m_particleSystems.end(), graphicsObject ),
                m_particleSystems.end() );
        }
        else if( graphicsObject->isDerived<IGraphicsTerrain>() )
        {
            m_terrains.erase( std::remove( m_terrains.begin(), m_terrains.end(), graphicsObject ),
                              m_terrains.end() );
            invalidateSceneState();
        }
        else if( graphicsObject->isDerived<ISky>() )
        {
            m_skies.erase( std::remove( m_skies.begin(), m_skies.end(), graphicsObject ),
                           m_skies.end() );
        }
        else
        {
            m_graphicsObjects.erase(
                std::remove( m_graphicsObjects.begin(), m_graphicsObjects.end(), graphicsObject ),
                m_graphicsObjects.end() );
        }

        return true;
    }

    Array<SmartPtr<IGraphicsObject>> GraphicsScene::getGraphicsObjects() const
    {
        auto objects = m_graphicsObjects.snapshot();
        auto particleSystems = m_particleSystems.snapshot();
        objects.reserve( objects.size() + particleSystems.size() );

        for( auto &particleSystem : particleSystems )
        {
            if( particleSystem )
            {
                objects.push_back( particleSystem );
            }
        }

        return objects;
    }

    Array<SmartPtr<IGraphicsTerrain>> GraphicsScene::getTerrains() const
    {
        auto terrains = m_terrains.snapshot();
        return Array<SmartPtr<IGraphicsTerrain>>( terrains.begin(), terrains.end() );
    }

    void GraphicsScene::invalidateSceneState()
    {
        if( auto stateContext = getStateContextPtr() )
        {
            stateContext->invalidateStateDataById<GraphicsSceneState>( getId() );
        }
    }

    ColourF GraphicsScene::getAmbientLight() const
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->getStateDataById<AmbientLightStateData>( getId() ) )
            {
                return stateData->ambientColour;
            }
        }

        return {};
    }

    void GraphicsScene::setAmbientLight( const ColourF &colour )
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->invalidateStateDataById<AmbientLightStateData>( getId() ) )
            {
                stateData->ambientColour = colour;
            }
        }
    }

    SmartPtr<IGraphicsCamera> GraphicsScene::getActiveCamera() const
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->getStateDataById<GraphicsSceneState>( getId() ) )
            {
                return stateData->activeCamera;
            }
        }

        return nullptr;
    }

    void GraphicsScene::setActiveCamera( SmartPtr<IGraphicsCamera> camera )
    {
        if( isLoaded() )
        {
            if( auto context = getStateContextPtr() )
            {
                if( auto stateData = context->invalidateStateDataById<GraphicsSceneState>( getId() ) )
                {
                    stateData->activeCamera = camera;
                }
            }
        }
    }

    SmartPtr<IGraphicsCamera> GraphicsScene::getDefaultCamera() const
    {
        return m_defaultCamera;
    }

    void GraphicsScene::setDefaultCamera( SmartPtr<IGraphicsCamera> camera )
    {
        m_defaultCamera = camera;
    }

    SmartPtr<IGraphicsSceneNode> GraphicsScene::getSceneNode( const String &name ) const
    {
        auto sceneNodes = m_sceneNodes.snapshot();
        for( auto &sceneNode : sceneNodes )
        {
            if( sceneNode && sceneNode->getName() == name )
            {
                return sceneNode;
            }
        }

        return nullptr;
    }

    SmartPtr<IGraphicsSceneNode> GraphicsScene::getSceneNodeById( hash_type id ) const
    {
        auto sceneNodes = m_sceneNodes.snapshot();
        for( auto &sceneNode : sceneNodes )
        {
            if( sceneNode && sceneNode->getId() == id )
            {
                return sceneNode;
            }
        }

        return nullptr;
    }

    SmartPtr<IGraphicsSceneNode> GraphicsScene::getRootSceneNode() const
    {
        return m_rootSceneNode;
    }

    SmartPtr<IGraphicsSceneNode> GraphicsScene::addSceneNode()
    {
        auto nodeName = sceneNodePrefix + StringUtil::toString( nodeCounter++ );
        return addSceneNode( nodeName );
    }

    SmartPtr<IGraphicsSceneNode> GraphicsScene::addSceneNode( const String &name )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        auto factoryManager = getFactoryManagerPtr();

        if( factoryManager )
        {
            if( auto sceneNode = factoryManager->make_object<IGraphicsSceneNode>() )
            {
                sceneNode->setCreator( this );
                sceneNode->setName( name );
                m_sceneNodes.push_back( sceneNode );
                graphicsSystem->loadObject( sceneNode );
                return sceneNode;
            }
        }

        return nullptr;
    }

    bool GraphicsScene::removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
    {
        if( !sceneNode || m_isClearing )
            return false;

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

        m_sceneNodes.erase( std::remove( m_sceneNodes.begin(), m_sceneNodes.end(), sceneNode ),
                            m_sceneNodes.end() );
        graphicsSystem->unloadObject( sceneNode );

        return true;
    }

    void GraphicsScene::setSkyBox( bool enable, SmartPtr<IMaterial> material, f32 distance,
                                   bool drawFirst )
    {
        // Store skybox settings in scene state
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->invalidateStateDataById<GraphicsSceneState>( getId() ) )
            {
                stateData->enableSkybox = enable;
                stateData->skyboxMaterial = material;
                stateData->skyboxDistance = distance;
                stateData->skyboxDrawFirst = drawFirst;
            }
        }
    }

    void GraphicsScene::setSkyBox( bool enable, SmartPtr<ITexture> texture, f32 distance /*= 5000*/,
                                   bool drawFirst /*= true*/ )
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->invalidateStateDataById<GraphicsSceneState>( getId() ) )
            {
                stateData->enableSkybox = enable;
                stateData->skyboxTexture = texture;
                stateData->skyboxDistance = distance;
                stateData->skyboxDrawFirst = drawFirst;
            }
        }
    }

    void GraphicsScene::setFog( u32 fogMode, const ColourF &colour, f32 expDensity, f32 linearStart,
                                f32 linearEnd )
    {
        // Store fog settings in scene state
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->invalidateStateDataById<GraphicsSceneState>( getId() ) )
            {
                stateData->fogMode = fogMode;
                stateData->fogColour = colour;
                stateData->fogExpDensity = expDensity;
                stateData->fogLinearStart = linearStart;
                stateData->fogLinearEnd = linearEnd;
            }
        }
    }

    bool GraphicsScene::getEnableSkybox() const
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->getStateDataById<GraphicsSceneState>( getId() ) )
            {
                return stateData->enableSkybox;
            }
        }

        return false;
    }

    bool GraphicsScene::getEnableShadows() const
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->getStateDataById<GraphicsSceneState>( getId() ) )
            {
                return stateData->enableShadows;
            }
        }

        return false;
    }

    void GraphicsScene::setEnableShadows( bool enableShadows, bool depthShadows )
    {
        if( auto context = getStateContextPtr() )
        {
            if( auto stateData = context->invalidateStateDataById<GraphicsSceneState>( getId() ) )
            {
                stateData->enableShadows = enableShadows;
                stateData->depthShadows = depthShadows;
            }
        }
    }

    bool GraphicsScene::castRay( const Ray3<real_Num> &ray, Vector3<real_Num> &result )
    {
        // Simple ray casting implementation - can be enhanced based on specific needs
        auto closestDistance = std::numeric_limits<f32>::max();
        auto hit = false;

        for( auto &object : m_graphicsObjects )
        {
            if( object && object->isDerived<IGraphicsMesh>() )
            {
                auto mesh = workphone::static_pointer_cast<IGraphicsMesh>( object );

                Vector3<real_Num> hitPoint;
                real_Num distance;

                if( mesh && mesh->intersects( ray, hitPoint, distance ) )
                {
                    if( distance < closestDistance )
                    {
                        closestDistance = distance;
                        result = hitPoint;
                        hit = true;
                    }
                }
            }
        }

        return hit;
    }

    Array<SmartPtr<IGraphicsMesh>> GraphicsScene::splitMesh( SmartPtr<IGraphicsMesh> mesh,
                                                             const SmartPtr<Properties> &properties )
    {
        Array<SmartPtr<IGraphicsMesh>> result;

        if( !mesh )
        {
            return result;
        }

        // Implementation would depend on specific splitting logic
        // This is a placeholder that returns the original mesh
        result.push_back( mesh );

        return result;
    }

    SmartPtr<IInstanceManager> GraphicsScene::createInstanceManager(
        const String &customName, const String &meshName, const String &groupName, u32 technique,
        u32 numInstancesPerBatch, u16 flags, u16 subMeshIdx )
    {
        // auto manager = addGraphicsObject( customName, "InstanceManager" );
        // auto instanceManager = manager.template get<IInstanceManager>();
        // if( instanceManager )
        //{
        //     instanceManager->setMeshName( meshName );
        //     instanceManager->setGroupName( groupName );
        //     instanceManager->setTechnique( technique );
        //     instanceManager->setNumInstancesPerBatch( numInstancesPerBatch );
        //     instanceManager->setFlags( flags );
        //     instanceManager->setSubMeshIndex( subMeshIdx );
        // }

        return nullptr;
    }

    SmartPtr<IInstancedObject> GraphicsScene::createInstancedObject( const String &materialName,
                                                                     const String &managerName )
    {
        // auto object = addGraphicsObject( "InstancedObject", "InstancedObject" );
        // auto instancedObject = object.template get<IInstancedObject>();
        // if( instancedObject )
        //{
        //     instancedObject->setMaterialName( materialName );
        //     instancedObject->setManagerName( managerName );
        // }
        // return instancedObject;

        return nullptr;
    }

    void GraphicsScene::destroyInstancedObject( SmartPtr<IInstancedObject> instancedObject )
    {
    }

    SmartPtr<IFactoryManager> GraphicsScene::getFactoryManager() const
    {
        return m_factoryManager;
    }

    void GraphicsScene::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

    bool GraphicsScene::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        auto cameras = m_cameras.snapshot();
        for( auto &camera : cameras )
        {
            if( camera )
            {
                if( camera->handleStateMessage( message ) )
                {
                    return true;
                }
            }
        }

        auto graphicsObjects = m_graphicsObjects.snapshot();
        for( auto &graphicsObject : graphicsObjects )
        {
            if( graphicsObject )
            {
                if( graphicsObject->handleStateMessage( message ) )
                {
                    return true;
                }
            }
        }

        auto sceneNodes = m_sceneNodes.snapshot();
        for( auto &sceneNode : sceneNodes )
        {
            if( sceneNode )
            {
                if( sceneNode->handleStateMessage( message ) )
                {
                    return true;
                }
            }
        }

        auto skies = m_skies.snapshot();
        for( auto &sky : skies )
        {
            if( sky )
            {
                if( sky->handleStateMessage( message ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool GraphicsScene::handleStateChanged( SmartPtr<IState> &state )
    {
        if( !state || !isLoaded() )
        {
            return false;
        }

        ScopedLock lock( this );

        // State owners are intrusive weak pointers.  A queued state update can outlive its
        // owner, so locking the owner here would first have to dereference potentially
        // reclaimed memory.  Match the stored address against objects kept alive by this
        // scene instead; only a live matching object may receive the update.
        auto owner = state->getOwnerPtr();
        if( !owner )
        {
            return false;
        }

        for( auto &camera : m_cameras )
        {
            if( camera && camera->handleStateChanged( state ) )
            {
                return true;
            }
        }

        for( auto &graphicsObject : m_graphicsObjects )
        {
            if( graphicsObject && graphicsObject->handleStateChanged( state ) )
            {
                return true;
            }
        }

        for( auto &sceneNode : m_sceneNodes )
        {
            if( sceneNode && sceneNode->handleStateChanged( state ) )
            {
                return true;
            }
        }

        for( auto &sky : m_skies )
        {
            if( sky && sky->handleStateChanged( state ) )
            {
                return true;
            }
        }

        return false;
    }

    StringPool<c8> *GraphicsScene::getStringPool() const
    {
        return m_stringPool;
    }

    void GraphicsScene::setStringPool( StringPool<c8> *pool )
    {
        m_stringPool = pool;
    }

    void GraphicsScene::_getObject( void **ppObject ) const
    {
        *ppObject = const_cast<GraphicsScene *>( this );
    }

    SmartPtr<IStateContext> GraphicsScene::getSceneNodeContext() const
    {
        return m_sceneNodeContext;
    }

    void GraphicsScene::setSceneNodeContext( SmartPtr<IStateContext> context )
    {
        m_sceneNodeContext = context;
    }

    SmartPtr<IStateContext> GraphicsScene::getGraphicsObjectContext( u32 typeId ) const
    {
        const auto it = m_graphicsObjectContexts.find( typeId );
        if( it != m_graphicsObjectContexts.end() )
        {
            return it->second.lock();
        }

        return nullptr;
    }

    void GraphicsScene::setGraphicsObjectContext( u32 typeId, SmartPtr<IStateContext> context )
    {
        m_graphicsObjectContexts[typeId] = context;
    }

    void GraphicsScene::StateListener::setOwner( SmartPtr<GraphicsScene> owner )
    {
        m_owner = owner;
    }

    SmartPtr<GraphicsScene> GraphicsScene::StateListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    bool GraphicsScene::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwnerPtr() )
        {
            return owner->handleStateChanged( state );
        }

        return false;
    }

    bool GraphicsScene::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwnerPtr() )
        {
            return owner->handleStateMessage( message );
        }

        return false;
    }

    void GraphicsScene::StateListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    GraphicsScene::StateListener::StateListener() = default;

    GraphicsScene::StateListener::~StateListener() = default;

}  // namespace workphone::render
