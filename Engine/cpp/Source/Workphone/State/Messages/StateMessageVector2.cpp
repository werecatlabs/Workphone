#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageVector2.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    //WP_CLASS_REGISTER_DERIVED_TEMPLATE( fb, StateMessageVector2, f32,  StateMessage);
    //WP_CLASS_REGISTER_DERIVED_TEMPLATE( fb, StateMessageVector2, f64, StateMessage );

    // explicit instantiation
    template class StateMessageVector2<s32>;
    template class StateMessageVector2<f32>;
    template class StateMessageVector2<f64>;
}  // namespace workphone
