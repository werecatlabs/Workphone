#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUITreeNode.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUITreeNode, IUIElement );

    IUITreeNode::~IUITreeNode() = default;
}  // namespace workphone::ui
