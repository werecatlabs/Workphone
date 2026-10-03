#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/UI/IUIPropertyGrid.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, IUIPropertyGrid, IUIElement );

    IUIPropertyGrid::~IUIPropertyGrid() = default;

}  // namespace workphone::ui
