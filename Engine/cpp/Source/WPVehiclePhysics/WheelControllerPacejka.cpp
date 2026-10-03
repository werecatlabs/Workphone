#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/WheelControllerPacejka.hpp>
#include <Workphone/Workphone.hpp>
#include <array>

namespace workphone
{
    namespace
    {
        constexpr auto MaxWheelForce = static_cast<physics_Num>( 1.0e8 );
        constexpr auto MaxWheelVelocity = static_cast<physics_Num>( 1.0e5 );
        constexpr auto MaxAngularVelocity = static_cast<physics_Num>( 1.0e5 );
        constexpr auto MaxWheelTorque = static_cast<physics_Num>( 1.0e8 );
        constexpr auto MaxSteeringAngle = static_cast<physics_Num>( 1080.0 );
        constexpr auto CompressionTolerance = static_cast<physics_Num>( 0.05 );
        constexpr auto MinRayHitDistance = static_cast<physics_Num>( 1.0e-4 );
        constexpr auto MinForceLimit = static_cast<physics_Num>( 1.0 );
        constexpr auto MaxForceLimit = MaxWheelForce * static_cast<physics_Num>( 0.999 );
        constexpr auto MaxSuspensionForceMultiplier = static_cast<physics_Num>( 3.0 );
        constexpr auto MaxRoadForceMultiplier = static_cast<physics_Num>( 4.0 );

        bool isFiniteValue( physics_Num value )
        {
            return Math<physics_Num>::isFinite( value );
        }

        bool isFiniteVector( const Vector3<physics_Num> &value )
        {
            return MathUtil<physics_Num>::isFinite( value );
        }

        bool isUnitInput( physics_Num value )
        {
            return value >= static_cast<physics_Num>( 0.0 ) && value <= static_cast<physics_Num>( 1.0 );
        }

        physics_Num clampFinite( physics_Num value, physics_Num low, physics_Num high )
        {
            if( !isFiniteValue( value ) )
            {
                return low;
            }

            return Math<physics_Num>::clamp( value, low, high );
        }

        physics_Num forceLimitFromNormalLoad( physics_Num normalForce, physics_Num multiplier )
        {
            WP_ASSERT( isFiniteValue( normalForce ) );
            WP_ASSERT( normalForce >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( multiplier ) );
            WP_ASSERT( multiplier > static_cast<physics_Num>( 0.0 ) );

            if( !isFiniteValue( normalForce ) || normalForce <= static_cast<physics_Num>( 0.0 ) )
            {
                return MinForceLimit;
            }

            auto limit = normalForce * multiplier;
            if( !isFiniteValue( limit ) || limit < MinForceLimit )
            {
                limit = MinForceLimit;
            }
            else if( limit > MaxForceLimit )
            {
                limit = MaxForceLimit;
            }

            WP_ASSERT( isFiniteValue( limit ) );
            WP_ASSERT( limit >= MinForceLimit );
            WP_ASSERT( limit < MaxWheelForce );
            return limit;
        }

        Vector3<physics_Num> clampForceMagnitude( const Vector3<physics_Num> &force,
                                                  physics_Num                 maxMagnitude )
        {
            WP_ASSERT( isFiniteVector( force ) );
            WP_ASSERT( isFiniteValue( maxMagnitude ) );
            WP_ASSERT( maxMagnitude >= static_cast<physics_Num>( 0.0 ) );

            if( !isFiniteVector( force ) || !isFiniteValue( maxMagnitude ) ||
                maxMagnitude <= static_cast<physics_Num>( 0.0 ) )
            {
                return Vector3<physics_Num>::zero();
            }

            const auto length = force.length();
            WP_ASSERT( isFiniteValue( length ) );
            WP_ASSERT( length >= static_cast<physics_Num>( 0.0 ) );

            if( length > maxMagnitude && length > Math<physics_Num>::epsilon() )
            {
                auto result = force * ( maxMagnitude / length );
                WP_ASSERT( isFiniteVector( result ) );
                WP_ASSERT( result.length() <= maxMagnitude + Math<physics_Num>::epsilon() );
                return result;
            }

            return force;
        }

        void assertForceVector( const Vector3<physics_Num> &force )
        {
            WP_ASSERT( isFiniteVector( force ) );
            WP_ASSERT( force.length() < MaxWheelForce );
        }

        void assertVelocityVector( const Vector3<physics_Num> &velocity )
        {
            WP_ASSERT( isFiniteVector( velocity ) );
            WP_ASSERT( velocity.length() < MaxWheelVelocity );
        }

        void assertCompressionValue( physics_Num compression )
        {
            WP_ASSERT( isFiniteValue( compression ) );
            WP_ASSERT( compression >= -CompressionTolerance );
            WP_ASSERT( compression <= static_cast<physics_Num>( 1.0 ) + CompressionTolerance );
        }

