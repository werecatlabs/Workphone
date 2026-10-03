#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/ConstraintD6StateData.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, ConstraintD6StateData, StateData );

    ConstraintD6StateData::ConstraintD6StateData() : StateData( ConstraintD6StateData::typeInfo() )
    {
    }

    ConstraintD6StateData::~ConstraintD6StateData() = default;

}  // namespace workphone
