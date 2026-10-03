#include <WPOISInput/WPOISKeyConverter.hpp>
#include <Workphone/Workphone.hpp>

template <>
workphone::OISKeyConverter *workphone::Singleton<workphone::OISKeyConverter>::m_singleton = nullptr;

namespace workphone
{
    OISKeyConverter::OISKeyConverter()
    {
        keys["UNASSIGNED"] = OIS::KC_UNASSIGNED;
        keys["ESCAPE"] = OIS::KC_ESCAPE;
        keys["1"] = OIS::KC_1;
        keys["2"] = OIS::KC_2;
        keys["3"] = OIS::KC_3;
        keys["4"] = OIS::KC_4;
        keys["5"] = OIS::KC_5;
        keys["6"] = OIS::KC_6;
        keys["7"] = OIS::KC_7;
        keys["8"] = OIS::KC_8;
        keys["9"] = OIS::KC_9;
        keys["0"] = OIS::KC_0;
        keys["MINUS"] = OIS::KC_MINUS;
        keys["EQUALS"] = OIS::KC_EQUALS;
        keys["BACK"] = OIS::KC_BACK;
        keys["TAB"] = OIS::KC_TAB;
        keys["Q"] = OIS::KC_Q;
        keys["W"] = OIS::KC_W;
        keys["E"] = OIS::KC_E;
        keys["R"] = OIS::KC_R;
        keys["T"] = OIS::KC_T;
        keys["Y"] = OIS::KC_Y;
        keys["U"] = OIS::KC_U;
        keys["I"] = OIS::KC_I;
        keys["O"] = OIS::KC_O;
        keys["P"] = OIS::KC_P;
        keys["LBRACKET"] = OIS::KC_LBRACKET;
        keys["RBRACKET"] = OIS::KC_RBRACKET;
        keys["RETURN"] = OIS::KC_RETURN;
        keys["LCONTROL"] = OIS::KC_LCONTROL;
        keys["A"] = OIS::KC_A;
        keys["S"] = OIS::KC_S;
        keys["D"] = OIS::KC_D;
        keys["F"] = OIS::KC_F;
        keys["G"] = OIS::KC_G;
        keys["H"] = OIS::KC_H;
        keys["J"] = OIS::KC_J;
        keys["K"] = OIS::KC_K;
        keys["L"] = OIS::KC_L;
        keys["SEMICOLON"] = OIS::KC_SEMICOLON;
        keys["APOSTROPHE"] = OIS::KC_APOSTROPHE;
        keys["GRAVE"] = OIS::KC_GRAVE;
        keys["LSHIFT"] = OIS::KC_LSHIFT;
        keys["BACKSLASH"] = OIS::KC_BACKSLASH;
        keys["Z"] = OIS::KC_Z;
        keys["X"] = OIS::KC_X;
        keys["C"] = OIS::KC_C;
        keys["V"] = OIS::KC_V;
        keys["B"] = OIS::KC_B;
        keys["N"] = OIS::KC_N;
        keys["M"] = OIS::KC_M;
        keys["COMMA"] = OIS::KC_COMMA;
        keys["PERIOD"] = OIS::KC_PERIOD;
        keys["SLASH"] = OIS::KC_SLASH;
        keys["RSHIFT"] = OIS::KC_RSHIFT;
        keys["MULTIPLY"] = OIS::KC_MULTIPLY;
        keys["LMENU"] = OIS::KC_LMENU;
        keys["SPACE"] = OIS::KC_SPACE;
        keys["CAPITAL"] = OIS::KC_CAPITAL;
        keys["F1"] = OIS::KC_F1;
        keys["F2"] = OIS::KC_F2;
        keys["F3"] = OIS::KC_F3;
        keys["F4"] = OIS::KC_F4;
        keys["F5"] = OIS::KC_F5;
        keys["F6"] = OIS::KC_F6;
        keys["F7"] = OIS::KC_F7;
        keys["F8"] = OIS::KC_F8;
        keys["F9"] = OIS::KC_F9;
        keys["F10"] = OIS::KC_F10;
        keys["NUMLOCK"] = OIS::KC_NUMLOCK;
        keys["SCROLL"] = OIS::KC_SCROLL;
        keys["NUMPAD7"] = OIS::KC_NUMPAD7;
        keys["NUMPAD8"] = OIS::KC_NUMPAD8;
        keys["NUMPAD9"] = OIS::KC_NUMPAD9;
        keys["SUBTRACT"] = OIS::KC_SUBTRACT;
        keys["NUMPAD4"] = OIS::KC_NUMPAD4;
        keys["NUMPAD5"] = OIS::KC_NUMPAD5;
        keys["NUMPAD6"] = OIS::KC_NUMPAD6;
        keys["ADD"] = OIS::KC_ADD;
        keys["NUMPAD1"] = OIS::KC_NUMPAD1;
        keys["NUMPAD2"] = OIS::KC_NUMPAD2;
        keys["NUMPAD3"] = OIS::KC_NUMPAD3;
        keys["NUMPAD0"] = OIS::KC_NUMPAD0;
        keys["DECIMAL"] = OIS::KC_DECIMAL;
        keys["RCONTROL"] = OIS::KC_RCONTROL;
        keys["DIVIDE"] = OIS::KC_DIVIDE;
        keys["SYSRQ"] = OIS::KC_SYSRQ;
        keys["RMENU"] = OIS::KC_RMENU;
        keys["PAUSE"] = OIS::KC_PAUSE;
        keys["HOME"] = OIS::KC_HOME;
        keys["UP"] = OIS::KC_UP;
        keys["PGUP"] = OIS::KC_PGUP;
        keys["LEFT"] = OIS::KC_LEFT;
        keys["RIGHT"] = OIS::KC_RIGHT;
        keys["END"] = OIS::KC_END;
        keys["DOWN"] = OIS::KC_DOWN;
        keys["PGDOWN"] = OIS::KC_PGDOWN;
        keys["INSERT"] = OIS::KC_INSERT;
        keys["DELETE"] = OIS::KC_DELETE;
        keys["LWIN"] = OIS::KC_LWIN;
        keys["RWIN"] = OIS::KC_RWIN;
        keys["APPS"] = OIS::KC_APPS;
    }

    OISKeyConverter::~OISKeyConverter() = default;

    auto OISKeyConverter::getCodeFromString( const String &key ) const -> OIS::KeyCode
    {
        auto it = keys.find( key );
        if( it != keys.end() )
        {
            return it->second;
        }

        return OIS::KC_UNASSIGNED;
    }
}  // namespace workphone
