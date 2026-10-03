#ifndef ConstraintFixedStateData_h__
#define ConstraintFixedStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{

    class WPCore_API ConstraintFixedStateData : public StateData
    {
    public:
        ConstraintFixedStateData();
        ~ConstraintFixedStateData() override;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ConstraintFixedStateData_h__
