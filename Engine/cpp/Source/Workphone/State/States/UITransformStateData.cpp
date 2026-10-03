#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UITransformStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, UITransformStateData, StateData );

    UITransformStateData::UITransformStateData() : StateData( UITransformStateData::typeInfo() )
    {
    }

    UITransformStateData::~UITransformStateData() = default;

}  // namespace workphone
