#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/WPPhysicsRigidBody2.hpp"
#include <WPPhysics/WPPhysicsManager2.hpp>

#include <Workphone/Workphone.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>

namespace workphone::physics
{
    u32 WPPhysicsRigidBody2::m_nextId = 0;

    const hash_type WPPhysicsRigidBody2::CONSTRAINBOUNDS_HASH = StringUtil::getHash( "constrainBounds" );
    const hash_type WPPhysicsRigidBody2::CAPSCREENPOSITION_HASH = StringUtil::getHash( "capScreenPosition" );
    const hash_type WPPhysicsRigidBody2::ENABLE_COLLISION_HASH = StringUtil::getHash( "enableCollision" );
    const hash_type WPPhysicsRigidBody2::DAMPLINEARVELOCITY_HASH = StringUtil::getHash( "dampLinearVelocity" );
    const hash_type WPPhysicsRigidBody2::ENABLEPHYSICS_HASH = StringUtil::getHash( "enablePhysics" );
    const hash_type WPPhysicsRigidBody2::CONSTRAIN_X_HASH = StringUtil::getHash( "constrainX" );
    const hash_type WPPhysicsRigidBody2::CONSTRAIN_Y_HASH = StringUtil::getHash( "constrainY" );
    const hash_type WPPhysicsRigidBody2::ALLOWROTATION_HASH = StringUtil::getHash( "allowRotation" );
    const hash_type WPPhysicsRigidBody2::SAME_TYPE_COLLISION_HASH = StringUtil::getHash( "sameTypeCollision" );
    const hash_type WPPhysicsRigidBody2::ENABLE_PARTICLE_COLLISION_HASH =
        StringUtil::getHash( "enableParticleCollision" );
    const hash_type WPPhysicsRigidBody2::USE_TARGET_POSITION_X_HASH =
        StringUtil::getHash( "useTargetPositionX" );
    const hash_type WPPhysicsRigidBody2::USE_TARGET_POSITION_Y_HASH =
        StringUtil::getHash( "useTargetPositionY" );
    const hash_type WPPhysicsRigidBody2::ENABLE_HASH = StringUtil::getHash( "enable" );

    const hash_type WPPhysicsRigidBody2::WORLD_ID_HASH = StringUtil::getHash( "worldId" );
    const hash_type WPPhysicsRigidBody2::RESTITUTION_HASH = StringUtil::getHash( "restitution" );
    const hash_type WPPhysicsRigidBody2::LINEAR_DAMP_VALUE_HASH = StringUtil::getHash( "linearDamp" );
    const hash_type WPPhysicsRigidBody2::MASS_HASH = StringUtil::getHash( "mass" );

    WPPhysicsRigidBody2::WPPhysicsRigidBody2( IPhysicsManager2D *creator ) : m_creator( creator ), m_userData( nullptr )
    {
        // auto engine = IApplicationMananger::instance();
        // FactoryPtr factory = engine->getFactory();

        // m_previousState = factory->createFromPool(StringUtil::getHash("WPPhysicsRigidBody2::DynamicState"));
        // m_currentState = factory->createFromPool(StringUtil::getHash("WPPhysicsRigidBody2::DynamicState"));
        // m_forceState = factory->createFromPool(StringUtil::getHash("WPPhysicsRigidBody2::ForceState"));
        // m_staticState = factory->createFromPool(StringUtil::getHash("WPPhysicsRigidBody2::StaticState"));

        m_staticState->m_id = m_nextId++;

        // PlatformManagerPtr& platformManager = Engine::getSingletonPtr()->getPlatformManager();
        // m_stateContext = platformManager->createStateObject();

        // StateListenerPtr stateListener(new RigidBody2StateListener(this));
        // m_stateContext->addStateListener(stateListener, true);

        // set all flags to defaults
        setFlag( RBF_CONSTRAINBOUNDS, true );
        setFlag( RBF_CAPSCREENPOSITION, true );
        setFlag( RBF_ENABLE_COLLISION, false );
        setFlag( RBF_DAMPLINEARVELOCITY, false );
        setFlag( RBF_ENABLEPHYSICS, true );
        setFlag( RBF_CONSTRAIN_X, false );
        setFlag( RBF_CONSTRAIN_Y, false );
        setFlag( RBF_ALLOWROTATION, false );
        setFlag( RBF_ENABLE_PARTICLE_COLLISION, false );
        setFlag( RBF_ENABLE, true );

        // m_staticState->m_contraint.m_minimum = Vector2<real_Num>(0.0, 0.0);
        // m_staticState->m_contraint.m_maximum = Vector2<real_Num>(1.0, 1.0);

        // setInvoker(ScriptInvokerPtr(new ScriptInvokerStandard));
        // setReceiver(ScriptReceiverPtr(new ScriptReceiver(this)));

        m_isGrounded = false;

        // SmartPtr<ICollisionListener2> collisionListener( new CollisionListener( this ) );
        // addListener( collisionListener );
    }

    WPPhysicsRigidBody2::~WPPhysicsRigidBody2()
    {
        if( auto creator = dynamic_cast<WPPhysicsManager2 *>( m_creator ) )
        {
            creator->removeRigidBody( this );
        }
    }

    void WPPhysicsRigidBody2::updateFlags()
    {
        if( m_staticState->m_flagUpdate != m_staticState->m_flagChange )
        {
            m_creator->OnChangeFlags( this );
            m_staticState->m_flagUpdate = static_cast<u32>( m_staticState->m_flagChange );
        }
    }

