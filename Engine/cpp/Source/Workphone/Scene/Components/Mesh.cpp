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

    void Mesh::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Component::load( data );

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
        properties->setPropertyAsType( m_meshStr, meshResource );

        auto skeleton = getSkeleton();
        properties->setPropertyAsType( m_skeletonStr, skeleton );

        auto &meshPathProperty = properties->getPropertyObject( m_meshPathStr );
        meshPathProperty.setTypeName( "file" );

        properties->setProperty( m_fsmPriorityStr, m_fsmPriority );
        properties->addChild( m_progressiveMeshOptions.toProperties( m_progressiveMeshOptionsStr ) );

        return properties;
    }

    void Mesh::setProperties( SmartPtr<Properties> properties )
    {
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
