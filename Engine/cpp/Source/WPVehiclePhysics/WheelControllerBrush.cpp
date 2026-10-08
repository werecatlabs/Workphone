#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/VehicleHandling.hpp>
#include <WPVehiclePhysics/WheelControllerBrush.hpp>
#include <Workphone/Workphone.hpp>
#include <limits>

namespace workphone
{
    namespace
    {
        constexpr auto MaxWheelForce = static_cast<physics_Num>( 1.0e8 );
        constexpr auto MaxWheelVelocity = static_cast<physics_Num>( 1.0e5 );
        constexpr auto MaxAngularVelocity = static_cast<physics_Num>( 1.0e5 );
        constexpr auto MaxWheelTorque = static_cast<physics_Num>( 1.0e8 );
        constexpr auto MaxWheelStiffness = static_cast<physics_Num>( 1.0e8 );
        constexpr auto MaxSteeringAngle = static_cast<physics_Num>( 1080.0 );
        constexpr auto MaxSlipRatio = static_cast<physics_Num>( 3.0 );
        constexpr auto MaxSlipAngle = static_cast<physics_Num>( 1.25 );
        constexpr auto MinSlipSpeed = static_cast<physics_Num>( 0.5 );
        constexpr auto CompressionTolerance = static_cast<physics_Num>( 0.05 );

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

        physics_Num clampNonNegativeFinite( physics_Num value )
        {
            if( !isFiniteValue( value ) || value < static_cast<physics_Num>( 0.0 ) )
            {
                return static_cast<physics_Num>( 0.0 );
            }

            return value;
        }

        physics_Num clampUnitFinite( physics_Num value )
        {
            if( !isFiniteValue( value ) )
            {
                return static_cast<physics_Num>( 0.0 );
            }

            return Math<physics_Num>::clamp( value, static_cast<physics_Num>( 0.0 ),
                                             static_cast<physics_Num>( 1.0 ) );
        }

        physics_Num clampSignedFinite( physics_Num value, physics_Num limit )
        {
            if( !isFiniteValue( value ) )
            {
                return static_cast<physics_Num>( 0.0 );
            }

            return Math<physics_Num>::clamp( value, -limit, limit );
        }

#if !WP_FINAL
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

