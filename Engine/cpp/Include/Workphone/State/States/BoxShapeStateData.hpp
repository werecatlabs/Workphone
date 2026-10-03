#ifndef BoxShapeState_h__
#define BoxShapeState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    class WPCore_API BoxShapeStateData : public StateData
    {
    public:
        BoxShapeStateData();
        ~BoxShapeStateData() override;

        WP_CLASS_REGISTER_DECL;

        Vector3<real_Num> extents = Vector3<real_Num>::unit();
    };
}  // namespace workphone

#endif  // BoxShapeState_h__
