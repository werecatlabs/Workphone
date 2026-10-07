#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/Material.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/State/Messages/StateMessageLoad.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, MeshRenderer, Renderer );

    const String MeshRenderer::meshNodeSuffixStr = "_MeshComponent";
    const String MeshRenderer::meshPathStr = "meshPath";

    MeshRenderer::MeshRenderer() = default;

    MeshRenderer::~MeshRenderer() = default;

    void MeshRenderer::updateFlags( u32 flags, u32 oldFlags )
    {
        if( auto actor = getActorPtr() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
            }
        }

        Renderer::updateFlags( flags, oldFlags );
    }

    void MeshRenderer::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            Renderer::load( data );

            if( !getGraphicsObject() )
            {
                updateMesh();
                updateMaterials();
                updateTransform();
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MeshRenderer::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Allocated || loadingState == LoadingState::Unloaded )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto gameManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( gameManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                auto smgr = graphicsSystem->getGraphicsScenePtr();
                if( !smgr )
                {
                    setMeshNode( nullptr );
                    setGraphicsObject( nullptr );
                    if( gameManager )
                    {
                        gameManager->unregisterAllComponent( this );
                    }
                    Renderer::unload( data );
                    setLoadingState( LoadingState::Unloaded );
                    return;
                }

                if( auto meshNode = getMeshNode() )
                {
                    meshNode->detachAllObjects();

                    if( auto meshObject = getGraphicsObjectByType<render::IGraphicsMesh>() )
                    {
                        meshNode->detachObject( meshObject );
                        smgr->removeGraphicsObject( meshObject );
                    }

                    smgr->removeSceneNode( meshNode );

                    setMeshNode( nullptr );
                    setGraphicsObject( nullptr );
                }
                else
                {
                    setMeshNode( nullptr );
                    setGraphicsObject( nullptr );
                }
            }

            if( gameManager )
            {
                gameManager->unregisterAllComponent( this );
            }

            Renderer::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MeshRenderer::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = Renderer::getChildObjects();
        //objects.reserve( 12 );

        //Commented out because these are add in the Renderer::getChildObjects() function
        // Left for debugging purposes
        //objects.emplace_back( m_meshObject );
        //objects.emplace_back( m_graphicsNode );
        //objects.emplace_back( m_sharedMaterial );
        return objects;
    }

    auto MeshRenderer::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Renderer::getProperties();
        return properties;
    }

    void MeshRenderer::setProperties( SmartPtr<Properties> properties )
    {
        Renderer::setProperties( properties );

        updateMesh();
        updateMaterials();
        updateVisibility();
    }

    auto MeshRenderer::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        Renderer::handleComponentEvent( state, eventType );

        switch( eventType )
        {
        case FSMEvent::Change:
        {
        }
        break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Destroyed:
            {
            }
            break;
            case State::Edit:
            case State::Play:
            {
                if( !getGraphicsObject() )
                {
                    updateMesh();
                    updateMaterials();
                    updateTransform();
                }

                updateVisibility();
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
            }
            break;
            default:
            {
            }
            }
        }
        break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    void MeshRenderer::updateMesh()
    {
        try
        {
            auto actor = getActorPtr();
            if( !actor || !isEnabled() || !actor->isEnabledInScene() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( !graphicsSystem )
            {
                WP_LOG_ERROR( "Graphics system not available" );
                return;
            }

            auto graphicsScene = graphicsSystem->getGraphicsScene();
            WP_ASSERT( graphicsScene );

            auto meshComponent = actor->getComponent<Mesh>();
            if( !meshComponent )
            {
                return;
            }

            const auto meshPath = meshComponent->getMeshPath();
            if( StringUtil::isNullOrEmpty( meshPath ) )
            {
                return;
            }

            const auto &progressiveMeshOptions = meshComponent->getProgressiveMeshOptions();
            if( auto meshObject = getGraphicsObjectByType<render::IGraphicsMesh>() )
            {
                const auto needsReload =
                    meshObject->getMeshName() != meshPath ||
                    meshObject->getProgressiveMeshOptions() != progressiveMeshOptions;
                meshObject->setMeshName( meshPath );
                meshObject->setProgressiveMeshOptions( progressiveMeshOptions );
                if( needsReload )
                {
                    graphicsSystem->reloadObject( meshObject, true );
                }
                return;
            }

            auto meshObject = graphicsScene->addGraphicsObjectByType<render::IGraphicsMesh>();
            if( !meshObject )
            {
                WP_LOG_ERROR( "Mesh not created: " + meshPath );
                return;
            }

            // A mesh needs its resource name before the render backend can load it.
            meshObject->setMeshName( meshPath );
            meshObject->setProgressiveMeshOptions( progressiveMeshOptions );
            setGraphicsObject( meshObject );
            graphicsSystem->loadObject( meshObject, true );

            const auto nodeName =
                actor->getName() + String( "_MeshComponent" ) + StringUtil::toString( m_idExt++ );
            auto meshNode = graphicsScene->getRootSceneNode()->addChildSceneNode( nodeName );
            WP_ASSERT( meshNode );

            meshNode->attachObject( meshObject );
            meshObject->setVisibilityFlags( render::IGraphicsObject::SceneFlag );
            setMeshNode( meshNode );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MeshRenderer::updateMaterials()
    {
        if( auto actor = getActor() )
        {
            auto materials = actor->getComponentsByType<Material>();
            for( auto &material : materials )
            {
                if( material )
                {
                    auto materialResource = material->getMaterial();
                    if( materialResource )
                    {
                        if( auto meshObject = getGraphicsObjectByType<render::IGraphicsMesh>() )
                        {
                            auto index = material->getIndex();
                            meshObject->setMaterial( materialResource, index );
                        }
                    }
                }
            }
        }
    }

    auto MeshRenderer::getMeshNode() const -> SmartPtr<render::IGraphicsSceneNode>
    {
        return m_graphicsNode;
    }

    void MeshRenderer::setMeshNode( SmartPtr<render::IGraphicsSceneNode> meshNode )
    {
        m_graphicsNode = meshNode;
    }

    Parameter MeshRenderer::handleEvent( EventType eventType, hash_type eventValue,
                                         const Array<Parameter> &arguments,
                                         SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                         SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::meshLoaded && object && object->isDerived<IMeshResource>() )
        {
            if( auto actor = getActorPtr() )
            {
                if( auto meshComponent = actor->getComponent<Mesh>() )
                {
                    auto meshResource = meshComponent->getMeshResource();
                    if( meshResource )
                    {
                        auto meshFilePathA = meshResource->getFilePath();

                        auto newMeshResource = workphone::static_pointer_cast<IMeshResource>( object );
                        auto meshFilePathB = newMeshResource->getFilePath();

                        if( meshFilePathA == meshFilePathB )
                        {
                            if( auto meshObject = getGraphicsObjectByType<render::IGraphicsMesh>() )
                            {
                                auto applicationManager = core::IApplicationManager::instancePtr();
                                if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
                                {
                                    // Native mesh and GPU cache replacement belongs to the render task.
                                    graphicsSystem->reloadObject( meshObject, true );
                                }
                            }
                        }
                    }
                }
            }
        }

        return Renderer::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

    AABB3<real_Num> MeshRenderer::calculateBoundingBox()
    {
        auto boundingBox = AABB3<real_Num>();
        if( auto actor = getActorPtr() )
        {
            if( auto meshComponent = actor->getComponent<Mesh>() )
            {
                if( auto meshResource = meshComponent->getMeshResource() )
                {
                    if( auto mesh = meshResource->getMesh() )
                    {
                        boundingBox = mesh->getAABB();
                    }
                }
            }
        }

        return boundingBox;
    }

}  // namespace workphone::scene
