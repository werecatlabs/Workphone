#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/HelicopterTurbineEngine .hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone::vehicle
{
    WP_CLASS_REGISTER_DERIVED(workphone::vehicle, HelicopterTurbineEngine,
                              CAircraftAttachment<IVehicleComponent>);

    // ===================================================================
    // ITT heat constant (matching C#: fuelFlow * 900 → temperature target)
    // ===================================================================
    static constexpr real_Num kHeatConstant = 900.0;
    static constexpr real_Num kOverTempFuelCutback = 0.9;

    // ===================================================================
    // Construction / destruction
    // ===================================================================

    HelicopterTurbineEngine::HelicopterTurbineEngine() = default;

    HelicopterTurbineEngine::~HelicopterTurbineEngine() = default;

    // ===================================================================
    // Lifecycle
    // ===================================================================

    void HelicopterTurbineEngine::load(SmartPtr<ISharedObject> data)
    {
        // CAircraftAttachment<IVehicleComponent>::load( data );
    }

    void HelicopterTurbineEngine::update(const double &t, const double &dt)
    {
        const real_Num fdt = static_cast<real_Num>(dt);

        // Run subsystems in the same order as the original C# FixedUpdate().
        updateFuelSystem(fdt);
        updateSpoolDynamics(fdt);
        updateGovernor(fdt);
        updateTemperature(fdt);
        computeOutputTorque();
    }

    // ===================================================================
    // FUEL CONTROL
    // ===================================================================
    // C# reference:
    //   fuelFlow = Lerp(fuelFlow, throttleInput, dt * fuelResponse)
    // ===================================================================

    void HelicopterTurbineEngine::updateFuelSystem(real_Num dt)
    {
        const real_Num alpha = Math<real_Num>::clamp(
            dt * m_fuelResponse, 0.0, 1.0);

        m_fuelFlow = m_fuelFlow + alpha * (m_throttleInput - m_fuelFlow);
    }

    // ===================================================================
    // ENGINE SPOOL PHYSICS
    // ===================================================================
    // C# reference:
    //   N1 = MoveTowards(N1, fuelFlow * maxN1, n1Accel * dt)
    //   N2 = MoveTowards(N2, N1,               n2Accel * dt)
    // ===================================================================

    void HelicopterTurbineEngine::updateSpoolDynamics(real_Num dt)
    {
        // N1 — gas generator tracks fuel flow demand.
        const real_Num targetN1 = m_fuelFlow * m_maxN1;
        const real_Num n1Step = m_n1Accel * dt;
        const real_Num n1Delta = targetN1 - m_n1;
        if(Math<real_Num>::Abs(n1Delta) <= n1Step)
            m_n1 = targetN1;
        else
            m_n1 += (n1Delta > static_cast<real_Num>(0.0) ? n1Step : -n1Step);

        // N2 — power turbine tracks N1.
        const real_Num n2Step = m_n2Accel * dt;
        const real_Num n2Delta = m_n1 - m_n2;
        if(Math<real_Num>::Abs(n2Delta) <= n2Step)
            m_n2 = m_n1;
        else
            m_n2 += (n2Delta > static_cast<real_Num>(0.0) ? n2Step : -n2Step);

        m_n1 = Math<real_Num>::clamp(m_n1, 0.0, m_maxN1);
        m_n2 = Math<real_Num>::clamp(m_n2, 0.0, m_maxN2);
    }

    // ===================================================================
    // GOVERNOR SYSTEM
    // ===================================================================
    // C# reference:
    //   rpmError   = targetRotorRPM - RotorRPM
    //   correction = rpmError * governorGain
    //   fuelFlow   = Clamp01(fuelFlow + correction * dt)
    // ===================================================================

    void HelicopterTurbineEngine::updateGovernor(real_Num dt)
    {
        const real_Num rpmError = m_targetRotorRPM - m_rotorRPM;
        const real_Num correction = rpmError * m_governorGain;

        m_fuelFlow =
            Math<real_Num>::clamp(m_fuelFlow + correction * dt, 0.0,
                                  1.0);
    }

    // ===================================================================
    // ENGINE TEMPERATURE
    // ===================================================================
    // C# reference:
    //   heat        = fuelFlow * 900
    //   temperature = Lerp(temperature, heat, dt)
    //   if temperature > maxTemp → fuelFlow *= 0.9
    // ===================================================================

    void HelicopterTurbineEngine::updateTemperature(real_Num dt)
    {
        const real_Num targetTemp = m_fuelFlow * kHeatConstant;

        const real_Num alpha =
            Math<real_Num>::clamp(dt, 0.0, 1.0);

        m_temperature = m_temperature + alpha * (targetTemp - m_temperature);

        if(m_temperature > m_maxTemp)
            m_fuelFlow *= kOverTempFuelCutback;
    }

    // ===================================================================
    // TORQUE OUTPUT
    // ===================================================================
    // C# reference:
    //   powerFraction = N2 / maxN2
    //   OutputTorque  = powerFraction * maxTorque
    // ===================================================================

    void HelicopterTurbineEngine::computeOutputTorque()
    {
        const real_Num powerFraction =
            (m_maxN2 > static_cast<real_Num>(0.0))
                ? Math<real_Num>::clamp(m_n2 / m_maxN2, 0.0,
                                        1.0)
                : static_cast<real_Num>(0.0);

        m_outputTorque = powerFraction * m_maxTorque;
    }

    // ===================================================================
    // Throttle / rotor RPM I/O
    // ===================================================================

    void HelicopterTurbineEngine::setThrottleInput(real_Num throttle)
    {
        m_throttleInput = Math<real_Num>::clamp(throttle, 0.0,
                                                1.0);
    }

    real_Num HelicopterTurbineEngine::getThrottleInput() const
    {
        return m_throttleInput;
    }

    void HelicopterTurbineEngine::setRotorRPM(real_Num rpm)
    {
        m_rotorRPM = rpm;
    }

    real_Num HelicopterTurbineEngine::getRotorRPM() const
    {
        return m_rotorRPM;
    }

    // ===================================================================
    // Output accessors
    // ===================================================================

    real_Num HelicopterTurbineEngine::getOutputTorque() const
    {
        return m_outputTorque;
    }

    real_Num HelicopterTurbineEngine::getN1() const
    {
        return m_n1;
    }

    real_Num HelicopterTurbineEngine::getN2() const
    {
        return m_n2;
    }

    real_Num HelicopterTurbineEngine::getTemperature() const
    {
        return m_temperature;
    }

    real_Num HelicopterTurbineEngine::getFuelFlow() const
    {
        return m_fuelFlow;
    }

    // ===================================================================
    // Spool parameter accessors
    // ===================================================================

    real_Num HelicopterTurbineEngine::getMaxN1() const
    {
        return m_maxN1;
    }

    void HelicopterTurbineEngine::setMaxN1(real_Num maxN1)
    {
        m_maxN1 = maxN1;
    }

    real_Num HelicopterTurbineEngine::getMaxN2() const
    {
        return m_maxN2;
    }

    void HelicopterTurbineEngine::setMaxN2(real_Num maxN2)
    {
        m_maxN2 = maxN2;
    }

    real_Num HelicopterTurbineEngine::getN1Accel() const
    {
        return m_n1Accel;
    }

    void HelicopterTurbineEngine::setN1Accel(real_Num n1Accel)
    {
        m_n1Accel = n1Accel;
    }

    real_Num HelicopterTurbineEngine::getN2Accel() const
    {
        return m_n2Accel;
    }

    void HelicopterTurbineEngine::setN2Accel(real_Num n2Accel)
    {
        m_n2Accel = n2Accel;
    }

    real_Num HelicopterTurbineEngine::getFuelResponse() const
    {
        return m_fuelResponse;
    }

    void HelicopterTurbineEngine::setFuelResponse(real_Num fuelResponse)
    {
        m_fuelResponse = fuelResponse;
    }

    // ===================================================================
    // Governor parameter accessors
    // ===================================================================

    real_Num HelicopterTurbineEngine::getTargetRotorRPM() const
    {
        return m_targetRotorRPM;
    }

    void HelicopterTurbineEngine::setTargetRotorRPM(real_Num targetRotorRPM)
    {
        m_targetRotorRPM = targetRotorRPM;
    }

    real_Num HelicopterTurbineEngine::getGovernorGain() const
    {
        return m_governorGain;
    }

    void HelicopterTurbineEngine::setGovernorGain(real_Num governorGain)
    {
        m_governorGain = governorGain;
    }

    // ===================================================================
    // Transmission parameter accessors
    // ===================================================================

    real_Num HelicopterTurbineEngine::getGearRatio() const
    {
        return m_gearRatio;
    }

    void HelicopterTurbineEngine::setGearRatio(real_Num gearRatio)
    {
        m_gearRatio = gearRatio;
    }

    real_Num HelicopterTurbineEngine::getMaxTorque() const
    {
        return m_maxTorque;
    }

    void HelicopterTurbineEngine::setMaxTorque(real_Num maxTorque)
    {
        m_maxTorque = maxTorque;
    }

    // ===================================================================
    // Engine limit accessors
    // ===================================================================

    real_Num HelicopterTurbineEngine::getMaxTemp() const
    {
        return m_maxTemp;
    }

    void HelicopterTurbineEngine::setMaxTemp(real_Num maxTemp)
    {
        m_maxTemp = maxTemp;
    }
}
