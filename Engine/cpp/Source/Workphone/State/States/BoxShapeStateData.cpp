#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/BoxShapeStateData.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, BoxShapeStateData, StateData );

    BoxShapeStateData::BoxShapeStateData() : StateData( BoxShapeStateData::typeInfo() )
    {
    }

    BoxShapeStateData::~BoxShapeStateData() = default;

}  // namespace workphone
