#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Input/IMouseState.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IMouseState, ISharedObject );

    const u32 IMouseState::MOUSE_LEFT = ( 1 << 0 );
    const u32 IMouseState::MOUSE_RIGHT = ( 1 << 1 );
    const u32 IMouseState::MOUSE_MIDDLE = ( 1 << 2 );
    const u32 IMouseState::MOUSE_LEFT_RELEASED = ( 1 << 3 );
    const u32 IMouseState::MOUSE_RIGHT_RELEASED = ( 1 << 4 );
    const u32 IMouseState::MOUSE_MIDDLE_RELEASED = ( 1 << 5 );
    const u32 IMouseState::MOUSE_SHIFT = ( 1 << 6 );
    const u32 IMouseState::MOUSE_CONTROL = ( 1 << 7 );
    const u32 IMouseState::MOUSE_DRAGGING = ( 1 << 8 );
    const u32 IMouseState::MOUSE_DOUBLE_CLICK = ( 1 << 9 );

    IMouseState::~IMouseState() = default;

}  // namespace workphone
