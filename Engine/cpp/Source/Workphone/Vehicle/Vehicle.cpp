#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Vehicle/Vehicle.hpp>
#include <Workphone/Vehicle/VehicleBody.hpp>
#include <Workphone/Vehicle/WheelController.hpp>
#include <Workphone/Interface/Physics/IRaycastHit.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Vehicle/IDriveTrain.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/Interface/Vehicle/IVehicleCallback.hpp>
#include <Workphone/Math/MathUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, Vehicle, IVehicle );

        Vehicle::Vehicle() = default;

        Vehicle::~Vehicle() = default;

        void Vehicle::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto pThis = getSharedFromThis<Vehicle>();

                m_channels.resize( 12 );

                auto body = factoryManager->make_ptr<VehicleBody>();
                body->setParentVehicle( pThis );
                setBody( body );

                auto worldTransform = Transform3<real_Num>();
                setWorldTransform( worldTransform );

                auto localTransform = Transform3<real_Num>();
                setLocalTransform( localTransform );

                auto bodyTransform = Transform3<real_Num>();
                m_bodyTransform = bodyTransform;

                m_driveTrain = factoryManager->make_object<IDriveTrain>();
                if( m_driveTrain )
                {
                    m_driveTrain->load( data );
                }

                m_wheels.reserve( 4 );

                for( size_t i = 0; i < 4; ++i )
                {
                    //auto w = factoryManager->make_object<IWheelComponent>( "Arcade" );
                    //if( !w )
                    //{
                    //    w = factoryManager->make_ptr<WheelComponent>();
                    //}

                    auto w = factoryManager->make_ptr<WheelComponent>();
                    if( w )
                    {
                        w->load( data );
                        w->setOwner( pThis );

                        m_wheels.push_back( w );
                    }
                }

                if( m_wheels.size() >= 4 )
                {
                    setupDriveWheels();
                }

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void Vehicle::setupDriveWheels()
        {
            auto driveType = getDriveType();
            switch( driveType )
            {
            case VehicleDriveType::FrontWheelDrive:
            {
                m_wheels[0]->setSteeringWheel( true );
                m_wheels[1]->setSteeringWheel( true );
                m_wheels[0]->setPoweredWheel( true );
                m_wheels[1]->setPoweredWheel( true );

                if( m_driveTrain )
                {
                    auto wheels = Array<SmartPtr<IWheelComponent>>( { m_wheels[0], m_wheels[1] } );
                    m_driveTrain->setWheels( wheels );
                }
            }
            break;
            case VehicleDriveType::RearWheelDrive:
            {
                m_wheels[0]->setSteeringWheel( true );
                m_wheels[1]->setSteeringWheel( true );

                m_wheels[0]->setPoweredWheel( false );
                m_wheels[1]->setPoweredWheel( false );
                m_wheels[2]->setPoweredWheel( true );
                m_wheels[3]->setPoweredWheel( true );

                if( m_driveTrain )
                {
                    auto wheels = Array<SmartPtr<IWheelComponent>>( { m_wheels[2], m_wheels[3] } );
                    m_driveTrain->setWheels( wheels );
                }
            }
            break;
            case VehicleDriveType::AllWheelDrive:
            {
                m_wheels[0]->setSteeringWheel( true );
                m_wheels[1]->setSteeringWheel( true );
                m_wheels[0]->setPoweredWheel( true );
                m_wheels[1]->setPoweredWheel( true );
                m_wheels[2]->setPoweredWheel( true );
                m_wheels[3]->setPoweredWheel( true );
                if( m_driveTrain )
                {
                    auto wheels = Array<SmartPtr<IWheelComponent>>(
                        { m_wheels[0], m_wheels[1], m_wheels[2], m_wheels[3] } );
                    m_driveTrain->setWheels( wheels );
                }
            }
            break;
            case VehicleDriveType::FourWheelDrive:
            {
                m_wheels[0]->setSteeringWheel( true );
                m_wheels[1]->setSteeringWheel( true );

                m_wheels[0]->setPoweredWheel( true );
                m_wheels[1]->setPoweredWheel( true );
                m_wheels[2]->setPoweredWheel( true );
                m_wheels[3]->setPoweredWheel( true );

                if( m_driveTrain )
                {
                    auto wheels = Array<SmartPtr<IWheelComponent>>(
                        { m_wheels[0], m_wheels[1], m_wheels[2], m_wheels[3] } );
                    m_driveTrain->setWheels( wheels );
                }
            }
            break;
            default:
            {
            }
            };
        }

        void Vehicle::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );

                for( auto &w : m_wheels )
                {
                    w->unload( data );
                }

                m_wheels.clear();

                if( auto body = workphone::dynamic_pointer_cast<VehicleBody>( getBody() ) )
                {
                    body->setParentVehicle( nullptr );
                    setBody( nullptr );
                }

                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void Vehicle::reset()
        {
            clearForces();

            m_worldTransform = Transform3<real_Num>();
            m_localTransform = Transform3<real_Num>();
            m_bodyTransform = Transform3<real_Num>();

            for( auto &w : m_wheels )
            {
                if( w )
                {
                    w->reset();
                }
            }

            if( m_driveTrain )
            {
                m_driveTrain->reset();
            }
        }

        void Vehicle::update()
        {
            updateTransform();

            auto state = getState();
            switch( state )
            {
            case State::EDIT:
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto timer = applicationManager->getTimerPtr();
                WP_ASSERT( timer );

                auto task = Thread::getCurrentTask();
                auto t = timer->getTime();
                auto dt = timer->getDeltaTime();

                for( auto &w : m_wheels )
                {
                    if( w )
                    {
                        w->update();
                    }
                }

                auto throttleValue = getChannel( 0 );
                auto brakeValue = getChannel( 1 );
                auto steeringValue = getChannel( 2 ) * 70.0f;

                for( auto &w : m_wheels )
                {
                    if( w )
                    {
                        w->setBrake( brakeValue );
                    }
                }

                for( auto &w : m_wheels )
                {
                    if( w )
                    {
                        if( w->isSteeringWheel() )
                        {
                            w->setSteeringAngle( steeringValue );
                        }
                    }
                }

                if( m_driveTrain )
                {
                    m_driveTrain->setThrottleInput( throttleValue );
                    m_driveTrain->update();
                }
            }
            break;
            case State::PLAY:
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto timer = applicationManager->getTimerPtr();
                WP_ASSERT( timer );

                auto task = Thread::getCurrentTask();
                auto t = timer->getTime();
                auto dt = timer->getDeltaTime();

                for( auto &w : m_wheels )
                {
                    if( w )
                    {
                        w->update();
                    }
                }

                auto throttleValue = getChannel( 0 );
                auto brakeValue = getChannel( 1 );
                auto steeringValue = getChannel( 2 ) * 50.0f;

                for( auto &w : m_wheels )
                {
                    if( w )
                    {
                        w->setBrake( brakeValue );
                    }
                }

                for( auto &w : m_wheels )
                {
                    if( w )
                    {
                        if( w->isSteeringWheel() )
                        {
                            w->setSteeringAngle( steeringValue );
                        }
                    }
                }

                if( auto driveTrain = getDriveTrain() )
                {
                    driveTrain->setThrottleInput( throttleValue );
                    driveTrain->setThrottle( throttleValue );
                    driveTrain->update();
                }
            }
            break;
            default:
            {
            }
            }

            if( m_callback )
            {
                auto linearVelocity = m_callback->getLinearVelocity();
                m_force -=
                    Vector3<real_Num>( linearVelocity.X() * m_drag.X(), linearVelocity.Y() * m_drag.Y(),
                                       linearVelocity.Z() * m_drag.Z() );

                m_callback->addForce( 0, m_force, Vector3<real_Num>::zero() );
                m_callback->addTorque( 0, m_torque );

                m_force = Vector3<real_Num>::zero();
                m_torque = Vector3<real_Num>::zero();
            }
        }

        f32 Vehicle::getChannel( s32 idx ) const
        {
            return m_channels[idx];
        }

        void Vehicle::setChannel( s32 idx, f32 channel )
        {
            m_channels[idx] = channel;
        }

        void Vehicle::updateTransform()
        {
            auto p = getPosition();
            auto q = getOrientation();
            auto s = getScale();

            WP_ASSERT( MathUtil<real_Num>::isFinite( p ) );
            WP_ASSERT( MathUtil<real_Num>::isFinite( q ) );

            // p = Vector3<real_Num>::zero();
            // q = Quaternion<real_Num>::identity();

            // if (m_bodyTransform)
            {
                m_bodyTransform.setPosition( p );
                m_bodyTransform.setOrientation( q );
                m_bodyTransform.setScale( s );

                // if (m_worldTransform)
                {
                    // if (m_localTransform)
                    {
                        m_worldTransform.transformFromParent( m_bodyTransform, m_localTransform );
                    }
                }
            }

            if( auto body = getBody() )
            {
                auto cg = m_bodyTransform.transformPoint( m_cg );
                body->setWorldCenterOfMass( cg );
            }
        }

        Vector3<real_Num> Vehicle::getPosition() const
        {
            if( m_callback )
            {
                return m_callback->getPosition();
            }

            return Vector3<real_Num>::zero();
        }

        Vector3<real_Num> Vehicle::getScale() const
        {
            if( m_callback )
            {
                return m_callback->getScale();
            }

            return Vector3<real_Num>::zero();
        }

        void Vehicle::setPosition( const Vector3<real_Num> &position )
        {
            m_worldTransform.setPosition( position );
        }

        Quaternion<real_Num> Vehicle::getOrientation() const
        {
            if( m_callback )
            {
                return m_callback->getOrientation();
            }

            return Quaternion<real_Num>::identity();
        }

        void Vehicle::setOrientation( const Quaternion<real_Num> &orientation )
        {
            m_worldTransform.setOrientation( orientation );
        }

        bool Vehicle::isUserControlled() const
        {
            return m_userControlled;
        }

        void Vehicle::setUserControlled( bool userControlled )
        {
            m_userControlled = userControlled;
        }

        real_Num Vehicle::getMass() const
        {
            if( auto body = getBody() )
            {
                return body->getMass();
            }

            return static_cast<real_Num>( 1.0 );
        }

        void Vehicle::setMass( real_Num mass )
        {
            if( auto body = getBody() )
            {
                body->setMass( mass );
            }
        }

        IVehicleBody *Vehicle::getBodyPtr() const
        {
            return m_rigidbody.get();
        }

        SmartPtr<IVehicleBody> Vehicle::getBody() const
        {
            return m_rigidbody;
        }

        void Vehicle::setBody( SmartPtr<IVehicleBody> body )
        {
            m_rigidbody = body;
        }

        Transform3<real_Num> Vehicle::getWorldTransform() const
        {
            return m_worldTransform;
        }

        void Vehicle::setWorldTransform( const Transform3<real_Num> &worldTransform )
        {
            m_worldTransform = worldTransform;
        }

        Transform3<real_Num> Vehicle::getLocalTransform() const
        {
            return m_localTransform;
        }

        void Vehicle::setLocalTransform( const Transform3<real_Num> &localTransform )
        {
            m_localTransform = localTransform;
        }

        void Vehicle::drawPoint( s32 body, int id, const Vector3<real_Num> &positon, u32 color )
        {
            auto size = static_cast<real_Num>( 0.1 );
            auto offset0 = Vector3<real_Num>::forward() * size;
            auto offset1 = Vector3<real_Num>::up() * size;
            auto offset2 = Vector3<real_Num>::right() * size;

            auto p = m_worldTransform.getOrientation() * positon;
            p += m_worldTransform.getPosition();

            displayVector( body, id, p + offset0, p - offset0, color );
            displayVector( body, id + 1, p + offset1, p - offset1, color );
            displayVector( body, id + 2, p + offset2, p - offset2, color );
        }

        void Vehicle::drawLocalPoint( s32 body, int id, const Vector3<real_Num> &positon, u32 color )
        {
            auto size = 0.1f;
            auto offset0 = Vector3<real_Num>::forward() * size;
            auto offset1 = Vector3<real_Num>::up() * size;
            auto offset2 = Vector3<real_Num>::right() * size;

            // if (m_worldTransform)
            {
                auto p = m_worldTransform.getOrientation() * positon;
                p += m_worldTransform.getPosition();

                displayVector( body, id, p + offset0, p - offset0, color );
                displayVector( body, id + 1000, p + offset1, p - offset1, color );
                displayVector( body, id + 2000, p + offset2, p - offset2, color );
            }
        }

        void Vehicle::displayLocalVector( s32 bodyId, const Vector3<real_Num> &start,
                                          const Vector3<real_Num> &end, u32 colour )
        {
            try
            {
                if( m_callback )
                {
                    m_callback->displayLocalVector(
                        bodyId, Vector3<real_Num>( start.X(), start.Y(), start.Z() ),
                        Vector3<real_Num>( end.X(), end.Y(), end.Z() ), colour );
                }
            }
            catch( std::exception &Err )
            {
                WP_LOG_EXCEPTION( Err );
            }
        }

        void Vehicle::displayVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                     const Vector3<real_Num> &end, u32 colour )
        {
            try
            {
                if( m_callback )
                {
                    m_callback->displayVector( bodyId, id, start, end, colour );
                }
            }
            catch( std::exception &Err )
            {
                WP_LOG_EXCEPTION( Err );
            }
        }

        void Vehicle::displayLocalVector( s32 bodyId, s32 id, const Vector3<real_Num> &start,
                                          const Vector3<real_Num> &end, u32 colour )
        {
            try
            {
                if( m_callback )
                {
                    auto worldTransform = getWorldTransform();

                    WP_ASSERT( worldTransform.isValid() );

                    WP_ASSERT( start.isValid() );
                    WP_ASSERT( end.isValid() );

                    auto worldStart = worldTransform.transformPoint( start );
                    auto worldEnd = worldTransform.transformPoint( end );

                    WP_ASSERT( worldStart.isValid() );
                    WP_ASSERT( worldEnd.isValid() );

                    m_callback->displayVector( bodyId, id, worldStart, worldEnd, colour );
                }
            }
            catch( std::exception &Err )
            {
                WP_LOG_EXCEPTION( Err );
            }
        }

        bool Vehicle::getDisplayDebugData() const
        {
            return m_displayDebugData;
        }

        void Vehicle::setDisplayDebugData( bool displayDebugData )
        {
            m_displayDebugData = displayDebugData;
        }

        void Vehicle::addForce( s32 bodyIdx, const Vector3<real_Num> &force,
                                const Vector3<real_Num> &loc )
        {
            WP_ASSERT( force.length() < 1e5 );
            WP_ASSERT( loc.length() < 1e5 );

            const Vector3<real_Num> centerOfMass = m_worldTransform.transformPoint( m_cg );
            const Vector3<real_Num> torque = ( loc - centerOfMass ).crossProduct( force );

            addForce( force );
            addTorque( torque );
        }

        void Vehicle::addTorque( s32 bodyIdx, const Vector3<real_Num> &torque )
        {
            WP_ASSERT( torque.length() < 1e5 );
            addTorque( torque );
        }

        void Vehicle::addLocalForce( s32 bodyIdx, const Vector3<real_Num> &force,
                                     const Vector3<real_Num> &loc )
        {
            WP_ASSERT( force.length() < 1e10 );
            WP_ASSERT( loc.length() < 1e10 );

            const auto worldPoint = m_worldTransform.transformPoint( loc );
            const auto f = m_worldTransform.transformVector( force );

            if( auto body = getBody() )
            {
                const auto centerOfMass = body->getWorldCenterOfMass();
                const auto torque = ( worldPoint - centerOfMass ).crossProduct( f );

                addForce( f );
                addTorque( torque );
            }
        }

        void Vehicle::addLocalTorque( s32 bodyIdx, const Vector3<real_Num> &torque )
        {
            WP_ASSERT( torque.length() < 1e5 );

            auto t = m_worldTransform.getOrientation() * torque;
            addTorque( t );
        }

        Vector3<real_Num> Vehicle::getPointVelocity( const Vector3<real_Num> &p )
        {
            if( m_callback )
            {
                return m_callback->getPointVelocity( p );
            }

            return Vector3<real_Num>::zero();
        }

        Vector3<real_Num> Vehicle::getAngularVelocity()
        {
            if( m_callback )
            {
                return m_callback->getAngularVelocity();
            }

            return Vector3<real_Num>::zero();
        }

        Vector3<real_Num> Vehicle::getLinearVelocity()
        {
            if( m_callback )
            {
                return m_callback->getLinearVelocity();
            }

            return Vector3<real_Num>::zero();
        }

        Vector3<real_Num> Vehicle::getLocalAngularVelocity()
        {
            if( m_callback )
            {
                auto worldAngularVelocity = m_callback->getAngularVelocity();
                return m_worldTransform.inverseTransformVector( worldAngularVelocity );
            }

            return Vector3<real_Num>::zero();
        }

        Vector3<real_Num> Vehicle::getLocalLinearVelocity()
        {
            if( m_callback )
            {
                return m_callback->getLocalLinearVelocity();
            }

            return Vector3<real_Num>::zero();
        }

        IVehicleCallback *Vehicle::getVehicleCallbackPtr() const
        {
            return m_callback.get();
        }

        SmartPtr<IVehicleCallback> Vehicle::getVehicleCallback() const
        {
            return m_callback;
        }

        void Vehicle::setVehicleCallback( SmartPtr<IVehicleCallback> callback )
        {
            m_callback = callback;
        }

        Vector3<real_Num> Vehicle::getCG() const
        {
            return m_cg;
        }

        void Vehicle::setState( State state )
        {
            m_vehicleState = state;

            switch( state )
            {
            case State::EDIT:
            {
                for( auto &w : m_wheels )
                {
                    w->setState( IVehicleComponent::State::EDIT );
                }
            }
            break;
            case State::PLAY:
            {
                for( auto &w : m_wheels )
                {
                    w->setState( IVehicleComponent::State::PLAY );
                }
            }
            break;
            default:
            {
            }
            break;
            }
        }

        IVehicle::State Vehicle::getState() const
        {
            return m_vehicleState;
        }

        void Vehicle::addForce( const Vector3<real_Num> &force )
        {
            m_force += force;
        }

        void Vehicle::addTorque( const Vector3<real_Num> &torque )
        {
            m_torque += torque;
        }

        void Vehicle::clearForces()
        {
            m_force = Vector3<real_Num>::zero();
            m_torque = Vector3<real_Num>::zero();
        }

        SmartPtr<IWheelComponent> Vehicle::getWheelController( u32 index ) const
        {
            return m_wheels[index];
        }

        SmartPtr<IDriveTrain> Vehicle::getDriveTrain() const
        {
            return m_driveTrain;
        }

        void Vehicle::setDriveTrain( SmartPtr<IDriveTrain> driveTrain )
        {
            m_driveTrain = driveTrain;
        }

        VehicleDriveType Vehicle::getDriveType() const
        {
            return m_driveType;
        }

        void Vehicle::setDriveType( VehicleDriveType driveType )
        {
            if( m_driveType != driveType )
            {
                m_driveType = driveType;

                // Re-setup the drive wheels based on the new drive type
                setupDriveWheels();
            }
        }

        bool Vehicle::isElectric() const
        {
            return m_electric;
        }

        void Vehicle::setElectric( bool electric )
        {
            m_electric = electric;
        }

        bool Vehicle::getEnablePowerUnit() const
        {
            return m_enablePowerUnit;
        }

        void Vehicle::setEnablePowerUnit( bool enabled )
        {
            m_enablePowerUnit = enabled;
        }

        bool Vehicle::getEmulateBattery() const
        {
            return m_emulateBattery;
        }

        void Vehicle::setEmulateBattery( bool emulate )
        {
            m_emulateBattery = emulate;
        }

    }  // namespace vehicle
}  // namespace workphone
