#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CCarController.hpp>
#include <WPVehiclePhysics/VehicleHandling.hpp>
#include <WPVehiclePhysics/CDriveTrain.hpp >
#include <WPVehiclePhysics/CVehicleBody.hpp>
#include <WPVehiclePhysics/WheelControllerArcade.hpp>
#include <WPVehiclePhysics/WheelControllerBrush.hpp>
#include <WPVehiclePhysics/WheelControllerPacejka.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace
    {
        constexpr auto MinInput = static_cast<physics_Num>( -1.0 );
        constexpr auto MaxInput = static_cast<physics_Num>( 1.0 );
        constexpr auto MaxSteeringScale = static_cast<physics_Num>( 1080.0 );
        constexpr auto MaxWheelForce = static_cast<physics_Num>( 1.0e8 );
        constexpr auto MaxWheelVelocity = static_cast<physics_Num>( 1.0e5 );
        constexpr auto MaxDebugForce = static_cast<physics_Num>( 1.0e8 );

        bool isFiniteValue( physics_Num value )
        {
            return Math<physics_Num>::isFinite( value );
        }

        bool isFiniteVector( const Vector3<physics_Num> &value )
        {
            return MathUtil<physics_Num>::isFinite( value );
        }

        bool isValidInput( physics_Num value )
        {
            return value >= MinInput && value <= MaxInput;
        }

        bool isValidPositiveInput( physics_Num value )
        {
            return value >= static_cast<physics_Num>( 0.0 ) && value <= MaxInput;
        }

        bool isValidDriveTypeValue( VehicleDriveType driveType )
        {
            const auto driveTypeValue = static_cast<u8>( driveType );
            return driveTypeValue < static_cast<u8>( VehicleDriveType::Count );
        }

        void assertTimerValues( real_Num t, real_Num dt )
        {
            WP_ASSERT( Math<real_Num>::isFinite( t ) );
            WP_ASSERT( Math<real_Num>::isFinite( dt ) );
            WP_ASSERT( dt >= static_cast<real_Num>( 0.0 ) );
            WP_ASSERT( dt < static_cast<real_Num>( 1.0 ) );
        }

        void assertControllerRuntimeState( const CCarController &controller )
        {
            const auto worldTransform = controller.getWorldTransform();
            const auto localTransform = controller.getLocalTransform();
            WP_ASSERT( worldTransform.isFinite() );
            WP_ASSERT( localTransform.isFinite() );

            auto body = controller.getBody();
            WP_ASSERT( body );
            if( body )
            {
                const auto mass = body->getMass();
                WP_ASSERT( isFiniteValue( mass ) );
                WP_ASSERT( mass > static_cast<physics_Num>( 0.0 ) );
                WP_ASSERT( mass < static_cast<physics_Num>( 1.0e10 ) );
            }

            WP_ASSERT( isFiniteValue( controller.getEditSteeringScale() ) );
            WP_ASSERT( controller.getEditSteeringScale() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( controller.getEditSteeringScale() <= MaxSteeringScale );
            WP_ASSERT( isFiniteValue( controller.getPlaySteeringScale() ) );
            WP_ASSERT( controller.getPlaySteeringScale() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( controller.getPlaySteeringScale() <= MaxSteeringScale );
            WP_ASSERT( isFiniteValue( controller.getDefaultMass() ) );
            WP_ASSERT( controller.getDefaultMass() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isValidDriveTypeValue( controller.getDriveType() ) );
        }

        void assertWheelConfig( const SmartPtr<IWheelComponent> &wheel, u32 index )
        {
            WP_ASSERT( index < 4 );
            WP_ASSERT( wheel );
            if( !wheel )
            {
                return;
            }

            const auto localTransform = wheel->getLocalTransform();
            const auto worldTransform = wheel->getWorldTransform();
            WP_ASSERT( localTransform.isFinite() );
            WP_ASSERT( worldTransform.isFinite() );

            const auto wheelMass = static_cast<physics_Num>( wheel->getMass() );
            const auto springRate = static_cast<physics_Num>( wheel->getSpringRate() );
            const auto radius = static_cast<physics_Num>( wheel->getRadius() );
            const auto suspensionTravel = static_cast<physics_Num>( wheel->getSuspensionTravel() );
            const auto damping = static_cast<physics_Num>( wheel->getDamping() );
            const auto suspensionDistance = static_cast<physics_Num>( wheel->getSuspensionDistance() );
            const auto steeringAngle = static_cast<physics_Num>( wheel->getSteeringAngle() );
            const auto angularVelocity = static_cast<physics_Num>( wheel->getAngularVelocity() );
            const auto brake = static_cast<physics_Num>( wheel->getBrake() );

            WP_ASSERT( isFiniteValue( wheelMass ) );
            WP_ASSERT( wheelMass >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( springRate ) );
            WP_ASSERT( springRate >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( radius ) );
            WP_ASSERT( radius > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( suspensionTravel ) );
            WP_ASSERT( suspensionTravel > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( damping ) );
            WP_ASSERT( damping >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( suspensionDistance ) );
            WP_ASSERT( suspensionDistance >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( steeringAngle ) );
            WP_ASSERT( Math<physics_Num>::Abs( steeringAngle ) <= MaxSteeringScale );
            WP_ASSERT( isFiniteValue( angularVelocity ) );
            WP_ASSERT( isValidPositiveInput( brake ) );
        }

        void assertPacejkaWheelOutput( const SmartPtr<WheelControllerPacejka> &wheel )
        {
            WP_ASSERT( wheel );
            if( !wheel )
            {
                return;
            }

            const auto suspensionForce = wheel->getSuspensionForceVector();
            const auto roadForce = wheel->getRoadForceVector();
            const auto wheelVelocity = wheel->getWheelVelo();
            const auto localVelocity = wheel->getLocalVelo();
            const auto groundNormal = wheel->getGroundNormal();
            const auto forward = wheel->getForward();
            const auto right = wheel->getRight();
            const auto up = wheel->getUp();
            const auto localRotation = wheel->getLocalRotation();
            const auto inverseLocalRotation = wheel->getInverseLocalRotation();
            const auto compression = wheel->getCompression();
            const auto fullCompressionSpringForce = wheel->getFullCompressionSpringForce();
            const auto normalForce = wheel->getNormalForce();
            const auto slipRatio = wheel->getSlipRatio();
            const auto slipAngle = wheel->getSlipAngle();
            const auto slipVelo = wheel->getSlipVelo();
            const auto maxSlip = wheel->getMaxSlip();
            const auto maxAngle = wheel->getMaxAngle();
            const auto suspensionForceInput = wheel->getSuspensionForceInput();

            WP_ASSERT( isFiniteVector( suspensionForce ) );
            WP_ASSERT( isFiniteVector( roadForce ) );
            WP_ASSERT( suspensionForce.length() < MaxWheelForce );
            WP_ASSERT( roadForce.length() < MaxWheelForce );
            WP_ASSERT( isFiniteVector( wheelVelocity ) );
            WP_ASSERT( isFiniteVector( localVelocity ) );
            WP_ASSERT( wheelVelocity.length() < MaxWheelVelocity );
            WP_ASSERT( localVelocity.length() < MaxWheelVelocity );
            WP_ASSERT( isFiniteVector( groundNormal ) );
            WP_ASSERT( isFiniteVector( forward ) );
            WP_ASSERT( isFiniteVector( right ) );
            WP_ASSERT( isFiniteVector( up ) );
            WP_ASSERT( MathUtil<physics_Num>::isFinite( localRotation ) );
            WP_ASSERT( MathUtil<physics_Num>::isFinite( inverseLocalRotation ) );
            WP_ASSERT( isFiniteValue( compression ) );
            WP_ASSERT( compression >= static_cast<physics_Num>( -0.05 ) );
            WP_ASSERT( compression <= static_cast<physics_Num>( 1.05 ) );
            WP_ASSERT( isFiniteValue( fullCompressionSpringForce ) );
            WP_ASSERT( fullCompressionSpringForce >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( normalForce ) );
            WP_ASSERT( normalForce >= static_cast<physics_Num>( -1.0 ) );
            WP_ASSERT( isFiniteValue( slipRatio ) );
            WP_ASSERT( isFiniteValue( slipAngle ) );
            WP_ASSERT( isFiniteValue( slipVelo ) );
            WP_ASSERT( slipVelo >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( maxSlip ) );
            WP_ASSERT( maxSlip > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( maxAngle ) );
            WP_ASSERT( maxAngle > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( suspensionForceInput ) );

            if( wheel->isOnGround() )
            {
                WP_ASSERT( groundNormal.lengthSquared() > Math<physics_Num>::epsilon() );
            }
            else
            {
                WP_ASSERT( suspensionForce.length() <= Math<physics_Num>::epsilon() );
                WP_ASSERT( roadForce.length() <= Math<physics_Num>::epsilon() );
                WP_ASSERT( compression <= Math<physics_Num>::epsilon() );
            }
        }

        void assertWheelOutput( const SmartPtr<IWheelComponent> &wheel, u32 index )
        {
            assertWheelConfig( wheel, index );
            if( wheel && wheel->isDerived<WheelControllerPacejka>() )
            {
                assertPacejkaWheelOutput(
                    workphone::static_pointer_cast<WheelControllerPacejka>( wheel ) );
            }
        }

        void assertControlInputs( physics_Num throttleValue, physics_Num brakeValue,
                                  physics_Num steeringChannel, physics_Num steeringScale,
                                  physics_Num steeringValue )
        {
            WP_ASSERT( isFiniteValue( throttleValue ) );
            WP_ASSERT( isFiniteValue( brakeValue ) );
            WP_ASSERT( isFiniteValue( steeringChannel ) );
            WP_ASSERT( isFiniteValue( steeringScale ) );
            WP_ASSERT( isFiniteValue( steeringValue ) );
            WP_ASSERT( isValidInput( throttleValue ) );
            WP_ASSERT( isValidPositiveInput( throttleValue ) );
            WP_ASSERT( isValidPositiveInput( brakeValue ) );
            WP_ASSERT( isValidInput( steeringChannel ) );
            WP_ASSERT( steeringScale >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( steeringScale <= MaxSteeringScale );
            WP_ASSERT( Math<physics_Num>::Abs( steeringValue ) <=
                       steeringScale + Math<physics_Num>::epsilon() );
        }

        void assertDriveTrainOutput( const CDriveTrain &driveTrain )
        {
            const auto gearRatios = driveTrain.getGearRatios();
            const auto gear = driveTrain.getGear();
            const auto throttle = static_cast<physics_Num>( driveTrain.getThrottle() );
            const auto throttleInput = driveTrain.getThrottleInput();
            const auto minRPM = driveTrain.getMinRPM();
            const auto maxRPM = driveTrain.getMaxRPM();
            const auto rpm = driveTrain.getRPM();
            const auto engineAngularVelocity = driveTrain.getEngineAngularVelocity();
            const auto slipRatio = driveTrain.getSlipRatio();

            WP_ASSERT( gearRatios.size() > 0 );
            if( gearRatios.size() > 0 )
            {
                WP_ASSERT( gear >= 0 );
                WP_ASSERT( gear < static_cast<s32>( gearRatios.size() ) );
            }

            WP_ASSERT( isValidPositiveInput( throttle ) );
            WP_ASSERT( isValidInput( throttleInput ) );
            WP_ASSERT( isFiniteValue( minRPM ) );
            WP_ASSERT( minRPM >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( maxRPM ) );
            WP_ASSERT( maxRPM >= minRPM );
            WP_ASSERT( isFiniteValue( rpm ) );
            WP_ASSERT( rpm >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( engineAngularVelocity ) );
            WP_ASSERT( isFiniteValue( slipRatio ) );
        }

        void assertLocalForceInput( const Vector3<physics_Num> &force,
                                    const Vector3<physics_Num> &position )
        {
            WP_ASSERT( isFiniteVector( force ) );
            WP_ASSERT( isFiniteVector( position ) );
            WP_ASSERT( force.length() < MaxDebugForce );
            WP_ASSERT( position.length() < MaxDebugForce );
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, CCarController, CVehicleController<IVehicle> );

    CCarController::CCarController() = default;

    CCarController::~CCarController()
    {
        unload( nullptr );
    }

    void CCarController::reset()
    {
        CVehicleController<Vehicle>::reset();
        clearForces();
        for( auto &channel : m_channels )
        {
            channel = 0.0f;
        }

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

    void CCarController::update()
    {
        auto state = getState();
        switch( state )
        {
        case State::EDIT:
        {
            updateTransform();

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto timer = applicationManager->getTimerPtr();
            WP_ASSERT( timer );

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();
            if( dt > 1.0 / 30.0 )
            {
                dt = 1.0 / 30.0;
            }

            assertTimerValues( static_cast<real_Num>( t ), static_cast<real_Num>( dt ) );
            assertControllerRuntimeState( *this );

            const auto throttleValue = static_cast<physics_Num>( getChannel( 0 ) );
            const auto brakeValue = static_cast<physics_Num>( getChannel( 1 ) );
            const auto steeringChannel = static_cast<physics_Num>( getChannel( 2 ) );
            const auto steeringValue = steeringChannel * getEditSteeringScale();
            assertControlInputs( throttleValue, brakeValue, steeringChannel, getEditSteeringScale(),
                                 steeringValue );

            for( auto &w : m_wheels )
            {
                if( w )
                {
                    if( w->isSteeringWheel() )
                    {
                        w->setSteeringAngle( steeringValue );
                        WP_ASSERT( Math<physics_Num>::Abs( w->getSteeringAngle() - steeringValue ) <=
                                   Math<physics_Num>::epsilon() );
                    }
                }
            }

            u32 wheelIndex = 0;
            for( auto &w : m_wheels )
            {
                assertWheelConfig( w, wheelIndex );
                if( w )
                {
                    w->update();
                    assertWheelOutput( w, wheelIndex );
                }
                ++wheelIndex;
            }
            // Publish this tick's contact forces, not the previous tick's accumulated forces.
            CVehicleController<Vehicle>::update();
        }
        break;
        case State::PLAY:
        {
            updateTransform();

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto timer = applicationManager->getTimerPtr();
            WP_ASSERT( timer );

            auto inputManager = applicationManager->getInputDeviceManager();
            WP_ASSERT( inputManager );

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();
            if( dt > 1.0 / 30.0 )
            {
                dt = 1.0 / 30.0;
            }

            assertTimerValues( static_cast<real_Num>( t ), static_cast<real_Num>( dt ) );
            assertControllerRuntimeState( *this );

            auto throttleValue = static_cast<physics_Num>( getChannel( 0 ) );
            auto brakeValue = static_cast<physics_Num>( getChannel( 1 ) );
            auto steeringChannel = static_cast<physics_Num>( getChannel( 2 ) );
            auto steeringValue = steeringChannel * getPlaySteeringScale();

#if !WP_FINAL
            assertControlInputs( throttleValue, brakeValue, steeringChannel, getPlaySteeringScale(),
                                 steeringValue );
#endif

            // clamp values
            throttleValue = Math<physics_Num>::clamp01( throttleValue );
            brakeValue = Math<physics_Num>::clamp01( brakeValue );
            steeringValue = Math<physics_Num>::clamp( steeringChannel, -1.0, 1.0 ) *
                            getPlaySteeringScale();

            if( m_keyboardInput && inputManager->isKeyPressed( KeyCodes::KEY_UP ) )
            {
                throttleValue = 1.0;
            }
            else if( m_keyboardInput && inputManager->isKeyPressed( KeyCodes::KEY_DOWN ) )
            {
                brakeValue = 1.0;
            }

            if( m_keyboardInput && inputManager->isKeyPressed( KeyCodes::KEY_RIGHT ) )
            {
                steeringChannel = 1.0;
                steeringValue = steeringChannel * getPlaySteeringScale();
            }
            else if( m_keyboardInput && inputManager->isKeyPressed( KeyCodes::KEY_LEFT ) )
            {
                steeringChannel = -1.0;
                steeringValue = steeringChannel * getPlaySteeringScale();
            }

            for( auto &w : m_wheels )
            {
                if( w )
                {
                    if( w->isSteeringWheel() )
                    {
                        const auto filtered = handling::steering(steeringValue, w->getSteeringAngle(),
                            getBody()->getVelocity().length(), m_steeringWheelbase,
                            m_steeringAcceleration, m_steeringRate, dt);
                        w->setSteeringAngle(filtered);
                    }
                }
            }

            auto driveTrain = workphone::static_pointer_cast<CDriveTrain>( getDriveTrain() );
            WP_ASSERT( driveTrain );
            if( driveTrain )
            {
                driveTrain->setThrottleInput( throttleValue );
                driveTrain->setBrakeInput( brakeValue );
                driveTrain->update();

#if !WP_FINAL
                assertDriveTrainOutput( *driveTrain );
#endif
            }
            else
            {
                WP_LOG_ERROR( "CCarController::update missing drive train." );
            }

            u32 wheelIndex = 0;
            for( auto &w : m_wheels )
            {
                assertWheelConfig( w, wheelIndex );
                if( w )
                {
                    w->update();
                    assertWheelOutput( w, wheelIndex );
                }
                ++wheelIndex;
            }
            // Publish this tick's contact forces, not the previous tick's accumulated forces.
            CVehicleController<Vehicle>::update();
        }
        break;
        default:
        {
        }
        }
    }

    void CCarController::postUpdate()
    {
    }

    void CCarController::loadDefaults()
    {
        WP_ASSERT( isFiniteValue( getDefaultMass() ) );
        WP_ASSERT( getDefaultMass() > static_cast<physics_Num>( 0.0 ) );
        setMass( getDefaultMass() );
    }

    void CCarController::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto pThis = getSharedFromThis<CCarController>();

            auto body = workphone::make_ptr<CVehicleBody>();
            WP_ASSERT( body );
            body->setParentVehicle( pThis );
            setBody( body );

            auto worldTransform = Transform3<physics_Num>();
            WP_ASSERT( worldTransform.isFinite() );
            setWorldTransform( worldTransform );

            auto localTransform = Transform3<physics_Num>();
            WP_ASSERT( localTransform.isFinite() );
            setLocalTransform( localTransform );

            auto bodyTransform = Transform3<physics_Num>();
            WP_ASSERT( bodyTransform.isFinite() );
            m_bodyTransform = bodyTransform;

            auto driveTrain = workphone::make_ptr<CDriveTrain>();
            driveTrain->load( nullptr );
            setDriveTrain( driveTrain );

            u32 wheelIndex = 0;
            for( auto &w : m_wheels )
            {
                // w = factoryManager->make_ptr<WheelControllerArcade>();
                w = factoryManager->make_ptr<WheelControllerBrush>();
                // w = factoryManager->make_ptr<WheelControllerPacejka>();

                WP_ASSERT( w );
                w->load( nullptr );
                w->setOwner( pThis );
                assertWheelOutput( w, wheelIndex );
                ++wheelIndex;
            }

            if( auto driveTrain = getDriveTrain() )
            {
                Array<SmartPtr<IWheelComponent>> wheels;
                for( auto &w : m_wheels )
                {
                    WP_ASSERT( w );
                    if( w )
                    {
                        wheels.push_back( w );
                    }
                }

                driveTrain->setWheels( wheels );
            }

            WP_ASSERT( m_wheels[0] );
            WP_ASSERT( m_wheels[1] );
            m_wheels[0]->setSteeringWheel( true );
            m_wheels[1]->setSteeringWheel( true );
            WP_ASSERT( m_wheels[0]->isSteeringWheel() );
            WP_ASSERT( m_wheels[1]->isSteeringWheel() );

            m_wheels[0]->setPoweredWheel( true );
            m_wheels[1]->setPoweredWheel( true );

            m_wheels[2]->setPoweredWheel( true );
            m_wheels[3]->setPoweredWheel( true );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CCarController::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                for( auto &w : m_wheels )
                {
                    if( w )
                    {
                        w->setOwner( nullptr );
                        w->unload( nullptr );
                        w = nullptr;
                    }
                }

                if( m_driveTrain )
                {
                    m_driveTrain->unload( nullptr );
                    m_driveTrain = nullptr;
                }

                if( auto body = getBody() )
                {
                    // body->setParent( nullptr );
                    body->unload( nullptr );
                    setBody( nullptr );
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<IWheelComponent> CCarController::getWheelController( u32 index ) const
    {
        WP_ASSERT( index < m_wheels.size() );
        if( index >= m_wheels.size() )
        {
            WP_LOG_ERROR( "CCarController::getWheelController index out of range." );
            return nullptr;
        }

        return m_wheels[index];
    }

    void CCarController::setState( State state )
    {
        CVehicleController<Vehicle>::setState( state );

        switch( state )
        {
        case State::EDIT:
        {
            for( auto &w : m_wheels )
            {
                if( w )
                {
                    w->setState( IVehicleComponent::State::EDIT );
                }
            }
        }
        break;
        case State::PLAY:
        {
            for( auto &w : m_wheels )
            {
                if( w )
                {
                    w->setState( IVehicleComponent::State::PLAY );
                }
            }
        }
        break;
        default:
        {
        }
        break;
        }
    }

    IVehicle::State CCarController::getState() const
    {
        return CVehicleController<Vehicle>::getState();
    }

    SmartPtr<IDriveTrain> CCarController::getDriveTrain() const
    {
        return m_driveTrain;
    }

    void CCarController::setDriveTrain( SmartPtr<IDriveTrain> driveTrain )
    {
        m_driveTrain = driveTrain;
    }

    VehicleDriveType CCarController::getDriveType() const
    {
        return m_driveType;
    }

    void CCarController::setDriveType( VehicleDriveType driveType )
    {
        WP_ASSERT( isValidDriveTypeValue( driveType ) );
        m_driveType = driveType;
    }

    SmartPtr<Properties> CCarController::getProperties() const
    {
        auto properties = CVehicleController<Vehicle>::getProperties();
        properties->setProperty( "Keyboard Input", m_keyboardInput );
        properties->setProperty( "Steering Rate", m_steeringRate );
        properties->setProperty( "Steering Wheelbase", m_steeringWheelbase );
        properties->setProperty( "Steering Acceleration", m_steeringAcceleration );
        WP_ASSERT( properties );

        properties->setProperty( "Edit Steering Scale", getEditSteeringScale() );
        properties->setProperty( "Play Steering Scale", getPlaySteeringScale() );
        properties->setProperty( "Default Mass", getDefaultMass() );
        properties->setProperty( "Drive Type", static_cast<s32>( getDriveType() ) );

        return properties;
    }

    void CCarController::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            WP_LOG_ERROR( "CCarController::setProperties received null properties." );
            return;
        }

        CVehicleController<Vehicle>::setProperties( properties );
        properties->getPropertyValue( "Keyboard Input", m_keyboardInput );
        properties->getPropertyValue( "Steering Rate", m_steeringRate );
        properties->getPropertyValue( "Steering Wheelbase", m_steeringWheelbase );
        properties->getPropertyValue( "Steering Acceleration", m_steeringAcceleration );
        m_steeringRate = std::isfinite(m_steeringRate) ? std::clamp(m_steeringRate, physics_Num(0), physics_Num(1080)) : 0;
        m_steeringWheelbase = std::isfinite(m_steeringWheelbase) ? std::clamp(m_steeringWheelbase, physics_Num(0), physics_Num(20)) : 0;
        m_steeringAcceleration = std::isfinite(m_steeringAcceleration) ? std::clamp(m_steeringAcceleration, physics_Num(0), physics_Num(100)) : 0;

        physics_Num editSteeringScale = getEditSteeringScale();
        physics_Num playSteeringScale = getPlaySteeringScale();
        physics_Num defaultMass = getDefaultMass();
        s32 driveType = static_cast<s32>( getDriveType() );

        properties->getPropertyValue( "Edit Steering Scale", editSteeringScale );
        properties->getPropertyValue( "Play Steering Scale", playSteeringScale );
        properties->getPropertyValue( "Default Mass", defaultMass );
        properties->getPropertyValue( "Drive Type", driveType );

        WP_ASSERT( isFiniteValue( editSteeringScale ) );
        WP_ASSERT( isFiniteValue( playSteeringScale ) );
        WP_ASSERT( isFiniteValue( defaultMass ) );
        WP_ASSERT( driveType >= 0 );
        WP_ASSERT( driveType < static_cast<s32>( VehicleDriveType::Count ) );

        setEditSteeringScale( editSteeringScale );
        setPlaySteeringScale( playSteeringScale );
        setDefaultMass( defaultMass );
        setDriveType( static_cast<VehicleDriveType>( driveType ) );
    }

    physics_Num CCarController::getEditSteeringScale() const
    {
        return m_editSteeringScale;
    }

    void CCarController::setEditSteeringScale( physics_Num editSteeringScale )
    {
        WP_ASSERT( isFiniteValue( editSteeringScale ) );
        WP_ASSERT( editSteeringScale >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( editSteeringScale <= MaxSteeringScale );
        m_editSteeringScale = editSteeringScale >= static_cast<physics_Num>( 0.0 )
                                  ? editSteeringScale
                                  : static_cast<physics_Num>( 0.0 );
    }

    physics_Num CCarController::getPlaySteeringScale() const
    {
        return m_playSteeringScale;
    }

    void CCarController::setPlaySteeringScale( physics_Num playSteeringScale )
    {
        WP_ASSERT( isFiniteValue( playSteeringScale ) );
        WP_ASSERT( playSteeringScale >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( playSteeringScale <= MaxSteeringScale );
        m_playSteeringScale = playSteeringScale >= static_cast<physics_Num>( 0.0 )
                                  ? playSteeringScale
                                  : static_cast<physics_Num>( 0.0 );
    }

    physics_Num CCarController::getDefaultMass() const
    {
        return m_defaultMass;
    }

    void CCarController::setDefaultMass( physics_Num defaultMass )
    {
        WP_ASSERT( isFiniteValue( defaultMass ) );
        WP_ASSERT( defaultMass > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( defaultMass < static_cast<physics_Num>( 1.0e10 ) );
        if( defaultMass <= static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CCarController::setDefaultMass rejected non-positive mass." );
            return;
        }

        m_defaultMass = defaultMass;
    }
}  // namespace workphone