    void WPPhysicsRigidBody2::handleEvent( const SmartPtr<IEvent> &event )
    {
        // if (event->isDerived(CEventUpdate::TYPE_INFO))
        {
            // WeakPtr<CEventUpdate> eventUpdate = event.get();
            // hash_type eventType;// = eventUpdate->getEventType();
            // if(eventType == StringUtil::getHash("update"))
            //{
            //	update(eventUpdate->getTask(), eventUpdate->getT(), eventUpdate->getDt());
            // }
            // else if(eventType == StringUtil::getHash("postUpdate"))
            //{
            //	postUpdate(eventUpdate->getTask(), eventUpdate->getT(), eventUpdate->getDt());
            // }
            // else if(eventType == StringUtil::getHash("updateForce"))
            //{
            //	updateForce(eventUpdate->getTask(), eventUpdate->getT(), eventUpdate->getDt());
            // }
            // else if(eventType == StringUtil::getHash("advancePosition"))
            //{
            //	advancePosition(eventUpdate->getTask(), eventUpdate->getT(), eventUpdate->getDt());
            // }
            // else if(eventType == StringUtil::getHash("updateKinematic"))
            //{
            //	updateKinematic(eventUpdate->getTask(), eventUpdate->getT(), eventUpdate->getDt());
            // }
            // else if(eventType == StringUtil::getHash("updateGravity"))
            //{
            //	updateGravity(eventUpdate->getTask(), eventUpdate->getT(), eventUpdate->getDt());
            // }

            /*			else
            {
                WP_EXCEPTION("Unhandled event.");
            }	*/
        } // else if (event->isDerived(CEvent::TYPE_INFO))
        {
            // WeakPtr<CEvent> eventStandard = event.get();
            // hash_type eventType = eventStandard->getEventType();
            // if(eventType == StringUtil::getHash("restorePosition"))
            //{
            //	restorePosition();
            // }
            // else if(eventType == StringUtil::getHash("collisionDetectionStarted"))
            //{
            //	//setGrounded(false);
            // }
        }
    }

    void WPPhysicsRigidBody2::update( const s32 &task, const time_interval &t, const time_interval &dt )
    {
        /*switch (task)
            {
            case Thread::TASK_ID_PHYSICS:
            {
            }
                break;
            default:
            {
                // do nothing
            }
            };
             */
    }

    void WPPhysicsRigidBody2::updateForce( const s32 &task, const time_interval &t, const time_interval &dt )
    {
        if( getMass() == 0.0 )
        {
            return;
        }

        if( isSleeping() )
        {
            return;
        }

        WP_ASSERT( Math<real_Num>::isFinite( m_forceState->getForce().X() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_forceState->getForce().Y() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_forceState->getTorque() ) );

        WP_ASSERT( Math<real_Num>::isFinite( m_currentState->getForce().X() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_currentState->getForce().Y() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_currentState->getTorque() ) );

        m_currentState->addForce( m_forceState->getForce() );
        m_currentState->addTorque( m_forceState->getTorque() );

        WP_ASSERT( Math<real_Num>::isFinite( m_currentState->getForce().X() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_currentState->getForce().Y() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_currentState->getTorque() ) );

        m_forceState->reset();
    }

    void WPPhysicsRigidBody2::updateGravity( const s32 &task, const time_interval &t, const time_interval &dt )
    {
        if( getMass() == 0.0 )
        {
            return;
        }

        if( isSleeping() )
        {
            return;
        }

        if( !m_isGrounded )
        {
            const auto        step = static_cast<real_Num>( dt );
            Vector2<real_Num> gravityForce = m_staticState->m_gravity * getMass() * step;
            m_currentState->addForce( gravityForce );
        }
    }

    void WPPhysicsRigidBody2::advancePosition( const s32 &task, const time_interval &t, const time_interval &dt )
    {
        if( getMass() == 0.0 )
        {
            return;
        }

        if( isSleeping() )
        {
            return;
        }

        Vector2<real_Num> force = m_currentState->getForce();
        if( force.lengthSquared() > static_cast<real_Num>( 0.0 ) )
        {
            ++m_staticState->m_nextStateUpdate;
        }

        if( force.lengthSquared() > static_cast<real_Num>( 1000.0 ) * static_cast<real_Num>( 1000.0 ) )
        {
            force = force.normaliseCopy() * static_cast<real_Num>( 1000.0 );
            ++m_staticState->m_nextStateUpdate;
        }

        Vector2<real_Num> velocity = force * m_staticState->m_invMass;

        if( velocity.X() > m_staticState->m_maxVelocity.X() )
            velocity.X() = m_staticState->m_maxVelocity.X();
        else if( velocity.X() < -m_staticState->m_maxVelocity.X() )
            velocity.X() = -m_staticState->m_maxVelocity.X();

        if( velocity.Y() > m_staticState->m_maxVelocity.Y() )
            velocity.Y() = m_staticState->m_maxVelocity.Y();
        else if( velocity.Y() < -m_staticState->m_maxVelocity.Y() )
            velocity.Y() = -m_staticState->m_maxVelocity.Y();

        Vector2<real_Num> position = m_currentState->getPosition();

        if( getFlag( RBF_CONSTRAIN_X ) == false )
        {
            position.X() = position.X() + ( velocity.X() * (real_Num)dt );
        }
        else
        {
            velocity.X() = 0.0f;
        }

        if( getFlag( RBF_CONSTRAIN_Y ) == false )
        {
            position.Y() = position.Y() + ( velocity.Y() * (real_Num)dt );
        }
        else
        {
            velocity.Y() = 0.0f;
        }

        if( !Math<real_Num>::isFinite( velocity.X() ) || !Math<real_Num>::isFinite( velocity.Y() ) )
        {
            velocity = Vector2<real_Num>::ZERO;
        }

        checkTargetPosition( velocity, position );

        // if(getFlag(RBF_CAPSCREENPOSITION) == true)
        {
            capVelocity( position, velocity );
            capPosition( position );
        }

        if( Math<real_Num>::isFinite( position.X() ) && Math<real_Num>::isFinite( position.Y() ) )
        {
            m_currentState->setForce( velocity * m_staticState->m_mass );
            m_currentState->setPosition( position );
        }
        else
        {
            m_currentState->setForce( Vector2<real_Num>::ZERO );
            WP_LOG_ERROR( "Physics RigidBody2::update INF detected." );
        }
    }

