#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IProceduralNode.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IProceduralNode, IProceduralObject );

    IProceduralNode::~IProceduralNode() = default;

}  // namespace workphone::procedural
