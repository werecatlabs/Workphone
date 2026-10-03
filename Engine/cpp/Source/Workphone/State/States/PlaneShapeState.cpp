#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/PlaneShapeState.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, PlaneShapeState, StateData );

    PlaneShapeState::PlaneShapeState() = default;

    PlaneShapeState::~PlaneShapeState() = default;

}  // namespace workphone
