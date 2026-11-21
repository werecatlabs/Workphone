#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUINodeGraph.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUINodeGraph, IUIElement );

    IUINodeGraph::~IUINodeGraph()
    {
    }

    IUINodeGraph::IUINodeGraph( u32 poolTypeId )
    {
    }

    IUINodeGraph::IUINodeGraph()
    {
    }

}  // namespace workphone::ui
