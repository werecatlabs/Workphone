#ifndef CAircraftMotor_h__
#define CAircraftMotor_h__

#include <Workphone/Interface/Vehicle/IAircraftPowerUnit.hpp>
#include "WPVehiclePhysics/CAircraftAttachment.hpp"
#include "WPVehiclePhysics/CBatteryPackStandard.hpp"
#include "WPVehiclePhysics/CESController.hpp"
#include "WPVehiclePhysics/CBatteryPack.hpp"

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API CAircraftMotor : public CAircraftAttachment<IAircraftPowerUnit>
        {
        public:
            CAircraftMotor();
            ~CAircraftMotor() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void load( void *pData ) override;

            void update( const double &t, const double &dt ) override;

            IAircraftPropeller          *getPropellerPtr() const override;
            SmartPtr<IAircraftPropeller> getPropeller() const override;
            void                         setPropeller( SmartPtr<IAircraftPropeller> propeller ) override;

            SmartPtr<IBatteryPack>       &getBatteryPack();
            const SmartPtr<IBatteryPack> &getBatteryPack() const;
            void                          setBatteryPack( SmartPtr<IBatteryPack> batteryPack );

            SmartPtr<IESController>       &getESC();
            const SmartPtr<IESController> &getESC() const;
            void                           setESC( SmartPtr<IESController> esc );

            real_Num getRPM() const override;
            void     setRPM( real_Num rpm ) override;

            bool isElectric() const override;
            void setElectric( bool electric ) override;

            real_Num getMaxRPM() const override;
            f32      getTorque( f32 throttlePosition ) const override;

            f32 getMaxTorque( u32 rpm ) const override;
            f32 getMinTorque( u32 rpm ) const override;

            real_Num getTorque() const override;
            void     setTorque( real_Num torque );

            real_Num getMotorOmega() const;
            void     setMotorOmega( real_Num motorOmega );

            real_Num getThrottle() const override;
            void     setThrottle( real_Num throttle ) override;

            real_Num getMoi() const override;
            void     setMoi( real_Num moi ) override;

            real_Num getThrustMultiplier() const override;

            void setThrustMultiplier( real_Num thrustMultiplier ) override;

            real_Num getTorqueMultiplier() const override;

            void setTorqueMultiplier( real_Num torqueMultiplier ) override;

            real_Num getPeakPowerW() const override;
            void     setPeakPowerW( real_Num peak_power_w ) override;

            real_Num getMsrGain() const;
            void     setMsrGain( real_Num msrGain );

        private:
            s32 getDebugId( s32 i ) const;
            s32 getDebugId( s32 section, s32 index ) const;

            void motorCalc( double dt, CBatteryPackStandard &pack, CESController &esc );
            void motorCalc( CAircraftMotor &motor, CESController &esc, float packTerminalV );

            SmartPtr<IAircraftPropeller> m_propeller;
            SmartPtr<IBatteryPack>       m_batteryPack;
            SmartPtr<IESController>      m_esc;

            real_Num m_thrustMultiplier;
            real_Num m_torqueMultiplier;

            real_Num m_msrGainFactor;
            real_Num m_throttle;

            real_Num m_opRPM;
            real_Num m_eRPM;
            real_Num m_loadInertia;
            real_Num m_loadTorque;

            real_Num m_moi;

            real_Num m_motorKV;         // RPM/volt of the motor
            real_Num m_motorTI;         // torque constant (in N.m/A).Derived from KV and efficiency
            real_Num m_motorEfficiency; // fractional efficiency
            real_Num m_noLoadCurrent;   // stated no-load current
            real_Num m_frictionFactor;  // loss factor derived from the
            real_Num m_motorR;          // the resistance of the motor windings

            real_Num m_motorEMF;
            real_Num m_motorCurrent; // the instantanious motor current

            real_Num m_motorAccel; // rate of acceleration of motor:
            real_Num m_motorPower;
            real_Num m_spragRPM;
            real_Num m_spragOmega;
            real_Num m_transmittedTorque;
            real_Num m_currentLimit; // the stated motor continuous current limit

            real_Num m_rpm;    // the current motor RPM
            real_Num m_torque; // the instantanious motor torque
            real_Num m_omega;

            real_Num m_msrGain = static_cast<real_Num>( 0.3 );

            s32 m_spragMode;

            s32 m_id;

            /// Used to generate a unique id.
            static u32 m_idExt;
        };
    } // namespace vehicle
} // namespace workphone

#endif // CElectricMotor_h__