    void WPPhysicsRigidBody2::restorePosition()
    {
        if( getMass() == 0.0 )
        {
            return;
        }

        if( isSleeping() )
        {
        }

        // m_currentState->m_position = m_previousState->m_position;
        //*m_currentState = *m_previousState;
    }

    void WPPhysicsRigidBody2::postUpdate( const s32 &task, const time_interval &t, const time_interval &dt )
    {
        if( getMass() == 0.0 )
        {
            return;
        }

        if( isSleeping() )
        {
            return;
        }

        // SpinRWMutex::ScopedLock lock(RigidBodyMutex);

        // Vector2<physics_Num> velocity = m_currentState->m_velocity;
        // Vector2<physics_Num> force = m_currentState->m_force;

        // if(force.X() > physics_Num(0.0))
        //{
        //	force.X() = force.X() - Math<physics_Num>::min(force.X(), force.X() *
        // m_staticState->m_linearDampValue * physics_Num(dt));

        //	physics_Num dragForce = physics_Num(0.5) * (velocity.X() * velocity.X()) *
        // m_staticState->m_airResistance * physics_Num(dt); 	force.X() = force.X() -
        // Math<physics_Num>::min(force.X(), dragForce);
        //}
        // else
        //{
        //	force.X() = force.X() - Math<physics_Num>::max(force.X(), force.X() *
        // m_staticState->m_linearDampValue * physics_Num(dt));

        //	physics_Num dragForce = physics_Num(0.5) * (velocity.X() * velocity.X()) *
        // m_staticState->m_airResistance * physics_Num(dt); 	force.X() = force.X() -
        // Math<physics_Num>::max(force.X(), -dragForce);
        //}

        // if(force.Y() > physics_Num(0.0))
        //{
        //	force.Y() = force.Y() - Math<physics_Num>::min(force.Y(), force.Y() *
        // m_staticState->m_linearDampValue * physics_Num(dt));

        //	physics_Num dragForce = physics_Num(0.5) * (velocity.Y() * velocity.Y()) *
        // m_staticState->m_airResistance * physics_Num(dt); 	force.Y() = force.Y() -
        // Math<physics_Num>::min(force.Y(), dragForce);

        //}
        // else
        //{
        //	force.Y() = force.Y() - Math<physics_Num>::max(force.Y(), force.Y() *
        // m_staticState->m_linearDampValue * physics_Num(dt));

        //	physics_Num dragForce = physics_Num(0.5) * (velocity.Y() * velocity.Y()) *
        // m_staticState->m_airResistance * physics_Num(dt); 	force.Y() = force.Y() -
        // Math<physics_Num>::max(force.Y(), -dragForce);
        //}

        // m_currentState->m_force = force;

        Vector2<real_Num> force = m_currentState->getForce() * m_staticState->m_linearDampValue;

        // some friction
        if( getGrounded() )
            force.X() = force.X() * static_cast<real_Num>( 0.975 );

        m_currentState->setForce( force );

        {
            Vector2<real_Num> position = m_currentState->m_position;
            capPosition( position );

            m_currentState->m_position = position;
        }

        if( force.lengthSquared() < Math<real_Num>::epsilon() )
        {
            setSleep( true );
        }

        // if(m_staticState->m_nextStateUpdate != m_staticState->m_lastStateUpdate)
        {
            // todo remove this message and use a state message.
            Vector2<real_Num> position = m_currentState->m_position;
            Vector2<real_Num> velocity = force * getMassInv();

            auto message = SmartPtr<StatePhysicsPosition2>( new StatePhysicsPosition2( position ) );
            m_stateContext->addMessage( TaskId::Application, message );

            // StatePhysicsDynamicState2Ptr stateMessage(new StatePhysicsDynamicState2);
            // stateMessage->setPosition(position);
            // stateMessage->setVelocity(velocity);
            // stateMessage->setForce(force);
            // m_stateContext->addMessage(Thread::TASK_ID_APPLICATION_LOGIC, stateMessage);

            m_staticState->m_nextStateUpdate = static_cast<u32>( m_staticState->m_lastStateUpdate );
        }
    }

    void WPPhysicsRigidBody2::checkTargetPosition( const Vector2<real_Num> &vector, Vector2<real_Num> &position )
    {
        bool use_target_position_x = getFlag( RBF_USE_TARGET_POSITION_X );
        bool use_target_position_y = getFlag( RBF_USE_TARGET_POSITION_Y );

        if( use_target_position_x || use_target_position_y )
        {
            Vector2<real_Num> direction = vector;
            direction.normalise();

            Vector2<real_Num> targetPosition = m_currentState->m_target;

            if( direction.normaliseLength() > Math<real_Num>::epsilon() )
            {
                real_Num d0 = direction.dotProduct( targetPosition );
                real_Num d1 = direction.dotProduct( position );
                real_Num diff = d1 - d0;
                if( diff > 0.0f )
                {
                    if( use_target_position_x )
                    {
                        position.X() = targetPosition.X();
                    }

                    if( use_target_position_y &&
                        direction.dotProduct( Vector2<real_Num>::UNIT_Y ) > 0.95f )
                    {
                        position.Y() = targetPosition.Y();
                    }
                }
            }
        }
    }

    u32 WPPhysicsRigidBody2::getId() const
    {
        return (u32)m_staticState->m_id;
    }

    void WPPhysicsRigidBody2::addVector( const Vector2<real_Num> &vector )
    {
        Vector2<real_Num> position = m_currentState->m_position;

        if( getFlag( RBF_CONSTRAIN_X ) == false )
        {
            position.X() += vector.X();
        }

        if( getFlag( RBF_CONSTRAIN_Y ) == false )
        {
            position.Y() += vector.Y();
        }

        checkTargetPosition( vector, position );

        m_currentState->m_position = position;
        ++m_staticState->m_nextStateUpdate;
    }

    void WPPhysicsRigidBody2::setPosition( const Vector2<real_Num> &position )
    {
        m_currentState->m_position = m_previousState->m_position = position;
        setSleep( false );
        ++m_staticState->m_nextStateUpdate;
    }

