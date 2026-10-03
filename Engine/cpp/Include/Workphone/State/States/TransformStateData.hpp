#ifndef TransformState_h__
#define TransformState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{

    class WPCore_API TransformStateData : public StateData
    {
    public:
        TransformStateData();

        ~TransformStateData() override;

        WP_CLASS_REGISTER_DECL;

        Transform3<real_Num> localTransform;
        Transform3<real_Num> worldTransform;
        Transform3<real_Num> derivedTransform;
    };

}  // namespace workphone

#endif  // TransformState_h__
