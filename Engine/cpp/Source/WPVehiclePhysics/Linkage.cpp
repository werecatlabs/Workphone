#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/Linkage.hpp"

namespace workphone
{
    namespace vehicle
    {
        TLinkage::TLinkage()
        {
            m_swashToMainMix = 0.0;
            m_swashToFBMix = 0.0;
            m_fbToMainMix = 0.0;
            m_maxSwashEle = 0.0;
            m_maxSwashAil = 0.0;
            m_collectivePerMM = 0.0;
            m_swashEleAngle = 0.0;
            m_swashAilAngle = 0.0;
        }
    } // namespace vehicle
} // namespace workphone
