#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/Test/Tests/TestInput.hpp"
#include <Workphone/Input/InputActionData.hpp>
#include <Workphone/Input/KeyboardState.hpp>
#include <Workphone/Input/MouseState.hpp>
#include <cassert>

namespace workphone
{
    void TestInput::report()
    {
    }

    void TestInput::run()
    {
        KeyboardState keyboardState;
        assert( keyboardState.getChar() == 0 );
        assert( keyboardState.getKeyCode() == 0 );
        assert( keyboardState.getRawKeyCode() == 0 );
        assert( !keyboardState.isPressedDown() );
        assert( !keyboardState.isShiftPressed() );
        assert( !keyboardState.isControlPressed() );

        keyboardState.setChar( 'A' );
        keyboardState.setKeyCode( 65 );
        keyboardState.setRawKeyCode( 30 );
        keyboardState.setPressedDown( true );
        keyboardState.setShiftPressed( true );
        keyboardState.setControlPressed( true );

        assert( keyboardState.getChar() == 'A' );
        assert( keyboardState.getKeyCode() == 65 );
        assert( keyboardState.getRawKeyCode() == 30 );
        assert( keyboardState.isPressedDown() );
        assert( keyboardState.isShiftPressed() );
        assert( keyboardState.isControlPressed() );

        keyboardState.setPressedDown( false );
        keyboardState.setShiftPressed( false );
        keyboardState.setControlPressed( false );
        assert( !keyboardState.isPressedDown() );
        assert( !keyboardState.isShiftPressed() );
        assert( !keyboardState.isControlPressed() );

        MouseState mouseState;
        assert( mouseState.getEventType() == IMouseState::Event::Count );
        assert( !mouseState.isButtonPressed( IMouseState::MOUSE_LEFT ) );
        assert( !mouseState.isDragging() );
        assert( !mouseState.isDoubleClick() );

        mouseState.setDelta( Vector2<real_Num>( 3, -2 ) );
        mouseState.setRelativePosition( Vector2<real_Num>( 0.25, 0.75 ) );
        mouseState.setAbsolutePosition( Vector2<real_Num>( 640, 480 ) );
        mouseState.setWheelDelta( Vector2<real_Num>( 0, 1 ) );
        mouseState.setButtonPressed( IMouseState::MOUSE_LEFT, true );
        mouseState.setShiftPressed( true );
        mouseState.setControlPressed( true );
        mouseState.setDragging( true );
        mouseState.setDoubleClick( true );
        mouseState.setEventType( IMouseState::Event::LeftPressed );

        assert( mouseState.getDelta() == Vector2<real_Num>( 3, -2 ) );
        assert( mouseState.getRelativePosition() == Vector2<real_Num>( 0.25, 0.75 ) );
        assert( mouseState.getAbsolutePosition() == Vector2<real_Num>( 640, 480 ) );
        assert( mouseState.getWheelDelta() == Vector2<real_Num>( 0, 1 ) );
        assert( mouseState.isButtonPressed( IMouseState::MOUSE_LEFT ) );
        assert( mouseState.isShiftPressed() );
        assert( mouseState.isControlPressed() );
        assert( mouseState.isDragging() );
        assert( mouseState.isDoubleClick() );
        assert( mouseState.getEventType() == IMouseState::Event::LeftPressed );

        InputActionData actionData( 10, 20, 30 );
        assert( actionData.getPrimaryAction() == 10 );
        assert( actionData.getSecondaryAction() == 20 );
        assert( actionData.getActionId() == 30 );

        actionData.setPrimaryAction( 100 );
        actionData.setSecondaryAction( 200 );
        actionData.setActionId( 300 );
        assert( actionData.getPrimaryAction() == 100 );
        assert( actionData.getSecondaryAction() == 200 );
        assert( actionData.getActionId() == 300 );
    }

    TestInput::~TestInput()
    {
    }

    TestInput::TestInput()
    {
    }

}  // namespace workphone
