#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxConstraintLimit.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {

        WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxConstraintLimit, ConstraintLimit );

        PhysxConstraintLimit::PhysxConstraintLimit()
        {
        }

        PhysxConstraintLimit::~PhysxConstraintLimit()
        {
        }

    } // namespace physics
} // namespace workphone
