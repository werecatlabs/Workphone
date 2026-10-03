#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CFlybarlessUnit.hpp"
#include "WPVehiclePhysics/HeliAero.hpp"

namespace workphone
{
    namespace vehicle
    {
        CFlybarlessUnit::CFlybarlessUnit()
        {
            m_errorMag = static_cast<physics_Num>( 0.0 );
            m_rollErrorAngle = static_cast<physics_Num>( 0.0 );
            m_pitchErrorAngle = static_cast<physics_Num>( 0.0 );
            m_stickDeadBand = static_cast<physics_Num>( 0.0 );
            m_stickSensitivity = static_cast<physics_Num>( 0.0 );
            m_stickExpo = static_cast<physics_Num>( 0.0 );
            m_rollGain = static_cast<physics_Num>( 0.0 );
            m_pitchGain = static_cast<physics_Num>( 0.0 );
            m_angleLimit = static_cast<physics_Num>( 0.0 );
            m_decay = static_cast<physics_Num>( 0.0 );
            m_stabGain = static_cast<physics_Num>( 0.0 );
            m_directMix = static_cast<physics_Num>( 0.0 );
            m_ailCommand = static_cast<physics_Num>( 0.0 );
            m_eleCommand = static_cast<physics_Num>( 0.0 );
            m_ailDemand = static_cast<physics_Num>( 0.0 );
            m_eleDemand = static_cast<physics_Num>( 0.0 );
            m_ailFilter = static_cast<physics_Num>( 0.0 );
            m_eleFilter = static_cast<physics_Num>( 0.0 );
            m_stabilize = false;
            m_bailValue = 0;
            m_bailGain = static_cast<physics_Num>( 0.0 );
            m_bailCollective = static_cast<physics_Num>( 0.0 );
        }
    } // namespace vehicle
} // namespace workphone
