#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IParticle.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IParticle, ISharedObject );

    IParticle::~IParticle() = default;

}  // namespace workphone::render
