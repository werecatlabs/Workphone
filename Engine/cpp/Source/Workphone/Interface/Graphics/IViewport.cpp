#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IViewport.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IViewport, ISharedObject );

    const u32 IViewport::overlaysEnabledFlag = ( 1 << 1 );
    const u32 IViewport::skiesEnabledFlag = ( 1 << 2 );
    const u32 IViewport::activeFlag = ( 1 << 3 );
    const u32 IViewport::shadowsEnabledFlag = ( 1 << 4 );
    const u32 IViewport::autoUpdatedFlag = ( 1 << 5 );
    const u32 IViewport::enableUIFlag = ( 1 << 6 );
    const u32 IViewport::clearFlag = ( 1 << 7 );
    const u32 IViewport::enableSceneRenderFlag = ( 1 << 8 );

    IViewport::~IViewport() = default;

}  // namespace workphone::render