    Vector2<real_Num> WPPhysicsRigidBody2::getPosition() const
    {
        return m_currentState->m_position;
    }

    void WPPhysicsRigidBody2::setTargetPosition( const Vector2<real_Num> &position )
    {
        // m_currentState->m_start = m_currentState->m_position;
        // m_currentState->m_target = position;
        // m_currentState->m_animationTime = real_Num(0.0);
    }

    Vector2<real_Num> WPPhysicsRigidBody2::getTargetPosition() const
    {
        return m_currentState->m_target;
    }

    void WPPhysicsRigidBody2::addForce( const Vector2<real_Num> &force )
    {
        m_forceState->addForce( force );
        setSleep( false );
    }

    void WPPhysicsRigidBody2::setForce( const Vector2<real_Num> &force )
    {
        m_currentState->setForce( force );
        setSleep( false );
        ++m_staticState->m_nextStateUpdate;
    }

    Vector2<real_Num> WPPhysicsRigidBody2::getForce() const
    {
        return m_currentState->getForce();
    }

    void WPPhysicsRigidBody2::addVelocity( const Vector2<real_Num> &velocity, const Vector2<real_Num> &relPos )
    {
        m_forceState->addForce( velocity * m_staticState->m_mass );
        setSleep( false );
    }

    void WPPhysicsRigidBody2::setVelocity( const Vector2<real_Num> &velocity )
    {
        m_currentState->setForce( velocity * m_staticState->m_mass );
        setSleep( false );
        ++m_staticState->m_nextStateUpdate;
    }

    Vector2<real_Num> WPPhysicsRigidBody2::getVelocity() const
    {
        return m_currentState->getForce() * getMassInv();
    }

    void WPPhysicsRigidBody2::setMaxVelocity( const Vector2<real_Num> &velocity )
    {
        m_staticState->m_maxVelocity = velocity;
    }

    Vector2<real_Num> WPPhysicsRigidBody2::getMaxVelocity() const
    {
        return m_staticState->m_maxVelocity;
    }

    void WPPhysicsRigidBody2::setCollisionShape( const SmartPtr<IPhysicsShape2> &shape )
    {
        m_shape = shape;
    }

    const SmartPtr<IPhysicsShape2> &WPPhysicsRigidBody2::getCollisionShape() const
    {
        return m_shape;
    }

    Transform2<real_Num> WPPhysicsRigidBody2::getTransformState() const
    {
        return Transform2<real_Num>( getPosition(), 0.0f );
    }

    void WPPhysicsRigidBody2::setFlag( u32 flag, bool value )
    {
        if( flag == RBF_ENABLE_COLLISION && value != getFlag( RBF_ENABLE_COLLISION ) )
        {
            m_creator->OnChangeFlags( this );
        }

        if( flag == RBF_ENABLE && value != getFlag( RBF_ENABLE ) )
        {
            m_creator->OnChangeFlags( this );
        }

        u32 flags = m_staticState->m_flags;
        if( value )
            flags |= flag;
        else
            flags &= ~flag;

        m_staticState->m_flags = flags;
    }

    void WPPhysicsRigidBody2::setWorldId( hash_type worldId )
    {
        m_staticState->m_worldId = worldId;
    }

    hash_type WPPhysicsRigidBody2::getWorldId() const
    {
        return m_staticState->m_worldId;
    }

    bool WPPhysicsRigidBody2::getFlag( u32 flag ) const
    {
        return ( m_staticState->m_flags & flag ) != 0;
    }

    void WPPhysicsRigidBody2::setEnabled( bool enabled )
    {
        m_staticState->m_enabled = enabled;
    }

    bool WPPhysicsRigidBody2::isEnabled() const
    {
        return m_staticState->m_enabled;
    }

    AABB2<real_Num> WPPhysicsRigidBody2::getLocalAABB() const
    {
        return AABB2<real_Num>();
    }

    AABB2<real_Num> WPPhysicsRigidBody2::getWorldAABB() const
    {
        return AABB2<real_Num>();
    }

    void WPPhysicsRigidBody2::setMaterialId( hash_type materialId )
    {
    }

    hash_type WPPhysicsRigidBody2::getMaterialId() const
    {
        return 0;
    }

    void WPPhysicsRigidBody2::setUserData( void *userData )
    {
        m_userData = userData;
    }

    void *WPPhysicsRigidBody2::getUserData() const
    {
        return m_userData;
    }

    void WPPhysicsRigidBody2::setLinearDampValue( real_Num linearDampValue )
    {
        m_staticState->m_linearDampValue = Math<real_Num>::clamp(
            linearDampValue, static_cast<real_Num>( 0.0 ), static_cast<real_Num>( 1.0 ) );
    }

    real_Num WPPhysicsRigidBody2::getLinearDampValue() const
    {
        return m_staticState->m_linearDampValue;
    }

    void WPPhysicsRigidBody2::setAngularDampValue( real_Num angularDampValue )
    {
        m_staticState->m_angularDampValue = angularDampValue;
    }

    real_Num WPPhysicsRigidBody2::getAngularDampValue() const
    {
        return m_staticState->m_angularDampValue;
    }

    void WPPhysicsRigidBody2::setRestitution( real_Num restitution )
    {
        // SpinRWMutex::ScopedLock lock( RigidBodyMutex );
        m_staticState->m_restitution = restitution;
    }

    real_Num WPPhysicsRigidBody2::getRestitution() const
    {
        // SpinRWMutex::ScopedLock lock( RigidBodyMutex );
        return m_staticState->m_restitution;
    }

    u32 WPPhysicsRigidBody2::getBodyType() const
    {
        return PBT_RIGID;
    }

    void WPPhysicsRigidBody2::setObjectType( hash_type type )
    {
        m_staticState->m_objectType = type;
    }

    hash_type WPPhysicsRigidBody2::getObjectType() const
    {
        return m_staticState->m_objectType;
    }

