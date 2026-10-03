#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IGraphicsCamera, IFrustum );
    const u32 IGraphicsCamera::CameraFlagRenderUI = ( 1 << 1 );
    const u32 IGraphicsCamera::CameraFlagAutoAspectRatio = ( 1 << 2 );

    IGraphicsCamera::IGraphicsCamera() : IFrustum( IGraphicsCamera::typeInfo() )
    {
    }

    IGraphicsCamera::~IGraphicsCamera() = default;

}  // namespace workphone::render
