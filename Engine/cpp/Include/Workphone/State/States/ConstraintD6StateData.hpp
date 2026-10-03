#ifndef ConstraintD6StateData_h__
#define ConstraintD6StateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Interface/Physics/PhysicsTypes.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{

    class WPCore_API ConstraintD6StateData : public StateData
    {
    public:
        ConstraintD6StateData();
        ~ConstraintD6StateData() override;

        Transform3<real_Num> drivePosition;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ConstraintD6StateData_h__
