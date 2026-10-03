#ifndef SphereShapeState_h__
#define SphereShapeState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{

    class WPCore_API SphereShapeStateData : public StateData
    {
    public:
        SphereShapeStateData();
        ~SphereShapeStateData() override;

        Sphere3<real_Num> sphere;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // SphereShapeState_h__