    void WPPhysicsRigidBody2::setContraintAABB( const AABB2<real_Num> &contraintRect )
    {
        m_staticState->m_contraint = contraintRect;
    }

    AABB2<real_Num> WPPhysicsRigidBody2::getContraintAABB() const
    {
        return m_staticState->m_contraint;
    }

    void WPPhysicsRigidBody2::capVelocity( const Vector2<real_Num> &position, Vector2<real_Num> &velocity )
    {
        if constexpr( true )
        {
            // real_Num restitution = 1.0; // m_restitution;
            // if (position.X() < m_staticState->m_contraint.m_minimum.X())
            //{
            //	velocity.X() = -velocity.X() * restitution;
            // }
            // else if (position.X() > m_staticState->m_contraint.m_maximum.X())
            //{
            //	velocity.X() = -velocity.X() * restitution;
            // }

            // if (position.Y() < m_staticState->m_contraint.m_minimum.Y())
            //{
            //	velocity.Y() = -velocity.Y() * restitution;
            // }
            // else if (position.Y() > m_staticState->m_contraint.m_maximum.Y())
            //{
            //	velocity.Y() = -velocity.Y() * restitution;
            // }
        }
        else
        {
            // if (position.X() < m_staticState->m_contraint.m_minimum.X())
            //{
            //	if (velocity.X() < 0.0f)
            //		velocity.X() = 0.0f;
            // }
            // else if (position.X() > m_staticState->m_contraint.m_maximum.X())
            //{
            //	if (velocity.X() > 0.0f)
            //		velocity.X() = 0.0f;
            // }

            // if (position.Y() < m_staticState->m_contraint.m_minimum.Y())
            //{
            //	if (velocity.Y() < 0.0f)
            //		velocity.Y() = 0.0f;
            // }
            // else if (position.Y() > m_staticState->m_contraint.m_maximum.Y())
            //{
            //	if (velocity.Y() > 0.0f)
            //		velocity.Y() = 0.0f;
            // }
        }

        /*if(velocity.getLength() > 1.0f)
            {
                velocity.setLength(1.0f);
            }*/
    }

    void WPPhysicsRigidBody2::capPosition( Vector2<real_Num> &position )
    {
        // if (position.X() < m_staticState->m_contraint.m_minimum.X())
        //{
        //	position.X() = m_staticState->m_contraint.m_minimum.X();
        // }
        // else if (position.X() > m_staticState->m_contraint.m_maximum.X())
        //{
        //	position.X() = m_staticState->m_contraint.m_maximum.X();
        // }

        // if (position.Y() < m_staticState->m_contraint.m_minimum.Y())
        //{
        //	position.Y() = m_staticState->m_contraint.m_minimum.Y();
        // }
        // else if (position.Y() > m_staticState->m_contraint.m_maximum.Y())
        //{
        //	position.Y() = m_staticState->m_contraint.m_maximum.Y();
        // }
    }

    s32 WPPhysicsRigidBody2::ScriptReceiver::setProperty( hash_type id, const Parameter &param )
    {
        bool value = static_cast<bool>( param.data.bData );

        if( id == StringUtil::getHash( "sleep" ) )
        {
            m_body->setSleep( value );
        }

        if( id == WORLD_ID_HASH )
        {
            m_body->setWorldId( param.data.iData );
        }
        else if( id == RESTITUTION_HASH )
        {
            m_body->setRestitution( param.data.fData );
        }
        else if( id == LINEAR_DAMP_VALUE_HASH )
        {
            m_body->setLinearDampValue( param.data.fData );
        }
        else if( id == MASS_HASH )
        {
            m_body->setMass( param.data.fData );
        }

        else if( id == CONSTRAINBOUNDS_HASH )
        {
            m_body->setFlag( RBF_CONSTRAINBOUNDS, value );
        }
        else if( id == CAPSCREENPOSITION_HASH )
        {
            m_body->setFlag( RBF_CAPSCREENPOSITION, value );
        }
        else if( id == ENABLE_COLLISION_HASH )
        {
            m_body->setFlag( RBF_ENABLE_COLLISION, value );
        }
        else if( id == DAMPLINEARVELOCITY_HASH )
        {
            m_body->setFlag( RBF_DAMPLINEARVELOCITY, value );
        }
        else if( id == ENABLEPHYSICS_HASH )
        {
            m_body->setFlag( RBF_ENABLEPHYSICS, value );
        }
        else if( id == CONSTRAIN_X_HASH )
        {
            m_body->setFlag( RBF_CONSTRAIN_X, value );
        }
        else if( id == CONSTRAIN_Y_HASH )
        {
            m_body->setFlag( RBF_CONSTRAIN_Y, value );
        }
        else if( id == ALLOWROTATION_HASH )
        {
            m_body->setFlag( RBF_ALLOWROTATION, value );
        }
        else if( id == ENABLE_PARTICLE_COLLISION_HASH )
        {
            m_body->setFlag( RBF_ENABLE_PARTICLE_COLLISION, value );
        }
        else if( id == USE_TARGET_POSITION_X_HASH )
        {
            m_body->setFlag( RBF_USE_TARGET_POSITION_X, value );
        }
        else if( id == USE_TARGET_POSITION_Y_HASH )
        {
            m_body->setFlag( RBF_USE_TARGET_POSITION_Y, value );
        }
        else if( id == ENABLE_HASH )
        {
            m_body->setFlag( RBF_ENABLE, value );
        }

        return 0;
    }

    s32 WPPhysicsRigidBody2::ScriptReceiver::setProperty( hash_type id, const Parameters &params )
    {
        // if(id == StringUtil::POSITION_2_HASH)
        //{
        //	Vector2<physics_Num> position(params[0].data.fData, params[1].data.fData);
        //	m_body->setPosition( position );
        //
        // }
        // else if(id == StringUtil::VELOCITY_2_HASH)
        //{
        //	Vector2<physics_Num> vel(params[0].data.fData, params[1].data.fData);
        //	m_body->setVelocity( vel );
        //
        // }

        return 0;
    }

