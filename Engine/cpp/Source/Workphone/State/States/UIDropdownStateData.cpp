#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UIDropdownStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UIDropdownStateData, StateData );

    UIDropdownStateData::UIDropdownStateData() : StateData( UIDropdownStateData::typeInfo() )
    {
    }

    UIDropdownStateData::~UIDropdownStateData() = default;

}  // namespace workphone
