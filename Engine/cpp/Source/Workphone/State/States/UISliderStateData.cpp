#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UISliderStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UISliderStateData, StateData );

    UISliderStateData::UISliderStateData() : StateData( UISliderStateData::typeInfo() )
    {
    }

    UISliderStateData::~UISliderStateData() = default;

}  // namespace workphone
