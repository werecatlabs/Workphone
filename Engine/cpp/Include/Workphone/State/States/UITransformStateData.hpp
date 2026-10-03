#ifndef UITransformState_h__
#define UITransformState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    class WPCore_API UITransformStateData : public StateData
    {
    public:
        UITransformStateData();
        ~UITransformStateData() override;

        WP_CLASS_REGISTER_DECL;

        Vector2<real_Num> position = Vector2<real_Num>::zero();
        Vector2<real_Num> size = Vector2<real_Num>( 300.0f, 100.0f );

        Vector2<real_Num> absolutePosition = Vector2<real_Num>::zero();
        Vector2<real_Num> absoluteSize = Vector2<real_Num>::zero();

        Vector2<real_Num> absoluteMin = Vector2<real_Num>::zero();
        Vector2<real_Num> absoluteMax = Vector2<real_Num>::zero();

        f32 scale = 1.0f;
        u32 zorder = 0;

        u8 metricsMode = 0;
        u8 gha = 0;
        u8 gva = 0;
    };
}  // namespace workphone

#endif  // UITransformState_h__
