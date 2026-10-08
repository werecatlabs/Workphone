#ifndef CameraStateData_h__
#define CameraStateData_h__

#include <Workphone/State/States/StateData.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Vector4.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>

namespace workphone
{

    class WPCore_API CameraStateData : public StateData
    {
    public:
        CameraStateData();

        ~CameraStateData() override;

        SmartPtr<render::ITexture> targetTexture;
        WeakPtr<render::IViewport> viewport;
        render::IGraphicsCamera::PostProcessSettings postProcessSettings;
        Array<render::IGraphicsCamera::CompositeLayer> compositeLayers;

        f32 lodBias = 1.0f;
        u32 flags = ( 1u << 1 );  // Cameras render UI by default.
        Vector4F windowDimensions = Vector4F( 0.0f, 0.0f, 1.0f, 1.0f );

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // CameraStateData_h__
