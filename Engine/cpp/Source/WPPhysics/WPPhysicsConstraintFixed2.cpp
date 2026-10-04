#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/WPPhysicsConstraintFixed2.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        WPPhysicsConstraintFixed2::WPPhysicsConstraintFixed2()
        {
        }
        WPPhysicsConstraintFixed2::~WPPhysicsConstraintFixed2()
        {
        }

        void *WPPhysicsConstraintFixed2::getUserData() const
        {
            return nullptr;
        }
        void WPPhysicsConstraintFixed2::setUserData( void *userData )
        {
        }

        void WPPhysicsConstraintFixed2::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 WPPhysicsConstraintFixed2::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 WPPhysicsConstraintFixed2::typeInfo()
        {
            return sTypeInfo;
        }
        void WPPhysicsConstraintFixed2::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 WPPhysicsConstraintFixed2::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