        void assertBrushConfig( const WheelControllerBrush &wheel )
        {
            WP_ASSERT( isFiniteValue( wheel.getRadius() ) );
            WP_ASSERT( wheel.getRadius() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getSuspensionTravel() ) );
            WP_ASSERT( wheel.getSuspensionTravel() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getSuspensionDistance() ) );
            WP_ASSERT( wheel.getSuspensionDistance() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getSpringRate() ) );
            WP_ASSERT( wheel.getSpringRate() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( wheel.getSpringRate() < MaxWheelStiffness );
            WP_ASSERT( isFiniteValue( wheel.getDamping() ) );
            WP_ASSERT( wheel.getDamping() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getMassFraction() ) );
            WP_ASSERT( wheel.getMassFraction() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( wheel.getMassFraction() <= static_cast<physics_Num>( 1.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getInertia() ) );
            WP_ASSERT( wheel.getInertia() > static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getGrip() ) );
            WP_ASSERT( wheel.getGrip() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getStaticFrictionCoefficient() ) );
            WP_ASSERT( wheel.getStaticFrictionCoefficient() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( isFiniteValue( wheel.getSlidingFrictionCoefficient() ) );
            WP_ASSERT( wheel.getSlidingFrictionCoefficient() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( wheel.getSlidingFrictionCoefficient() <=
                       wheel.getStaticFrictionCoefficient() + Math<physics_Num>::epsilon() );
            WP_ASSERT( isFiniteValue( wheel.getLongitudinalStiffness() ) );
            WP_ASSERT( wheel.getLongitudinalStiffness() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( wheel.getLongitudinalStiffness() < MaxWheelStiffness );
            WP_ASSERT( isFiniteValue( wheel.getLateralStiffness() ) );
            WP_ASSERT( wheel.getLateralStiffness() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( wheel.getLateralStiffness() < MaxWheelStiffness );
            WP_ASSERT( isFiniteValue( wheel.getBrakeFrictionTorque() ) );
            WP_ASSERT( wheel.getBrakeFrictionTorque() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( wheel.getBrakeFrictionTorque() < MaxWheelTorque );
            WP_ASSERT( isFiniteValue( wheel.getHandbrakeFrictionTorque() ) );
            WP_ASSERT( wheel.getHandbrakeFrictionTorque() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( wheel.getHandbrakeFrictionTorque() < MaxWheelTorque );
            WP_ASSERT( isFiniteValue( wheel.getRollingResistanceTorque() ) );
            WP_ASSERT( wheel.getRollingResistanceTorque() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( wheel.getRollingResistanceTorque() < MaxWheelTorque );
            WP_ASSERT( isFiniteValue( wheel.getTorque() ) );
            WP_ASSERT( Math<physics_Num>::Abs( wheel.getTorque() ) < MaxWheelTorque );
            WP_ASSERT( isUnitInput( wheel.getBrake() ) );
            WP_ASSERT( isUnitInput( wheel.getHandbrake() ) );
            WP_ASSERT( isFiniteValue( wheel.getSteeringAngle() ) );
            WP_ASSERT( Math<physics_Num>::Abs( wheel.getSteeringAngle() ) <= MaxSteeringAngle );
            WP_ASSERT( isFiniteValue( wheel.getAngularVelocity() ) );
            WP_ASSERT( Math<physics_Num>::Abs( wheel.getAngularVelocity() ) < MaxAngularVelocity );
        }

        void assertBrushOutput( const WheelControllerBrush &wheel )
        {
            assertCompressionValue( wheel.getCompression() );
            WP_ASSERT( isFiniteValue( wheel.getNormalForce() ) );
            WP_ASSERT( wheel.getNormalForce() >= static_cast<physics_Num>( 0.0 ) );
            WP_ASSERT( wheel.getNormalForce() < MaxWheelForce );
            WP_ASSERT( isFiniteValue( wheel.getSlipRatio() ) );
            WP_ASSERT( Math<physics_Num>::Abs( wheel.getSlipRatio() ) <=
                       MaxSlipRatio + Math<physics_Num>::epsilon() );
            WP_ASSERT( isFiniteValue( wheel.getSlipAngle() ) );
            WP_ASSERT( Math<physics_Num>::Abs( wheel.getSlipAngle() ) <=
                       MaxSlipAngle + Math<physics_Num>::epsilon() );
            WP_ASSERT( isFiniteValue( wheel.getSlipVelo() ) );
            WP_ASSERT( wheel.getSlipVelo() >= static_cast<physics_Num>( 0.0 ) );
            assertVelocityVector( wheel.getWheelVelo() );
            assertVelocityVector( wheel.getLocalVelo() );
            assertForceVector( wheel.getSuspensionForceVector() );
            assertForceVector( wheel.getRoadForceVector() );

            if( !wheel.isOnGround() )
            {
                WP_ASSERT( wheel.getCompression() <= Math<physics_Num>::epsilon() );
                WP_ASSERT( wheel.getNormalForce() <= Math<physics_Num>::epsilon() );
                WP_ASSERT( wheel.getSuspensionForceVector().length() <= Math<physics_Num>::epsilon() );
                WP_ASSERT( wheel.getRoadForceVector().length() <= Math<physics_Num>::epsilon() );
            }
        }
#endif
    } // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, WheelControllerBrush, CVehicleComponent<IWheelComponent> );

    WheelControllerBrush::WheelControllerBrush()
    {
        m_hit = workphone::make_ptr<physics::RaycastHit>();
        m_hit->setCheckDynamic( false );
        m_hit->setCheckStatic( true );
    }

    WheelControllerBrush::~WheelControllerBrush() = default;

    void WheelControllerBrush::reset()
    {
        m_angularVelocity = 0.0f;
        m_driveTorque = 0.0f;
        m_brake = 0.0f;
        m_handbrake = 0.0f;
        m_steeringAngle = 0.0f;
        m_onGround = false;
        m_compression = 0.0f;
        m_normalForce = 0.0f;
        m_slipRatio = 0.0f;
        m_slipAngle = 0.0f;
        m_slipVelo = 0.0f;
        m_wheelVelo = Vector3<physics_Num>::zero();
        m_localVelo = Vector3<physics_Num>::zero();
        m_suspensionForce = Vector3<physics_Num>::zero();
        m_roadForce = Vector3<physics_Num>::zero();
    }

    void WheelControllerBrush::update()
    {
        if( Thread::getCurrentTask() != TaskId::Physics )
        {
            return;
        }

        const auto state = getState();
        if( state != State::AWAKE && state != State::EDIT && state != State::PLAY )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
#if !WP_FINAL
        WP_ASSERT( applicationManager );
#endif
        if( !applicationManager )
        {
            return;
        }

        auto timer = applicationManager->getTimerPtr();
#if !WP_FINAL
        WP_ASSERT( timer );
#endif
        if( !timer )
        {
            return;
        }

        auto dt = static_cast<physics_Num>( timer->getDeltaTime() );
        if( dt > static_cast<physics_Num>( 1.0 / 30.0 ) )
        {
            dt = static_cast<physics_Num>( 1.0 / 30.0 );
        }

#if !WP_FINAL
        WP_ASSERT( isValid() );
        WP_ASSERT( isFiniteValue( dt ) );
        WP_ASSERT( dt >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( dt < static_cast<physics_Num>( 1.0 ) );
        assertBrushConfig( *this );
#endif

        if( !isFiniteValue( dt ) || dt < static_cast<physics_Num>( 0.0 ) )
        {
            return;
        }

        updateTransform();
        updateWheel( dt );
    }

    void WheelControllerBrush::updateWheel( physics_Num dt )
    {
#if !WP_FINAL
        WP_ASSERT( m_hit );
        WP_ASSERT( isFiniteValue( dt ) );
        WP_ASSERT( dt >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( dt < static_cast<physics_Num>( 1.0 ) );
        WP_ASSERT( m_localTransform.isFinite() );
        WP_ASSERT( m_worldTransform.isFinite() );
        assertBrushConfig( *this );
#endif

        m_onGround = false;
        m_compression = static_cast<physics_Num>( 0.0 );
        m_normalForce = static_cast<physics_Num>( 0.0 );
        m_suspensionForce = Vector3<physics_Num>::zero();
        m_roadForce = Vector3<physics_Num>::zero();

        auto vehicle = getOwner();
#if !WP_FINAL
        WP_ASSERT( vehicle );
#endif
        if( !vehicle )
        {
            integrateFreeSpin( dt );
            return;
        }

        auto body = vehicle->getBody();
#if !WP_FINAL
        WP_ASSERT( body );
#endif
        if( !body )
        {
            integrateFreeSpin( dt );
            return;
        }

        const auto vehicleWorldTransform = vehicle->getWorldTransform();
        const auto localPos = m_localTransform.getPosition();
        const auto worldPos = m_worldTransform.getPosition();
        auto       up = vehicleWorldTransform.up();

#if !WP_FINAL
        WP_ASSERT( vehicleWorldTransform.isFinite() );
        WP_ASSERT( isFiniteVector( localPos ) );
        WP_ASSERT( isFiniteVector( worldPos ) );
        WP_ASSERT( isFiniteVector( up ) );
        WP_ASSERT( up.lengthSquared() > Math<physics_Num>::epsilon() );
#endif

        if( !isFiniteVector( up ) || up.lengthSquared() <= Math<physics_Num>::epsilon() )
        {
            up = Vector3<physics_Num>::unitY();
        }

        const auto steeringRotation = Quaternion<physics_Num>::eulerDegrees(
            static_cast<physics_Num>( 0.0 ), -m_steeringAngle, static_cast<physics_Num>( 0.0 ) );
        const auto inverseSteeringRotation = steeringRotation.inverse();

        m_wheelVelo = body->getPointVelocity( worldPos );
        const auto vehicleLocalVelo = vehicleWorldTransform.inverseTransformVector( m_wheelVelo );
        m_localVelo = inverseSteeringRotation * vehicleLocalVelo;

#if !WP_FINAL
        WP_ASSERT( MathUtil<physics_Num>::isFinite( steeringRotation ) );
        WP_ASSERT( MathUtil<physics_Num>::isFinite( inverseSteeringRotation ) );
        assertVelocityVector( m_wheelVelo );
        assertVelocityVector( vehicleLocalVelo );
        assertVelocityVector( m_localVelo );
#endif

        const auto ray = Ray3<physics_Num>( worldPos, -up );
#if !WP_FINAL
        WP_ASSERT( ray.isValid() );
#endif

        physics_Num hitDistance = static_cast<physics_Num>( 0.0 );
        if( body->castWorldRay( ray, m_hit ) )
        {
            hitDistance = m_hit->getDistance();
#if !WP_FINAL
            WP_ASSERT( isFiniteValue( hitDistance ) );
            WP_ASSERT( hitDistance >= static_cast<physics_Num>( 0.0 ) );
#endif

            if( hitDistance > std::numeric_limits<physics_Num>::epsilon() &&
                hitDistance < m_suspensionTravel + m_radius )
            {
                m_onGround = true;
            }
        }

#if !WP_FINAL
        if( vehicle->getDisplayDebugData() )
        {
            DEBUG_DRAW_LINE_BY_ID( getDebugId( 0 ), worldPos,
                                   worldPos - up * ( m_suspensionTravel + m_radius ), 0xFF );
            DEBUG_DRAW_LINE_BY_ID( getDebugId( 1 ), worldPos,
                                   worldPos + vehicleWorldTransform.forward() * m_radius, 0xFF );
        }
#endif

        if( !m_onGround )
        {
            integrateFreeSpin( dt );
#if !WP_FINAL
            assertBrushOutput( *this );
#endif
            return;
        }

        const auto vehicleMass = body->getMass();
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( vehicleMass ) );
        WP_ASSERT( vehicleMass > static_cast<physics_Num>( 0.0 ) );
#endif
        if( !isFiniteValue( vehicleMass ) || vehicleMass <= static_cast<physics_Num>( 0.0 ) )
        {
            integrateFreeSpin( dt );
            return;
        }

        auto compressionDistance = m_suspensionTravel - ( hitDistance - m_radius );
        compressionDistance = Math<physics_Num>::clamp(
            compressionDistance, static_cast<physics_Num>( 0.0 ), m_suspensionTravel );
        if( m_suspensionTravel > Math<physics_Num>::epsilon() )
        {
            m_compression = compressionDistance / m_suspensionTravel;
        }
        else
        {
            m_compression = static_cast<physics_Num>( 0.0 );
        }

        const auto verticalVelocity = m_wheelVelo.dotProduct( up );
        const auto springForce = compressionDistance * m_springRate;
        const auto sprungWeightForce = vehicleMass * m_massFraction * static_cast<physics_Num>( 2.0 ) *
                                       static_cast<physics_Num>( 9.81 ) * m_compression;
        const auto damperForce = -verticalVelocity * m_damping;
        auto suspensionMagnitude = springForce + sprungWeightForce + damperForce;
        if( m_implicitSuspension && dt > 0 )
        {
            // Backward Euler for a sprung corner. Retains the static spring rate while
            // avoiding an explicit spring/damper instability at low render frame rates.
            const auto sprungMass =
                Math<physics_Num>::max( vehicleMass * m_massFraction, static_cast<physics_Num>( 1.0 ) );
            // Allow for coupled contacts while retaining spring and damper response.
            // Excessively reducing this mass makes stiff race suspension floaty.
            const auto effectiveMass =
                ( m_contactEffectiveMass > 0 ? m_contactEffectiveMass : sprungMass ) /
                static_cast<physics_Num>( 2 );
            const auto preloadRate = m_suspensionTravel > Math<physics_Num>::epsilon()
                                         ? vehicleMass * m_massFraction *
                                               static_cast<physics_Num>( 19.62 ) / m_suspensionTravel
                                         : 0;
            const auto wheelRate = m_springRate + preloadRate;
            const auto response = m_damping * dt + wheelRate * dt * dt;
            suspensionMagnitude =
                ( springForce + sprungWeightForce - ( m_damping + wheelRate * dt ) * verticalVelocity +
                  response * sprungMass / effectiveMass * m_contactAcceleration ) /
                ( static_cast<physics_Num>( 1.0 ) + response / effectiveMass );
        }
        suspensionMagnitude = clampNonNegativeFinite( suspensionMagnitude );

        m_normalForce = suspensionMagnitude;
        auto contactNormal = up;
        if( m_implicitSuspension )
        {
            contactNormal = m_hit->getNormal();
            if( contactNormal.lengthSquared() < Math<physics_Num>::epsilon() )
                contactNormal = Vector3<physics_Num>::unitY();
            contactNormal.normalise();
        }
        m_suspensionForce = contactNormal * suspensionMagnitude;

#if !WP_FINAL
        WP_ASSERT( isFiniteValue( compressionDistance ) );
        WP_ASSERT( compressionDistance >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( compressionDistance <= m_suspensionTravel + Math<physics_Num>::epsilon() );
        assertCompressionValue( m_compression );
        WP_ASSERT( isFiniteValue( verticalVelocity ) );
        WP_ASSERT( isFiniteValue( springForce ) );
        WP_ASSERT( springForce >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( sprungWeightForce ) );
        WP_ASSERT( sprungWeightForce >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( damperForce ) );
        WP_ASSERT( isFiniteValue( suspensionMagnitude ) );
        WP_ASSERT( suspensionMagnitude >= static_cast<physics_Num>( 0.0 ) );
        assertForceVector( m_suspensionForce );
#endif

        if( m_inertia > Math<physics_Num>::epsilon() )
        {
            auto driveTorque = m_driveTorque;
            if( m_tractionControl && isPoweredWheel() )
            {
                const auto lateralDemand =
                    m_lateralStiffness * m_grip *
                    std::atan2( m_localVelo.X(),
                                std::max( std::abs( m_localVelo.Z() ), physics_Num( 3 ) ) );
                driveTorque =
                    handling::tractionTorque( driveTorque, m_normalForce, m_staticFrictionCoefficient,
                                              m_grip, lateralDemand, m_radius );
            }
            if( m_tractionControl && isPoweredWheel() )
                driveTorque = handling::slipLimitedTorque( driveTorque, m_angularVelocity,
                                                           -m_localVelo.Z(), m_radius );
            m_angularVelocity += ( driveTorque * dt ) / m_inertia;
        }

        auto serviceBrakeTorque = m_brakeFrictionTorque * m_brake;
        if( m_antiLockBrakes )
            serviceBrakeTorque = handling::brakeTorque( serviceBrakeTorque, m_angularVelocity,
                                                        -m_localVelo.Z(), m_radius, m_inertia, dt );
        const auto brakeTorque =
            serviceBrakeTorque + m_handbrakeFrictionTorque * m_handbrake + m_rollingResistanceTorque;
        applyAngularFriction( brakeTorque, dt );

        m_angularVelocity =
            clampSignedFinite( m_angularVelocity, MaxAngularVelocity - static_cast<physics_Num>( 1.0 ) );

        const auto cornerMass = Math<physics_Num>::max( vehicleMass * m_massFraction, physics_Num( 1 ) );
        auto brushLocalForce = calculateBrushForce( m_normalForce, dt, cornerMass );
        if( m_implicitSuspension && dt > Math<physics_Num>::epsilon() && m_inertia > 0 )
        {
            // Limit tire impulses to the impulse that would bring the wheel and contact
            // patch to rolling speed. A stiff tire must not reverse slip every frame.
            const auto slipSpeed = m_angularVelocity * m_radius + m_localVelo.Z();
            const auto impulseForce = Math<physics_Num>::Abs( slipSpeed ) /
                                      ( dt * ( m_radius * m_radius / m_inertia +
                                               static_cast<physics_Num>( 1 ) / cornerMass ) );
            brushLocalForce.Z() =
                Math<physics_Num>::clamp( brushLocalForce.Z(), -impulseForce, impulseForce );
            const auto lateralLimit = Math<physics_Num>::Abs( m_localVelo.X() ) * cornerMass /
                                      ( static_cast<physics_Num>( 4 ) * dt );
            brushLocalForce.X() =
                Math<physics_Num>::clamp( brushLocalForce.X(), -lateralLimit, lateralLimit );
        }
        auto vehicleLocalRoadForce = steeringRotation * brushLocalForce;
        m_roadForce = vehicleWorldTransform.transformVector( vehicleLocalRoadForce );
        if( m_implicitSuspension )
        {
            m_roadForce -= contactNormal * m_roadForce.dotProduct( contactNormal );
            vehicleLocalRoadForce = vehicleWorldTransform.inverseTransformVector( m_roadForce );
        }

#if !WP_FINAL
        assertForceVector( brushLocalForce );
        assertForceVector( vehicleLocalRoadForce );
        assertForceVector( m_roadForce );
#endif

        if( m_inertia > Math<physics_Num>::epsilon() && m_radius > Math<physics_Num>::epsilon() )
        {
            const auto longitudinalTireForce = -brushLocalForce.Z();
#if !WP_FINAL
            WP_ASSERT( isFiniteValue( longitudinalTireForce ) );
            WP_ASSERT( Math<physics_Num>::Abs( longitudinalTireForce ) < MaxWheelForce );
#endif
            m_angularVelocity -= ( longitudinalTireForce * m_radius * dt ) / m_inertia;
            m_angularVelocity = clampSignedFinite(
                m_angularVelocity, MaxAngularVelocity - static_cast<physics_Num>( 1.0 ) );
        }

        const auto localSuspensionForce =
            vehicleWorldTransform.inverseTransformVector( m_suspensionForce );
#if !WP_FINAL
        assertForceVector( localSuspensionForce );
#endif

        body->addLocalForceAtLocalPosition( localSuspensionForce, localPos );
        body->addLocalForceAtLocalPosition( vehicleLocalRoadForce, localPos );

#if !WP_FINAL
        assertBrushOutput( *this );
#endif
    }

    Vector3<physics_Num> WheelControllerBrush::calculateBrushForce( physics_Num normalForce, physics_Num dt, physics_Num cornerMass )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( normalForce ) );
        WP_ASSERT( normalForce >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( normalForce < MaxWheelForce );
        assertBrushConfig( *this );
        assertVelocityVector( m_localVelo );
#endif

        m_slipRatio = static_cast<physics_Num>( 0.0 );
        m_slipAngle = static_cast<physics_Num>( 0.0 );
        m_slipVelo = static_cast<physics_Num>( 0.0 );

        if( normalForce <= Math<physics_Num>::epsilon() || m_radius <= Math<physics_Num>::epsilon() ||
            m_grip <= Math<physics_Num>::epsilon() )
        {
            return Vector3<physics_Num>::zero();
        }

        const auto longitudinalSpeed = -m_localVelo.Z();
        const auto lateralSpeed = m_localVelo.X();
        const auto absLongitudinalSpeed = Math<physics_Num>::Abs( longitudinalSpeed );
        const auto slipDenominator =
            absLongitudinalSpeed > MinSlipSpeed ? absLongitudinalSpeed : MinSlipSpeed;
        const auto wheelSurfaceSpeed = m_angularVelocity * m_radius;
        const auto longitudinalSlipSpeed = wheelSurfaceSpeed - longitudinalSpeed;

        m_slipRatio = Math<physics_Num>::clamp( longitudinalSlipSpeed / slipDenominator, -MaxSlipRatio,
                                                MaxSlipRatio );
        m_slipAngle = Math<physics_Num>::ATan2( lateralSpeed, slipDenominator );
        m_slipAngle = Math<physics_Num>::clamp( m_slipAngle, -MaxSlipAngle, MaxSlipAngle );
        m_slipVelo = Math<physics_Num>::Sqrt( longitudinalSlipSpeed * longitudinalSlipSpeed +
                                              lateralSpeed * lateralSpeed );

        auto requestedLongitudinalForce = m_longitudinalStiffness * m_grip * m_slipRatio;
        if( m_implicitSuspension && dt > 0 && m_inertia > 0 )
        {
            // Bound the demand before combined-slip saturation. Limiting only
            // the output let transient pre-contact wheel spin consume all rear
            // lateral grip, even when its longitudinal impulse was later clipped.
            const auto impulseLimit =
                std::abs( longitudinalSlipSpeed ) /
                ( dt * ( m_radius * m_radius / m_inertia + physics_Num( 1 ) / cornerMass ) );
            requestedLongitudinalForce =
                std::clamp( requestedLongitudinalForce, -impulseLimit, impulseLimit );
        }
        const auto requestedLateralForce = -m_lateralStiffness * m_grip * m_slipAngle;
        const auto requestedForce = Vector3<physics_Num>(
            requestedLateralForce, static_cast<physics_Num>( 0.0 ), -requestedLongitudinalForce );
        const auto requestedMagnitude = requestedForce.length();

#if !WP_FINAL
        WP_ASSERT( isFiniteValue( longitudinalSpeed ) );
        WP_ASSERT( isFiniteValue( lateralSpeed ) );
        WP_ASSERT( isFiniteValue( absLongitudinalSpeed ) );
        WP_ASSERT( isFiniteValue( slipDenominator ) );
        WP_ASSERT( slipDenominator > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( wheelSurfaceSpeed ) );
        WP_ASSERT( isFiniteValue( longitudinalSlipSpeed ) );
        WP_ASSERT( isFiniteValue( m_slipRatio ) );
        WP_ASSERT( Math<physics_Num>::Abs( m_slipRatio ) <= MaxSlipRatio );
        WP_ASSERT( isFiniteValue( m_slipAngle ) );
        WP_ASSERT( Math<physics_Num>::Abs( m_slipAngle ) <= MaxSlipAngle );
        WP_ASSERT( isFiniteValue( m_slipVelo ) );
        WP_ASSERT( m_slipVelo >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( requestedLongitudinalForce ) );
        WP_ASSERT( Math<physics_Num>::Abs( requestedLongitudinalForce ) < MaxWheelForce );
        WP_ASSERT( isFiniteValue( requestedLateralForce ) );
        WP_ASSERT( Math<physics_Num>::Abs( requestedLateralForce ) < MaxWheelForce );
        assertForceVector( requestedForce );
        WP_ASSERT( isFiniteValue( requestedMagnitude ) );
        WP_ASSERT( requestedMagnitude >= static_cast<physics_Num>( 0.0 ) );
#endif

        if( requestedMagnitude <= Math<physics_Num>::epsilon() )
        {
            return Vector3<physics_Num>::zero();
        }

        const auto staticLimit = m_staticFrictionCoefficient * normalForce * m_grip;
        auto       slidingLimit = m_slidingFrictionCoefficient * normalForce * m_grip;
        if( slidingLimit > staticLimit )
        {
            slidingLimit = staticLimit;
        }

#if !WP_FINAL
        WP_ASSERT( isFiniteValue( staticLimit ) );
        WP_ASSERT( staticLimit >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( slidingLimit ) );
        WP_ASSERT( slidingLimit >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( slidingLimit <= staticLimit + Math<physics_Num>::epsilon() );
#endif

        if( staticLimit <= Math<physics_Num>::epsilon() )
        {
            return Vector3<physics_Num>::zero();
        }

        const auto forceMagnitude = handling::brushMagnitude(requestedMagnitude, staticLimit, slidingLimit);

        const auto result = requestedForce * ( forceMagnitude / requestedMagnitude );

#if !WP_FINAL
        WP_ASSERT( isFiniteValue( forceMagnitude ) );
        WP_ASSERT( forceMagnitude >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( forceMagnitude <= staticLimit + Math<physics_Num>::epsilon() );
        assertForceVector( result );
        // WP_ASSERT( result.length() <= staticLimit + Math<physics_Num>::epsilon() );
#endif

        return result;
    }

    void WheelControllerBrush::integrateFreeSpin( physics_Num dt )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( dt ) );
        WP_ASSERT( dt >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_inertia ) );
        WP_ASSERT( m_inertia > static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_driveTorque ) );
        WP_ASSERT( Math<physics_Num>::Abs( m_driveTorque ) < MaxWheelTorque );
#endif

        m_onGround = false;
        m_compression = static_cast<physics_Num>( 0.0 );
        m_normalForce = static_cast<physics_Num>( 0.0 );
        m_slipRatio = static_cast<physics_Num>( 0.0 );
        m_slipAngle = static_cast<physics_Num>( 0.0 );
        m_slipVelo = static_cast<physics_Num>( 0.0 );
        m_suspensionForce = Vector3<physics_Num>::zero();
        m_roadForce = Vector3<physics_Num>::zero();

        if( m_inertia > Math<physics_Num>::epsilon() )
        {
            m_angularVelocity += ( m_driveTorque * dt ) / m_inertia;
        }

        const auto brakeTorque = m_brakeFrictionTorque * m_brake +
                                 m_handbrakeFrictionTorque * m_handbrake + m_rollingResistanceTorque;
        applyAngularFriction( brakeTorque, dt );
        m_angularVelocity =
            clampSignedFinite( m_angularVelocity, MaxAngularVelocity - static_cast<physics_Num>( 1.0 ) );

#if !WP_FINAL
        assertBrushOutput( *this );
#endif
    }

    void WheelControllerBrush::applyAngularFriction( physics_Num torque, physics_Num dt )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( torque ) );
        WP_ASSERT( torque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( torque < MaxWheelTorque );
        WP_ASSERT( isFiniteValue( dt ) );
        WP_ASSERT( dt >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( isFiniteValue( m_inertia ) );
        WP_ASSERT( m_inertia > static_cast<physics_Num>( 0.0 ) );
#endif

        if( torque <= static_cast<physics_Num>( 0.0 ) || m_inertia <= Math<physics_Num>::epsilon() )
        {
            return;
        }

        const auto angularDelta = torque * dt / m_inertia;
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( angularDelta ) );
        WP_ASSERT( angularDelta >= static_cast<physics_Num>( 0.0 ) );
#endif

        if( Math<physics_Num>::Abs( m_angularVelocity ) > angularDelta )
        {
            m_angularVelocity -= angularDelta * Math<physics_Num>::Sign( m_angularVelocity );
        }
        else
        {
            m_angularVelocity = static_cast<physics_Num>( 0.0 );
        }
    }

    void WheelControllerBrush::addTorque( physics_Num torque )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( torque ) );
        WP_ASSERT( Math<physics_Num>::Abs( torque ) < MaxWheelTorque );
#endif
        if( isFiniteValue( torque ) )
        {
            m_driveTorque += torque;
            m_driveTorque = clampSignedFinite( m_driveTorque, MaxWheelTorque );
        }
    }

    void WheelControllerBrush::setTorque( physics_Num torque )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( torque ) );
        WP_ASSERT( Math<physics_Num>::Abs( torque ) < MaxWheelTorque );
