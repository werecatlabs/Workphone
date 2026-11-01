#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Scene/Components/MeshRenderer.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Mesh/ProgressiveMeshOptions.hpp>
#include <Workphone/System/RuntimeAssetContext.hpp>
#include <Workphone/System/CompiledResource.hpp>
#include <stdexcept>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Mesh, IComponent );

    u32 Mesh::m_idExt = 0;

    const String Mesh::m_meshPathStr = String( "Mesh Path" );
    const String Mesh::m_meshStr = String( "Mesh Resource" );
    const String Mesh::m_skeletonStr = String( "Skeleton" );
    const String Mesh::m_fsmPriorityStr = String( "FSM Priority" );
    const String Mesh::m_progressiveMeshOptionsStr = String( "Progressive Mesh Options" );

    Mesh::Mesh() = default;

    Mesh::~Mesh() = default;

    void Mesh::configureRuntimeAssets(std::shared_ptr<const resource::RuntimeAssetContext> context)
    {
        if(!context || !context->resolve || isLoaded() || getLoadingState()==LoadingState::Loading ||
           getLoadingState()==LoadingState::Unloading || (m_runtimeAssets && m_runtimeAssets!=context))
            throw std::runtime_error("Mesh runtime assets require an unloaded component and resolver");
        m_runtimeAssets=std::move(context);m_runtimeAssetsMode=true;
    }
    std::shared_ptr<const resource::RuntimeResource> Mesh::getRuntimeResource() const {return m_runtimeResource;}
    std::shared_ptr<const resource::RuntimeResource> Mesh::getRuntimeSkeletonResource() const {return m_runtimeSkeleton;}

    void Mesh::load( SmartPtr<ISharedObject> data )
    {
        if(m_runtimeAssetsMode && !m_runtimeAssets)throw std::runtime_error("Mesh runtime assets must be reconfigured before load");
        setLoadingState( LoadingState::Loading );
        Component::load( data );
        if(m_runtimeAssetsMode){setLoadingState(LoadingState::Loaded);return;}

        if( m_componentFSM )
        {
            m_componentFSM->setPriority( m_fsmPriority );
        }

        if( data )
        {
            if( data->isDerived<Properties>() )
            {
                auto properties = workphone::static_pointer_cast<Properties>( data );

                auto meshPath = getMeshPath();

                properties->getPropertyValue( m_meshPathStr, meshPath );
                properties->getPropertyAsType( m_meshStr, m_meshResource );
                properties->getPropertyAsType( m_skeletonStr, m_skeleton );

                const auto progressiveMeshProperties =
                    properties->getChildrenByName( m_progressiveMeshOptionsStr );
                if( !progressiveMeshProperties.empty() )
                {
                    m_progressiveMeshOptions.fromProperties( progressiveMeshProperties.front() );
                }

                setMeshPath( meshPath );
            }
        }

        setLoadingState( LoadingState::Loaded );
    }

    void Mesh::unload( SmartPtr<ISharedObject> data )
    {
        m_runtimeResource.reset();m_runtimeSkeleton.reset();m_runtimeAssets.reset();
        try
        {
            setLoadingState( LoadingState::Unloading );

            Component::unload( data );

            m_meshResource = nullptr;
            m_meshPath.clear();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    String Mesh::getMeshPath() const
    {
        return m_meshPath;
    }

    void Mesh::setMeshPath( const String &meshPath )
    {
        if(m_runtimeAssetsMode){if(!meshPath.empty())throw std::runtime_error("Runtime mesh paths cannot load authoring files");m_meshPath.clear();return;}
        m_meshPath = meshPath;

        if( !StringUtil::isNullOrEmpty( meshPath ) )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto resourceDatabase = applicationManager->getResourceDatabase();

            m_meshResource = resourceDatabase->loadResourceByType<IMeshResource>( meshPath );
        }
        else
        {
            m_meshResource = nullptr;
        }

        if( auto actor = getActor() )
        {
            auto meshRenderer = actor->getComponent<MeshRenderer>();
            if( meshRenderer )
            {
                meshRenderer->updateMesh();
            }
        }
    }

    SmartPtr<IMeshResource> Mesh::getMeshResource() const
    {
        return m_meshResource;
    }

    void Mesh::setMeshResource( SmartPtr<IMeshResource> meshResource )
    {
        if(m_runtimeAssetsMode && meshResource)throw std::runtime_error("Runtime meshes require their captured UUID resource");
        m_meshResource = meshResource;

        if( m_meshResource )
        {
            auto meshFilePath = m_meshResource->getFilePath();
            m_meshPath = meshFilePath;
        }

        if( auto actor = getActor() )
        {
            auto meshRenderer = actor->getComponent<MeshRenderer>();
            if( meshRenderer )
            {
                meshRenderer->updateMesh();
            }
        }
    }

    SmartPtr<ISkeleton> Mesh::getSkeleton() const
    {
        return m_skeleton;
    }

    void Mesh::setSkeleton( SmartPtr<ISkeleton> skeleton )
    {
        m_skeleton = skeleton;
    }

    const ProgressiveMeshOptions &Mesh::getProgressiveMeshOptions() const
    {
        return m_progressiveMeshOptions;
    }

    void Mesh::setProgressiveMeshOptions( const ProgressiveMeshOptions &options )
    {
        if( m_progressiveMeshOptions == options )
        {
            return;
        }

        m_progressiveMeshOptions = options;
        if( auto actor = getActor() )
        {
            if( auto meshRenderer = actor->getComponent<MeshRenderer>() )
            {
                meshRenderer->updateMesh();
            }
        }
    }

    SmartPtr<Properties> Mesh::getProperties() const
    {
        auto properties = Component::getProperties();

        auto meshPath = getMeshPath();
        properties->setProperty( m_meshPathStr, meshPath );

        auto meshResource = getMeshResource();
        if(m_runtimeAssetsMode)properties->setProperty(m_meshStr,m_runtimeMeshUuid,Properties::resourceStr);
        else properties->setPropertyAsType( m_meshStr, meshResource );

        auto skeleton = getSkeleton();
        if(m_runtimeAssetsMode)properties->setProperty(m_skeletonStr,m_runtimeSkeletonUuid,Properties::resourceStr);
        else properties->setPropertyAsType( m_skeletonStr, skeleton );

        auto &meshPathProperty = properties->getPropertyObject( m_meshPathStr );
        meshPathProperty.setTypeName( "file" );

        properties->setProperty( m_fsmPriorityStr, m_fsmPriority );
        properties->addChild( m_progressiveMeshOptions.toProperties( m_progressiveMeshOptionsStr ) );

        return properties;
    }

    void Mesh::setProperties( SmartPtr<Properties> properties )
    {
        if(m_runtimeAssetsMode)
        {
            if(!properties || !m_runtimeAssets)throw std::runtime_error("Missing runtime Mesh asset context/properties");
            String meshId=m_runtimeMeshUuid,skeletonId=m_runtimeSkeletonUuid,path,error;
            properties->getPropertyValue(m_meshStr,meshId);properties->getPropertyValue(m_skeletonStr,skeletonId);
            properties->getPropertyValue(m_meshPathStr,path);
            if(!path.empty())throw std::runtime_error("Runtime Mesh forbids authoring paths");
            if(meshId.empty())throw std::runtime_error("Runtime Mesh requires its captured mesh UUID");
            auto mesh=m_runtimeAssets->resolve(meshId,error);
            if(!mesh)throw std::runtime_error(error.empty()?"Missing captured mesh UUID":error.c_str());
#if WP_TYPED_ANIMATION_RESOURCE
            if(mesh->header.resourceType!=resource::ResourceTypeID("mesh") && mesh->header.resourceType!=resource::ResourceTypeID("skinres"))
                throw std::runtime_error("Mesh UUID requires a mesh or skinres resource");
#else
            throw std::runtime_error("Cooked mesh resources are unavailable in this build");
#endif
            std::shared_ptr<const resource::RuntimeResource> skeleton;
            if(!skeletonId.empty()){
                skeleton=m_runtimeAssets->resolve(skeletonId,error);
                if(!skeleton
#if WP_TYPED_ANIMATION_RESOURCE
                   || skeleton->header.resourceType!=resource::ResourceTypeID("skelres")
#endif
                   )
                    throw std::runtime_error(error.empty()?"Skeleton UUID requires a skelres resource":error.c_str());
            }
            m_runtimeResource.swap(mesh);m_runtimeSkeleton.swap(skeleton);
            try {if(auto actor=getActor())if(auto renderer=actor->getComponent<MeshRenderer>())renderer->updateMesh();}
            catch(...){m_runtimeResource.swap(mesh);m_runtimeSkeleton.swap(skeleton);throw;}
            m_runtimeMeshUuid=std::move(meshId);m_runtimeSkeletonUuid=std::move(skeletonId);m_meshPath.clear();
            m_meshResource=nullptr;m_skeleton=nullptr;
            s32 priority=m_fsmPriority;properties->getPropertyValue(m_fsmPriorityStr,priority);setFsmPriority(priority);
            Component::setProperties(properties);return;
        }
        Component::setProperties( properties );

        auto applicationManager = core::IApplicationManager::instance();
        auto resourceDatabase = applicationManager->getResourceDatabase();

        auto meshPath = getMeshPath();
        properties->getPropertyValue( m_meshPathStr, meshPath );

        auto meshResource = getMeshResource();
        properties->getPropertyAsType( m_meshStr, meshResource );

        auto skeleton = getSkeleton();
        properties->getPropertyAsType( m_skeletonStr, skeleton );

        auto progressiveMeshOptions = getProgressiveMeshOptions();
        const auto progressiveMeshProperties =
            properties->getChildrenByName( m_progressiveMeshOptionsStr );
        if( !progressiveMeshProperties.empty() )
        {
            progressiveMeshOptions.fromProperties( progressiveMeshProperties.front() );
        }

        if( meshResource && meshResource != getMeshResource() )
        {
            m_meshResource = meshResource;

            auto meshFilePath = meshResource->getFilePath();
            m_meshPath = meshFilePath;

            if( auto actor = getActor() )
            {
                auto meshRenderer = actor->getComponent<MeshRenderer>();
                if( meshRenderer )
                {
                    meshRenderer->updateMesh();
                }
            }
        }
        else if( meshPath != getMeshPath() )
        {
            meshPath = StringUtil::cleanupPath( meshPath );
            m_meshPath = meshPath;

            if( !StringUtil::isNullOrEmpty( meshPath ) )
            {
                auto meshResourceResult =
                    resourceDatabase->loadResourceByType<IMeshResource>( meshPath );
                m_meshResource = meshResourceResult;
            }

            if( auto actor = getActor() )
            {
                auto meshRenderer = actor->getComponent<MeshRenderer>();
                if( meshRenderer )
                {
                    meshRenderer->updateMesh();
                }
            }
        }

        setProgressiveMeshOptions( progressiveMeshOptions );

        s32 fsmPriority = m_fsmPriority;
        properties->getPropertyValue( m_fsmPriorityStr, fsmPriority );
        if( fsmPriority != m_fsmPriority )
        {
            m_fsmPriority = fsmPriority;
            if( m_componentFSM )
            {
                m_componentFSM->setPriority( m_fsmPriority );
            }
        }
    }

    s32 Mesh::getFsmPriority() const
    {
        return m_fsmPriority;
    }

    void Mesh::setFsmPriority( s32 priority )
    {
        m_fsmPriority = priority;

        if( m_componentFSM )
        {
            m_componentFSM->setPriority( m_fsmPriority );
        }
    }

    FSMReturnType Mesh::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        Component::handleComponentEvent( state, eventType );

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
            case State::Edit:
            case State::Play:
            {
            }
            break;
            default:
            {
            }
            break;
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
            break;
            }
        }
        break;
        case FSMEvent::Pending:
        {
        }
        break;
        case FSMEvent::Complete:
        {
        }
        break;
        case FSMEvent::NewState:
        {
        }
        break;
        case FSMEvent::WaitForChange:
        {
        }
        break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

}  // namespace workphone::scene
