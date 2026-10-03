#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IParticleManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IParticleManager, ISharedObject );

    IParticleManager::~IParticleManager() = default;

}  // namespace workphone::render