#endif
        m_driveTorque = clampSignedFinite( torque, MaxWheelTorque );
    }

    physics_Num WheelControllerBrush::getTorque() const
    {
        return m_driveTorque;
    }

    physics_Num WheelControllerBrush::getMass() const
    {
        if( auto vehicle = getOwner() )
        {
            return vehicle->getMass() * m_massFraction;
        }

        return m_chassisMass;
    }

    void WheelControllerBrush::setMass( physics_Num mass )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( mass ) );
        WP_ASSERT( mass >= static_cast<physics_Num>( 0.0 ) );
#endif
        m_chassisMass = clampNonNegativeFinite( mass );

        if( auto vehicle = getOwner() )
        {
            const auto vehicleMass = vehicle->getMass();
#if !WP_FINAL
            WP_ASSERT( isFiniteValue( vehicleMass ) );
            WP_ASSERT( vehicleMass > static_cast<physics_Num>( 0.0 ) );
#endif
            if( isFiniteValue( vehicleMass ) && vehicleMass > static_cast<physics_Num>( 0.0 ) )
            {
                setMassFraction( mass / vehicleMass );
            }
        }
    }

    physics_Num WheelControllerBrush::getSpringRate() const
    {
        return m_springRate;
    }

    void WheelControllerBrush::setSpringRate( physics_Num springRate )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( springRate ) );
        WP_ASSERT( springRate >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( springRate < MaxWheelStiffness );
#endif
        m_springRate = clampNonNegativeFinite( springRate );
    }

    physics_Num WheelControllerBrush::getRadius() const
    {
        return m_radius;
    }

    void WheelControllerBrush::setRadius( physics_Num radius )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( radius ) );
        WP_ASSERT( radius > static_cast<physics_Num>( 0.0 ) );
