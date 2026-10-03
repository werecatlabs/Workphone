#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Vehicle/WheelController.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/Physics/IRigidBody3.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Physics/RaycastHit.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, WheelComponent,
                                   VehicleComponent<IWheelComponent> );

        WheelComponent::WheelComponent()
        {
            m_hit = workphone::make_ptr<physics::RaycastHit>();
            m_hit->setCheckDynamic( false );
            m_hit->setCheckStatic( true );
        }

        WheelComponent::~WheelComponent() = default;

        void WheelComponent::update()
        {
            auto task = Thread::getCurrentTask();
            switch( task )
            {
            case TaskId::Physics:
            {
                auto state = getState();
                switch( state )
                {
                case State::AWAKE:
                {
                    updateTransform();

                    // Select tire model based on current setting
                    switch( m_tireModel )
                    {
                    case TireModel::Simple:
                        updateWheel();
                        break;
                    case TireModel::Pacejka:
                        updatePacejka();
                        break;
                    case TireModel::Brush:
                        updateBrushTireModel();
                        break;
                    }
                }
                break;
                case State::EDIT:
                {
                    updateTransform();

                    // Select tire model based on current setting
                    switch( m_tireModel )
                    {
                    case TireModel::Simple:
                        updateWheel();
                        break;
                    case TireModel::Pacejka:
                        updatePacejka();
                        break;
                    case TireModel::Brush:
                        updateBrushTireModel();
                        break;
                    }
                }
                break;
                case State::PLAY:
                {
                    updateTransform();

                    // Select tire model based on current setting
                    switch( m_tireModel )
                    {
                    case TireModel::Simple:
                        updateWheel();
                        break;
                    case TireModel::Pacejka:
                        updatePacejka();
                        break;
                    case TireModel::Brush:
                        updateBrushTireModel();
                        break;
                    }
                }
                break;
                }
            }
            break;
            }
        }

        void WheelComponent::updateWheel()
        {
            if( auto vehicle = getOwnerPtr() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto timer = applicationManager->getTimerPtr();
                auto dt = timer->getDeltaTime();

                auto vehicleLocalTransform = vehicle->getLocalTransform();
                auto vehicleWorldTransform = vehicle->getWorldTransform();
                auto body = vehicle->getBody();

                auto localPos = m_localTransform.getPosition();
                auto pos = m_worldTransform.getPosition();
                auto up = vehicleWorldTransform.up();

                auto onGround = false;

                auto ray = Ray3( pos, -up );

                if( body->castWorldRay( ray, m_hit ) )
                {
                    auto hitDistance = m_hit->getDistance();
                    if( hitDistance > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        if( hitDistance < m_suspensionDistance + m_radius )
                        {
                            onGround = true;
                        }
                    }
                }

                if( onGround )
                {
                    auto groundNormal =
                        vehicleWorldTransform.inverseTransformVector( m_hit->getNormal() );

                    // Calculate the suspension compression and apply a force.
                    const Vector3<physics_Num> gravity( 0.0, 9.81, 0.0 );
                    const auto vehicleMass = body->getMass();
                    auto fullCompressionSpringForce =
                        vehicleMass * m_massFraction * static_cast<physics_Num>( 2.0 ) * gravity.Y();

                    auto steeringAngle = getSteeringAngle();
                    auto groundVelocityWorld = body->getPointVelocity( m_worldTransform.getPosition() );

                    auto wheelOrientation =
                        Quaternion<physics_Num>::eulerDegrees( 0.0, steeringAngle, 0.0 );
                    auto groundVelocity =
                        wheelOrientation *
                        vehicleWorldTransform.inverseTransformVector( groundVelocityWorld );

                    auto compression = m_suspensionDistance - ( m_hit->getDistance() - m_radius );
                    auto springForceMagnitude = compression * fullCompressionSpringForce;
                    auto dampingForceMagnitude = groundVelocity.dotProduct( groundNormal ) * m_damping;
                    auto totalForceMagnitude = springForceMagnitude - dampingForceMagnitude;
                    auto totalSuspensionForce = totalForceMagnitude * Vector3<physics_Num>::unitY();

                    // Apply the force to the car's Rigidbody.
                    body->addLocalForceAtLocalPosition( totalSuspensionForce, localPos );

                    // Simple tire model with torque-driven wheel

                    // Calculate wheel circumferential velocity from angular velocity
                    auto wheelCircumferentialVelocity = m_angularVelocity * m_radius;
                    auto longitudinalGroundVelocity = groundVelocity.Z();

                    // Calculate slip ratio for longitudinal force
                    auto slipRatio = static_cast<physics_Num>( 0.0 );
                    if( Math<physics_Num>::Abs( longitudinalGroundVelocity ) >
                        std::numeric_limits<physics_Num>::epsilon() )
                    {
                        slipRatio = ( wheelCircumferentialVelocity - longitudinalGroundVelocity ) /
                                    Math<physics_Num>::Abs( longitudinalGroundVelocity );
                        slipRatio =
                            Math<physics_Num>::clamp( slipRatio, static_cast<physics_Num>( -1.0 ),
                                                      static_cast<physics_Num>( 1.0 ) );
                    }

                    // Calculate normal force for tire grip
                    auto normalForce =
                        Math<physics_Num>::max( totalForceMagnitude, static_cast<physics_Num>( 0.0 ) );

                    // Simple tire grip model - longitudinal force based on slip
                    auto maxTireForce =
                        normalForce * m_grip * physics_Num( 1.0 );      // Tire grip coefficient
                    auto longitudinalForce = slipRatio * maxTireForce;  // Removed dt multiplication

                    // Limit longitudinal force to available grip
                    longitudinalForce =
                        Math<physics_Num>::clamp( longitudinalForce, -maxTireForce, maxTireForce );

                    // Improved lateral force calculation for simple tire model
                    auto lateralVelocity = groundVelocity.X();
                    auto longitudinalVelocity = groundVelocity.Z();
                    auto vehicleSpeed = groundVelocity.length();

                    // Calculate slip angle (lateral slip)
                    auto slipAngle = static_cast<physics_Num>( 0.0 );
                    auto speedThreshold =
                        static_cast<physics_Num>( 0.5 );  // Minimum speed for slip angle calculation

                    if( vehicleSpeed > speedThreshold )
                    {
                        // Calculate slip angle based on velocity ratio
                        slipAngle = Math<physics_Num>::ATan2(
                            lateralVelocity, Math<physics_Num>::Abs( longitudinalVelocity ) );

                        // Limit slip angle to realistic range (±30 degrees)
                        auto maxSlipAngle =
                            static_cast<physics_Num>( Math<physics_Num>::pi() / physics_Num( 6.0 ) );
                        slipAngle = Math<physics_Num>::clamp( slipAngle, -maxSlipAngle, maxSlipAngle );
                    }

                    // Calculate lateral force using improved model
                    auto lateralForce = static_cast<physics_Num>( 0.0 );

                    if( Math<physics_Num>::Abs( slipAngle ) >
                        std::numeric_limits<physics_Num>::epsilon() )
                    {
                        /*
                        // Lateral tire stiffness (adjustable parameter)
                        auto lateralStiffness = static_cast<physics_Num>( 8.0 );  // Can be tuned

                        // Calculate cornering force with saturation
                        auto corneringStiffness = lateralStiffness * normalForce;
                        auto linearLateralForce = -corneringStiffness * slipAngle;

                        // Apply saturation curve to prevent unrealistic forces at high slip angles
                        auto maxLateralForce =
                            maxTireForce *
                            static_cast<physics_Num>( 0.9 );  // Reserve some grip for longitudinal
                        auto saturationFactor =
                            static_cast<physics_Num>( 3.0 );  // Controls saturation curve steepness

                        // Sigmoid-like saturation function
                        auto normalizedForce = linearLateralForce / maxLateralForce;
                        auto saturatedNormalizedForce =
                            normalizedForce /
                            ( static_cast<physics_Num>( 1.0 ) +
                              Math<physics_Num>::Abs( normalizedForce ) / saturationFactor );

                        lateralForce = saturatedNormalizedForce * maxLateralForce;

                        // Speed-dependent factor (less grip at very low speeds)
                        auto speedFactor =
                            Math<physics_Num>::min( vehicleSpeed / static_cast<physics_Num>( 2.0 ),
                                                    static_cast<physics_Num>( 1.0 ) );
                        lateralForce *= speedFactor;

                        // Clamp to maximum available force
                        lateralForce =
                            Math<physics_Num>::clamp( lateralForce, -maxLateralForce, maxLateralForce );
                        */

                        auto steeringAngle = getSteeringAngle();
                        auto groundVelocity = body->getPointVelocity( m_worldTransform.getPosition() );
                        groundVelocity =
                            Quaternion<physics_Num>::eulerDegrees( 0.0, steeringAngle, 0.0 ) *
                            vehicleWorldTransform.inverseTransformVector( groundVelocity );

                        auto frictionForce =
                            -groundVelocity.normaliseCopy() * vehicle->getMass() * physics_Num( 10.0 );
                        lateralForce = frictionForce.X() * physics_Num( 0.5 );
                        frictionForce.Y() = 0.0;
                        frictionForce.Z() *= 0.001;
                    }
                    else
                    {
                        // At very low slip angles, apply simple velocity-based damping
                        auto lateralDamping = static_cast<physics_Num>( 3.0 ) * normalForce * m_grip;
                        //lateralForce = -lateralVelocity * lateralDamping;

                        // Clamp to maximum available force
                        auto maxDampingForce = maxTireForce * static_cast<physics_Num>( 2.0 );
                        //lateralForce =
                        //    Math<physics_Num>::clamp( lateralForce, -maxDampingForce, maxDampingForce );
                    }

                    // Apply tire forces (both lateral AND longitudinal)
                    //Vector3<physics_Num> tireForce( lateralForce, 0.0, longitudinalForce );
                    Vector3<physics_Num> tireForce( lateralForce, 0.0, 0.0 );
                    body->addLocalForceAtLocalPosition( tireForce, localPos );

                    // Apply torque feedback to wheel angular velocity
                    auto torqueFromTireForce = -longitudinalForce * m_radius;

                    // Calculate braking torque
                    auto brakingTorque = static_cast<physics_Num>( 0.0 );
                    if( m_brake > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        // Calculate maximum braking torque based on brake input
                        auto maxBrakeTorque = m_brakeFrictionTorque * m_brake;

                        // Apply braking torque in opposite direction to wheel rotation
                        if( Math<physics_Num>::Abs( m_angularVelocity ) >
                            std::numeric_limits<physics_Num>::epsilon() )
                        {
                            brakingTorque =
                                -Math<physics_Num>::Sign( m_angularVelocity ) * maxBrakeTorque;

                            // Prevent over-braking (don't reverse wheel direction)
                            auto maxAllowedBrakeTorque = Math<physics_Num>::Abs(
                                m_angularVelocity * m_wheelInertia / (physics_Num)dt );
                            brakingTorque = Math<physics_Num>::clamp(
                                brakingTorque, -maxAllowedBrakeTorque, maxAllowedBrakeTorque );
                        }
                    }

                    // Calculate handbrake torque
                    auto handbrakeTorque = static_cast<physics_Num>( 0.0 );
                    if( m_handbrake > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        // Calculate maximum handbrake torque based on handbrake input
                        auto maxHandbrakeTorque = m_handbrakeFrictionTorque * m_handbrake;

                        // Apply handbrake torque in opposite direction to wheel rotation
                        if( Math<physics_Num>::Abs( m_angularVelocity ) >
                            std::numeric_limits<physics_Num>::epsilon() )
                        {
                            handbrakeTorque =
                                -Math<physics_Num>::Sign( m_angularVelocity ) * maxHandbrakeTorque;

                            // Prevent over-braking (don't reverse wheel direction)
                            auto maxAllowedHandbrakeTorque = Math<physics_Num>::Abs(
                                m_angularVelocity * m_wheelInertia / (physics_Num)dt );
                            handbrakeTorque = Math<physics_Num>::clamp(
                                handbrakeTorque, -maxAllowedHandbrakeTorque, maxAllowedHandbrakeTorque );
                        }
                    }

                    // Update angular velocity: applied torque minus tire resistance plus braking
                    auto netTorque = m_torque - torqueFromTireForce + brakingTorque + handbrakeTorque;
                    m_angularVelocity += ( netTorque / m_wheelInertia ) * (physics_Num)dt;

                    // Apply rolling resistance
                    m_angularVelocity *=
                        ( static_cast<physics_Num>( 1.0 ) - m_rollingResistance * (physics_Num)dt );

                    // Update the wheel's linear velocity.
                    m_wheelVelocity = groundVelocity;
                }
                else
                {
                    // Wheel not on ground - apply air resistance to angular velocity
                    m_angularVelocity *=
                        ( static_cast<physics_Num>( 1.0 ) - m_airResistance * (physics_Num)dt );
                    m_wheelVelocity = Vector3<physics_Num>::zero();
                }
            }
        }

        void WheelComponent::updatePacejka()
        {
            if( auto vehicle = getOwner() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto timer = applicationManager->getTimer();
                auto dt = timer->getDeltaTime();

                auto vehicleLocalTransform = vehicle->getLocalTransform();
                auto vehicleWorldTransform = vehicle->getWorldTransform();
                auto body = vehicle->getBody();

                auto localPos = m_localTransform.getPosition();
                auto pos = m_worldTransform.getPosition();
                auto up = vehicleWorldTransform.up();

                auto onGround = false;
                auto ray = Ray3( pos, -up );

                if( body->castWorldRay( ray, m_hit ) )
                {
                    auto hitDistance = m_hit->getDistance();
                    if( hitDistance > std::numeric_limits<real_Num>::epsilon() )
                    {
                        if( hitDistance < m_suspensionDistance + m_radius )
                        {
                            onGround = true;
                        }
                    }
                }

                if( onGround )
                {
                    // Calculate suspension compression and forces
                    auto compression = m_suspensionDistance - ( m_hit->getDistance() - m_radius );
                    compression = Math<real_Num>::clamp( compression, static_cast<real_Num>( 0.0 ),
                                                         m_suspensionDistance );
                    auto compressionRatio = compression / m_suspensionDistance;

                    // Suspension force calculation
                    const Vector3<real_Num> gravity( 0.0, 9.81, 0.0 );
                    const auto mass = body->getMass();
                    auto fullCompressionSpringForce =
                        mass * m_massFraction * static_cast<real_Num>( 2.0 ) * gravity.Y();

                    auto springForceMagnitude =
                        compressionRatio * ( m_springForce + fullCompressionSpringForce );
                    auto dampingForceMagnitude = -m_damping * m_wheelVelocity.y;
                    auto totalSuspensionForce = springForceMagnitude + dampingForceMagnitude;

                    // Apply suspension force
                    body->addLocalForceAtLocalPosition(
                        totalSuspensionForce * Vector3<real_Num>::unitY(), localPos );

                    // Get wheel velocity and calculate tire forces using simplified Pacejka model
                    auto groundVelocity = body->getPointVelocity( m_worldTransform.getPosition() );

                    // Transform to local wheel coordinates considering steering
                    auto steeringAngle = getSteeringAngle();
                    auto steeringRotation =
                        Quaternion<real_Num>::eulerDegrees( 0.0, steeringAngle, 0.0 );
                    auto localGroundVelocity =
                        steeringRotation *
                        vehicleWorldTransform.inverseTransformVector( groundVelocity );

                    // Calculate slip ratio and slip angle
                    auto wheelCircumferentialVelocity = m_angularVelocity * m_radius;
                    auto longitudinalVelocity = localGroundVelocity.Z();
                    auto lateralVelocity = localGroundVelocity.X();

                    // Slip ratio calculation (longitudinal slip)
                    auto slipRatio = static_cast<real_Num>( 0.0 );
                    if( Math<real_Num>::Abs( longitudinalVelocity ) >
                        std::numeric_limits<real_Num>::epsilon() )
                    {
                        slipRatio = ( wheelCircumferentialVelocity - longitudinalVelocity ) /
                                    Math<real_Num>::Abs( longitudinalVelocity );
                        slipRatio = Math<real_Num>::clamp( slipRatio, static_cast<real_Num>( -1.0 ),
                                                           static_cast<real_Num>( 1.0 ) );
                    }

                    // Slip angle calculation (lateral slip)
                    auto slipAngle = static_cast<real_Num>( 0.0 );
                    auto speedThreshold = static_cast<real_Num>( 0.1 );
                    if( localGroundVelocity.length() > speedThreshold )
                    {
                        slipAngle = Math<real_Num>::ATan2( lateralVelocity,
                                                           Math<real_Num>::Abs( longitudinalVelocity ) );
                        slipAngle = Math<real_Num>::clamp(
                            slipAngle, static_cast<real_Num>( -Math<real_Num>::pi() / 4.0 ),
                            static_cast<real_Num>( Math<real_Num>::pi() / 4.0 ) );
                    }

                    // Normal force on tire (from suspension compression)
                    auto normalForce =
                        Math<real_Num>::max( totalSuspensionForce, static_cast<real_Num>( 0.0 ) );

                    // Simplified Pacejka tire model coefficients
                    const auto tireGrip = static_cast<real_Num>( 1.0 );
                    const auto longitudinalStiffness = static_cast<real_Num>( 10.0 );
                    const auto lateralStiffness = static_cast<real_Num>( 8.0 );
                    const auto maxSlipRatio = static_cast<real_Num>( 0.15 );
                    const auto maxSlipAngle =
                        static_cast<real_Num>( Math<real_Num>::pi() / 12.0 );  // 15 degrees

                    // Calculate tire forces using simplified Pacejka-like formulation
                    auto normalizedSlipRatio = slipRatio / maxSlipRatio;
                    auto normalizedSlipAngle = slipAngle / maxSlipAngle;

                    // Longitudinal force (braking/acceleration)
                    auto longitudinalForce = static_cast<real_Num>( 0.0 );
                    if( Math<real_Num>::Abs( normalizedSlipRatio ) >
                        std::numeric_limits<real_Num>::epsilon() )
                    {
                        auto slipFactor =
                            Math<real_Num>::Atan( longitudinalStiffness * normalizedSlipRatio ) /
                            normalizedSlipRatio;
                        longitudinalForce = normalForce * tireGrip * slipFactor * normalizedSlipRatio;
                    }

                    // Lateral force (cornering)
                    auto lateralForce = static_cast<real_Num>( 0.0 );
                    if( Math<real_Num>::Abs( normalizedSlipAngle ) >
                        std::numeric_limits<real_Num>::epsilon() )
                    {
                        auto corneringFactor =
                            Math<real_Num>::Atan( lateralStiffness * normalizedSlipAngle ) /
                            normalizedSlipAngle;
                        lateralForce = normalForce * tireGrip * corneringFactor * normalizedSlipAngle;
                    }

                    // Apply circle of friction constraint (combined forces cannot exceed maximum available grip)
                    auto maxTireForceMagnitude = normalForce * tireGrip;
                    auto totalTireForce = Math<real_Num>::Sqrt( longitudinalForce * longitudinalForce +
                                                                lateralForce * lateralForce );

                    if( totalTireForce > maxTireForceMagnitude &&
                        totalTireForce > std::numeric_limits<real_Num>::epsilon() )
                    {
                        auto scale = maxTireForceMagnitude / totalTireForce;
                        longitudinalForce *= scale;
                        lateralForce *= scale;
                    }

                    // Create force vector in local wheel coordinates
                    Vector3<real_Num> localTireForce( lateralForce, 0.0, longitudinalForce );

                    // Transform back to world coordinates
                    auto worldTireForce =
                        vehicleWorldTransform.transformVector( steeringRotation * localTireForce );

                    // Apply tire forces to the vehicle body
                    body->addLocalForceAtLocalPosition(
                        vehicleWorldTransform.inverseTransformVector( worldTireForce ), localPos );

                    // Update wheel angular velocity based on applied torques
                    auto wheelInertia = m_wheelInertia;
                    auto torqueFromTireForce = longitudinalForce * m_radius;

                    // Calculate throttle torque (driving force)
                    auto throttleTorque = static_cast<real_Num>( 0.0 );
                    if( m_torque > std::numeric_limits<real_Num>::epsilon() )
                    {
                        // Apply throttle torque in the direction that accelerates the wheel
                        throttleTorque = m_torque;
                    }

                    // Calculate braking torque
                    auto brakingTorque = static_cast<real_Num>( 0.0 );
                    if( m_brake > std::numeric_limits<real_Num>::epsilon() )
                    {
                        // Calculate maximum braking torque based on brake input
                        auto maxBrakeTorque = m_brakeFrictionTorque * m_brake;

                        // Apply braking torque in opposite direction to wheel rotation
                        if( Math<real_Num>::Abs( m_angularVelocity ) >
                            std::numeric_limits<real_Num>::epsilon() )
                        {
                            brakingTorque = -Math<real_Num>::Sign( m_angularVelocity ) * maxBrakeTorque;

                            // Prevent over-braking (don't reverse wheel direction)
                            auto maxAllowedBrakeTorque = Math<real_Num>::Abs(
                                m_angularVelocity * wheelInertia / static_cast<real_Num>( dt ) );
                            brakingTorque = Math<real_Num>::clamp( brakingTorque, -maxAllowedBrakeTorque,
                                                                   maxAllowedBrakeTorque );
                        }
                    }

                    // Calculate handbrake torque
                    auto handbrakeTorque = static_cast<real_Num>( 0.0 );
                    if( m_handbrake > std::numeric_limits<real_Num>::epsilon() )
                    {
                        // Calculate maximum handbrake torque based on handbrake input
                        auto maxHandbrakeTorque = m_handbrakeFrictionTorque * m_handbrake;

                        // Apply handbrake torque in opposite direction to wheel rotation
                        if( Math<real_Num>::Abs( m_angularVelocity ) >
                            std::numeric_limits<real_Num>::epsilon() )
                        {
                            handbrakeTorque =
                                -Math<real_Num>::Sign( m_angularVelocity ) * maxHandbrakeTorque;

                            // Prevent over-braking (don't reverse wheel direction)
                            auto maxAllowedHandbrakeTorque = Math<real_Num>::Abs(
                                m_angularVelocity * wheelInertia / static_cast<real_Num>( dt ) );
                            handbrakeTorque = Math<real_Num>::clamp(
                                handbrakeTorque, -maxAllowedHandbrakeTorque, maxAllowedHandbrakeTorque );
                        }
                    }

                    // Net torque calculation: throttle adds angular velocity, brakes reduce it
                    auto netTorque =
                        throttleTorque - torqueFromTireForce + brakingTorque + handbrakeTorque;
                    m_angularVelocity += ( netTorque / wheelInertia ) * static_cast<physics_Num>( dt );

                    // Apply basic rolling resistance
                    auto rollingResistance = static_cast<real_Num>( 0.1 );
                    m_angularVelocity *= ( static_cast<real_Num>( 1.0 ) -
                                           rollingResistance * static_cast<physics_Num>( dt ) );

                    // Update wheel velocity for next frame
                    m_wheelVelocity = groundVelocity;
                }
                else
                {
                    // Wheel not on ground - apply basic friction to angular velocity
                    auto airResistance = static_cast<real_Num>( 0.05 );

                    // Apply throttle torque even when not on ground (wheel spin)
                    auto throttleTorque = static_cast<real_Num>( 0.0 );
                    if( m_torque > std::numeric_limits<real_Num>::epsilon() )
                    {
                        throttleTorque = m_torque;
                        m_angularVelocity +=
                            ( throttleTorque / m_wheelInertia ) * static_cast<physics_Num>( dt );
                    }

                    // Apply air resistance
                    m_angularVelocity *= ( static_cast<real_Num>( 1.0 ) -
                                           airResistance * static_cast<physics_Num>( dt ) );
                    m_wheelVelocity = Vector3<real_Num>::zero();
                }
            }
        }

        void WheelComponent::updateBrushTireModel()
        {
            if( auto vehicle = getOwnerPtr() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto timer = applicationManager->getTimerPtr();
                auto dt = timer->getDeltaTime();

                auto vehicleLocalTransform = vehicle->getLocalTransform();
                auto vehicleWorldTransform = vehicle->getWorldTransform();
                auto body = vehicle->getBody();

                auto localPos = m_localTransform.getPosition();
                auto pos = m_worldTransform.getPosition();
                auto up = vehicleWorldTransform.up();

                auto onGround = false;
                auto ray = Ray3( pos, -up );

                if( body->castWorldRay( ray, m_hit ) )
                {
                    auto hitDistance = m_hit->getDistance();
                    if( hitDistance > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        if( hitDistance < m_suspensionDistance + m_radius )
                        {
                            onGround = true;
                        }
                    }
                }

                if( onGround )
                {
                    // Calculate suspension compression and forces
                    auto compression = m_suspensionDistance - ( m_hit->getDistance() - m_radius );
                    compression = Math<physics_Num>::clamp( compression, static_cast<physics_Num>( 0.0 ),
                                                            m_suspensionDistance );

                    // Suspension force calculation
                    const Vector3<physics_Num> gravity( 0.0, 9.81, 0.0 );
                    const auto vehicleMass = body->getMass();
                    auto fullCompressionSpringForce =
                        vehicleMass * m_massFraction * static_cast<physics_Num>( 2.0 ) * gravity.Y();

                    auto springForceMagnitude = compression * fullCompressionSpringForce;
                    auto dampingForceMagnitude = -m_damping * m_wheelVelocity.Y();
                    auto totalSuspensionForce = springForceMagnitude + dampingForceMagnitude;

                    // Apply suspension force
                    body->addLocalForceAtLocalPosition(
                        totalSuspensionForce * Vector3<physics_Num>::unitY(), localPos );

                    // Get ground velocity and transform to wheel coordinates
                    auto steeringAngle = getSteeringAngle();
                    auto groundVelocityWorld = body->getPointVelocity( m_worldTransform.getPosition() );
                    auto wheelOrientation =
                        Quaternion<physics_Num>::eulerDegrees( 0.0, steeringAngle, 0.0 );
                    auto groundVelocity =
                        wheelOrientation *
                        vehicleWorldTransform.inverseTransformVector( groundVelocityWorld );

                    // Calculate slip values
                    auto wheelCircumferentialVelocity = m_angularVelocity * m_radius;
                    auto longitudinalVelocity = groundVelocity.Z();
                    auto lateralVelocity = groundVelocity.X();

                    // Longitudinal slip ratio calculation
                    auto slipRatio = static_cast<physics_Num>( 0.0 );
                    if( Math<physics_Num>::Abs( longitudinalVelocity ) >
                        std::numeric_limits<physics_Num>::epsilon() )
                    {
                        slipRatio = ( wheelCircumferentialVelocity - longitudinalVelocity ) /
                                    Math<physics_Num>::Abs( longitudinalVelocity );
                        slipRatio =
                            Math<physics_Num>::clamp( slipRatio, static_cast<physics_Num>( -1.0 ),
                                                      static_cast<physics_Num>( 1.0 ) );
                    }

                    // Lateral slip angle calculation
                    auto slipAngle = static_cast<physics_Num>( 0.0 );
                    auto speedThreshold = static_cast<physics_Num>( 0.1 );
                    if( groundVelocity.length() > speedThreshold )
                    {
                        slipAngle = Math<physics_Num>::ATan2(
                            lateralVelocity, Math<physics_Num>::Abs( longitudinalVelocity ) );
                        slipAngle = Math<physics_Num>::clamp(
                            slipAngle, static_cast<physics_Num>( -Math<physics_Num>::pi() / 6.0 ),
                            static_cast<physics_Num>( Math<physics_Num>::pi() / 6.0 ) );
                    }

                    // Normal force calculation
                    auto normalForce =
                        Math<physics_Num>::max( totalSuspensionForce, static_cast<physics_Num>( 0.0 ) );

                    // Brush tire model parameters
                    const auto brushLength = static_cast<physics_Num>( 0.15 );  // Contact patch length
                    const auto longitudinalStiffness = static_cast<physics_Num>( 12.0 );
                    const auto lateralStiffness = static_cast<physics_Num>( 10.0 );
                    const auto frictionCoeff = m_grip;

                    // Calculate maximum available friction force
                    auto maxFrictionForce = normalForce * frictionCoeff;

                    // Brush model longitudinal force calculation
                    auto longitudinalForce = static_cast<physics_Num>( 0.0 );
                    auto absSlipRatio = Math<physics_Num>::Abs( slipRatio );

                    if( absSlipRatio > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        auto criticalSlipRatio =
                            maxFrictionForce / ( longitudinalStiffness * brushLength );

                        if( absSlipRatio < criticalSlipRatio )
                        {
                            // Linear region - tire not sliding
                            longitudinalForce = longitudinalStiffness * brushLength * slipRatio;
                        }
                        else
                        {
                            // Sliding region - constant friction
                            longitudinalForce = Math<physics_Num>::Sign( slipRatio ) * maxFrictionForce *
                                                ( static_cast<physics_Num>( 1.0 ) -
                                                  criticalSlipRatio / ( static_cast<physics_Num>( 3.0 ) *
                                                                        absSlipRatio ) );
                        }
                    }

                    // Brush model lateral force calculation
                    auto lateralForce = static_cast<physics_Num>( 0.0 );
                    auto absSlipAngle = Math<physics_Num>::Abs( slipAngle );

                    if( absSlipAngle > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        auto criticalSlipAngle = maxFrictionForce / ( lateralStiffness * brushLength );

                        if( absSlipAngle < criticalSlipAngle )
                        {
                            // Linear region - tire not sliding
                            lateralForce = -lateralStiffness * brushLength * slipAngle;
                        }
                        else
                        {
                            // Sliding region - constant friction
                            lateralForce = -Math<physics_Num>::Sign( slipAngle ) * maxFrictionForce *
                                           ( static_cast<physics_Num>( 1.0 ) -
                                             criticalSlipAngle /
                                                 ( static_cast<physics_Num>( 3.0 ) * absSlipAngle ) );
                        }
                    }

                    // Apply friction circle constraint for combined slip
                    auto combinedForce = Math<physics_Num>::Sqrt( longitudinalForce * longitudinalForce +
                                                                  lateralForce * lateralForce );

                    if( combinedForce > maxFrictionForce &&
                        combinedForce > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        auto scale = maxFrictionForce / combinedForce;
                        longitudinalForce *= scale;
                        lateralForce *= scale;
                    }

                    // Apply tire forces to vehicle body
                    Vector3<physics_Num> tireForce( lateralForce, 0.0, longitudinalForce );
                    body->addLocalForceAtLocalPosition( tireForce, localPos );

                    // Calculate torque from tire forces
                    auto torqueFromTireForce = longitudinalForce * m_radius;

                    // Calculate braking torque
                    auto brakingTorque = static_cast<physics_Num>( 0.0 );
                    if( m_brake > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        auto maxBrakeTorque = m_brakeFrictionTorque * m_brake;

                        if( Math<physics_Num>::Abs( m_angularVelocity ) >
                            std::numeric_limits<physics_Num>::epsilon() )
                        {
                            brakingTorque =
                                -Math<physics_Num>::Sign( m_angularVelocity ) * maxBrakeTorque;

                            // Prevent over-braking
                            auto maxAllowedBrakeTorque = Math<physics_Num>::Abs(
                                m_angularVelocity * m_wheelInertia / static_cast<physics_Num>( dt ) );
                            brakingTorque = Math<physics_Num>::clamp(
                                brakingTorque, -maxAllowedBrakeTorque, maxAllowedBrakeTorque );
                        }
                    }

                    // Calculate handbrake torque
                    auto handbrakeTorque = static_cast<physics_Num>( 0.0 );
                    if( m_handbrake > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        auto maxHandbrakeTorque = m_handbrakeFrictionTorque * m_handbrake;

                        if( Math<physics_Num>::Abs( m_angularVelocity ) >
                            std::numeric_limits<physics_Num>::epsilon() )
                        {
                            handbrakeTorque =
                                -Math<physics_Num>::Sign( m_angularVelocity ) * maxHandbrakeTorque;

                            // Prevent over-braking
                            auto maxAllowedHandbrakeTorque = Math<physics_Num>::Abs(
                                m_angularVelocity * m_wheelInertia / static_cast<physics_Num>( dt ) );
                            handbrakeTorque = Math<physics_Num>::clamp(
                                handbrakeTorque, -maxAllowedHandbrakeTorque, maxAllowedHandbrakeTorque );
                        }
                    }

                    // Update wheel angular velocity
                    auto netTorque = m_torque - torqueFromTireForce + brakingTorque + handbrakeTorque;
                    m_angularVelocity += ( netTorque / m_wheelInertia ) * static_cast<physics_Num>( dt );

                    // Apply rolling resistance
                    m_angularVelocity *= ( static_cast<physics_Num>( 1.0 ) -
                                           m_rollingResistance * static_cast<physics_Num>( dt ) );

                    // Update wheel velocity for next frame
                    m_wheelVelocity = groundVelocity;
                }
                else
                {
                    // Wheel not on ground - apply air resistance and throttle
                    if( m_torque > std::numeric_limits<physics_Num>::epsilon() )
                    {
                        m_angularVelocity +=
                            ( m_torque / m_wheelInertia ) * static_cast<physics_Num>( dt );
                    }

                    m_angularVelocity *= ( static_cast<physics_Num>( 1.0 ) -
                                           m_airResistance * static_cast<physics_Num>( dt ) );
                    m_wheelVelocity = Vector3<physics_Num>::zero();
                }
            }
        }

        void WheelComponent::addTorque( real_Num torque )
        {
            m_torque += torque;
        }

        void WheelComponent::setTorque( real_Num torque )
        {
            m_torque = torque;
        }

        real_Num WheelComponent::getTorque() const
        {
            return m_torque;
        }

        real_Num WheelComponent::getMass() const
        {
            return m_mass;
        }

        void WheelComponent::setMass( real_Num mass )
        {
            m_mass = mass;
        }

        real_Num WheelComponent::getSpringRate() const
        {
            return m_springRate;
        }

        void WheelComponent::setSpringRate( real_Num springRate )
        {
            m_springRate = springRate;
        }

        real_Num WheelComponent::getRadius() const
        {
            return m_radius;
        }

        void WheelComponent::setRadius( real_Num radius )
        {
            m_radius = radius;
        }

        real_Num WheelComponent::getSuspensionTravel() const
        {
            return m_suspensionDistance;
        }

        void WheelComponent::setSuspensionTravel( real_Num suspensionTravel )
        {
            m_suspensionDistance = suspensionTravel;
        }

        real_Num WheelComponent::getDamping() const
        {
            return m_damping;
        }

        void WheelComponent::setDamping( real_Num damping )
        {
            m_damping = damping;
        }

        real_Num WheelComponent::getSuspensionDistance() const
        {
            return m_suspensionDistance;
        }

        void WheelComponent::setSuspensionDistance( real_Num suspensionDistance )
        {
            m_suspensionDistance = suspensionDistance;
        }

        real_Num WheelComponent::getSteeringAngle() const
        {
            return m_steeringAngle;
        }

        void WheelComponent::setSteeringAngle( real_Num steeringAngle )
        {
            m_steeringAngle = steeringAngle;
        }

        bool WheelComponent::isSteeringWheel() const
        {
            return m_isSteeringWheel;
        }

        void WheelComponent::setSteeringWheel( bool steeringWheel )
        {
            m_isSteeringWheel = steeringWheel;
        }

        bool WheelComponent::isPoweredWheel() const
        {
            return m_isPoweredWheel;
        }

        void WheelComponent::setPoweredWheel( bool poweredWheel )
        {
            m_isPoweredWheel = poweredWheel;
        }

        physics_Num WheelComponent::getAngularVelocity() const
        {
            return m_angularVelocity;
        }

        void WheelComponent::setAngularVelocity( physics_Num angularVelocity )
        {
            m_angularVelocity = angularVelocity;
        }

        physics_Num WheelComponent::getBrake() const
        {
            return m_brake;
        }

        void WheelComponent::setBrake( physics_Num brake )
        {
            m_brake = brake;
        }

        TireModel WheelComponent::getTireModel() const
        {
            return m_tireModel;
        }

        void WheelComponent::setTireModel( TireModel tireModel )
        {
            m_tireModel = tireModel;
        }
    }  // namespace vehicle
}  // namespace workphone
