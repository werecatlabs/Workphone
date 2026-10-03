#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IParticleTechnique.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IParticleTechnique, ISharedObject );

    IParticleTechnique::~IParticleTechnique() = default;

}  // namespace workphone::render
