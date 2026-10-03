#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IParticleNode.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IParticleNode, ISharedObject );

    IParticleNode::~IParticleNode() = default;

}  // namespace workphone::render
