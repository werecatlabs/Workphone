#ifndef __LightData_h__
#define __LightData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>

namespace workphone
{
    class WPCore_API LightStateData : public StateData
    {
    public:
        LightStateData();
        ~LightStateData() override;

        ColourF diffuseColour = ColourF::White;
        ColourF specularColour = ColourF::White;
        Vector3<real_Num> position = Vector3<real_Num>::zero();
        Vector3<real_Num> direction = Vector3<real_Num>::negativeY();
        Vector3<real_Num> derivedDirection = Vector3<real_Num>::negativeY();
        f32 spotlightInnerAngle = 0.0f;
        f32 spotlightOuterAngle = 0.0f;
        f32 spotlightFalloff = 0.0f; 
        LightTypes lightType = LightTypes::LT_DIRECTIONAL;
        u32 flags = 0;
        u8 renderQueueGroup = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // __LightData_h__
