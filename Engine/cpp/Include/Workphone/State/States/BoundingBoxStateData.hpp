#ifndef BoundingBoxState_h__
#define BoundingBoxState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    class WPCore_API BoundingBoxStateData : public StateData
    {
    public:
        BoundingBoxStateData();
        ~BoundingBoxStateData() override;

        WP_CLASS_REGISTER_DECL;

        AABB3<real_Num> localAABB;
        AABB3<real_Num> worldAABB;
    };
}  // namespace workphone

#endif  // BoundingBoxState_h__