        void assertWheelScalarInputs( const WheelControllerPacejka &wheel )
        {
            WP_ASSERT( isFiniteValue( wheel.getRadius() ) );
            WP_ASSERT( wheel.getRadius() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getSuspensionTravel() ) );
            WP_ASSERT( wheel.getSuspensionTravel() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getDamping() ) );
            WP_ASSERT( wheel.getDamping() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getInertia() ) );
            WP_ASSERT( wheel.getInertia() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getGrip() ) );
            WP_ASSERT( wheel.getGrip() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isUnitInput( wheel.getBrake() ) );
            WP_ASSERT( isUnitInput( wheel.getHandbrake() ) );
            WP_ASSERT( isFiniteValue( wheel.getDrivetrainInertia() ) );
            WP_ASSERT( wheel.getDrivetrainInertia() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getSuspensionForceInput() ) );
            WP_ASSERT( Math<physics_Num>::Abs( wheel.getSuspensionForceInput() ) < MaxWheelForce );
            WP_ASSERT( isFiniteValue( wheel.getDriveTorque() ) );
            WP_ASSERT( Math<physics_Num>::Abs( wheel.getDriveTorque() ) < MaxWheelTorque );
            WP_ASSERT( isFiniteValue( wheel.getDriveFrictionTorque() ) );
            WP_ASSERT( Math<physics_Num>::Abs( wheel.getDriveFrictionTorque() ) < MaxWheelTorque );

            if( !wheel.isPoweredWheel() )
            {
                WP_ASSERT( Math<physics_Num>::Abs( wheel.getDriveTorque() ) <=
                           Math<physics_Num>::epsilon() );
                WP_ASSERT( Math<physics_Num>::Abs( wheel.getDriveFrictionTorque() ) <=
                           Math<physics_Num>::epsilon() );
                WP_ASSERT( wheel.getDrivetrainInertia() <= Math<physics_Num>::epsilon() );
            }
        }
    } // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, WheelControllerPacejka, CVehicleComponent<IWheelComponent> );

    const std::array<physics_Num, 15> gA{
        1.0f, -60.0f,           1688.0f,         4140.f, 6.026f, 0.f, -0.3589f, 1.f,
        0.f,  -6.111f / 1000.f, -3.244f / 100.f, 0.f,    0.f,    0.f, 0.f
    };
    const std::array<physics_Num, 11> gB{ 1.0f, -60.0f, 1588.0f, 0.f, 229.f, 0.f,
                                          0.f,  0.f,    -10.f,   0.f, 0.f };

    s32 WheelControllerPacejka::m_ext = 0;

    WheelControllerPacejka::WheelControllerPacejka()
    {
        m_radius = 0.3f;
        m_suspensionTravel = 1.0f;
        m_damping = 3000.0f;
        m_inertia = 2.2f;
        m_grip = 0.2f;
        m_brakeFrictionTorque = 8000;
        m_handbrakeFrictionTorque = 0;
        m_frictionTorque = 0.01;
        m_massFraction = 0.25f;

        m_pacejkaA.insert( m_pacejkaA.begin(), gA.begin(), gA.end() );
        m_pacejkaB.insert( m_pacejkaB.begin(), gB.begin(), gB.end() );

        m_driveTorque = 0;
        m_driveFrictionTorque = 0;
        m_brake = 0;
        m_handbrake = 0;
        m_steeringAngle = 0;
        m_drivetrainInertia = 0;
        m_suspensionForceInput = 0;

        m_angularVelocity = 0;
        m_slipRatio = 0;
        m_slipVelo = 0;
        m_compression = 0;

        m_isSteeringWheel = false;

        initSlipMaxima();

        m_lastSkid = -1;

        m_hit = workphone::make_ptr<physics::RaycastHit>();
        m_hit->setCheckDynamic( false );
        m_hit->setCheckStatic( true );

        setId( m_ext++ );
    }

    void WheelControllerPacejka::update()
    {
        auto task = Thread::getCurrentTask();
        if( task == TaskId::Physics )
        {
            auto state = getState();
            switch( state )
            {
            case State::EDIT:
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );
                if( !applicationManager )
                {
                    return;
                }

                auto timer = applicationManager->getTimerPtr();
                WP_ASSERT( timer );
                if( !timer )
                {
                    return;
                }

                auto t = timer->getTime();
                auto dt = timer->getDeltaTime();
                if( dt > 1.0 / 30.0 )
                {
                    dt = 1.0 / 30.0;
                }

                WP_ASSERT( isValid() );
                WP_ASSERT( Math<real_Num>::isFinite( static_cast<real_Num>( t ) ) );
                WP_ASSERT( Math<real_Num>::isFinite( static_cast<real_Num>( dt ) ) );
                WP_ASSERT( dt >= static_cast<real_Num>( 0.0 ) );
                WP_ASSERT( dt < static_cast<real_Num>( 1.0 ) );
                assertWheelScalarInputs( *this );
                updateTransform();
                updateWheel( 0, t, dt );
            }
            break;
            case State::PLAY:
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );
                if( !applicationManager )
                {
                    return;
                }

                auto timer = applicationManager->getTimerPtr();
                WP_ASSERT( timer );
                if( !timer )
                {
                    return;
                }

                auto t = timer->getTime();
                auto dt = timer->getDeltaTime();
                if( dt > 1.0 / 30.0 )
                {
                    dt = 1.0 / 30.0;
                }

                WP_ASSERT( isValid() );
                WP_ASSERT( Math<real_Num>::isFinite( static_cast<real_Num>( t ) ) );
                WP_ASSERT( Math<real_Num>::isFinite( static_cast<real_Num>( dt ) ) );
                WP_ASSERT( dt >= static_cast<real_Num>( 0.0 ) );
                WP_ASSERT( dt < static_cast<real_Num>( 1.0 ) );
                assertWheelScalarInputs( *this );
                updateTransform();
                updateWheel( 0, t, dt );
            }
            break;
            }
        }
    }

    float WheelControllerPacejka::calcLongitudinalForce( float Fz, float slip )
    {
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( Fz ) ) );
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( slip ) ) );
        WP_ASSERT( Fz >= 0.0f );
        WP_ASSERT( m_pacejkaA.size() >= 15 );
        WP_ASSERT( m_pacejkaB.size() >= 11 );

        if( !Math<physics_Num>::isFinite( static_cast<physics_Num>( Fz ) ) ||
            !Math<physics_Num>::isFinite( static_cast<physics_Num>( slip ) ) ||
            Fz <= std::numeric_limits<float>::epsilon() )
        {
            return 0.0f;
        }

        if( m_pacejkaA.size() >= 15 && m_pacejkaB.size() >= 11 )
        {
            Fz *= static_cast<physics_Num>( 0.001 );   // convert to kN
            slip *= static_cast<physics_Num>( 100.0 ); // covert to %

            auto uP = m_pacejkaB[1] * Fz + m_pacejkaB[2];
            WP_ASSERT( Math<physics_Num>::Abs( uP ) > Math<physics_Num>::epsilon() );
            auto D = uP * Fz;
            auto B = ( ( m_pacejkaB[3] * Fz + m_pacejkaB[4] ) *
                       Math<physics_Num>::Exp( -m_pacejkaB[5] * Fz ) ) /
                     ( m_pacejkaB[0] * uP );
            auto S = slip + m_pacejkaB[9] * Fz + m_pacejkaB[10];
            auto E = m_pacejkaB[6] * Fz * Fz + m_pacejkaB[7] * Fz + m_pacejkaB[8];
            auto Fx =
                D * Math<physics_Num>::Sin(
                        m_pacejkaB[0] * Math<physics_Num>::Atan(
                                            S * B + E * ( Math<physics_Num>::Atan( S * B ) - S * B ) ) );
            WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( Fx ) ) );
            WP_ASSERT( Math<physics_Num>::Abs( static_cast<physics_Num>( Fx ) ) < MaxWheelForce );
            if( !Math<physics_Num>::isFinite( static_cast<physics_Num>( Fx ) ) )
            {
                return 0.0f;
            }

            return clampFinite( Fx, -MaxWheelForce, MaxWheelForce );
        }

        return 0.0f;
    }

    float WheelControllerPacejka::calcLateralForce( float Fz, float slipAngle )
    {
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( Fz ) ) );
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( slipAngle ) ) );
        WP_ASSERT( Fz >= 0.0f );
        WP_ASSERT( m_pacejkaA.size() >= 15 );
        WP_ASSERT( m_pacejkaB.size() >= 11 );

        if( !Math<physics_Num>::isFinite( static_cast<physics_Num>( Fz ) ) ||
            !Math<physics_Num>::isFinite( static_cast<physics_Num>( slipAngle ) ) ||
            Fz <= std::numeric_limits<float>::epsilon() )
        {
            return 0.0f;
        }

        if( m_pacejkaA.size() >= 15 && m_pacejkaB.size() >= 11 )
        {
            Fz *= static_cast<physics_Num>( 0.001 ); // convert to kN
            WP_ASSERT( Fz > static_cast<physics_Num>( 0.0 ) );
            slipAngle *= ( static_cast<physics_Num>( 360.0 ) /
                           ( static_cast<physics_Num>( 2.0 ) * Math<physics_Num>::pi() ) );
            // convert angle to deg
            auto uP = m_pacejkaA[1] * Fz + m_pacejkaA[2];
            WP_ASSERT( Math<physics_Num>::Abs( uP ) > Math<physics_Num>::epsilon() );
            auto D = uP * Fz;
            auto B = ( m_pacejkaA[3] *
                       Math<physics_Num>::Sin( static_cast<physics_Num>( 2.0 ) *
                                               Math<physics_Num>::Atan( Fz / m_pacejkaA[4] ) ) ) /
                     ( m_pacejkaA[0] * uP * Fz );
            auto S = slipAngle + m_pacejkaA[9] * Fz + m_pacejkaA[10];
            auto E = m_pacejkaA[6] * Fz + m_pacejkaA[7];
            auto Sv = m_pacejkaA[12] * Fz + m_pacejkaA[13];
            auto Fy = D * Math<physics_Num>::Sin(
                              m_pacejkaA[0] *
                              Math<physics_Num>::Atan(
                                  S * B + E * ( Math<physics_Num>::Atan( S * B ) - S * B ) ) ) +
                      Sv;
            WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( Fy ) ) );
            WP_ASSERT( Math<physics_Num>::Abs( static_cast<physics_Num>( Fy ) ) < MaxWheelForce );
            if( !Math<physics_Num>::isFinite( static_cast<physics_Num>( Fy ) ) )
            {
                return 0.0f;
            }

            return clampFinite( Fy, -MaxWheelForce, MaxWheelForce );
        }

        return 0.0f;
    }

    WheelControllerPacejka::~WheelControllerPacejka()
    {
        unload( nullptr );
    }

    void WheelControllerPacejka::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                m_hit = nullptr;

                CVehicleComponent<IWheelComponent>::unload( data );
                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    float WheelControllerPacejka::calcLongitudinalForceUnit( float Fz, float slip )
    {
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( Fz ) ) );
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( slip ) ) );
        WP_ASSERT( Fz >= 0.0f );
        WP_ASSERT( isFiniteValue( m_maxSlip ) );
        WP_ASSERT( m_maxSlip > static_cast<physics_Num>( 0.0 ) );

        auto result = calcLongitudinalForce( Fz, slip * m_maxSlip );
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( result ) ) );
        return result;
    }

    float WheelControllerPacejka::calcLateralForceUnit( float Fz, float slipAngle )
    {
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( Fz ) ) );
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( slipAngle ) ) );
        WP_ASSERT( Fz >= 0.0f );
        WP_ASSERT( isFiniteValue( m_maxAngle ) );
        WP_ASSERT( m_maxAngle > static_cast<physics_Num>( 0.0 ) );

        auto result = calcLateralForce( Fz, slipAngle * m_maxAngle );
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( result ) ) );
        return result;
    }

    Vector3<physics_Num> WheelControllerPacejka::combinedForce( float Fz, float slip, float slipAngle )
    {
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( Fz ) ) );
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( slip ) ) );
        WP_ASSERT( Math<physics_Num>::isFinite( static_cast<physics_Num>( slipAngle ) ) );
        WP_ASSERT( Fz >= 0.0f );
        WP_ASSERT( isFiniteValue( m_maxSlip ) );
        WP_ASSERT( isFiniteValue( m_maxAngle ) );
        WP_ASSERT( m_maxSlip > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( m_maxAngle > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteVector( m_localVelo ) );

        if( !Math<physics_Num>::isFinite( static_cast<physics_Num>( Fz ) ) ||
            !Math<physics_Num>::isFinite( static_cast<physics_Num>( slip ) ) ||
            !Math<physics_Num>::isFinite( static_cast<physics_Num>( slipAngle ) ) ||
            Fz <= std::numeric_limits<float>::epsilon() )
        {
            return Vector3<physics_Num>::zero();
        }

        auto unitSlip = slip / m_maxSlip;
        auto unitAngle = slipAngle / m_maxAngle;
        auto p = Math<physics_Num>::Sqrt( unitSlip * unitSlip + unitAngle * unitAngle );
        WP_ASSERT( isFiniteValue( p ) );

        if( p > std::numeric_limits<physics_Num>::epsilon() )
        {
            if( slip < static_cast<physics_Num>( -0.8 ) )
            {
                WP_ASSERT( m_localVelo.lengthSquared() > Math<physics_Num>::epsilon() );
                auto result =
                    -m_localVelo.normaliseCopy() *
                    ( Math<physics_Num>::Abs( unitAngle / p * calcLateralForceUnit( Fz, p ) ) +
                      Math<physics_Num>::Abs( unitSlip / p * calcLongitudinalForceUnit( Fz, p ) ) );
                result = clampForceMagnitude(
                    result,
                    forceLimitFromNormalLoad( static_cast<physics_Num>( Fz ), MaxRoadForceMultiplier ) );
                assertForceVector( result );
                return result;
            }
            // auto forward = Vector3<physics_Num>(0.0, m_groundNormal.Z(), m_groundNormal.Y());

            auto forward = Vector3<physics_Num>::forward();
            auto result = Vector3<physics_Num>::right() * unitAngle / p * calcLateralForceUnit( Fz, p ) +
                          forward * unitSlip / p * calcLongitudinalForceUnit( Fz, p );
            result =
                clampForceMagnitude( result, forceLimitFromNormalLoad( static_cast<physics_Num>( Fz ),
                                                                       MaxRoadForceMultiplier ) );
            assertForceVector( result );
            return result;
        }

        return Vector3<physics_Num>::zero();
    }

    void WheelControllerPacejka::initSlipMaxima()
    {
        const auto stepSize = static_cast<physics_Num>( 0.001 );
        const auto testNormalForce = static_cast<physics_Num>( 4000.0 );
        WP_ASSERT( stepSize > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( testNormalForce > static_cast<physics_Num>( 0.0 ) );

        physics_Num force = 0;
        for( physics_Num slip = stepSize;; slip += stepSize )
        {
            physics_Num newForce = calcLongitudinalForce( testNormalForce, slip );
            WP_ASSERT( isFiniteValue( newForce ) );
            if( force < newForce )
            {
                force = newForce;
            }
            else
            {
                m_maxSlip = slip - stepSize;
                break;
            }
        }

        force = 0;
        for( physics_Num slipAngle = stepSize;; slipAngle += stepSize )
        {
            physics_Num newForce = calcLateralForce( testNormalForce, slipAngle );
            WP_ASSERT( isFiniteValue( newForce ) );
            if( force < newForce )
            {
                force = newForce;
            }
            else
            {
                m_maxAngle = slipAngle - stepSize;
                break;
            }
        }

        WP_ASSERT( isFiniteValue( m_maxSlip ) );
        WP_ASSERT( m_maxSlip > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_maxAngle ) );
        WP_ASSERT( m_maxAngle > static_cast<physics_Num>( 0.0 ) );
    }

    float WheelControllerPacejka::calculateSlipRatio()
    {
        const auto fullSlipVelo = static_cast<physics_Num>( 4.0 );
        const auto minSlipVelo = static_cast<physics_Num>( 0.5 );
        WP_ASSERT( fullSlipVelo > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( minSlipVelo > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteVector( m_wheelVelo ) );
        WP_ASSERT( isFiniteVector( m_forward ) );
        WP_ASSERT( isFiniteValue( m_angularVelocity ) );
        WP_ASSERT( isFiniteValue( m_radius ) );
        WP_ASSERT( m_radius > static_cast<physics_Num>( 0.0 ) );

        auto wheelRoadVelo = m_wheelVelo.dotProduct( m_forward );
        WP_ASSERT( isFiniteValue( wheelRoadVelo ) );

        auto absRoadVelo = Math<physics_Num>::Abs( wheelRoadVelo );
        WP_ASSERT( isFiniteValue( absRoadVelo ) );

        auto wheelTireVelo = m_angularVelocity * m_radius;
        WP_ASSERT( isFiniteValue( wheelTireVelo ) );

        const auto slipDenominator = absRoadVelo > minSlipVelo ? absRoadVelo : minSlipVelo;
        WP_ASSERT( isFiniteValue( slipDenominator ) );
        WP_ASSERT( slipDenominator > static_cast<physics_Num>( 0.0 ) );

        auto result = ( wheelTireVelo - wheelRoadVelo ) / slipDenominator;
        result = Math<physics_Num>::clamp( result, static_cast<physics_Num>( -3.0 ),
                                           static_cast<physics_Num>( 3.0 ) );
        WP_ASSERT( isFiniteValue( result ) );
        return result;
    }

    float WheelControllerPacejka::calculateSlipAngle()
    {
        const float fullAngleVelo = static_cast<physics_Num>( 2.0 );
        WP_ASSERT( fullAngleVelo > 0.0f );
        WP_ASSERT( isFiniteVector( m_localVelo ) );

        auto wheelMotionDirection = m_localVelo;
        wheelMotionDirection.Y() = 0;
        WP_ASSERT( isFiniteVector( wheelMotionDirection ) );

        if( wheelMotionDirection.lengthSquared() < std::numeric_limits<physics_Num>::epsilon() )
        {
            return static_cast<physics_Num>( 0.0 );
        }

        auto sinSlipAngle = wheelMotionDirection.normaliseCopy().X();
        sinSlipAngle = Math<physics_Num>::clamp( sinSlipAngle, static_cast<physics_Num>( -1.0 ),
                                                 static_cast<physics_Num>( 1.0 ) );
        WP_ASSERT( isFiniteValue( sinSlipAngle ) );
        // To avoid precision errors.

        auto damping =
            Math<physics_Num>::clamp( m_localVelo.length() / fullAngleVelo,
                                      static_cast<physics_Num>( 0.0 ), static_cast<physics_Num>( 1.0 ) );
        WP_ASSERT( isFiniteValue( damping ) );

        auto result = -Math<physics_Num>::ASin( sinSlipAngle ) * damping * damping;
        WP_ASSERT( isFiniteValue( result ) );
        return result;
    }

    Vector3<physics_Num> WheelControllerPacejka::roadForce( physics_Num dt )
    {
        WP_ASSERT( isFiniteValue( dt ) );
        WP_ASSERT( dt >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( dt < static_cast<physics_Num>( 1.0 ) );
        assertWheelScalarInputs( *this );
        WP_ASSERT( isFiniteValue( m_normalForce ) );
        WP_ASSERT( m_normalForce >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_maxSteeringAngle ) );
        WP_ASSERT( m_maxSteeringAngle >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( m_maxSteeringAngle <= MaxSteeringAngle );
        WP_ASSERT( isFiniteValue( m_steeringAngle ) );
        WP_ASSERT( Math<physics_Num>::Abs( m_steeringAngle ) <= MaxSteeringAngle );
        WP_ASSERT( isFiniteValue( m_driveTorque ) );
        WP_ASSERT( Math<physics_Num>::Abs( m_driveTorque ) < MaxWheelTorque );
        WP_ASSERT( isFiniteValue( m_driveFrictionTorque ) );
        WP_ASSERT( Math<physics_Num>::Abs( m_driveFrictionTorque ) < MaxWheelTorque );
        assertVelocityVector( m_wheelVelo );

        auto vehicle = getOwner();
        WP_ASSERT( vehicle );
        if( !vehicle )
        {
            return Vector3<physics_Num>::zero();
        }

        auto vehicleBody = vehicle->getBody();
        WP_ASSERT( vehicleBody );
        if( !vehicleBody )
        {
            return Vector3<physics_Num>::zero();
        }

        auto vehicleWorldTransform = vehicle->getWorldTransform();
        WP_ASSERT( vehicleWorldTransform.isFinite() );

        auto slipRes =
            ( static_cast<physics_Num>( 100.0 ) - Math<physics_Num>::Abs( m_angularVelocity ) ) /
            static_cast<physics_Num>( 10.0 );
        if( slipRes < 1 )
        {
            slipRes = 1;
        }
        WP_ASSERT( isFiniteValue( slipRes ) );
        WP_ASSERT( slipRes >= static_cast<physics_Num>( 1.0 ) );

        auto mass = vehicle->getMass();
        WP_ASSERT( isFiniteValue( mass ) );
        WP_ASSERT( mass > static_cast<physics_Num>( 0.0 ) );
        auto invMass = static_cast<physics_Num>( 1.0 ) / mass;
        if( Math<physics_Num>::equals( mass, 0.0 ) )
        {
            invMass = static_cast<physics_Num>( 0.0 );
        }
        WP_ASSERT( isFiniteValue( invMass ) );
        WP_ASSERT( invMass >= static_cast<physics_Num>( 0.0 ) );

        auto invSlipRes = static_cast<physics_Num>( 1.0 ) / slipRes;
        auto totalInertia = m_inertia + m_drivetrainInertia;
        WP_ASSERT( isFiniteValue( totalInertia ) );
        WP_ASSERT( totalInertia > static_cast<physics_Num>( 0.0 ) );
        auto driveAngularDelta = m_driveTorque * dt * invSlipRes / totalInertia;
        auto totalFrictionTorque = m_brakeFrictionTorque * m_brake +
                                   m_handbrakeFrictionTorque * m_handbrake + m_frictionTorque +
                                   m_driveFrictionTorque;
        auto frictionAngularDelta = totalFrictionTorque * dt * invSlipRes / totalInertia;
        WP_ASSERT( isFiniteValue( invSlipRes ) );
        WP_ASSERT( isFiniteValue( driveAngularDelta ) );
        WP_ASSERT( isFiniteValue( totalFrictionTorque ) );
        WP_ASSERT( totalFrictionTorque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( frictionAngularDelta ) );
        WP_ASSERT( frictionAngularDelta >= static_cast<physics_Num>( 0.0 ) );

        auto       totalForce = Vector3<physics_Num>::zero();
        const auto roadForceLimit = forceLimitFromNormalLoad( m_normalForce, MaxRoadForceMultiplier );
        const auto subStepRoadForceLimit = roadForceLimit * invSlipRes;
        auto       newAngle =
            Math<physics_Num>::clamp( -m_steeringAngle, -m_maxSteeringAngle, m_maxSteeringAngle );
        WP_ASSERT( isFiniteValue( newAngle ) );
        WP_ASSERT( isFiniteValue( roadForceLimit ) );
        WP_ASSERT( roadForceLimit >= MinForceLimit );
        WP_ASSERT( isFiniteValue( subStepRoadForceLimit ) );
        WP_ASSERT( subStepRoadForceLimit > static_cast<physics_Num>( 0.0 ) );

        m_forward = m_localRotation * Vector3<physics_Num>::forward();
        m_right = m_localRotation * Vector3<physics_Num>::right();
        WP_ASSERT( isFiniteVector( m_forward ) );
        WP_ASSERT( isFiniteVector( m_right ) );

        for( s32 i = 0; i < static_cast<s32>( slipRes ); ++i )
        {
            auto f = static_cast<physics_Num>( i ) * static_cast<physics_Num>( 1.0 ) / slipRes;
            WP_ASSERT( isFiniteValue( f ) );
            m_localRotation = Quaternion<physics_Num>::eulerDegrees(
                0.0, m_oldAngle + ( newAngle - m_oldAngle ) * f, 0.0 );
            m_inverseLocalRotation = m_localRotation.inverse();
            WP_ASSERT( MathUtil<physics_Num>::isFinite( m_localRotation ) );
            WP_ASSERT( MathUtil<physics_Num>::isFinite( m_inverseLocalRotation ) );

            m_forward =
                vehicleWorldTransform.transformVector( m_localRotation * Vector3<real_Num>::forward() );
            m_right =
                vehicleWorldTransform.transformVector( m_localRotation * Vector3<real_Num>::right() );
            WP_ASSERT( isFiniteVector( m_forward ) );
            WP_ASSERT( isFiniteVector( m_right ) );
            WP_ASSERT( m_forward.lengthSquared() > Math<physics_Num>::epsilon() );
            WP_ASSERT( m_right.lengthSquared() > Math<physics_Num>::epsilon() );

            auto vehicleLocalWheelVelo = vehicleWorldTransform.inverseTransformVector( m_wheelVelo );
            m_localVelo = m_inverseLocalRotation * vehicleLocalWheelVelo;
            assertVelocityVector( vehicleLocalWheelVelo );
            assertVelocityVector( m_localVelo );

            m_slipRatio = calculateSlipRatio();
            m_slipAngle = calculateSlipAngle();
            WP_ASSERT( isFiniteValue( m_slipRatio ) );
            WP_ASSERT( isFiniteValue( m_slipAngle ) );

            Vector3<real_Num> force =
                invSlipRes * m_grip * combinedForce( m_normalForce, m_slipRatio, m_slipAngle );
            force = clampForceMagnitude( force, subStepRoadForceLimit );
            Vector3<real_Num> worldForce =
                vehicleWorldTransform.transformVector( m_localRotation * force );
            worldForce = clampForceMagnitude( worldForce, subStepRoadForceLimit );
            assertForceVector( force );
            assertForceVector( worldForce );

            // Tire force on the chassis applies the opposite torque back through the contact patch.
            m_angularVelocity -= ( force.Z() * m_radius * dt ) / totalInertia;
            m_angularVelocity += driveAngularDelta;
            WP_ASSERT( isFiniteValue( m_angularVelocity ) );
            WP_ASSERT( Math<physics_Num>::Abs( m_angularVelocity ) < MaxAngularVelocity );

            if( Math<physics_Num>::Abs( m_angularVelocity ) > frictionAngularDelta )
            {
                m_angularVelocity -= frictionAngularDelta * Math<physics_Num>::Sign( m_angularVelocity );
            }
            else
            {
                m_angularVelocity = static_cast<physics_Num>( 0.0 );
            }

            WP_ASSERT( isFiniteValue( m_angularVelocity ) );

            WP_ASSERT( MathUtil<real_Num>::isFinite( worldForce ) );
            WP_ASSERT( Math<real_Num>::isFinite( invSlipRes ) );

            m_wheelVelo += worldForce * invMass * dt * invSlipRes;
            WP_ASSERT( MathUtil<real_Num>::isFinite( m_wheelVelo ) );
            assertVelocityVector( m_wheelVelo );

            totalForce += worldForce;
            totalForce = clampForceMagnitude( totalForce, roadForceLimit );
            assertForceVector( totalForce );
        }

        real_Num longitunalSlipVelo =
            Math<real_Num>::Abs( m_angularVelocity * m_radius - m_wheelVelo.dotProduct( m_forward ) );
        real_Num lateralSlipVelo = m_wheelVelo.dotProduct( m_right );
        WP_ASSERT( Math<real_Num>::isFinite( longitunalSlipVelo ) );
        WP_ASSERT( Math<real_Num>::isFinite( lateralSlipVelo ) );
        m_slipVelo = Math<real_Num>::Sqrt( longitunalSlipVelo * longitunalSlipVelo +
                                           lateralSlipVelo * lateralSlipVelo );
        WP_ASSERT( isFiniteValue( m_slipVelo ) );
        WP_ASSERT( m_slipVelo >= static_cast<physics_Num>( 0.0 ) );

        m_oldAngle = newAngle;
        WP_ASSERT( isFiniteValue( m_oldAngle ) );

        WP_ASSERT( MathUtil<physics_Num>::isFinite( totalForce ) );
        totalForce = clampForceMagnitude( totalForce, roadForceLimit );
        assertForceVector( totalForce );
        return totalForce;
    }

    Vector3<physics_Num> WheelControllerPacejka::suspensionForce()
    {
        assertCompressionValue( m_compression );
        WP_ASSERT( isFiniteValue( m_fullCompressionSpringForce ) );
        WP_ASSERT( m_fullCompressionSpringForce >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteVector( m_localVelo ) );
        WP_ASSERT( isFiniteVector( m_groundNormal ) );
        WP_ASSERT( m_groundNormal.lengthSquared() > Math<physics_Num>::epsilon() );
        WP_ASSERT( isFiniteValue( m_damping ) );
        WP_ASSERT( m_damping >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_suspensionForceInput ) );

        auto springForce = m_compression * m_fullCompressionSpringForce;
        WP_ASSERT( isFiniteValue( springForce ) );
        WP_ASSERT( springForce >= static_cast<physics_Num>( 0.0 ) );

        auto damperForce = m_localVelo.Y() * m_damping;
        WP_ASSERT( isFiniteValue( damperForce ) );

        auto       suspensionMagnitude = springForce - damperForce + m_suspensionForceInput;
        const auto maxSuspensionForce =
            forceLimitFromNormalLoad( m_fullCompressionSpringForce, MaxSuspensionForceMultiplier );
        suspensionMagnitude =
            clampFinite( suspensionMagnitude, static_cast<physics_Num>( 0.0 ), maxSuspensionForce );
        m_normalForce = suspensionMagnitude;

        WP_ASSERT( isFiniteValue( suspensionMagnitude ) );
        WP_ASSERT( suspensionMagnitude >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( suspensionMagnitude <= maxSuspensionForce + Math<physics_Num>::epsilon() );
        WP_ASSERT( isFiniteValue( m_normalForce ) );
        WP_ASSERT( m_normalForce >= static_cast<physics_Num>( 0.0 ) );

        auto result = suspensionMagnitude * Vector3<physics_Num>::up();
        assertForceVector( result );
        return result;
    }

    void WheelControllerPacejka::updateCompressionForce()
    {
        auto owner = getOwner();
        if( owner )
        {
            auto body = owner->getBody();
            if( body )
            {
                const Vector3<physics_Num> gravity( 0.0, 9.81, 0.0 );
                const auto                 mass = body->getMass();
                WP_ASSERT( isFiniteValue( mass ) );
                WP_ASSERT( mass > static_cast<physics_Num>( 0.0 ) );
                WP_ASSERT( isFiniteValue( m_massFraction ) );
                WP_ASSERT( m_massFraction >= static_cast<physics_Num>( 0.0 ) );
                WP_ASSERT( m_massFraction <= static_cast<physics_Num>( 1.0 ) );
                m_fullCompressionSpringForce =
                    mass * m_massFraction * static_cast<physics_Num>( 2.0 ) * gravity.Y();
                WP_ASSERT( isFiniteValue( m_fullCompressionSpringForce ) );
                WP_ASSERT( m_fullCompressionSpringForce >= static_cast<physics_Num>( 0.0 ) );
            }
        }
    }

    bool WheelControllerPacejka::isPoweredWheel() const
    {
        return m_isPoweredWheel;
    }

    void WheelControllerPacejka::setPoweredWheel( bool poweredWheel )
    {
        m_isPoweredWheel = poweredWheel;

        if( !m_isPoweredWheel )
        {
            m_driveTorque = static_cast<physics_Num>( 0.0 );
            m_driveFrictionTorque = static_cast<physics_Num>( 0.0 );
            m_drivetrainInertia = static_cast<physics_Num>( 0.0 );
        }
    }

    Vector3<physics_Num> WheelControllerPacejka::getWheelVelo() const
    {
        return m_wheelVelo;
    }

    void WheelControllerPacejka::setWheelVelo( const Vector3<physics_Num> &wheelVelo )
    {
        assertVelocityVector( wheelVelo );
        m_wheelVelo = wheelVelo;
    }

    Vector3<physics_Num> WheelControllerPacejka::getLocalVelo() const
    {
        return m_localVelo;
    }

    void WheelControllerPacejka::setLocalVelo( const Vector3<physics_Num> &localVelo )
    {
        assertVelocityVector( localVelo );
        m_localVelo = localVelo;
    }

    Vector3<physics_Num> WheelControllerPacejka::getGroundNormal() const
    {
        return m_groundNormal;
    }

    void WheelControllerPacejka::setGroundNormal( const Vector3<physics_Num> &groundNormal )
    {
        WP_ASSERT( isFiniteVector( groundNormal ) );
        m_groundNormal = groundNormal;
    }

    Vector3<physics_Num> WheelControllerPacejka::getSuspensionForceVector() const
    {
        return m_suspensionForce;
    }

    void WheelControllerPacejka::setSuspensionForceVector( const Vector3<physics_Num> &suspensionForce )
    {
        assertForceVector( suspensionForce );
        m_suspensionForce = suspensionForce;
    }

    Vector3<physics_Num> WheelControllerPacejka::getRoadForceVector() const
    {
        return m_roadForce;
    }

    void WheelControllerPacejka::setRoadForceVector( const Vector3<physics_Num> &roadForce )
    {
        assertForceVector( roadForce );
        m_roadForce = roadForce;
    }

    Vector3<physics_Num> WheelControllerPacejka::getUp() const
    {
        return m_up;
    }

    void WheelControllerPacejka::setUp( const Vector3<physics_Num> &up )
    {
        WP_ASSERT( isFiniteVector( up ) );
        WP_ASSERT( up.lengthSquared() > Math<physics_Num>::epsilon() );
        m_up = up;
    }

    Vector3<physics_Num> WheelControllerPacejka::getRight() const
    {
        return m_right;
    }

    void WheelControllerPacejka::setRight( const Vector3<physics_Num> &right )
    {
        WP_ASSERT( isFiniteVector( right ) );
        WP_ASSERT( right.lengthSquared() > Math<physics_Num>::epsilon() );
        m_right = right;
    }

    Vector3<physics_Num> WheelControllerPacejka::getForward() const
    {
        return m_forward;
    }

    void WheelControllerPacejka::setForward( const Vector3<physics_Num> &forward )
    {
        WP_ASSERT( isFiniteVector( forward ) );
        WP_ASSERT( forward.lengthSquared() > Math<physics_Num>::epsilon() );
        m_forward = forward;
    }

    Quaternion<physics_Num> WheelControllerPacejka::getLocalRotation() const
    {
        return m_localRotation;
    }

    void WheelControllerPacejka::setLocalRotation( const Quaternion<physics_Num> &localRotation )
    {
        WP_ASSERT( MathUtil<physics_Num>::isFinite( localRotation ) );
        m_localRotation = localRotation;
    }

    Quaternion<physics_Num> WheelControllerPacejka::getInverseLocalRotation() const
    {
        return m_inverseLocalRotation;
    }

    void WheelControllerPacejka::setInverseLocalRotation(
        const Quaternion<physics_Num> &inverseLocalRotation )
    {
        WP_ASSERT( MathUtil<physics_Num>::isFinite( inverseLocalRotation ) );
        m_inverseLocalRotation = inverseLocalRotation;
    }

    SmartPtr<physics::IRaycastHit> WheelControllerPacejka::getHit() const
    {
        return m_hit;
    }

    void WheelControllerPacejka::setHit( SmartPtr<physics::IRaycastHit> hit )
    {
        WP_ASSERT( hit );
        m_hit = hit;
    }

    physics_Num WheelControllerPacejka::getInertia() const
    {
        return m_inertia;
    }

    void WheelControllerPacejka::setInertia( physics_Num inertia )
    {
        WP_ASSERT( isFiniteValue( inertia ) );
        WP_ASSERT( inertia > static_cast<physics_Num>( 0.0 ) );
        m_inertia = inertia;
    }

    physics_Num WheelControllerPacejka::getGrip() const
    {
        return m_grip;
    }

    void WheelControllerPacejka::setGrip( physics_Num grip )
    {
        WP_ASSERT( isFiniteValue( grip ) );
        WP_ASSERT( grip >= static_cast<physics_Num>( 0.0 ) );
        m_grip = grip;
    }

    physics_Num WheelControllerPacejka::getBrakeFrictionTorque() const
    {
        return m_brakeFrictionTorque;
    }

    void WheelControllerPacejka::setBrakeFrictionTorque( physics_Num brakeFrictionTorque )
    {
        WP_ASSERT( isFiniteValue( brakeFrictionTorque ) );
        WP_ASSERT( brakeFrictionTorque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( brakeFrictionTorque < MaxWheelTorque );
        m_brakeFrictionTorque = brakeFrictionTorque;
    }

    physics_Num WheelControllerPacejka::getHandbrakeFrictionTorque() const
    {
        return m_handbrakeFrictionTorque;
    }

    void WheelControllerPacejka::setHandbrakeFrictionTorque( physics_Num handbrakeFrictionTorque )
    {
        WP_ASSERT( isFiniteValue( handbrakeFrictionTorque ) );
        WP_ASSERT( handbrakeFrictionTorque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( handbrakeFrictionTorque < MaxWheelTorque );
        m_handbrakeFrictionTorque = handbrakeFrictionTorque;
    }

    physics_Num WheelControllerPacejka::getFrictionTorque() const
    {
        return m_frictionTorque;
    }

    void WheelControllerPacejka::setFrictionTorque( physics_Num frictionTorque )
    {
        WP_ASSERT( isFiniteValue( frictionTorque ) );
        WP_ASSERT( frictionTorque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( frictionTorque < MaxWheelTorque );
        m_frictionTorque = frictionTorque;
    }

    physics_Num WheelControllerPacejka::getMaxSteeringAngle() const
    {
        return m_maxSteeringAngle;
    }

    void WheelControllerPacejka::setMaxSteeringAngle( physics_Num maxSteeringAngle )
    {
        WP_ASSERT( isFiniteValue( maxSteeringAngle ) );
        WP_ASSERT( maxSteeringAngle >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( maxSteeringAngle <= MaxSteeringAngle );
        m_maxSteeringAngle = maxSteeringAngle;
    }

    physics_Num WheelControllerPacejka::getMassFraction() const
    {
        return m_massFraction;
    }

    void WheelControllerPacejka::setMassFraction( physics_Num massFraction )
    {
        WP_ASSERT( isFiniteValue( massFraction ) );
        WP_ASSERT( massFraction >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( massFraction <= static_cast<physics_Num>( 1.0 ) );
        m_massFraction = massFraction;
    }

    physics_Num WheelControllerPacejka::getDriveTorque() const
    {
        return m_driveTorque;
    }

    void WheelControllerPacejka::setDriveTorque( physics_Num driveTorque )
    {
        WP_ASSERT( isFiniteValue( driveTorque ) );
        WP_ASSERT( Math<physics_Num>::Abs( driveTorque ) < MaxWheelTorque );
        WP_ASSERT( m_isPoweredWheel ||
                   Math<physics_Num>::Abs( driveTorque ) <= Math<physics_Num>::epsilon() );
        m_driveTorque = driveTorque;
    }

    physics_Num WheelControllerPacejka::getDriveFrictionTorque() const
    {
        return m_driveFrictionTorque;
    }

    void WheelControllerPacejka::setDriveFrictionTorque( physics_Num driveFrictionTorque )
    {
        WP_ASSERT( isFiniteValue( driveFrictionTorque ) );
        WP_ASSERT( Math<physics_Num>::Abs( driveFrictionTorque ) < MaxWheelTorque );
        WP_ASSERT( m_isPoweredWheel ||
                   Math<physics_Num>::Abs( driveFrictionTorque ) <= Math<physics_Num>::epsilon() );
        m_driveFrictionTorque = driveFrictionTorque;
    }

    physics_Num WheelControllerPacejka::getBrake() const
    {
        return m_brake;
    }

    void WheelControllerPacejka::setBrake( physics_Num brake )
    {
        WP_ASSERT( isFiniteValue( brake ) );
        WP_ASSERT( isUnitInput( brake ) );
        m_brake = brake;
    }

    physics_Num WheelControllerPacejka::getHandbrake() const
    {
        return m_handbrake;
    }

    void WheelControllerPacejka::setHandbrake( physics_Num handbrake )
    {
        WP_ASSERT( isFiniteValue( handbrake ) );
        WP_ASSERT( isUnitInput( handbrake ) );
        m_handbrake = handbrake;
    }

    physics_Num WheelControllerPacejka::getDrivetrainInertia() const
    {
        return m_drivetrainInertia;
    }

    void WheelControllerPacejka::setDrivetrainInertia( physics_Num drivetrainInertia )
    {
        WP_ASSERT( isFiniteValue( drivetrainInertia ) );
        WP_ASSERT( drivetrainInertia >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( m_isPoweredWheel || drivetrainInertia <= Math<physics_Num>::epsilon() );
        m_drivetrainInertia = drivetrainInertia;
    }

    physics_Num WheelControllerPacejka::getSuspensionForceInput() const
    {
        return m_suspensionForceInput;
    }

    void WheelControllerPacejka::setSuspensionForceInput( physics_Num suspensionForceInput )
    {
        WP_ASSERT( isFiniteValue( suspensionForceInput ) );
        WP_ASSERT( Math<physics_Num>::Abs( suspensionForceInput ) < MaxWheelForce );
        m_suspensionForceInput = suspensionForceInput;
    }

    physics_Num WheelControllerPacejka::getAngularVelocity() const
    {
        return m_angularVelocity;
    }

    void WheelControllerPacejka::setAngularVelocity( physics_Num angularVelocity )
    {
        WP_ASSERT( isFiniteValue( angularVelocity ) );
        WP_ASSERT( Math<physics_Num>::Abs( angularVelocity ) < MaxAngularVelocity );
        m_angularVelocity = angularVelocity;
    }

    physics_Num WheelControllerPacejka::getSlipRatio() const
    {
        return m_slipRatio;
    }

    void WheelControllerPacejka::setSlipRatio( physics_Num slipRatio )
    {
        WP_ASSERT( isFiniteValue( slipRatio ) );
        m_slipRatio = slipRatio;
    }

    physics_Num WheelControllerPacejka::getSlipVelo() const
    {
        return m_slipVelo;
    }

    void WheelControllerPacejka::setSlipVelo( physics_Num slipVelo )
    {
        WP_ASSERT( isFiniteValue( slipVelo ) );
        WP_ASSERT( slipVelo >= static_cast<physics_Num>( 0.0 ) );
        m_slipVelo = slipVelo;
    }

    physics_Num WheelControllerPacejka::getCompression() const
    {
        return m_compression;
    }

    void WheelControllerPacejka::setCompression( physics_Num compression )
    {
        assertCompressionValue( compression );
        m_compression = compression;
    }

    physics_Num WheelControllerPacejka::getFullCompressionSpringForce() const
    {
        return m_fullCompressionSpringForce;
    }

    void WheelControllerPacejka::setFullCompressionSpringForce( physics_Num fullCompressionSpringForce )
    {
        WP_ASSERT( isFiniteValue( fullCompressionSpringForce ) );
        WP_ASSERT( fullCompressionSpringForce >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( fullCompressionSpringForce < MaxWheelForce );
        m_fullCompressionSpringForce = fullCompressionSpringForce;
    }

    physics_Num WheelControllerPacejka::getRotation() const
    {
        return m_rotation;
    }

    void WheelControllerPacejka::setRotation( physics_Num rotation )
    {
        WP_ASSERT( isFiniteValue( rotation ) );
        m_rotation = rotation;
    }

    physics_Num WheelControllerPacejka::getNormalForce() const
    {
        return m_normalForce;
    }

    void WheelControllerPacejka::setNormalForce( physics_Num normalForce )
    {
        WP_ASSERT( isFiniteValue( normalForce ) );
        WP_ASSERT( normalForce >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( normalForce < MaxWheelForce );
        m_normalForce = normalForce;
    }

    physics_Num WheelControllerPacejka::getSlipAngle() const
    {
        return m_slipAngle;
    }

    void WheelControllerPacejka::setSlipAngle( physics_Num slipAngle )
    {
        WP_ASSERT( isFiniteValue( slipAngle ) );
        m_slipAngle = slipAngle;
    }

    physics_Num WheelControllerPacejka::getMaxSlip() const
    {
        return m_maxSlip;
    }

    void WheelControllerPacejka::setMaxSlip( physics_Num maxSlip )
    {
        WP_ASSERT( isFiniteValue( maxSlip ) );
        WP_ASSERT( maxSlip > static_cast<physics_Num>( 0.0 ) );
        m_maxSlip = maxSlip;
    }

    physics_Num WheelControllerPacejka::getMaxAngle() const
    {
        return m_maxAngle;
    }

    void WheelControllerPacejka::setMaxAngle( physics_Num maxAngle )
    {
        WP_ASSERT( isFiniteValue( maxAngle ) );
        WP_ASSERT( maxAngle > static_cast<physics_Num>( 0.0 ) );
        m_maxAngle = maxAngle;
    }

    physics_Num WheelControllerPacejka::getOldAngle() const
    {
        return m_oldAngle;
    }

    void WheelControllerPacejka::setOldAngle( physics_Num oldAngle )
    {
        WP_ASSERT( isFiniteValue( oldAngle ) );
        WP_ASSERT( Math<physics_Num>::Abs( oldAngle ) <= MaxSteeringAngle );
        m_oldAngle = oldAngle;
    }

    physics_Num WheelControllerPacejka::getChassisMass() const
    {
        return m_chassisMass;
    }

    void WheelControllerPacejka::setChassisMass( physics_Num chassisMass )
    {
        WP_ASSERT( isFiniteValue( chassisMass ) );
        WP_ASSERT( chassisMass >= static_cast<physics_Num>( 0.0 ) );
        m_chassisMass = chassisMass;
    }

    int WheelControllerPacejka::getLastSkid() const
    {
        return m_lastSkid;
    }

    void WheelControllerPacejka::setLastSkid( int lastSkid )
    {
        m_lastSkid = lastSkid;
    }

    bool WheelControllerPacejka::isOnGround() const
    {
        return m_onGround;
    }

    void WheelControllerPacejka::setOnGround( bool onGround )
    {
        m_onGround = onGround;
    }

    const Array<physics_Num> &WheelControllerPacejka::getPacejkaA() const
    {
        return m_pacejkaA;
    }

    void WheelControllerPacejka::setPacejkaA( const Array<physics_Num> &pacejkaA )
    {
        WP_ASSERT( pacejkaA.size() >= 15 );
        for( auto value : pacejkaA )
        {
            WP_ASSERT( isFiniteValue( value ) );
        }
        m_pacejkaA = pacejkaA;
    }

    const Array<physics_Num> &WheelControllerPacejka::getPacejkaB() const
    {
        return m_pacejkaB;
    }

    void WheelControllerPacejka::setPacejkaB( const Array<physics_Num> &pacejkaB )
    {
        WP_ASSERT( pacejkaB.size() >= 11 );
        for( auto value : pacejkaB )
        {
            WP_ASSERT( isFiniteValue( value ) );
        }
        m_pacejkaB = pacejkaB;
    }

    TireModel WheelControllerPacejka::getTireModel() const
    {
        return TireModel::Pacejka; // This implementation only supports Pacejka model
    }

    void WheelControllerPacejka::setTireModel( TireModel tireModel )
    {
        // WP_ASSERT( tireModel == TireModel::Pacejka );
        if( tireModel != TireModel::Pacejka )
        {
            WP_LOG_ERROR( "WheelControllerPacejka only supports the Pacejka tire model." );
        }
    }

    SmartPtr<Properties> WheelControllerPacejka::getProperties() const
    {
        auto properties = CVehicleComponent<IWheelComponent>::getProperties();
        WP_ASSERT( properties );

        properties->setProperty( "Radius", getRadius() );
        properties->setProperty( "Suspension Travel", getSuspensionTravel() );
        properties->setProperty( "Damping", getDamping() );
        properties->setProperty( "Inertia", getInertia() );
        properties->setProperty( "Grip", getGrip() );
        properties->setProperty( "Brake Friction Torque", getBrakeFrictionTorque() );
        properties->setProperty( "Handbrake Friction Torque", getHandbrakeFrictionTorque() );
        properties->setProperty( "Friction Torque", getFrictionTorque() );
        properties->setProperty( "Max Steering Angle", getMaxSteeringAngle() );
        properties->setProperty( "Mass Fraction", getMassFraction() );
        properties->setProperty( "Brake", getBrake() );
        properties->setProperty( "Handbrake", getHandbrake() );
        properties->setProperty( "Angular Velocity", getAngularVelocity() );
        properties->setProperty( "Compression", getCompression(), true );
        properties->setProperty( "Spring Rate", getSpringRate() );
        properties->setProperty( "Suspension Distance", getSuspensionDistance() );
        properties->setProperty( "Steering Angle", getSteeringAngle() );
        properties->setProperty( "Steering Wheel", isSteeringWheel() );
        properties->setProperty( "Powered Wheel", isPoweredWheel() );

        return properties;
    }

    void WheelControllerPacejka::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            WP_LOG_ERROR( "WheelControllerPacejka::setProperties received null properties." );
            return;
        }

        CVehicleComponent<IWheelComponent>::setProperties( properties );

        auto radius = getRadius();
        auto suspensionTravel = getSuspensionTravel();
        auto damping = getDamping();
        auto inertia = getInertia();
        auto grip = getGrip();
        auto brakeFrictionTorque = getBrakeFrictionTorque();
        auto handbrakeFrictionTorque = getHandbrakeFrictionTorque();
        auto frictionTorque = getFrictionTorque();
        auto maxSteeringAngle = getMaxSteeringAngle();
        auto massFraction = getMassFraction();
        auto brake = getBrake();
        auto handbrake = getHandbrake();
        auto angularVelocity = getAngularVelocity();
        auto springRate = getSpringRate();
        auto suspensionDistance = getSuspensionDistance();
        auto steeringAngle = getSteeringAngle();
        auto steeringWheel = isSteeringWheel();
        auto poweredWheel = isPoweredWheel();

        properties->getPropertyValue( "Radius", radius );
        properties->getPropertyValue( "Suspension Travel", suspensionTravel );
        properties->getPropertyValue( "Damping", damping );
        properties->getPropertyValue( "Inertia", inertia );
        properties->getPropertyValue( "Grip", grip );
        properties->getPropertyValue( "Brake Friction Torque", brakeFrictionTorque );
        properties->getPropertyValue( "Handbrake Friction Torque", handbrakeFrictionTorque );
        properties->getPropertyValue( "Friction Torque", frictionTorque );
        properties->getPropertyValue( "Max Steering Angle", maxSteeringAngle );
        properties->getPropertyValue( "Mass Fraction", massFraction );
        properties->getPropertyValue( "Brake", brake );
        properties->getPropertyValue( "Handbrake", handbrake );
        properties->getPropertyValue( "Angular Velocity", angularVelocity );
        properties->getPropertyValue( "Spring Rate", springRate );
        properties->getPropertyValue( "Suspension Distance", suspensionDistance );
        properties->getPropertyValue( "Steering Angle", steeringAngle );
        properties->getPropertyValue( "Steering Wheel", steeringWheel );
        properties->getPropertyValue( "Powered Wheel", poweredWheel );

        setRadius( radius );
        setSuspensionTravel( suspensionTravel );
        setDamping( damping );
        setInertia( inertia );
        setGrip( grip );
        setBrakeFrictionTorque( brakeFrictionTorque );
        setHandbrakeFrictionTorque( handbrakeFrictionTorque );
        setFrictionTorque( frictionTorque );
        setMaxSteeringAngle( maxSteeringAngle );
        setMassFraction( massFraction );
        setBrake( brake );
        setHandbrake( handbrake );
        setAngularVelocity( angularVelocity );
        setSpringRate( springRate );
        setSuspensionDistance( suspensionDistance );
        setSteeringAngle( steeringAngle );
        setSteeringWheel( steeringWheel );
        setPoweredWheel( poweredWheel );
    }

    physics_Num WheelControllerPacejka::getMass() const
    {
        return m_chassisMass;
    }

    void WheelControllerPacejka::setMass( physics_Num mass )
    {
        WP_ASSERT( isFiniteValue( mass ) );
        WP_ASSERT( mass >= static_cast<physics_Num>( 0.0 ) );
        m_chassisMass = mass;
        updateCompressionForce();
    }

    physics_Num WheelControllerPacejka::getRadius() const
    {
        return m_radius;
    }

    void WheelControllerPacejka::setRadius( physics_Num radius )
    {
        WP_ASSERT( isFiniteValue( radius ) );
        WP_ASSERT( radius > static_cast<physics_Num>( 0.0 ) );
        m_radius = radius;
    }

    physics_Num WheelControllerPacejka::getSuspensionTravel() const
    {
        return m_suspensionTravel;
    }

    void WheelControllerPacejka::setSuspensionTravel( physics_Num suspensionTravel )
    {
        WP_ASSERT( isFiniteValue( suspensionTravel ) );
        WP_ASSERT( suspensionTravel > static_cast<physics_Num>( 0.0 ) );
        m_suspensionTravel = suspensionTravel;
    }

    physics_Num WheelControllerPacejka::getDamping() const
    {
        return m_damping;
    }

    void WheelControllerPacejka::setDamping( physics_Num damping )
    {
        WP_ASSERT( isFiniteValue( damping ) );
        WP_ASSERT( damping >= static_cast<physics_Num>( 0.0 ) );
        m_damping = damping;
    }

    physics_Num WheelControllerPacejka::getSpringRate() const
    {
        return m_springRate;
    }

    void WheelControllerPacejka::setSpringRate( physics_Num springRate )
    {
        WP_ASSERT( isFiniteValue( springRate ) );
        WP_ASSERT( springRate >= static_cast<physics_Num>( 0.0 ) );
        m_springRate = springRate;
    }

    physics_Num WheelControllerPacejka::getSuspensionDistance() const
    {
        return m_suspensionDistance;
    }

    void WheelControllerPacejka::setSuspensionDistance( physics_Num suspensionDistance )
    {
        WP_ASSERT( isFiniteValue( suspensionDistance ) );
        WP_ASSERT( suspensionDistance >= static_cast<physics_Num>( 0.0 ) );
        m_suspensionDistance = suspensionDistance;
    }

    physics_Num WheelControllerPacejka::getSteeringAngle() const
    {
        return m_steeringAngle;
    }

    void WheelControllerPacejka::setSteeringAngle( physics_Num steeringAngle )
    {
        WP_ASSERT( isFiniteValue( steeringAngle ) );
        WP_ASSERT( Math<physics_Num>::Abs( steeringAngle ) <= MaxSteeringAngle );
        m_steeringAngle = steeringAngle;
    }

    bool WheelControllerPacejka::isSteeringWheel() const
    {
        return m_isSteeringWheel;
    }

    void WheelControllerPacejka::setSteeringWheel( bool isSteeringWheel )
    {
        m_isSteeringWheel = isSteeringWheel;
    }

    void WheelControllerPacejka::addTorque( physics_Num torque )
    {
    }

    void WheelControllerPacejka::setTorque( physics_Num torque )
    {
    }

    physics_Num WheelControllerPacejka::getTorque() const
    {
        return 0.0f;
    }

    Vector3<physics_Num> WheelControllerPacejka::getLocalPosition() const
    {
        return m_localPosition;
    }

    void WheelControllerPacejka::setLocalPosition( const Vector3<physics_Num> &localPosition )
    {
        WP_ASSERT( isFiniteVector( localPosition ) );
        m_localPosition = localPosition;
    }

    Vector3<physics_Num> WheelControllerPacejka::getWorldPosition()
    {
        // auto vehicle = getOwner();
        // auto t = vehicle->getWorldTransform();
        ////return t->transformPoint(localPosition);
        // return (t->getOrientation() * localPosition) + t->getPosition();

        return m_worldTransform.getPosition();
    }

    void WheelControllerPacejka::updateWheel( const int &task, const double &t, const double &dt )
    {
        WP_ASSERT( Math<real_Num>::isFinite( static_cast<real_Num>( t ) ) );
        WP_ASSERT( Math<real_Num>::isFinite( static_cast<real_Num>( dt ) ) );
        WP_ASSERT( dt >= static_cast<double>( 0.0 ) );
        WP_ASSERT( dt < static_cast<double>( 1.0 ) );
        WP_ASSERT( task >= 0 );
        WP_ASSERT( m_hit );
        if( !m_hit )
        {
            return;
        }
        assertWheelScalarInputs( *this );

        initSlipMaxima();         // hack
        updateCompressionForce(); // hack

        auto vehicle = getOwner();
        WP_ASSERT( vehicle );
        if( !vehicle )
        {
            return; // todo work around
        }

        auto vehicleLocalTransform = vehicle->getLocalTransform();
        auto vehicleWorldTransform = vehicle->getWorldTransform();
        auto body = vehicle->getBody();
        WP_ASSERT( vehicleLocalTransform.isFinite() );
        WP_ASSERT( vehicleWorldTransform.isFinite() );
        WP_ASSERT( body );
        if( !body )
        {
            return;
        }

        // WP_ASSERT(vehicleLocalTransform);
        // WP_ASSERT(vehicleWorldTransform);
        // WP_ASSERT(body);

        auto vehiclePos = vehicleWorldTransform.getPosition();
        WP_ASSERT( isFiniteVector( vehiclePos ) );

        auto localPos = m_localTransform.getPosition();
        auto worldPos = m_worldTransform.getPosition();
        WP_ASSERT( isFiniteVector( localPos ) );
        WP_ASSERT( isFiniteVector( worldPos ) );
        WP_ASSERT( m_localTransform.isFinite() );
        WP_ASSERT( m_worldTransform.isFinite() );
        // m_up = vehicleWorldTransform.up();
        m_up = Vector3<physics_Num>::unitY();
        WP_ASSERT( isFiniteVector( m_up ) );
        WP_ASSERT( m_up.lengthSquared() > Math<physics_Num>::epsilon() );

        const auto wasOnGround = m_onGround;
        m_onGround = false;
        m_normalForce = static_cast<physics_Num>( 0.0 );

        auto dist = static_cast<physics_Num>( 0.0 );      // m_suspensionTravel + m_radius;
        auto rayOffset = static_cast<physics_Num>( 0.0 ); // m_compression - 1.0f;
        auto ray = Ray3<physics_Num>( localPos + ( m_up * dist ), -m_up );
        WP_ASSERT( isFiniteValue( dist ) );
        WP_ASSERT( isFiniteValue( rayOffset ) );
        WP_ASSERT( ray.isValid() );

        if( body->castLocalRay( ray, m_hit ) )
        {
            auto hitDistance = m_hit->getDistance();
            WP_ASSERT( isFiniteValue( hitDistance ) );
            WP_ASSERT( hitDistance >= static_cast<physics_Num>( 0.0 ) );

            if( hitDistance > MinRayHitDistance )
            {
                if( hitDistance < rayOffset + ( m_suspensionTravel + m_radius ) )
                {
                    // WP_ASSERT( worldPos.Y() < static_cast<physics_Num>(3.0) );
                    m_onGround = true;
                    dist = hitDistance;
                    WP_ASSERT( isFiniteValue( dist ) );
                }
            }
        }

#if !WP_FINAL
        if( vehicle->getDisplayDebugData() )
        {
            auto       rayLineId = getDebugId( 0 );
            const auto debugDistance = m_onGround ? dist : m_suspensionTravel + m_radius;
            DEBUG_DRAW_LINE_BY_ID( rayLineId, m_worldTransform.getPosition() + ( m_up * dist ),
                                   ( m_worldTransform.getPosition() + ( m_up * dist ) ) +
                                       ( -m_worldTransform.up() * debugDistance ),
                                   0xFF );
            DEBUG_DRAW_LINE_BY_ID( getDebugId( 1 ), m_worldTransform.getPosition() + ( m_up * 0 ),
                                   ( m_worldTransform.getPosition() + ( m_up * 0 ) ) +
                                       ( m_worldTransform.forward() * m_radius ),
                                   0xFF );
        }
#endif

        auto localPosition = Vector3F::zero();
        auto localRotation = QuaternionF::identity();
        WP_ASSERT( localPosition.isFinite() );
        WP_ASSERT( localRotation.isFinite() );

        if( m_onGround )
        {
            const auto firstContact = !wasOnGround;

            auto hitPoint = m_hit->getPoint();
            WP_ASSERT( isFiniteVector( hitPoint ) );

            auto hitNormal = m_hit->getNormal();
            WP_ASSERT( isFiniteVector( hitNormal ) );
            WP_ASSERT( hitNormal.lengthSquared() > Math<physics_Num>::epsilon() );
            m_groundNormal =
                m_worldTransform.inverseTransformVector( m_inverseLocalRotation * hitNormal );
            WP_ASSERT( isFiniteVector( m_groundNormal ) );
            WP_ASSERT( m_groundNormal.lengthSquared() > Math<physics_Num>::epsilon() );

            auto hitDistance = m_hit->getDistance() - rayOffset;
            WP_ASSERT( isFiniteValue( hitDistance ) );

            // Protect against invalid configuration where suspension travel is zero
            if( Math<physics_Num>::Abs( m_suspensionTravel ) < Math<physics_Num>::epsilon() )
            {
                WP_LOG_WARNING(
                    "WheelControllerPacejka::updateWheel: m_suspensionTravel is zero or near-zero; "
                    "using compression=0." );
                m_compression = static_cast<real_Num>( 0.0 );
            }
            else
            {
                auto compressionInv = ( hitDistance - m_radius ) / m_suspensionTravel;
                WP_ASSERT( isFiniteValue( compressionInv ) );
                // Compute compression and clamp to allowed range to protect against
                // numerical error or invalid configuration producing out-of-range values.
                m_compression = static_cast<real_Num>( 1.0 ) - compressionInv;
                m_compression = Math<real_Num>::clamp( m_compression, static_cast<real_Num>( 0.0 ),
                                                       static_cast<real_Num>( 1.0 ) );
            }

            assertCompressionValue( m_compression );

            auto worldPosition = m_worldTransform.getPosition();
            WP_ASSERT( isFiniteVector( worldPosition ) );
            m_wheelVelo = body->getPointVelocity( worldPosition );
            assertVelocityVector( m_wheelVelo );

            m_localVelo = vehicleWorldTransform.inverseTransformVector( m_wheelVelo );
            assertVelocityVector( m_localVelo );

            if( firstContact )
            {
                // On scene load or landing, do not feed stale/startup contact velocity into the
                // damper and tire model. The next physics tick has a settled contact history.
                m_wheelVelo = Vector3<physics_Num>::zero();
                m_localVelo = Vector3<physics_Num>::zero();
                m_slipRatio = static_cast<physics_Num>( 0.0 );
                m_slipAngle = static_cast<physics_Num>( 0.0 );
                m_slipVelo = static_cast<physics_Num>( 0.0 );
            }

            m_suspensionForce = suspensionForce();
            m_roadForce = firstContact ? Vector3<physics_Num>::zero()
                                       : roadForce( static_cast<physics_Num>( dt ) );
            assertForceVector( m_suspensionForce );
            assertForceVector( m_roadForce );

            auto localSuspensionForce =
                vehicleWorldTransform.inverseTransformVector( m_suspensionForce );
            auto localRoadForce = vehicleWorldTransform.inverseTransformVector( m_roadForce );
            assertForceVector( localSuspensionForce );
            assertForceVector( localRoadForce );

            body->addLocalForceAtLocalPosition( localSuspensionForce, localPos );
            body->addLocalForceAtLocalPosition( localRoadForce, localPos );
        }
        else
        {
            m_compression = static_cast<physics_Num>( 0.0 );
            m_normalForce = static_cast<physics_Num>( 0.0 );
            m_suspensionForce = Vector3<physics_Num>::zero();
            m_roadForce = Vector3<physics_Num>::zero();

            auto totalInertia = m_inertia + m_drivetrainInertia;
            WP_ASSERT( isFiniteValue( totalInertia ) );
            WP_ASSERT( totalInertia > static_cast<physics_Num>( 0.0 ) );
            auto driveAngularDelta = m_driveTorque * static_cast<physics_Num>( dt ) / totalInertia;
            auto totalFrictionTorque = m_brakeFrictionTorque * m_brake +
                                       m_handbrakeFrictionTorque * m_handbrake + m_frictionTorque +
                                       m_driveFrictionTorque;
            auto frictionAngularDelta =
                totalFrictionTorque * static_cast<physics_Num>( dt ) / totalInertia;
            WP_ASSERT( isFiniteValue( driveAngularDelta ) );
            WP_ASSERT( isFiniteValue( totalFrictionTorque ) );
            WP_ASSERT( totalFrictionTorque >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( frictionAngularDelta ) );
            WP_ASSERT( frictionAngularDelta >= static_cast<physics_Num>( 0.0 ) );
            m_angularVelocity += driveAngularDelta;
            WP_ASSERT( isFiniteValue( m_angularVelocity ) );
            WP_ASSERT( Math<physics_Num>::Abs( m_angularVelocity ) < MaxAngularVelocity );

            if( Math<physics_Num>::Abs( m_angularVelocity ) > frictionAngularDelta )
            {
                m_angularVelocity -= frictionAngularDelta * Math<physics_Num>::Sign( m_angularVelocity );
            }
            else
            {
                m_angularVelocity = 0;
            }
            WP_ASSERT( isFiniteValue( m_angularVelocity ) );
            WP_ASSERT( Math<physics_Num>::Abs( m_angularVelocity ) < MaxAngularVelocity );

            m_slipRatio = 0;
            m_slipVelo = 0;
            m_slipAngle = 0;
        }

        assertCompressionValue( m_compression );
        assertForceVector( m_suspensionForce );
        assertForceVector( m_roadForce );
        WP_ASSERT( isFiniteValue( m_normalForce ) );
        WP_ASSERT( m_normalForce >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_angularVelocity ) );
        WP_ASSERT( isFiniteValue( m_slipRatio ) );
        WP_ASSERT( isFiniteValue( m_slipAngle ) );
        WP_ASSERT( isFiniteValue( m_slipVelo ) );
        WP_ASSERT( m_slipVelo >= static_cast<physics_Num>( 0.0 ) );
    }
} // namespace workphone

#undef WP_ASSERT
