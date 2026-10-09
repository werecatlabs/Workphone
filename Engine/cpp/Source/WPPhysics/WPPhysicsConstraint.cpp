#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsConstraint.hpp>

namespace workphone::physics
{
    WPPhysicsConstraint::WPPhysicsConstraint()
    {
    }

    WPPhysicsConstraint::~WPPhysicsConstraint()
    {
    }

    void *WPPhysicsConstraint::getUserData() const
    {
        return nullptr;
    }

    void WPPhysicsConstraint::setUserData(void *userData)
    {
    }

    void WPPhysicsConstraint::setTypeInfo(u32 id)
    {
        sTypeInfo = id;
    }

    u32 WPPhysicsConstraint::getTypeInfo() const
    {
        return sTypeInfo;
    }

    u32 WPPhysicsConstraint::typeInfo()
    {
        return sTypeInfo;
    }

    void WPPhysicsConstraint::setupTypeInfo()
    {
        sTypeInfo = 0;
    }

    u32 WPPhysicsConstraint::sTypeInfo = 0;
}
