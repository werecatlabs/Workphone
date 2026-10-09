#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/ExtremeHelicopterController.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED(workphone::vehicle, ExtremeHelicopterController,
                              CAerodynamicsVehicle<IAircraft>);

    // ===================================================================
    // Channel indices (standard CAerodynamicsVehicle layout)
    // ===================================================================
    static constexpr s32 kThrChannel = 0; ///< Throttle      [0, 1]
    static constexpr s32 kAilChannel = 1; ///< Roll          [-1, 1]
    static constexpr s32 kEleChannel = 2; ///< Pitch         [-1, 1]
    static constexpr s32 kYawChannel = 3; ///< Yaw / pedal   [-1, 1]
    static constexpr s32 kColChannel = 5; ///< Collective    [-1, 1]

    // Translational lift speed band (m/s)  — matching C#: 10 → 30 m/s, 1 → 1.35
    static constexpr real_Num kETLOnset = 10.0;
    static constexpr real_Num kETLPeak = 30.0;
    static constexpr real_Num kETLBoost = 1.35;

    // VRS collective threshold (matching C#: > 0.3 normalised)
    static constexpr real_Num kVRSCollectiveThreshold = 0.3;
    static constexpr real_Num kVRSThrustFactor = 0.4;

    // Downward velocity onset for VRS check (world-space, positive = down)
    static constexpr real_Num kVRSDownwardOnset = 2.0;

    // ===================================================================
    // Construction / destruction
    // ===================================================================

    ExtremeHelicopterController::ExtremeHelicopterController()
    {
        m_rotorArea = Math<real_Num>::pi() * m_rotorRadius * m_rotorRadius;
    }

    ExtremeHelicopterController::~ExtremeHelicopterController() = default;

    // ===================================================================
    // Lifecycle
    // ===================================================================

    bool ExtremeHelicopterController::isValid() const
    {
        return true;
    }

    void ExtremeHelicopterController::load(SmartPtr<ISharedObject> data)
    {
        CAerodynamicsVehicle<IAircraft>::load(data);
    }

    void ExtremeHelicopterController::update()
    {
        CAerodynamicsVehicle<IAircraft>::update();
    }

    void ExtremeHelicopterController::update(const double &t, const double &dt)
    {
        clearForces();

        // Derive world-space body axes.
        const auto &worldTransform = getWorldTransform();
        const auto orientation = worldTransform.getOrientation();

        const Vector3<real_Num> worldUp = orientation * Vector3<real_Num>::unitY();
        const Vector3<real_Num> worldFwd = orientation * Vector3<real_Num>::unitZ();
        const Vector3<real_Num> worldRight = orientation * Vector3<real_Num>::unitX();

        const real_Num fdt = static_cast<real_Num>(dt);

        // Run subsystems in the same order as the original C# FixedUpdate().
        updateRotorRPM(fdt);
        calculateInducedVelocity();
        applyMainRotorForces(worldUp, worldFwd, worldRight);
        applyTailRotor(worldUp);

        CAerodynamicsVehicle<IAircraft>::update();
    }

    // ===================================================================
    // ROTOR RPM DYNAMICS
    // ===================================================================
    // C# reference:
    //   engineTorque = throttle * maxEngineTorque
    //   dragTorque   = rotorDragCoeff * rotorRPM
    //   angularAccel = (engineTorque - dragTorque) / rotorInertia
    //   rotorRPM    += angularAccel * dt * 60
    // ===================================================================

    void ExtremeHelicopterController::updateRotorRPM(real_Num dt)
    {
        const real_Num throttle = Math<real_Num>::clamp(
            getChannel(kThrChannel), 0.0, 1.0);

        m_engineTorque = throttle * m_maxEngineTorque;

        const real_Num dragTorque = m_rotorDragCoeff * m_rotorRPM;
        const real_Num netTorque = m_engineTorque - dragTorque;

        const real_Num angularAccel = (m_rotorInertia > static_cast<real_Num>(0.0))
                                          ? (netTorque / m_rotorInertia)
                                          : static_cast<real_Num>(0.0);

        m_rotorRPM += angularAccel * dt * static_cast<real_Num>(60.0);
        m_rotorRPM =
            Math<real_Num>::clamp(m_rotorRPM, 0.0, m_maxRotorRPM);
    }

    // ===================================================================
    // INDUCED FLOW CALCULATION
    // ===================================================================
    // C# reference:
    //   collectiveRad = collective * Deg2Rad * maxCollectivePitch
    //   CL            = bladeLiftSlope * collectiveRad
    //   tipSpeed      = rotorRPM * 2π * rotorRadius / 60
    //   thrustGuess   = 0.5 * ρ * A * tipSpeed² * CL
    //   inducedVelocity = sqrt(|thrustGuess| / (2 * ρ * A))
    // ===================================================================

    void ExtremeHelicopterController::calculateInducedVelocity()
    {
        const real_Num two_pi = static_cast<real_Num>(2.0) * Math<real_Num>::pi();

        const real_Num collectiveIn = Math<real_Num>::clamp(
            getChannel(kColChannel), -1.0, 1.0);

        const real_Num collectiveRad =
            collectiveIn * Math<real_Num>::deg_to_rad() * m_maxCollectivePitch;

        const real_Num liftCoeff = m_bladeLiftSlope * collectiveRad;

        const real_Num tipSpeed =
            m_rotorRPM * two_pi * m_rotorRadius / static_cast<real_Num>(60.0);

        const real_Num thrustGuess = static_cast<real_Num>(0.5) * m_airDensity * m_rotorArea *
                                     tipSpeed * tipSpeed * liftCoeff;

        const real_Num denominator = static_cast<real_Num>(2.0) * m_airDensity * m_rotorArea;
        m_inducedVelocity =
            (denominator > static_cast<real_Num>(0.0))
                ? Math<real_Num>::Sqrt(Math<real_Num>::Abs(thrustGuess) / denominator)
                : static_cast<real_Num>(0.0);
    }

    // ===================================================================
    // MAIN ROTOR FORCES
    // ===================================================================
    // C# reference:
    //   thrust = 0.5 * ρ * A * tipSpeed² * (bladeLiftSlope * collectiveRad)
    //   thrust *= groundEffect * translationalLift * vortexRingState
    //   discNormal = up + fwd*pitch*tiltRad + right*roll*tiltRad
    //   AddForce(discNormal.normalized * thrust)
    //   AddTorque(-up * thrust * 0.015)
    // ===================================================================

    void ExtremeHelicopterController::applyMainRotorForces(const Vector3<real_Num> &up,
                                                           const Vector3<real_Num> &fwd,
                                                           const Vector3<real_Num> &right)
    {
        const real_Num pitchIn = getChannel(kEleChannel);
        const real_Num rollIn = getChannel(kAilChannel);

        const real_Num collectiveIn = Math<real_Num>::clamp(
            getChannel(kColChannel), -1.0, 1.0);

        const real_Num two_pi = static_cast<real_Num>(2.0) * Math<real_Num>::pi();

        const real_Num tipSpeed =
            m_rotorRPM * two_pi * m_rotorRadius / static_cast<real_Num>(60.0);

        const real_Num collectiveRad =
            collectiveIn * Math<real_Num>::deg_to_rad() * m_maxCollectivePitch;

        const real_Num liftCoeff = m_bladeLiftSlope * collectiveRad;

        real_Num thrust = static_cast<real_Num>(0.5) * m_airDensity * m_rotorArea * tipSpeed *
                          tipSpeed * liftCoeff;

        // Apply modifiers (matching C# order).
        thrust *= computeGroundEffect();
        thrust *= computeTranslationalLift(getLinearVelocity().length());
        thrust *= computeVortexRingState(collectiveIn);

        // Rotor disc tilt from cyclic input.
        const real_Num tiltRad = m_cyclicTiltMax * Math<real_Num>::deg_to_rad();
        Vector3<real_Num> discNormal =
            up + fwd * (pitchIn * tiltRad) + right * (rollIn * tiltRad);
        discNormal.normalise();

        addForce(0, discNormal * thrust, getPosition());

        // Reactive torque from main rotor.
        addTorque(0, -up * (thrust * static_cast<real_Num>(0.015)));
    }

    // ===================================================================
    // TAIL ROTOR ANTI-TORQUE
    // ===================================================================
    // C# reference:
    //   antiTorque = rotorRPM * 0.6
    //   pedalForce = yaw * tailRotorPower
    //   AddTorque(up * (antiTorque + pedalForce))
    // ===================================================================

    void ExtremeHelicopterController::applyTailRotor(const Vector3<real_Num> &up)
    {
        const real_Num yawIn = getChannel(kYawChannel);

        const real_Num antiTorque = m_rotorRPM * static_cast<real_Num>(0.6);
        const real_Num pedalMoment = yawIn * m_tailRotorPower;

        addTorque(0, up * (antiTorque + pedalMoment));
    }

    // ===================================================================
    // TRANSLATIONAL LIFT
    // ===================================================================
    // C# reference:
    //   speed < 10  → 1
    //   else        → Lerp(1, 1.35, (speed - 10) / 20)
    // ===================================================================

    real_Num ExtremeHelicopterController::computeTranslationalLift(real_Num speed) const
    {
        if(speed <= kETLOnset)
            return 1.0;

        const real_Num t =
            Math<real_Num>::clamp((speed - kETLOnset) / (kETLPeak - kETLOnset),
                                  0.0, 1.0);

        return static_cast<real_Num>(1.0) + t * (kETLBoost - static_cast<real_Num>(1.0));
    }

    // ===================================================================
    // GROUND EFFECT
    // ===================================================================
    // C# reference (raycast → distance):
    //   factor = 1 + (1 - hit.distance / groundEffectHeight)
    // Approximated here from world-space Y altitude (AGL proxy).
    // ===================================================================

    real_Num ExtremeHelicopterController::computeGroundEffect() const
    {
        const real_Num agl = getPosition().Y();

        if(agl < m_groundEffectHeight && m_groundEffectHeight > static_cast<real_Num>(0.0))
        {
            const real_Num t =
                Math<real_Num>::clamp(agl / m_groundEffectHeight, 0.0,
                                      1.0);

            // Mirrors C# formula: 1 + (1 − distance/height)
            return static_cast<real_Num>(1.0) + (static_cast<real_Num>(1.0) - t);
        }

        return 1.0;
    }

    // ===================================================================
    // VORTEX RING STATE
    // ===================================================================
    // C# reference:
    //   verticalSpeed = Dot(rb.velocity, Vector3.down)   (positive = descending)
    //   if verticalSpeed > 2  → 1
    //   if verticalSpeed < vortexDescentRate && collective > 0.3 → 0.4
    //   else → 1
    // ===================================================================

    real_Num ExtremeHelicopterController::computeVortexRingState(real_Num collectiveIn) const
    {
        // Positive downward speed = descending (dot with world-down = -worldUp).
        const Vector3<real_Num> worldVel; //= getLinearVelocity();
        const auto &orientation = getWorldTransform().getOrientation();
        const Vector3<real_Num> worldUp = orientation * Vector3<real_Num>::unitY();

        // verticalSpeed > 0 means descending (matching Unity's Vector3.down dot).
        const real_Num verticalSpeed = -worldVel.dotProduct(worldUp);

        if(verticalSpeed <= kVRSDownwardOnset)
        {
            const_cast<ExtremeHelicopterController *>(this)->m_vrsActive = false;
            return 1.0;
        }

        // Check steep powered descent triggering VRS.
        if(verticalSpeed > -m_vortexDescentRate && collectiveIn > kVRSCollectiveThreshold)
        {
            const_cast<ExtremeHelicopterController *>(this)->m_vrsActive = true;
            return kVRSThrustFactor;
        }

        const_cast<ExtremeHelicopterController *>(this)->m_vrsActive = false;
        return 1.0;
    }

    // ===================================================================
    // Rotor parameter accessors
    // ===================================================================

    real_Num ExtremeHelicopterController::getRotorRadius() const
    {
        return m_rotorRadius;
    }

    void ExtremeHelicopterController::setRotorRadius(real_Num rotorRadius)
    {
        m_rotorRadius = rotorRadius;
        m_rotorArea = Math<real_Num>::pi() * rotorRadius * rotorRadius;
    }

    real_Num ExtremeHelicopterController::getRotorArea() const
    {
        return m_rotorArea;
    }

    real_Num ExtremeHelicopterController::getRotorInertia() const
    {
        return m_rotorInertia;
    }

    void ExtremeHelicopterController::setRotorInertia(real_Num rotorInertia)
    {
        m_rotorInertia = rotorInertia;
    }

    real_Num ExtremeHelicopterController::getMaxEngineTorque() const
    {
        return m_maxEngineTorque;
    }

    void ExtremeHelicopterController::setMaxEngineTorque(real_Num maxEngineTorque)
    {
        m_maxEngineTorque = maxEngineTorque;
    }

    real_Num ExtremeHelicopterController::getRotorDragCoeff() const
    {
        return m_rotorDragCoeff;
    }

    void ExtremeHelicopterController::setRotorDragCoeff(real_Num rotorDragCoeff)
    {
        m_rotorDragCoeff = rotorDragCoeff;
    }

    real_Num ExtremeHelicopterController::getMaxRotorRPM() const
    {
        return m_maxRotorRPM;
    }

    void ExtremeHelicopterController::setMaxRotorRPM(real_Num maxRotorRPM)
    {
        m_maxRotorRPM = maxRotorRPM;
    }

    real_Num ExtremeHelicopterController::getRotorRPM() const
    {
        return m_rotorRPM;
    }

    // ===================================================================
    // Aerodynamic parameter accessors
    // ===================================================================

    real_Num ExtremeHelicopterController::getAirDensity() const
    {
        return m_airDensity;
    }

    void ExtremeHelicopterController::setAirDensity(real_Num airDensity)
    {
        m_airDensity = airDensity;
    }

    real_Num ExtremeHelicopterController::getBladeLiftSlope() const
    {
        return m_bladeLiftSlope;
    }

    void ExtremeHelicopterController::setBladeLiftSlope(real_Num bladeLiftSlope)
    {
        m_bladeLiftSlope = bladeLiftSlope;
    }

    // ===================================================================
    // Blade pitch accessors
    // ===================================================================

    real_Num ExtremeHelicopterController::getMaxCollectivePitch() const
    {
        return m_maxCollectivePitch;
    }

    void ExtremeHelicopterController::setMaxCollectivePitch(real_Num maxCollectivePitch)
    {
        m_maxCollectivePitch = maxCollectivePitch;
    }

    real_Num ExtremeHelicopterController::getCyclicTiltMax() const
    {
        return m_cyclicTiltMax;
    }

    void ExtremeHelicopterController::setCyclicTiltMax(real_Num cyclicTiltMax)
    {
        m_cyclicTiltMax = cyclicTiltMax;
    }

    // ===================================================================
    // Tail rotor accessors
    // ===================================================================

    real_Num ExtremeHelicopterController::getTailRotorPower() const
    {
        return m_tailRotorPower;
    }

    void ExtremeHelicopterController::setTailRotorPower(real_Num tailRotorPower)
    {
        m_tailRotorPower = tailRotorPower;
    }

    // ===================================================================
    // Ground effect accessors
    // ===================================================================

    real_Num ExtremeHelicopterController::getGroundEffectHeight() const
    {
        return m_groundEffectHeight;
    }

    void ExtremeHelicopterController::setGroundEffectHeight(real_Num groundEffectHeight)
    {
        m_groundEffectHeight = groundEffectHeight;
    }

    // ===================================================================
    // Vortex ring state accessors
    // ===================================================================

    real_Num ExtremeHelicopterController::getVortexDescentRate() const
    {
        return m_vortexDescentRate;
    }

    void ExtremeHelicopterController::setVortexDescentRate(real_Num vortexDescentRate)
    {
        m_vortexDescentRate = vortexDescentRate;
    }

    // ===================================================================
    // Runtime state queries
    // ===================================================================

    real_Num ExtremeHelicopterController::getInducedVelocity() const
    {
        return m_inducedVelocity;
    }

    bool ExtremeHelicopterController::isVortexRingStateActive() const
    {
        return m_vrsActive;
    }

    // ===================================================================
    // IAircraftPowerUnit / IVehiclePowerUnit passthrough stubs
    // ===================================================================

    real_Num ExtremeHelicopterController::getRPM() const
    {
        return m_rotorRPM;
    }

    void ExtremeHelicopterController::setRPM(real_Num rpm)
    {
        m_rotorRPM = Math<real_Num>::clamp(rpm, 0.0, m_maxRotorRPM);
    }

    real_Num ExtremeHelicopterController::getThrottle() const
    {
        return Math<real_Num>::clamp(getChannel(kThrChannel), 0.0,
                                     1.0);
    }

    void ExtremeHelicopterController::setThrottle(real_Num throttle)
    {
        setChannel(kThrChannel, throttle);
    }

    real_Num ExtremeHelicopterController::getMoi() const
    {
        return m_moi;
    }

    void ExtremeHelicopterController::setMoi(real_Num moi)
    {
        m_moi = moi;
    }

    real_Num ExtremeHelicopterController::getThrustMultiplier() const
    {
        return m_thrustMultiplier;
    }

    void ExtremeHelicopterController::setThrustMultiplier(real_Num thrustMultiplier)
    {
        m_thrustMultiplier = thrustMultiplier;
    }

    real_Num ExtremeHelicopterController::getTorqueMultiplier() const
    {
        return m_torqueMultiplier;
    }

    void ExtremeHelicopterController::setTorqueMultiplier(real_Num torqueMultiplier)
    {
        m_torqueMultiplier = torqueMultiplier;
    }

    real_Num ExtremeHelicopterController::getPeakPowerW() const
    {
        return m_peakPowerW;
    }

    void ExtremeHelicopterController::setPeakPowerW(real_Num peakPowerW)
    {
        m_peakPowerW = peakPowerW;
    }

    real_Num ExtremeHelicopterController::getTorque(f32 /*throttlePosition*/) const
    {
        return m_engineTorque;
    }

    real_Num ExtremeHelicopterController::getMaxTorque(u32 /*rpm*/) const
    {
        return m_maxEngineTorque;
    }

    real_Num ExtremeHelicopterController::getMinTorque(u32 /*rpm*/) const
    {
        return 0.0;
    }

    real_Num ExtremeHelicopterController::getTorque() const
    {
        return m_engineTorque;
    }

    real_Num ExtremeHelicopterController::getEngineRPM(int /*idx*/) const
    {
        return m_rotorRPM;
    }

    real_Num ExtremeHelicopterController::getThrust(int /*idx*/) const
    {
        return 0.0;
    }

    // ===================================================================
    // IAircraft component accessors
    // ===================================================================

    SmartPtr<IAircraftCallback> ExtremeHelicopterController::getCallback() const
    {
        return m_aircraftCallback;
    }

    void ExtremeHelicopterController::setCallback(SmartPtr<IAircraftCallback> callback)
    {
        m_aircraftCallback = callback;
    }

    SmartPtr<IBatteryPack> ExtremeHelicopterController::getBatteryPack() const
    {
        return m_batteryPack;
    }

    void ExtremeHelicopterController::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_batteryPack = batteryPack;
    }

    SmartPtr<IAerodymanicsWind> ExtremeHelicopterController::getWind() const
    {
        return m_wind;
    }

    void ExtremeHelicopterController::setWind(SmartPtr<IAerodymanicsWind> wind)
    {
        m_wind = wind;
    }

    Transform3<real_Num> ExtremeHelicopterController::getBodyTransform() const
    {
        return m_bodyTransform;
    }

    void ExtremeHelicopterController::setBodyTransform(Transform3<real_Num> bodyTransform)
    {
        m_bodyTransform = bodyTransform;
    }

    real_Num ExtremeHelicopterController::getRollwiseDamping() const
    {
        return m_rollwiseDamping;
    }

    void ExtremeHelicopterController::setRollwiseDamping(real_Num rollwiseDamping)
    {
        m_rollwiseDamping = rollwiseDamping;
    }

    // ===================================================================
    // Propeller unit management
    // ===================================================================

    void ExtremeHelicopterController::addPropellerUnit(
        SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        if(propellerUnit)
            m_propellerUnits.push_back(propellerUnit);
    }

    void ExtremeHelicopterController::removePropellerUnit(
        SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        auto it = std::find(m_propellerUnits.begin(), m_propellerUnits.end(), propellerUnit);
        if(it != m_propellerUnits.end())
            m_propellerUnits.erase(it);
    }

    Array<SmartPtr<IAircraftPropellerUnit>> ExtremeHelicopterController::getPropellerUnits() const
    {
        return m_propellerUnits;
    }

    void ExtremeHelicopterController::setPropellerUnits(
        const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits)
    {
        m_propellerUnits = propellerUnits;
    }

    // ===================================================================
    // Wheel management
    // ===================================================================

    void ExtremeHelicopterController::addWheel(SmartPtr<IWheelComponent> wheel)
    {
        if(wheel)
            m_wheels.push_back(wheel);
    }

    void ExtremeHelicopterController::removeWheel(SmartPtr<IWheelComponent> wheel)
    {
        auto it = std::find(m_wheels.begin(), m_wheels.end(), wheel);
        if(it != m_wheels.end())
            m_wheels.erase(it);
    }

    Array<SmartPtr<IWheelComponent>> ExtremeHelicopterController::getWheels() const
    {
        return m_wheels;
    }

    void ExtremeHelicopterController::setWheels(const Array<SmartPtr<IWheelComponent>> &wheels)
    {
        m_wheels = wheels;
    }

    // ===================================================================
    // Control surface / section / model path stubs
    // ===================================================================

    void ExtremeHelicopterController::setControlAngle(s32 /*id*/, f32 /*angle*/)
    {
        // Not used by this controller.
    }

    real_Num ExtremeHelicopterController::getSectionMultiplier() const
    {
        return m_sectionMultiplier;
    }

    void ExtremeHelicopterController::setSectionMultiplier(real_Num sectionMultiplier)
    {
        m_sectionMultiplier = sectionMultiplier;
    }

    String ExtremeHelicopterController::getModelDataFilePath() const
    {
        return m_modelDataFilePath;
    }

    void ExtremeHelicopterController::setModelDataFilePath(const String &filePath)
    {
        m_modelDataFilePath = filePath;
    }
}
