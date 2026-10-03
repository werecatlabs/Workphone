#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/VehicleStateData.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, VehicleStateData, StateData );

    VehicleStateData::VehicleStateData() : StateData( VehicleStateData::typeInfo() )
    {
    }

    VehicleStateData::~VehicleStateData() = default;

}  // namespace workphone
