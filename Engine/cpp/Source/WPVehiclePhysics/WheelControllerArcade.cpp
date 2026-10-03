// WheelControllerArcade.cpp
//
// Refactored so that CDriveTrain has a real effect on wheel and vehicle motion.
//
// Root causes fixed
// -----------------
// 1. TORQUE / ANGULAR-VELOCITY ALIASING
//    setTorque() was storing the drive torque directly into m_angularVelocity.
//    CDriveTrain also calls getAngularVelocity() to build the differential average
//    and ultimately to derive engine RPM via:
//        m_engineAngularVelo = averageAngularVelo * ratio
//    Returning the torque instead of the real wheel spin made RPM nonsensical.
//    Fix: m_driveTorque is now a separate member.  m_angularVelocity is the true
//    wheel spin (rad/s), updated every frame from the contact-patch velocity.
//
// 2. DRIVE FORCE NEVER APPLIED
//    The torque delivered by CDriveTrain was stored in m_angularVelocity but
//    never converted into a propulsive force on the rigidbody.
//    Fix: updateWheel() now converts m_driveTorque to a contact-patch force
//    (F = T / r) and applies it along the wheel's local forward axis (+Z).
//
// 3. BROKEN FRICTION MODEL
//    The original normalised the contact velocity before scaling, so friction
//    magnitude was constant regardless of speed (causing jitter at low velocity
//    and under-damping at high speed).  The longitudinal scale (0.001) nearly
//    zeroed the only axis the drivetrain could push along.
//    Fix: velocity-proportional (viscous) friction with independently tunable
//    lateral and rolling-resistance coefficients.
//
// Header additions required (WheelControllerArcade.hpp)
// -----------------------------------------------------
//    physics_Num m_driveTorque                = 0;
//    physics_Num m_lateralFrictionCoefficient = 5.0;   // N·s/m per unit mass
//    physics_Num m_rollingResistanceCoefficient = 0.01; // N·s/m per unit mass
//    bool        m_isGrounded                 = false;
//
//    // New public accessors (add declarations + matching getters/setters below):
//    physics_Num getLateralFrictionCoefficient() const;
//    void        setLateralFrictionCoefficient( physics_Num coefficient );
//    physics_Num getRollingResistanceCoefficient() const;
//    void        setRollingResistanceCoefficient( physics_Num coefficient );
//    bool        isGrounded() const;

