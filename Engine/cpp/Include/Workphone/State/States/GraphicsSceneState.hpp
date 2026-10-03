#ifndef SceneManagerState_h__
#define SceneManagerState_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Core/FixedString.hpp>

namespace workphone
{
    class WPCore_API GraphicsSceneState : public StateData
    {
    public:
        GraphicsSceneState();
        ~GraphicsSceneState() override;

        WP_CLASS_REGISTER_DECL;

        SmartPtr<render::IGraphicsCamera> activeCamera;

        SmartPtr<render::IMaterial> skyboxMaterial;
        SmartPtr<render::ITexture> skyboxTexture;

        ColourF fogColour = ColourF::White;

        f32 depthShadows = 0.0f;

        f32 skyboxDistance = 0.0f;
        f32 skyboxDrawFirst = 0.0f;

        s32 fogMode = fogMode;

        f32 fogExpDensity = 0.0f;
        f32 fogLinearStart = 0.0f;
        f32 fogLinearEnd = 0.0f;

        bool enableSkybox = false;
        bool enableShadows = false;

        FixedString<WP_MAX_CLASSNAME> skyboxMaterialName;
    };
}  // namespace workphone

#endif  // SceneManagerState_h__
