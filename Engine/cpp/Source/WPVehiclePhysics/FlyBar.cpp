#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/FlyBar.hpp"

namespace workphone
{
    namespace vehicle
    {
        TFlyBar::TFlyBar()
        {
            m_rodDia = static_cast<physics_Num>( 0.0 );
            m_rodLength = static_cast<physics_Num>( 0.0 );
            m_rodDensity = static_cast<physics_Num>( 0.0 );
            m_flyBarSpan = static_cast<physics_Num>( 0.0 );
            m_paddleSpan = static_cast<physics_Num>( 0.0 );
            m_rootChord = static_cast<physics_Num>( 0.0 );
            m_tipChord = static_cast<physics_Num>( 0.0 );
            m_paddleWeight = static_cast<physics_Num>( 0.0 );
            m_radOfGyr = static_cast<physics_Num>( 0.0 );
            m_omega = static_cast<physics_Num>( 0.0 );
            m_ailCyclic = static_cast<physics_Num>( 0.0 );
            m_eleCyclic = static_cast<physics_Num>( 0.0 );
            m_momOfI = static_cast<physics_Num>( 0.0 );
            m_angMom = static_cast<physics_Num>( 0.0 );
            m_forceMoments = static_cast<physics_Num>( 0.0 );
            m_eleAngle = static_cast<physics_Num>( 0.0 );
            m_ailAngle = static_cast<physics_Num>( 0.0 );

            for( auto &i : m_gndEffect )
            {
                i = static_cast<physics_Num>( 0.0 );
            }

            for( auto &i : m_groundDistance )
            {
                i = static_cast<physics_Num>( 0.0 );
            }
        }
    } // namespace vehicle
} // namespace workphone
