#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/MassData2.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{

    void MassData2::setInertia( real_Num inertia )
    {
        m_inertia = inertia;
    }

    workphone::real_Num MassData2::getInertia() const
    {
        return m_inertia;
    }

    void MassData2::setCenter( Vector2<real_Num> center )
    {
        m_center = center;
    }

    workphone::Vector2<workphone::real_Num> MassData2::getCenter() const
    {
        return m_center;
    }

    void MassData2::setMass( real_Num mass )
    {
        m_mass = mass;
    }

    workphone::real_Num MassData2::getMass() const
    {
        return m_mass;
    }

    MassData2::~MassData2()
    {
    }

    MassData2::MassData2()
    {
    }

} // namespace workphone::physics
