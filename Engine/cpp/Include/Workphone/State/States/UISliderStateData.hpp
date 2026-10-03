#ifndef UISliderStateData_h__
#define UISliderStateData_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{

    class WPCore_API UISliderStateData : public StateData
    {
    public:
        UISliderStateData();
        ~UISliderStateData() override;

        WP_CLASS_REGISTER_DECL;

        FixedString<64> label;
        f32 value = 0.5f;
        f32 minValue = 0.0f;
        f32 maxValue = 1.0f;
        bool dragging = false;
        bool showValue = true;
        Direction direction = Direction::Horizontal;
    };

}  // namespace workphone

#endif  // UISliderStateData_h__
