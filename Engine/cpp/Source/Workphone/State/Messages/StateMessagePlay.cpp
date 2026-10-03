#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessagePlay.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessagePlay, StateMessage );

    //---------------------------------------------
    StateMessagePlay::StateMessagePlay() = default;

    //---------------------------------------------
    StateMessagePlay::~StateMessagePlay() = default;
}  // namespace workphone
