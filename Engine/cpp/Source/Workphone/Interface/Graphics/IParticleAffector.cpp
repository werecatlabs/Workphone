#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IParticleAffector.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IParticleAffector, IParticleNode );

    IParticleAffector::~IParticleAffector() = default;

}  // namespace workphone::render
