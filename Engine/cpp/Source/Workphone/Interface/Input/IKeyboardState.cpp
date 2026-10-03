#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IKeyboardState.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IKeyboardState, ISharedObject );

    IKeyboardState::~IKeyboardState() = default;

}  // namespace workphone
