#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/AmbientLightStateData.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, AmbientLightStateData, StateData );

    AmbientLightStateData::AmbientLightStateData() : StateData( AmbientLightStateData::typeInfo() )
    {
    }

    AmbientLightStateData::~AmbientLightStateData() = default;

}  // namespace workphone
