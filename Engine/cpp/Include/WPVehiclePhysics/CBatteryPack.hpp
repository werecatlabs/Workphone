#ifndef EPowerUnit1H
#define EPowerUnit1H

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include "WPVehiclePhysics/HeliVars.hpp"

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API CBatteryPack : public ISharedObject
        {
        public:
            CBatteryPack();
            ~CBatteryPack() override;

            s32         m_cellsInPack = 0; // number of cells in pack
            physics_Num m_cellFullV =
                static_cast<physics_Num>( 0.0 ); // cell off-load voltage when fully charged
            physics_Num m_cellFlatV = static_cast<physics_Num>( 0.0 ); // cell off-load voltage when flat
            physics_Num m_cellV = static_cast<physics_Num>( 0.0 );
            // current Cell off-load voltage at given state of charge
            physics_Num m_cellR = static_cast<physics_Num>( 0.0 );
            // ESR of an individual cell of the pack when full
            physics_Num m_cellAHr = static_cast<physics_Num>( 0.0 ); // cell capacity in Amp.Hrs
            physics_Num m_packState = static_cast<physics_Num>(
                0.0 ); // fractional state of charge of pack (1 = full, 0 = flat)
            physics_Num m_packV = static_cast<physics_Num>( 0.0 );   // pack no-load voltage
            physics_Num m_packR = static_cast<physics_Num>( 0.0 );   // Pack resistance
            physics_Num m_aHrUsed = static_cast<physics_Num>( 0.0 ); // sum of the used AHr used
            physics_Num m_dischargeDuration =
                static_cast<physics_Num>( 0.0 ); // the time for which a non-zero current has been drawn
        };
    } // namespace vehicle
} // namespace workphone

#endif //  EPowerUnit1H
