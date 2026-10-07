#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CarController.hpp>
#include <Workphone/Scene/Components/WheelController.hpp>
#include <Workphone/Scene/Components/CollisionBox.hpp>
#include <Workphone/Scene/Components/Rigidbody.hpp>
#include <Workphone/Interface/Graphics/IDebug.hpp>
#include <Workphone/Interface/Graphics/IDebugLine.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Input/IInputEvent.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/Interface/Input/IJoystickState.hpp>
#include <Workphone/Interface/Input/IInputDeviceManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IRaycastHit.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/Interface/Vehicle/IVehicleManager.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, CarController, VehicleController );

    // Static property key definitions
    const String CarController::radiusStr = "radius";
    const String CarController::suspensionTravelStr = "suspensionTravel";
    const String CarController::dampingStr = "damping";
    const String CarController::driveTypeStr = "Drive Type";

    const Array<String> CarController::driveTypesEnumTypes = Array<String>(
        { "All Wheel Drive", "Four Wheel Drive", "Rear Wheel Drive", "Front Wheel Drive" } );

    CarController::CarController() = default;

    CarController::~CarController() = default;

    void CarController::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            VehicleController::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            m_vehicleCallback = workphone::make_ptr<VehicleCallback>();
            m_vehicleCallback->setOwner( this );

            // IVehicle has multiple registered implementations (cars and aircraft).
            // Use the factory hint intended for polymorphic creation so this scene
            // component cannot resolve to an aircraft and lose its wheel controllers.
            auto vehicleController = factoryManager->make_object<vehicle::IVehicle>( "CCarController" );
            setVehicleController( vehicleController );

            if( vehicleController )
            {
                vehicleController->load( nullptr );
                vehicleController->setVehicleCallback( m_vehicleCallback );
                vehicleController->setDisplayDebugData( true );

                auto vehicleManager = applicationManager->getVehicleManager();
                if( vehicleManager )
                {
                    vehicleManager->addVehicle( vehicleController );
                }
            }

            auto actor = getActor();
            if( actor )
            {
                m_collision = actor->getComponent<Collision>();
                m_chassis = actor->getComponent<Rigidbody>();
            }

            if( m_chassis )
            {
                m_chassis->setMass( m_mass );
            }

            if( vehicleController )
            {
                vehicleController->setMass( m_mass );
            }

            auto collisionBox = workphone::dynamic_pointer_cast<CollisionBox>( m_collision );
            if( collisionBox )
            {
                auto extents = Vector3<real_Num>( m_width * 0.5f, m_height * 0.5f, m_length * 0.5f );
                collisionBox->setExtents( extents );
            }

            auto inputManager = applicationManager->getInputDeviceManager();
            if( inputManager )
            {
                auto inputListener = workphone::make_ptr<InputListener>();
                inputListener->setOwner( this );
                inputManager->addListener( inputListener );
                m_inputListener = inputListener;
            }

            m_wheels.reserve( 4 );
            m_poweredWheels.reserve( 4 );
            m_wheelSpinAngles.reserve( 4 );
            m_wheelBaseOrientations.reserve( 4 );

            auto moi = Vector3<real_Num>::unit() * 1000.0f;
            setMOI( moi );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CarController::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                if( m_chassis )
                {
                    m_chassis->unload( data );
                    m_chassis = nullptr;
                }

                if( m_collision )
                {
                    m_collision->unload( data );
                    m_collision = nullptr;
                }

                if( m_inputListener )
                {
                    m_inputListener->unload( data );
                    m_inputListener = nullptr;
                }

                if( m_vehicleController )
                {
                    auto vehicleManager = applicationManager->getVehicleManager();
                    if( vehicleManager )
                    {
                        vehicleManager->removeVehicle( m_vehicleController );
                    }

                    m_vehicleController->unload( data );
                    m_vehicleController = nullptr;
                }

                m_vehicleCallback = nullptr;

                m_collision = nullptr;
                m_chassis = nullptr;

                for( auto wheel : m_wheels )
                {
                    wheel->unload( data );
                }

                m_wheels.clear();
                m_poweredWheels.clear();
                m_wheelSpinAngles.clear();
                m_wheelBaseOrientations.clear();

                VehicleController::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CarController::update()
    {
        try
        {
            // Scene updates run on several tasks. Animate wheel transforms once
            // on the application task, never while the render task samples them.
            auto application = core::IApplicationManager::instancePtr();
            const auto task = Thread::getCurrentTask();
            const auto gameManager = application ? application->getGameManager() : nullptr;
            const auto stateTask = gameManager ? gameManager->getStateTask() : TaskId::Primary;
            if( application && task != stateTask )
                return;
            VehicleController::update();

            auto app = core::IApplicationManager::instancePtr();
            if( app && task == stateTask )
            {
                ScopedLock controlsLock( this, true );
                const bool driving = app->isPlaying() && !app->isPaused() && isEnabled();
                if( driving && m_playerControls.load() )
                    if( auto input = app->getInputDeviceManager() )
                    {
                        const bool throttle = input->isKeyPressed( KeyCodes::KEY_KEY_W ) ||
                                              input->isKeyPressed( KeyCodes::KEY_UP );
                        const bool brake = input->isKeyPressed( KeyCodes::KEY_KEY_S ) ||
                                           input->isKeyPressed( KeyCodes::KEY_DOWN );
                        const bool left = input->isKeyPressed( KeyCodes::KEY_KEY_A ) ||
                                          input->isKeyPressed( KeyCodes::KEY_LEFT );
                        const bool right = input->isKeyPressed( KeyCodes::KEY_KEY_D ) ||
                                           input->isKeyPressed( KeyCodes::KEY_RIGHT );
                        if( throttle || brake || left || right )
                            m_joystickActive = false;
                        if( !m_joystickActive.load() )
                        {
                            setThrottle( throttle ? 1.f : 0.f );
                            setBrake( brake ? 1.f : 0.f );
                            setSteering( float( right ) - float( left ) );
                        }
                    }
                if( auto vehicle = getVehicleController() )
                {
                    vehicle->setChannel( s32( vehicle::IVehicle::Input::THROTTLE ),
                                         driving ? getThrottle() : 0.f );
                    vehicle->setChannel( s32( vehicle::IVehicle::Input::BRAKE ),
                                         driving ? getBrake() : 0.f );
                    float steering = getSteering();
                    if( m_playerControls.load() && m_chassis )
                    {
                        auto front = vehicle->getWheelController( 0 );
                        auto rear = vehicle->getWheelController( 2 );
                        if( front && rear )
                        {
                            const auto wheelbase =
                                MathF::Abs( float( front->getLocalTransform().getPosition().Z() -
                                                   rear->getLocalTransform().getPosition().Z() ) );
                            const auto speed = float( m_chassis->getLinearVelocity().length() );
                            const auto maxSteer = MathF::Abs( MathF::DegToRad( m_maxSteeringAngle ) );
                            if( maxSteer > 0 && wheelbase > 0 )
                                steering *=
                                    std::min( 1.f, wheelbase * 8.f /
                                                       ( std::max( speed * speed, 1.f ) * maxSteer ) );
                        }
                    }
                    vehicle->setChannel( s32( vehicle::IVehicle::Input::STEERING ),
                                         driving ? steering : 0.f );
                }
            }

            if( m_wheels.empty() )
            {
                setupWheels();
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            auto timer = applicationManager->getTimerPtr();
            if( !timer )
            {
                return;
            }

            const auto dt = static_cast<real_Num>( timer->getDeltaTime() );
            if( dt <= std::numeric_limits<real_Num>::epsilon() )
            {
                return;
            }

            const auto twoPi = static_cast<real_Num>( 2.0 ) * Math<real_Num>::pi();
            // Positive wheel speed follows -Z. Up cross forward gives the rolling
            // axis (-X), so the bottom of the tyre moves opposite vehicle travel.
            const auto spinAxis =
                Vector3<real_Num>::unitY().crossProduct( Vector3<real_Num>::forward() );

            if( m_wheelSpinAngles.size() < m_wheels.size() )
            {
                m_wheelSpinAngles.resize( m_wheels.size(), static_cast<real_Num>( 0.0 ) );
            }

            if( m_wheelBaseOrientations.size() < m_wheels.size() )
            {
                m_wheelBaseOrientations.resize( m_wheels.size(), Quaternion<real_Num>::identity() );
            }

            for( u32 i = 0; i < m_wheels.size(); ++i )
            {
                auto wheel = m_wheels[i];
                if( !wheel )
                {
                    continue;
                }

                auto wheelActor = wheel->getActor();
                auto wheelController = wheel->getWheelController();
                if( !wheelActor || !wheelController )
                {
                    continue;
                }

                m_wheelSpinAngles[i] +=
                    static_cast<real_Num>( wheelController->getAngularVelocity() ) * dt;

                while( m_wheelSpinAngles[i] > twoPi )
                {
                    m_wheelSpinAngles[i] -= twoPi;
                }

                while( m_wheelSpinAngles[i] < -twoPi )
                {
                    m_wheelSpinAngles[i] += twoPi;
                }

                auto steeringAngle = static_cast<real_Num>( 0.0 );
                if( wheelController->isSteeringWheel() )
                {
                    steeringAngle = static_cast<real_Num>( wheelController->getSteeringAngle() );
                    if( Math<real_Num>::Abs( steeringAngle ) <=
                        std::numeric_limits<real_Num>::epsilon() )
                    {
                        steeringAngle = static_cast<real_Num>( m_steering ) *
                                        static_cast<real_Num>( m_maxSteeringAngle );
                    }
                }

                const auto steering =
                    Quaternion<real_Num>::eulerDegrees( 0.0, steeringAngle * m_visualSteeringSign, 0.0 );
                const auto spin = Quaternion<real_Num>::angleAxis( m_wheelSpinAngles[i], spinAxis );
                wheelActor->setLocalOrientation( m_wheelBaseOrientations[i] * steering * spin );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CarController::setupWheels()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto vehicleController = getVehicleController();
        if( !vehicleController )
            return;

        // position wheels
        if( auto actor = getActor() )
        {
            auto wheelControllers = actor->getComponentsInChildren<WheelController>();
            if( wheelControllers.size() == 4 )
            {
                m_wheels.clear();
                m_poweredWheels.clear();
                m_wheelSpinAngles.clear();
                m_wheelBaseOrientations.clear();
                m_wheelSpinAngles.resize( wheelControllers.size(), static_cast<real_Num>( 0.0 ) );
                m_wheelBaseOrientations.resize( wheelControllers.size(),
                                                Quaternion<real_Num>::identity() );

                auto halfWheelBase = m_wheelBase / static_cast<real_Num>( 2.0 );
                auto halfWidth = m_width / static_cast<real_Num>( 2.0 );

                auto frontLeft = wheelControllers[static_cast<u32>( Wheels::FRONT_LEFT )];
                if( frontLeft )
                {
                    auto frontLeftActor = frontLeft->getActor();
                    m_wheels.emplace_back( frontLeft );

                    auto wheelPosition =
                        Vector3<real_Num>( -halfWidth, -m_wheelChassisOffset, -halfWheelBase );

                    auto localWheelTransform = Transform3<real_Num>();
                    localWheelTransform.setPosition( wheelPosition );

                    frontLeftActor->setLocalPosition( wheelPosition );

                    auto wheelController =
                        vehicleController->getWheelController( static_cast<u32>( Wheels::FRONT_LEFT ) );
                    if( wheelController )
                    {
                        frontLeft->setWheelController( wheelController );
                        wheelController->setLocalTransform( localWheelTransform );
                        wheelController->setSteeringWheel( true );
                        wheelController->setPoweredWheel(
                            m_driveType == VehicleDriveType::AllWheelDrive ||
                            m_driveType == VehicleDriveType::FourWheelDrive ||
                            m_driveType == VehicleDriveType::FrontWheelDrive );
                        wheelController->setRadius( m_radius );
                        wheelController->setSuspensionTravel( m_suspensionTravel );
                        wheelController->setDamping( m_damping );

                        if( wheelController->isPoweredWheel() )
                        {
                            m_poweredWheels.emplace_back( frontLeft );
                        }
                    }

                    if( frontLeftActor )
                    {
                        m_wheelBaseOrientations[static_cast<u32>( Wheels::FRONT_LEFT )] =
                            frontLeftActor->getLocalOrientation();
                    }
                }

                auto frontRight = wheelControllers[static_cast<u32>( Wheels::FRONT_RIGHT )];
                if( frontRight )
                {
                    auto frontRightActor = frontRight->getActor();
                    m_wheels.emplace_back( frontRight );

                    auto wheelPosition =
                        Vector3<real_Num>( halfWidth, -m_wheelChassisOffset, -halfWheelBase );

                    auto localWheelTransform = Transform3<real_Num>();
                    localWheelTransform.setPosition( wheelPosition );

                    frontRightActor->setLocalPosition( wheelPosition );

                    auto wheelController =
                        vehicleController->getWheelController( static_cast<u32>( Wheels::FRONT_RIGHT ) );
                    if( wheelController )
                    {
                        frontRight->setWheelController( wheelController );
                        wheelController->setLocalTransform( localWheelTransform );
                        wheelController->setSteeringWheel( true );
                        wheelController->setPoweredWheel(
                            m_driveType == VehicleDriveType::AllWheelDrive ||
                            m_driveType == VehicleDriveType::FourWheelDrive ||
                            m_driveType == VehicleDriveType::FrontWheelDrive );
                        wheelController->setRadius( m_radius );
                        wheelController->setSuspensionTravel( m_suspensionTravel );
                        wheelController->setDamping( m_damping );

                        if( wheelController->isPoweredWheel() )
                        {
                            m_poweredWheels.emplace_back( frontRight );
                        }
                    }

                    if( frontRightActor )
                    {
                        m_wheelBaseOrientations[static_cast<u32>( Wheels::FRONT_RIGHT )] =
                            frontRightActor->getLocalOrientation();
                    }
                }

                auto rearLeft = wheelControllers[static_cast<u32>( Wheels::REAR_LEFT )];
                if( rearLeft )
                {
                    auto frontLeftActor = rearLeft->getActor();
                    m_wheels.emplace_back( rearLeft );

                    auto wheelPosition =
                        Vector3<real_Num>( -halfWidth, -m_wheelChassisOffset, halfWheelBase );

                    auto localWheelTransform = Transform3<real_Num>();
                    localWheelTransform.setPosition( wheelPosition );

                    frontLeftActor->setLocalPosition( wheelPosition );

                    auto wheelController =
                        vehicleController->getWheelController( static_cast<u32>( Wheels::REAR_LEFT ) );
                    if( wheelController )
                    {
                        rearLeft->setWheelController( wheelController );
                        wheelController->setLocalTransform( localWheelTransform );
                        wheelController->setSteeringWheel( false );
                        wheelController->setPoweredWheel(
                            m_driveType == VehicleDriveType::AllWheelDrive ||
                            m_driveType == VehicleDriveType::FourWheelDrive ||
                            m_driveType == VehicleDriveType::RearWheelDrive );
                        wheelController->setRadius( m_radius );
                        wheelController->setSuspensionTravel( m_suspensionTravel );
                        wheelController->setDamping( m_damping );

                        if( wheelController->isPoweredWheel() )
                        {
                            m_poweredWheels.emplace_back( rearLeft );
                        }
                    }

                    if( frontLeftActor )
                    {
                        m_wheelBaseOrientations[static_cast<u32>( Wheels::REAR_LEFT )] =
                            frontLeftActor->getLocalOrientation();
                    }
                }

                auto rearRight = wheelControllers[static_cast<u32>( Wheels::REAR_RIGHT )];
                if( rearRight )
                {
                    auto frontRightActor = rearRight->getActor();
                    m_wheels.emplace_back( rearRight );

                    auto wheelPosition =
                        Vector3<real_Num>( halfWidth, -m_wheelChassisOffset, halfWheelBase );

                    auto localWheelTransform = Transform3<real_Num>();
                    localWheelTransform.setPosition( wheelPosition );

                    frontRightActor->setLocalPosition( wheelPosition );

                    auto wheelController =
                        vehicleController->getWheelController( static_cast<u32>( Wheels::REAR_RIGHT ) );
                    if( wheelController )
                    {
                        rearRight->setWheelController( wheelController );
                        wheelController->setLocalTransform( localWheelTransform );
                        wheelController->setSteeringWheel( false );
                        wheelController->setPoweredWheel(
                            m_driveType == VehicleDriveType::AllWheelDrive ||
                            m_driveType == VehicleDriveType::FourWheelDrive ||
                            m_driveType == VehicleDriveType::RearWheelDrive );
                        wheelController->setRadius( m_radius );
                        wheelController->setSuspensionTravel( m_suspensionTravel );
                        wheelController->setDamping( m_damping );

                        if( wheelController->isPoweredWheel() )
                        {
                            m_poweredWheels.emplace_back( rearRight );
                        }
                    }

                    if( frontRightActor )
                    {
                        m_wheelBaseOrientations[static_cast<u32>( Wheels::REAR_RIGHT )] =
                            frontRightActor->getLocalOrientation();
                    }
                }
            }
        }
    }

    CarController::VehicleCallback::VehicleCallback( CarController *controller ) : m_owner( controller )
    {
    }

    CarController::VehicleCallback::VehicleCallback() = default;

    CarController::VehicleCallback::~VehicleCallback()
    {
        m_owner = nullptr;
    }

    auto CarController::VehicleCallback::getData( const String &filePath ) -> String
    {
        return {};
    }

    void CarController::VehicleCallback::addForce( s32 bodyId, const Vector3<real_Num> &force,
                                                   const Vector3<real_Num> &pos )
    {
        if( auto owner = getOwner() )
        {
            if( auto chassis = owner->getChassis() )
            {
                chassis->addForce( force );
            }
        }
    }

    void CarController::VehicleCallback::addTorque( s32 bodyId, const Vector3<real_Num> &torque )
    {
        if( auto owner = getOwner() )
        {
            if( auto chassis = owner->getChassis() )
            {
                chassis->addTorque( torque );
            }
        }
    }

    void CarController::VehicleCallback::addLocalForce( s32 bodyId, const Vector3<real_Num> &force,
                                                        const Vector3<real_Num> &pos )
    {
    }

    void CarController::VehicleCallback::addLocalTorque( s32 bodyId, const Vector3<real_Num> &torque )
    {
    }

    auto CarController::VehicleCallback::getAngularVelocity() const -> Vector3<real_Num>
    {
        if( auto owner = getOwner() )
        {
            if( auto chassis = owner->getChassis() )
            {
                return chassis->getAngularVelocity();
            }
        }

        return Vector3<real_Num>::zero();
    }

    auto CarController::VehicleCallback::getLinearVelocity() const -> Vector3<real_Num>
    {
        if( auto owner = getOwner() )
        {
            if( auto chassis = owner->getChassis() )
            {
                return chassis->getLinearVelocity();
            }
        }

        return Vector3<real_Num>::zero();
    }

    auto CarController::VehicleCallback::getLocalAngularVelocity() const -> Vector3<real_Num>
    {
        if( auto owner = getOwner() )
        {
            if( auto chassis = owner->getChassis() )
            {
                return chassis->getLocalAngularVelocity();
            }
        }

        return Vector3<real_Num>::zero();
    }

    auto CarController::VehicleCallback::getLocalLinearVelocity() const -> Vector3<real_Num>
    {
        if( auto owner = getOwner() )
        {
            if( auto chassis = owner->getChassis() )
            {
                return chassis->getLocalLinearVelocity();
            }
        }

        return Vector3<real_Num>::zero();
    }

    auto CarController::VehicleCallback::getScale() const -> Vector3<real_Num>
    {
        if( auto owner = getOwner() )
        {
            if( auto actor = owner->getActor() )
            {
                return actor->getScale();
            }
        }

        return Vector3<real_Num>::zero();
    }

    auto CarController::VehicleCallback::getPosition() const -> Vector3<real_Num>
    {
        if( auto owner = getOwner() )
        {
            if( auto actor = owner->getActor() )
            {
                if( auto rb = owner->getChassis() )
                {
                    if( auto r = rb->getRigidDynamic() )
                    {
                        auto transform = r->getTransform();
                        return transform.getPosition();
                    }
                }

                return actor->getPosition();
            }
        }

        return Vector3<real_Num>::zero();
    }

    auto CarController::VehicleCallback::getOrientation() const -> Quaternion<real_Num>
    {
        if( auto owner = getOwner() )
        {
            if( auto actor = owner->getActor() )
            {
                if( auto rb = owner->getChassis() )
                {
                    if( auto r = rb->getRigidDynamic() )
                    {
                        auto transform = r->getTransform();
                        return transform.getOrientation();
                    }
                }

                return actor->getOrientation();
            }
        }

        return Quaternion<real_Num>::identity();
    }

    void CarController::VehicleCallback::displayLocalVector( s32 Bdy, const Vector3<real_Num> &V,
                                                             const Vector3<real_Num> &Org,
                                                             s32 colour ) const
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto debug = graphicsSystem->getDebug();
        WP_ASSERT( debug );

        debug->drawLine( 0, V, Org, colour );
    }

    void CarController::VehicleCallback::displayLocalVector( s32 Bdy, s32 id, const Vector3<real_Num> &V,
                                                             const Vector3<real_Num> &Org,
                                                             s32 colour ) const
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto debug = graphicsSystem->getDebug();
        WP_ASSERT( debug );

        debug->drawLine( id, V, Org, colour );
    }

    auto CarController::VehicleCallback::getCallbackFunction() const -> void *
    {
        return nullptr;
    }

    void CarController::VehicleCallback::setCallbackFunction( void *callbackFunction )
    {
    }

    auto CarController::VehicleCallback::getCallbackDataFunction() const -> void *
    {
        return nullptr;
    }

    void CarController::VehicleCallback::setCallbackDataFunction( void *callbackDataFunction )
    {
    }

    auto CarController::VehicleCallback::getInputData() const -> FixedArray<f32, 8>
    {
        return {};
    }

    auto CarController::VehicleCallback::getControlAngles() const -> FixedArray<f32, 11>
    {
        return {};
    }

    auto CarController::VehicleCallback::getPointVelocity( const Vector3<real_Num> &p )
        -> Vector3<real_Num>
    {
        auto chassis = m_owner->getChassis();
        return chassis->getPointVelocity( p );
    }

    auto CarController::VehicleCallback::castLocalRay( const Ray3<real_Num> &ray,
                                                       SmartPtr<physics::IRaycastHit> &data ) -> bool
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManagerPtr();

        // auto raycastScene = applicationManager->getRaycastScene();
        auto raycastScene = physicsManager->getPhysicsScene();
        if( raycastScene )
        {
            auto origin = ray.getOrigin();
            auto dir = ray.getDirection();

            if( auto owner = getOwner() )
            {
                if( auto actor = owner->getActor() )
                {
                    if( auto rb = owner->getChassis() )
                    {
                        if( auto rigidDynamic = rb->getRigidDynamic() )
                        {
                            auto worldTransform = rigidDynamic->getTransform();
                            auto worigin = worldTransform.transformPoint( origin );
                            auto wdir = worldTransform.transformVector( dir );

                            return raycastScene->castRay( Ray3<real_Num>( worigin, wdir ), data );
                        }
                    }
                }
            }
        }

        return false;
    }

    auto CarController::VehicleCallback::castWorldRay( const Ray3<real_Num> &ray,
                                                       SmartPtr<physics::IRaycastHit> &data ) -> bool
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManager();

        // auto raycastScene = applicationManager->getRaycastScene();
        auto raycastScene = physicsManager->getPhysicsScene();
        if( raycastScene )
        {
            return raycastScene->castRay( ray, data );
        }

        return false;
    }

    auto CarController::VehicleCallback::getOwner() const -> SmartPtr<CarController>
    {
        auto p = m_owner.lock();
        return p;
    }

    void CarController::VehicleCallback::setOwner( SmartPtr<CarController> owner )
    {
        m_owner = owner;
    }

    void CarController::VehicleCallback::displayVector( s32 Bdy, s32 id, const Vector3<real_Num> &V,
                                                        const Vector3<real_Num> &Org, s32 colour ) const
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto debug = graphicsSystem->getDebug();
        WP_ASSERT( debug );

        debug->drawLine( id, V, Org, colour );
    }

    auto CarController::getProperties() const -> SmartPtr<Properties>
    {
        if( auto properties = VehicleController::getProperties() )
        {
            properties->setProperty( radiusStr, m_radius );
            properties->setProperty( suspensionTravelStr, m_suspensionTravel );
            properties->setProperty( dampingStr, m_damping );
            properties->setProperty( "Visual Steering Sign", m_visualSteeringSign );

            properties->setPropertyAsEnum( driveTypeStr, static_cast<s32>( m_driveType ),
                                           driveTypesEnumTypes );

            return properties;
        }

        return nullptr;
    }

    void CarController::setProperties( SmartPtr<Properties> properties )
    {
        VehicleController::setProperties( properties );

        properties->getPropertyValue( radiusStr, m_radius );
        properties->getPropertyValue( suspensionTravelStr, m_suspensionTravel );
        properties->getPropertyValue( dampingStr, m_damping );
        properties->getPropertyValue( "Visual Steering Sign", m_visualSteeringSign );
        m_visualSteeringSign = m_visualSteeringSign < 0.f ? -1.f : 1.f;

        s32 driveType = 0;
        properties->getPropertyValue( driveTypeStr, driveType );

        m_driveType = static_cast<VehicleDriveType>( driveType );
    }

    auto CarController::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = VehicleController::getChildObjects();
        objects.emplace_back( m_vehicleController );
        objects.emplace_back( m_vehicleCallback );
        return objects;
    }

    auto CarController::getVehicleController() const -> SmartPtr<vehicle::IVehicle>
    {
        return m_vehicleController;
    }

    void CarController::setVehicleController( SmartPtr<vehicle::IVehicle> vehicleController )
    {
        m_vehicleController = vehicleController;
    }

    void CarController::setControls( f32 throttle, f32 brake, f32 steering )
    {
        ScopedLock controlsLock( this, true );
        m_playerControls = false;
        setThrottle( std::clamp( throttle, 0.f, 1.f ) );
        setBrake( std::clamp( brake, 0.f, 1.f ) );
        setSteering( std::clamp( steering, -1.f, 1.f ) );
    }

    void CarController::usePlayerControls()
    {
        ScopedLock controlsLock( this, true );
        if( !m_playerControls.exchange( true ) )
        {
            m_joystickActive = false;
            setThrottle( 0.f );
            setBrake( 0.f );
            setSteering( 0.f );
        }
    }

    auto CarController::getThrottle() const -> f32
    {
        return m_throttle;
    }

    void CarController::setThrottle( f32 throttle )
    {
        m_throttle = throttle;
    }

    auto CarController::getBrake() const -> f32
    {
        return m_brake;
    }

    void CarController::setBrake( f32 brake )
    {
        m_brake = brake;
    }

    auto CarController::getSteering() const -> f32
    {
        return m_steering;
    }

    void CarController::setSteering( f32 steering )
    {
        m_steering = steering;
    }

    auto CarController::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        //WP_ASSERT( Thread::getCurrentTask() == TaskId::Application );

        VehicleController::handleComponentEvent( state, eventType );

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
            {
                setupWheels();

                if( m_vehicleController )
                {
                    m_vehicleController->setState( vehicle::IVehicle::State::EDIT );
                }
            }
            break;
            case State::Play:
            {
                setupWheels();

                if( m_vehicleController )
                {
                    m_vehicleController->setState( vehicle::IVehicle::State::PLAY );
                }
            }
            break;
            default:
            {
            }
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            {
                if( m_vehicleController )
                {
                    m_vehicleController->setState( vehicle::IVehicle::State::RESET );
                }
            }
            break;
            case State::Play:
            {
                if( m_vehicleController )
                {
                    m_vehicleController->setState( vehicle::IVehicle::State::RESET );
                }
            }
            break;
            default:
            {
            }
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

    void CarController::setDriveType( VehicleDriveType driveType )
    {
        m_driveType = driveType;
    }

    VehicleDriveType CarController::getDriveType() const
    {
        return m_driveType;
    }

    void CarController::InputListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    auto CarController::InputListener::handleEvent( EventType eventType, hash_type eventValue,
                                                    const Array<Parameter> &arguments,
                                                    SmartPtr<ISharedObject> sender,
                                                    SmartPtr<ISharedObject> object,
                                                    SmartPtr<IEvent> event ) -> Parameter
    {
        if( eventValue == IEvent::inputEvent )
        {
            auto result = inputEvent( event );
            return Parameter( result );
        }

        return {};
    }

    auto CarController::InputListener::inputEvent( SmartPtr<IInputEvent> event ) -> bool
    {
        auto owner = getOwner();
        if( !owner || !event )
            return false;
        ScopedLock controlsLock( owner.get(), true );
        if( !owner->m_playerControls.load() )
            return false;
        if( event->getEventType() == IInputEvent::EventType::Key )
        {
            if( auto state = event->getKeyboardState() )
            {
                const auto key = state->getKeyCode();
                if( key == u32( KeyCodes::KEY_KEY_W ) || key == u32( KeyCodes::KEY_KEY_S ) ||
                    key == u32( KeyCodes::KEY_KEY_A ) || key == u32( KeyCodes::KEY_KEY_D ) ||
                    key == u32( KeyCodes::KEY_UP ) || key == u32( KeyCodes::KEY_DOWN ) ||
                    key == u32( KeyCodes::KEY_LEFT ) || key == u32( KeyCodes::KEY_RIGHT ) )
                    owner->m_joystickActive = false;
            }
        }
        else if( event->getEventType() == IInputEvent::EventType::Joystick )
        {
            if( auto joystick = event->getJoystickState() )
            {
                float throttle = 0, steering = owner->getSteering();
                if( joystick->getEventType() == u32( IJoystickState::Type::AxisMoved ) )
                {
                    throttle = joystick->getAxis( 4 );
                    steering = joystick->getAxis( 0 );
                }
                else
                    throttle = joystick->isButtonPressed( 0 )   ? 1.f
                               : joystick->isButtonPressed( 1 ) ? -1.f
                                                                : 0.f;
                owner->m_joystickActive = true;
                owner->setThrottle( std::max( throttle, 0.f ) );
                owner->setBrake( std::max( -throttle, 0.f ) );
                owner->setSteering( steering );
            }
        }
        // Only CarController::update publishes channels; input events never write the vehicle.
        return false;
    }

    auto CarController::InputListener::updateEvent( const SmartPtr<IInputEvent> &event ) -> bool
    {
        return false;
    }

    void CarController::InputListener::setPriority( s32 priority )
    {
        m_priority = priority;
    }

    auto CarController::InputListener::getPriority() const -> s32
    {
        return m_priority;
    }

    auto CarController::InputListener::getOwner() const -> SmartPtr<CarController>
    {
        auto p = m_owner.lock();
        return p;
    }

    void CarController::InputListener::setOwner( SmartPtr<CarController> owner )
    {
        m_owner = owner;
    }

    CarController::InputListener::InputListener() = default;

    CarController::InputListener::~InputListener() = default;

}  // namespace workphone::scene
