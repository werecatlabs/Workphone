#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIAnimationGraph.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIAnimationGraph, IUIElement );

    IUIAnimationGraph::~IUIAnimationGraph() = default;

    IUIAnimationGraph::IUIAnimationGraph( u32 poolTypeId )
    {
    }

    IUIAnimationGraph::IUIAnimationGraph()
    {
    }

}  // namespace workphone::ui
