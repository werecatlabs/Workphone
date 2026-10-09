#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAircraftMotor.hpp>
#include <WPVehiclePhysics/CAircraftPropeller.hpp>
#include <WPVehiclePhysics/CAircraftBody.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
#include <WPVehiclePhysics/CBatteryPack.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    u32 CAircraftMotor::m_idExt = 0;

    CAircraftMotor::CAircraftMotor() :
        m_moi(0.0000036),
        m_motorKV(2300),
        m_motorTI(0),
        m_motorEfficiency(0.8),

        m_noLoadCurrent(0.5),
        m_frictionFactor(0),
        m_motorR(0),

        m_motorEMF(0),
        m_motorCurrent(0),
        m_motorAccel(0),

        m_motorPower(0),
        m_spragRPM(0),
        m_spragOmega(0),

        m_transmittedTorque(0),
        m_currentLimit(0),
        m_rpm(0),

        m_torque(0),
        m_omega(0),
        m_spragMode(0)
    {
        m_rpm = 0;
        m_omega = 0;
        m_motorEMF = 0;
        m_motorCurrent = 0;
        m_torque = 0;
        m_motorPower = 0;
        m_motorAccel = 0;
        m_spragRPM = 0;
        m_spragOmega = 0;
        m_spragMode = 2; // set the motor mode to a realistic one!

        m_opRPM = 0;
        m_eRPM = 0;
        m_loadInertia = 0;
        m_loadTorque = 0;

        m_id = StringUtil::parseInt("Motor" + StringUtil::toString(m_idExt++));
    }

    CAircraftMotor::~CAircraftMotor()
    {
    }

    void CAircraftMotor::load(SmartPtr<ISharedObject> data)
    {
        SmartPtr<Properties> properties = workphone::dynamic_pointer_cast<Properties>(data);
        if(properties)
        {
            properties->getPropertyValue("FlEqMotorMOI", m_moi);
            properties->getPropertyValue("FlEqMotorKV", m_motorKV);
            properties->getPropertyValue("FlEqMotorEfficiency", m_motorEfficiency);
            properties->getPropertyValue("FlEqMotorNoLoadCurrent", m_noLoadCurrent);
            properties->getPropertyValue("FlEqMotorR", m_motorR);
            properties->getPropertyValue("FlEqMotorILimit", m_currentLimit);
        }
    }

    void CAircraftMotor::load(void *pData)
    {
        // auto data = static_cast<data::aircraft_engine_data *>(pData);
        // m_torqueMultiplier = data->torqueMultiplier;
        // m_thrustMultiplier = data->thrustMultiplier;

        // data::vec4 p = data->localTransform.position;
        // data::vec4 q = data->localTransform.orientation;
        // data::vec4 s = data->localTransform.scale;

        // auto vPos = Vector3<real_Num>( p.x, p.y, -p.z );
        // auto vScale = Vector3<real_Num>( s.x, s.y, s.z );
        // auto qRot = Quaternion<real_Num>( q.w, -q.x, -q.y, q.z );

        // WP_ASSERT( vScale.length() > std::numeric_limits<f32>::epsilon() );

        // m_localTransform.setPosition( vPos );
        // m_localTransform.setScale( vScale );
        // m_localTransform.setOrientation( qRot );
    }

    void CAircraftMotor::motorCalc(double dt, CBatteryPackStandard &pack, CESController &esc)
    {
        auto throttlePos =
            static_cast<real_Num>(0.8) - m_parentAircraft->getChannel(CAircraft::m_thrChannel);
        auto throttle = throttlePos / (0.8 * 2.0);

        auto currentLim = static_cast<real_Num>(0.0);
        // the limit depending on the ESC and motor limits (the lower of the two ruling)

        if(m_currentLimit > esc.m_iLimit)
        {
            // if ESC limit is the lower
            currentLim = (esc.m_iLimit + static_cast<real_Num>(0.3) * m_currentLimit);
            // use 100% of the lower limit + 30% of the higher limit
        }
        else
        {
            // if Motor limit is the lower
            currentLim = (m_currentLimit + static_cast<real_Num>(0.3) * esc.m_iLimit);
        }

        m_motorTI =
            static_cast<real_Num>(30.0) * m_motorEfficiency / (Math<real_Num>::pi() * m_motorKV);
        // compute the torque constant of the motor from the KV and efficiency

        auto rpm = getRPM();
        m_motorEMF = rpm / m_motorKV; // calculate the motor back emp

        auto packVoltage = pack.getVoltage();

        auto reqRPM = esc.m_output * m_motorKV * packVoltage;

        auto msr = m_msrGain * ((reqRPM - m_rpm) / (m_motorKV * packVoltage));
        msr = Math<real_Num>::clamp(msr, -1.0, 1.0);

        auto driveVolts = packVoltage - m_motorEMF;
        // this is the voltage available across the pack+motor resistances
        auto resistance = m_motorR + esc.m_resistance;
        m_motorCurrent = msr * (driveVolts / resistance);
        // calc current as (Duty Cycle of ESC)*(V-EMF)/Rtot

        if(m_motorCurrent > esc.m_iLimit)
        {
            m_motorCurrent = currentLim; // apply the calculated current limit
        }

        if(esc.m_cutoffActive)
        {
            m_motorCurrent = static_cast<real_Num>(0.0);
            // if the cutoff is active we zero the motor current
        }

        m_torque = m_motorTI * (m_motorCurrent - m_noLoadCurrent);
        // calc the torque for this current allowing for no-load current;
        m_motorPower = m_omega * m_torque; // calculate the output power of the motor
    }

    void CAircraftMotor::update(const double &t, const double &dt)
    {
        constexpr int Locked = 2;
        constexpr int Overrun = 3;

        const real_Num RPMToOmega = Math<real_Num>::pi() / static_cast<real_Num>(30.0);
        const real_Num OmegaToRPM = static_cast<real_Num>(30.0) / Math<real_Num>::pi();

        CAircraftMotor &EM = *this;

        auto &batteryPack = getBatteryPack();

        SmartPtr<CESController> esc = workphone::static_pointer_cast<CESController>(getESC());
        CESController &ESC = *esc;

        s32 LL, Loops;
        real_Num MicroT;
        real_Num DeltaOmegaL; // the change in the Omega of the locked assembly

        real_Num DeltaOmegaOS; // the change in the sprag Omega in overrun mode

        real_Num DeltaOmegaOM; // the change in motor omega in overrun mode

        bool ModeSwitched;
        // Loops = (s32)std::ceil(real_Num(1.0) + Math<real_Num>::trunc(dt / real_Num(0.001)));
        // //find the number of loops needed for 1ms timesteps if (Loops < 1)
        {
            Loops = 1;
        }

        MicroT = dt / Loops;
        EM.m_spragRPM = m_opRPM;
        // make the Sprag RPM (the final output rpm) equal to the RPM passed to the procedure.
        EM.m_spragOmega = EM.m_spragRPM * static_cast<real_Num>(RPMToOmega);

        for(LL = 0; LL < Loops; LL++)
        {
            // LookupEnginePower(Throttle,TheEngineClutch); //get the engine output
            motorCalc(dt, *workphone::static_pointer_cast<CBatteryPackStandard>(batteryPack),
                      ESC); // todo refactor
            // motorCalc(EM, ESC, batteryPack->getTerminalVoltage());

            real_Num dischargeRate = (m_motorCurrent * MicroT) / static_cast<real_Num>(3600.0);
            batteryPack->setDischargeRate(dischargeRate);
            batteryPack->discharge(m_motorCurrent, dt);

            auto throttlePos = static_cast<real_Num>(0.8) -
                               m_parentAircraft->getChannel(CAircraft::m_thrChannel);
            auto idleRPM = ESC.m_fixedRpm * 0.0;
            auto throttle = throttlePos / (0.8 * 2.0);
            auto rpm = idleRPM + ((ESC.m_fixedRpm - idleRPM) * throttle);
            // setRPM(rpm);

            ModeSwitched = false;
            // switch (EM.m_spragMode)
            //{
            // case Locked:
            //{
            //	if (ModeSwitched == false)
            //	{
            //		DeltaOmegaL = MicroT * (EM.m_torque - LoadTorque) / (EM.m_moi + LoadInertia);
            ////calculate the deltaOmega for the assembly 		EM.m_transmittedTorque = EM.m_torque
                ///-
            // EM.m_moi * DeltaOmegaL / MicroT; //and sebtract the fraction of the motor torque used
            // to accelerate the motor itself 		EM.m_spragOmega = EM.m_spragOmega + DeltaOmegaL;
            ////update the Sprag omega to the new value 		EM.m_spragRPM = EM.m_spragOmega *
            //(real_Num)OmegaToRPM;   //and update the rpm 		EM.m_omega = EM.m_spragOmega; //match
            //the motor omega to the sprag 		EM.m_rpm = EM.m_spragRPM;  //and likewise the rpm
            // if (EM.m_torque < 0)
            //		{
            //			EM.m_spragMode = Overrun;
            //			ModeSwitched = true;
            //		} //if motor torque is less than zero then go to overrun state
            //	}
            // }
            // break; //Locked

            // case Overrun:
            //{
            //	if (ModeSwitched == false)
            //	{
            //		EM.m_transmittedTorque = real_Num(0.0);
            //		DeltaOmegaOS = -MicroT * LoadTorque / LoadInertia; //calculate the slowing of the
            // undriven load inertia 		EM.m_spragOmega = EM.m_spragOmega + DeltaOmegaOS;
            // //update the sprag omega appropriately 		EM.m_spragRPM = EM.m_spragOmega *
            // (real_Num)OmegaToRPM;
            ////update the sprag rpm  to match the omega figure 		DeltaOmegaOM = MicroT *
                ///EM.m_torque /
            // EM.m_moi; //calculate the change in motor omega under its own torque EM.m_omega =
            // EM.m_omega + DeltaOmegaOM;  //and update the motor omega appropriately 		EM.m_rpm
            // = EM.m_omega * (real_Num)OmegaToRPM;  //and update the resulting rpm 		if
            // (EM.m_rpm > EM.m_spragRPM)  //if the motor has accelerated to exceed the sprag speed
            // then go to locked mode
            //		{
            //			EM.m_spragMode = Locked; //set the mode to locked
            //			EM.m_rpm = EM.m_spragRPM; //match the motor to the sprag speed
            //			EM.m_omega = EM.m_spragOmega; //(both omega and rpm)
            //			ModeSwitched = true; //flag the mode change
            //		}
            //	}
            //  }
            //  break; //Overrun
            //  default:
            //{
            //	int stop = 0;
            //	stop = 0;
            //  }
            //
            //  } //case

            if(EM.m_omega < 0)
            {
                EM.m_omega = 0; // stop motor going backwards!
                EM.m_rpm = 0;
            }
        } // for LL

        if(m_parentAircraft->getDisplayDebugData())
        {
            auto localTransform = getLocalTransform();
            auto localPosition = localTransform.getPosition();
            m_parentAircraft->drawPoint(0, 1665165123, localPosition, 0xFF0000);
        }

        static auto nextUpdate = 0.0;
        if(nextUpdate < t)
        {
            WP_LOG("RPM: " + StringUtil::toString( getRPM() ));
            WP_LOG("Torque: " + StringUtil::toString( getTorque() ));
            WP_LOG("MSR Gain: " + StringUtil::toString( m_msrGain ));
            nextUpdate = t + 3.0;
        }
    }

    IAircraftPropeller *CAircraftMotor::getPropellerPtr() const
    {
        return m_propeller.get();
    }

    SmartPtr<IAircraftPropeller> CAircraftMotor::getPropeller() const
    {
        return m_propeller;
    }

    void CAircraftMotor::setPropeller(SmartPtr<IAircraftPropeller> propeller)
    {
        m_propeller = propeller;
    }

    real_Num CAircraftMotor::getRPM() const
    {
        return m_rpm;
    }

    void CAircraftMotor::setRPM(real_Num rpm)
    {
        m_rpm = rpm;
        m_opRPM = rpm;
        m_spragRPM = rpm;
    }

    bool CAircraftMotor::isElectric() const
    {
        return true;
    }

    void CAircraftMotor::setElectric(bool electric)
    {
    }

    real_Num CAircraftMotor::getMaxRPM() const
    {
        return 0.0f;
    }

    f32 CAircraftMotor::getTorque(f32 throttlePosition) const
    {
        return 0.0f;
    }

    f32 CAircraftMotor::getMaxTorque(u32 rpm) const
    {
        return 0.0f;
    }

    f32 CAircraftMotor::getMinTorque(u32 rpm) const
    {
        return 0.0f;
    }

    real_Num CAircraftMotor::getTorque() const
    {
        return m_torque;
    }

    void CAircraftMotor::setTorque(real_Num torque)
    {
        m_torque = torque;
    }

    real_Num CAircraftMotor::getMotorOmega() const
    {
        return m_omega;
    }

    void CAircraftMotor::setMotorOmega(real_Num motorOmega)
    {
        m_omega = motorOmega;
        m_spragOmega = motorOmega;
    }

    real_Num CAircraftMotor::getThrottle() const
    {
        return m_throttle;
    }

    void CAircraftMotor::setThrottle(real_Num throttle)
    {
        m_throttle = throttle;
    }

    real_Num CAircraftMotor::getMoi() const
    {
        return m_moi;
    }

    void CAircraftMotor::setMoi(real_Num moi)
    {
        m_moi = moi;
    }

    real_Num CAircraftMotor::getThrustMultiplier() const
    {
        return m_thrustMultiplier;
    }

    void CAircraftMotor::setThrustMultiplier(real_Num thrustMultiplier)
    {
        m_thrustMultiplier = thrustMultiplier;
    }

    real_Num CAircraftMotor::getTorqueMultiplier() const
    {
        return m_torqueMultiplier;
    }

    void CAircraftMotor::setTorqueMultiplier(real_Num torqueMultiplier)
    {
        m_torqueMultiplier = torqueMultiplier;
    }

    real_Num CAircraftMotor::getPeakPowerW() const
    {
        return 0;
    }

    void CAircraftMotor::setPeakPowerW(real_Num peak_power_w)
    {
    }

    real_Num CAircraftMotor::getMsrGain() const
    {
        return m_msrGain;
    }

    void CAircraftMotor::setMsrGain(real_Num msrGain)
    {
        m_msrGain = msrGain;
    }

    const SmartPtr<IBatteryPack> &CAircraftMotor::getBatteryPack() const
    {
        return m_batteryPack;
    }

    SmartPtr<IBatteryPack> &CAircraftMotor::getBatteryPack()
    {
        return m_batteryPack;
    }

    void CAircraftMotor::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_batteryPack = batteryPack;
    }

    const SmartPtr<IESController> &CAircraftMotor::getESC() const
    {
        return m_esc;
    }

    SmartPtr<IESController> &CAircraftMotor::getESC()
    {
        return m_esc;
    }

    void CAircraftMotor::setESC(SmartPtr<IESController> esc)
    {
        m_esc = esc;
    }

    // modified to bring pack resistive losses out to be handled in the discharge procedure for the
    // sum of the motor currents Procedure motorCalc(Var Motor:TEMotor; ESC:TESController);
    void CAircraftMotor::motorCalc(CAircraftMotor &motor, CESController &esc, float packTerminalV)
    {
        // hack
        auto throttlePos =
            static_cast<real_Num>(0.8) - m_parentAircraft->getChannel(CAircraft::m_thrChannel);
        auto throttle = throttlePos / (0.8 * 2.0);

        m_msrGainFactor = 0.0f; // gain adjustment for crude governor
        float msr;

        float driveVolts;
        float currentLim;
        // the limit depending on the ESC and motor limits (the lower of the two ruling)

        if(m_currentLimit > esc.m_iLimit)
        {
            // if ESC limit is the lower
            currentLim = (0.75f * esc.m_iLimit + 0.25f * m_currentLimit);
            // use 0.75% of the lower limit + 25% of the higher limit
        }
        else
        {
            // if Motor limit is the lower
            currentLim = (0.75f * m_currentLimit + 0.25f * esc.m_iLimit);
        }
        m_motorTI = 30.0f * m_motorEfficiency / (Math<real_Num>::pi() * m_motorKV);
        // compute the torque constant of the motor from the KV and efficiency
        m_motorEMF = m_rpm / m_motorKV; // calculate the motor back emf

        // MSR is a Mark-space ratio value based on how much above/below the target RPM the motor is
        msr = m_msrGainFactor * ((throttle - m_rpm) / (m_motorKV * packTerminalV));
        // MSR is + for drive and neg for braking
        // msr = m_msrGainFactor * (m_throttle - m_rpm / (m_motorKV * packTerminalV));
        // msr = Math<real_Num>::clamp(msr, -1.0, 1.0);
        // if (msr > 1.0)
        //{
        //	msr = 1.0;
        // }
        // if (msr < -1.0)
        //{
        //	msr = -1.0;
        // }

        if(esc.m_braking == true && msr < static_cast<real_Num>(0.0))
        {
            driveVolts = -m_motorEMF;
            // short out the motor if the motor set to braking and throttle is at or below zero
        }
        else // motor is not being braked
        {
            driveVolts = packTerminalV - m_motorEMF;
            // this is the voltage available across the pack+motor resistances
        }

        m_motorCurrent = Math<real_Num>::Abs(msr) * driveVolts / (m_motorR + esc.m_resistance);
        // calc current assuming a dead short on the motor

        if(esc.m_braking == true && throttle <= static_cast<real_Num>(0.0))
        {
            driveVolts = -m_motorEMF;
            // short out the motor if the motor set to braking and throttle is at or below zero
            m_motorCurrent = driveVolts / (m_motorR + esc.m_resistance);
            // calc current assuming a dead short on the motor
        }
        else
        {
            driveVolts = packTerminalV - m_motorEMF;
            // this is the voltage available across the pack+motor resistances
            // m_motorCurrent = Mathf.Pow( Throttle, 2.0f) * driveVolts / ( m_motorR +
            // esc.Resistance); //calc current as (Du
            m_motorCurrent = throttle * driveVolts / (m_motorR + esc.m_resistance);
            // calc current as (Duty Cycle of ESC)*(V-EMF)/Rtot
        }

        if(m_motorCurrent > currentLim)
            m_motorCurrent = currentLim; // apply the calculated current limit
        if(m_motorCurrent < -1.0f * currentLim)
            m_motorCurrent = -1.0f * currentLim;
        // apply 50% of the the calculated current limit for motor braking as well
        if(esc.m_cutoffActive)
            m_motorCurrent = 0.0f; // if the cutoff is active we zero the motor current

        m_torque = m_motorTI * (m_motorCurrent - m_noLoadCurrent);
        // calc the torque for this current allowing for no-load current;
        m_motorPower = m_omega * m_torque; // calculate the output power of the motor

        // m_torque = m_motorTI * throttle;
    }
}
