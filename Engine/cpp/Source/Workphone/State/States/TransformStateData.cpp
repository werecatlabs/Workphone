#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/TransformStateData.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TransformStateData, StateData );

    TransformStateData::TransformStateData() : StateData( TransformStateData::typeInfo() )
    {
    }

    TransformStateData::~TransformStateData()
    {
    }

}  // namespace workphone
