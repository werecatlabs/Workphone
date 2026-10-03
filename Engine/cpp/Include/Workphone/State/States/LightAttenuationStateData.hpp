#ifndef LightAttenuationState_h__
#define LightAttenuationState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>

namespace workphone
{
    class WPCore_API LightAttenuationStateData : public StateData
    {
    public:
        LightAttenuationStateData();

        ~LightAttenuationStateData() override;

        WP_CLASS_REGISTER_DECL;

        Vector4F attenuation = Vector4F::zero();
        f32 range = 1000.0f;
        f32 constant = 1.0f;
        f32 linear = 1.0f;
        f32 quadratic = 1.0f;
        f32 powerScale = 1.0f;
    };
}  // namespace workphone

#endif  // LightAttenuationState_h__
