#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IParticleSystem, ISharedObject );

    IParticleSystem::~IParticleSystem() = default;

}  // namespace workphone::render
