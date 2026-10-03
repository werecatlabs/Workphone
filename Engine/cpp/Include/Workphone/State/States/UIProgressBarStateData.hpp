#ifndef UIProgressBarStateData_h__
#define UIProgressBarStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Core/ColourF.hpp>

namespace workphone
{

    class WPCore_API UIProgressBarStateData : public StateData
    {
    public:
        UIProgressBarStateData();
        ~UIProgressBarStateData() override;

        WP_CLASS_REGISTER_DECL;

        ColourF fillColour = ColourF( 0.2f, 0.75f, 0.35f, 1.0f );
        ColourF backgroundColour = ColourF( 0.05f, 0.06f, 0.07f, 0.85f );
        ColourF borderColour = ColourF( 0.0f, 0.0f, 0.0f, 0.75f );
        ColourF textColour = ColourF( 1.0f, 1.0f, 1.0f, 1.0f );

        f32 borderWidth = 1.0f;
        f32 rounding = 4.0f;
        f32 value = 1.0f;
        f32 minValue = 0.0f;
        f32 maxValue = 1.0f;
        bool showText = false;
    };

}  // namespace workphone

#endif  // UIProgressBarStateData_h__