#endif
        if( !isFiniteValue( radius ) || radius <= static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "WheelControllerBrush::setRadius rejected non-positive radius." );
            return;
        }

        m_radius = radius;
    }

    physics_Num WheelControllerBrush::getSuspensionTravel() const
    {
        return m_suspensionTravel;
    }

    void WheelControllerBrush::setSuspensionTravel( physics_Num suspensionTravel )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( suspensionTravel ) );
        WP_ASSERT( suspensionTravel > static_cast<physics_Num>( 0.0 ) );
#endif
        if( !isFiniteValue( suspensionTravel ) || suspensionTravel <= static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "WheelControllerBrush::setSuspensionTravel rejected non-positive travel." );
            return;
        }

        m_suspensionTravel = suspensionTravel;
        m_suspensionDistance = suspensionTravel;
    }

    physics_Num WheelControllerBrush::getDamping() const
    {
        return m_damping;
    }

    void WheelControllerBrush::setDamping( physics_Num damping )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( damping ) );
        WP_ASSERT( damping >= static_cast<physics_Num>( 0.0 ) );
#endif
        m_damping = clampNonNegativeFinite( damping );
    }

    physics_Num WheelControllerBrush::getSuspensionDistance() const
    {
        return m_suspensionDistance;
    }

    void WheelControllerBrush::setSuspensionDistance( physics_Num suspensionDistance )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( suspensionDistance ) );
        WP_ASSERT( suspensionDistance >= static_cast<physics_Num>( 0.0 ) );
