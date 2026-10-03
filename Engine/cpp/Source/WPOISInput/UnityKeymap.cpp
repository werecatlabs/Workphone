#include <WPOISInput/WPOISKeyConverter.hpp>
#include <WPOISInput/UnityKeymap.hpp>
#include <Workphone/Workphone.hpp>
#include <OIS.h>

using namespace workphone;

//#define unityKey(a, b) case OIS::KC_##a: return KeyCode::##b;

#define unityKey( a, b ) \
    case KeyCodes::KEY_##b: \
        return OIS::KC_##a;

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, UnityKeymap, ExternalKeymap );

    UnityKeymap::UnityKeymap()
    {
    }

    UnityKeymap::~UnityKeymap()
    {
    }

    s32 UnityKeymap::getKeycode( s32 keycode ) const
    {
        auto eKeycode = static_cast<KeyCodes>( keycode );
        OIS::KeyCode oisKeycode = OIS::KC_UNASSIGNED;

        switch( eKeycode )
        {
        case KeyCodes::KEY_BACK:
            oisKeycode = OIS::KC_BACK;
            break;
        default:
            oisKeycode = static_cast<OIS::KeyCode>( keycode );
        }

        return oisKeycode;
    }

    s32 UnityKeymap::getExternalKeycode( s32 keycode ) const
    {
        //switch (keycode)
        //{
        //	unityKey(BACK, BACKSPACE)
        //		unityKey(TAB, TAB)
        //		//unityKey(CLEAR, CLEAR)
        //		unityKey(RETURN, RETURN)
        //		unityKey(PAUSE, PAUSE)
        //		unityKey(ESCAPE, ESCAPE)
        //		unityKey(SPACE, SPACE)
        //		//unityKey(EXCLAIM, 1)
        //		//unityKey(QUOTEDBL, 2)
        //		//unityKey(HASH, 3)
        //		//unityKey(DOLLAR, DOLLAR)
        //		//unityKey(AMPERSAND, AMPERSAND)
        //		//unityKey(QUOTE, QUOTE)
        //		//unityKey(LBRACKET, 9)
        //		//unityKey(RBRACKET, 0)
        //		//unityKey(MULTIPLY, 8)
        //		//unityKey(ADD, OEM_PLUS)
        //		//unityKey(COMMA, OEM_COMMA)
        //		//unityKey(MINUS, OEM_MINUS)
        //		//unityKey(PERIOD, OEM_PERIOD)
        //		//unityKey(SLASH, OEM_2)
        //		unityKey(0, ALPHA0)
        //		unityKey(1, ALPHA1)
        //		unityKey(2, ALPHA2)
        //		unityKey(3, ALPHA3)
        //		unityKey(4, ALPHA4)
        //		unityKey(5, ALPHA5)
        //		unityKey(6, ALPHA6)
        //		unityKey(7, ALPHA7)
        //		unityKey(8, ALPHA8)
        //		unityKey(9, ALPHA9)
        //		unityKey(COLON, COLON)
        //		unityKey(SEMICOLON, SEMICOLON)
        //		//unityKey(LESS, LESS)
        //		unityKey(EQUALS, EQUALS)
        //		//unityKey(GREATER, GREATER)
        //		//unityKey(QUESTION, QUESTION)
        //		unityKey(AT, AT)
        //		//unityKey(LEFTBRACKET, OEM_4)
        //		//unityKey(BACKSLASH, OEM_5)
        //		//unityKey(RIGHTBRACKET, OEM_6)
        //		//unityKey(CARET, 6)
        //		//unityKey(UNDERLINE, OEM_MINUS)
        //		//unityKey(BACKQUOTE, OEM_3)
        //		unityKey(A, A)
        //		unityKey(B, B)
        //		unityKey(C, C)
        //		unityKey(D, D)
        //		unityKey(E, E)
        //		unityKey(F, F)
        //		unityKey(G, G)
        //		unityKey(H, H)
        //		unityKey(I, I)
        //		unityKey(J, J)
        //		unityKey(K, K)
        //		unityKey(L, L)
        //		unityKey(M, M)
        //		unityKey(N, N)
        //		unityKey(O, O)
        //		unityKey(P, P)
        //		unityKey(Q, Q)
        //		unityKey(R, R)
        //		unityKey(S, S)
        //		unityKey(T, T)
        //		unityKey(U, U)
        //		unityKey(V, V)
        //		unityKey(W, W)
        //		unityKey(X, X)
        //		unityKey(Y, Y)
        //		unityKey(Z, Z)
        //		//unityKey(DELETE, DELETE)
        //		unityKey(NUMPAD0, KEYPAD0)
        //		unityKey(NUMPAD1, KEYPAD1)
        //		unityKey(NUMPAD2, KEYPAD2)
        //		unityKey(NUMPAD3, KEYPAD3)
        //		unityKey(NUMPAD4, KEYPAD4)
        //		unityKey(NUMPAD5, KEYPAD5)
        //		unityKey(NUMPAD6, KEYPAD6)
        //		unityKey(NUMPAD7, KEYPAD7)
        //		unityKey(NUMPAD8, KEYPAD8)
        //		unityKey(NUMPAD9, KEYPAD9)
        //		//unityKey(DECIMAL, DECIMAL)
        //		//unityKey(DIVIDE, DIVIDE)
        //		//unityKey(MULTIPLY, MULTIPLY)
        //		//unityKey(SUBTRACT, SUBTRACT)
        //		//unityKey(ADD, ADD)
        //		//unityKey(NUMPADENTER, SEPARATOR)
        //		//unityKey(NUMPADEQUALS, UNKNOWN)
        //		unityKey(UP, UPARROW)
        //		unityKey(DOWN, DOWNARROW)
        //		unityKey(RIGHT, RIGHTARROW)
        //		unityKey(LEFT, LEFTARROW)
        //		unityKey(INSERT, INSERT)
        //		unityKey(HOME, HOME)
        //		unityKey(END, END)
        //		unityKey(PGUP, PAGEUP)
        //		unityKey(PGDOWN, PAGEDOWN)
        //		unityKey(F1, F1)
        //		unityKey(F2, F2)
        //		unityKey(F3, F3)
        //		unityKey(F4, F4)
        //		unityKey(F5, F5)
        //		unityKey(F6, F6)
        //		unityKey(F7, F7)
        //		unityKey(F8, F8)
        //		unityKey(F9, F9)
        //		unityKey(F10, F10)
        //		unityKey(F11, F11)
        //		unityKey(F12, F12)
        //		unityKey(F13, F13)
        //		unityKey(F14, F14)
        //		unityKey(F15, F15)
        //		unityKey(NUMLOCK, NUMLOCK)
        //		//unityKey(CAPITAL, CAPITAL)
        //		//unityKey(SCROLL, SCROLL)
        //		//unityKey(RSHIFT, RSHIFT)
        //		//unityKey(LSHIFT, LSHIFT)
        //		//unityKey(RCONTROL, RCONTROL)
        //		//unityKey(LCONTROL, LCONTROL)
        //		//unityKey(RMENU, RMENU)
        //		//unityKey(LMENU, LMENU)
        //		//unityKey(LWIN, LWIN)
        //		//unityKey(RWIN, RWIN)
        //		//unityKey(LSUPER, LWIN)
        //		//unityKey(RSUPER, RWIN)
        //		//unityKey(MODE, MODECHANGE)
        //		//unityKey(COMPOSE, ACCEPT)
        //		//unityKey(HELP, HELP)
        //		//unityKey(PRINT, SNAPSHOT)
        //		//unityKey(SYSRQ, EXECUTE)
        //	default:
        //		return KeyCode::NONE;
        //}

        return 0;
    }
}  // namespace workphone
