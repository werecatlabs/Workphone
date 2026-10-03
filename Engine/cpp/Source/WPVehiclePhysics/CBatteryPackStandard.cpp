#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CBatteryPackStandard.hpp>
#include <Workphone/Interface/Vehicle/IAircraft.hpp>
#include <Workphone/Core/Properties.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/WorkphoneHeaders.hpp>

namespace workphone
{
    namespace vehicle
    {
        CBatteryPackStandard::CBatteryPackStandard() :
            m_fullVolts( 4.2 ),
            m_flatVolts( 3.0 ),
            m_offloadVoltage( 0.0 ),
            m_cellESR( 0.007 ),
            m_ampAHr( 3.7 ),
            m_charge( 0.0 ),
            m_voltage( 0.0 ),
            m_resistance( 0.0 ),
            m_ahrUsed( 0.0 ),
            m_dischargeTime( 0.0 ),
            m_numCells( 4 ),
            m_emulateBattery( true )
        {
        }

        CBatteryPackStandard::~CBatteryPackStandard()
        {
        }

        void CBatteryPackStandard::load( SmartPtr<ISharedObject> data )
        {
            auto properties = workphone::dynamic_pointer_cast<Properties>( data );
            if( properties )
            {
                properties->getPropertyValue( "FlEqCellR", m_cellESR );
                properties->getPropertyValue( "FlEqCellAHr", m_ampAHr );
                properties->getPropertyValue( "FlEqCellsInPack", m_numCells );
                properties->getPropertyValue( "FlEqFlightBatteryVoltage", m_fullVolts );
                properties->getPropertyValue( "FlEqCellFullV", m_fullVolts );
                properties->getPropertyValue( "FlEqCellFlatV", m_flatVolts );
            }

            charge();
        }

        void CBatteryPackStandard::charge()
        {
            m_charge = 1;        // set the pack to fully charged
            m_ahrUsed = 0;       // reset the sum of the used AHr used
            m_dischargeTime = 0; // reset the time for which a non-zero current has been drawn
            m_offloadVoltage = m_fullVolts;

            real_Num voltage = m_numCells * m_offloadVoltage;
            setVoltage( voltage );
            setTerminalVoltage( voltage );
        }

        bool CBatteryPackStandard::getEmulateBattery() const
        {
            return m_emulateBattery;
        }

        void CBatteryPackStandard::setEmulateBattery( bool emulateBattery )
        {
            m_emulateBattery = emulateBattery;
        }

        void CBatteryPackStandard::discharge( real_Num I, real_Num dt )
        {
            if( getEmulateBattery() == true )
            {
                if( I > static_cast<real_Num>( 0.0 ) )
                {
                    m_dischargeTime = m_dischargeTime + dt; // sum up the discharge time
                }

                m_deltaAHr =
                    I * dt / static_cast<real_Num>( 3600.0 ); // this is the amount of discharge in AHr
                m_ahrUsed = m_ahrUsed + m_deltaAHr;           // accumulate the used AHr of the pack
                m_charge = m_charge - m_deltaAHr / m_ampAHr;  // and calculate the new state of charge

                // now use our simple three-point curve to get no-load voltage
                real_Num VRange =
                    m_fullVolts - m_flatVolts; // calc the voltage range between full and flat

                if( m_charge > static_cast<real_Num>( 0.9 ) )
                {
                    m_offloadVoltage =
                        static_cast<real_Num>( 0.72 ) * VRange + m_flatVolts +
                        static_cast<real_Num>( 2.8 ) * VRange *
                            ( m_charge - static_cast<real_Num>( 0.9 ) ); // if pack >90% charged
                }
                else
                {
                    if( m_charge > static_cast<real_Num>( 0.05 ) )
                    {
                        m_offloadVoltage = static_cast<real_Num>( 0.44 ) * VRange + m_flatVolts +
                                           static_cast<real_Num>( 0.329 ) * VRange *
                                               ( m_charge - static_cast<real_Num>( 0.05 ) );
                        // else if pack>5% charged
                    }
                    else
                    {
                        m_offloadVoltage =
                            m_flatVolts + static_cast<real_Num>( 8.8 ) * VRange * ( m_charge );
                        // else if pack <5% charged
                    }
                }

                setVoltage( m_numCells * m_offloadVoltage );
                // use the current Cell no-load voltage to calc the total pack no-load voltage
                setResistance( m_numCells * m_cellESR ); // calc the total pack resistance
            }
            else
            {
                m_charge = static_cast<real_Num>( 0.8 );
                // if battery not emulated then maintain steady 80% charge state
                setVoltage( m_numCells * ( m_flatVolts + ( m_fullVolts - m_flatVolts ) * m_charge ) );
                setResistance( m_numCells * m_cellESR );
            }

            // ESCCutoutControl(dt, Pack, ESC);
        }

        real_Num CBatteryPackStandard::getDischargeRate() const
        {
            WP_ASSERT( m_deltaAHr < 1e3 );
            return m_deltaAHr;
        }

        void CBatteryPackStandard::setDischargeRate( real_Num dischargeRate )
        {
            WP_ASSERT( dischargeRate < 1e3 );
            m_deltaAHr = dischargeRate;
        }

        real_Num CBatteryPackStandard::getVolts() const
        {
            return getVoltage();
        }

        void CBatteryPackStandard::setVolts( real_Num volts )
        {
            setVoltage( volts );
        }

        real_Num CBatteryPackStandard::getCharge() const
        {
            WP_ASSERT( m_charge < 1e3 );
            return m_charge;
        }

        void CBatteryPackStandard::setCharge( real_Num charge )
        {
            WP_ASSERT( charge < 1e3 );
            m_charge = charge;
        }

        s32 CBatteryPackStandard::getNumCells() const
        {
            WP_ASSERT( m_numCells < 1e3 );
            return m_numCells;
        }

        void CBatteryPackStandard::setNumCells( s32 numCells )
        {
            WP_ASSERT( numCells < 1e3 );
            m_numCells = numCells;
        }

        real_Num CBatteryPackStandard::getVoltage() const
        {
            WP_ASSERT( m_voltage < 1e3 );
            return m_voltage;
        }

        void CBatteryPackStandard::setVoltage( real_Num voltage )
        {
            WP_ASSERT( voltage < 1e3 );
            m_voltage = voltage;
        }

        real_Num CBatteryPackStandard::getResistance() const
        {
            WP_ASSERT( m_resistance < 1e3 );
            return m_resistance;
        }

        void CBatteryPackStandard::setResistance( real_Num resistance )
        {
            WP_ASSERT( resistance < 1e3 );
            m_resistance = resistance;
        }

        real_Num CBatteryPackStandard::getTerminalVoltage() const
        {
            WP_ASSERT( m_terminalVoltage < 1e3 );
            return m_terminalVoltage;
        }

        void CBatteryPackStandard::setTerminalVoltage( real_Num terminalVoltage )
        {
            WP_ASSERT( terminalVoltage < 1e3 );
            m_terminalVoltage = terminalVoltage;
        }
    } // namespace vehicle
} // namespace workphone
