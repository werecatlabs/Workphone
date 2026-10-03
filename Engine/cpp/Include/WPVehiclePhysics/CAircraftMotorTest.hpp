#ifndef CAircraftMotorTest_h__
#define CAircraftMotorTest_h__

#include <Workphone/Interface/Vehicle/IAircraftPowerUnit.hpp>
#include "WPVehiclePhysics/CAircraftAttachment.hpp"
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include "WPVehiclePhysics/CBatteryPackStandard.hpp"
#include "WPVehiclePhysics/CESController.hpp"

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API CAircraftMotorTest : public CAircraftAttachment<IAircraftPowerUnit>
        {
        public:
            CAircraftMotorTest();
            ~CAircraftMotorTest() override;

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
            void     setThrustMultiplier( real_Num thrustMultiplier ) override;

            real_Num getTorqueMultiplier() const override;
            void     setTorqueMultiplier( real_Num torqueMultiplier ) override;

        private:
            s32 getDebugId( s32 i ) const;
            s32 getDebugId( s32 section, s32 index ) const;

            void MotorCalc( CBatteryPackStandard &Pack, CESController &ESC );
            void MotorCalc( CAircraftMotorTest &motor, CESController &esc, float packTerminalV );

            SmartPtr<IAircraftPropeller> m_propeller;
            SmartPtr<IBatteryPack>       m_batteryPack;
            SmartPtr<IESController>      m_esc;

            real_Num m_thrustMultiplier;
            real_Num m_torqueMultiplier;

            real_Num m_msrGainFactor;
            real_Num m_throttle;

            real_Num m_opRpm;
            real_Num m_eRpm;
            real_Num m_loadInertia;
            real_Num m_loadTorque;

            real_Num m_moi;

            real_Num m_motorKv;         // RPM/volt of the motor
            real_Num m_motorTi;         // torque constant (in N.m/A).Derived from KV and efficiency
            real_Num m_motorEfficiency; // fractional efficiency
            real_Num m_noLoadCurrent;   // stated no-load current
            real_Num m_frictionFactor;  // loss factor derived from the
            real_Num m_motorR;          // the resistance of the motor windings

            real_Num m_motorEmf;
            real_Num m_motorCurrent; // the instantanious motor current

            real_Num m_motorAccel; // rate of acceleration of motor:
            real_Num m_motorPower;
            real_Num m_spragRpm;
            real_Num m_spragOmega;
            real_Num m_transmittedTorque;
            real_Num m_currentLimit; // the stated motor continuous current limit

            real_Num m_rpm;    // the current motor RPM
            real_Num m_torque; // the instantanious motor torque
            real_Num m_omega;

            s32 m_spragMode;

            s32 m_id;

            /// Used to generate a unique id.
            static u32 m_idExt;
        };
    } // namespace vehicle
} // namespace workphone

#endif // CAircraftMotorTest_h__
