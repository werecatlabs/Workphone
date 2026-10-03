#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CollisionSphere.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/ISphereShape3.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, CollisionSphere, Collision );

    CollisionSphere::CollisionSphere() = default;

    CollisionSphere::~CollisionSphere()
    {
        if( isLoaded() )
        {
            unload( nullptr );
        }
    }

    void CollisionSphere::load( SmartPtr<ISharedObject> data )
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
            WP_ASSERT( applicationManager );

            if( auto physicsManager = applicationManager->getPhysicsManager() )
            {
                if( !getShape() )
                {
                    auto shape = physicsManager->addCollisionShape<physics::ISphereShape3>( nullptr );
                    WP_ASSERT( shape );

                    setShape( shape );

                    auto radius = getRadius();
                    shape->setRadius( getRadius() );
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CollisionSphere::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        Collision::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    AABB3<real_Num> CollisionSphere::getBoundingBox() const
    {
        const auto radius = static_cast<real_Num>( getRadius() );
        const auto radiusVector = Vector3<real_Num>( radius, radius, radius );
        return AABB3<real_Num>( m_position - radiusVector, m_position + radiusVector );
    }
}  // namespace workphone::scene
