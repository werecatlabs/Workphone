#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UIProgressBarStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UIProgressBarStateData, StateData );

    UIProgressBarStateData::UIProgressBarStateData() : StateData( UIProgressBarStateData::typeInfo() )
    {
    }

    UIProgressBarStateData::~UIProgressBarStateData() = default;

}  // namespace workphone