    s32 WPPhysicsRigidBody2::ScriptReceiver::setProperty( hash_type hash, void *param )
    {
        return 0;
    }

    s32 WPPhysicsRigidBody2::ScriptReceiver::getProperty( hash_type id, Parameter &param ) const
    {
        if( id == StringUtil::getHash( "sleep" ) )
        {
            param.setBool( m_body->isSleeping() );
        }

        if( id == StringUtil::getHash( "isVelocityZero" ) )
        {
            param.setBool( m_body->getVelocity() == Vector2<real_Num>::ZERO );
        }
        else if( id == StringUtil::getHash( "ObjectType" ) )
        {
            param.setS64( m_body->getObjectType() );
        }
        else if( id == StringUtil::getHash( "userData" ) )
        {
            param.setPtr( m_body->getUserData() );
        }
        else if( id == StringUtil::getHash( "worldId" ) )
        {
            param.setS64( static_cast<s64>( m_body->getWorldId() ) );
        }

        return 0;
    }

    s32 WPPhysicsRigidBody2::ScriptReceiver::getProperty( hash_type id, Parameters &params ) const
    {
        // if(id == StringUtil::VELOCITY_2_HASH)
        //{
        //	params.resize(2);

        //	Vector2<physics_Num> velocity = m_body->m_currentState->getForce() * m_body->getMassInv();
        //	params[0].data.fData = velocity.X();
        //	params[1].data.fData = velocity.Y();
        //
        //}
        // else if(id == StringUtil::POSITION_2_HASH)
        //{
        //	params.resize(2);

        //	Vector2<physics_Num> position = m_body->m_currentState->m_position;
        //	params[0].data.fData = position.X();
        //	params[1].data.fData = position.Y();
        //
        //}

        return 0;
    }

    s32 WPPhysicsRigidBody2::ScriptReceiver::getProperty( hash_type hash, void *param ) const
    {
        return 0;
    }

    WPPhysicsRigidBody2::ScriptReceiver::ScriptReceiver( WPPhysicsRigidBody2 *body ) : m_body( body )
    {
    }

    void WPPhysicsRigidBody2::RigidBody2StateListener::OnStateChanged( const SmartPtr<IStateMessage> &message )
    {
        // if(CURRENT_TASK_ID == Thread::TASK_ID_PHYSICS)
        //{
        //	u32 messageType = message->getType();
        //	if(messageType == StatePhysicsPosition2::TYPE)
        //	{
        //		StatePhysicsPosition2Ptr posMsg = message;
        //		m_body->setPosition(posMsg->getPosition());
        //	}
        //	else if(messageType == StatePhysicsVelocity2::ADD_VELOCITY_HASH)
        //	{
        //		StatePhysicsVelocity2Ptr velMsg = message;
        //		m_body->addVelocity(velMsg->getVelocity(), velMsg->getRelativePosition());
        //	}
        //	else if(messageType == StatePhysicsVelocity2::SET_VELOCITY_HASH)
        //	{
        //		StatePhysicsVelocity2Ptr velMsg = message;
        //		m_body->setVelocity(velMsg->getVelocity());
        //	}
        //	else if(messageType == StatePhysicsForce2::ADD_FORCE_HASH)
        //	{
        //		StatePhysicsForce2Ptr forceMsg = message;
        //		m_body->addForce(forceMsg->getForce());
        //	}
        //	else if(messageType == StatePhysicsForce2::SET_FORCE_HASH)
        //	{
        //		StatePhysicsForce2Ptr forceMsg = message;
        //		m_body->setForce(forceMsg->getForce());
        //	}
        // }
    }

    void WPPhysicsRigidBody2::RigidBody2StateListener::OnStateChanged( const SmartPtr<IState> &state )
    {
    }

    WPPhysicsRigidBody2::RigidBody2StateListener::~RigidBody2StateListener()
    {
    }

    WPPhysicsRigidBody2::RigidBody2StateListener::RigidBody2StateListener( WPPhysicsRigidBody2 *body ) :
        m_body( body ),
        m_taskId( 0 )
    {
    }

    void WPPhysicsRigidBody2::setSleep( bool sleep )
    {
        m_staticState->m_sleep = sleep;
    }

    bool WPPhysicsRigidBody2::isSleeping() const
    {
        return m_staticState->m_sleep;
    }

    void WPPhysicsRigidBody2::setKinematicMode( bool kinematicMode )
    {
        m_staticState->m_kinematicMode = kinematicMode;
    }

    bool WPPhysicsRigidBody2::getKinematicMode() const
    {
        return m_staticState->m_kinematicMode;
    }

    const SmartPtr<IStateContext> &WPPhysicsRigidBody2::getStateContext() const
    {
        return m_stateContext;
    }

    SmartPtr<IStateContext> &WPPhysicsRigidBody2::getStateContext()
    {
        return m_stateContext;
    }

    void WPPhysicsRigidBody2::setMass( real_Num mass )
    {
        m_staticState->m_mass = mass;
        if( m_staticState->m_mass != 0.0f )
            m_staticState->m_invMass = static_cast<real_Num>( 1.0 ) / m_staticState->m_mass;
        else
            m_staticState->m_invMass = 0.0f;
    }

    void WPPhysicsRigidBody2::setFriction( real_Num friction )
    {
        m_staticState->m_friction = friction;
    }

    real_Num WPPhysicsRigidBody2::getFriction() const
    {
        return m_staticState->m_friction;
    }

    u32 WPPhysicsRigidBody2::getCollisionType() const
    {
        return m_staticState->m_collisionType;
    }

    void WPPhysicsRigidBody2::setCollisionType( u32 mask )
    {
        m_staticState->m_collisionType = mask;
    }

    real_Num WPPhysicsRigidBody2::getAirResistance() const
    {
        return m_staticState->m_airResistance;
    }

