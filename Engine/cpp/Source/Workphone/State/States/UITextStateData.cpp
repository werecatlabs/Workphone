#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UITextStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UITextStateData, StateData );

    UITextStateData::UITextStateData() : StateData( UITextStateData::typeInfo() )
    {
    }

    UITextStateData::~UITextStateData() = default;

}  // namespace workphone
