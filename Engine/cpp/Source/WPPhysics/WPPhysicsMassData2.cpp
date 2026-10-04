#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/WPPhysicsMassData2.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{

    void WPPhysicsMassData2::setInertia( real_Num inertia )
    {
        m_inertia = inertia;
    }

    workphone::real_Num WPPhysicsMassData2::getInertia() const
    {
        return m_inertia;
    }

    void WPPhysicsMassData2::setCenter( Vector2<real_Num> center )
    {
        m_center = center;
    }

    workphone::Vector2<workphone::real_Num> WPPhysicsMassData2::getCenter() const
    {
        return m_center;
    }

    void WPPhysicsMassData2::setMass( real_Num mass )
    {
        m_mass = mass;
    }

    workphone::real_Num WPPhysicsMassData2::getMass() const
    {
        return m_mass;
    }

    WPPhysicsMassData2::~WPPhysicsMassData2()
    {
    }

    WPPhysicsMassData2::WPPhysicsMassData2()
    {
    }

} // namespace workphone::physics
