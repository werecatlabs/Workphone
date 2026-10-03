#ifndef PlaneShapeState_h__
#define PlaneShapeState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Plane3.hpp>

namespace workphone
{

    class WPCore_API PlaneShapeState : public StateData
    {
    public:
        PlaneShapeState();
        ~PlaneShapeState() override;

        Plane3<real_Num> plane;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // PlaneShapeState_h__
