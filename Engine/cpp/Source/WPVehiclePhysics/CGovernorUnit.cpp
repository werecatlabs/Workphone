#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CGovernorUnit.hpp"
#include "WPVehiclePhysics/HeliAero.hpp"

namespace workphone
{
    namespace vehicle
    {
        CGovernorUnit::CGovernorUnit()
        {
            m_mode = 0;

            m_fixedRpm = static_cast<physics_Num>( 0.0 );
            m_rpmRangeBottom = static_cast<physics_Num>( 0.0 );
            m_rpmRangeTop = static_cast<physics_Num>( 0.0 );
            m_input = static_cast<physics_Num>( 0.0 );
            m_remoteSig = static_cast<physics_Num>( 0.0 );
            m_reqRpm = static_cast<physics_Num>( 0.0 );
            m_lastRpm = static_cast<physics_Num>( 0.0 );
            m_targetRpm = static_cast<physics_Num>( 0.0 );
            m_rampRate = static_cast<physics_Num>( 0.0 );
            m_acceleration = static_cast<physics_Num>( 0.0 );
            m_accTimeConstant = static_cast<physics_Num>( 0.0 );
            m_rpmError = static_cast<physics_Num>( 0.0 );
            m_phaseError = static_cast<physics_Num>( 0.0 );
            m_accGain = static_cast<physics_Num>( 0.0 );
            m_rpmGain = static_cast<physics_Num>( 0.0 );
            m_phaseGain = static_cast<physics_Num>( 0.0 );
            m_accLimit = static_cast<physics_Num>( 0.0 );
            m_rpmErrorLimit = static_cast<physics_Num>( 0.0 );
            m_phaseErrorLimit = static_cast<physics_Num>( 0.0 );
            m_output = static_cast<physics_Num>( 0.0 );
            m_maxControlPoint = static_cast<physics_Num>( 0.0 );
            m_minControlPoint = static_cast<physics_Num>( 0.0 );
            m_resetLimiter = static_cast<physics_Num>( 0.0 );

            m_positiveGrowthEnable = false;
            m_negativeGrowthEnable = false;
            m_active = false;
        }
    } // namespace vehicle
} // namespace workphone
