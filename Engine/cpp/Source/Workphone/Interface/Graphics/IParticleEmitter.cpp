#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IParticleEmitter.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IParticleEmitter, ISharedObject );

    IParticleEmitter::~IParticleEmitter() = default;

}  // namespace workphone::render
