#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Constraint.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Physics/IConstraintD6.hpp>
#include <Workphone/Interface/Physics/IConstraintFixed3.hpp>
#include <Workphone/Interface/Physics/IConstraintLinearLimit.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/IRigidStatic3.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Constraint, Component );

    const String Constraint::ConstraintTypeStr = String( "constraintType" );
    const String Constraint::BodyAStr = String( "bodyA" );
    const String Constraint::BodyBStr = String( "bodyB" );
    const String Constraint::XMotionStr = String( "xMotion" );
    const String Constraint::YMotionStr = String( "yMotion" );
    const String Constraint::ZMotionStr = String( "zMotion" );
    const String Constraint::Swing1MotionStr = String( "swing1Motion" );
    const String Constraint::Swing2MotionStr = String( "swing2Motion" );
    const String Constraint::TwistMotionStr = String( "twistMotion" );
    const String Constraint::BreakForceStr = String( "breakForce" );
    const String Constraint::BreakTorqueStr = String( "breakTorque" );

    const Array<String> Constraint::ConstraintTypeNames = { "D6", "Fixed" };
    const Array<String> Constraint::AxisMotionNames = { "Locked", "Limited", "Free" };

    Constraint::Constraint() = default;

    Constraint::~Constraint()
    {
        Constraint::unload( nullptr );
    }

    void Constraint::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Constraint::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( data );
            load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Constraint::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto loadingState = getLoadingState();
            if( loadingState == LoadingState::Unloaded || loadingState == LoadingState::Unloading )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            destroyConstraint();
            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Constraint::updateFlags( u32 flags, u32 oldFlags )
    {
        Component::updateFlags( flags, oldFlags );

        auto componentState = getState();
        switch( componentState )
        {
        case State::Edit:
        case State::Play:
        {
            if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
                BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene ) )
            {
                updatePhysicsState();
            }
            else if( BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                     BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled ) )
            {
                updatePhysicsState();
            }
        }
        break;
        default:
        {
        }
        break;
        }
    }

    SmartPtr<Properties> Constraint::getProperties() const
    {
        auto properties = Component::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        try
        {
            properties->setPropertyAsEnum( ConstraintTypeStr, static_cast<s32>( getType() ),
                                           ConstraintTypeNames );

            auto bodyA = getBodyA();
            auto bodyB = getBodyB();

            if( bodyA )
            {
                properties->setProperty( BodyAStr, workphone::static_pointer_cast<IComponent>( bodyA ) );
            }
            else
            {
                properties->setProperty( BodyAStr, SmartPtr<IComponent>( nullptr ) );
            }

            if( bodyB )
            {
                properties->setProperty( BodyBStr, workphone::static_pointer_cast<IComponent>( bodyB ) );
            }
            else
            {
                properties->setProperty( BodyBStr, SmartPtr<IComponent>( nullptr ) );
            }

            properties->setPropertyAsEnum( XMotionStr, static_cast<s32>( m_axisX ), AxisMotionNames );
            properties->setPropertyAsEnum( YMotionStr, static_cast<s32>( m_axisY ), AxisMotionNames );
            properties->setPropertyAsEnum( ZMotionStr, static_cast<s32>( m_axisZ ), AxisMotionNames );

            properties->setPropertyAsEnum( Swing1MotionStr, static_cast<s32>( m_swing1 ),
                                           AxisMotionNames );
            properties->setPropertyAsEnum( Swing2MotionStr, static_cast<s32>( m_swing2 ),
                                           AxisMotionNames );
            properties->setPropertyAsEnum( TwistMotionStr, static_cast<s32>( m_twist ),
                                           AxisMotionNames );

            properties->setProperty( BreakForceStr, getBreakForce() );
            properties->setProperty( BreakTorqueStr, getBreakTorque() );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            // We return the properties we have so far even if some failed to be set
        }

        return properties;
    }

    void Constraint::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        try
        {
            Component::setProperties( properties );

            s32 iConstraintType = static_cast<s32>( getType() );
            properties->getPropertyValue( ConstraintTypeStr, iConstraintType );
            if( iConstraintType >= 0 && iConstraintType < static_cast<s32>( Type::Count ) )
            {
                setType( static_cast<Type>( iConstraintType ) );
            }
            else
            {
                WP_LOG_ERROR( "Constraint::setProperties - Invalid constraint type: " +
                              std::to_string( iConstraintType ) );
            }

            auto setAxisEnum = [&]( const String &propName, physics::D6MotionEnum &axis ) {
                s32 iVal = static_cast<s32>( axis );
                properties->getPropertyValue( propName, iVal );
                if( iVal >= 0 && iVal < static_cast<s32>( AxisMotionNames.size() ) )
                {
                    axis = static_cast<physics::D6MotionEnum>( iVal );
                }
                else
                {
                    WP_LOG_ERROR( "Constraint::setProperties - Invalid axis motion value for " +
                                  propName + ": " + std::to_string( iVal ) );
                }
            };

            setAxisEnum( XMotionStr, m_axisX );
            setAxisEnum( YMotionStr, m_axisY );
            setAxisEnum( ZMotionStr, m_axisZ );
            setAxisEnum( Swing1MotionStr, m_swing1 );
            setAxisEnum( Swing2MotionStr, m_swing2 );
            setAxisEnum( TwistMotionStr, m_twist );

            real_Num breakForce = getBreakForce();
            real_Num breakTorque = getBreakTorque();
            properties->getPropertyValue( BreakForceStr, breakForce );
            properties->getPropertyValue( BreakTorqueStr, breakTorque );
            setBreakForce( breakForce );
            setBreakTorque( breakTorque );

            SmartPtr<IComponent> bodyAComp;
            SmartPtr<IComponent> bodyBComp;
            properties->getPropertyValue( BodyAStr, bodyAComp );
            properties->getPropertyValue( BodyBStr, bodyBComp );

            SmartPtr<IGameActor> actorA;
            SmartPtr<IGameActor> actorB;
            properties->getPropertyValue( BodyAStr, actorA );
            properties->getPropertyValue( BodyBStr, actorB );

            auto resolveBody = [&]( SmartPtr<IGameActor> actor,
                                    SmartPtr<IComponent> comp ) -> SmartPtr<Rigidbody> {
                if( actor )
                    return actor->getComponent<Rigidbody>();
                if( comp )
                    return workphone::static_pointer_cast<Rigidbody>( comp );
                return nullptr;
            };

            SmartPtr<Rigidbody> bodyA = resolveBody( actorA, bodyAComp );
            SmartPtr<Rigidbody> bodyB = resolveBody( actorB, bodyBComp );

            if( bodyA && bodyB && bodyA == bodyB )
            {
                WP_LOG_ERROR(
                    "Constraint::setProperties - Body A and Body B cannot be the same object." );
            }

            setBodyA( bodyA );
            setBodyB( bodyB );

            updatePhysicsState();
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Array<SmartPtr<ISharedObject>> Constraint::getChildObjects() const
    {
        auto objects = Component::getChildObjects();

        if( auto body0 = getBodyA() )
        {
            objects.emplace_back( body0 );
        }

        if( auto body1 = getBodyB() )
        {
            objects.emplace_back( body1 );
        }

        if( auto constraint = getConstraint() )
        {
            objects.emplace_back( constraint );
        }

        return objects;
    }

    Constraint::Type Constraint::getType() const
    {
        return m_type;
    }

    void Constraint::setType( Type type )
    {
        if( m_type != type )
        {
            m_type = type;

            destroyConstraint();
            updatePhysicsState();
        }
    }

    SmartPtr<physics::IPhysicsConstraint3> Constraint::getConstraint() const
    {
        return m_constraint;
    }

    void Constraint::setConstraint( SmartPtr<physics::IPhysicsConstraint3> constraint )
    {
        m_constraint = constraint;
    }

    void Constraint::setBodyA( SmartPtr<Rigidbody> body )
    {
        auto pThis = getSharedFromThis<Constraint>();

        if( m_body0 )
        {
            m_body0->removeConstraint( pThis );
        }

        m_body0 = body;

        if( m_body0 )
        {
            m_body0->addConstraint( pThis );
        }

        updatePhysicsState();
    }

    void Constraint::setBodyB( SmartPtr<Rigidbody> body )
    {
        auto pThis = getSharedFromThis<Constraint>();

        if( m_body1 )
        {
            m_body1->removeConstraint( pThis );
        }

        m_body1 = body;

        if( m_body1 )
        {
            m_body1->addConstraint( pThis );
        }

        updatePhysicsState();
    }

    void Constraint::createConstraint()
    {
        if( getConstraint() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        auto physics = applicationManager->getPhysicsManager();
        if( !physics )
        {
            WP_LOG_ERROR( "Physics manager is not available." );
            return;
        }

        auto bodyA = getBodyA();
        auto bodyB = getBodyB();

        Transform3<real_Num> worldTransformA;
        Transform3<real_Num> worldTransformB;

        if( bodyA )
        {
            if( auto actor = bodyA->getActor() )
            {
                if( auto transform = actor->getTransform() )
                {
                    worldTransformA = transform->getWorldTransform();
                }
            }
        }

        if( bodyB )
        {
            if( auto actor = bodyB->getActor() )
            {
                if( auto transform = actor->getTransform() )
                {
                    worldTransformB = transform->getWorldTransform();
                }
            }
        }

        SmartPtr<physics::IPhysicsBody3> physBodyA;
        SmartPtr<physics::IPhysicsBody3> physBodyB;

        if( bodyA )
        {
            if( auto dyn = bodyA->getRigidDynamic() )
                physBodyA = dyn;
            else if( auto sta = bodyA->getRigidStatic() )
                physBodyA = sta;
        }

        if( bodyB )
        {
            if( auto dyn = bodyB->getRigidDynamic() )
                physBodyB = dyn;
            else if( auto sta = bodyB->getRigidStatic() )
                physBodyB = sta;
        }

        const auto breakForce = getBreakForce();
        const auto breakTorque = getBreakTorque();

        auto constraintType = getType();
        switch( constraintType )
        {
        case Type::D6:
        {
            auto constraint = physics->addConstraintD6(
                workphone::static_pointer_cast<physics::IRigidDynamic3>( physBodyA ), worldTransformA,
                workphone::static_pointer_cast<physics::IRigidDynamic3>( physBodyB ), worldTransformB );

            if( !constraint )
            {
                WP_LOG_ERROR( "Failed to create D6 constraint." );
                return;
            }

            constraint->setMotion( physics::D6AxisEnum::eX, getAxisX() );
            constraint->setMotion( physics::D6AxisEnum::eY, getAxisY() );
            constraint->setMotion( physics::D6AxisEnum::eZ, getAxisZ() );
            constraint->setMotion( physics::D6AxisEnum::eSWING1, getSwing1() );
            constraint->setMotion( physics::D6AxisEnum::eSWING2, getSwing2() );
            constraint->setMotion( physics::D6AxisEnum::eTWIST, getTwist() );

            constraint->setBreakForce( breakForce, breakTorque );
            constraint->setConstraintFlag( physics::ConstraintFlagEnum::eVISUALIZATION, true );
            setConstraint( constraint );
        }
        break;
        case Type::Fixed:
        {
            auto constraint = physics->addFixedConstraint(
                workphone::static_pointer_cast<physics::IRigidDynamic3>( physBodyA ), worldTransformA,
                workphone::static_pointer_cast<physics::IRigidDynamic3>( physBodyB ), worldTransformB );

            if( !constraint )
            {
                WP_LOG_ERROR( "Failed to create Fixed constraint." );
                return;
            }

            constraint->setBreakForce( breakForce, breakTorque );
            constraint->setConstraintFlag( physics::ConstraintFlagEnum::eVISUALIZATION, true );
            setConstraint( constraint );
        }
        break;
        default:
        {
        }
        break;
        }
    }

    void Constraint::destroyConstraint()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto physics = applicationManager->getPhysicsManager();

        auto constraint = getConstraint();
        if( constraint && physics )
        {
            physics->removeConstraint( constraint );
            setConstraint( nullptr );
        }
    }

    FSMReturnType Constraint::handleComponentEvent( u32 state, FSMEvent eventType )
    {
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
                updatePhysicsState();
            }
            break;
            case State::Destroyed:
            {
            }
            break;
            default:
            {
            }
            };
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
                destroyConstraint();
            }
            break;
            case State::Destroyed:
            {
            }
            break;
            default:
            {
            }
            };
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

    void Constraint::updatePhysicsState()
    {
        if( auto actor = getActor() )
        {
            auto enabled = isEnabled() && actor->isEnabledInScene();
            if( !enabled )
            {
                if( auto constraint = getConstraint() )
                {
                    destroyConstraint();
                }
            }
            else
            {
                auto constraint = getConstraint();
                if( !constraint )
                {
                    createConstraint();
                }
            }
        }

        auto bodyA = getBodyA();
        auto bodyB = getBodyB();

        if( auto constraint = getConstraint() )
        {
            if( bodyA )
            {
                if( auto body = bodyA->getRigidDynamic() )
                {
                    constraint->setBodyA( body );
                }

                if( auto body = bodyA->getRigidStatic() )
                {
                    constraint->setBodyA( body );
                }
            }

            if( bodyB )
            {
                if( auto body = bodyB->getRigidDynamic() )
                {
                    constraint->setBodyB( body );
                }

                if( auto body = bodyB->getRigidStatic() )
                {
                    constraint->setBodyB( body );
                }
            }

            auto breakForce = getBreakForce();
            auto breakTorque = getBreakTorque();

            constraint->setBreakForce( breakForce, breakTorque );

            if( constraint->isDerived<physics::IConstraintD6>() )
            {
                auto constraintD6 = workphone::static_pointer_cast<physics::IConstraintD6>( constraint );

                constraintD6->setMotion( physics::D6AxisEnum::eX, getAxisX() );
                constraintD6->setMotion( physics::D6AxisEnum::eY, getAxisY() );
                constraintD6->setMotion( physics::D6AxisEnum::eZ, getAxisZ() );

                constraintD6->setMotion( physics::D6AxisEnum::eSWING1, getSwing1() );
                constraintD6->setMotion( physics::D6AxisEnum::eSWING2, getSwing2() );
                constraintD6->setMotion( physics::D6AxisEnum::eTWIST, getTwist() );
            }
        }
    }

    SmartPtr<Rigidbody> Constraint::getBodyA() const
    {
        auto p = m_body0.lock();
        return p;
    }

    SmartPtr<Rigidbody> Constraint::getBodyB() const
    {
        auto p = m_body1.lock();
        return p;
    }

    void Constraint::setTwist( physics::D6MotionEnum twist )
    {
        m_twist = twist;

        if( auto constraint = getConstraint() )
        {
            if( constraint->isDerived<physics::IConstraintD6>() )
            {
                auto d6 = workphone::static_pointer_cast<physics::IConstraintD6>( constraint );
                d6->setMotion( physics::D6AxisEnum::eTWIST, twist );
            }
        }
    }

    physics::D6MotionEnum Constraint::getTwist() const
    {
        return m_twist;
    }

    void Constraint::setSwing2( physics::D6MotionEnum swing2 )
    {
        m_swing2 = swing2;

        if( auto constraint = getConstraint() )
        {
            if( constraint->isDerived<physics::IConstraintD6>() )
            {
                auto d6 = workphone::static_pointer_cast<physics::IConstraintD6>( constraint );
                d6->setMotion( physics::D6AxisEnum::eSWING2, swing2 );
            }
        }
    }

    physics::D6MotionEnum Constraint::getSwing2() const
    {
        return m_swing2;
    }

    void Constraint::setSwing1( physics::D6MotionEnum swing1 )
    {
        m_swing1 = swing1;

        if( auto constraint = getConstraint() )
        {
            if( constraint->isDerived<physics::IConstraintD6>() )
            {
                auto d6 = workphone::static_pointer_cast<physics::IConstraintD6>( constraint );
                d6->setMotion( physics::D6AxisEnum::eSWING1, swing1 );
            }
        }
    }

    physics::D6MotionEnum Constraint::getSwing1() const
    {
        return m_swing1;
    }

    void Constraint::setAxisZ( physics::D6MotionEnum axisZ )
    {
        m_axisZ = axisZ;

        if( auto constraint = getConstraint() )
        {
            if( constraint->isDerived<physics::IConstraintD6>() )
            {
                auto d6 = workphone::static_pointer_cast<physics::IConstraintD6>( constraint );
                d6->setMotion( physics::D6AxisEnum::eZ, axisZ );
            }
        }
    }

    physics::D6MotionEnum Constraint::getAxisZ() const
    {
        return m_axisZ;
    }

    void Constraint::setAxisY( physics::D6MotionEnum axisY )
    {
        m_axisY = axisY;

        if( auto constraint = getConstraint() )
        {
            if( constraint->isDerived<physics::IConstraintD6>() )
            {
                auto d6 = workphone::static_pointer_cast<physics::IConstraintD6>( constraint );
                d6->setMotion( physics::D6AxisEnum::eY, axisY );
            }
        }
    }

    physics::D6MotionEnum Constraint::getAxisY() const
    {
        return m_axisY;
    }

    void Constraint::setAxisX( physics::D6MotionEnum axisX )
    {
        m_axisX = axisX;

        if( auto constraint = getConstraint() )
        {
            if( constraint->isDerived<physics::IConstraintD6>() )
            {
                auto d6 = workphone::static_pointer_cast<physics::IConstraintD6>( constraint );
                d6->setMotion( physics::D6AxisEnum::eX, axisX );
            }
        }
    }

    physics::D6MotionEnum Constraint::getAxisX() const
    {
        return m_axisX;
    }

    void Constraint::setBreakTorque( real_Num breakTorque )
    {
        m_breakTorque = breakTorque;

        if( auto constraint = getConstraint() )
        {
            constraint->setBreakForce( m_breakForce, m_breakTorque );
        }
    }

    real_Num Constraint::getBreakTorque() const
    {
        return m_breakTorque;
    }

    void Constraint::setBreakForce( real_Num breakForce )
    {
        m_breakForce = breakForce;

        if( auto constraint = getConstraint() )
        {
            constraint->setBreakForce( m_breakForce, m_breakTorque );
        }
    }

    real_Num Constraint::getBreakForce() const
    {
        return m_breakForce;
    }
}  // namespace workphone::scene