#endif
        m_suspensionDistance = clampNonNegativeFinite( suspensionDistance );
        if( m_suspensionDistance > static_cast<physics_Num>( 0.0 ) )
        {
            m_suspensionTravel = m_suspensionDistance;
        }
    }

    physics_Num WheelControllerBrush::getSteeringAngle() const
    {
        return m_steeringAngle;
    }

    void WheelControllerBrush::setSteeringAngle( physics_Num steeringAngle )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( steeringAngle ) );
        WP_ASSERT( Math<physics_Num>::Abs( steeringAngle ) <= MaxSteeringAngle );
#endif

        m_steeringAngle = clampSignedFinite( steeringAngle, MaxSteeringAngle );
    }

    bool WheelControllerBrush::isSteeringWheel() const
    {
        return m_isSteeringWheel;
    }

    void WheelControllerBrush::setSteeringWheel( bool steeringWheel )
    {
        m_isSteeringWheel = steeringWheel;
    }

    bool WheelControllerBrush::isPoweredWheel() const
    {
        return m_isPoweredWheel;
    }

    void WheelControllerBrush::setPoweredWheel( bool poweredWheel )
    {
        m_isPoweredWheel = poweredWheel;
        if( !m_isPoweredWheel )
        {
            m_driveTorque = static_cast<physics_Num>( 0.0 );
        }
    }

    physics_Num WheelControllerBrush::getAngularVelocity() const
    {
        return m_angularVelocity;
    }

    void WheelControllerBrush::setAngularVelocity( physics_Num angularVelocity )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( angularVelocity ) );
        WP_ASSERT( Math<physics_Num>::Abs( angularVelocity ) < MaxAngularVelocity );
