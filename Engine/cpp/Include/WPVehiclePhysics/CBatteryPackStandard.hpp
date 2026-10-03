#ifndef CBatteryPack_h__
#define CBatteryPack_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include <Workphone/Interface/Vehicle/IBatteryPack.hpp>
#include <WPVehiclePhysics/CAircraftAttachment.hpp>

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API CBatteryPackStandard : public CAircraftAttachment<IBatteryPack>
        {
        public:
            CBatteryPackStandard();
            ~CBatteryPackStandard() override;

            void load( SmartPtr<ISharedObject> data ) override;

            void charge() override;
            void discharge( real_Num I, real_Num dt ) override;

            real_Num getDischargeRate() const override;
            void     setDischargeRate( real_Num dischargeRate ) override;

            real_Num getVolts() const override;
            void     setVolts( real_Num volts ) override;

            real_Num getCharge() const override;
            void     setCharge( real_Num charge ) override;

            s32  getNumCells() const override;
            void setNumCells( s32 numCells ) override;

            real_Num getVoltage() const override;
            void     setVoltage( real_Num voltage ) override;

            real_Num getResistance() const override;
            void     setResistance( real_Num resistance ) override;

            real_Num getTerminalVoltage() const override;
            void     setTerminalVoltage( real_Num terminalVoltage ) override;

            bool getEmulateBattery() const override;
            void setEmulateBattery( bool emulateBattery ) override;

        protected:
            real_Num m_fullVolts;      // cell off-load voltage when fully charged
            real_Num m_flatVolts;      // cell off-load voltage when flat
            real_Num m_offloadVoltage; // current Cell off-load voltage at given state of charge
            real_Num m_cellESR;        // ESR of an individual cell of the pack when full
            real_Num m_ampAHr;         // cell capacity in Amp.Hrs
            real_Num m_charge;         // fractional state of charge of pack (1 = full, 0 = flat)
            real_Num m_voltage;        // pack no-load voltage
            real_Num m_resistance;     // Pack resistance
            real_Num m_terminalVoltage;
            // added to fixedwing to allow pack resistance drop to be calculated for the sum of the motor
            // currents
            real_Num m_ahrUsed;       // sum of the used AHr used
            real_Num m_dischargeTime; // the time for which a non-zero current has been drawn
            real_Num m_deltaAHr;
            s32      m_numCells; // number of cells in pack
            bool     m_emulateBattery;
        };
    } // namespace vehicle
} // namespace workphone

#endif // CBatteryPack_h__
