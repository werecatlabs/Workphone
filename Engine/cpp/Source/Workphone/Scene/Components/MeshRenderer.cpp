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
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <Workphone/System/ResourceChunkStream.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <thread>
#include <Workphone/Thread/ScopedLock.hpp>
#include <stdexcept>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, MeshRenderer, Renderer );

    const String MeshRenderer::meshNodeSuffixStr = "_MeshComponent";
    const String MeshRenderer::meshPathStr = "meshPath";

    struct MeshRenderer::RuntimeLODData
    {
        StreamBudget budget;
        std::unique_ptr<resource::ResourceChunkStream> stream;
        std::thread::id owner;
        std::shared_ptr<const resource::RuntimeResourceChunk> ready;
        u64 metadataBytes=0;
        bool requested=false,failed=false;
        String error;
        ~RuntimeLODData(){if(stream)stream->retireAfterOwnerQuiesced();}
    };

    MeshRenderer::MeshRenderer() = default;

    MeshRenderer::~MeshRenderer() = default;

    bool MeshRenderer::isRuntimeLODStreamed() const{return m_streamMesh;}
    bool MeshRenderer::hasRuntimeLODResidency() const{return getGraphicsObject()!=nullptr;}
    String MeshRenderer::runtimeLODError() const{return m_runtimeLOD?m_runtimeLOD->error:String();}
    bool MeshRenderer::prepareRuntimeLOD(String &error,s32 priority)
    {
        error.clear();if(!m_streamMesh)return hasRuntimeLODResidency();
        if(hasRuntimeLODResidency())return true;
        try {
            if(!m_runtimeAssets||!m_runtimeAssets->mount||!m_runtimeResource||!m_runtimeResource->payload.empty()||
               m_runtimeResource->header.resourceType!=resource::ResourceTypeID("mesh"))throw std::runtime_error("Streamed LOD requires captured static mesh metadata and mount");
            if(!m_runtimeLOD){auto budget=m_runtimeAssets->mount->reserveOwner(StreamMemoryClass::SourceCpu,sizeof(RuntimeLODData)+4096,error);if(!budget)return false;
                auto next=std::make_unique<RuntimeLODData>();next->budget=std::move(budget);m_runtimeLOD=std::move(next);}
            auto &state=*m_runtimeLOD;
            if(state.failed){error=state.error;return false;}
            if(!state.stream||state.owner!=std::this_thread::get_id()){
                if(state.stream)state.stream->retireAfterOwnerQuiesced();state.stream.reset();state.requested=false;
                auto mount=m_runtimeAssets->mount->createReadOnlyOwnerView(error);if(!mount)return false;
                state.stream=resource::ResourceChunkStream::create(std::move(mount),error);if(!state.stream)return false;state.owner=std::this_thread::get_id();
            }
            const auto &header=m_runtimeResource->header;
            if(!state.requested&&!state.ready){resource::RuntimeResourceRangeInfo info;if(!state.stream->resourceInfo(header.resourceId,info,error))return false;
                if(info.header.sourceHash!=header.sourceHash||info.header.payloadHash!=header.payloadHash||info.header.payloadSize!=header.payloadSize||
                   info.header.compilerVersion!=header.compilerVersion||info.header.resourceType!=header.resourceType)throw std::runtime_error("LOD mesh metadata differs from its captured package generation");
                state.metadataBytes=info.metadataBytes;
                if(!state.stream->request(1,header.resourceId,0,header.payloadSize,priority,priority,error))return false;state.requested=true;
            }else state.stream->setPriority(1,priority);
            if(!state.ready){state.stream->process();resource::ResourceChunkResult result;if(!state.stream->take(result))return false;state.requested=false;
                if(!result.resource||result.resourceId!=header.resourceId||result.offset!=0||result.resource->bytes.size()!=header.payloadSize||
                   resource::hashBytes(result.resource->bytes.data(),result.resource->bytes.size())!=header.payloadHash)
                    throw std::runtime_error(result.error.empty()?"Streamed LOD mesh integrity failure":result.error.c_str());state.ready=std::move(result.resource);}
            auto admission=state.stream->reserveOwner(StreamMemoryClass::SourceCpu,header.payloadSize+state.metadataBytes+8192,error);if(!admission)return false;
            struct Candidate {StreamBudget budget;resource::RuntimeResource value;};auto holder=std::make_shared<Candidate>();holder->budget=std::move(admission);
            holder->value.header=header;holder->value.dependencies=m_runtimeResource->dependencies;holder->value.payload=state.ready->bytes;
            auto candidate=std::shared_ptr<const resource::RuntimeResource>(holder,&holder->value);auto metadata=m_runtimeResource;
            if(!installRuntimeMesh(candidate,error)){m_runtimeResource=metadata;return false;}
            m_runtimeResource=std::move(metadata);state.stream->retireAfterOwnerQuiesced();state.stream.reset();state.ready.reset();
            setCastShadows(getCastShadows());setZOrder(getZOrder());setVisibilityFlags(getVisibilityFlags());updateMaterials();updateTransform();return true;
        }catch(const std::exception &failure){error=failure.what();if(getGraphicsObject())releaseRuntimeLOD();if(m_runtimeLOD){m_runtimeLOD->failed=true;m_runtimeLOD->error=error.substr(0,2048);if(m_runtimeLOD->stream)m_runtimeLOD->stream->retireAfterOwnerQuiesced();m_runtimeLOD->stream.reset();m_runtimeLOD->ready.reset();m_runtimeLOD->requested=false;}return false;}
    }
    void MeshRenderer::releaseRuntimeLOD()
    {
        if(!m_streamMesh)return;
        m_runtimeLOD.reset();
        auto app=core::IApplicationManager::instancePtr();auto graphics=app?app->getGraphicsSystemPtr():nullptr;auto scene=graphics?graphics->getGraphicsScenePtr():nullptr;
        if(!scene)return;
        ScopedLock lock(graphics);
        auto mesh=getGraphicsObjectByType<render::IGraphicsMesh>();auto node=getMeshNode();
        if(mesh){mesh->setVisible(false);if(node)node->detachObject(mesh);mesh->unload(nullptr);scene->removeGraphicsObject(mesh);}
        if(node)scene->removeSceneNode(node);setGraphicsObject(nullptr);setMeshNode(nullptr);
    }

    void MeshRenderer::configureRuntimeAssets(std::shared_ptr<const resource::RuntimeAssetContext> context)
    {
        if(!context || !context->resolve || isLoaded() || getLoadingState()==LoadingState::Loading ||
           getLoadingState()==LoadingState::Unloading || (m_runtimeAssets && m_runtimeAssets!=context))
            throw std::runtime_error("MeshRenderer runtime assets require an unloaded component and resolver");
        m_runtimeAssets=std::move(context);m_runtimeAssetsMode=true;
    }

    bool MeshRenderer::installRuntimeMesh(std::shared_ptr<const resource::RuntimeResource> resource,String &error)
    {
#if WP_TYPED_ANIMATION_RESOURCE
        if(!m_runtimeAssets || !resource){error="Missing captured runtime mesh/context";return false;}
        const auto type=resource->header.resourceType;
        if(type!=workphone::resource::ResourceTypeID("mesh") && type!=workphone::resource::ResourceTypeID("skinres"))
        {error="MeshRenderer UUID requires a mesh or skinres resource";return false;}
        if(resource==m_runtimeResource && getGraphicsObject())return true;
        auto app=core::IApplicationManager::instancePtr();auto graphics=app?app->getGraphicsSystemPtr():nullptr;
        auto scene=graphics?graphics->getGraphicsScene():nullptr;auto actor=getActor();
        if(!scene || !actor){error="Runtime MeshRenderer requires its actor and graphics scene";return false;}
        ScopedLock lock(graphics);
        if(auto mesh=getGraphicsObjectByType<render::IGraphicsMesh>()){
            if(!mesh->loadResource(resource,error,m_runtimeAssets->target,m_runtimeAssets->packagedBuild,m_runtimeAssets->mount))return false;
            m_runtimeResource=std::move(resource);return true;
        }
        SmartPtr<render::IGraphicsMesh> mesh;
        SmartPtr<render::IGraphicsSceneNode> node;
        try {
            mesh=scene->addGraphicsObjectByType<render::IGraphicsMesh>();
            if(!mesh)throw std::runtime_error("Cannot allocate runtime graphics mesh");
            mesh->setVisible(false);
            if(!mesh->loadResource(resource,error,m_runtimeAssets->target,m_runtimeAssets->packagedBuild,m_runtimeAssets->mount))throw std::runtime_error(error.c_str());
            auto root=scene->getRootSceneNode();if(!root)throw std::runtime_error("Runtime graphics scene has no root node");
            node=root->addChildSceneNode(actor->getName()+"_RuntimeMesh_"+StringUtil::toString(m_idExt++));
            if(!node)throw std::runtime_error("Cannot allocate runtime mesh scene node");
            node->attachObject(mesh);mesh->setVisibilityFlags(render::IGraphicsObject::SceneFlag);
            setGraphicsObject(mesh);setMeshNode(node);
            m_runtimeResource=std::move(resource);return true;
        }catch(const std::exception &failure){
            if(node){node->detachAllObjects();scene->removeSceneNode(node);}
            if(mesh){mesh->unload(nullptr);scene->removeGraphicsObject(mesh);}
            setGraphicsObject(nullptr);setMeshNode(nullptr);error=failure.what();return false;
        }
#else
        error="Cooked mesh resources are unavailable in this build";return false;
#endif
    }

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
            if(m_runtimeAssetsMode && getLoadingState()==LoadingState::Error)
                throw std::runtime_error("Runtime MeshRenderer initialization failed");

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
            if(m_runtimeAssetsMode){setLoadingState(LoadingState::Error);throw;}
        }
    }

    void MeshRenderer::unload( SmartPtr<ISharedObject> data )
    {
        if(m_streamMesh)releaseRuntimeLOD();
        m_runtimeResource.reset();m_runtimeAssets.reset();
        try
        {
            const auto loadingState = getLoadingState();
            if( (loadingState == LoadingState::Allocated || loadingState == LoadingState::Unloaded) && !getGraphicsObject() )
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
        if(properties)properties->setProperty("meshAssetUuid",m_meshAssetUuid,Properties::resourceStr);
        if(properties)properties->setProperty("streamMesh",m_streamMesh);
        return properties;
    }

    void MeshRenderer::setProperties( SmartPtr<Properties> properties )
    {
        if(m_runtimeAssetsMode){
            if(!properties || !m_runtimeAssets)throw std::runtime_error("Missing runtime MeshRenderer context/properties");
            String uuid=m_meshAssetUuid,error;properties->getPropertyValue("meshAssetUuid",uuid);
            bool streamed=m_streamMesh;properties->getPropertyValue("streamMesh",streamed);
            std::shared_ptr<const resource::RuntimeResource> resource;
            if(!uuid.empty())resource=m_runtimeAssets->resolve(uuid,error);
            else if(auto actor=getActor())if(auto mesh=actor->getComponent<Mesh>())resource=mesh->getRuntimeResource();
            if(!resource)throw std::runtime_error(error.empty()?"Runtime MeshRenderer requires a captured mesh UUID":error.c_str());
            if(streamed){if(!m_runtimeAssets->mount||uuid.empty()||resource->header.resourceType!=workphone::resource::ResourceTypeID("mesh")||!resource->payload.empty())
                    throw std::runtime_error("streamMesh requires an explicit captured static mesh UUID with sparse payload metadata");
                if(isLoaded()&&(!m_streamMesh||uuid!=m_meshAssetUuid))throw std::runtime_error("Loaded streamMesh bindings cannot be changed");
                m_runtimeResource=resource;m_streamMesh=true;setLODVisible(false);
            }else {if(m_streamMesh)throw std::runtime_error("Runtime streamed mesh mode cannot be downgraded");if(!installRuntimeMesh(resource,error))throw std::runtime_error(error.c_str());}
            m_meshAssetUuid=std::move(uuid);
        }
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
        if(m_runtimeAssetsMode){
            // A Mesh sibling may notify us before our own UUID properties load.
            // setProperties stages the selected asset during real initialization.
            if(getLoadingState()==LoadingState::Allocated)return;
            if(!m_runtimeAssets)throw std::runtime_error("Runtime MeshRenderer requires its captured asset context");
            if(m_streamMesh)return;
            String error;std::shared_ptr<const resource::RuntimeResource> resource;
            if(!m_meshAssetUuid.empty())resource=m_runtimeAssets->resolve(m_meshAssetUuid,error);
            else if(auto actor=getActor())if(auto mesh=actor->getComponent<Mesh>())resource=mesh->getRuntimeResource();
            if(!resource || !installRuntimeMesh(resource,error))throw std::runtime_error(error.empty()?"Runtime mesh UUID is unavailable":error.c_str());
            return;
        }
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
        setGraphicsNode( meshNode );
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
        if(m_runtimeAssetsMode)if(auto mesh=getGraphicsObjectByType<render::IGraphicsMesh>())return mesh->getLocalAABB();
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