#endif
        m_angularVelocity = clampSignedFinite( angularVelocity, MaxAngularVelocity );
    }

    physics_Num WheelControllerBrush::getBrake() const
    {
        return m_brake;
    }

    void WheelControllerBrush::setBrake( physics_Num brake )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( brake ) );
        WP_ASSERT( isUnitInput( brake ) );
#endif
        m_brake = clampUnitFinite( brake );
    }

    TireModel WheelControllerBrush::getTireModel() const
    {
        return TireModel::Brush;
    }

    void WheelControllerBrush::setTireModel( TireModel tireModel )
    {
        if( tireModel != TireModel::Brush )
        {
            WP_LOG_ERROR( "WheelControllerBrush only supports the Brush tire model." );
        }
    }

    physics_Num WheelControllerBrush::getMassFraction() const
    {
        return m_massFraction;
    }

    void WheelControllerBrush::setMassFraction( physics_Num massFraction )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( massFraction ) );
        WP_ASSERT( massFraction >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( massFraction <= static_cast<physics_Num>( 1.0 ) );
#endif
        m_massFraction = clampUnitFinite( massFraction );
    }

    physics_Num WheelControllerBrush::getInertia() const
    {
        return m_inertia;
    }

    void WheelControllerBrush::setInertia( physics_Num inertia )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( inertia ) );
        WP_ASSERT( inertia > static_cast<physics_Num>( 0.0 ) );
