#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IRenderer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IRenderer, ISharedObject );

    IRenderer::~IRenderer() = default;

}  // namespace workphone::render
