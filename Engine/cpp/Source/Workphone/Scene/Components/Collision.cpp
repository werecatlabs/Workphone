#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Collision.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/Physics/IRigidBody3.hpp>
#include <Workphone/Interface/Physics/ISphereShape3.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Collision, Component );

    const String Collision::isTriggerStr = "isTrigger";
    const String Collision::extentsStr = "extents";
    const String Collision::positionStr = "position";
    const String Collision::radiusStr = "radius";
    const String Collision::staticFrictionStr = "staticFriction";
    const String Collision::dynamicFrictionStr = "dynamicFriction";
    const String Collision::restitutionStr = "restitution";

    Collision::Collision() = default;
    Collision::~Collision() = default;

    void Collision::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            return;
        }

        Component::load( data );

        if( m_componentFSM )
        {
            m_componentFSM->setPriority( 10000 );
        }
    }

    void Collision::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManagerPtr();
            if( !physicsManager )
            {
                WP_LOG_ERROR( "CollisionBox::unload physicsManager null" );
            }

            if( auto shape = getShape() )
            {
                if( physicsManager )
                {
                    physicsManager->removeCollisionShape( shape );
                }

                setShape( nullptr );
            }

            Component::unload( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Collision::getMaterial() const -> SmartPtr<physics::IPhysicsMaterial3>
    {
        return m_material;
    }

    void Collision::setMaterial( SmartPtr<physics::IPhysicsMaterial3> material )
    {
        m_material = material;
    }

    physics::IPhysicsShape3 *Collision::getShapePtr() const
    {
        return m_shape.get();
    }

    auto Collision::getShape() const -> SmartPtr<physics::IPhysicsShape3>
    {
        return m_shape;
    }

    void Collision::setShape( SmartPtr<physics::IPhysicsShape3> shape )
    {
        m_shape = shape;
    }

    auto Collision::getExtents() const -> Vector3<real_Num>
    {
        return m_extents;
    }

    void Collision::setExtents( const Vector3<real_Num> &extents )
    {
        m_extents = Vector3<real_Num>( Math<real_Num>::max( extents.x, real_Num( 0 ) ),
                                       Math<real_Num>::max( extents.y, real_Num( 0 ) ),
                                       Math<real_Num>::max( extents.z, real_Num( 0 ) ) );
    }

    auto Collision::getPosition() const -> Vector3<real_Num>
    {
        return m_position;
    }

    void Collision::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
        if( auto shape = getShape() )
        {
            auto pose = shape->getLocalPose();
            pose.setPosition( m_position );
            shape->setLocalPose( pose );
        }
    }

    auto Collision::getRadius() const -> f32
    {
        return m_radius;
    }

    void Collision::setRadius( f32 radius )
    {
        m_radius = Math<f32>::max( radius, 0.0f );

        if( auto sphereShape = workphone::dynamic_pointer_cast<physics::ISphereShape3>( getShape() ) )
        {
            sphereShape->setRadius( m_radius );
        }
    }

    auto Collision::isTrigger() const -> bool
    {
        return m_isTrigger;
    }

    void Collision::setEnabled( bool enabled )
    {
        Component::setEnabled( enabled );

        if( auto shape = getShape() )
        {
            shape->setEnabled( isEnabled() );
        }
    }

    void Collision::setTrigger( bool trigger )
    {
        m_isTrigger = trigger;
        if( auto shape = getShape() )
        {
            shape->setTrigger( m_isTrigger );
            shape->setEnabled( isEnabled() );
        }
    }

    f32 Collision::getStaticFriction() const
    {
        return m_staticFriction;
    }

    void Collision::setStaticFriction( f32 friction )
    {
        m_staticFriction = Math<f32>::clamp( friction, 0.0f, 1.0f );
        if( auto mat = getMaterial() )
            mat->setStaticFriction( m_staticFriction, 0 );
    }

    f32 Collision::getDynamicFriction() const
    {
        return m_dynamicFriction;
    }

    void Collision::setDynamicFriction( f32 friction )
    {
        m_dynamicFriction = Math<f32>::clamp( friction, 0.0f, 1.0f );
        if( auto mat = getMaterial() )
            mat->setDynamicFriction( m_dynamicFriction, 0 );
    }

    f32 Collision::getRestitution() const
    {
        return m_restitution;
    }

    void Collision::setRestitution( f32 restitution )
    {
        m_restitution = Math<f32>::clamp( restitution, 0.0f, 1.0f );
        if( auto mat = getMaterial() )
            mat->setRestitution( m_restitution );
    }

    auto Collision::getBoundingBox() const -> AABB3<real_Num>
    {
        return {};
    }

    auto Collision::getRigidBody() const -> SmartPtr<Rigidbody>
    {
        return m_rigidBody;
    }

    void Collision::setRigidBody( SmartPtr<Rigidbody> rigidBody )
    {
        m_rigidBody = rigidBody;
    }

    auto Collision::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        Array<SmartPtr<ISharedObject>> objects;

        objects.emplace_back( m_rigidBody );
        objects.emplace_back( m_material );
        objects.emplace_back( m_shape );

        return objects;
    }

    auto Collision::getProperties() const -> SmartPtr<Properties>
    {
        try
        {
            auto properties = Component::getProperties();

            properties->setProperty( isTriggerStr, m_isTrigger );
            properties->setProperty( extentsStr, m_extents );
            properties->setProperty( positionStr, m_position );
            properties->setProperty( radiusStr, m_radius );
            properties->setProperty( staticFrictionStr, m_staticFriction );
            properties->setProperty( dynamicFrictionStr, m_dynamicFriction );
            properties->setProperty( restitutionStr, m_restitution );

            const auto setEditorMetadata =
                [&]( const String &name, const String &label, const String &category,
                     const String &description, const String &minimum = String(),
                     const String &maximum = String(), const String &step = String() ) {
                    auto &property = properties->getPropertyObject( name );
                    property.setAttribute( "label", label );
                    property.setAttribute( "category", category );
                    property.setAttribute( "description", description );
                    if( !StringUtil::isNullOrEmpty( minimum ) )
                        property.setAttribute( "min", minimum );
                    if( !StringUtil::isNullOrEmpty( maximum ) )
                        property.setAttribute( "max", maximum );
                    if( !StringUtil::isNullOrEmpty( step ) )
                        property.setAttribute( "step", step );
                };

            setEditorMetadata( isTriggerStr, "Trigger", "Collision",
                               "Report overlaps without generating contact response." );
            setEditorMetadata( extentsStr, "Extents", "Shape",
                               "Local dimensions of the collision shape.", "0", "", "0.1" );
            setEditorMetadata( positionStr, "Center Offset", "Shape",
                               "Local-space offset from the actor origin.", "", "", "0.1" );
            setEditorMetadata( radiusStr, "Radius", "Shape", "Radius used by round shapes.", "0", "",
                               "0.1" );
            setEditorMetadata( staticFrictionStr, "Static Friction", "Material",
                               "Resistance before surfaces begin sliding.", "0", "1", "0.01" );
            setEditorMetadata( dynamicFrictionStr, "Dynamic Friction", "Material",
                               "Resistance while surfaces are sliding.", "0", "1", "0.01" );
            setEditorMetadata( restitutionStr, "Restitution", "Material", "Bounciness of contacts.", "0",
                               "1", "0.01" );

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void Collision::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            if( !properties )
            {
                return;
            }

            Component::setProperties( properties );

            bool trigger = isTrigger();
            properties->getPropertyValue( isTriggerStr, trigger );

            Vector3<real_Num> extents = m_extents;
            properties->getPropertyValue( extentsStr, extents );

            Vector3<real_Num> position = m_position;
            properties->getPropertyValue( positionStr, position );

            f32 radius = m_radius;
            properties->getPropertyValue( radiusStr, radius );

            f32 staticFriction = m_staticFriction;
            properties->getPropertyValue( staticFrictionStr, staticFriction );

            f32 dynamicFriction = m_dynamicFriction;
            properties->getPropertyValue( dynamicFrictionStr, dynamicFriction );

            f32 restitution = m_restitution;
            properties->getPropertyValue( restitutionStr, restitution );

            if( trigger != isTrigger() )
                setTrigger( trigger );

            if( !MathUtil<real_Num>::equals( extents, m_extents ) )
                setExtents( extents );

            if( !MathUtil<real_Num>::equals( position, m_position ) )
                setPosition( position );

            if( !MathUtil<real_Num>::equals( radius, m_radius ) )
                setRadius( radius );

            if( !MathUtil<real_Num>::equals( staticFriction, m_staticFriction ) )
                setStaticFriction( staticFriction );

            if( !MathUtil<real_Num>::equals( dynamicFriction, m_dynamicFriction ) )
                setDynamicFriction( dynamicFriction );

            if( !MathUtil<real_Num>::equals( restitution, m_restitution ) )
                setRestitution( restitution );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto Collision::isValid() const -> bool
    {
        switch( auto state = getState() )
        {
        case IComponent::State::None:
        {
            return true;
        }
        break;
        case IComponent::State::Edit:
        case IComponent::State::Play:
        {
            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                if( m_shape )
                {
                    if( m_shape->isValid() )
                    {
                        return true;
                    }
                }
            }
            else if( loadingState == LoadingState::Unloaded )
            {
                if( m_shape )
                {
                    return false;
                }
            }
        }
        break;
        default:
        {
        }
        }

        return false;
    }

    void Collision::updateTransform()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            if( auto actor = getActor() )
            {
                auto physicsManager = applicationManager->getPhysicsManager();
                if( physicsManager )
                {
                    auto shape = getShape();
                    if( shape )
                    {
                        auto localTransform = actor->getWorldTransform();
                        localTransform.setPosition( Vector3<real_Num>::zero() );
                        localTransform.setOrientation( Quaternion<real_Num>::identity() );
                        shape->setLocalPose( localTransform );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    FSMReturnType Collision::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        switch( eventType )
        {
        case FSMEvent::Enter:
        {
            switch( (IComponent::State)state )
            {
            case IComponent::State::Edit:
            case IComponent::State::Play:
            {
                updateTransform();
            }
            break;
            default:
            {
            }
            };
        }
        break;
        default:
        {
        }
        };

        return FSMReturnType::Ok;
    }

    void Collision::createPhysicsShape()
    {
    }

    void Collision::updateRigidBody()
    {
        if( auto actor = getActor() )
        {
            auto rigidBodies = actor->getComponentsByType<Rigidbody>();
            for( auto rigidBody : rigidBodies )
            {
                rigidBody->updateShapes();
            }
        }
    }

}  // namespace workphone::scene
