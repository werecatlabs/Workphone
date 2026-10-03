#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CAircraftMotorTest.hpp>
#include "WPVehiclePhysics/CAircraftPropeller.hpp"
#include "WPVehiclePhysics/CAircraftBody.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    u32 CAircraftMotorTest::m_idExt = 0;

    CAircraftMotorTest::CAircraftMotorTest() :
        m_moi( 0.0000036 ),
        m_motorKv( 2300 ),
        m_motorTi( 0 ),
        m_motorEfficiency( 0.8 ),

        m_noLoadCurrent( 0.5 ),
        m_frictionFactor( 0 ),
        m_motorR( 0 ),

        m_motorEmf( 0 ),
        m_motorCurrent( 0 ),
        m_motorAccel( 0 ),

        m_motorPower( 0 ),
        m_spragRpm( 0 ),
        m_spragOmega( 0 ),

        m_transmittedTorque( 0 ),
        m_currentLimit( 0 ),
        m_rpm( 0 ),

        m_torque( 0 ),
        m_omega( 0 ),
        m_spragMode( 0 )
    {
        m_rpm = 0;
        m_omega = 0;
        m_motorEmf = 0;
        m_motorCurrent = 0;
        m_torque = 0;
        m_motorPower = 0;
        m_motorAccel = 0;
        m_spragRpm = 0;
        m_spragOmega = 0;
        m_spragMode = 2; // set the motor mode to a realistic one!

        m_opRpm = 0;
        m_eRpm = 0;
        m_loadInertia = 0;
        m_loadTorque = 0;

        m_id = StringUtil::parseInt( "Motor" + StringUtil::toString( m_idExt++ ) );
    }

    CAircraftMotorTest::~CAircraftMotorTest()
    {
    }

    void CAircraftMotorTest::load( SmartPtr<ISharedObject> data )
    {
        auto properties = workphone::dynamic_pointer_cast<Properties>( data );
        if( properties )
        {
            properties->getPropertyValue( "FlEqMotorMoI", m_moi );
            properties->getPropertyValue( "FlEqMotorKV", m_motorKv );
            properties->getPropertyValue( "FlEqMotorEfficiency", m_motorEfficiency );
            properties->getPropertyValue( "FlEqMotorNoLoadCurrent", m_noLoadCurrent );
            properties->getPropertyValue( "FlEqMotorR", m_motorR );
            properties->getPropertyValue( "FlEqMotorILimit", m_currentLimit );
        }
    }

    void CAircraftMotorTest::load( void *pData )
    {
        // auto data = static_cast<data::aircraft_engine_data *>(pData);
        // m_torqueMultiplier = data->torqueMultiplier;
        // m_thrustMultiplier = data->thrustMultiplier;
    }

    void CAircraftMotorTest::MotorCalc( CBatteryPackStandard &Pack, CESController &ESC )
    {
        real_Num currentLim = static_cast<real_Num>( 0.0 );
        // the limit depending on the ESC and motor limits (the lower of the two ruling)

        if( m_currentLimit > ESC.m_iLimit )
        {
            // if ESC limit is the lower
            currentLim = ( ESC.m_iLimit + static_cast<real_Num>( 0.3 ) * m_currentLimit );
            // use 100% of the lower limit + 30% of the higher limit
        }
        else
        {
            // if Motor limit is the lower
            currentLim = ( m_currentLimit + static_cast<real_Num>( 0.3 ) * ESC.m_iLimit );
        }

        m_motorTi =
            static_cast<real_Num>( 30.0 ) * m_motorEfficiency / ( Math<real_Num>::pi() * m_motorKv );
        // compute the torque constant of the motor from the KV and efficiency
        m_motorEmf = m_rpm / m_motorKv; // calculate the motor back emp
        auto DriveVolts = Pack.getVoltage() - m_motorEmf;
        // this is the voltage available across the pack+motor resistances
        m_motorCurrent = ESC.m_output * DriveVolts / ( Pack.getResistance() + m_motorR + ESC.m_resistance );
        // calc current as (Duty Cycle of ESC)*(V-EMF)/Rtot

        if( m_motorCurrent > ESC.m_iLimit )
        {
            m_motorCurrent = currentLim; // apply the calculated current limit
        }

        if( ESC.m_cutoffActive )
        {
            m_motorCurrent = static_cast<real_Num>( 0.0 );
            // if the cutoff is active we zero the motor current
        }

        m_torque = m_motorTi * ( m_motorCurrent - m_noLoadCurrent );
        // calc the torque for this current allowing for no-load current;
        m_motorPower = m_omega * m_torque; // calculate the output power of the motor
    }

    void CAircraftMotorTest::update( const double &t, const double &dt )
    {
        // real_Num motorIn =  m_parentAircraft->getChannel(CAircraft::THR_CHANNEL);

        // float Thr = 0.0f;
        ////first sort out the throttle signal from the raw throttle channel value and allow for slightly
        /// different rim state for IC and electric

        // if (isElectric())
        //{
        //	if (motorIn > 0)
        //	{   //this is the bottom half of the stick electric
        //		Thr = 0.5f - 0.625f * motorIn;
        //	}
        //	else
        //	{  //this is the top half of the stick Electric
        //		Thr = 0.5f - 0.725f * motorIn;
        //	}
        // } // end of model is electric
        // else //ie model is IC
        //{
        //	if (motorIn > 0.0f)
        //	{  //this is the bottom half of the stick IC
        //		Thr = 0.4f - 0.482f * motorIn; //add in a bit of trim to make the engine tick over
        //	}
        //	else
        //	{  //this is the top half of the stick IC
        //		Thr = 0.4f - 0.750f * motorIn;
        //	} // end top half stick IC
        // }// end model is IC

        // Thr = Math<real_Num>::clamp(Thr, real_Num(0.0), real_Num(1.0));
        // Throttle = Thr;

        ////todo
        // Throttle = (real_Num(0.8) + motorIn) / (real_Num(0.8) * real_Num(2.0));
        // Throttle = 1.0 - Throttle;

        const int Locked = 2;
        const int Overrun = 3;

        const real_Num RPMToOmega = Math<real_Num>::pi() / static_cast<real_Num>( 30.0 );
        const real_Num OmegaToRPM = static_cast<real_Num>( 30.0 ) / Math<real_Num>::pi();

        CAircraftMotorTest &EM = *this;

        auto &batteryPack = getBatteryPack();

        auto           esc = workphone::static_pointer_cast<CESController>( getESC() );
        CESController &ESC = *esc;

        s32      LL, Loops;
        real_Num MicroT;
        real_Num DeltaOmegaL; // the change in the Omega of the locked assembly

        real_Num DeltaOmegaOS; // the change in the sprag Omega in overrun mode

        real_Num DeltaOmegaOM; // the change in motor omega in overrun mode

        bool ModeSwitched;
        Loops = static_cast<s32>(
            std::ceil( static_cast<real_dNum>( 1.0 ) +
                       Math<real_dNum>::trunc( dt / static_cast<real_dNum>( 0.001 ) ) ) );
        // find the number of loops needed for 1ms timesteps
        if( Loops < 1 )
        {
            Loops = 1;
        }

        MicroT = dt / static_cast<real_dNum>( Loops );
        EM.m_spragRpm = m_opRpm;
        // make the Sprag RPM (the final output rpm) equal to the RPM passed to the procedure.
        EM.m_spragOmega = EM.m_spragRpm * static_cast<real_Num>( RPMToOmega );

        for( LL = 0; LL < Loops; LL++ )
        {
            // LookupEnginePower(Throttle,TheEngineClutch); //get the engine output
            MotorCalc( *workphone::static_pointer_cast<CBatteryPackStandard>( batteryPack ),
                       ESC ); // todo refactor
            // MotorCalc(EM, ESC, batteryPack->getTerminalVoltage());

            real_Num dischargeRate = ( m_motorCurrent * MicroT ) / static_cast<real_Num>( 3600.0 );
            batteryPack->setDischargeRate( dischargeRate );
            // batteryPack->discharge(TODO);

            ModeSwitched = false;
            // switch (EM.SpragMode)
            //{
            // case Locked:
            //{
            //	if (ModeSwitched == false)
            //	{
            //		DeltaOmegaL = MicroT * (EM.MotorTorque - LoadTorque) / (EM.MotorMoI + LoadInertia);
            ////calculate the deltaOmega for the assembly 		EM.TransmittedTorque = EM.MotorTorque -
            // EM.MotorMoI * DeltaOmegaL / MicroT; //and sebtract the fraction of the motor torque used
            // to accelerate the motor itself 		EM.SpragOmega = EM.SpragOmega + DeltaOmegaL; //update
            // the Sprag omega to the new value 		EM.SpragRPM = EM.SpragOmega *
            // (real_Num)OmegaToRPM;   //and update the rpm 		EM.MotorOmega = EM.SpragOmega;
            // //match the motor omega to the sprag 		EM.m_rpm = EM.SpragRPM;  //and likewise the
            //rpm 		if (EM.MotorTorque < 0)
            //		{
            //			EM.SpragMode = Overrun;
            //			ModeSwitched = true;
            //		} //if motor torque is less than zero then go to overrun state
            //	}
            //  }
            //  break; //Locked

            // case Overrun:
            //{
            //	if (ModeSwitched == false)
            //	{
            //		EM.TransmittedTorque = real_Num(0.0);
            //		DeltaOmegaOS = -MicroT * LoadTorque / LoadInertia; //calculate the slowing of the
            // undriven load inertia 		EM.SpragOmega = EM.SpragOmega + DeltaOmegaOS; //update the
            // sprag omega appropriately 		EM.SpragRPM = EM.SpragOmega * (real_Num)OmegaToRPM;
            // //update the sprag rpm  to match the omega figure 		DeltaOmegaOM = MicroT *
            // EM.MotorTorque / EM.MotorMoI;
            ////calculate the change in motor omega under its own torque 		EM.MotorOmega =
            ///EM.MotorOmega +
            // DeltaOmegaOM;  //and update the motor omega appropriately 		EM.m_rpm = EM.MotorOmega
            // * (real_Num)OmegaToRPM;  //and update the resulting rpm 		if (EM.m_rpm > EM.SpragRPM)
            ////if the motor has accelerated to exceed the sprag speed then go to locked mode
            //		{
            //			EM.SpragMode = Locked; //set the mode to locked
            //			EM.m_rpm = EM.SpragRPM; //match the motor to the sprag speed
            //			EM.MotorOmega = EM.SpragOmega; //(both omega and rpm)
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

            if( EM.m_omega < 0 )
            {
                EM.m_omega = 0; // stop motor going backwards!
                EM.m_rpm = 0;
            }
        } // for LL
    }

    IAircraftPropeller *CAircraftMotorTest::getPropellerPtr() const
    {
        return m_propeller.get();
    }

    SmartPtr<IAircraftPropeller> CAircraftMotorTest::getPropeller() const
    {
        return m_propeller;
    }

    void CAircraftMotorTest::setPropeller( SmartPtr<IAircraftPropeller> propeller )
    {
        m_propeller = propeller;
    }

    real_Num CAircraftMotorTest::getRPM() const
    {
        return m_spragRpm;
    }

    void CAircraftMotorTest::setRPM( real_Num rpm )
    {
        m_rpm = rpm;
        m_opRpm = rpm;
        m_spragRpm = rpm;
    }

    bool CAircraftMotorTest::isElectric() const
    {
        return true;
    }

    void CAircraftMotorTest::setElectric( bool electric )
    {
    }

    real_Num CAircraftMotorTest::getMaxRPM() const
    {
        return 0.0;
    }

    f32 CAircraftMotorTest::getTorque( f32 throttlePosition ) const
    {
        return 0.0f;
    }

    f32 CAircraftMotorTest::getMaxTorque( u32 rpm ) const
    {
        return 0.0f;
    }

    f32 CAircraftMotorTest::getMinTorque( u32 rpm ) const
    {
        return 0.0f;
    }

    real_Num CAircraftMotorTest::getTorque() const
    {
        return m_torque;
    }

    void CAircraftMotorTest::setTorque( real_Num torque )
    {
        m_torque = torque;
    }

    real_Num CAircraftMotorTest::getMotorOmega() const
    {
        return m_omega;
    }

    void CAircraftMotorTest::setMotorOmega( real_Num motorOmega )
    {
        m_omega = motorOmega;
        m_spragOmega = motorOmega;
    }

    real_Num CAircraftMotorTest::getThrottle() const
    {
        return m_throttle;
    }

    void CAircraftMotorTest::setThrottle( real_Num throttle )
    {
        m_throttle = throttle;
    }

    real_Num CAircraftMotorTest::getMoi() const
    {
        return m_moi;
    }

    void CAircraftMotorTest::setMoi( real_Num moi )
    {
        m_moi = moi;
    }

    real_Num CAircraftMotorTest::getThrustMultiplier() const
    {
        return m_thrustMultiplier;
    }

    void CAircraftMotorTest::setThrustMultiplier( real_Num thrustMultiplier )
    {
        m_thrustMultiplier = thrustMultiplier;
    }

    real_Num CAircraftMotorTest::getTorqueMultiplier() const
    {
        return m_torqueMultiplier;
    }

    void CAircraftMotorTest::setTorqueMultiplier( real_Num torqueMultiplier )
    {
        m_torqueMultiplier = torqueMultiplier;
    }

    const SmartPtr<IBatteryPack> &CAircraftMotorTest::getBatteryPack() const
    {
        return m_batteryPack;
    }

    SmartPtr<IBatteryPack> &CAircraftMotorTest::getBatteryPack()
    {
        return m_batteryPack;
    }

    void CAircraftMotorTest::setBatteryPack( SmartPtr<IBatteryPack> batteryPack )
    {
        m_batteryPack = batteryPack;
    }

    const SmartPtr<IESController> &CAircraftMotorTest::getESC() const
    {
        return m_esc;
    }

    SmartPtr<IESController> &CAircraftMotorTest::getESC()
    {
        return m_esc;
    }

    void CAircraftMotorTest::setESC( SmartPtr<IESController> esc )
    {
        m_esc = esc;
    }

    // modified to bring pack resistive losses out to be handled in the discharge procedure for the sum
    // of the motor currents Procedure MotorCalc(Var Motor:TEMotor; ESC:TESController);
    void CAircraftMotorTest::MotorCalc( CAircraftMotorTest &motor, CESController &esc,
                                        float packTerminalV )
    {
        m_msrGainFactor = 4.0f; // gain adjustment for crude governor
        float MSR;

        float DriveVolts;
        float CurrentLim; // the limit depending on the ESC and motor limits (the lower of the two
                          // ruling)

        if( m_currentLimit > esc.m_iLimit )
        {
            // if ESC limit is the lower
            CurrentLim = ( 0.75f * esc.m_iLimit + 0.25f * m_currentLimit );
            // use 0.75% of the lower limit + 25% of the higher limit
        }
        else
        {
            // if Motor limit is the lower
            CurrentLim = ( 0.75f * m_currentLimit + 0.25f * esc.m_iLimit );
        }
        m_motorTi = 30.0f * m_motorEfficiency / ( Math<real_Num>::pi() * m_motorKv );
        // compute the torque constant of the motor from the KV and efficiency
        m_motorEmf = m_rpm / m_motorKv; // calculate the motor back emf

        // MSR is a Mark-space ratio value based on how much above/below the target RPM the motor is
        // MSR = MSRGainFactor * ((Throttle - m_rpm) / (MotorKV * packTerminalV)); //MSR is + for drive
        // and neg for braking
        MSR = m_msrGainFactor * ( m_throttle - m_rpm / ( m_motorKv * packTerminalV ) );
        MSR = Math<real_Num>::clamp( MSR, -1.0, 1.0 );
        if( MSR > 1.0 )
        {
            MSR = 1.0;
        }
        if( MSR < -1.0 )
        {
            MSR = -1.0;
        }

        if( esc.m_braking == true && MSR < static_cast<real_Num>( 0.0 ) )
        {
            DriveVolts = -m_motorEmf;
            // short out the motor if the motor set to braking and throttle is at or below zero
        }
        else // motor is not being braked
        {
            DriveVolts = packTerminalV - m_motorEmf;
            // this is the voltage available across the pack+motor resistances
        }

        m_motorCurrent = Math<real_Num>::Abs( MSR ) * DriveVolts / ( m_motorR + esc.m_resistance );
        // calc current assuming a dead short on the motor

        // if (esc.Braking == true &&  Throttle <= real_Num(0.0))
        //{
        //	DriveVolts = - MotorEMF;  //short out the motor if the motor set to braking and throttle is
        // at or below zero 	 MotorCurrent = DriveVolts / ( MotorR + esc.Resistance); //calc current
        // assuming a dead short on the motor
        // }
        // else
        //{
        //	DriveVolts = packTerminalV -  MotorEMF; //this is the voltage available across the pack+motor
        // resistances
        //	//MotorCurrent = Mathf.Pow( Throttle, 2.0f) * DriveVolts / ( MotorR + esc.Resistance); //calc
        // current as (Du 	MotorCurrent =  Throttle * DriveVolts / ( MotorR + esc.Resistance); //calc
        // current as (Duty Cycle of ESC)*(V-EMF)/Rtot
        // }

        if( m_motorCurrent > CurrentLim )
            m_motorCurrent = CurrentLim; // apply the calculated current limit
        if( m_motorCurrent < -1.0f * CurrentLim )
            m_motorCurrent = -1.0f * CurrentLim;
        // apply 50% of the the calculated current limit for motor braking as well
        if( esc.m_cutoffActive )
            m_motorCurrent = 0.0f; // if the cutoff is active we zero the motor current

        m_torque = m_motorTi * ( m_motorCurrent - m_noLoadCurrent );
        // calc the torque for this current allowing for no-load current;
        m_motorPower = m_omega * m_torque; // calculate the output power of the motor
    }
} // namespace workphone::vehicle
