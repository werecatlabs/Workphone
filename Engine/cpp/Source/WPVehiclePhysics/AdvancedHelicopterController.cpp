#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/AdvancedHelicopterController.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED(workphone::vehicle, AdvancedHelicopterController,
                              CAerodynamicsVehicle<IAircraft>);

    // ===================================================================
    // Channel indices (shared with CAircraft convention)
    // ===================================================================
    static constexpr s32 kThrChannel = 0; ///< Throttle
    static constexpr s32 kAilChannel = 1; ///< Roll (aileron)
    static constexpr s32 kEleChannel = 2; ///< Pitch (elevator)
    static constexpr s32 kYawChannel = 3; ///< Yaw (rudder / tail rotor)
    static constexpr s32 kColChannel = 5; ///< Collective blade pitch

    // ===================================================================
    // Construction
    // ===================================================================

    AdvancedHelicopterController::AdvancedHelicopterController() = default;
    AdvancedHelicopterController::~AdvancedHelicopterController() = default;

    // ===================================================================
    // ISharedObject / IVehicle lifecycle
    // ===================================================================

    bool AdvancedHelicopterController::isValid() const
    {
        return true;
    }

    void AdvancedHelicopterController::load(SmartPtr<ISharedObject> data)
    {
        CAerodynamicsVehicle<IAircraft>::load(data);
    }

    void AdvancedHelicopterController::update()
    {
        CAerodynamicsVehicle<IAircraft>::update();
    }

    void AdvancedHelicopterController::update(const double &t, const double &dt)
    {
        // Clear accumulators from previous frame.
        clearForces();

        // Extract current body axes from world orientation.
        const auto &worldTransform = getWorldTransform();
        const auto orientation = worldTransform.getOrientation();

        // World-space basis vectors derived from the body orientation.
        const Vector3<real_Num> worldUp = orientation * Vector3<real_Num>::unitY();
        const Vector3<real_Num> worldFwd = orientation * Vector3<real_Num>::unitZ();
        const Vector3<real_Num> worldRight = orientation * Vector3<real_Num>::unitX();

        // Integrate rotor RPM.
        updateRotorPhysics(static_cast<real_Num>(dt));

        // Accumulate aerodynamic forces/torques.
        applyLift(worldUp, worldFwd, worldRight);
        applyTailRotor(worldUp);

        // Dispatch accumulated force+torque to the physics body.
        CAerodynamicsVehicle<IAircraft>::update();
    }

    // ===================================================================
    // Internal helpers
    // ===================================================================

    void AdvancedHelicopterController::updateRotorPhysics(real_Num dt)
    {
        // Throttle input drives engine torque.
        const real_Num throttle = Math<real_Num>::clamp(
            getChannel(kThrChannel), 0.0, 1.0);

        m_engineTorque = throttle * m_maxEngineTorque;

        // Aerodynamic drag opposes rotation and grows with RPM.
        const real_Num dragTorque = m_rotorDrag * m_rotorRPM;

        // Net torque → angular acceleration → RPM change.
        const real_Num netTorque = m_engineTorque - dragTorque;
        const real_Num angularAccel = (m_rotorInertia > static_cast<real_Num>(0.0))
                                          ? (netTorque / m_rotorInertia)
                                          : static_cast<real_Num>(0.0);

        // Convert rad/s² → RPM/s and integrate.
        m_rotorRPM += angularAccel * dt * static_cast<real_Num>(60.0);
        m_rotorRPM =
            Math<real_Num>::clamp(m_rotorRPM, 0.0, m_maxRotorRPM);
    }

    void AdvancedHelicopterController::applyLift(const Vector3<real_Num> &up,
                                                 const Vector3<real_Num> &fwd,
                                                 const Vector3<real_Num> &right)
    {
        // Collective and cyclic channel inputs.
        const real_Num collectiveIn = Math<real_Num>::clamp(
            getChannel(kColChannel), -1.0, 1.0);
        const real_Num pitchIn = getChannel(kEleChannel);
        const real_Num rollIn = getChannel(kAilChannel);

        // Effective blade pitch angle from collective input.
        const real_Num pitchAngleDeg = collectiveIn * m_maxCollectivePitch;
        const real_Num pitchAngleRad = pitchAngleDeg * Math<real_Num>::deg_to_rad();

        // Rotor speed in rev/s.
        const real_Num rps = m_rotorRPM / static_cast<real_Num>(60.0);

        // Simplified momentum-theory lift:
        //   L = 0.5 * � * A * (ω)² * CL * θ
        // where θ is in radians (blade pitch angle).
        real_Num lift = static_cast<real_Num>(0.5) * m_airDensity * m_rotorArea * (rps * rps) *
                        m_liftCoefficient * pitchAngleRad;

        // Clamp to non-negative (rotor cannot push downward in this model).
        lift = Math<real_Num>::max(lift, 0.0);

        // Apply environmental modifiers.
        lift *= computeGroundEffect();
        lift *= computeTranslationalLift(getLinearVelocity().length());

        // Cyclic disc-tilt: offset the thrust vector from pure "up".
        const real_Num tiltRad = m_cyclicTiltAngle * Math<real_Num>::deg_to_rad();
        Vector3<real_Num> discAxis = up + fwd * (pitchIn * tiltRad) + right * (rollIn * tiltRad);
        discAxis.normalise();

        // Apply main-rotor thrust force through the CG (body origin).
        addForce(0, discAxis * lift, getPosition());

        // Reactive torque from main rotor spins the fuselage opposite to rotor.
        const real_Num reactiveTorqueMag = lift * static_cast<real_Num>(0.02);
        addTorque(0, -up * reactiveTorqueMag);
    }

    void AdvancedHelicopterController::applyTailRotor(const Vector3<real_Num> &up)
    {
        const real_Num yawIn = getChannel(kYawChannel);

        // Tail rotor produces a side-force that reacts as a yaw moment.
        const real_Num antiTorque = m_rotorRPM * static_cast<real_Num>(0.5);
        const real_Num yawControl = yawIn * m_tailRotorForce;

        addTorque(0, up * (yawControl + antiTorque));
    }

    real_Num AdvancedHelicopterController::computeGroundEffect() const
    {
        // Use the world-space Y position as an AGL approximation.
        // A proper implementation would raycast down via the physics callback.
        const real_Num agl = getPosition().Y();

        if(agl < m_groundEffectHeight && m_groundEffectHeight > static_cast<real_Num>(0.0))
        {
            // Factor rises as the helicopter descends (matches Unity formula).
            const real_Num t = agl / m_groundEffectHeight;
            const real_Num factor =
                static_cast<real_Num>(1.0) + (static_cast<real_Num>(1.0) - t);
            return Math<real_Num>::max(factor, 1.0);
        }

        return 1.0;
    }

    real_Num AdvancedHelicopterController::computeTranslationalLift(real_Num speed) const
    {
        if(speed <= m_etlStartSpeed)
            return 1.0;

        const real_Num etlEndSpeed = m_etlStartSpeed * static_cast<real_Num>(3.0);
        const real_Num t =
            Math<real_Num>::clamp((speed - m_etlStartSpeed) / (etlEndSpeed - m_etlStartSpeed),
                                  0.0, 1.0);

        return static_cast<real_Num>(1.0) + t * (m_etlMaxBoost - static_cast<real_Num>(1.0));
    }

    // ===================================================================
    // Rotor physics accessors
    // ===================================================================

    real_Num AdvancedHelicopterController::getRotorInertia() const
    {
        return m_rotorInertia;
    }

    void AdvancedHelicopterController::setRotorInertia(real_Num rotorInertia)
    {
        m_rotorInertia = rotorInertia;
    }

    real_Num AdvancedHelicopterController::getMaxEngineTorque() const
    {
        return m_maxEngineTorque;
    }

    void AdvancedHelicopterController::setMaxEngineTorque(real_Num maxEngineTorque)
    {
        m_maxEngineTorque = maxEngineTorque;
    }

    real_Num AdvancedHelicopterController::getRotorDrag() const
    {
        return m_rotorDrag;
    }

    void AdvancedHelicopterController::setRotorDrag(real_Num rotorDrag)
    {
        m_rotorDrag = rotorDrag;
    }

    real_Num AdvancedHelicopterController::getMaxRotorRPM() const
    {
        return m_maxRotorRPM;
    }

    void AdvancedHelicopterController::setMaxRotorRPM(real_Num maxRotorRPM)
    {
        m_maxRotorRPM = maxRotorRPM;
    }

    real_Num AdvancedHelicopterController::getRotorRPM() const
    {
        return m_rotorRPM;
    }

    // ===================================================================
    // Lift accessors
    // ===================================================================

    real_Num AdvancedHelicopterController::getRotorArea() const
    {
        return m_rotorArea;
    }

    void AdvancedHelicopterController::setRotorArea(real_Num rotorArea)
    {
        m_rotorArea = rotorArea;
    }

    real_Num AdvancedHelicopterController::getAirDensity() const
    {
        return m_airDensity;
    }

    void AdvancedHelicopterController::setAirDensity(real_Num airDensity)
    {
        m_airDensity = airDensity;
    }

    real_Num AdvancedHelicopterController::getLiftCoefficient() const
    {
        return m_liftCoefficient;
    }

    void AdvancedHelicopterController::setLiftCoefficient(real_Num liftCoefficient)
    {
        m_liftCoefficient = liftCoefficient;
    }

    // ===================================================================
    // Blade pitch accessors
    // ===================================================================

    real_Num AdvancedHelicopterController::getMaxCollectivePitch() const
    {
        return m_maxCollectivePitch;
    }

    void AdvancedHelicopterController::setMaxCollectivePitch(real_Num maxCollectivePitch)
    {
        m_maxCollectivePitch = maxCollectivePitch;
    }

    real_Num AdvancedHelicopterController::getCyclicTiltAngle() const
    {
        return m_cyclicTiltAngle;
    }

    void AdvancedHelicopterController::setCyclicTiltAngle(real_Num cyclicTiltAngle)
    {
        m_cyclicTiltAngle = cyclicTiltAngle;
    }

    // ===================================================================
    // Tail rotor accessors
    // ===================================================================

    real_Num AdvancedHelicopterController::getTailRotorForce() const
    {
        return m_tailRotorForce;
    }

    void AdvancedHelicopterController::setTailRotorForce(real_Num tailRotorForce)
    {
        m_tailRotorForce = tailRotorForce;
    }

    // ===================================================================
    // Ground effect accessors
    // ===================================================================

    real_Num AdvancedHelicopterController::getGroundEffectHeight() const
    {
        return m_groundEffectHeight;
    }

    void AdvancedHelicopterController::setGroundEffectHeight(real_Num groundEffectHeight)
    {
        m_groundEffectHeight = groundEffectHeight;
    }

    // ===================================================================
    // ETL accessors
    // ===================================================================

    real_Num AdvancedHelicopterController::getETLStartSpeed() const
    {
        return m_etlStartSpeed;
    }

    void AdvancedHelicopterController::setETLStartSpeed(real_Num etlStartSpeed)
    {
        m_etlStartSpeed = etlStartSpeed;
    }

    real_Num AdvancedHelicopterController::getETLMaxBoost() const
    {
        return m_etlMaxBoost;
    }

    void AdvancedHelicopterController::setETLMaxBoost(real_Num etlMaxBoost)
    {
        m_etlMaxBoost = etlMaxBoost;
    }

    // ===================================================================
    // IAircraftPowerUnit / IVehiclePowerUnit passthrough stubs
    // ===================================================================

    real_Num AdvancedHelicopterController::getRPM() const
    {
        return m_rotorRPM;
    }

    void AdvancedHelicopterController::setRPM(real_Num rpm)
    {
        m_rotorRPM = Math<real_Num>::clamp(rpm, 0.0, m_maxRotorRPM);
    }

    real_Num AdvancedHelicopterController::getThrottle() const
    {
        return Math<real_Num>::clamp(getChannel(kThrChannel), 0.0,
                                     1.0);
    }

    void AdvancedHelicopterController::setThrottle(real_Num throttle)
    {
        setChannel(kThrChannel, throttle);
    }

    real_Num AdvancedHelicopterController::getMoi() const
    {
        return m_moi;
    }

    void AdvancedHelicopterController::setMoi(real_Num moi)
    {
        m_moi = moi;
    }

    real_Num AdvancedHelicopterController::getThrustMultiplier() const
    {
        return m_thrustMultiplier;
    }

    void AdvancedHelicopterController::setThrustMultiplier(real_Num thrustMultiplier)
    {
        m_thrustMultiplier = thrustMultiplier;
    }

    real_Num AdvancedHelicopterController::getTorqueMultiplier() const
    {
        return m_torqueMultiplier;
    }

    void AdvancedHelicopterController::setTorqueMultiplier(real_Num torqueMultiplier)
    {
        m_torqueMultiplier = torqueMultiplier;
    }

    real_Num AdvancedHelicopterController::getPeakPowerW() const
    {
        return m_peakPowerW;
    }

    void AdvancedHelicopterController::setPeakPowerW(real_Num peakPowerW)
    {
        m_peakPowerW = peakPowerW;
    }

    real_Num AdvancedHelicopterController::getTorque(f32 /*throttlePosition*/) const
    {
        return m_engineTorque;
    }

    real_Num AdvancedHelicopterController::getMaxTorque(u32 /*rpm*/) const
    {
        return m_maxEngineTorque;
    }

    real_Num AdvancedHelicopterController::getMinTorque(u32 /*rpm*/) const
    {
        return 0.0;
    }

    real_Num AdvancedHelicopterController::getTorque() const
    {
        return m_engineTorque;
    }

    real_Num AdvancedHelicopterController::getEngineRPM(int /*idx*/) const
    {
        return m_rotorRPM;
    }

    real_Num AdvancedHelicopterController::getThrust(int /*idx*/) const
    {
        return 0.0;
    }

    // ===================================================================
    // IAircraft component stubs
    // ===================================================================

    SmartPtr<IAircraftCallback> AdvancedHelicopterController::getCallback() const
    {
        return m_aircraftCallback;
    }

    void AdvancedHelicopterController::setCallback(SmartPtr<IAircraftCallback> callback)
    {
        m_aircraftCallback = callback;
    }

    SmartPtr<IBatteryPack> AdvancedHelicopterController::getBatteryPack() const
    {
        return m_batteryPack;
    }

    void AdvancedHelicopterController::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_batteryPack = batteryPack;
    }

    SmartPtr<IAerodymanicsWind> AdvancedHelicopterController::getWind() const
    {
        return m_wind;
    }

    void AdvancedHelicopterController::setWind(SmartPtr<IAerodymanicsWind> wind)
    {
        m_wind = wind;
    }

    Transform3<real_Num> AdvancedHelicopterController::getBodyTransform() const
    {
        return m_bodyTransform;
    }

    void AdvancedHelicopterController::setBodyTransform(Transform3<real_Num> bodyTransform)
    {
        m_bodyTransform = bodyTransform;
    }

    real_Num AdvancedHelicopterController::getRollwiseDamping() const
    {
        return m_rollwiseDamping;
    }

    void AdvancedHelicopterController::setRollwiseDamping(real_Num rollwiseDamping)
    {
        m_rollwiseDamping = rollwiseDamping;
    }

    // ===================================================================
    // IAircraft propeller unit management stubs
    // ===================================================================

    void AdvancedHelicopterController::addPropellerUnit(
        SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        if(propellerUnit)
            m_propellerUnits.push_back(propellerUnit);
    }

    void AdvancedHelicopterController::removePropellerUnit(
        SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        auto it = std::find(m_propellerUnits.begin(), m_propellerUnits.end(), propellerUnit);
        if(it != m_propellerUnits.end())
            m_propellerUnits.erase(it);
    }

    Array<SmartPtr<IAircraftPropellerUnit>> AdvancedHelicopterController::getPropellerUnits() const
    {
        return m_propellerUnits;
    }

    void AdvancedHelicopterController::setPropellerUnits(
        const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits)
    {
        m_propellerUnits = propellerUnits;
    }

    // ===================================================================
    // IAircraft wheel management stubs
    // ===================================================================

    void AdvancedHelicopterController::addWheel(SmartPtr<IWheelComponent> wheel)
    {
        if(wheel)
            m_wheels.push_back(wheel);
    }

    void AdvancedHelicopterController::removeWheel(SmartPtr<IWheelComponent> wheel)
    {
        auto it = std::find(m_wheels.begin(), m_wheels.end(), wheel);
        if(it != m_wheels.end())
            m_wheels.erase(it);
    }

    Array<SmartPtr<IWheelComponent>> AdvancedHelicopterController::getWheels() const
    {
        return m_wheels;
    }

    void AdvancedHelicopterController::setWheels(const Array<SmartPtr<IWheelComponent>> &wheels)
    {
        m_wheels = wheels;
    }

    // ===================================================================
    // IAircraft control surface / section / model path stubs
    // ===================================================================

    void AdvancedHelicopterController::setControlAngle(s32 /*id*/, f32 /*angle*/)
    {
        // Not used by this controller.
    }

    real_Num AdvancedHelicopterController::getSectionMultiplier() const
    {
        return m_sectionMultiplier;
    }

    void AdvancedHelicopterController::setSectionMultiplier(real_Num sectionMultiplier)
    {
        m_sectionMultiplier = sectionMultiplier;
    }

    String AdvancedHelicopterController::getModelDataFilePath() const
    {
        return m_modelDataFilePath;
    }

    void AdvancedHelicopterController::setModelDataFilePath(const String &filePath)
    {
        m_modelDataFilePath = filePath;
    }
}