    void WPPhysicsRigidBody2::setAirResistance( real_Num airResistance )
    {
        m_staticState->m_airResistance = airResistance;
    }

    real_Num WPPhysicsRigidBody2::getTorque() const
    {
        return static_cast<real_Num>( 0.0 );
    }

    void WPPhysicsRigidBody2::setTorque( real_Num torque )
    {
    }

    void WPPhysicsRigidBody2::addTorque( real_Num torque )
    {
    }

    real_Num WPPhysicsRigidBody2::getAngularVelocity() const
    {
        return static_cast<real_Num>( 0.0 );
    }

    real_Num WPPhysicsRigidBody2::getOrientation() const
    {
        return static_cast<real_Num>( 0.0 );
    }

    void WPPhysicsRigidBody2::setOrientation( real_Num orientation )
    {
    }

    void WPPhysicsRigidBody2::setCurrentState( DynamicState *currentState )
    {
        // m_currentState = currentState;
    }

    WPPhysicsRigidBody2::DynamicState *WPPhysicsRigidBody2::getCurrentState() const
    {
        return nullptr; // m_currentState;
    }

    void WPPhysicsRigidBody2::setPreviousState( DynamicState *previousState )
    {
        // m_previousState = previousState;
    }

    WPPhysicsRigidBody2::DynamicState *WPPhysicsRigidBody2::getPreviousState() const
    {
        return nullptr; // m_previousState;
    }

    WPPhysicsRigidBody2::ForceState *WPPhysicsRigidBody2::getForceState() const
    {
        return nullptr; // m_forceState;
    }

    void WPPhysicsRigidBody2::setForceState( ForceState *forceState )
    {
        // m_forceState = forceState;
    }

    void WPPhysicsRigidBody2::updateKinematic( const s32 &task, const time_interval &t, const time_interval &dt )
    {
        return;

        if( m_staticState->m_kinematicMode )
        {
            auto distance = ( m_currentState->m_target - m_currentState->m_start ).length();
            auto speed = 0.0f;

            if( distance > 0.0f )
            {
                speed = 0.1f / distance;
            }

            m_currentState->m_animationTime += (real_Num)dt;
            if( m_currentState->m_animationTime > static_cast<real_Num>( 1.0 ) )
            {
                m_currentState->m_animationTime = static_cast<real_Num>( 1.0 );
            }

            Vector2<real_Num> newPosition =
                m_currentState->m_start +
                ( m_currentState->m_target - m_currentState->m_start ) * m_currentState->m_animationTime;
            Vector2<real_Num> moveVector = ( newPosition - m_currentState->m_position );
            if( moveVector.length() > static_cast<real_Num>( 0.1 ) )
                moveVector = moveVector.normaliseCopy();

            // m_currentState->m_position = newPosition;
            // m_currentState->m_velocity = moveVector;
            // m_currentState->m_force = m_currentState->m_force + m_forceState->m_force + (moveVector *
            // m_mass); m_currentState->m_force = (moveVector * m_mass) - (moveVector *
            // m_currentState->m_force.dotProduct(moveVector.normaliseCopy())); m_currentState->m_force
            // += m_forceState->m_force;
            m_forceState->addForce( moveVector * m_staticState->m_mass );

            // m_currentState->m_force += m_forceState->m_force;
            // m_currentState->m_torque += m_forceState->m_torque;
            // m_forceState->reset();

            // m_currentState->m_velocity = m_currentState->m_force * getMassInv();

            ++m_staticState->m_nextStateUpdate;

            // m_forceState->reset();

            // m_currentState->m_position = m_currentState->m_position + (m_currentState->m_velocity *
            // dt);
        }
    }

    void WPPhysicsRigidBody2::setGravity( const Vector2<real_Num> &gravity )
    {
        m_staticState->m_gravity = gravity;
    }

    Vector2<real_Num> WPPhysicsRigidBody2::getGravity() const
    {
        return m_staticState->m_gravity;
    }

    void WPPhysicsRigidBody2::removeEffect( SmartPtr<IPhysicsEffect2> effect )
    {
    }

    void WPPhysicsRigidBody2::addEffect( SmartPtr<IPhysicsEffect2> effect )
    {
    }

    void WPPhysicsRigidBody2::setGrounded( bool grounded )
    {
        m_isGrounded = grounded;
    }

    bool WPPhysicsRigidBody2::getGrounded() const
    {
        return m_isGrounded;
    }

    WPPhysicsRigidBody2::DynamicState::~DynamicState()
    {
    }

    WPPhysicsRigidBody2::DynamicState::DynamicState() :
        m_torque( static_cast<real_Num>( 0.0 ) ),
        m_angularVelocity( static_cast<real_Num>( 0.0 ) ),
        m_rotation( static_cast<real_Num>( 0.0 ) )
    {
    }

    void WPPhysicsRigidBody2::DynamicState::addTorque( const real_Num &torque )
    {
        m_torque = m_torque + torque;
    }

    void WPPhysicsRigidBody2::DynamicState::setTorque( const real_Num &torque )
    {
        m_torque = torque;
    }

    workphone::real_Num WPPhysicsRigidBody2::DynamicState::getTorque() const
    {
        return m_torque;
    }

    void WPPhysicsRigidBody2::DynamicState::addForce( const Vector2<real_Num> &force )
    {
        m_force = m_force + force;

        WP_ASSERT( Math<real_Num>::isFinite( m_force.X() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_force.Y() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_torque ) );
    }

    void WPPhysicsRigidBody2::DynamicState::setForce( const Vector2<real_Num> &force )
    {
        m_force = force;

        WP_ASSERT( Math<real_Num>::isFinite( m_force.X() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_force.Y() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_torque ) );
    }

    workphone::Vector2<workphone::real_Num> WPPhysicsRigidBody2::DynamicState::getForce() const
    {
        WP_ASSERT( Math<real_Num>::isFinite( m_force.X() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_force.Y() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_torque ) );

        return m_force;
    }

