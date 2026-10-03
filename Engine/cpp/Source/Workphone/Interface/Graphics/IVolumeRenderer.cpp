#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IVolumeRenderer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IVolumeRenderer, ISharedObject );

    IVolumeRenderer::~IVolumeRenderer() = default;
}  // namespace workphone::render
