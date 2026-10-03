#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/Rotor.hpp"
#include "WPVehiclePhysics/RotorSector.hpp"

namespace workphone
{
    namespace vehicle
    {
        TRotor::TRotor() : m_section( "" )
        {
            m_blades = 0;
            m_minRad = 0.0;
            m_maxRad = 0.0;
            m_cuffChord = 0.0;
            m_tipChord = 0.0;
            m_twist = 0.0;
            m_bladeWeight = 0.0;
            m_radOfGyr = 0.0;
            m_omega = 0.0;
            m_cone = 0.0;
            m_collective = 0.0;
            m_ailCyclic = 0.0;
            m_eleCyclic = 0.0;
            m_momentOfInertia = 0.0;
            m_angularMomentum = 0.0;
            m_eleAngle = 0.0;
            m_ailAngle = 0.0;
            m_totalAngleLimit = 0.0f;

            for( auto &sector : m_sector )
            {
                sector = TRotorSector();
            }

            for( auto &groundEffect : m_groundEffect )
            {
                groundEffect = 0.0;
            }

            for( auto &groundDistance : m_groundDistance )
            {
                groundDistance = 0.0;
            }

            setCw( true );
        }

        TRotor::~TRotor()
        {
            int stop = 0;
            stop = 0;
        }
    } // namespace vehicle
} // namespace workphone
