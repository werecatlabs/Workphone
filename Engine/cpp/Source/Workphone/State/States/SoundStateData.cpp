#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/SoundStateData.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, SoundStateData, StateData );

    SoundStateData::SoundStateData() : StateData( SoundStateData::typeInfo() )
    {
    }

    SoundStateData::~SoundStateData() = default;

}  // namespace workphone
