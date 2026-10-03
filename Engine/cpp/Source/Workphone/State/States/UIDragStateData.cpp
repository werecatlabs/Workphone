#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/UIDragStateData.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, UIDragStateData, StateData );

    UIDragStateData::UIDragStateData() : StateData( UIDragStateData::typeInfo() )
    {
    }

    UIDragStateData::~UIDragStateData() = default;

}  // namespace workphone
