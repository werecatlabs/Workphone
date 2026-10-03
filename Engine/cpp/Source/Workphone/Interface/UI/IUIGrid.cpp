#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIGrid.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIGrid, IUIElement );

    IUIGrid::~IUIGrid() = default;

}  // namespace workphone::ui
