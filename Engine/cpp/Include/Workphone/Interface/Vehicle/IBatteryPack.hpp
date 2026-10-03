#ifndef IBatteryPack_h__
#define IBatteryPack_h__

#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {

        /**
         * @brief Interface for a battery pack component of a vehicle.
         */
        class WPCore_API IBatteryPack : public IVehicleComponent
        {
        public:
            /** Destructor */
            ~IBatteryPack() override;

            /**
             * @brief Charges the battery.
             */
            virtual void charge() = 0;

            /**
             * @brief Discharges the battery over a given time period.
             * @param I The current.
             * @param dt The time period.
             */
            virtual void discharge( real_Num I, real_Num dt ) = 0;

            /**
             * @brief Gets the discharge rate of the battery.
             * @return The discharge rate.
             */
            virtual real_Num getDischargeRate() const = 0;

            /**
             * @brief Sets the discharge rate of the battery.
             * @param dischargeRate The discharge rate to set.
             */
            virtual void setDischargeRate( real_Num dischargeRate ) = 0;

            /**
             * @brief Gets the voltage of the battery.
             * @return The voltage.
             */
            virtual real_Num getVolts() const = 0;

            /**
             * @brief Sets the voltage of the battery.
             * @param volts The voltage to set.
             */
            virtual void setVolts( real_Num volts ) = 0;

            /**
             * @brief Gets the charge of the battery.
             * @return The charge.
             */
            virtual real_Num getCharge() const = 0;

            /**
             * @brief Sets the charge of the battery.
             * @param charge The charge to set.
             */
            virtual void setCharge( real_Num charge ) = 0;

            /**
             * @brief Gets the number of cells in the battery.
             * @return The number of cells.
             */
            virtual s32 getNumCells() const = 0;

            /**
             * @brief Sets the number of cells in the battery.
             * @param numCells The number of cells to set.
             */
            virtual void setNumCells( s32 numCells ) = 0;

            /**
             * @brief Gets the voltage of each cell in the battery.
             * @return The voltage of each cell.
             */
            virtual real_Num getVoltage() const = 0;

            /**
             * @brief Sets the voltage of each cell in the battery.
             * @param voltage The voltage of each cell to set.
             */
            virtual void setVoltage( real_Num voltage ) = 0;

            /**
             * @brief Gets the resistance of the battery.
             * @return The resistance.
             */
            virtual real_Num getResistance() const = 0;

            /**
             * @brief Sets the resistance of the battery.
             * @param resistance The resistance to set.
             */
            virtual void setResistance( real_Num resistance ) = 0;

            /**
             * @brief Gets the terminal voltage of the battery.
             * @return The terminal voltage.
             */
            virtual real_Num getTerminalVoltage() const = 0;

            /**
             * @brief Sets the terminal voltage of the battery.
             * @param terminalVoltage The terminal voltage to set.
             */
            virtual void setTerminalVoltage( real_Num terminalVoltage ) = 0;

            /**
             * @brief Determines whether the battery is emulated.
             * @return Whether the battery is emulated.
             */
            virtual bool getEmulateBattery() const = 0;

            /**
             * @brief Sets whether the battery is emulated.
             * @param emulateBattery Whether to emulate the battery.
             */
            virtual void setEmulateBattery( bool emulateBattery ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace vehicle
}  // namespace workphone

#endif  // IBatteryPack_h__
