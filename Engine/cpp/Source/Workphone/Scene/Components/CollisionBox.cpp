#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Interface/Physics/IBoxShape3.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, CollisionBox, Collision );

    CollisionBox::CollisionBox() = default;

    CollisionBox::~CollisionBox()
    {
        if( isLoaded() )
        {
            unload( nullptr );
        }
    }

    void CollisionBox::load( SmartPtr<ISharedObject> data )
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
                    auto shape = physicsManager->addCollisionShape<physics::IBoxShape3>( nullptr );
                    WP_ASSERT( shape );

                    setShape( shape );

                    auto extents = getExtents();
                    WP_ASSERT( MathUtil<real_Num>::isFinite( extents ) );
                    shape->setExtents( extents );

                    // Apply position offset stored in the base class.
                    Transform3<real_Num> pose;
                    pose.setPosition( getPosition() );
                    shape->setLocalPose( pose );

                    // Apply trigger and enabled flags.
                    shape->setTrigger( isTrigger() );
                    shape->setEnabled( isEnabled() );

                    // Apply material properties if a material is assigned.
                    if( auto mat = getMaterial() )
                    {
                        shape->setMaterial( mat );
                        mat->setStaticFriction( getStaticFriction(), 0 );
                        mat->setDynamicFriction( getDynamicFriction(), 0 );
                        mat->setRestitution( getRestitution() );
                    }
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CollisionBox::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() != LoadingState::Loaded )
                return;

            setLoadingState( LoadingState::Unloading );
            Collision::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CollisionBox::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( nullptr );
            load( nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto CollisionBox::getProperties() const -> SmartPtr<Properties>
    {
        return Collision::getProperties();
    }

    void CollisionBox::setProperties( SmartPtr<Properties> properties )
    {
        Collision::setProperties( properties );
        updateTransform();
    }

    void CollisionBox::setExtents( const Vector3<real_Num> &extents )
    {
        Collision::setExtents( extents );
        updateTransform();
    }

    auto CollisionBox::isValid() const -> bool
    {
        if( auto actor = getActor() )
        {
            switch( actor->getState() )
            {
            case IGameActor::State::Edit:
            case IGameActor::State::Play:
            {
                if( !Collision::isValid() )
                    return false;

                if( auto shape = getShape() )
                    return workphone::dynamic_pointer_cast<physics::IBoxShape3>( shape ) != nullptr;

                return false;
            }
            default:
                return true;
            }
        }

        return false;
    }

    void CollisionBox::updateTransform()
    {
        Collision::updateTransform();

        auto extents = getExtents();

        if( auto shape = getShape() )
        {
            if( auto boxShape = workphone::static_pointer_cast<physics::IBoxShape3>( shape ) )
            {
                boxShape->setExtents( extents );
            }
        }
    }

    AABB3<real_Num> CollisionBox::getBoundingBox() const
    {
        if( auto shape = getShape() )
        {
            if( auto boxShape = workphone::dynamic_pointer_cast<physics::IBoxShape3>( shape ) )
                return boxShape->getAABB();
        }

        // Fall back to an AABB built from the stored extents centred on the position offset.
        return AABB3<real_Num>( m_position - m_extents, m_position + m_extents );
    }

}  // namespace workphone::scene
