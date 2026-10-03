#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IRenderTarget.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IRenderTarget, ISharedObject );

    const u8 IRenderTarget::fullscreenFlag = 1 << 1;
    const u8 IRenderTarget::activeFlag = 1 << 2;
    const u8 IRenderTarget::autoupdatedFlag = 1 << 3;

    IRenderTarget::~IRenderTarget() = default;

}  // namespace workphone::render
