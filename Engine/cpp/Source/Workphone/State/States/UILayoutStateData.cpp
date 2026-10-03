#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UILayoutStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UILayoutStateData, StateData );

    const u32 UILayoutStateData::dirtyFlag = 1 << 1;

    UILayoutStateData::UILayoutStateData() : StateData( UILayoutStateData::typeInfo() )
    {
    }

    UILayoutStateData::~UILayoutStateData() = default;

}  // namespace workphone
