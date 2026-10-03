#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/BoundingBoxStateData.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, BoundingBoxStateData, StateData );

    BoundingBoxStateData::BoundingBoxStateData() : StateData( BoundingBoxStateData::typeInfo() )
    {
    }

    BoundingBoxStateData::~BoundingBoxStateData() = default;

}  // namespace workphone
