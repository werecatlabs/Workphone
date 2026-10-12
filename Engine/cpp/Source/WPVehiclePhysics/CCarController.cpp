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

    WP_CLASS_REGISTER_DERIVED( workphone, CCarController, CVehicleController<IVehicle> );

    CCarController::CCarController() = default;

    CCarController::~CCarController() = default;

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

            const auto throttleValue = getChannel( 0 );
            const auto brakeValue = static_cast<physics_Num>( handling::brakeInput( getChannel( 1 ) ) );
            const auto steeringChannel = getChannel( 2 );
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

            auto throttleValue = getChannel( 0 );
            auto brakeValue = static_cast<physics_Num>( handling::brakeInput( getChannel( 1 ) ) );
            auto steeringChannel = getChannel( 2 );
            auto steeringValue = steeringChannel * getPlaySteeringScale();

#if !WP_FINAL
            assertControlInputs( throttleValue, brakeValue, steeringChannel, getPlaySteeringScale(),
                                 steeringValue );
#endif

            // clamp values
            throttleValue = Math<physics_Num>::clamp01( throttleValue );
            brakeValue = Math<physics_Num>::clamp01( brakeValue );
            steeringValue =
                Math<physics_Num>::clamp( steeringChannel, -1.0, 1.0 ) * getPlaySteeringScale();

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
                        const auto filtered = handling::steering(
                            steeringValue, w->getSteeringAngle(), getBody()->getVelocity().length(),
                            m_steeringWheelbase, m_steeringAcceleration, m_steeringRate, dt );
                        w->setSteeringAngle( filtered );
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
                assertDriveTrainOutput( driveTrain.get() );
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
        m_steeringRate = std::isfinite( m_steeringRate )
                             ? std::clamp( m_steeringRate, static_cast<physics_Num>( 0 ),
                                           static_cast<physics_Num>( 1080 ) )
                             : 0;
        m_steeringWheelbase = std::isfinite( m_steeringWheelbase )
                                  ? std::clamp( m_steeringWheelbase, static_cast<physics_Num>( 0 ),
                                                static_cast<physics_Num>( 20 ) )
                                  : 0;
        m_steeringAcceleration = std::isfinite( m_steeringAcceleration )
                                     ? std::clamp( m_steeringAcceleration, static_cast<physics_Num>( 0 ),
                                                   static_cast<physics_Num>( 100 ) )
                                     : 0;

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
