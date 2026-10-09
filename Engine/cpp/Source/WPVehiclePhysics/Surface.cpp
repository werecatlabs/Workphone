#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/Surface.hpp"

namespace workphone::vehicle
{
    TSurface::TSurface()
    {
        m_iFactor = 0.0;
        m_alphaG = 0.0;
        m_alphaL = 0.0;
        setFlowSpeed(0.0);
        m_cl = 0.0;
        m_cd = 0.0;
        setLift(0.0);
        m_drag = 0.0;
        m_area = 0.0;
        m_chord = 0.0;
        m_span = 0.0;
        m_rad = 0.0;
    }
}
