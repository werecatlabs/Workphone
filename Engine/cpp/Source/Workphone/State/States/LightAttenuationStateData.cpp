#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/LightAttenuationStateData.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, LightAttenuationStateData, StateData );

    LightAttenuationStateData::LightAttenuationStateData() :
        StateData( LightAttenuationStateData::typeInfo() )
    {
    }

    LightAttenuationStateData::~LightAttenuationStateData()
    {
    }

}  // namespace workphone
