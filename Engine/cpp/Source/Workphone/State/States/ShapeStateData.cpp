#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/ShapeStateData.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ShapeStateData, StateData );

    ShapeStateData::ShapeStateData() : StateData( ShapeStateData::typeInfo() )
    {
    }

    ShapeStateData::~ShapeStateData() = default;

}  // namespace workphone
