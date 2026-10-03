#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/EngineClutchUnit.hpp"
#include "WPVehiclePhysics/HeliVars.hpp"
#include "WPVehiclePhysics/HeliAero.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::vehicle
{
    EngineClutchUnit::EngineClutchUnit() = default;

    EngineClutchUnit::~EngineClutchUnit() = default;

    physics_Num EngineClutchUnit::getCrankRPM() const
    {
        WP_ASSERT( Math<physics_Num>::isFinite( m_crankRPM ) );
        return m_crankRPM;
    }

    void EngineClutchUnit::setCrankRPM( physics_Num rpm )
    {
        WP_ASSERT( Math<physics_Num>::isFinite( rpm ) );
        m_crankRPM = rpm;
    }

    physics_Num EngineClutchUnit::getCrankOmega() const
    {
        WP_ASSERT( Math<physics_Num>::isFinite( m_crankOmega ) );
        return m_crankOmega;
    }

    void EngineClutchUnit::setCrankOmega( physics_Num omega )
    {
        WP_ASSERT( Math<physics_Num>::isFinite( omega ) );
        m_crankOmega = omega;
    }

    physics_Num EngineClutchUnit::getEngineTorque() const
    {
        WP_ASSERT( Math<physics_Num>::isFinite( m_engineTorque ) );
        return m_engineTorque;
    }

    void EngineClutchUnit::setEngineTorque( physics_Num torque )
    {
        WP_ASSERT( Math<physics_Num>::isFinite( torque ) );
        m_engineTorque = torque;
    }

}  // namespace workphone::vehicle
