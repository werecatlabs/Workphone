#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/MaterialStateData.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, MaterialStateData, StateData );

    MaterialStateData::MaterialStateData() : StateData( MaterialStateData::typeInfo() )
    {
    }

    MaterialStateData::~MaterialStateData() = default;

}  // namespace workphone
