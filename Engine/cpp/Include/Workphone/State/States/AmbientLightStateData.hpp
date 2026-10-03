#ifndef AmbientLightStateData_h__
#define AmbientLightStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    class WPCore_API AmbientLightStateData : public StateData
    {
    public:
        AmbientLightStateData();
        ~AmbientLightStateData() override;

        WP_CLASS_REGISTER_DECL;

        ColourF ambientColour = ColourF::White;
        ColourF upperHemisphere = ColourF::White;
        ColourF lowerHemisphere = ColourF::White;
        Vector3<real_Num> hemisphereDir = Vector3<real_Num>::unitY();
        f32 envmapScale = 1.0f;
    };
}  // namespace workphone

#endif  // AmbientLightStateData_h__