#endif
        if( isFiniteValue( inertia ) && inertia > static_cast<physics_Num>( 0.0 ) )
        {
            m_inertia = inertia;
        }
    }

    physics_Num WheelControllerBrush::getGrip() const
    {
        return m_grip;
    }

    void WheelControllerBrush::setGrip( physics_Num grip )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( grip ) );
        WP_ASSERT( grip >= static_cast<physics_Num>( 0.0 ) );
#endif
        m_grip = clampNonNegativeFinite( grip );
    }

    void WheelControllerBrush::setContactAcceleration( physics_Num acceleration )
    {
        m_contactAcceleration = clampNonNegativeFinite( acceleration );
    }

    physics_Num WheelControllerBrush::getStaticFrictionCoefficient() const
    {
        return m_staticFrictionCoefficient;
    }

    void WheelControllerBrush::setStaticFrictionCoefficient( physics_Num coefficient )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( coefficient ) );
        WP_ASSERT( coefficient >= static_cast<physics_Num>( 0.0 ) );
#endif
        m_staticFrictionCoefficient = clampNonNegativeFinite( coefficient );
        if( m_slidingFrictionCoefficient > m_staticFrictionCoefficient )
        {
            m_slidingFrictionCoefficient = m_staticFrictionCoefficient;
        }
    }

    physics_Num WheelControllerBrush::getSlidingFrictionCoefficient() const
    {
        return m_slidingFrictionCoefficient;
    }

    void WheelControllerBrush::setSlidingFrictionCoefficient( physics_Num coefficient )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( coefficient ) );
        WP_ASSERT( coefficient >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( coefficient <= m_staticFrictionCoefficient + Math<physics_Num>::epsilon() );
#endif
        m_slidingFrictionCoefficient = clampNonNegativeFinite( coefficient );
        if( m_slidingFrictionCoefficient > m_staticFrictionCoefficient )
        {
            m_slidingFrictionCoefficient = m_staticFrictionCoefficient;
        }
    }

    physics_Num WheelControllerBrush::getLongitudinalStiffness() const
    {
        return m_longitudinalStiffness;
    }

    void WheelControllerBrush::setLongitudinalStiffness( physics_Num stiffness )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( stiffness ) );
        WP_ASSERT( stiffness >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( stiffness < MaxWheelStiffness );
#endif
        m_longitudinalStiffness = clampNonNegativeFinite( stiffness );
    }

    physics_Num WheelControllerBrush::getLateralStiffness() const
    {
        return m_lateralStiffness;
    }

    void WheelControllerBrush::setLateralStiffness( physics_Num stiffness )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( stiffness ) );
        WP_ASSERT( stiffness >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( stiffness < MaxWheelStiffness );
#endif
        m_lateralStiffness = clampNonNegativeFinite( stiffness );
    }

    physics_Num WheelControllerBrush::getBrakeFrictionTorque() const
    {
        return m_brakeFrictionTorque;
    }

    void WheelControllerBrush::setBrakeFrictionTorque( physics_Num torque )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( torque ) );
        WP_ASSERT( torque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( torque < MaxWheelTorque );
#endif
        m_brakeFrictionTorque = clampNonNegativeFinite( torque );
    }

    physics_Num WheelControllerBrush::getHandbrake() const
    {
        return m_handbrake;
    }

    void WheelControllerBrush::setHandbrake( physics_Num handbrake )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( handbrake ) );
        WP_ASSERT( isUnitInput( handbrake ) );
