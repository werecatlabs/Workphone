#ifndef __SkyboxStateData_h__
#define __SkyboxStateData_h__

#include <Workphone/State/States/StateData.hpp>

namespace workphone
{
    class WPCore_API SkyStateData : public StateData
    {
    public:
        SkyStateData();
        ~SkyStateData() override;

        FixedArray<SmartPtr<render::ITexture>, 6> textures;
        SmartPtr<render::IMaterial> material;
        SmartPtr<render::IGraphicsScene> scene;
        f32 distance = 50000.0f;
        bool visible = false;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // __SkyboxStateData_h__
