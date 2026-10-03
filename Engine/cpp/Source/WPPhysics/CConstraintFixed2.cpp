#include "WPPhysics/WPPhysicsPCH.hpp"
#include <WPPhysics/CConstraintFixed2.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        CConstraintFixed2::CConstraintFixed2()
        {
        }
        CConstraintFixed2::~CConstraintFixed2()
        {
        }

        void *CConstraintFixed2::getUserData() const
        {
            return nullptr;
        }
        void CConstraintFixed2::setUserData( void *userData )
        {
        }

        void CConstraintFixed2::setTypeInfo( u32 id )
        {
            sTypeInfo = id;
        }
        u32 CConstraintFixed2::getTypeInfo() const
        {
            return sTypeInfo;
        }
        u32 CConstraintFixed2::typeInfo()
        {
            return sTypeInfo;
        }
        void CConstraintFixed2::setupTypeInfo()
        {
            sTypeInfo = 0;
        }
        u32 CConstraintFixed2::sTypeInfo = 0;
    } // namespace physics
} // namespace workphone