    void WPPhysicsRigidBody2::DynamicState::setPosition( const Vector2<real_Num> &position )
    {
        m_position = position;
    }

    workphone::Vector2<workphone::real_Num> WPPhysicsRigidBody2::DynamicState::getPosition() const
    {
        return m_position;
    }

    void WPPhysicsRigidBody2::ForceState::reset()
    {
        m_force = Vector2<real_Num>::ZERO;
        m_torque = static_cast<real_Num>( 0.0 );
    }

    WPPhysicsRigidBody2::ForceState::~ForceState()
    {
    }

    WPPhysicsRigidBody2::ForceState::ForceState() : m_torque( static_cast<real_Num>( 0.0 ) )
    {
    }

    void WPPhysicsRigidBody2::ForceState::addTorque( const real_Num &torque )
    {
        m_torque = m_torque + torque;
    }

    void WPPhysicsRigidBody2::ForceState::setTorque( const real_Num &torque )
    {
        m_torque = torque;
    }

    workphone::real_Num WPPhysicsRigidBody2::ForceState::getTorque() const
    {
        return m_torque;
    }

    void WPPhysicsRigidBody2::ForceState::addForce( const Vector2<real_Num> &force )
    {
        m_force = m_force + force;

        WP_ASSERT( Math<real_Num>::isFinite( m_force.X() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_force.Y() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_torque ) );
    }

    void WPPhysicsRigidBody2::ForceState::setForce( const Vector2<real_Num> &force )
    {
        m_force = force;

        WP_ASSERT( Math<real_Num>::isFinite( m_force.X() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_force.Y() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_torque ) );
    }

    workphone::Vector2<workphone::real_Num> WPPhysicsRigidBody2::ForceState::getForce() const
    {
        WP_ASSERT( Math<real_Num>::isFinite( m_force.X() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_force.Y() ) );
        WP_ASSERT( Math<real_Num>::isFinite( m_torque ) );

        return m_force;
    }

    WPPhysicsRigidBody2::StaticState::StaticState() :
        m_maxVelocity( Vector2<real_Num>( 1e10, 1e10 ) ),

        m_flags( 0 ),
        m_id( 0 ),
        m_linearDampValue( 0.9f ),

        m_angularDampValue( 1.0f ),
        m_restitution( 0.0f ),
        m_mass( 1.0f ),
        m_invMass( 1.0f ),
        m_airResistance( static_cast<real_Num>( 0.0 ) ),
        m_worldId( 255 ),
        m_objectType( 0 ),
        m_flagChange( 0 ),
        m_flagUpdate( 0 ),

        m_collisionType( 0 ),
        m_collisionMask( 0 ),

        m_nextStateUpdate( 0 ),
        m_lastStateUpdate( 0 ),

        m_sleep( false ),

        m_kinematicMode( false )
    {
    }

    void WPPhysicsRigidBody2::CollisionListener::OnMaterialSetup( IPhysicsMaterial2 *material )
    {
    }

    int WPPhysicsRigidBody2::CollisionListener::OnContactStart( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB )
    {
        return 0;
    }

    int WPPhysicsRigidBody2::CollisionListener::OnContactEnd( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB )
    {
        return 0;
    }

    int WPPhysicsRigidBody2::CollisionListener::OnContactBreak( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB )
    {
        return 0;
    }

    int WPPhysicsRigidBody2::CollisionListener::OnNoContact( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB )
    {
        return 0;
    }

    int WPPhysicsRigidBody2::CollisionListener::OnContact( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB )
    {
        return 0;
    }

    int WPPhysicsRigidBody2::CollisionListener::OnContactProcess( IPhysicsMaterial2 *material )
    {
        // if(Math<physics_Num>::Abs(material->getContactNormal().Y()) > physics_Num(0.95))
        //{
        //	m_body->setGrounded(true);
        // }

        return 0;
    }

    WPPhysicsRigidBody2::CollisionListener::~CollisionListener()
    {
    }

    WPPhysicsRigidBody2::CollisionListener::CollisionListener( WPPhysicsRigidBody2 *body ) : m_body( body )
    {
    }

    void WPPhysicsRigidBody2::PhysicsBodyEffectSnap2::handleEvent( const SmartPtr<IEvent> &event )
    {
        /*
            if (event->isDerived(CEventUpdate::TYPE_INFO))
            {
                WeakPtr<CEventUpdate> eventUpdate;// = event.get();
                hash_type eventType;// = eventUpdate->getEventType();
                if (eventType == StringUtil::getHash("update"))
                {
                    //update(eventUpdate->getTask(), eventUpdate->getT(), eventUpdate->getDt());
                }
            }
            */
    }

    void WPPhysicsRigidBody2::PhysicsBodyEffectSnap2::setUseAxis( int axis, bool useAxis )
    {
        m_axis[axis] = useAxis;
    }

    bool WPPhysicsRigidBody2::PhysicsBodyEffectSnap2::getUseAxis( int axis ) const
    {
        return m_axis[axis];
    }

    void WPPhysicsRigidBody2::PhysicsBodyEffectSnap2::setTarget( const Vector2<real_Num> &target )
    {
        m_target = target;
    }

    Vector2<real_Num> WPPhysicsRigidBody2::PhysicsBodyEffectSnap2::getTarget() const
    {
        return m_target;
    }

    void WPPhysicsRigidBody2::PhysicsBodyEffectSnap2::setOwner( IPhysicsBody2D *owner )
    {
        m_body = static_cast<WPPhysicsRigidBody2 *>( owner );
    }

    IPhysicsBody2D *WPPhysicsRigidBody2::PhysicsBodyEffectSnap2::getOwner() const
    {
        return m_body;
    }

    WPPhysicsRigidBody2::PhysicsBodyEffectSnap2::~PhysicsBodyEffectSnap2()
    {
    }

    WPPhysicsRigidBody2::PhysicsBodyEffectSnap2::PhysicsBodyEffectSnap2()
    {
    }
} // namespace workphone::physics
