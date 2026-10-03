#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IParticleRenderer.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, IParticleRenderer, ISharedObject );

    IParticleRenderer::~IParticleRenderer() = default;

}  // namespace workphone::render
