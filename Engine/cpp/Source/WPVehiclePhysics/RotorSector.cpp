#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/RotorSector.hpp"
#include "WPVehiclePhysics/Surface.hpp"

namespace workphone::vehicle
{
    TRotorSector::TRotorSector() : m_secPower(0.0)
    {
        for(auto &surface : m_arc)
        {
            surface = TSurface();
        }
    }
}
