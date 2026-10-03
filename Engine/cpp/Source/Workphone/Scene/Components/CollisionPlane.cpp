#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CollisionPlane.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPlaneShape3.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, CollisionPlane, Collision );

    CollisionPlane::CollisionPlane() = default;

    CollisionPlane::~CollisionPlane()
    {
        if( isLoaded() )
        {
            unload( nullptr );
        }
    }

    void CollisionPlane::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            Collision::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            auto factory = applicationManager->getFactoryManager();
            auto physicsManager = applicationManager->getPhysicsManager();
            if( !physicsManager )
            {
                WP_LOG_ERROR( "Physics manager is not available" );
                setLoadingState( LoadingState::Loaded );
                return;
            }

            m_material = physicsManager->addMaterial();

            if( auto physicsManager = applicationManager->getPhysicsManager() )
            {
                auto shape = physicsManager->addCollisionShape<physics::IPlaneShape3>( nullptr );
                WP_ASSERT( shape );

                setShape( shape );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CollisionPlane::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );
            Collision::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

}  // namespace workphone::scene
