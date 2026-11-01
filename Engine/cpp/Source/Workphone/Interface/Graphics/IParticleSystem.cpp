#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IParticleSystem.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IParticleSystem, ISharedObject );

    IParticleSystem::~IParticleSystem() = default;
    bool IParticleSystem::loadRuntimeResource(std::shared_ptr<const resource::RuntimeResource>,
        std::shared_ptr<resource::ResourcePayloadMount>,const String &,bool,String &error)
    {error="This graphics backend does not support captured particle resources";return false;}

}  // namespace workphone::render
