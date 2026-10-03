#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Procedural/IRoadNode.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::procedural
{
    WP_CLASS_REGISTER_DERIVED( workphone, IRoadNode, IProceduralNode );

    IRoadNode::~IRoadNode() = default;
}  // namespace workphone::procedural
