#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/ApplicationStateData.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ApplicationStateData, StateData );

    ApplicationStateData::ApplicationStateData() : StateData( ApplicationStateData::typeInfo() )
    {
    }

    ApplicationStateData::~ApplicationStateData() = default;

}  // namespace workphone
