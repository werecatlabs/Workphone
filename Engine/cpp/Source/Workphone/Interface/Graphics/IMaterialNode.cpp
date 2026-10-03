#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Graphics/IMaterialNode.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, IMaterialNode, ISharedObject );

    IMaterialNode::IMaterialNode() : ISharedObject( IMaterialNode::typeInfo() )
    {
    }

    IMaterialNode::~IMaterialNode() = default;

}  // namespace workphone::render
