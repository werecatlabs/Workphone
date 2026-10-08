#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <sstream>
#include <WPVehiclePhysics/CDriveTrain.hpp>
#include <WPVehiclePhysics/WheelControllerPacejka.hpp>
#include <WPVehiclePhysics/WheelControllerBrush.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace
    {
        constexpr auto MaxWheelCount = static_cast<size_t>( 32 );
        constexpr auto MaxRPM = static_cast<physics_Num>( 50000.0 );
        constexpr auto MaxTorque = static_cast<physics_Num>( 1.0e8 );
        constexpr auto MaxBrakeTorque = static_cast<physics_Num>( 8000.0 );
        constexpr auto MaxAngularVelocity = static_cast<physics_Num>( 1.0e5 );
        constexpr auto MaxInertia = static_cast<physics_Num>( 1.0e8 );
        constexpr auto WheelStopAngularVelocity = static_cast<physics_Num>( 0.1 );
        constexpr auto ABSFullBrakeSlipRatio = static_cast<physics_Num>( 0.10 );
        constexpr auto ABSReleaseSlipRatio = static_cast<physics_Num>( 0.20 );

        bool isFiniteValue( physics_Num value )
        {
            return Math<physics_Num>::isFinite( value );
        }

        bool isUnitInput( physics_Num value )
        {
            return value >= static_cast<physics_Num>( 0.0 ) && value <= static_cast<physics_Num>( 1.0 );
        }

        bool isSignedUnitInput( physics_Num value )
        {
            return value >= static_cast<physics_Num>( -1.0 ) && value <= static_cast<physics_Num>( 1.0 );
        }

        physics_Num clampUnitInput( physics_Num value )
        {
            WP_ASSERT( isFiniteValue( value ) );
            return Math<physics_Num>::clamp( value, static_cast<physics_Num>( 0.0 ),
                                             static_cast<physics_Num>( 1.0 ) );
        }

        physics_Num getTorqueOpposingWheelRotation( physics_Num angularVelocity,
                                                    physics_Num torqueMagnitude )
        {
            WP_ASSERT( isFiniteValue( angularVelocity ) );
            WP_ASSERT( Math<physics_Num>::Abs( angularVelocity ) < MaxAngularVelocity );
            WP_ASSERT( isFiniteValue( torqueMagnitude ) );
            WP_ASSERT( torqueMagnitude >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( torqueMagnitude < MaxTorque );

            if( torqueMagnitude <= static_cast<physics_Num>( 0.0 ) ||
                Math<physics_Num>::Abs( angularVelocity ) <= WheelStopAngularVelocity )
            {
                return static_cast<physics_Num>( 0.0 );
            }

            return -Math<physics_Num>::Sign( angularVelocity ) * torqueMagnitude;
        }

        physics_Num calcABSBrakeScale( physics_Num brakeInput, physics_Num slipRatio )
        {
            WP_ASSERT( isFiniteValue( brakeInput ) );
            WP_ASSERT( isFiniteValue( slipRatio ) );

            const auto clampedBrakeInput = clampUnitInput( brakeInput );
            if( clampedBrakeInput <= static_cast<physics_Num>( 0.0 ) )
            {
                return static_cast<physics_Num>( 0.0 );
            }

            const auto absSlipRatio = Math<physics_Num>::Abs( slipRatio );
            if( absSlipRatio <= ABSFullBrakeSlipRatio )
            {
                return clampedBrakeInput;
            }

            if( absSlipRatio >= ABSReleaseSlipRatio )
            {
                return static_cast<physics_Num>( 0.0 );
            }

            const auto absBlend =
                ( ABSReleaseSlipRatio - absSlipRatio ) / ( ABSReleaseSlipRatio - ABSFullBrakeSlipRatio );
            WP_ASSERT( isFiniteValue( absBlend ) );

            return clampedBrakeInput * clampUnitInput( absBlend );
        }

        physics_Num calcBrakeTorque( physics_Num wheelAngularVelocity, physics_Num brakeInput,
                                     physics_Num slipRatio )
        {
            const auto brakeScale = calcABSBrakeScale( brakeInput, slipRatio );
            WP_ASSERT( isUnitInput( brakeScale ) );

            const auto brakeTorqueMagnitude = MaxBrakeTorque * brakeScale;
            WP_ASSERT( isFiniteValue( brakeTorqueMagnitude ) );
            WP_ASSERT( brakeTorqueMagnitude >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( brakeTorqueMagnitude <= MaxBrakeTorque );

            return getTorqueOpposingWheelRotation( wheelAngularVelocity, brakeTorqueMagnitude );
        }

        bool isPoweredWheel( const SmartPtr<IWheelComponent> &wheel )
        {
            return wheel && wheel->isPoweredWheel();
        }

        physics_Num getWheelAngularVelocity( const SmartPtr<IWheelComponent> &wheel )
        {
            WP_ASSERT( wheel );
            if( !wheel )
            {
                return static_cast<physics_Num>( 0.0 );
            }

            auto angularVelocity = static_cast<physics_Num>( wheel->getAngularVelocity() );
            WP_ASSERT( isFiniteValue( angularVelocity ) );
            WP_ASSERT( Math<physics_Num>::Abs( angularVelocity ) < MaxAngularVelocity );
            return angularVelocity;
        }

        void assertWheelList( const Array<SmartPtr<IWheelComponent>> &wheels )
        {
            WP_ASSERT( wheels.size() > 0 );
            WP_ASSERT( wheels.size() <= MaxWheelCount );

            for( auto &wheel : wheels )
            {
                WP_ASSERT( wheel );
                if( wheel )
                {
                    const auto angularVelocity = static_cast<physics_Num>( wheel->getAngularVelocity() );
                    WP_ASSERT( isFiniteValue( angularVelocity ) );
                    WP_ASSERT( Math<physics_Num>::Abs( angularVelocity ) < MaxAngularVelocity );
                }
            }
        }

        size_t countPoweredWheels( const Array<SmartPtr<IWheelComponent>> &wheels )
        {
            size_t poweredWheelCount = 0;
            for( auto &wheel : wheels )
            {
                if( isPoweredWheel( wheel ) )
                {
                    ++poweredWheelCount;
                }
            }

            return poweredWheelCount;
        }

        void assertFiniteGearRatios( const Array<physics_Num> &gearRatios )
        {
            WP_ASSERT( gearRatios.size() > 0 );
            for( auto ratio : gearRatios )
            {
                WP_ASSERT( isFiniteValue( ratio ) );
                WP_ASSERT( Math<physics_Num>::Abs( ratio ) < static_cast<physics_Num>( 100.0 ) );
            }
        }

        void assertDriveTrainConfig( const CDriveTrain &driveTrain )
        {
            WP_ASSERT( isFiniteValue( driveTrain.getThrottleInput() ) );
            WP_ASSERT( isSignedUnitInput( driveTrain.getThrottleInput() ) );
            WP_ASSERT( isFiniteValue( static_cast<physics_Num>( driveTrain.getThrottle() ) ) );
            WP_ASSERT( isUnitInput( static_cast<physics_Num>( driveTrain.getThrottle() ) ) );
            WP_ASSERT( isFiniteValue( static_cast<physics_Num>( driveTrain.getBrake() ) ) );
            WP_ASSERT( isUnitInput( static_cast<physics_Num>( driveTrain.getBrake() ) ) );
            WP_ASSERT( isFiniteValue( driveTrain.getBrakeInput() ) );
            WP_ASSERT( isUnitInput( driveTrain.getBrakeInput() ) );
            WP_ASSERT( isFiniteValue( driveTrain.getFinalDriveRatio() ) );
            WP_ASSERT( driveTrain.getFinalDriveRatio() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getStarterGearRatio() ) );
            WP_ASSERT( driveTrain.getStarterGearRatio() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getMinRPM() ) );
            WP_ASSERT( driveTrain.getMinRPM() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getMaxRPM() ) );
            WP_ASSERT( driveTrain.getMaxRPM() >= driveTrain.getMinRPM() );
            WP_ASSERT( driveTrain.getMaxRPM() < MaxRPM );
            WP_ASSERT( isFiniteValue( driveTrain.getRPM() ) );
            WP_ASSERT( driveTrain.getRPM() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( driveTrain.getRPM() <= MaxRPM );

            WP_ASSERT( isFiniteValue( driveTrain.getEngineAngularVelocity() ) );
            WP_ASSERT( Math<physics_Num>::Abs( driveTrain.getEngineAngularVelocity() ) <
                       MaxAngularVelocity );
            WP_ASSERT( isFiniteValue( driveTrain.getEngineInertia() ) );
            WP_ASSERT( driveTrain.getEngineInertia() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( driveTrain.getEngineInertia() < MaxInertia );
            WP_ASSERT( isFiniteValue( driveTrain.getEngineBaseFriction() ) );
            WP_ASSERT( driveTrain.getEngineBaseFriction() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getEngineRPMFriction() ) );
            WP_ASSERT( driveTrain.getEngineRPMFriction() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getDifferentialLockCoefficient() ) );
            WP_ASSERT( driveTrain.getDifferentialLockCoefficient() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( driveTrain.getDifferentialLockCoefficient() <= static_cast<physics_Num>( 1.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getMaxTorque() ) );
            WP_ASSERT( driveTrain.getMaxTorque() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( driveTrain.getMaxTorque() < MaxTorque );
            WP_ASSERT( isFiniteValue( driveTrain.getMaxPower() ) );
            WP_ASSERT( driveTrain.getMaxPower() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getTorqueRPM() ) );
            WP_ASSERT( driveTrain.getTorqueRPM() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getPowerRPM() ) );
            WP_ASSERT( driveTrain.getPowerRPM() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getClutchThrottleRPMBoost() ) );
            WP_ASSERT( driveTrain.getClutchThrottleRPMBoost() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( driveTrain.getOverRevTorqueFalloff() ) );
            WP_ASSERT( driveTrain.getOverRevTorqueFalloff() >= static_cast<physics_Num>( 0.0 ) );

            const auto gearRatios = driveTrain.getGearRatios();
            assertFiniteGearRatios( gearRatios );
            if( gearRatios.size() > 0 )
            {
                WP_ASSERT( driveTrain.getGear() >= 0 );
                WP_ASSERT( driveTrain.getGear() < static_cast<s32>( gearRatios.size() ) );
            }
        }

        void assertPoweredWheelDriveValues( const WheelControllerPacejka &wheel,
                                            physics_Num                   drivetrainInertia,
                                            physics_Num driveFrictionTorque, physics_Num driveTorque )
        {
            WP_ASSERT( wheel.isPoweredWheel() );
            WP_ASSERT( isFiniteValue( drivetrainInertia ) );
            WP_ASSERT( drivetrainInertia >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( drivetrainInertia < MaxInertia );
            WP_ASSERT( isFiniteValue( driveFrictionTorque ) );
            WP_ASSERT( Math<physics_Num>::Abs( driveFrictionTorque ) < MaxTorque );
            WP_ASSERT( isFiniteValue( driveTorque ) );
            WP_ASSERT( Math<physics_Num>::Abs( driveTorque ) < MaxTorque );
        }
    } // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, CDriveTrain, CVehicleComponent<IDriveTrain> );

    CDriveTrain::CDriveTrain()
    {
        // The gear ratios, including neutral (0) and reverse (negative) gears
        m_gearRatios = Array<physics_Num>( { 0.0, -2.5, 3.5, 2.0, 1.0, 0.75, 0.5 } );

        // The final drive ratio, which is multiplied to each gear ratio
        m_finalDriveRatio = 3.23;

        // The engine's torque curve characteristics. Since actual curves are often hard to come by,
        // we approximate the torque curve from these values instead.

        // powerband RPM range
        m_minRPM = 800;
        m_maxRPM = 6400;

        // engine's maximal torque (in Nm) and RPM.
        m_maxTorque = 664;
        m_torqueRPM = 4000;

        // engine's maximal power (in Watts) and RPM.
        m_maxPower = 317000;
        m_powerRPM = 5000;

        m_engineInertia = 0.3;

        m_engineBaseFriction = 25.0;
        m_engineRPMFriction = 0.02;

        m_engineOrientation = Vector3<physics_Num>::forward();

        m_differentialLockCoefficient = 0;

        m_throttle = 0;
        m_throttleInput = 0;
        m_brakeInput = 0;
        m_automatic = true;
        m_gear = 2;
        m_rpm = 0;
        m_slipRatio = 0.0;
        m_engineAngularVelo = 0.0;
    }

    CDriveTrain::~CDriveTrain() = default;

    void CDriveTrain::update()
    {
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

        WP_ASSERT( Math<real_Num>::isFinite( static_cast<real_Num>( t ) ) );
        WP_ASSERT( Math<real_Num>::isFinite( static_cast<real_Num>( dt ) ) );
        WP_ASSERT( dt >= static_cast<real_Num>( 0.0 ) );
        WP_ASSERT( dt < static_cast<real_Num>( 1.0 ) );
        assertDriveTrainConfig( *this );

        auto wheels = getWheels();
        assertWheelList( wheels );

        if( wheels.size() == 0 )
        {
            WP_LOG_ERROR( "CDriveTrain::update has no wheels assigned." );
            return;
        }

        if( m_gearRatios.size() == 0 )
        {
            WP_LOG_ERROR( "CDriveTrain::update has no gear ratios configured." );
            return;
        }

        assertFiniteGearRatios( m_gearRatios );
        WP_ASSERT( m_engineInertia > static_cast<physics_Num>( 0.0 ) );
        if( m_engineInertia <= static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CDriveTrain::update rejected non-positive engine inertia." );
            return;
        }

        auto gearIndex = m_gear;
        if( gearIndex < 0 )
        {
            gearIndex = 0;
        }
        else if( gearIndex >= static_cast<s32>( m_gearRatios.size() ) )
        {
            gearIndex = static_cast<s32>( m_gearRatios.size() - 1 );
        }
        if( gearIndex != m_gear )
        {
            WP_LOG_ERROR( "CDriveTrain::update clamped out-of-range gear." );
            m_gear = gearIndex;
        }

        // process throttle from input. Apply traction control if necessary. This is a simple
        // implementation and can be improved.
        m_throttle = clampUnitInput( m_throttleInput );

        WP_ASSERT( gearIndex >= 0 );
        WP_ASSERT( gearIndex < static_cast<s32>( m_gearRatios.size() ) );
        const auto currentGearRatio = m_gearRatios[gearIndex];
        WP_ASSERT( isFiniteValue( currentGearRatio ) );

        auto ratio = currentGearRatio * m_finalDriveRatio;
        WP_ASSERT( isFiniteValue( ratio ) );
        WP_ASSERT( Math<physics_Num>::Abs( ratio ) < static_cast<physics_Num>( 1000.0 ) );
        auto inertia = m_engineInertia * Math<physics_Num>::Sqr( ratio );
        WP_ASSERT( isFiniteValue( inertia ) );
        WP_ASSERT( inertia >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( inertia < MaxInertia );
        auto engineFrictionTorque = m_engineBaseFriction + m_rpm * m_engineRPMFriction;
        WP_ASSERT( isFiniteValue( engineFrictionTorque ) );
        WP_ASSERT( engineFrictionTorque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( engineFrictionTorque < MaxTorque );
        auto engineTorque = ( calcEngineTorque() + Math<physics_Num>::Abs( engineFrictionTorque ) ) *
                            m_throttle * static_cast<physics_Num>( 1.0 );
        WP_ASSERT( isFiniteValue( engineTorque ) );
        WP_ASSERT( engineTorque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( engineTorque < MaxTorque );

        const auto brakeInput = clampUnitInput( m_brakeInput );
        WP_ASSERT( isUnitInput( brakeInput ) );
        m_brake = brakeInput;

        m_slipRatio = 0.0;

        if( ratio == 0 )
        {
            // Neutral gear - just rev up engine
            auto engineAngularAcceleration = ( engineTorque - engineFrictionTorque ) / m_engineInertia;
            WP_ASSERT( isFiniteValue( engineAngularAcceleration ) );
            m_engineAngularVelo += engineAngularAcceleration * static_cast<physics_Num>( dt );
            WP_ASSERT( isFiniteValue( m_engineAngularVelo ) );
            WP_ASSERT( Math<physics_Num>::Abs( m_engineAngularVelo ) < MaxAngularVelocity );

            // Apply torque to car body
            // GetComponent<Rigidbody>().AddTorque(-engineOrientation * engineTorque);

            // Neutral still needs service brakes. No engine braking is coupled to the wheels in neutral.
            for( auto &wheel : wheels )
            {
                WP_ASSERT( wheel );
                if( !wheel )
                {
                    continue;
                }

                const auto wheelAngularVelocity = getWheelAngularVelocity( wheel );

                if( wheel->isDerived<WheelControllerPacejka>() )
                {
                    auto w = workphone::static_pointer_cast<WheelControllerPacejka>( wheel );
                    WP_ASSERT( w );
                    if( w )
                    {
                        const auto wheelBrakeTorque =
                            calcBrakeTorque( wheelAngularVelocity, m_brake, w->getSlipRatio() );
                        WP_ASSERT( isFiniteValue( wheelBrakeTorque ) );
                        WP_ASSERT( Math<physics_Num>::Abs( wheelBrakeTorque ) < MaxTorque );

                        w->setDrivetrainInertia( static_cast<physics_Num>( 0.0 ) );
                        w->setDriveFrictionTorque( static_cast<physics_Num>( 0.0 ) );
                        w->setDriveTorque( wheelBrakeTorque );
                    }
                }
                else
                {
                    const auto brush = wheel->isDerived<WheelControllerBrush>();
                    if(brush)
                        wheel->setBrake(m_brake);
                    const auto wheelBrakeTorque = brush ? physics_Num(0) :
                        calcBrakeTorque(wheelAngularVelocity, m_brake, physics_Num(0));
                    WP_ASSERT( isFiniteValue( wheelBrakeTorque ) );
                    WP_ASSERT( Math<physics_Num>::Abs( wheelBrakeTorque ) < MaxTorque );
                    wheel->setTorque( wheelBrakeTorque );
                }
            }
        }
        else
        {
            const auto poweredWheelCount = countPoweredWheels( wheels );
            WP_ASSERT( poweredWheelCount > 0 );
            if( poweredWheelCount == 0 )
            {
                WP_LOG_ERROR( "CDriveTrain::update has a non-neutral gear but no powered wheels." );
                return;
            }

            auto drivetrainFraction =
                static_cast<physics_Num>( 1.0 ) / static_cast<physics_Num>( poweredWheelCount );
            WP_ASSERT( isFiniteValue( drivetrainFraction ) );
            WP_ASSERT( drivetrainFraction > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( drivetrainFraction <= static_cast<physics_Num>( 1.0 ) );
            auto averageAngularVelo = static_cast<physics_Num>( 0.0 );

            for( auto &wheel : wheels )
            {
                WP_ASSERT( wheel );
                if( !wheel )
                {
                    WP_LOG_ERROR( "CDriveTrain::update skipped null wheel." );
                    continue;
                }

                if( wheel->isDerived<WheelControllerPacejka>() )
                {
                    auto w = workphone::static_pointer_cast<WheelControllerPacejka>( wheel );
                    WP_ASSERT( w );
                    if( w && w->isPoweredWheel() )
                    {
                        averageAngularVelo += getWheelAngularVelocity( wheel ) * drivetrainFraction;
                    }
                }
                else if( wheel->isPoweredWheel() )
                {
                    averageAngularVelo += getWheelAngularVelocity( wheel ) * drivetrainFraction;
                }
            }
            WP_ASSERT( isFiniteValue( averageAngularVelo ) );
            WP_ASSERT( Math<physics_Num>::Abs( averageAngularVelo ) < MaxAngularVelocity );

            // Apply torque to wheels
            for( auto &wheel : wheels )
            {
                WP_ASSERT( wheel );
                if( !wheel )
                {
                    continue;
                }

                if( wheel->isDerived<WheelControllerPacejka>() )
                {
                    auto w = workphone::static_pointer_cast<WheelControllerPacejka>( wheel );
                    WP_ASSERT( w );
                    if( w->isPoweredWheel() )
                    {
                        const auto wheelAngularVelocity =
                            static_cast<physics_Num>( w->getAngularVelocity() );
                        WP_ASSERT( isFiniteValue( wheelAngularVelocity ) );
                        WP_ASSERT( Math<physics_Num>::Abs( wheelAngularVelocity ) < MaxAngularVelocity );

                        auto lockingTorque = ( averageAngularVelo - wheelAngularVelocity ) *
                                             m_differentialLockCoefficient;
                        WP_ASSERT( isFiniteValue( lockingTorque ) );
                        WP_ASSERT( Math<physics_Num>::Abs( lockingTorque ) < MaxTorque );

                        const auto wheelInertia = inertia * drivetrainFraction;
                        const auto wheelFrictionTorque =
                            engineFrictionTorque * Math<physics_Num>::Abs( ratio ) * drivetrainFraction;
                        const auto wheelBrakeTorque =
                            calcBrakeTorque( wheelAngularVelocity, brakeInput, w->getSlipRatio() );
                        WP_ASSERT( isFiniteValue( wheelBrakeTorque ) );
                        WP_ASSERT( Math<physics_Num>::Abs( wheelBrakeTorque ) < MaxTorque );

                        const auto wheelDriveTorque =
                            engineTorque * ratio * drivetrainFraction + lockingTorque + wheelBrakeTorque;
                        assertPoweredWheelDriveValues( *w, wheelInertia, wheelFrictionTorque,
                                                       wheelDriveTorque );

                        w->setDrivetrainInertia( wheelInertia );
                        w->setDriveFrictionTorque( wheelFrictionTorque );
                        w->setDriveTorque( wheelDriveTorque );
                        WP_ASSERT(
                            Math<physics_Num>::equals( w->getDrivetrainInertia(), wheelInertia ) );
                        WP_ASSERT( Math<physics_Num>::equals( w->getDriveFrictionTorque(),
                                                              wheelFrictionTorque ) );
                        WP_ASSERT( Math<physics_Num>::equals( w->getDriveTorque(), wheelDriveTorque ) );

                        m_slipRatio += w->getSlipRatio() * drivetrainFraction;
                        WP_ASSERT( isFiniteValue( m_slipRatio ) );
                    }
                    else
                    {
                        const auto wheelAngularVelocity = getWheelAngularVelocity( wheel );
                        const auto wheelBrakeTorque =
                            calcBrakeTorque( wheelAngularVelocity, brakeInput, w->getSlipRatio() );
                        WP_ASSERT( isFiniteValue( wheelBrakeTorque ) );
                        WP_ASSERT( Math<physics_Num>::Abs( wheelBrakeTorque ) < MaxTorque );

                        w->setDrivetrainInertia( static_cast<physics_Num>( 0.0 ) );
                        w->setDriveFrictionTorque( static_cast<physics_Num>( 0.0 ) );
                        w->setDriveTorque( wheelBrakeTorque );
                    }
                }
                else
                {
                    const auto wheelAngularVelocity = getWheelAngularVelocity( wheel );
                    const auto brush = wheel->isDerived<WheelControllerBrush>();
                    if(brush)
                        wheel->setBrake(brakeInput);
                    const auto wheelBrakeTorque = brush ? physics_Num(0) :
                        calcBrakeTorque(wheelAngularVelocity, brakeInput, physics_Num(0));
                    WP_ASSERT( isFiniteValue( wheelBrakeTorque ) );
                    WP_ASSERT( Math<physics_Num>::Abs( wheelBrakeTorque ) < MaxTorque );

                    if( wheel->isPoweredWheel() )
                    {
                        auto lockingTorque = ( averageAngularVelo - wheelAngularVelocity ) *
                                             m_differentialLockCoefficient;
                        WP_ASSERT( isFiniteValue( lockingTorque ) );
                        WP_ASSERT( Math<physics_Num>::Abs( lockingTorque ) < MaxTorque );

                        const auto wheelFrictionTorque =
                            engineFrictionTorque * Math<physics_Num>::Abs( ratio ) * drivetrainFraction;
                        const auto wheelEngineBrakeTorque =
                            getTorqueOpposingWheelRotation( wheelAngularVelocity, wheelFrictionTorque );
                        WP_ASSERT( isFiniteValue( wheelEngineBrakeTorque ) );
                        WP_ASSERT( Math<physics_Num>::Abs( wheelEngineBrakeTorque ) < MaxTorque );

                        const auto wheelDriveTorque = engineTorque * ratio * drivetrainFraction +
                                                      lockingTorque + wheelEngineBrakeTorque +
                                                      wheelBrakeTorque;
                        WP_ASSERT( isFiniteValue( wheelDriveTorque ) );
                        WP_ASSERT( Math<physics_Num>::Abs( wheelDriveTorque ) < MaxTorque );
                        wheel->setTorque( wheelDriveTorque );
                    }
                    else
                    {
                        wheel->setTorque( wheelBrakeTorque );
                    }
                }
            }

            // update engine angular velo
            m_engineAngularVelo = averageAngularVelo * ratio;
            WP_ASSERT( isFiniteValue( m_engineAngularVelo ) );
            WP_ASSERT( Math<physics_Num>::Abs( m_engineAngularVelo ) < MaxAngularVelocity );
        }

        // update state
        m_slipRatio *= Math<physics_Num>::Sign( ratio );
        WP_ASSERT( isFiniteValue( m_slipRatio ) );
        m_rpm = m_engineAngularVelo * ( static_cast<physics_Num>( 60.0 ) /
                                        ( static_cast<physics_Num>( 2.0 ) * Math<physics_Num>::pi() ) );
        WP_ASSERT( isFiniteValue( m_rpm ) );

        // very simple simulation of clutch - just pretend we are at a higher rpm.
        auto minClutchRPM = m_minRPM;
        WP_ASSERT( isFiniteValue( minClutchRPM ) );

        if( m_gear == 2 )
        {
            minClutchRPM += m_throttle * getClutchThrottleRPMBoost();
            WP_ASSERT( isFiniteValue( minClutchRPM ) );
        }

        if( m_rpm < minClutchRPM )
        {
            m_rpm = minClutchRPM;
        }

        WP_ASSERT( m_rpm >= static_cast<physics_Num>( 0.0 ) );

        if( m_rpm >= MaxRPM )
        {
            m_rpm = MaxRPM;
        }

        // Automatic gear shifting. Bases shift points on throttle input and rpm.
        if( m_automatic )
        {
            const auto upShiftRPM =
                m_maxRPM * ( getUpShiftBaseRPMScale() + getUpShiftThrottleRPMScale() * m_throttleInput );
            const auto downShiftRPM = m_maxRPM * ( getDownShiftBaseRPMScale() +
                                                   getDownShiftThrottleRPMScale() * m_throttleInput );
            WP_ASSERT( isFiniteValue( upShiftRPM ) );
            WP_ASSERT( isFiniteValue( downShiftRPM ) );

            if( m_rpm >= upShiftRPM )
            {
                shiftUp();
            }
            else if( m_rpm <= downShiftRPM && m_gear > 2 )
            {
                shiftDown();
            }
            if( m_throttleInput < 0 && m_rpm <= m_minRPM )
            {
                m_gear = ( m_gear == 0 ? 2 : 0 );
            }
        }

        assertDriveTrainConfig( *this );
    }

    physics_Num CDriveTrain::calcEngineTorque()
    {
        WP_ASSERT( isFiniteValue( m_rpm ) );
        WP_ASSERT( isFiniteValue( m_torqueRPM ) );
        WP_ASSERT( m_torqueRPM > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_powerRPM ) );
        WP_ASSERT( m_powerRPM > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_maxTorque ) );
        WP_ASSERT( m_maxTorque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_maxPower ) );
        WP_ASSERT( m_maxPower >= static_cast<physics_Num>( 0.0 ) );

        physics_Num result;

        if( m_rpm < m_torqueRPM )
        {
            result = m_maxTorque *
                     ( -Math<physics_Num>::Sqr( m_rpm / m_torqueRPM - static_cast<physics_Num>( 1.0 ) ) +
                       static_cast<physics_Num>( 1.0 ) );
            WP_ASSERT( isFiniteValue( result ) );
        }
        else
        {
            auto maxPowerTorque = m_maxPower / ( m_powerRPM * 2 * Math<physics_Num>::pi() /
                                                 static_cast<physics_Num>( 60.0 ) );
            WP_ASSERT( isFiniteValue( maxPowerTorque ) );
            WP_ASSERT( maxPowerTorque >= static_cast<physics_Num>( 0.0 ) );
            auto aproxFactor = ( m_maxTorque - maxPowerTorque ) /
                               ( 2 * m_torqueRPM * m_powerRPM - Math<physics_Num>::Sqr( m_powerRPM ) -
                                 Math<physics_Num>::Sqr( m_torqueRPM ) );
            WP_ASSERT( isFiniteValue( aproxFactor ) );
            auto torque = aproxFactor * Math<physics_Num>::Sqr( m_rpm - m_torqueRPM ) + m_maxTorque;
            WP_ASSERT( isFiniteValue( torque ) );
            result = torque > static_cast<physics_Num>( 0 ) ? torque : static_cast<physics_Num>( 0 );
            WP_ASSERT( isFiniteValue( result ) );
        }

        if( m_rpm > m_maxRPM )
        {
            result *=
                static_cast<physics_Num>( 1 ) - ( ( m_rpm - m_maxRPM ) * getOverRevTorqueFalloff() );
            if( result < static_cast<physics_Num>( 0 ) )
            {
                result = static_cast<physics_Num>( 0 );
            }
        }
        WP_ASSERT( isFiniteValue( result ) );

        if( m_rpm < 0 )
        {
            result = 0;
        }

        WP_ASSERT( result >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( result < MaxTorque );
        return result;
    }

    void CDriveTrain::shiftUp()
    {
        WP_ASSERT( m_gearRatios.size() > 0 );
        WP_ASSERT( m_gear >= 0 );
        WP_ASSERT( m_gear < static_cast<s32>( m_gearRatios.size() ) );
        if( m_gear < m_gearRatios.size() - 1 )
        {
            m_gear++;
        }
        WP_ASSERT( m_gear >= 0 );
        WP_ASSERT( m_gear < static_cast<s32>( m_gearRatios.size() ) );
    }

    void CDriveTrain::shiftDown()
    {
        WP_ASSERT( m_gearRatios.size() > 0 );
        WP_ASSERT( m_gear >= 0 );
        WP_ASSERT( m_gear < static_cast<s32>( m_gearRatios.size() ) );
        if( m_gear > 0 )
        {
            m_gear--;
        }
        WP_ASSERT( m_gear >= 0 );
        WP_ASSERT( m_gear < static_cast<s32>( m_gearRatios.size() ) );
    }

    Array<SmartPtr<IWheelComponent>> CDriveTrain::getWheels() const
    {
        return m_wheels;
    }

    void CDriveTrain::setWheels( Array<SmartPtr<IWheelComponent>> wheels )
    {
        assertWheelList( wheels );
        m_wheels = wheels;
    }

    SmartPtr<IGearBox> CDriveTrain::getGearBox() const
    {
        return m_gearBox;
    }

    void CDriveTrain::setGearBox( SmartPtr<IGearBox> gearBox )
    {
        m_gearBox = gearBox;
    }

    SmartPtr<IDifferential> CDriveTrain::getDifferential() const
    {
        return m_differential;
    }

    void CDriveTrain::setDifferential( SmartPtr<IDifferential> differential )
    {
        m_differential = differential;
    }

    f32 CDriveTrain::getThrottle() const
    {
        return this->m_throttle;
    }

    void CDriveTrain::setThrottle( f32 throttle )
    {
        WP_ASSERT( Math<f32>::isFinite( throttle ) );
        WP_ASSERT( throttle >= 0.0f && throttle <= 1.0f );
        this->m_throttle = Math<f32>::clamp( throttle, 0.0f, 1.0f );
        WP_ASSERT( Math<f32>::isFinite( this->m_throttle ) );
        WP_ASSERT( this->m_throttle >= 0.0f && this->m_throttle <= 1.0f );
    }

    f32 CDriveTrain::getBrake() const
    {
        return m_brake;
    }

    void CDriveTrain::setBrake( f32 brake )
    {
        WP_ASSERT( Math<f32>::isFinite( brake ) );
        WP_ASSERT( brake >= 0.0f && brake <= 1.0f );
        m_brake = brake;
    }

    // Engine orientation accessors
    Vector3F CDriveTrain::getEngineOrientation() const
    {
        return m_engineOrientation;
    }

    void CDriveTrain::setEngineOrientation( const Vector3F &orientation )
    {
        WP_ASSERT( orientation.isFinite() );
        WP_ASSERT( orientation.lengthSquared() > MathF::epsilon() );
        m_engineOrientation = orientation;
    }

    // Gear ratios accessors
    Array<physics_Num> CDriveTrain::getGearRatios() const
    {
        return m_gearRatios;
    }

    void CDriveTrain::setGearRatios( const Array<physics_Num> &ratios )
    {
        WP_ASSERT( ratios.size() > 0 );
        assertFiniteGearRatios( ratios );
        if( ratios.size() == 0 )
        {
            WP_LOG_ERROR( "CDriveTrain::setGearRatios rejected empty ratios." );
            return;
        }

        m_gearRatios = ratios;
        setGear( m_gear );
    }

    // Final drive ratio accessors
    physics_Num CDriveTrain::getFinalDriveRatio() const
    {
        return m_finalDriveRatio;
    }

    void CDriveTrain::setFinalDriveRatio( physics_Num ratio )
    {
        WP_ASSERT( isFiniteValue( ratio ) );
        WP_ASSERT( ratio > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( ratio < static_cast<physics_Num>( 100.0 ) );
        if( ratio <= static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CDriveTrain::setFinalDriveRatio rejected non-positive ratio." );
            return;
        }

        m_finalDriveRatio = ratio;
    }

    // RPM range accessors
    physics_Num CDriveTrain::getMinRPM() const
    {
        return m_minRPM;
    }

    void CDriveTrain::setMinRPM( physics_Num rpm )
    {
        WP_ASSERT( isFiniteValue( rpm ) );
        WP_ASSERT( rpm >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( rpm < MaxRPM );
        if( rpm < static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CDriveTrain::setMinRPM rejected negative rpm." );
            return;
        }
        m_minRPM = rpm;
    }

    physics_Num CDriveTrain::getMaxRPM() const
    {
        return m_maxRPM;
    }

    void CDriveTrain::setMaxRPM( physics_Num rpm )
    {
        WP_ASSERT( isFiniteValue( rpm ) );
        WP_ASSERT( rpm >= m_minRPM );
        WP_ASSERT( rpm < MaxRPM );
        if( rpm < m_minRPM )
        {
            WP_LOG_ERROR( "CDriveTrain::setMaxRPM rejected rpm below min rpm." );
            return;
        }
        m_maxRPM = rpm;
    }

    // Torque characteristics accessors
    physics_Num CDriveTrain::getMaxTorque() const
    {
        return m_maxTorque;
    }

    void CDriveTrain::setMaxTorque( physics_Num torque )
    {
        WP_ASSERT( isFiniteValue( torque ) );
        WP_ASSERT( torque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( torque < MaxTorque );
        if( torque < static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CDriveTrain::setMaxTorque rejected negative torque." );
            return;
        }
        m_maxTorque = torque;
    }

    physics_Num CDriveTrain::getTorqueRPM() const
    {
        return m_torqueRPM;
    }

    void CDriveTrain::setTorqueRPM( physics_Num rpm )
    {
        WP_ASSERT( isFiniteValue( rpm ) );
        WP_ASSERT( rpm > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( rpm < MaxRPM );
        if( rpm <= static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CDriveTrain::setTorqueRPM rejected non-positive rpm." );
            return;
        }
        m_torqueRPM = rpm;
    }

    // Power characteristics accessors
    physics_Num CDriveTrain::getMaxPower() const
    {
        return m_maxPower;
    }

    void CDriveTrain::setMaxPower( physics_Num power )
    {
        WP_ASSERT( isFiniteValue( power ) );
        WP_ASSERT( power >= static_cast<physics_Num>( 0.0 ) );
        if( power < static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CDriveTrain::setMaxPower rejected negative power." );
            return;
        }
        m_maxPower = power;
    }

    physics_Num CDriveTrain::getPowerRPM() const
    {
        return m_powerRPM;
    }

    void CDriveTrain::setPowerRPM( physics_Num rpm )
    {
        WP_ASSERT( isFiniteValue( rpm ) );
        WP_ASSERT( rpm > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( rpm < MaxRPM );
        if( rpm <= static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CDriveTrain::setPowerRPM rejected non-positive rpm." );
            return;
        }
        m_powerRPM = rpm;
    }

    // Engine inertia accessors
    physics_Num CDriveTrain::getEngineInertia() const
    {
        return m_engineInertia;
    }

    void CDriveTrain::setEngineInertia( physics_Num inertia )
    {
        WP_ASSERT( isFiniteValue( inertia ) );
        WP_ASSERT( inertia > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( inertia < MaxInertia );
        if( inertia <= static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CDriveTrain::setEngineInertia rejected non-positive inertia." );
            return;
        }

        m_engineInertia = inertia;
    }

    // Engine friction accessors
    physics_Num CDriveTrain::getEngineBaseFriction() const
    {
        return m_engineBaseFriction;
    }

    void CDriveTrain::setEngineBaseFriction( physics_Num friction )
    {
        WP_ASSERT( isFiniteValue( friction ) );
        WP_ASSERT( friction >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( friction < MaxTorque );
        m_engineBaseFriction = friction;
    }

    physics_Num CDriveTrain::getEngineRPMFriction() const
    {
        return m_engineRPMFriction;
    }

    void CDriveTrain::setEngineRPMFriction( physics_Num friction )
    {
        WP_ASSERT( isFiniteValue( friction ) );
        WP_ASSERT( friction >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( friction < MaxTorque );
        m_engineRPMFriction = friction;
    }

    // Differential lock coefficient accessors
    physics_Num CDriveTrain::getDifferentialLockCoefficient() const
    {
        return m_differentialLockCoefficient;
    }

    void CDriveTrain::setDifferentialLockCoefficient( physics_Num coefficient )
    {
        WP_ASSERT( isFiniteValue( coefficient ) );
        WP_ASSERT( coefficient >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( coefficient <= static_cast<physics_Num>( 1.0 ) );
        m_differentialLockCoefficient = Math<physics_Num>::clamp(
            coefficient, static_cast<physics_Num>( 0.0 ), static_cast<physics_Num>( 1.0 ) );
        WP_ASSERT( isUnitInput( m_differentialLockCoefficient ) );
    }

    // Throttle input accessors
    physics_Num CDriveTrain::getThrottleInput() const
    {
        return m_throttleInput;
    }

    void CDriveTrain::setThrottleInput( physics_Num throttle )
    {
        WP_ASSERT( isFiniteValue( throttle ) );
        WP_ASSERT( throttle >= static_cast<physics_Num>( -1.0 ) &&
                   throttle <= static_cast<physics_Num>( 1.0 ) );
        m_throttleInput = Math<physics_Num>::clamp( throttle, static_cast<physics_Num>( -1.0 ),
                                                    static_cast<physics_Num>( 1.0 ) );
        WP_ASSERT( isSignedUnitInput( m_throttleInput ) );
    }

    physics_Num CDriveTrain::getBrakeInput() const
    {
        return m_brakeInput;
    }

    void CDriveTrain::setBrakeInput( physics_Num brakeInput )
    {
        WP_ASSERT( isFiniteValue( brakeInput ) );
        WP_ASSERT( isUnitInput( brakeInput ) );
        m_brakeInput = clampUnitInput( brakeInput );
        WP_ASSERT( isUnitInput( m_brakeInput ) );
    }

    // Automatic transmission accessors
    bool CDriveTrain::isAutomatic() const
    {
        return m_automatic;
    }

    void CDriveTrain::setAutomatic( bool automatic )
    {
        m_automatic = automatic;
    }

    // Gear state accessors
    s32 CDriveTrain::getGear() const
    {
        return m_gear;
    }

    void CDriveTrain::setGear( s32 gear )
    {
        WP_ASSERT( m_gearRatios.size() == 0 ||
                   ( gear >= 0 && gear < static_cast<s32>( m_gearRatios.size() ) ) );
        if( m_gearRatios.size() > 0 )
        {
            if( gear < 0 )
            {
                m_gear = 0;
            }
            else if( gear >= static_cast<s32>( m_gearRatios.size() ) )
            {
                m_gear = static_cast<s32>( m_gearRatios.size() - 1 );
            }
            else
            {
                m_gear = gear;
            }
            return;
        }

        m_gear = gear;
        WP_ASSERT( m_gearRatios.size() == 0 ||
                   ( m_gear >= 0 && m_gear < static_cast<s32>( m_gearRatios.size() ) ) );
    }

    // Engine state accessors
    physics_Num CDriveTrain::getRPM() const
    {
        return m_rpm;
    }

    void CDriveTrain::setRPM( physics_Num rpm )
    {
        WP_ASSERT( isFiniteValue( rpm ) );
        WP_ASSERT( rpm >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( rpm < MaxRPM );
        if( rpm < static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "CDriveTrain::setRPM rejected negative rpm." );
            return;
        }

        m_rpm = rpm;

        // Option 1: Fix the clamp to use a value strictly less than MaxRPM
        if( m_rpm >= MaxRPM )
        {
            m_rpm = std::nextafter( MaxRPM, 0.0f );
        }
    }

    physics_Num CDriveTrain::getSlipRatio() const
    {
        return m_slipRatio;
    }

    void CDriveTrain::setSlipRatio( physics_Num ratio )
    {
        WP_ASSERT( isFiniteValue( ratio ) );
        m_slipRatio = ratio;
    }

    physics_Num CDriveTrain::getEngineAngularVelocity() const
    {
        return m_engineAngularVelo;
    }

    void CDriveTrain::setEngineAngularVelocity( physics_Num velocity )
    {
        WP_ASSERT( isFiniteValue( velocity ) );
        WP_ASSERT( Math<physics_Num>::Abs( velocity ) < MaxAngularVelocity );
        m_engineAngularVelo = velocity;
    }

    SmartPtr<Properties> CDriveTrain::getProperties() const
    {
        auto properties = CVehicleComponent<IDriveTrain>::getProperties();
        WP_ASSERT( properties );

        String ratios;
        for(auto ratio:getGearRatios())
        {
            if(!ratios.empty())ratios+=",";
            ratios+=StringUtil::toString(ratio);
        }
        properties->setProperty("Gear Ratios",ratios);
        properties->setProperty( "Final Drive Ratio", getFinalDriveRatio() );
        properties->setProperty( "Min RPM", getMinRPM() );
        properties->setProperty( "Max RPM", getMaxRPM() );
        properties->setProperty( "Max Torque", getMaxTorque() );
        properties->setProperty( "Torque RPM", getTorqueRPM() );
        properties->setProperty( "Max Power", getMaxPower() );
        properties->setProperty( "Power RPM", getPowerRPM() );
        properties->setProperty( "Engine Inertia", getEngineInertia() );
        properties->setProperty( "Engine Base Friction", getEngineBaseFriction() );
        properties->setProperty( "Engine RPM Friction", getEngineRPMFriction() );
        properties->setProperty( "Differential Lock Coefficient", getDifferentialLockCoefficient() );
        properties->setProperty( "Throttle", getThrottle() );
        properties->setProperty( "Throttle Input", getThrottleInput() );
        properties->setProperty( "Brake", getBrake() );
        properties->setProperty( "Brake Input", getBrakeInput() );
        properties->setProperty( "Automatic", isAutomatic() );
        properties->setProperty( "Gear", getGear() );
        properties->setProperty( "RPM", getRPM() );
        properties->setProperty( "Slip Ratio", getSlipRatio(), true );
        properties->setProperty( "Engine Angular Velocity", getEngineAngularVelocity() );
        properties->setProperty( "Starter Gear Ratio", getStarterGearRatio() );
        properties->setProperty( "Clutch Throttle RPM Boost", getClutchThrottleRPMBoost() );
        properties->setProperty( "Up Shift Base RPM Scale", getUpShiftBaseRPMScale() );
        properties->setProperty( "Up Shift Throttle RPM Scale", getUpShiftThrottleRPMScale() );
        properties->setProperty( "Down Shift Base RPM Scale", getDownShiftBaseRPMScale() );
        properties->setProperty( "Down Shift Throttle RPM Scale", getDownShiftThrottleRPMScale() );
        properties->setProperty( "Over Rev Torque Falloff", getOverRevTorqueFalloff() );

        return properties;
    }

    void CDriveTrain::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            WP_LOG_ERROR( "CDriveTrain::setProperties received null properties." );
            return;
        }

        CVehicleComponent<IDriveTrain>::setProperties( properties );

        auto finalDriveRatio = getFinalDriveRatio();
        auto minRPM = getMinRPM();
        auto maxRPM = getMaxRPM();
        auto maxTorque = getMaxTorque();
        auto torqueRPM = getTorqueRPM();
        auto maxPower = getMaxPower();
        auto powerRPM = getPowerRPM();
        auto engineInertia = getEngineInertia();
        auto engineBaseFriction = getEngineBaseFriction();
        auto engineRPMFriction = getEngineRPMFriction();
        auto differentialLockCoefficient = getDifferentialLockCoefficient();
        auto throttle = getThrottle();
        auto throttleInput = getThrottleInput();
        auto brake = getBrake();
        auto brakeInput = getBrakeInput();
        auto automatic = isAutomatic();
        auto gear = getGear();
        auto rpm = getRPM();
        auto engineAngularVelocity = getEngineAngularVelocity();
        auto starterGearRatio = getStarterGearRatio();
        auto clutchThrottleRPMBoost = getClutchThrottleRPMBoost();
        auto upShiftBaseRPMScale = getUpShiftBaseRPMScale();
        auto upShiftThrottleRPMScale = getUpShiftThrottleRPMScale();
        auto downShiftBaseRPMScale = getDownShiftBaseRPMScale();
        auto downShiftThrottleRPMScale = getDownShiftThrottleRPMScale();
        auto overRevTorqueFalloff = getOverRevTorqueFalloff();

        String ratioText;
        if(properties->getPropertyValue("Gear Ratios",ratioText) && !ratioText.empty())
        {
            std::istringstream input(ratioText.c_str());std::string token;
            Array<physics_Num> ratios;bool valid=true;
            while(std::getline(input,token,','))
            {
                std::istringstream number(token);physics_Num value;
                if(!(number>>value) || !isFiniteValue(value) || std::abs(value)>100 || ratios.size()>=24)
                {valid=false;break;}
                number>>std::ws;if(!number.eof()){valid=false;break;}
                ratios.push_back(value);
            }
            if(valid && !ratios.empty())setGearRatios(ratios);
            else WP_LOG_WARNING("CDriveTrain: invalid Gear Ratios; preserving the current transmission.");
        }
        properties->getPropertyValue( "Final Drive Ratio", finalDriveRatio );
        properties->getPropertyValue( "Min RPM", minRPM );
        properties->getPropertyValue( "Max RPM", maxRPM );
        properties->getPropertyValue( "Max Torque", maxTorque );
        properties->getPropertyValue( "Torque RPM", torqueRPM );
        properties->getPropertyValue( "Max Power", maxPower );
        properties->getPropertyValue( "Power RPM", powerRPM );
        properties->getPropertyValue( "Engine Inertia", engineInertia );
        properties->getPropertyValue( "Engine Base Friction", engineBaseFriction );
        properties->getPropertyValue( "Engine RPM Friction", engineRPMFriction );
        properties->getPropertyValue( "Differential Lock Coefficient", differentialLockCoefficient );
        properties->getPropertyValue( "Throttle", throttle );
        properties->getPropertyValue( "Throttle Input", throttleInput );
        properties->getPropertyValue( "Brake", brake );
        brakeInput = static_cast<physics_Num>( brake );
        properties->getPropertyValue( "Brake Input", brakeInput );
        properties->getPropertyValue( "Automatic", automatic );
        properties->getPropertyValue( "Gear", gear );
        properties->getPropertyValue( "RPM", rpm );
        properties->getPropertyValue( "Engine Angular Velocity", engineAngularVelocity );
        properties->getPropertyValue( "Starter Gear Ratio", starterGearRatio );
        properties->getPropertyValue( "Clutch Throttle RPM Boost", clutchThrottleRPMBoost );
        properties->getPropertyValue( "Up Shift Base RPM Scale", upShiftBaseRPMScale );
        properties->getPropertyValue( "Up Shift Throttle RPM Scale", upShiftThrottleRPMScale );
        properties->getPropertyValue( "Down Shift Base RPM Scale", downShiftBaseRPMScale );
        properties->getPropertyValue( "Down Shift Throttle RPM Scale", downShiftThrottleRPMScale );
        properties->getPropertyValue( "Over Rev Torque Falloff", overRevTorqueFalloff );

        setFinalDriveRatio( finalDriveRatio );
        setMinRPM( minRPM );
        setMaxRPM( maxRPM );
        setMaxTorque( maxTorque );
        setTorqueRPM( torqueRPM );
        setMaxPower( maxPower );
        setPowerRPM( powerRPM );
        setEngineInertia( engineInertia );
        setEngineBaseFriction( engineBaseFriction );
        setEngineRPMFriction( engineRPMFriction );
        setDifferentialLockCoefficient( differentialLockCoefficient );
        setThrottle( throttle );
        setThrottleInput( throttleInput );
        setBrakeInput( brakeInput );
        setAutomatic( automatic );
        setGear( gear );
        setRPM( rpm );
        setEngineAngularVelocity( engineAngularVelocity );
        setStarterGearRatio( starterGearRatio );
        setClutchThrottleRPMBoost( clutchThrottleRPMBoost );
        setUpShiftBaseRPMScale( upShiftBaseRPMScale );
        setUpShiftThrottleRPMScale( upShiftThrottleRPMScale );
        setDownShiftBaseRPMScale( downShiftBaseRPMScale );
        setDownShiftThrottleRPMScale( downShiftThrottleRPMScale );
        setOverRevTorqueFalloff( overRevTorqueFalloff );
    }

    physics_Num CDriveTrain::getStarterGearRatio() const
    {
        return m_starterGearRatio;
    }

    void CDriveTrain::setStarterGearRatio( physics_Num starterGearRatio )
    {
        WP_ASSERT( isFiniteValue( starterGearRatio ) );
        WP_ASSERT( starterGearRatio >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( starterGearRatio < static_cast<physics_Num>( 100.0 ) );
        m_starterGearRatio = starterGearRatio;
    }

    physics_Num CDriveTrain::getClutchThrottleRPMBoost() const
    {
        return m_clutchThrottleRPMBoost;
    }

    void CDriveTrain::setClutchThrottleRPMBoost( physics_Num clutchThrottleRPMBoost )
    {
        WP_ASSERT( isFiniteValue( clutchThrottleRPMBoost ) );
        WP_ASSERT( clutchThrottleRPMBoost >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( clutchThrottleRPMBoost < MaxRPM );
        m_clutchThrottleRPMBoost = clutchThrottleRPMBoost >= static_cast<physics_Num>( 0.0 )
                                     ? clutchThrottleRPMBoost
                                     : static_cast<physics_Num>( 0.0 );
    }

    physics_Num CDriveTrain::getUpShiftThrottleRPMScale() const
    {
        return m_upShiftThrottleRPMScale;
    }

    void CDriveTrain::setUpShiftThrottleRPMScale( physics_Num upShiftThrottleRPMScale )
    {
        WP_ASSERT( isFiniteValue( upShiftThrottleRPMScale ) );
        m_upShiftThrottleRPMScale = upShiftThrottleRPMScale;
    }

    physics_Num CDriveTrain::getUpShiftBaseRPMScale() const
    {
        return m_upShiftBaseRPMScale;
    }

    void CDriveTrain::setUpShiftBaseRPMScale( physics_Num upShiftBaseRPMScale )
    {
        WP_ASSERT( isFiniteValue( upShiftBaseRPMScale ) );
        WP_ASSERT( upShiftBaseRPMScale >= static_cast<physics_Num>( 0.0 ) );
        m_upShiftBaseRPMScale = upShiftBaseRPMScale;
    }

    physics_Num CDriveTrain::getDownShiftThrottleRPMScale() const
    {
        return m_downShiftThrottleRPMScale;
    }

    void CDriveTrain::setDownShiftThrottleRPMScale( physics_Num downShiftThrottleRPMScale )
    {
        WP_ASSERT( isFiniteValue( downShiftThrottleRPMScale ) );
        m_downShiftThrottleRPMScale = downShiftThrottleRPMScale;
    }

    physics_Num CDriveTrain::getDownShiftBaseRPMScale() const
    {
        return m_downShiftBaseRPMScale;
    }

    void CDriveTrain::setDownShiftBaseRPMScale( physics_Num downShiftBaseRPMScale )
    {
        WP_ASSERT( isFiniteValue( downShiftBaseRPMScale ) );
        WP_ASSERT( downShiftBaseRPMScale >= static_cast<physics_Num>( 0.0 ) );
        m_downShiftBaseRPMScale = downShiftBaseRPMScale;
    }

    physics_Num CDriveTrain::getOverRevTorqueFalloff() const
    {
        return m_overRevTorqueFalloff;
    }

    void CDriveTrain::setOverRevTorqueFalloff( physics_Num overRevTorqueFalloff )
    {
        WP_ASSERT( isFiniteValue( overRevTorqueFalloff ) );
        WP_ASSERT( overRevTorqueFalloff >= static_cast<physics_Num>( 0.0 ) );
        m_overRevTorqueFalloff = overRevTorqueFalloff >= static_cast<physics_Num>( 0.0 )
                                   ? overRevTorqueFalloff
                                   : static_cast<physics_Num>( 0.0 );
    }
} // namespace workphone
