#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/LightStateData.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, LightStateData, StateData );

    LightStateData::LightStateData() : StateData( LightStateData::typeInfo() )
    {
    }

    LightStateData::~LightStateData() = default;

}  // namespace workphone