#include <Workphone/WorkphonePCH.hpp>
#include <WPVehiclePhysics/WheelControllerArcade.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WheelControllerArcade, CVehicleComponent<IWheelComponent> );

    static const physics_Num MaxAngularVelocity = static_cast<physics_Num>( 1.0e3 );

    WheelControllerArcade::WheelControllerArcade()
    {
        m_hit = workphone::make_ptr<physics::RaycastHit>();
        m_hit->setCheckDynamic( false );
        m_hit->setCheckStatic( true );
    }

    WheelControllerArcade::~WheelControllerArcade() = default;

    // -------------------------------------------------------------------------
    // update
    // -------------------------------------------------------------------------
    void WheelControllerArcade::update()
    {
        if( Thread::getCurrentTask() != TaskId::Physics )
            return;

        // AWAKE, EDIT and PLAY all require the same work; avoid repeating the body
        // three times by falling through after a single state guard.
        const auto state = getState();
        if( state == State::AWAKE || state == State::EDIT || state == State::PLAY )
        {
            updateTransform();
            updateWheel();
        }
    }

    // -------------------------------------------------------------------------
    // updateWheel
    // -------------------------------------------------------------------------
    void WheelControllerArcade::updateWheel()
    {
        auto vehicle = getOwner();
        if( !vehicle )
            return;

        const auto vehicleWorldTransform = vehicle->getWorldTransform();
        auto       body = vehicle->getBody();
        WP_ASSERT( body );
        if( !body )
        {
            WP_LOG_ERROR( "WheelControllerArcade::updateWheel missing vehicle body." );
            return;
        }

        const auto localPos = m_localTransform.getPosition();
        const auto pos = m_worldTransform.getPosition();
        const auto up = vehicleWorldTransform.up();

        // ---- Ground detection -----------------------------------------------
        m_isGrounded = false;
        const auto ray = Ray3( pos, -up );

        if( body->castWorldRay( ray, m_hit ) )
        {
            const auto hitDistance = m_hit->getDistance();
            if( hitDistance > std::numeric_limits<physics_Num>::epsilon() &&
                hitDistance < m_suspensionDistance + m_radius )
            {
                m_isGrounded = true;
            }
        }

        if( !m_isGrounded )
        {
            // No contact forces; reset stored contact velocity so suspension
            // damping starts cleanly when the wheel lands again.
            m_wheelVelocity = Vector3<physics_Num>::zero();
            m_angularVelocity = static_cast<physics_Num>( 0.0 );
            return;
        }

        const auto vehicleMass = body->getMass();
        WP_ASSERT( vehicleMass > static_cast<physics_Num>( 0.0 ) );

        // ---- 1. Suspension: spring + damper ---------------------------------
        {
            constexpr physics_Num kGravity = static_cast<physics_Num>( 9.81 );

            // The full-compression spring force is sized so that at maximum
            // suspension travel the spring alone carries 2× the wheel's share
            // of the vehicle weight, giving some headroom for bumps.
            const auto fullCompressionForce =
                vehicleMass * m_massFraction * static_cast<physics_Num>( 2.0 ) * kGravity;

            const auto compression = m_suspensionDistance - ( m_hit->getDistance() - m_radius );
            const auto springForce = compression * ( m_springForce + fullCompressionForce );

            // m_wheelVelocity.y is the contact-point velocity along the vehicle's
            // up axis from the previous frame, used as the damper input.
            const auto dampingForce = -m_damping * m_wheelVelocity.y;
            const auto suspensionForce = springForce + dampingForce;

            body->addLocalForceAtLocalPosition( suspensionForce * Vector3<physics_Num>::unitY(),
                                                localPos );
        }

        // ---- 2. Contact velocity in wheel-local space -----------------------
        // Transform the world-space velocity at the contact point into vehicle
        // space, then rotate by the steering angle so that:
        //   X  = lateral (side-slip)
        //   Y  = vertical (used for suspension damping next frame)
        //   Z  = longitudinal (forward / backward)
        const auto steeringAngle = getSteeringAngle();
        auto       contactVelocity = body->getPointVelocity( pos );
        contactVelocity = Quaternion<physics_Num>::eulerDegrees( 0.0, steeringAngle, 0.0 ) *
                          vehicleWorldTransform.inverseTransformVector( contactVelocity );

        // Cache for the suspension damper on the next frame.
        m_wheelVelocity = contactVelocity;

        // Derive the true wheel spin from the longitudinal contact speed so that
        // CDriveTrain::update() receives a real angular velocity when it calls
        // getAngularVelocity().  CDriveTrain uses averageAngularVelo * ratio to
        // compute m_engineAngularVelo and therefore RPM.
        if( m_radius > static_cast<physics_Num>( 0.0 ) )
        {
            m_angularVelocity = contactVelocity.Z() / m_radius;
        }

        // cap the angular velocity to prevent numerical instability in extreme cases (e.g. very high
        // speed with a very small radius)
        if( Math<physics_Num>::Abs( m_angularVelocity ) > MaxAngularVelocity )
        {
            m_angularVelocity = Math<physics_Num>::Sign( m_angularVelocity ) * MaxAngularVelocity;
        }

        // ---- 3. Drive force -------------------------------------------------
        // CDriveTrain::update() calls wheel->setTorque(wheelDriveTorque) every
        // physics tick (see CDriveTrain.cpp, non-Pacejka else-branch).
        // wheelDriveTorque = engineTorque * gearRatio * finalDriveRatio
        //                  * (1 / poweredWheelCount) + differentialLockingTorque
        //
        // Convert torque → contact-patch force along the wheel's forward axis:
        //   F_drive = T / r   [N = N·m / m]
        //
        // Sign convention: positive torque → forward (+Z in vehicle-local space).
        // Reverse gear produces a negative ratio in CDriveTrain, so
        // wheelDriveTorque < 0 → force in -Z (backward).  No special casing needed.
        if( m_isPoweredWheel && m_radius > static_cast<physics_Num>( 0.0 ) )
        {
            const auto driveForceMagnitude = m_driveTorque / m_radius;
            body->addLocalForceAtLocalPosition( Vector3<physics_Num>( 0.0, 0.0, -driveForceMagnitude ),
                                                localPos );
        }

        // ---- 4. Tire friction -----------------------------------------------
        // Velocity-proportional (viscous) model: F = -v * mass * coefficient.
        // This avoids the constant-magnitude artefact of the previous
        // normaliseCopy() approach and keeps forces physically dimensionally sane.
        //
        // Lateral (X): high coefficient simulates the cornering stiffness of a
        // pneumatic tire; prevents sideways drift without locking the vehicle.
        //
        // Longitudinal (Z): small rolling-resistance coefficient.  It must be
        // small enough that the engine can accelerate against it, but large
        // enough to decelerate the vehicle when throttle is released.
        const auto lateralFriction = -contactVelocity.X() * vehicleMass * m_lateralFrictionCoefficient;

        const auto rollingResistance =
            -contactVelocity.Z() * vehicleMass * m_rollingResistanceCoefficient;

        body->addLocalForceAtLocalPosition(
            Vector3<physics_Num>( lateralFriction, 0.0, rollingResistance ), localPos );
    }

    // =========================================================================
    // Torque interface (called by CDriveTrain)
    // =========================================================================

    // addTorque / setTorque / getTorque now operate on m_driveTorque, which is
    // the torque delivered by the drivetrain each physics tick.  This is entirely
    // separate from m_angularVelocity, which is the wheel's actual spin rate.
    void WheelControllerArcade::addTorque( physics_Num torque )
    {
        m_driveTorque += torque * (physics_Num)0.25;
    }

    void WheelControllerArcade::setTorque( physics_Num torque )
    {
        m_driveTorque = torque * (physics_Num)0.25;
    }

    physics_Num WheelControllerArcade::getTorque() const
    {
        return m_driveTorque;
    }

    // =========================================================================
    // Vehicle mass helpers
    // =========================================================================

    physics_Num WheelControllerArcade::getMass() const
    {
        if( auto vehicle = getOwner() )
        {
            return vehicle->getMass() * m_massFraction;
        }

        return static_cast<physics_Num>( 0.0 );
    }

    void WheelControllerArcade::setMass( physics_Num mass )
    {
        WP_ASSERT( mass >= static_cast<physics_Num>( 0.0 ) );
        if( auto vehicle = getOwner() )
        {
            const auto vehicleMass = vehicle->getMass();
            WP_ASSERT( vehicleMass > static_cast<physics_Num>( 0.0 ) );
            if( vehicleMass > static_cast<physics_Num>( 0.0 ) )
            {
                setMassFraction( mass / vehicleMass );
            }
        }
    }

    // =========================================================================
    // Suspension / geometry accessors
    // =========================================================================

    physics_Num WheelControllerArcade::getSpringRate() const
    {
        return m_springForce;
    }

    void WheelControllerArcade::setSpringRate( physics_Num springRate )
    {
        setSpringForce( springRate );
    }

    physics_Num WheelControllerArcade::getRadius() const
    {
        return m_radius;
    }

    void WheelControllerArcade::setRadius( physics_Num radius )
    {
        WP_ASSERT( radius > static_cast<physics_Num>( 0.0 ) );
        if( radius <= static_cast<physics_Num>( 0.0 ) )
        {
            WP_LOG_ERROR( "WheelControllerArcade::setRadius rejected non-positive radius." );
            return;
        }

        m_radius = radius;
    }

    physics_Num WheelControllerArcade::getSuspensionTravel() const
    {
        return m_suspensionDistance;
    }

    void WheelControllerArcade::setSuspensionTravel( physics_Num suspensionTravel )
    {
        setSuspensionDistance( suspensionTravel );
    }

    physics_Num WheelControllerArcade::getDamping() const
    {
        return m_damping;
    }

    void WheelControllerArcade::setDamping( physics_Num damping )
    {
        WP_ASSERT( damping >= static_cast<physics_Num>( 0.0 ) );
        m_damping = damping;
    }

    physics_Num WheelControllerArcade::getSuspensionDistance() const
    {
        return m_suspensionDistance;
    }

    void WheelControllerArcade::setSuspensionDistance( physics_Num suspensionDistance )
    {
        WP_ASSERT( suspensionDistance >= static_cast<physics_Num>( 0.0 ) );
        m_suspensionDistance = suspensionDistance;
    }

    // =========================================================================
    // Steering accessors
    // =========================================================================

    physics_Num WheelControllerArcade::getSteeringAngle() const
    {
        return m_steeringAngle;
    }

    void WheelControllerArcade::setSteeringAngle( physics_Num steeringAngle )
    {
        m_steeringAngle = steeringAngle;
    }

    bool WheelControllerArcade::isSteeringWheel() const
    {
        return m_isSteeringWheel;
    }

    void WheelControllerArcade::setSteeringWheel( bool steeringWheel )
    {
        m_isSteeringWheel = steeringWheel;
    }

    // =========================================================================
    // Drive-wheel flag
    // =========================================================================

    bool WheelControllerArcade::isPoweredWheel() const
    {
        return m_isPoweredWheel;
    }

    void WheelControllerArcade::setPoweredWheel( bool poweredWheel )
    {
        m_isPoweredWheel = poweredWheel;
    }

    // =========================================================================
    // Angular velocity (true wheel spin, rad/s)
    // Used by CDriveTrain to compute the differential average and derive RPM.
    // Updated from ground-contact speed every frame that the wheel is grounded.
    // =========================================================================

    physics_Num WheelControllerArcade::getAngularVelocity() const
    {
        return m_angularVelocity;
    }

    void WheelControllerArcade::setAngularVelocity( physics_Num angularVelocity )
    {
        m_angularVelocity = angularVelocity;
    }

    // =========================================================================
    // Braking (not yet implemented for this controller)
    // =========================================================================

    physics_Num WheelControllerArcade::getBrake() const
    {
        return static_cast<physics_Num>( 0.0 );
    }

    void WheelControllerArcade::setBrake( physics_Num /*brake*/ )
    {
        // TODO: apply a braking torque opposing m_angularVelocity and add
        // a corresponding longitudinal friction force in updateWheel().
    }

    // =========================================================================
    // Tire model (this controller only supports Simple)
    // =========================================================================

    TireModel WheelControllerArcade::getTireModel() const
    {
        return TireModel::Simple;
    }

    void WheelControllerArcade::setTireModel( TireModel tireModel )
    {
        // WP_ASSERT( tireModel == TireModel::Simple );
        // if( tireModel != TireModel::Simple )
        //{
        //     WP_LOG_ERROR( "WheelControllerArcade only supports the Simple tire model." );
        // }
    }

    // =========================================================================
    // Spring force
    // =========================================================================

    physics_Num WheelControllerArcade::getSpringForce() const
    {
        return m_springForce;
    }

    void WheelControllerArcade::setSpringForce( physics_Num springForce )
    {
        WP_ASSERT( springForce >= static_cast<physics_Num>( 0.0 ) );
        m_springForce = springForce >= static_cast<physics_Num>( 0.0 ) ? springForce
                                                                       : static_cast<physics_Num>( 0.0 );
    }

    // =========================================================================
    // Mass fraction
    // =========================================================================

    physics_Num WheelControllerArcade::getMassFraction() const
    {
        return m_massFraction;
    }

    void WheelControllerArcade::setMassFraction( physics_Num massFraction )
    {
        WP_ASSERT( massFraction >= static_cast<physics_Num>( 0.0 ) );
        m_massFraction = massFraction >= static_cast<physics_Num>( 0.0 )
                           ? massFraction
                           : static_cast<physics_Num>( 0.0 );
    }

    // =========================================================================
    // Wheel velocity (contact-patch velocity in wheel-local space)
    // =========================================================================

    Vector3<physics_Num> WheelControllerArcade::getWheelVelocity() const
    {
        return m_wheelVelocity;
    }

    void WheelControllerArcade::setWheelVelocity( const Vector3<physics_Num> &wheelVelocity )
    {
        m_wheelVelocity = wheelVelocity;
    }

    // =========================================================================
    // Friction coefficients (new)
    // =========================================================================

    physics_Num WheelControllerArcade::getLateralFrictionCoefficient() const
    {
        return m_lateralFrictionCoefficient;
    }

    void WheelControllerArcade::setLateralFrictionCoefficient( physics_Num coefficient )
    {
        WP_ASSERT( coefficient >= static_cast<physics_Num>( 0.0 ) );
        m_lateralFrictionCoefficient = coefficient >= static_cast<physics_Num>( 0.0 )
                                         ? coefficient
                                         : static_cast<physics_Num>( 0.0 );
    }

    physics_Num WheelControllerArcade::getRollingResistanceCoefficient() const
    {
        return m_rollingResistanceCoefficient;
    }

    void WheelControllerArcade::setRollingResistanceCoefficient( physics_Num coefficient )
    {
        WP_ASSERT( coefficient >= static_cast<physics_Num>( 0.0 ) );
        m_rollingResistanceCoefficient = coefficient >= static_cast<physics_Num>( 0.0 )
                                           ? coefficient
                                           : static_cast<physics_Num>( 0.0 );
    }

    // =========================================================================
    // Ground contact state (new, read-only)
    // =========================================================================

    bool WheelControllerArcade::isGrounded() const
    {
        return m_isGrounded;
    }

    // =========================================================================
    // Properties serialisation
    // =========================================================================

    SmartPtr<Properties> WheelControllerArcade::getProperties() const
    {
        auto properties = CVehicleComponent<IWheelComponent>::getProperties();
        WP_ASSERT( properties );

        properties->setProperty( "Radius", getRadius() );
        properties->setProperty( "Suspension Distance", getSuspensionDistance() );
        properties->setProperty( "Spring Force", getSpringForce() );
        properties->setProperty( "Damping", getDamping() );
        properties->setProperty( "Mass Fraction", getMassFraction() );
        properties->setProperty( "Angular Velocity", getAngularVelocity() );
        properties->setProperty( "Steering Angle", getSteeringAngle() );
        properties->setProperty( "Steering Wheel", isSteeringWheel() );
        properties->setProperty( "Powered Wheel", isPoweredWheel() );
        properties->setProperty( "Lateral Friction Coefficient", getLateralFrictionCoefficient() );
        properties->setProperty( "Rolling Resistance Coefficient", getRollingResistanceCoefficient() );
        // Read-only runtime values:
        properties->setProperty( "Drive Torque", m_driveTorque, true );
        properties->setProperty( "Is Grounded", m_isGrounded, true );
        properties->setProperty( "Wheel Velocity", getWheelVelocity(), true );

        return properties;
    }

    void WheelControllerArcade::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            WP_LOG_ERROR( "WheelControllerArcade::setProperties received null properties." );
            return;
        }

        CVehicleComponent<IWheelComponent>::setProperties( properties );

        auto radius = getRadius();
        auto suspensionDistance = getSuspensionDistance();
        auto springForce = getSpringForce();
        auto damping = getDamping();
        auto massFraction = getMassFraction();
        auto angularVelocity = getAngularVelocity();
        auto steeringAngle = getSteeringAngle();
        auto steeringWheel = isSteeringWheel();
        auto poweredWheel = isPoweredWheel();
        auto lateralFrictionCoeff = getLateralFrictionCoefficient();
        auto rollingResistanceCoeff = getRollingResistanceCoefficient();

        properties->getPropertyValue( "Radius", radius );
        properties->getPropertyValue( "Suspension Distance", suspensionDistance );
        properties->getPropertyValue( "Spring Force", springForce );
        properties->getPropertyValue( "Damping", damping );
        properties->getPropertyValue( "Mass Fraction", massFraction );
        properties->getPropertyValue( "Angular Velocity", angularVelocity );
        properties->getPropertyValue( "Steering Angle", steeringAngle );
        properties->getPropertyValue( "Steering Wheel", steeringWheel );
        properties->getPropertyValue( "Powered Wheel", poweredWheel );
        properties->getPropertyValue( "Lateral Friction Coefficient", lateralFrictionCoeff );
        properties->getPropertyValue( "Rolling Resistance Coefficient", rollingResistanceCoeff );

        setRadius( radius );
        setSuspensionDistance( suspensionDistance );
        setSpringForce( springForce );
        setDamping( damping );
        setMassFraction( massFraction );
        setAngularVelocity( angularVelocity );
        setSteeringAngle( steeringAngle );
        setSteeringWheel( steeringWheel );
        setPoweredWheel( poweredWheel );
        setLateralFrictionCoefficient( lateralFrictionCoeff );
        setRollingResistanceCoefficient( rollingResistanceCoeff );
    }

} // namespace workphone