#endif
        m_handbrake = clampUnitFinite( handbrake );
    }

    physics_Num WheelControllerBrush::getHandbrakeFrictionTorque() const
    {
        return m_handbrakeFrictionTorque;
    }

    void WheelControllerBrush::setHandbrakeFrictionTorque( physics_Num torque )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( torque ) );
        WP_ASSERT( torque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( torque < MaxWheelTorque );
#endif
        m_handbrakeFrictionTorque = clampNonNegativeFinite( torque );
    }

    physics_Num WheelControllerBrush::getRollingResistanceTorque() const
    {
        return m_rollingResistanceTorque;
    }

    void WheelControllerBrush::setRollingResistanceTorque( physics_Num torque )
    {
#if !WP_FINAL
        WP_ASSERT( isFiniteValue( torque ) );
        WP_ASSERT( torque >= static_cast<physics_Num>( 0.0 ) );
        WP_ASSERT( torque < MaxWheelTorque );
#endif
        m_rollingResistanceTorque = clampNonNegativeFinite( torque );
    }

    physics_Num WheelControllerBrush::getCompression() const
    {
        return m_compression;
    }

    physics_Num WheelControllerBrush::getNormalForce() const
    {
        return m_normalForce;
    }

    physics_Num WheelControllerBrush::getSlipRatio() const
    {
        return m_slipRatio;
    }

    physics_Num WheelControllerBrush::getSlipAngle() const
    {
        return m_slipAngle;
    }

    physics_Num WheelControllerBrush::getSlipVelo() const
    {
        return m_slipVelo;
    }

    bool WheelControllerBrush::isOnGround() const
    {
        return m_onGround;
    }

    bool WheelControllerBrush::isGrounded() const
    {
        return m_onGround;
    }

    Vector3<physics_Num> WheelControllerBrush::getWheelVelo() const
    {
        return m_wheelVelo;
    }

    Vector3<physics_Num> WheelControllerBrush::getLocalVelo() const
    {
        return m_localVelo;
    }

    Vector3<physics_Num> WheelControllerBrush::getSuspensionForceVector() const
    {
        return m_suspensionForce;
    }

    Vector3<physics_Num> WheelControllerBrush::getRoadForceVector() const
    {
        return m_roadForce;
    }

    SmartPtr<Properties> WheelControllerBrush::getProperties() const
    {
        auto properties = CVehicleComponent<IWheelComponent>::getProperties();
#if !WP_FINAL
        WP_ASSERT( properties );
#endif
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( "Radius", getRadius() );
        properties->setProperty( "Suspension Travel", getSuspensionTravel() );
        properties->setProperty( "Spring Rate", getSpringRate() );
        properties->setProperty( "Damping", getDamping() );
        properties->setProperty( "Mass Fraction", getMassFraction() );
        properties->setProperty( "Inertia", getInertia() );
        properties->setProperty( "Grip", getGrip() );
        properties->setProperty( "Static Friction Coefficient", getStaticFrictionCoefficient() );
        properties->setProperty( "Sliding Friction Coefficient", getSlidingFrictionCoefficient() );
        properties->setProperty( "Longitudinal Stiffness", getLongitudinalStiffness() );
        properties->setProperty( "Lateral Stiffness", getLateralStiffness() );
        properties->setProperty( "Brake Friction Torque", getBrakeFrictionTorque() );
        properties->setProperty( "Handbrake", getHandbrake() );
        properties->setProperty( "Handbrake Friction Torque", getHandbrakeFrictionTorque() );
        properties->setProperty( "Rolling Resistance Torque", getRollingResistanceTorque() );
        properties->setProperty( "Steering Angle", getSteeringAngle() );
        properties->setProperty( "Steering Wheel", isSteeringWheel() );
        properties->setProperty( "Powered Wheel", isPoweredWheel() );
        properties->setProperty( "Brake", getBrake() );
        properties->setProperty( "Angular Velocity", getAngularVelocity(), true );
        properties->setProperty( "Drive Torque", getTorque(), true );
        properties->setProperty( "Compression", getCompression(), true );
        properties->setProperty( "Normal Force", getNormalForce(), true );
        properties->setProperty( "Slip Ratio", getSlipRatio(), true );
        properties->setProperty( "Slip Angle", getSlipAngle(), true );
        properties->setProperty( "Slip Velocity", getSlipVelo(), true );
        properties->setProperty( "Is On Ground", isOnGround(), true );
        properties->setProperty( "Wheel Velocity", getWheelVelo(), true );
        properties->setProperty( "Local Velocity", getLocalVelo(), true );
        properties->setProperty( "Stable Contacts", m_implicitSuspension );
        properties->setProperty( "Traction Control", m_tractionControl );
        properties->setProperty( "Anti Lock Brakes", m_antiLockBrakes );
        properties->setProperty( "Contact Effective Mass", m_contactEffectiveMass );
        properties->setProperty( "Contact Acceleration", m_contactAcceleration );
        properties->setProperty( "Suspension Force", getSuspensionForceVector(), true );
        properties->setProperty( "Road Force", getRoadForceVector(), true );

        return properties;
    }

    void WheelControllerBrush::setProperties( SmartPtr<Properties> properties )
    {
#if !WP_FINAL
        WP_ASSERT( properties );
#endif
        if( !properties )
        {
            WP_LOG_ERROR( "WheelControllerBrush::setProperties received null properties." );
            return;
        }

        CVehicleComponent<IWheelComponent>::setProperties( properties );

        auto radius = getRadius();
        auto suspensionTravel = getSuspensionTravel();
        auto springRate = getSpringRate();
        auto damping = getDamping();
        auto massFraction = getMassFraction();
        auto inertia = getInertia();
        auto grip = getGrip();
        auto staticFrictionCoefficient = getStaticFrictionCoefficient();
        auto slidingFrictionCoefficient = getSlidingFrictionCoefficient();
        auto longitudinalStiffness = getLongitudinalStiffness();
        auto lateralStiffness = getLateralStiffness();
        auto brakeFrictionTorque = getBrakeFrictionTorque();
        auto handbrake = getHandbrake();
        auto handbrakeFrictionTorque = getHandbrakeFrictionTorque();
        auto rollingResistanceTorque = getRollingResistanceTorque();
        auto steeringAngle = getSteeringAngle();
        auto steeringWheel = isSteeringWheel();
        auto poweredWheel = isPoweredWheel();
        auto brake = getBrake();

        properties->getPropertyValue( "Radius", radius );
        properties->getPropertyValue( "Suspension Travel", suspensionTravel );
        properties->getPropertyValue( "Stable Contacts", m_implicitSuspension );
        properties->getPropertyValue( "Traction Control", m_tractionControl );
        properties->getPropertyValue( "Anti Lock Brakes", m_antiLockBrakes );
        properties->getPropertyValue( "Contact Effective Mass", m_contactEffectiveMass );
        m_contactEffectiveMass = clampNonNegativeFinite(m_contactEffectiveMass);
        properties->getPropertyValue( "Contact Acceleration", m_contactAcceleration );
        m_contactAcceleration = clampNonNegativeFinite( m_contactAcceleration );
        properties->getPropertyValue( "Spring Rate", springRate );
        properties->getPropertyValue( "Damping", damping );
        properties->getPropertyValue( "Mass Fraction", massFraction );
        properties->getPropertyValue( "Inertia", inertia );
        properties->getPropertyValue( "Grip", grip );
        properties->getPropertyValue( "Static Friction Coefficient", staticFrictionCoefficient );
        properties->getPropertyValue( "Sliding Friction Coefficient", slidingFrictionCoefficient );
        properties->getPropertyValue( "Longitudinal Stiffness", longitudinalStiffness );
        properties->getPropertyValue( "Lateral Stiffness", lateralStiffness );
        properties->getPropertyValue( "Brake Friction Torque", brakeFrictionTorque );
        properties->getPropertyValue( "Handbrake", handbrake );
        properties->getPropertyValue( "Handbrake Friction Torque", handbrakeFrictionTorque );
        properties->getPropertyValue( "Rolling Resistance Torque", rollingResistanceTorque );
        properties->getPropertyValue( "Steering Angle", steeringAngle );
        properties->getPropertyValue( "Steering Wheel", steeringWheel );
        properties->getPropertyValue( "Powered Wheel", poweredWheel );
        properties->getPropertyValue( "Brake", brake );

        setRadius( radius );
        setSuspensionTravel( suspensionTravel );
        setSpringRate( springRate );
        setDamping( damping );
        setMassFraction( massFraction );
        setInertia( inertia );
        setGrip( grip );
        setStaticFrictionCoefficient( staticFrictionCoefficient );
        setSlidingFrictionCoefficient( slidingFrictionCoefficient );
        setLongitudinalStiffness( longitudinalStiffness );
        setLateralStiffness( lateralStiffness );
        setBrakeFrictionTorque( brakeFrictionTorque );
        setHandbrake( handbrake );
        setHandbrakeFrictionTorque( handbrakeFrictionTorque );
        setRollingResistanceTorque( rollingResistanceTorque );
        setSteeringAngle( steeringAngle );
        setSteeringWheel( steeringWheel );
        setPoweredWheel( poweredWheel );
        setBrake( brake );
    }

} // namespace workphone
