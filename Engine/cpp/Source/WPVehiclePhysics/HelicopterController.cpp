#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/HelicopterController.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED(workphone::vehicle, HelicopterController,
                              CAerodynamicsVehicle<IAircraft>);

    // ===================================================================
    // Channel indices (standard CAerodynamicsVehicle layout)
    // ===================================================================
    static constexpr s32 kThrChannel = 0; ///< Collective / throttle  [0, 1]
    static constexpr s32 kAilChannel = 1; ///< Roll                  [-1, 1]
    static constexpr s32 kEleChannel = 2; ///< Pitch                 [-1, 1]
    static constexpr s32 kYawChannel = 3; ///< Yaw                   [-1, 1]

    // ===================================================================
    // Construction / destruction
    // ===================================================================

    HelicopterController::HelicopterController() = default;

    HelicopterController::~HelicopterController() = default;

    // ===================================================================
    // Lifecycle
    // ===================================================================

    bool HelicopterController::isValid() const
    {
        return true;
    }

    void HelicopterController::load(SmartPtr<ISharedObject> data)
    {
        CAerodynamicsVehicle<IAircraft>::load(data);
    }

    void HelicopterController::update()
    {
        CAerodynamicsVehicle<IAircraft>::update();
    }

    void HelicopterController::update(const double &t, const double &dt)
    {
        clearForces();

        // Derive world-space body axes from the current orientation.
        const auto &worldTransform = getWorldTransform();
        const auto orientation = worldTransform.getOrientation();

        const Vector3<real_Num> worldUp = orientation * Vector3<real_Num>::unitY();
        const Vector3<real_Num> worldFwd = orientation * Vector3<real_Num>::unitZ();
        const Vector3<real_Num> worldRight = orientation * Vector3<real_Num>::unitX();

        const real_Num fdt = static_cast<real_Num>(dt);

        // Engine spool simulation — mirrors C# Update() Lerp.
        // C# reference:
        //   engineThrottle = Lerp(engineThrottle, collectiveInput, dt * spoolUpSpeed)
        const real_Num collectiveIn = Math<real_Num>::clamp(
            getChannel(kThrChannel), 0.0, 1.0);

        const real_Num alpha = Math<real_Num>::clamp(
            fdt * m_spoolUpSpeed, 0.0, 1.0);

        m_engineThrottle = m_engineThrottle + alpha * (collectiveIn - m_engineThrottle);

        // Run subsystems in the same order as the original C# FixedUpdate().
        applyLift(worldUp);
        applyTilt(worldFwd, worldRight);
        applyYaw(worldUp);
        applyStabilization(worldUp);

        CAerodynamicsVehicle<IAircraft>::update();
    }

    // ===================================================================
    // LIFT
    // ===================================================================
    // C# reference:
    //   lift = engineThrottle * liftPower * groundEffect
    //   AddForce(transform.up * lift)
    // ===================================================================

    void HelicopterController::applyLift(const Vector3<real_Num> &up)
    {
        const real_Num heightFactor = computeGroundEffect();
        const real_Num lift = m_engineThrottle * m_liftPower * heightFactor;

        addForce(0, up * lift, getPosition());
    }

    // ===================================================================
    // GROUND EFFECT
    // ===================================================================
    // C# reference (raycast → distance):
    //   factor = 1 + (1 − hit.distance / groundEffectHeight)
    // Approximated from world-space Y altitude (AGL proxy).
    // ===================================================================

    real_Num HelicopterController::computeGroundEffect() const
    {
        const real_Num agl = getPosition().Y();

        if(agl < m_groundEffectHeight && m_groundEffectHeight > static_cast<real_Num>(0.0))
        {
            const real_Num t =
                Math<real_Num>::clamp(agl / m_groundEffectHeight, 0.0,
                                      1.0);

            return static_cast<real_Num>(1.0) + (static_cast<real_Num>(1.0) - t);
        }

        return 1.0;
    }

    // ===================================================================
    // TILT
    // ===================================================================
    // C# reference:
    //   pitch = transform.right * (-pitchInput * pitchForce)
    //   roll  = transform.forward * (rollInput * rollForce)
    //   AddTorque(pitch + roll)
    // ===================================================================

    void HelicopterController::applyTilt(const Vector3<real_Num> &fwd,
                                         const Vector3<real_Num> &right)
    {
        const real_Num pitchIn = getChannel(kEleChannel);
        const real_Num rollIn = getChannel(kAilChannel);

        // Negative pitch input tilts nose down (matches C# negation).
        const Vector3<real_Num> pitchTorque = right * (-pitchIn * m_pitchForce);
        const Vector3<real_Num> rollTorque = fwd * (rollIn * m_rollForce);

        addTorque(0, pitchTorque + rollTorque);
    }

    // ===================================================================
    // YAW
    // ===================================================================
    // C# reference:
    //   AddTorque(transform.up * yawInput * yawTorque)
    // ===================================================================

    void HelicopterController::applyYaw(const Vector3<real_Num> &up)
    {
        const real_Num yawIn = getChannel(kYawChannel);
        addTorque(0, up * (yawIn * m_yawTorque));
    }

    // ===================================================================
    // STABILIZATION
    // ===================================================================
    // C# reference:
    //   torque = Cross(transform.up, Vector3.up)
    //   AddTorque(torque * stabilization)
    // ===================================================================

    void HelicopterController::applyStabilization(const Vector3<real_Num> &up)
    {
        const Vector3<real_Num> worldUp(0.0, 1.0,
                                        0.0);

        const Vector3<real_Num> restoringTorque = up.crossProduct(worldUp);
        addTorque(0, restoringTorque * m_stabilization);
    }

    // ===================================================================
    // Engine parameter accessors
    // ===================================================================

    real_Num HelicopterController::getEnginePower() const
    {
        return m_enginePower;
    }

    void HelicopterController::setEnginePower(real_Num enginePower)
    {
        m_enginePower = enginePower;
    }

    real_Num HelicopterController::getSpoolUpSpeed() const
    {
        return m_spoolUpSpeed;
    }

    void HelicopterController::setSpoolUpSpeed(real_Num spoolUpSpeed)
    {
        m_spoolUpSpeed = spoolUpSpeed;
    }

    real_Num HelicopterController::getEngineThrottle() const
    {
        return m_engineThrottle;
    }

    // ===================================================================
    // Lift parameter accessors
    // ===================================================================

    real_Num HelicopterController::getLiftPower() const
    {
        return m_liftPower;
    }

    void HelicopterController::setLiftPower(real_Num liftPower)
    {
        m_liftPower = liftPower;
    }

    real_Num HelicopterController::getGroundEffectHeight() const
    {
        return m_groundEffectHeight;
    }

    void HelicopterController::setGroundEffectHeight(real_Num groundEffectHeight)
    {
        m_groundEffectHeight = groundEffectHeight;
    }

    // ===================================================================
    // Tilt parameter accessors
    // ===================================================================

    real_Num HelicopterController::getPitchForce() const
    {
        return m_pitchForce;
    }

    void HelicopterController::setPitchForce(real_Num pitchForce)
    {
        m_pitchForce = pitchForce;
    }

    real_Num HelicopterController::getRollForce() const
    {
        return m_rollForce;
    }

    void HelicopterController::setRollForce(real_Num rollForce)
    {
        m_rollForce = rollForce;
    }

    // ===================================================================
    // Yaw parameter accessors
    // ===================================================================

    real_Num HelicopterController::getYawTorque() const
    {
        return m_yawTorque;
    }

    void HelicopterController::setYawTorque(real_Num yawTorque)
    {
        m_yawTorque = yawTorque;
    }

    // ===================================================================
    // Stability parameter accessors
    // ===================================================================

    real_Num HelicopterController::getStabilization() const
    {
        return m_stabilization;
    }

    void HelicopterController::setStabilization(real_Num stabilization)
    {
        m_stabilization = stabilization;
    }

    // ===================================================================
    // IAircraftPowerUnit / IVehiclePowerUnit passthrough stubs
    // ===================================================================

    real_Num HelicopterController::getRPM() const
    {
        return 0.0;
    }

    void HelicopterController::setRPM(real_Num /*rpm*/)
    {
    }

    real_Num HelicopterController::getThrottle() const
    {
        return m_engineThrottle;
    }

    void HelicopterController::setThrottle(real_Num throttle)
    {
        setChannel(kThrChannel, throttle);
    }

    real_Num HelicopterController::getMoi() const
    {
        return m_moi;
    }

    void HelicopterController::setMoi(real_Num moi)
    {
        m_moi = moi;
    }

    real_Num HelicopterController::getThrustMultiplier() const
    {
        return m_thrustMultiplier;
    }

    void HelicopterController::setThrustMultiplier(real_Num thrustMultiplier)
    {
        m_thrustMultiplier = thrustMultiplier;
    }

    real_Num HelicopterController::getTorqueMultiplier() const
    {
        return m_torqueMultiplier;
    }

    void HelicopterController::setTorqueMultiplier(real_Num torqueMultiplier)
    {
        m_torqueMultiplier = torqueMultiplier;
    }

    real_Num HelicopterController::getPeakPowerW() const
    {
        return m_peakPowerW;
    }

    void HelicopterController::setPeakPowerW(real_Num peakPowerW)
    {
        m_peakPowerW = peakPowerW;
    }

    real_Num HelicopterController::getTorque(f32 /*throttlePosition*/) const
    {
        return 0.0;
    }

    real_Num HelicopterController::getMaxTorque(u32 /*rpm*/) const
    {
        return 0.0;
    }

    real_Num HelicopterController::getMinTorque(u32 /*rpm*/) const
    {
        return 0.0;
    }

    real_Num HelicopterController::getTorque() const
    {
        return 0.0;
    }

    real_Num HelicopterController::getEngineRPM(int /*idx*/) const
    {
        return 0.0;
    }

    real_Num HelicopterController::getThrust(int /*idx*/) const
    {
        return 0.0;
    }

    real_Num HelicopterController::getMaxRotorRPM() const
    {
        return m_maxRotorRPM;
    }

    real_Num HelicopterController::getAirDensity() const
    {
        return m_airDensity;
    }

    void HelicopterController::setAirDensity(real_Num airDensity)
    {
        m_airDensity = airDensity;
    }

    // ===================================================================
    // IAircraft component accessors
    // ===================================================================

    SmartPtr<IAircraftCallback> HelicopterController::getCallback() const
    {
        return m_aircraftCallback;
    }

    void HelicopterController::setCallback(SmartPtr<IAircraftCallback> callback)
    {
        m_aircraftCallback = callback;
    }

    SmartPtr<IBatteryPack> HelicopterController::getBatteryPack() const
    {
        return m_batteryPack;
    }

    void HelicopterController::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_batteryPack = batteryPack;
    }

    SmartPtr<IAerodymanicsWind> HelicopterController::getWind() const
    {
        return m_wind;
    }

    void HelicopterController::setWind(SmartPtr<IAerodymanicsWind> wind)
    {
        m_wind = wind;
    }

    Transform3<real_Num> HelicopterController::getBodyTransform() const
    {
        return m_bodyTransform;
    }

    void HelicopterController::setBodyTransform(Transform3<real_Num> bodyTransform)
    {
        m_bodyTransform = bodyTransform;
    }

    real_Num HelicopterController::getRollwiseDamping() const
    {
        return m_rollwiseDamping;
    }

    void HelicopterController::setRollwiseDamping(real_Num rollwiseDamping)
    {
        m_rollwiseDamping = rollwiseDamping;
    }

    // ===================================================================
    // Propeller unit management
    // ===================================================================

    void HelicopterController::addPropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        if(propellerUnit)
            m_propellerUnits.push_back(propellerUnit);
    }

    void HelicopterController::removePropellerUnit(SmartPtr<IAircraftPropellerUnit> propellerUnit)
    {
        auto it = std::find(m_propellerUnits.begin(), m_propellerUnits.end(), propellerUnit);
        if(it != m_propellerUnits.end())
            m_propellerUnits.erase(it);
    }

    Array<SmartPtr<IAircraftPropellerUnit>> HelicopterController::getPropellerUnits() const
    {
        return m_propellerUnits;
    }

    void HelicopterController::setPropellerUnits(
        const Array<SmartPtr<IAircraftPropellerUnit>> &propellerUnits)
    {
        m_propellerUnits = propellerUnits;
    }

    // ===================================================================
    // Wheel management
    // ===================================================================

    void HelicopterController::addWheel(SmartPtr<IWheelComponent> wheel)
    {
        if(wheel)
            m_wheels.push_back(wheel);
    }

    void HelicopterController::removeWheel(SmartPtr<IWheelComponent> wheel)
    {
        auto it = std::find(m_wheels.begin(), m_wheels.end(), wheel);
        if(it != m_wheels.end())
            m_wheels.erase(it);
    }

    Array<SmartPtr<IWheelComponent>> HelicopterController::getWheels() const
    {
        return m_wheels;
    }

    void HelicopterController::setWheels(const Array<SmartPtr<IWheelComponent>> &wheels)
    {
        m_wheels = wheels;
    }

    // ===================================================================
    // Control surface / section / model path stubs
    // ===================================================================

    void HelicopterController::setControlAngle(s32 /*id*/, f32 /*angle*/)
    {
    }

    real_Num HelicopterController::getSectionMultiplier() const
    {
        return m_sectionMultiplier;
    }

    void HelicopterController::setSectionMultiplier(real_Num sectionMultiplier)
    {
        m_sectionMultiplier = sectionMultiplier;
    }

    String HelicopterController::getModelDataFilePath() const
    {
        return m_modelDataFilePath;
    }

    void HelicopterController::setModelDataFilePath(const String &filePath)
    {
        m_modelDataFilePath = filePath;
    }
}
