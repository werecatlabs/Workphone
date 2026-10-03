#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CollisionMesh.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Scene/Components/Mesh.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Interface/Physics/IMeshShape.hpp>
#include <Workphone/Interface/Physics/IRigidbody3.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, CollisionMesh, Collision );
    const String CollisionMesh::meshStr = String( "Mesh" );
    const String CollisionMesh::meshPathStr = String( "Mesh Path" );
    const String CollisionMesh::isConvexStr = String( "isConvex" );

    CollisionMesh::CollisionMesh() = default;

    CollisionMesh::~CollisionMesh()
    {
        if( isLoaded() )
        {
            unload( nullptr );
        }
    }

    void CollisionMesh::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            Collision::load( data );

            if( auto actor = getActorPtr() )
            {
                if( auto meshComponent = actor->getComponent<Mesh>() )
                {
                    auto meshPath = meshComponent->getMeshPath();
                    setMeshPath( meshPath );
                }
            }

            setupMesh();
            createPhysicsShape();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CollisionMesh::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() != LoadingState::Loaded )
                return;

            setLoadingState( LoadingState::Unloading );
            m_meshResource = nullptr;
            Collision::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto CollisionMesh::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Collision::getProperties();

        auto meshPath = getMeshPath();
        properties->setProperty( meshPathStr, meshPath );

        properties->setProperty( meshStr, m_meshResource );
        properties->setProperty( isConvexStr, m_isConvex );

        auto &meshPathProperty = properties->getPropertyObject( meshPathStr );
        meshPathProperty.setTypeName( "file" );

        return properties;
    }

    void CollisionMesh::setProperties( SmartPtr<Properties> properties )
    {
        Collision::setProperties( properties );

        auto meshPath = getMeshPath();
        properties->getPropertyValue( meshPathStr, meshPath );

        properties->getPropertyValue( meshStr, m_meshResource );

        bool convex = m_isConvex;
        properties->getPropertyValue( isConvexStr, convex );
        if( convex != m_isConvex )
            setConvex( convex );

        if( meshPath != getMeshPath() )
        {
            meshPath = StringUtil::cleanupPath( meshPath );

            auto ext = Path::getFileExtension( meshPath );
            if( ext == ".mesh" )
                meshPath =
                    StringUtil::cleanupPath( StringUtil::replaceAll( meshPath, ".mesh", ".fbmeshbin" ) );

            setMeshPath( meshPath );
        }
    }

    auto CollisionMesh::getMeshPath() const -> String
    {
        return m_meshPath;
    }

    void CollisionMesh::setMeshPath( const String &meshPath )
    {
        auto meshFilePath = StringUtil::cleanupPath( meshPath );

        if( m_meshPath.str() != meshFilePath )
        {
            m_meshPath = meshFilePath;
            setupMesh();

            // Propagate to the live shape if one already exists.
            if( auto shape = getShape() )
            {
                if( auto meshShape = workphone::dynamic_pointer_cast<physics::IMeshShape>( shape ) )
                {
                    if( m_meshResource )
                        meshShape->setMeshResource( m_meshResource );
                }
            }
        }
    }

    auto CollisionMesh::getMeshResource() const -> SmartPtr<IMeshResource>
    {
        return m_meshResource;
    }

    void CollisionMesh::setupMesh()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto meshPath = getMeshPath();
        if( !StringUtil::isNullOrEmpty( meshPath ) )
        {
            if( auto meshManager = applicationManager->getMeshManager() )
            {
                auto resource = meshManager->loadFromFile( meshPath );
                if( resource )
                {
                    WP_ASSERT( workphone::dynamic_pointer_cast<IMeshResource>( resource ) );

                    auto meshResource = workphone::static_pointer_cast<IMeshResource>( resource );
                    WP_ASSERT( meshResource );

                    setMeshResource( meshResource );

                    if( auto shape = getShape() )
                    {
                        WP_ASSERT( workphone::dynamic_pointer_cast<physics::IMeshShape>( shape ) );

                        auto meshShape = workphone::static_pointer_cast<physics::IMeshShape>( shape );
                        if( meshShape )
                        {
                            meshShape->setMeshResource( meshResource );
                        }
                    }
                }
            }
        }
    }

    auto CollisionMesh::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        Component::handleComponentEvent( state, eventType );

        switch( eventType )
        {
        case FSMEvent::Change:
            break;
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                try
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    WP_ASSERT( applicationManager );

                    auto factory = applicationManager->getFactoryManagerPtr();
                    WP_ASSERT( factory );

                    if( auto actor = getActorPtr() )
                    {
                        if( auto meshComponent = actor->getComponent<Mesh>() )
                        {
                            auto meshPath = meshComponent->getMeshPath();
                            setMeshPath( meshPath );
                        }
                    }

                    setupMesh();
                    createPhysicsShape();
                    updateRigidBody();
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
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
            break;
        case FSMEvent::Complete:
            break;
        case FSMEvent::NewState:
            break;
        case FSMEvent::WaitForChange:
            break;
        default:
        {
        }
        break;
        }

        return FSMReturnType::Ok;
    }

    void CollisionMesh::setMeshResource( SmartPtr<IMeshResource> meshResource )
    {
        m_meshResource = meshResource;

        if( m_meshResource )
        {
            m_meshPath = m_meshResource->getFilePath();
        }
    }

    auto CollisionMesh::isValid() const -> bool
    {
        return Collision::isValid();
    }

    void CollisionMesh::createPhysicsShape()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager = applicationManager->getPhysicsManager();
        if( physicsManager )
        {
            auto shape = workphone::dynamic_pointer_cast<physics::IMeshShape>( getShape() );
            if( !shape )
            {
                shape = physicsManager->addCollisionShape<physics::IMeshShape>( nullptr );
                WP_ASSERT( shape );
                setShape( shape );
            }

            if( !shape )
            {
                return;
            }

            auto mesh = getMeshResource();
            if( mesh )
            {
                if( !mesh->isLoaded() )
                    mesh->load( nullptr );

                shape->setMeshResource( mesh );
            }

            // Apply all stored base-class and local state to the new shape.
            shape->setConvex( m_isConvex );
            shape->setTrigger( isTrigger() );
            shape->setEnabled( isEnabled() );

            Transform3<real_Num> pose;
            pose.setPosition( getPosition() );
            shape->setLocalPose( pose );

            if( auto mat = getMaterial() )
            {
                shape->setMaterial( mat );
                mat->setStaticFriction( getStaticFriction(), 0 );
                mat->setDynamicFriction( getDynamicFriction(), 0 );
                mat->setRestitution( getRestitution() );
            }
        }
    }

    auto CollisionMesh::isConvex() const -> bool
    {
        return m_isConvex;
    }

    void CollisionMesh::setConvex( bool convex )
    {
        m_isConvex = convex;

        if( auto shape = getShape() )
        {
            if( auto meshShape = workphone::dynamic_pointer_cast<physics::IMeshShape>( shape ) )
                meshShape->setConvex( m_isConvex );
        }
    }
}  // namespace workphone::scene
