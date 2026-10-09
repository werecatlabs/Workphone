#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/DCSHelicopterController.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED(workphone::vehicle, DCSHelicopterController,
                              CAerodynamicsVehicle<IAircraft>);

    // ===================================================================
    // Channel indices (standard CAerodynamicsVehicle layout)
    // ===================================================================
    static constexpr s32 kThrChannel = 0; ///< Throttle      [0, 1]
    static constexpr s32 kAilChannel = 1; ///< Roll          [-1, 1]
    static constexpr s32 kEleChannel = 2; ///< Pitch         [-1, 1]
    static constexpr s32 kYawChannel = 3; ///< Yaw / pedal   [-1, 1]
    static constexpr s32 kColChannel = 5; ///< Collective    [-1, 1]

    // Retreating blade stall speed band (m/s)
    static constexpr real_Num kRBSOnsetSpeed = 60.0;
    static constexpr real_Num kRBSFullSpeed = 100.0;

    // Translational lift speed band (m/s)
    static constexpr real_Num kETLOnset = 10.0;
    static constexpr real_Num kETLPeak = 35.0;
    static constexpr real_Num kETLBoost = 1.4;

    // ===================================================================
    // Construction / destruction
    // ===================================================================

    DCSHelicopterController::DCSHelicopterController()
    {
        // Derive disc area from default radius on construction.
        m_rotorArea = Math<real_Num>::pi() * m_rotorRadius * m_rotorRadius;
    }

    DCSHelicopterController::~DCSHelicopterController() = default;

    // ===================================================================
    // Lifecycle
    // ===================================================================

    bool DCSHelicopterController::isValid() const
    {
        return true;
    }

    void DCSHelicopterController::load(SmartPtr<ISharedObject> data)
    {
        CAerodynamicsVehicle<IAircraft>::load(data);
    }

    void DCSHelicopterController::update()
    {
        CAerodynamicsVehicle<IAircraft>::update();
    }

    void DCSHelicopterController::update(const double &t, const double &dt)
    {
        // Clear force/torque accumulators from the previous frame.
        clearForces();

        // Derive world-space body axes from the current orientation.
        const auto &worldTransform = getWorldTransform();
        const auto orientation = worldTransform.getOrientation();

        const Vector3<real_Num> worldUp = orientation * Vector3<real_Num>::unitY();
        const Vector3<real_Num> worldFwd = orientation * Vector3<real_Num>::unitZ();
        const Vector3<real_Num> worldRight = orientation * Vector3<real_Num>::unitX();

        const real_Num fdt = static_cast<real_Num>(dt);

        // Run DCS-style aerodynamic subsystems in the same order as the
        // original C# FixedUpdate().
        updateRotorRPM(fdt);
        updateDynamicInflow(fdt);
        updateBladeFlapping(fdt, worldFwd, worldRight);
        applyRotorForces(worldUp, worldFwd, worldRight);
        applyTailRotor(worldUp);
        applySAS();

        // Dispatch accumulated forces and torques to the physics body.
        CAerodynamicsVehicle<IAircraft>::update();
    }

    // ===================================================================
    // ROTOR RPM PHYSICS
    // ===================================================================
    // C# reference:
    //   engineTorque = throttle * maxEngineTorque;
    //   dragTorque   = rotorDrag * rotorRPM;
    //   accel        = (engineTorque - dragTorque) / rotorInertia;
    //   rotorRPM    += accel * dt * 60;
    // ===================================================================

    void DCSHelicopterController::updateRotorRPM(real_Num dt)
    {
        const real_Num throttle = Math<real_Num>::clamp(
            getChannel(kThrChannel), 0.0, 1.0);

        m_engineTorque = throttle * m_maxEngineTorque;

        const real_Num dragTorque = m_rotorDrag * m_rotorRPM;
        const real_Num netTorque = m_engineTorque - dragTorque;

        const real_Num angularAccel = (m_rotorInertia > static_cast<real_Num>(0.0))
                                          ? (netTorque / m_rotorInertia)
                                          : static_cast<real_Num>(0.0);

        // Convert rad/s² to RPM/s and integrate (matching × 60 from C#).
        m_rotorRPM += angularAccel * dt * static_cast<real_Num>(60.0);
        m_rotorRPM =
            Math<real_Num>::clamp(m_rotorRPM, 0.0, m_maxRotorRPM);
    }

    // ===================================================================
    // DYNAMIC INFLOW MODEL
    // ===================================================================
    // C# reference:
    //   tipSpeed      = rotorRPM * 2π * rotorRadius / 60
    //   collectiveRad = collective * Deg2Rad * maxCollectivePitch
    //   thrustEst     = 0.5 * � * A * v_tip² * θ
    //   targetInduced = sqrt(|thrustEst| / (2 * � * A))
    //   inducedVelocity = Lerp(inducedVelocity, targetInduced, dt * inflowLag)
    // ===================================================================

    void DCSHelicopterController::updateDynamicInflow(real_Num dt)
    {
        const real_Num two_pi = static_cast<real_Num>(2.0) * Math<real_Num>::pi();

        // Blade tip tangential speed (m/s).
        const real_Num tipSpeed =
            m_rotorRPM * two_pi * m_rotorRadius / static_cast<real_Num>(60.0);

        // Effective collective pitch in radians.
        const real_Num collectiveIn = Math<real_Num>::clamp(
            getChannel(kColChannel), -1.0, 1.0);
        const real_Num collectiveRad =
            collectiveIn * Math<real_Num>::deg_to_rad() * m_maxCollectivePitch;

        // Simplified thrust estimate from momentum theory:
        //   T ≈ 0.5 · � · A · v_tip² · θ
        const real_Num thrustEstimate = static_cast<real_Num>(0.5) * m_airDensity * m_rotorArea *
                                        tipSpeed * tipSpeed * collectiveRad;

        // Momentum-theory induced velocity target:
        //   vi = sqrt(|T| / (2 · � · A))
        const real_Num denominator = static_cast<real_Num>(2.0) * m_airDensity * m_rotorArea;
        real_Num targetInduced = 0.0;
        if(denominator > static_cast<real_Num>(0.0))
            targetInduced =
                Math<real_Num>::Sqrt(Math<real_Num>::Abs(thrustEstimate) / denominator);

        // First-order lag toward the momentum-theory target.
        const real_Num alpha = Math<real_Num>::clamp(dt * m_inflowLag, 0.0,
                                                     1.0);
        m_inducedVelocity = m_inducedVelocity + alpha * (targetInduced - m_inducedVelocity);
    }

    // ===================================================================
    // BLADE FLAPPING MODEL
    // ===================================================================
    // C# reference:
    //   lateralVelocity = InverseTransformDirection(rb.velocity)
    //   targetFlap      = (-vz * 0.05, 0, vx * 0.05)
    //   flappingOffset  = Lerp(flappingOffset, targetFlap, dt * flappingStiffness)
    // ===================================================================

    void DCSHelicopterController::updateBladeFlapping(real_Num dt,
                                                      const Vector3<real_Num> &worldFwd,
                                                      const Vector3<real_Num> &worldRight)
    {
        const Vector3<real_Num> worldVel = getLinearVelocity();

        // Project world velocity onto body axes to get local components.
        const real_Num localX = worldVel.dotProduct(worldRight); // right component
        const real_Num localZ = worldVel.dotProduct(worldFwd); // forward component

        // Flapping disc tilt in body forward/right directions (matching C#).
        const Vector3<real_Num> targetFlap = worldFwd * (-localZ * static_cast<real_Num>(0.05)) +
                                             worldRight * (localX * static_cast<real_Num>(0.05));

        // First-order convergence toward aerodynamic equilibrium.
        const real_Num alpha = Math<real_Num>::clamp(
            dt * m_flappingStiffness, 0.0, 1.0);
        m_flappingOffset = m_flappingOffset + alpha * (targetFlap - m_flappingOffset);
    }

    // ===================================================================
    // MAIN ROTOR FORCES
    // ===================================================================
    // C# reference:
    //   thrust     = 2 * � * A * vi * (vi - vy)
    //   thrust    *= translationalLift * retreatingBladeStall
    //   discNormal = up + fwd*pitch*tilt + right*roll*tilt + flappingOffset
    //   AddForce(discNormal.normalized * thrust)
    //   AddTorque(-up * thrust * 0.02)
    // ===================================================================

    void DCSHelicopterController::applyRotorForces(const Vector3<real_Num> &up,
                                                   const Vector3<real_Num> &fwd,
                                                   const Vector3<real_Num> &right)
    {
        const real_Num pitchIn = getChannel(kEleChannel);
        const real_Num rollIn = getChannel(kAilChannel);

        // World-space vertical velocity component (positive = climbing).
        const real_Num vy = getLinearVelocity().dotProduct(up);

        // Momentum-theory thrust:
        //   T = 2 · � · A · vi · (vi − vy)
        real_Num thrust = static_cast<real_Num>(2.0) * m_airDensity * m_rotorArea *
                          m_inducedVelocity * (m_inducedVelocity - vy);

        // Modifiers (matching C# order).
        const real_Num airspeed = getLinearVelocity().length();
        thrust *= computeTranslationalLift(airspeed);

        const real_Num forwardSpeed = getLinearVelocity().dotProduct(fwd);
        m_retreatingFactor = computeRetreatingBladeStall(forwardSpeed);
        thrust *= m_retreatingFactor;

        // Rotor disc normal: body-up biased by cyclic tilt and blade flapping.
        const real_Num tiltRad = m_cyclicMaxTilt * Math<real_Num>::deg_to_rad();
        Vector3<real_Num> discNormal =
            up + fwd * (pitchIn * tiltRad) + right * (rollIn * tiltRad) + m_flappingOffset;
        discNormal.normalise();

        // Apply thrust along the tilted disc normal through the CG.
        addForce(0, discNormal * thrust, getPosition());

        // Reactive torque: main rotor torques the fuselage in the opposite
        // sense to rotor rotation.
        addTorque(0, -up * (thrust * static_cast<real_Num>(0.02)));
    }

    // ===================================================================
    // TAIL ROTOR
    // ===================================================================
    // C# reference:
    //   antiTorque = rotorRPM * 0.7
    //   pedal      = yaw * tailRotorPower
    //   AddTorque(up * (antiTorque + pedal))
    // ===================================================================

    void DCSHelicopterController::applyTailRotor(const Vector3<real_Num> &up)
    {
        const real_Num yawIn = getChannel(kYawChannel);

        // Anti-torque moment from the tail rotor (proportional to main rotor speed).
        const real_Num antiTorque = m_rotorRPM * static_cast<real_Num>(0.7);

        // Pilot pedal authority.
        const real_Num pedalMoment = yawIn * m_tailRotorPower;

        addTorque(0, up * (antiTorque + pedalMoment));
    }

    // ===================================================================
    // STABILITY AUGMENTATION SYSTEM (SAS)
    // ===================================================================
    // C# reference:
    //   AddTorque(-rb.angularVelocity * angularDamping)
    // ===================================================================

    void DCSHelicopterController::applySAS()
    {
        const Vector3<real_Num> angVel = getAngularVelocity();
        addTorque(0, -angVel * m_angularDamping);
    }

    // ===================================================================
    // TRANSLATIONAL LIFT
    // ===================================================================
    // C# reference:
    //   speed < 10  → 1
    //   else        → Lerp(1, 1.4, (speed - 10) / 25)
    // ===================================================================

    real_Num DCSHelicopterController::computeTranslationalLift(real_Num speed) const
    {
        if(speed <= kETLOnset)
            return 1.0;

        const real_Num t =
            Math<real_Num>::clamp((speed - kETLOnset) / (kETLPeak - kETLOnset),
                                  0.0, 1.0);

        return static_cast<real_Num>(1.0) + t * (kETLBoost - static_cast<real_Num>(1.0));
    }

    // ===================================================================
    // RETREATING BLADE STALL
    // ===================================================================
    // C# reference:
    //   forwardSpeed < 60  → 1
    //   else               → Lerp(1, 0.5, (forwardSpeed - 60) / 40)
    // ===================================================================

    real_Num DCSHelicopterController::computeRetreatingBladeStall(real_Num forwardSpeed) const
    {
        if(forwardSpeed < kRBSOnsetSpeed)
            return 1.0;

        const real_Num t = Math<real_Num>::clamp(
            (forwardSpeed - kRBSOnsetSpeed) / (kRBSFullSpeed - kRBSOnsetSpeed),
            0.0, 1.0);

        return static_cast<real_Num>(1.0) - t * static_cast<real_Num>(0.5); // 1 → 0.5
    }

    // ===================================================================
    // Rotor parameter accessors
    // ===================================================================

    real_Num DCSHelicopterController::getRotorRadius() const
    {
        return m_rotorRadius;
    }

    void DCSHelicopterController::setRotorRadius(real_Num rotorRadius)
    {
        m_rotorRadius = rotorRadius;
        m_rotorArea = Math<real_Num>::pi() * rotorRadius * rotorRadius;
    }

    real_Num DCSHelicopterController::getRotorArea() const
    {
        return m_rotorArea;
    }

    real_Num DCSHelicopterController::getRotorInertia() const
    {
        return m_rotorInertia;
    }

    void DCSHelicopterController::setRotorInertia(real_Num rotorInertia)
    {
        m_rotorInertia = rotorInertia;
    }

    real_Num DCSHelicopterController::getMaxEngineTorque() const
    {
        return m_maxEngineTorque;
    }

    void DCSHelicopterController::setMaxEngineTorque(real_Num maxEngineTorque)
    {
        m_maxEngineTorque = maxEngineTorque;
    }

    real_Num DCSHelicopterController::getRotorDrag() const
    {
        return m_rotorDrag;
    }

    void DCSHelicopterController::setRotorDrag(real_Num rotorDrag)
    {
        m_rotorDrag = rotorDrag;
    }

    real_Num DCSHelicopterController::getMaxRotorRPM() const
    {
        return m_maxRotorRPM;
    }

    void DCSHelicopterController::setMaxRotorRPM(real_Num maxRotorRPM)
    {
        m_maxRotorRPM = maxRotorRPM;
    }

    real_Num DCSHelicopterController::getRotorRPM() const
    {
        return m_rotorRPM;
    }

    // ===================================================================
    // Aerodynamic parameter accessors
    // ===================================================================

    real_Num DCSHelicopterController::getAirDensity() const
    {
        return m_airDensity;
    }

    void DCSHelicopterController::setAirDensity(real_Num airDensity)
    {
        m_airDensity = airDensity;
    }

    // ===================================================================
    // Blade pitch accessors
    // ===================================================================

    real_Num DCSHelicopterController::getMaxCollectivePitch() const
    {
        return m_maxCollectivePitch;
    }

    void DCSHelicopterController::setMaxCollectivePitch(real_Num maxCollectivePitch)
    {
        m_maxCollectivePitch = maxCollectivePitch;
    }

    real_Num DCSHelicopterController::getCyclicMaxTilt() const
    {
        return m_cyclicMaxTilt;
    }

    void DCSHelicopterController::setCyclicMaxTilt(real_Num cyclicMaxTilt)
    {
        m_cyclicMaxTilt = cyclicMaxTilt;
    }

    // ===================================================================
    // Dynamic inflow accessors
    // ===================================================================

    real_Num DCSHelicopterController::getInflowLag() const
    {
        return m_inflowLag;
    }

    void DCSHelicopterController::setInflowLag(real_Num inflowLag)
    {
        m_inflowLag = inflowLag;
    }

    real_Num DCSHelicopterController::getInducedVelocity() const
    {
        return m_inducedVelocity;
    }

    // ===================================================================
    // Blade flapping accessors
    // ===================================================================

    real_Num DCSHelicopterController::getFlappingStiffness() const
    {
        return m_flappingStiffness;
    }

    void DCSHelicopterController::setFlappingStiffness(real_Num flappingStiffness)
    {
        m_flappingStiffness = flappingStiffness;
    }

    Vector3<real_Num> DCSHelicopterController::getFlappingOffset() const
    {
        return m_flappingOffset;
    }

    // ===================================================================
    // Tail rotor accessors
    // ===================================================================

    real_Num DCSHelicopterController::getTailRotorPower() const
    {
        return m_tailRotorPower;
    }

    void DCSHelicopterController::setTailRotorPower(real_Num tailRotorPower)
    {
        m_tailRotorPower = tailRotorPower;
    }

    // ===================================================================
    // SAS accessors
    // ===================================================================

    real_Num DCSHelicopterController::getAngularDamping() const
    {
        return m_angularDamping;
    }

    void DCSHelicopterController::setAngularDamping(real_Num angularDamping)
    {
        m_angularDamping = angularDamping;
    }

    // ===================================================================
    // Retreating blade stall query
    // ===================================================================

    real_Num DCSHelicopterController::getRetreatingBladeStallFactor() const
    {
        return m_retreatingFactor;
    }

    // ===================================================================
    // IAircraftPowerUnit / IVehiclePowerUnit passthrough stubs
    // ===================================================================

    real_Num DCSHelicopterController::getRPM() const
    {
        return m_rotorRPM;
    }

    void DCSHelicopterController::setRPM(real_Num rpm)
    {
        m_rotorRPM = Math<real_Num>::clamp(rpm, 0.0, m_maxRotorRPM);
    }

    real_Num DCSHelicopterController::getThrottle() const
    {
        return Math<real_Num>::clamp(getChannel(kThrChannel), 0.0,
                                     1.0);
    }

    void DCSHelicopterController::setThrottle(real_Num throttle)
    {
        setChannel(kThrChannel, throttle);
    }

    real_Num DCSHelicopterController::getMoi() const
    {
        return m_moi;
    }

    void DCSHelicopterController::setMoi(real_Num moi)
    {
        m_moi = moi;
    }

    real_Num DCSHelicopterController::getThrustMultiplier() const
    {
        return m_thrustMultiplier;
    }

    void DCSHelicopterController::setThrustMultiplier(real_Num thrustMultiplier)
    {
        m_thrustMultiplier = thrustMultiplier;
    }

    real_Num DCSHelicopterController::getTorqueMultiplier() const
    {
        return m_torqueMultiplier;
    }

    void DCSHelicopterController::setTorqueMultiplier(real_Num torqueMultiplier)
    {
        m_torqueMultiplier = torqueMultiplier;
    }

    real_Num DCSHelicopterController::getPeakPowerW() const
    {
        return m_peakPowerW;
    }

    void DCSHelicopterController::setPeakPowerW(real_Num peakPowerW)
    {
        m_peakPowerW = peakPowerW;
    }

    real_Num DCSHelicopterController::getTorque(f32 /*throttlePosition*/) const
    {
        return m_engineTorque;
    }

    real_Num DCSHelicopterController::getMaxTorque(u32 /*rpm*/) const
    {
        return m_maxEngineTorque;
    }

    real_Num DCSHelicopterController::getMinTorque(u32 /*rpm*/) const
    {
        return 0.0;
    }

    real_Num DCSHelicopterController::getTorque() const
    {
        return m_engineTorque;
    }

    real_Num DCSHelicopterController::getEngineRPM(int /*idx*/) const
    {
        return m_rotorRPM;
    }

    real_Num DCSHelicopterController::getThrust(int /*idx*/) const
    {
        return 0.0;
    }

    // ===================================================================
    // IAircraft component accessors
    // ===================================================================

    SmartPtr<IAircraftCallback> DCSHelicopterController::getCallback() const
    {
        return m_aircraftCallback;
    }

    void DCSHelicopterController::setCallback(SmartPtr<IAircraftCallback> callback)
    {
        m_aircraftCallback = callback;
    }

    SmartPtr<IBatteryPack> DCSHelicopterController::getBatteryPack() const
    {
        return m_batteryPack;
    }

    void DCSHelicopterController::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_batteryPack = batteryPack;
    }

    SmartPtr<IAerodymanicsWind> DCSHelicopterController::getWind() const
    {
        return m_wind;
    }

    void DCSHelicopterController::setWind(SmartPtr<IAerodymanicsWind> wind)
    {
        m_wind = wind;
    }

    Transform3<real_Num> DCSHelicopterController::getBodyTransform() const
    {
        return m_bodyTransform;
    }

    void DCSHelicopterController::setBodyTransform(Transform3<real_Num> bodyTransform)
    {
        m_bodyTransform = bodyTransform;
    }

    real_Num DCSHelicopterController::getRollwiseDamping() const
    {
        return m_rollwiseDamping;
    }

    void DCSHelicopterController::setRollwiseDamping(real_Num rollwiseDamping)
    {
        m_rollwiseDamping = rollwiseDamping;
    }

    // ===================================================================
    // Propeller unit management
    // ===================================================================

    void DCSHelicopterController::addPropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        if(propellerUnit)
            m_propellerUnits.push_back(propellerUnit);
    }

    void DCSHelicopterController::removePropellerUnit(
        SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        auto it = std::find(m_propellerUnits.begin(), m_propellerUnits.end(), propellerUnit);
        if(it != m_propellerUnits.end())
            m_propellerUnits.erase(it);
    }

    Array<SmartPtr<IAircraftPropellerUnit>> DCSHelicopterController::getPropellerUnits() const
    {
        return m_propellerUnits;
    }

    void DCSHelicopterController::setPropellerUnits(
        const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits)
    {
        m_propellerUnits = propellerUnits;
    }

    // ===================================================================
    // Wheel management
    // ===================================================================

    void DCSHelicopterController::addWheel(SmartPtr<IWheelComponent> wheel)
    {
        if(wheel)
            m_wheels.push_back(wheel);
    }

    void DCSHelicopterController::removeWheel(SmartPtr<IWheelComponent> wheel)
    {
        auto it = std::find(m_wheels.begin(), m_wheels.end(), wheel);
        if(it != m_wheels.end())
            m_wheels.erase(it);
    }

    Array<SmartPtr<IWheelComponent>> DCSHelicopterController::getWheels() const
    {
        return m_wheels;
    }

    void DCSHelicopterController::setWheels(const Array<SmartPtr<IWheelComponent>> &wheels)
    {
        m_wheels = wheels;
    }

    // ===================================================================
    // Control surface / section / model path stubs
    // ===================================================================

    void DCSHelicopterController::setControlAngle(s32 /*id*/, f32 /*angle*/)
    {
        // Not used by this controller.
    }

    real_Num DCSHelicopterController::getSectionMultiplier() const
    {
        return m_sectionMultiplier;
    }

    void DCSHelicopterController::setSectionMultiplier(real_Num sectionMultiplier)
    {
        m_sectionMultiplier = sectionMultiplier;
    }

    String DCSHelicopterController::getModelDataFilePath() const
    {
        return m_modelDataFilePath;
    }

    void DCSHelicopterController::setModelDataFilePath(const String &filePath)
    {
        m_modelDataFilePath = filePath;
    }
}
