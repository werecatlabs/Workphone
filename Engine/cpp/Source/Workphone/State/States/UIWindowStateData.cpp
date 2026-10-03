#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UIWindowStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UIWindowStateData, StateData );

    UIWindowStateData::UIWindowStateData() : StateData( UIWindowStateData::typeInfo() )
    {
    }

    UIWindowStateData::~UIWindowStateData() = default;

}  // namespace workphone
