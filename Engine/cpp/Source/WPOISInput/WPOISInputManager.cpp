#include <WPOISInput/WPOISInputManager.hpp>
#include <WPOISInput/WPJoyStickData.hpp>
#include <WPOISInput/WPOISKeyConverter.hpp>
#include <WPOISInput/WPOISJoystick.hpp>
#include <Workphone/Workphone.hpp>
#include <OIS.h>
#include <sstream>

#if defined WP_PLATFORM_WIN32
#    include <WPOISInput/Platform/win32/PlatformInputManager.hpp>
#elif defined WP_PLATFORM_APPLE
#    include <WPOISInput/Platform/macOS/PlatformInputManager.hpp>
#endif

enum
{
    MAX_JOYS = 8
};

namespace workphone
{
    // OIS to Irrlicht key map. Table position is the OIS keycode. KEY_PA1
    // is used when Irrlicht doesn't contain a corresponding OIS key code.
    static KeyCodes oiKeyMap[238] = {
        //  Irrlicht Key       OIS Key as Index
        KeyCodes::KEY_PA1,                     // KC_UNASSIGNED  = 0x00,
        KeyCodes::KEY_ESCAPE,                  // KC_ESCAPE      = 0x01,
        KeyCodes::KEY_KEY_1,                   // KC_1           = 0x02,
        KeyCodes::KEY_KEY_2,                   // KC_2           = 0x03,
        KeyCodes::KEY_KEY_3,                   // KC_3           = 0x04,
        KeyCodes::KEY_KEY_4,                   // KC_4           = 0x05,
        KeyCodes::KEY_KEY_5,                   // KC_5           = 0x06,
        KeyCodes::KEY_KEY_6,                   // KC_6           = 0x07,
        KeyCodes::KEY_KEY_7,                   // KC_7           = 0x08,
        KeyCodes::KEY_KEY_8,                   // KC_8           = 0x09,
        KeyCodes::KEY_KEY_9,                   // KC_9           = 0x0A,
        KeyCodes::KEY_KEY_0,                   // KC_0           = 0x0B,
        KeyCodes::KEY_MINUS,                   // KC_MINUS       = 0x0C,    // - on main keyboard
        KeyCodes::KEY_PLUS,                    // KC_EQUALS      = 0x0D,
        KeyCodes::KEY_BACK,                    // KC_BACK        = 0x0E,    // backspace
        KeyCodes::KEY_TAB,                     // KC_TAB         = 0x0F,
        KeyCodes::KEY_KEY_Q,                   // KC_Q           = 0x10,
        KeyCodes::KEY_KEY_W,                   // KC_W           = 0x11,
        KeyCodes::KEY_KEY_E,                   // KC_E           = 0x12,
        KeyCodes::KEY_KEY_R,                   // KC_R           = 0x13,
        KeyCodes::KEY_KEY_T,                   // KC_T           = 0x14,
        KeyCodes::KEY_KEY_Y,                   // KC_Y           = 0x15,
        KeyCodes::KEY_KEY_U,                   // KC_U           = 0x16,
        KeyCodes::KEY_KEY_I,                   // KC_I           = 0x17,
        KeyCodes::KEY_KEY_O,                   // KC_O           = 0x18,
        KeyCodes::KEY_KEY_P,                   // KC_P           = 0x19,
        KeyCodes::KEY_PA1,                     // KC_LBRACKET    = 0x1A,
        KeyCodes::KEY_PA1,                     // KC_RBRACKET    = 0x1B,
        KeyCodes::KEY_RETURN,                  // KC_RETURN      = 0x1C,    // Enter on main keyboard
        KeyCodes::KEY_LCONTROL,                // KC_LCONTROL    = 0x1D,
        KeyCodes::KEY_KEY_A,                   // KC_A           = 0x1E,
        KeyCodes::KEY_KEY_S,                   // KC_S           = 0x1F,
        KeyCodes::KEY_KEY_D,                   // KC_D           = 0x20,
        KeyCodes::KEY_KEY_F,                   // KC_F           = 0x21,
        KeyCodes::KEY_KEY_G,                   // KC_G           = 0x22,
        KeyCodes::KEY_KEY_H,                   // KC_H           = 0x23,
        KeyCodes::KEY_KEY_J,                   // KC_J           = 0x24,
        KeyCodes::KEY_KEY_K,                   // KC_K           = 0x25,
        KeyCodes::KEY_KEY_L,                   // KC_L           = 0x26,
        KeyCodes::KEY_PA1,                     // KC_SEMICOLON   = 0x27,
        KeyCodes::KEY_PA1,                     // KC_APOSTROPHE  = 0x28,
        KeyCodes::KEY_PA1,                     // KC_GRAVE       = 0x29,    // accent
        KeyCodes::KEY_LSHIFT,                  // KC_LSHIFT      = 0x2A,
        KeyCodes::KEY_PA1,                     // KC_BACKSLASH   = 0x2B,
        KeyCodes::KEY_KEY_Z,                   // KC_Z           = 0x2C,
        KeyCodes::KEY_KEY_X,                   // KC_X           = 0x2D,
        KeyCodes::KEY_KEY_C,                   // KC_C           = 0x2E,
        KeyCodes::KEY_KEY_V,                   // KC_V           = 0x2F,
        KeyCodes::KEY_KEY_B,                   // KC_B           = 0x30,
        KeyCodes::KEY_KEY_N,                   // KC_N           = 0x31,
        KeyCodes::KEY_KEY_M,                   // KC_M           = 0x32,
        KeyCodes::KEY_COMMA,                   // KC_COMMA       = 0x33,
        KeyCodes::KEY_PERIOD,                  // KC_PERIOD      = 0x34,    // . on main keyboard
        KeyCodes::KEY_PA1,                     // KC_SLASH       = 0x35,    // / on main keyboard
        KeyCodes::KEY_RSHIFT,                  // KC_RSHIFT      = 0x36,
        KeyCodes::KEY_MULTIPLY,                // KC_MULTIPLY    = 0x37,    // * on numeric keypad
        KeyCodes::KEY_LALT,                    // KC_LMENU       = 0x38,    // left Alt
        KeyCodes::KEY_SPACE,                   // KC_SPACE       = 0x39,
        KeyCodes::KEY_CAPITAL,                 // KC_CAPITAL     = 0x3A,
        KeyCodes::KEY_F1,                      // KC_F1          = 0x3B,
        KeyCodes::KEY_F2,                      // KC_F2          = 0x3C,
        KeyCodes::KEY_F3,                      // KC_F3          = 0x3D,
        KeyCodes::KEY_F4,                      // KC_F4          = 0x3E,
        KeyCodes::KEY_F5,                      // KC_F5          = 0x3F,
        KeyCodes::KEY_F6,                      // KC_F6          = 0x40,
        KeyCodes::KEY_F7,                      // KC_F7          = 0x41,
        KeyCodes::KEY_F8,                      // KC_F8          = 0x42,
        KeyCodes::KEY_F9,                      // KC_F9          = 0x43,
        KeyCodes::KEY_F10,                     // KC_F10         = 0x44,
        KeyCodes::KEY_NUMLOCK,                 // KC_NUMLOCK     = 0x45,
        KeyCodes::KEY_SCROLL,                  // KC_SCROLL      = 0x46,    // Scroll Lock
        KeyCodes::KEY_NUMPAD7,                 // KC_NUMPAD7     = 0x47,
        KeyCodes::KEY_NUMPAD8,                 // KC_NUMPAD8     = 0x48,
        KeyCodes::KEY_NUMPAD9,                 // KC_NUMPAD9     = 0x49,
        KeyCodes::KEY_SUBTRACT,                // KC_SUBTRACT    = 0x4A,    // - on numeric keypad
        KeyCodes::KEY_NUMPAD4,                 // KC_NUMPAD4     = 0x4B,
        KeyCodes::KEY_NUMPAD5,                 // KC_NUMPAD5     = 0x4C,
        KeyCodes::KEY_NUMPAD6,                 // KC_NUMPAD6     = 0x4D,
        KeyCodes::KEY_ADD,                     // KC_ADD         = 0x4E,    // + on numeric keypad
        KeyCodes::KEY_NUMPAD1,                 // KC_NUMPAD1     = 0x4F,
        KeyCodes::KEY_NUMPAD2,                 // KC_NUMPAD2     = 0x50,
        KeyCodes::KEY_NUMPAD3,                 // KC_NUMPAD3     = 0x51,
        KeyCodes::KEY_NUMPAD0,                 // KC_NUMPAD0     = 0x52,
        KeyCodes::KEY_DECIMAL,                 // KC_DECIMAL     = 0x53,    // . on numeric keypad
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,  // 0x54, 0x55,
        KeyCodes::KEY_PA1,  // KC_OEM_102     = 0x56,    // < > | on UK/Germany keyboards
        KeyCodes::KEY_F11,  // KC_F11         = 0x57,
        KeyCodes::KEY_F12,  // KC_F12         = 0x58,
        // 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1,
        KeyCodes::KEY_F13,  // KC_F13         = 0x64,    //                     (NEC PC98)
        KeyCodes::KEY_F14,  // KC_F14         = 0x65,    //                     (NEC PC98)
        KeyCodes::KEY_F15,  // KC_F15         = 0x66,    //                     (NEC PC98)
        // 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_KANA,  // KC_KANA        = 0x70,    // (Japanese keyboard)
        // 0x71, 0x72,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1,  // KC_ABNT_C1     = 0x73,    // / ? on Portugese (Brazilian) keyboards
        // 0x74, 0x75, 0x76, 0x77, 0x78,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_CONVERT,     // KC_CONVERT     = 0x79,    // (Japanese keyboard)
        KeyCodes::KEY_PA1,         // 0x7A,
        KeyCodes::KEY_NONCONVERT,  // KC_NOCONVERT   = 0x7B,    // (Japanese keyboard)
        KeyCodes::KEY_PA1,         // 0x7C,
        KeyCodes::KEY_PA1,         // KC_YEN         = 0x7D,    // (Japanese keyboard)
        KeyCodes::KEY_PA1,         // KC_ABNT_C2     = 0x7E,    // Numpad . on Portugese (Brazilian)
        // keyboards
        // 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        // 0x8A, 0x8B, 0x8C,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1,  // KC_NUMPADEQUALS= 0x8D,    // = on numeric keypad (NEC PC98)
        // 0x8E, 0x8F,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1,  // KC_PREVTRACK   = 0x90,    // Previous Track (KC_CIRCUMFLEX on
        // Japanese keyboard)
        KeyCodes::KEY_PA1,    // KC_AT          = 0x91,    //                     (NEC PC98)
        KeyCodes::KEY_PA1,    // KC_COLON       = 0x92,    //                     (NEC PC98)
        KeyCodes::KEY_PA1,    // KC_UNDERLINE   = 0x93,    //                     (NEC PC98)
        KeyCodes::KEY_KANJI,  // KC_KANJI       = 0x94,    // (Japanese keyboard)
        KeyCodes::KEY_PA1,    // KC_STOP        = 0x95,    //                     (NEC PC98)
        KeyCodes::KEY_PA1,    // KC_AX          = 0x96,    //                     (Japan AX)
        KeyCodes::KEY_PA1,    // KC_UNLABELED   = 0x97,    //                        (J3100)
        KeyCodes::KEY_PA1,    // 0x98,
        KeyCodes::KEY_PA1,    // KC_NEXTTRACK   = 0x99,    // Next Track
        // 0x9A, 0x9B,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1,       // KC_NUMPADENTER = 0x9C,    // Enter on numeric keypad
        KeyCodes::KEY_RCONTROL,  // KC_RCONTROL    = 0x9D,
        // 0x9E, 0x9F,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1,   // KC_MUTE        = 0xA0,    // Mute
        KeyCodes::KEY_PA1,   // KC_CALCULATOR  = 0xA1,    // Calculator
        KeyCodes::KEY_PLAY,  // KC_PLAYPAUSE   = 0xA2,    // Play / Pause
        KeyCodes::KEY_PA1,   // 0xA3,
        KeyCodes::KEY_PA1,   // KC_MEDIASTOP   = 0xA4,    // Media Stop
        // 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1,        // KC_VOLUMEDOWN  = 0xAE,    // Volume -
        KeyCodes::KEY_PA1,        // 0xAF,
        KeyCodes::KEY_PA1,        // KC_VOLUMEUP    = 0xB0,    // Volume +
        KeyCodes::KEY_PA1,        // 0xB1,
        KeyCodes::KEY_PA1,        // KC_WEBHOME     = 0xB2,    // Web home
        KeyCodes::KEY_SEPARATOR,  // KC_NUMPADCOMMA = 0xB3,    // , on numeric keypad (NEC PC98)
        KeyCodes::KEY_PA1,        // 0xB4,
        KeyCodes::KEY_DIVIDE,     // KC_DIVIDE      = 0xB5,    // / on numeric keypad
        KeyCodes::KEY_PA1,        // 0xB6,
        KeyCodes::KEY_SNAPSHOT,   // KC_SYSRQ       = 0xB7,
        KeyCodes::KEY_RMENU,      // KC_RMENU       = 0xB8,    // right Alt
        // 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        // 0xC2, 0xC3, 0xC4,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PAUSE,   // KC_PAUSE       = 0xC5,    // Pause
        KeyCodes::KEY_PA1,     // 0xC6,
        KeyCodes::KEY_HOME,    // KC_HOME        = 0xC7,    // Home on arrow keypad
        KeyCodes::KEY_UP,      // KC_UP          = 0xC8,    // UpArrow on arrow keypad
        KeyCodes::KEY_PRIOR,   // KC_PGUP        = 0xC9,    // PgUp on arrow keypad
        KeyCodes::KEY_PA1,     // 0xCA,
        KeyCodes::KEY_LEFT,    // KC_LEFT        = 0xCB,    // LeftArrow on arrow keypad
        KeyCodes::KEY_PA1,     // 0xCC,
        KeyCodes::KEY_RIGHT,   // KC_RIGHT       = 0xCD,    // RightArrow on arrow keypad
        KeyCodes::KEY_PA1,     // 0xCE,
        KeyCodes::KEY_END,     // KC_END         = 0xCF,    // End on arrow keypad
        KeyCodes::KEY_DOWN,    // KC_DOWN        = 0xD0,    // DownArrow on arrow keypad
        KeyCodes::KEY_NEXT,    // KC_PGDOWN      = 0xD1,    // PgDn on arrow keypad
        KeyCodes::KEY_INSERT,  // KC_INSERT      = 0xD2,    // Insert on arrow keypad
        KeyCodes::KEY_DELETE,  // KC_DELETE      = 0xD3,    // Delete on arrow keypad
        // 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_LWIN,   // KC_LWIN        = 0xDB,    // Left Windows key
        KeyCodes::KEY_RWIN,   // KC_RWIN        = 0xDC,    // Right Windows key
        KeyCodes::KEY_APPS,   // KC_APPS        = 0xDD,    // AppMenu key
        KeyCodes::KEY_PA1,    // KC_POWER       = 0xDE,    // System Power
        KeyCodes::KEY_SLEEP,  // KC_SLEEP       = 0xDF,    // System Sleep
        // 0xE0, 0xE1, 0xE2,
        KeyCodes::KEY_PA1, KeyCodes::KEY_PA1, KeyCodes::KEY_PA1,
        KeyCodes::KEY_PA1,  // KC_WAKE        = 0xE3,    // System Wake
        KeyCodes::KEY_PA1,  // 0xE4,
        KeyCodes::KEY_PA1,  // KC_WEBSEARCH   = 0xE5,    // Web Search
        KeyCodes::KEY_PA1,  // KC_WEBFAVORITES= 0xE6,    // Web Favorites
        KeyCodes::KEY_PA1,  // KC_WEBREFRESH  = 0xE7,    // Web Refresh
        KeyCodes::KEY_PA1,  // KC_WEBSTOP     = 0xE8,    // Web Stop
        KeyCodes::KEY_PA1,  // KC_WEBFORWARD  = 0xE9,    // Web Forward
        KeyCodes::KEY_PA1,  // KC_WEBBACK     = 0xEA,    // Web Back
        KeyCodes::KEY_PA1,  // KC_MYCOMPUTER  = 0xEB,    // My Computer
        KeyCodes::KEY_PA1,  // KC_MAIL        = 0xEC,    // Mail
        KeyCodes::KEY_PA1   // KC_MEDIASELECT = 0xED     // Media Select
    };

    static const u32 KC_RESERVED = 0;

    // OIS to Irrlicht key map. Table position is the OIS keycode. KEY_PA1
    // is used when Irrlicht doesn't contain a corresponding OIS key code.
    static u32 fbToOISKeyMap[238] = {
        // OIS Key as Index						//  Irrlicht Key
        OIS::KC_UNASSIGNED,        //				KEY_PA1,
        OIS::KC_ESCAPE,            //				KEY_ESCAPE,
        OIS::KC_1,                 //				KEY_KEY_1,
        OIS::KC_2,                 //				KEY_KEY_2,
        OIS::KC_3,                 //				KEY_KEY_3,
        OIS::KC_4,                 //				KEY_KEY_4,
        OIS::KC_5,                 //				KEY_KEY_5,
        OIS::KC_6,                 //				KEY_KEY_6,
        OIS::KC_7,                 //				KEY_KEY_7,
        OIS::KC_8,                 //				KEY_KEY_8,
        OIS::KC_9,                 //				KEY_KEY_9,
        OIS::KC_0,                 //				KEY_KEY_0,
        OIS::KC_MINUS,             //				KEY_MINUS,           // - on main keyboard
        OIS::KC_EQUALS,            //				KEY_PLUS,
        OIS::KC_BACK,              //				KEY_BACK,            // backspace
        OIS::KC_TAB,               //				KEY_TAB,
        OIS::KC_Q,                 //				KEY_KEY_Q,
        OIS::KC_W,                 //				KEY_KEY_W,
        OIS::KC_E,                 //				KEY_KEY_E,
        OIS::KC_R,                 //				KEY_KEY_R,
        OIS::KC_T,                 //				KEY_KEY_T,
        OIS::KC_Y,                 //				KEY_KEY_Y,
        OIS::KC_U,                 //				KEY_KEY_U,
        OIS::KC_I,                 //				KEY_KEY_I,
        OIS::KC_O,                 //				KEY_KEY_O,
        OIS::KC_P,                 //				KEY_KEY_P,
        OIS::KC_LBRACKET,          //				KEY_PA1,
        OIS::KC_RBRACKET,          //				KEY_PA1,
        OIS::KC_RETURN,            //				KEY_RETURN,          // Enter on main keyboard
        OIS::KC_LCONTROL,          //				KEY_LCONTROL,
        OIS::KC_A,                 //				KEY_KEY_A,
        OIS::KC_S,                 //				KEY_KEY_S,
        OIS::KC_D,                 //				KEY_KEY_D,
        OIS::KC_F,                 //				KEY_KEY_F,
        OIS::KC_G,                 //				KEY_KEY_G,
        OIS::KC_H,                 //				KEY_KEY_H,
        OIS::KC_J,                 //				KEY_KEY_J,
        OIS::KC_K,                 //				KEY_KEY_K,
        OIS::KC_L,                 //				KEY_KEY_L,
        OIS::KC_SEMICOLON,         //				KEY_PA1,
        OIS::KC_APOSTROPHE,        //				KEY_PA1,
        OIS::KC_GRAVE,             //				KEY_PA1,             // accent
        OIS::KC_LSHIFT,            //				KEY_LSHIFT,
        OIS::KC_BACKSLASH,         //				KEY_PA1,
        OIS::KC_Z,                 //				KEY_KEY_Z,
        OIS::KC_X,                 //				KEY_KEY_X,
        OIS::KC_C,                 //				KEY_KEY_C,
        OIS::KC_V,                 //				KEY_KEY_V,
        OIS::KC_B,                 //				KEY_KEY_B,
        OIS::KC_N,                 //				KEY_KEY_N,
        OIS::KC_M,                 //				KEY_KEY_M,
        OIS::KC_COMMA,             //				KEY_COMMA,
        OIS::KC_PERIOD,            //				KEY_PERIOD,          // . on main keyboard
        OIS::KC_SLASH,             //				KEY_PA1,             // / on main keyboard
        OIS::KC_RSHIFT,            //				KEY_RSHIFT,
        OIS::KC_MULTIPLY,          //				KEY_MULTIPLY,        // * on numeric keypad
        OIS::KC_LMENU,             //				KEY_LMENU,           // left Alt
        OIS::KC_SPACE,             //				KEY_SPACE,
        OIS::KC_CAPITAL,           //				KEY_CAPITAL,
        OIS::KC_F1,                //				KEY_F1,
        OIS::KC_F2,                //				KEY_F2,
        OIS::KC_F3,                //				KEY_F3,
        OIS::KC_F4,                //				KEY_F4,
        OIS::KC_F5,                //				KEY_F5,
        OIS::KC_F6,                //				KEY_F6,
        OIS::KC_F7,                //				KEY_F7,
        OIS::KC_F8,                //				KEY_F8,
        OIS::KC_F9,                //				KEY_F9,
        OIS::KC_F10,               //				KEY_F10,
        OIS::KC_NUMLOCK,           //				KEY_NUMLOCK,
        OIS::KC_SCROLL,            //				KEY_SCROLL,          // Scroll Lock
        OIS::KC_NUMPAD7,           //				KEY_NUMPAD7,
        OIS::KC_NUMPAD8,           //				KEY_NUMPAD8,
        OIS::KC_NUMPAD9,           //				KEY_NUMPAD9,
        OIS::KC_SUBTRACT,          //				KEY_SUBTRACT,        // - on numeric keypad
        OIS::KC_NUMPAD4,           //				KEY_NUMPAD4,
        OIS::KC_NUMPAD5,           //				KEY_NUMPAD5,
        OIS::KC_NUMPAD6,           //				KEY_NUMPAD6,
        OIS::KC_ADD,               //				KEY_ADD,             // + on numeric keypad
        OIS::KC_NUMPAD1,           //				KEY_NUMPAD1,
        OIS::KC_NUMPAD2,           //				KEY_NUMPAD2,
        OIS::KC_NUMPAD3,           //				KEY_NUMPAD3,
        OIS::KC_NUMPAD0,           //				KEY_NUMPAD0,
        OIS::KC_DECIMAL,           //				KEY_DECIMAL,         // . on numeric keypad
        KC_RESERVED, KC_RESERVED,  //				KEY_PA1, KEY_PA1, /
        OIS::KC_OEM_102,           //				KEY_PA1,             // < > | on UK/Germany keyboards
        OIS::KC_F11,               //				KEY_F11,
        OIS::KC_F12,               //				KEY_F12,

        // 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63
        KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED,
        KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED,

        OIS::KC_F13,  // 				KEY_F13,        //                     (NEC PC98)
        OIS::KC_F14,  // 				KEY_F14,        //                     (NEC PC98)
        OIS::KC_F15,  // 				KEY_F15,        //                     (NEC PC98)

        // 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
        KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED,
        KC_RESERVED, KC_RESERVED,

        OIS::KC_KANA,  //					KEY_KANA,       // (Japanese keyboard)

        // 0x71, 0x72,
        KC_RESERVED, KC_RESERVED,

        OIS::KC_ABNT_C1,  //					KEY_PA1,             // / ? on Portugese (Brazilian)
        // keyboards

        // 0x74, 0x75, 0x76, 0x77, 0x78,
        KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED,

        OIS::KC_CONVERT,    //					KEY_CONVERT,         // (Japanese keyboard)
        KC_RESERVED,        //					KEY_PA1,
        OIS::KC_NOCONVERT,  //					KEY_NONCONVERT,      // (Japanese keyboard)
        KC_RESERVED,        //					KEY_PA1,
        OIS::KC_YEN,        //					KEY_PA1,             // (Japanese keyboard)
        OIS::KC_ABNT_C2,    //					KEY_PA1,             // Numpad . on Portugese (Brazilian)
        // keyboards

        // 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C,
        KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED,
        KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED,

        OIS::KC_NUMPADEQUALS,  //		KEY_PA1,        // KC_NUMPADEQUALS= 0x8D,    // = on numeric
        // keypad (NEC PC98)
        KC_RESERVED, KC_RESERVED,
        // KEY_PA1, KEY_PA1,

        OIS::KC_PREVTRACK,  //		KEY_PA1,            // Previous Track (KC_CIRCUMFLEX on Japanese
        // keyboard)
        OIS::KC_AT,         //		KEY_PA1,            //                     (NEC PC98)
        OIS::KC_COLON,      //		KEY_PA1,            //                     (NEC PC98)
        OIS::KC_UNDERLINE,  //		KEY_PA1,            //                     (NEC PC98)
        OIS::KC_KANJI,      //		KEY_KANJI,          // (Japanese keyboard)
        OIS::KC_STOP,       //		KEY_PA1,            //                     (NEC PC98)
        OIS::KC_AX,         //		KEY_PA1,            //                     (Japan AX)
        OIS::KC_UNLABELED,  //		KEY_PA1,            //                        (J3100)
        KC_RESERVED,        //		KEY_PA1,
        OIS::KC_NEXTTRACK,  //		KEY_PA1,            // Next Track

        KC_RESERVED, KC_RESERVED,
        // KEY_PA1, KEY_PA1,

        OIS::KC_NUMPADENTER,  // 		KEY_PA1,            // Enter on numeric keypad
        OIS::KC_RCONTROL,     // 		KEY_RCONTROL,

        KC_RESERVED, KC_RESERVED,
        // KEY_PA1, KEY_PA1,

        OIS::KC_MUTE,        //			KEY_PA1,             // Mute
        OIS::KC_CALCULATOR,  //			KEY_PA1,             // Calculator
        OIS::KC_PLAYPAUSE,   //			KEY_PLAY,            // Play / Pause
        KC_RESERVED,         //			KEY_PA1,
        OIS::KC_MEDIASTOP,   //			KEY_PA1,             // Media Stop

        KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED,
        KC_RESERVED, KC_RESERVED,
        // KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1,

        OIS::KC_VOLUMEDOWN,   //							KEY_PA1,            // Volume -
        KC_RESERVED,          //			KEY_PA1,        //
        OIS::KC_VOLUMEUP,     //							KEY_PA1,            // Volume +
        KC_RESERVED,          //			KEY_PA1,        //
        OIS::KC_WEBHOME,      //							KEY_PA1,            // Web home
        OIS::KC_NUMPADCOMMA,  //							KEY_SEPARATOR,      // , on numeric keypad
        //(NEC PC98)
        KC_RESERVED,     //			KEY_PA1,        //
        OIS::KC_DIVIDE,  //							KEY_DIVIDE,         // / on numeric keypad
        KC_RESERVED,     //			KEY_PA1,        //
        OIS::KC_SYSRQ,   //							KEY_SNAPSHOT,
        OIS::KC_RMENU,   //							KEY_RMENU,          // right Alt

        KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED,
        KC_RESERVED, KC_RESERVED,
        // KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1,
        KC_RESERVED, KC_RESERVED, KC_RESERVED,
        // KEY_PA1, KEY_PA1, KEY_PA1,

        OIS::KC_PAUSE,   //							KEY_PAUSE,           // Pause
        KC_RESERVED,     //							KEY_PA1,        //
        OIS::KC_HOME,    //							KEY_HOME,            // Home on arrow keypad
        OIS::KC_UP,      //							KEY_UP,              // UpArrow on arrow keypad
        OIS::KC_PGUP,    //							KEY_PRIOR,           // PgUp on arrow keypad
        KC_RESERVED,     //							KEY_PA1,        //
        OIS::KC_LEFT,    //							KEY_LEFT,            // LeftArrow on arrow keypad
        KC_RESERVED,     //							KEY_PA1,        //
        OIS::KC_RIGHT,   //							KEY_RIGHT,           // RightArrow on arrow keypad
        KC_RESERVED,     //							KEY_PA1,        //
        OIS::KC_END,     //							KEY_END,             // End on arrow keypad
        OIS::KC_DOWN,    //							KEY_DOWN,            // DownArrow on arrow keypad
        OIS::KC_PGDOWN,  //							KEY_NEXT,            // PgDn on arrow keypad
        OIS::KC_INSERT,  //							KEY_INSERT,          // Insert on arrow keypad
        OIS::KC_DELETE,  //							KEY_DELETE,          // Delete on arrow keypad

        KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED, KC_RESERVED,
        // KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1, KEY_PA1,

        OIS::KC_LWIN,   //									KEY_LWIN,            // Left Windows key
        OIS::KC_RWIN,   //									KEY_RWIN,            // Right Windows key
        OIS::KC_APPS,   //									KEY_APPS,            // AppMenu key
        OIS::KC_POWER,  //									KEY_PA1,             // System Power
        OIS::KC_SLEEP,  //									KEY_SLEEP,           // System Sleep

        KC_RESERVED, KC_RESERVED, KC_RESERVED,
        // KEY_PA1, KEY_PA1, KEY_PA1,

        OIS::KC_WAKE,          //				KEY_PA1,             // System Wake
        KC_RESERVED,           //								KEY_PA1,        //
        OIS::KC_WEBSEARCH,     //				KEY_PA1,             // Web Search
        OIS::KC_WEBFAVORITES,  //				KEY_PA1,             // Web Favorites
        OIS::KC_WEBREFRESH,    //				KEY_PA1,             // Web Refresh
        OIS::KC_WEBSTOP,       //				KEY_PA1,             // Web Stop
        OIS::KC_WEBFORWARD,    //				KEY_PA1,             // Web Forward
        OIS::KC_WEBBACK,       //				KEY_PA1,             // Web Back
        OIS::KC_MYCOMPUTER,    //				KEY_PA1,             // My Computer
        OIS::KC_MAIL,          //				KEY_PA1,             // Mail
        OIS::KC_MEDIASELECT    //				KEY_PA1              // Media Select
    };

    /** Gets a firebird keycode. */
    inline u32 getKeyCode( OIS::KeyCode key )
    {
        return static_cast<u32>( oiKeyMap[key] );
    }

    /** */
    inline OIS::KeyCode getOISKeyCode( u32 fbKeyCode )
    {
        // return (OIS::KeyCode)fbToOISKeyMap[fbKeyCode];

        // todo hack loop through array till value found and return the element index
        for( u32 i = 0; i < 238; ++i )
        {
            if( static_cast<u32>( oiKeyMap[i] ) == fbKeyCode )
            {
                return static_cast<OIS::KeyCode>( i );
            }
        }

        return static_cast<OIS::KeyCode>( KC_RESERVED );
    }

    WP_CLASS_REGISTER_DERIVED( workphone, OISInputManager, IInputDeviceManager );
    WP_CLASS_REGISTER_DERIVED( workphone, OISInputManager::WindowListener,
                               render::IGraphicsWindowListener );

    OISInputManager::OISInputManager() = default;

    OISInputManager::OISInputManager( SmartPtr<render::IGraphicsWindow> win, bool bufferedKeys,
                                      bool bufferedMouse, bool bufferedJoy ) :
        // mInputManager(0),
        m_mouse( nullptr ),
        m_keyboard( nullptr )
    {
        m_window = win;
    }

    OISInputManager::~OISInputManager()
    {
        if( m_inputManager )
        {
            m_inputManager->destroyInputObject( m_mouse );
            m_inputManager->destroyInputObject( m_keyboard );
            // mInputManager->destroyInputObject( mJoy0 );
            // mInputManager->destroyInputObject( mJoy1 );

            OIS::InputManager::destroyInputSystem( m_inputManager );

            m_inputManager = nullptr;
            m_mouse = nullptr;
            m_keyboard = nullptr;
        }

        // Remove ourself as a Window listener
        // Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
        // windowClosed(mWindow);

        delete m_keyConverter.get();
        m_keyConverter = nullptr;
    }

#ifdef WP_PLATFORM_WIN32
    bool OISInputManager::registerRawInput( HWND hwnd )
    {
        RAWINPUTDEVICE Rid[1];

        // Register for mouse input
        Rid[0].usUsagePage = 0x01;
        Rid[0].usUsage = 0x02;  // Mouse
        Rid[0].dwFlags = 0;
        Rid[0].hwndTarget = hwnd;

        if( !RegisterRawInputDevices( Rid, 1, sizeof( RAWINPUTDEVICE ) ) )
        {
            // Registration failed
            return false;
        }

        return true;
    }
#endif

    void OISInputManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto bufferedKeys = true;
            auto bufferedMouse = true;
            auto bufferedJoy = true;

            m_keyConverter = new OISKeyConverter;

            using namespace OIS;

            ParamList pl;
            size_t windowHnd = 0;

            m_window->getWindowHandle( &windowHnd );

#ifdef WP_PLATFORM_WIN32
            if( windowHnd != 0 )
            {
                // Register for raw input
                if( !registerRawInput( (HWND)windowHnd ) )
                {
                    // Registration failed
                    // Handle error appropriately
                }
            }
#endif

            auto windowListener = workphone::make_ptr<WindowListener>();
            windowListener->setOwner( this );
            m_windowListener = windowListener;

            m_window->addListener( windowListener );

#if defined WP_PLATFORM_WIN32
            m_platformInputManager = workphone::make_ptr<PlatformInputManager>();
            m_platformInputManager->setWindow( m_window );
#elif defined WP_PLATFORM_APPLE
            m_platformInputManager = workphone::make_ptr<PlatformInputManager>();
            m_platformInputManager->setWindow( m_window );
#endif

            m_keyListener = workphone::make_shared<KeyListener>();
            m_mouseListener = workphone::make_shared<MouseListener>();
            m_joyStickListener = workphone::make_shared<JoyStickListener>();

            m_keyListener->setOwner( this );
            m_mouseListener->setOwner( this );
            m_joyStickListener->setOwner( this );

            if( windowHnd != 0 )
            {
                std::ostringstream windowHndStr;
                windowHndStr << windowHnd;
                pl.insert( std::make_pair( std::string( "WINDOW" ), windowHndStr.str() ) );
            }

#if defined WP_PLATFORM_WIN32
            pl.insert( std::make_pair( std::string( "w32_mouse" ), std::string( "DISCL_FOREGROUND" ) ) );
            pl.insert(
                std::make_pair( std::string( "w32_mouse" ), std::string( "DISCL_NONEXCLUSIVE" ) ) );
            // pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_EXCLUSIVE")));
#endif

            m_inputManager = OIS::InputManager::createInputSystem( pl );

            // Create all devices (We only catch joystick exceptions here, as, most people have
            // Key/Mouse)
            if( m_createKeyboard )
            {
                m_keyboard = static_cast<Keyboard *>(
                    m_inputManager->createInputObject( OISKeyboard, bufferedKeys ) );
            }

            if( m_createMouse )
            {
                m_mouse =
                    static_cast<Mouse *>( m_inputManager->createInputObject( OISMouse, bufferedMouse ) );
            }

            try
            {
                if( getCreateJoysticks() )
                {
                    // This demo uses at most MAX_JOYS joysticks
                    m_numSticks = m_inputManager->getNumberOfDevices( OISJoyStick );
                    if( m_numSticks > MAX_JOYS )
                    {
                        m_numSticks = MAX_JOYS;
                    }

                    m_oisJoysticks.resize( m_numSticks );
                    m_forceFeedbackDevices.resize( m_numSticks );

                    for( u32 i = 0; i < m_numSticks; ++i )
                    {
                        m_oisJoysticks[i] = static_cast<JoyStick *>(
                            m_inputManager->createInputObject( OISJoyStick, bufferedJoy ) );
                        m_oisJoysticks[i]->setEventCallback( m_joyStickListener.get() );

                        auto joyStickData = new JoyStickData;
                        setUserData( m_oisJoysticks[i], joyStickData );
                        joyStickData->m_joyStickIdx = i;

                        if( m_debugEnabled )
                        {
                            printf( "\nJoystick %d - ", i + 1 );
                        }

                        // Check for FF, if so, dump info
                        m_forceFeedbackDevices[i] = static_cast<ForceFeedback *>(
                            m_oisJoysticks[i]->queryInterface( Interface::ForceFeedback ) );
                        if( m_forceFeedbackDevices[i] )
                        {
                            if( m_debugEnabled )
                            {
                                printf( " Has FF Support!, Effects:\n" );
                            }

                            const ForceFeedback::SupportedEffectList &list =
                                m_forceFeedbackDevices[i]->getSupportedEffects();
                            auto i = list.begin(), e = list.end();

                            if( m_debugEnabled )
                            {
                                for( ; i != e; ++i )
                                {
                                    // printf("   Force=%s,
                                    // Type=%s\n",SForce[i->first].c_str(),SType[i->second].c_str());
                                }
                            }
                        }
                        else if( m_debugEnabled )
                        {
                            printf( "No FF Support\n" );
                        }

                        auto oisJoystick = workphone::make_ptr<workphone::OISJoystick>();
                        oisJoystick->setJoystick( m_oisJoysticks[i] );
                        m_joysticks.push_back( oisJoystick );
                    }
                }
            }
            catch( OIS::Exception &ex )
            {
                if( m_debugEnabled )
                {
                    printf( "Exception raised on joystick creation: %s\n", ex.eText );
                }
            }

            if( m_mouse )
            {
                m_mouse->setEventCallback( m_mouseListener.get() );
            }

            if( m_keyboard )
            {
                m_keyboard->setEventCallback( m_keyListener.get() );
            }

            if( m_mouse )
            {
                const OIS::MouseState &ms = m_mouse->getMouseState();

                auto size = m_window->getSize();
                ms.width = size.X();
                ms.height = size.Y();

                // OIS::MouseState &mutableMouseState =
                // const_cast<OIS::MouseState&>(mMouse->getMouseState()); mutableMouseState.X.rel = 0.5;
                // mutableMouseState.Y.rel = 0.5;

                // mutableMouseState.X.abs = (f32)win->getWidth() / 0.5f;
                // mutableMouseState.Y.abs = (f32)win->getHeight() / 0.5f;
            }

            m_currentInputEvent = workphone::make_ptr<InputEvent>();
            m_currentMouseState = workphone::make_ptr<MouseState>();
            m_currentKeyboardState = workphone::make_ptr<KeyboardState>();

            m_inputEventQueue.resize( static_cast<u32>( TaskId::Count ) );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void OISInputManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            m_gameInputs.clear();

            m_window = nullptr;
            m_windowListener = nullptr;
            m_platformInputManager = nullptr;

            m_keyListener = nullptr;
            m_mouseListener = nullptr;
            m_joyStickListener = nullptr;

            delete m_keyConverter.get();
            m_keyConverter = nullptr;

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void OISInputManager::update()
    {
        auto inputTask = getTask();
        auto task = Thread::getCurrentTask();

        if( task == inputTask )
        {
            // Need to capture/update each device
            if( m_keyboard )
            {
                m_keyboard->capture();
            }

            if( m_mouse )
            {
                m_mouse->capture();
            }

            for( auto joystick : m_oisJoysticks )
            {
                joystick->capture();
            }

            // poll input devices
            auto gameInputs = getGameInputs();
            for( auto &gameInput : gameInputs )
            {
                // gameInput->update();
            }

            if( m_mouse )
            {
                auto state = m_mouse->getMouseState();

                m_mouseScroll = Vector3<real_Num>( static_cast<real_Num>( state.X.rel ),
                                                   static_cast<real_Num>( state.Z.rel ),
                                                   static_cast<real_Num>( state.Y.rel ) );
            }
        }

        auto &taskQueue = m_inputEventQueue[static_cast<u32>( task )];
        while( !taskQueue.empty() )
        {
            SmartPtr<IInputEvent> event;
            if( taskQueue.try_pop( event ) )
            {
                if( event )
                {
                    auto listeners = m_listeners.snapshot();
                    for( auto &listener : listeners )
                    {
                        if( listener )
                        {
                            listener->handleEvent( EventType::Input, IEvent::inputEvent,
                                                   Array<Parameter>(), this, this, event );
                        }
                    }
                }
            }
        }

        // WP_LOG("InputManager::update");
    }

    bool OISInputManager::mouseMoved( const OIS::MouseEvent &arg )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto event = factoryManager->make_ptr<InputEvent>();
        event->setEventType( IInputEvent::EventType::Mouse );

        auto it = m_gameInputs.begin();
        for( ; it != m_gameInputs.end(); ++it )
        {
            auto gameInput = it->second;
            event->setGameInputId( gameInput->getId() );
            break;  // todo check this
        }

        auto mouseState = factoryManager->make_ptr<MouseState>();
        event->setMouseState( mouseState );

        auto size = m_window->getSize();
        if( size.X() == 0 || size.Y() == 0 )
        {
            return false;
        }

        const auto &ms = arg.state;
        ms.width = size.X();
        ms.height = size.Y();

        mouseState->setEventType( IMouseState::Event::Moved );

        if( ms.buttonDown( OIS::MB_Left ) )
        {
            mouseState->setButtonPressed( IMouseState::MOUSE_LEFT, true );
        }
        else
        {
            mouseState->setButtonPressed( IMouseState::MOUSE_LEFT, false );
        }

        if( ms.buttonDown( OIS::MB_Right ) )
        {
            mouseState->setButtonPressed( IMouseState::MOUSE_RIGHT, true );
        }
        else
        {
            mouseState->setButtonPressed( IMouseState::MOUSE_RIGHT, false );
        }

        if( ms.buttonDown( OIS::MB_Middle ) )
        {
            mouseState->setButtonPressed( IMouseState::MOUSE_MIDDLE, true );
        }
        else
        {
            mouseState->setButtonPressed( IMouseState::MOUSE_MIDDLE, false );
        }

        auto currentMousePosition =
            Vector2F( static_cast<f32>( arg.state.X.abs ), static_cast<f32>( arg.state.Y.abs ) );
        auto relativePosition = m_mousePosition - currentMousePosition;

        mouseState->setDelta( relativePosition );
        mouseState->setRelativePosition(
            currentMousePosition /
            Vector2F( static_cast<real_Num>( size.x ), static_cast<real_Num>( size.y ) ) );
        mouseState->setAbsolutePosition( currentMousePosition );

        auto relativeWheelPos = arg.state.Z.abs - m_lastWheelPos;

        auto wheelMoved = relativeWheelPos != 0;
        auto wheelDelta = Vector2<real_Num>( static_cast<real_Num>( relativeWheelPos ), 0 );
        mouseState->setWheelDelta( wheelDelta );
        m_lastWheelPos = arg.state.Z.abs;

        //WP_LOG( "InputManager::mouseMoved Test mouse move" );

        m_mousePosition = currentMousePosition;

        if( m_isDragging )
        {
            int deltaX = arg.state.X.abs - m_initialState.X.abs;
            int deltaY = arg.state.Y.abs - m_initialState.Y.abs;
            mouseState->setDragValue(
                Vector2<f32>( static_cast<f32>( deltaX ), static_cast<f32>( deltaY ) ) );
        }

        mouseState->setDragging( m_isDragging );

        postEvent( event );

        if( wheelMoved )
        {
            auto wheelEvent = factoryManager->make_ptr<InputEvent>();
            wheelEvent->setEventType( IInputEvent::EventType::Mouse );

            auto wheelMouseState = factoryManager->make_ptr<MouseState>();
            wheelEvent->setMouseState( wheelMouseState );

            wheelMouseState->setEventType( IMouseState::Event::Wheel );
            wheelMouseState->setWheelDelta( wheelDelta );

            if( m_isDragging )
            {
                int deltaX = arg.state.X.abs - m_initialState.X.abs;
                int deltaY = arg.state.Y.abs - m_initialState.Y.abs;
                wheelMouseState->setDragValue(
                    Vector2<f32>( static_cast<f32>( deltaX ), static_cast<f32>( deltaY ) ) );
            }

            wheelMouseState->setDragging( m_isDragging );

            postEvent( wheelEvent );
        }

        return true;
    }

    bool OISInputManager::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if( id == OIS::MB_Left )
        {
            m_isDragging = true;
            m_initialState = arg.state;  // Save initial mouse state
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto event = factoryManager->make_ptr<InputEvent>();
        event->setEventType( IInputEvent::EventType::Mouse );

        // GameInputs::iterator it = m_gameInputs.begin();
        // for(; it!=m_gameInputs.end(); ++it)
        //{
        //	OISGameInputPtr gameInput = it->second;
        //	event->setGameInputId(gameInput->getId());
        //	break;
        // }

        auto mouseState = factoryManager->make_ptr<MouseState>();
        event->setMouseState( mouseState );

        switch( id )
        {
        case OIS::MB_Left:
        {
            mouseState->setEventType( IMouseState::Event::LeftPressed );
            mouseState->setButtonPressed( IMouseState::MOUSE_LEFT, true );
        }
        break;
        case OIS::MB_Right:
        {
            mouseState->setEventType( IMouseState::Event::RightPressed );
            mouseState->setButtonPressed( IMouseState::MOUSE_RIGHT, true );
        }
        break;
        case OIS::MB_Middle:
        {
            mouseState->setEventType( IMouseState::Event::MiddlePressed );
            mouseState->setButtonPressed( IMouseState::MOUSE_MIDDLE, true );
        }
        break;
        default:
        {
        }
        break;
        }

        auto size = m_window->getSize();
        if( size.X() == 0 || size.Y() == 0 )
        {
            return false;
        }

        auto currentMousePosition =
            Vector2F( static_cast<f32>( arg.state.X.abs ), static_cast<f32>( arg.state.Y.abs ) );
        auto relativePosition = m_mousePosition - currentMousePosition;

        mouseState->setDelta( relativePosition );
        mouseState->setRelativePosition(
            currentMousePosition /
            Vector2F( static_cast<real_Num>( size.x ), static_cast<real_Num>( size.y ) ) );
        mouseState->setAbsolutePosition( currentMousePosition );
        mouseState->setWheelDelta( Vector2<f32>( (f32)arg.state.Z.rel, 0 ) );

        mouseState->setDragging( m_isDragging );

        postEvent( event );
        return true;
    }

    bool OISInputManager::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
    {
        if( id == OIS::MB_Left )
        {
            m_isDragging = false;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto event = factoryManager->make_ptr<InputEvent>();
        event->setEventType( IInputEvent::EventType::Mouse );

        // GameInputs::iterator it = m_gameInputs.begin();
        // for(; it!=m_gameInputs.end(); ++it)
        //{
        //	OISGameInputPtr gameInput = it->second;
        //	event->setGameInputId(gameInput->getId());
        //	break;
        // }

        auto mouseState = factoryManager->make_ptr<MouseState>();
        event->setMouseState( mouseState );

        switch( id )
        {
        case OIS::MB_Left:
        {
            mouseState->setEventType( IMouseState::Event::LeftReleased );
            mouseState->setButtonPressed( IMouseState::MOUSE_LEFT, false );
        }
        break;
        case OIS::MB_Right:
        {
            mouseState->setEventType( IMouseState::Event::RightReleased );
            mouseState->setButtonPressed( IMouseState::MOUSE_RIGHT, false );
        }
        break;
        case OIS::MB_Middle:
        {
            mouseState->setEventType( IMouseState::Event::MiddleReleased );
            mouseState->setButtonPressed( IMouseState::MOUSE_MIDDLE, false );
        }
        break;
        default:
        {
        }
        break;
        }

        auto size = m_window->getSize();
        if( size.X() == 0 || size.Y() == 0 )
        {
            return false;
        }

        auto currentMousePosition = Vector2F( static_cast<real_Num>( arg.state.X.abs ),
                                              static_cast<real_Num>( arg.state.Y.abs ) );
        auto relativePosition = m_mousePosition - currentMousePosition;

        mouseState->setDelta( relativePosition );
        mouseState->setRelativePosition(
            currentMousePosition /
            Vector2F( static_cast<real_Num>( size.x ), static_cast<real_Num>( size.y ) ) );
        mouseState->setAbsolutePosition( currentMousePosition );
        mouseState->setWheelDelta( Vector2<real_Num>( (real_Num)arg.state.Z.rel, 0 ) );

        mouseState->setDragging( m_isDragging );

        postEvent( event );
        return true;
    }

    bool OISInputManager::keyPressed( const OIS::KeyEvent &arg )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto event = factoryManager->make_ptr<InputEvent>();
        event->setEventType( IInputEvent::EventType::Key );

        auto keyboardState = factoryManager->make_ptr<KeyboardState>();
        event->setKeyboardState( keyboardState );

        keyboardState->setPressedDown( true );
        keyboardState->setShiftPressed( OIS::KC_LSHIFT == arg.key );
        keyboardState->setControlPressed( false );

        if( !keyboardState->isShiftPressed() )
        {
            keyboardState->setChar( arg.text );
            keyboardState->setKeyCode( getKeyCode( arg.key ) );
            keyboardState->setRawKeyCode( arg.key );
        }
        else
        {
            keyboardState->setChar( 0 );
            keyboardState->setKeyCode( 0 );
            keyboardState->setRawKeyCode( 0 );
        }

        // generate game event
        auto it = m_gameInputs.begin();
        for( ; it != m_gameInputs.end(); ++it )
        {
            SmartPtr<GameInput> gameInput = it->second;

            auto gameInputState = factoryManager->make_ptr<GameInputState>();
            event->setGameInputState( gameInputState );

            gameInputState->setEventType( static_cast<hash32>( IGameInputState::Action::Pressed ) );

            if( auto gameInputMap = gameInput->getGameInputMap() )
            {
                u32 action = gameInputMap->getActionFromKey( arg.key );
                if( action != IGameInput::UNASSIGNED )
                {
                    event->setGameInputId( gameInput->getId() );
                    gameInputState->setAction( action );
                    break;
                }
            }
        }

        postEvent( event );
        return true;
    }

    bool OISInputManager::keyReleased( const OIS::KeyEvent &arg )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto event = factoryManager->make_ptr<InputEvent>();
        event->setEventType( IInputEvent::EventType::Key );

        auto keyboardState = factoryManager->make_ptr<KeyboardState>();
        event->setKeyboardState( keyboardState );

        keyboardState->setPressedDown( false );
        keyboardState->setShiftPressed( OIS::KC_LSHIFT == arg.key );
        keyboardState->setControlPressed( false );

        if( !keyboardState->isShiftPressed() )
        {
            keyboardState->setChar( arg.text );
            keyboardState->setKeyCode( getKeyCode( arg.key ) );
            keyboardState->setRawKeyCode( arg.key );
        }
        else
        {
            keyboardState->setChar( 0 );
            keyboardState->setKeyCode( 0 );
            keyboardState->setRawKeyCode( 0 );
        }

        // generate game event
        auto it = m_gameInputs.begin();
        for( ; it != m_gameInputs.end(); ++it )
        {
            SmartPtr<GameInput> gameInput = it->second;

            auto gameInputState = factoryManager->make_ptr<GameInputState>();
            event->setGameInputState( gameInputState );

            gameInputState->setEventType( static_cast<hash32>( IGameInputState::Action::Released ) );

            if( auto gameInputMap = gameInput->getGameInputMap() )
            {
                auto action = gameInputMap->getActionFromKey( (u32)arg.key );
                if( action != IGameInput::UNASSIGNED )
                {
                    event->setGameInputId( gameInput->getId() );
                    gameInputState->setAction( action );
                    break;
                }
            }
        }

        postEvent( event );
        return true;
    }

    void OISInputManager::setJoystickIdx( const String &gameInputDeviceName, s32 joystickIdx )
    {
        m_gameDeviceJoystickMap[gameInputDeviceName] = joystickIdx;
    }

    s32 OISInputManager::getJoystickIdx( const String &gameInputDeviceName ) const
    {
        auto it = m_gameDeviceJoystickMap.find( gameInputDeviceName );
        if( it != m_gameDeviceJoystickMap.end() )
        {
            return it->second;
        }

        return -1;
    }

    SmartPtr<IGameInput> OISInputManager::addGameInput( hash_type id )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto gameInput = factoryManager->make_ptr<GameInput>();
            gameInput->setId( id );
            m_gameInputs[id] = gameInput;
            return gameInput;
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IGameInput> OISInputManager::findGameInput( hash_type id ) const
    {
        auto it = m_gameInputs.find( id );
        if( it != m_gameInputs.end() )
        {
            return it->second;
        }

        return nullptr;
    }

    Array<SmartPtr<IGameInput>> OISInputManager::getGameInputs() const
    {
        Array<SmartPtr<IGameInput>> gameInputs;
        gameInputs.reserve( static_cast<u32>( m_gameInputs.size() ) );

        auto iter = m_gameInputs.begin();
        for( ; iter != m_gameInputs.end(); ++iter )
        {
            gameInputs.push_back( iter->second );
        }

        return gameInputs;
    }

    bool OISInputManager::isCursorVisible() const
    {
        return true;
    }

    void OISInputManager::setCursorVisible( bool visible )
    {
#ifdef WP_PLATFORM_WIN32
        ShowCursor( visible ? TRUE : FALSE );
#endif
    }

    bool OISInputManager::povMoved( const OIS::JoyStickEvent &arg, int index )
    {
        SmartPtr<IInputEvent> event( new InputEvent );
        event->setEventType( IInputEvent::EventType::Joystick );

        SmartPtr<IJoystickState> joystickState( new JoystickState );
        event->setJoystickState( joystickState );

        if( auto joyStickData = static_cast<JoyStickData *>( getUserData( (void *)arg.device ) ) )
        {
            event->setGameInputId( joyStickData->m_gameInputId );
            joystickState->setJoystick( joyStickData->m_joyStickIdx );
        }

        joystickState->setEventType( static_cast<u32>( IJoystickState::Type::PovMoved ) );
        joystickState->setPOV( arg.state.mPOV[index].direction );

        for( u32 i = 0; i < static_cast<int>( IJoystickState::Axis::NUMBER_OF_AXES ); ++i )
        {
            joystickState->setAxis( i, 0 );
        }

        joystickState->setButtonId( 0 );
        joystickState->setPressedDown( true );

        // postEvent(event);
        return true;
    }

    bool OISInputManager::vector3Moved( const OIS::JoyStickEvent &arg, int index )
    {
        return true;
    }

    bool OISInputManager::buttonPressed( const OIS::JoyStickEvent &arg, int button )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto event = factoryManager->make_ptr<InputEvent>();
        event->setEventType( IInputEvent::EventType::Joystick );

        auto joystickState = factoryManager->make_ptr<JoystickState>();
        event->setJoystickState( joystickState );

        auto joyStickData = static_cast<JoyStickData *>( getUserData( (void *)arg.device ) );
        if( joyStickData )
        {
            event->setGameInputId( joyStickData->m_gameInputId );
            joystickState->setJoystick( joyStickData->m_joyStickIdx );
        }

        joystickState->setEventType( static_cast<u32>( IJoystickState::Type::ButtonPressed ) );
        joystickState->setPOV( 0 );

        for( u32 i = 0; i < static_cast<int>( IJoystickState::Axis::NUMBER_OF_AXES ); ++i )
        {
            joystickState->setAxis( i, static_cast<f32>( arg.state.mAxes[i].abs ) / 32768.0f );
        }

        joystickState->setButtonId( button );
        joystickState->setPressedDown( true );
        joystickState->setButtonPressed( button, true );

        SmartPtr<GameInput> gameInput = findGameInput( event->getGameInputId() );
        if( gameInput )
        {
            //generate game event
            auto gameInputState = factoryManager->make_ptr<GameInputState>();
            event->setGameInputState( gameInputState );

            gameInputState->setEventType( static_cast<hash32>( IGameInputState::Action::Pressed ) );
            gameInputState->setAction( gameInput->getGameInputMap()->getActionFromButton( button ) );
        }

        postEvent( event );
        return true;
    }

    bool OISInputManager::sliderMoved( const OIS::JoyStickEvent &arg, int index )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto event = factoryManager->make_ptr<InputEvent>();
        event->setEventType( IInputEvent::EventType::Joystick );

        auto joystickState = factoryManager->make_ptr<JoystickState>();
        event->setJoystickState( joystickState );

        auto joyStickData = static_cast<JoyStickData *>( getUserData( (void *)arg.device ) );
        if( joyStickData )
        {
            event->setGameInputId( joyStickData->m_gameInputId );
            joystickState->setJoystick( joyStickData->m_joyStickIdx );
        }

        joystickState->setEventType( static_cast<u32>( IJoystickState::Type::SliderMoved ) );
        joystickState->setPOV( 0 );

        for( u32 i = 0; i < static_cast<int>( IJoystickState::Axis::NUMBER_OF_AXES ); ++i )
        {
            joystickState->setAxis( i, static_cast<f32>( arg.state.mAxes[i].abs ) / 32768.0f );
        }

        //joystickState->setButtonId( button );
        //joystickState->setPressedDown( true );
        //joystickState->setButtonPressed( button, true );

        SmartPtr<GameInput> gameInput = findGameInput( event->getGameInputId() );
        if( gameInput )
        {
            //generate game event
            auto gameInputState = factoryManager->make_ptr<GameInputState>();
            event->setGameInputState( gameInputState );

            gameInputState->setEventType( static_cast<hash32>( IGameInputState::Action::Pressed ) );
            //gameInputState->setAction( gameInput->getGameInputMap()->getActionFromButton( button ) );
        }

        postEvent( event );
        return true;
    }

    bool OISInputManager::axisMoved( const OIS::JoyStickEvent &arg, int axis )
    {
        if( isLoaded() )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto event = factoryManager->make_ptr<InputEvent>();
            event->setEventType( IInputEvent::EventType::Joystick );

            auto joystickState = factoryManager->make_ptr<JoystickState>();
            event->setJoystickState( joystickState );

            if( arg.device )
            {
                auto joyStickData = static_cast<JoyStickData *>( getUserData( (void *)arg.device ) );
                if( joyStickData )
                {
                    event->setGameInputId( joyStickData->m_gameInputId );
                    joystickState->setJoystick( joyStickData->m_joyStickIdx );
                }

                joystickState->setEventType( static_cast<u32>( IJoystickState::Type::AxisMoved ) );
                joystickState->setPOV( 0 );

                for( u32 i = 0; i < static_cast<int>( IJoystickState::Axis::NUMBER_OF_AXES ); ++i )
                {
                    joystickState->setAxis( i, static_cast<f32>( arg.state.mAxes[i].abs ) / 32768.0f );
                }

                //joystickState->setButtonId( button );
                //joystickState->setPressedDown( true );
                //joystickState->setButtonPressed( button, true );

                SmartPtr<GameInput> gameInput = findGameInput( event->getGameInputId() );
                if( gameInput )
                {
                    //generate game event
                    auto gameInputState = factoryManager->make_ptr<GameInputState>();
                    event->setGameInputState( gameInputState );

                    gameInputState->setEventType(
                        static_cast<hash32>( IGameInputState::Action::Pressed ) );
                    //gameInputState->setAction( gameInput->getGameInputMap()->getActionFromButton( button ) );
                }

                postEvent( event );
                return true;
            }
        }

        return false;
    }

    bool OISInputManager::buttonReleased( const OIS::JoyStickEvent &arg, int button )
    {
        SmartPtr<IInputEvent> event( new InputEvent );
        event->setEventType( IInputEvent::EventType::Joystick );

        SmartPtr<IJoystickState> joystickState( new JoystickState );
        event->setJoystickState( joystickState );

        auto joyStickData = static_cast<JoyStickData *>( getUserData( (void *)arg.device ) );
        if( joyStickData )
        {
            event->setGameInputId( joyStickData->m_gameInputId );
            joystickState->setJoystick( joyStickData->m_joyStickIdx );
        }

        joystickState->setEventType( static_cast<u32>( IJoystickState::Type::ButtonReleased ) );
        // event.joystick.ButtonStates = 0;
        joystickState->setPOV( 0 );

        for( u32 i = 0; i < static_cast<int>( IJoystickState::Axis::NUMBER_OF_AXES ); ++i )
        {
            joystickState->setAxis( i, static_cast<f32>( arg.state.mAxes[i].abs ) / 32768.0f );
        }

        joystickState->setButtonId( button );
        joystickState->setPressedDown( false );
        joystickState->setButtonPressed( button, true );

        SmartPtr<GameInput> gameInput = findGameInput( event->getGameInputId() );
        if( gameInput )
        {
            // generate game event
            SmartPtr<IGameInputState> gameInputState = workphone::make_ptr<GameInputState>();
            event->setGameInputState( gameInputState );

            gameInputState->setEventType( static_cast<u32>( IGameInputState::Action::Released ) );
            gameInputState->setAction( gameInput->getGameInputMap()->getActionFromButton( button ) );
        }

        postEvent( event );
        return true;
    }

    bool OISInputManager::postEvent( SmartPtr<IInputEvent> event )
    {
        if( !m_inputEventQueue.empty() )
        {
            for( auto &queue : m_inputEventQueue )
            {
                queue.push( event );
            }

            return true;
        }

        return false;
    }

    SmartPtr<IInputEvent> OISInputManager::createInputEvent()
    {
        return workphone::make_ptr<InputEvent>();
    }

    SmartPtr<IMouseState> OISInputManager::createMouseState()
    {
        return workphone::make_ptr<MouseState>();
    }

    SmartPtr<IKeyboardState> OISInputManager::createKeyboardState()
    {
        return workphone::make_ptr<KeyboardState>();
    }

    SmartPtr<IInputEvent> OISInputManager::getCurrentInputEvent() const
    {
        return m_currentInputEvent;
    }

    SmartPtr<IMouseState> OISInputManager::getCurrentMouseState() const
    {
        return m_currentMouseState;
    }

    SmartPtr<IKeyboardState> OISInputManager::getCurrentKeyboardState() const
    {
        return m_currentKeyboardState;
    }

    void OISInputManager::addListener( SmartPtr<IEventListener> listener )
    {
        m_listeners.push_back( listener );
    }

    void OISInputManager::removeListener( SmartPtr<IEventListener> listener )
    {
        auto it = std::find( m_listeners.begin(), m_listeners.end(), listener );
        if( it != m_listeners.end() )
        {
            m_listeners.erase( it );
        }
    }

    void OISInputManager::removeListeners()
    {
    }

    Vector3<real_Num> OISInputManager::getMouseScroll() const
    {
        return m_mouseScroll;
    }

    bool OISInputManager::isShiftPressed() const
    {
        return false;
    }

    void OISInputManager::setShiftPressed( bool shiftPressed )
    {
    }

    f64 OISInputManager::getLastClickTime() const
    {
        return 0.0;
    }

    void OISInputManager::setLastClickTime( f64 lastClickTime )
    {
    }

    f64 OISInputManager::getDoubleClickInterval() const
    {
        return 0.0;
    }

    void OISInputManager::setDoubleClickInterval( f64 doubleClickInterval )
    {
    }

    f64 OISInputManager::getLastInputTime() const
    {
        return 0.0;
    }

    void OISInputManager::setLastInputTime( f64 lastInputTime )
    {
    }

    f64 OISInputManager::getInputTime() const
    {
        return 0.0;
    }

    void OISInputManager::setInputTime( f64 inputTime )
    {
    }

    void OISInputManager::addInputTime( f64 inputTime )
    {
    }

    void OISInputManager::setUserData( void *ptr1, void *ptr2 )
    {
        m_userData[ptr1] = ptr2;
    }

    void *OISInputManager::getUserData( void *ptr ) const
    {
        auto it = m_userData.find( ptr );
        if( it != m_userData.end() )
        {
            return it->second;
        }

        return nullptr;
    }

    void OISInputManager::triggerEvent( SmartPtr<IInputEvent> inputEvent )
    {
    }

    void OISInputManager::queueEvent( SmartPtr<IInputEvent> event )
    {
    }

    void OISInputManager::setForceFeedbackDevices(
        const Array<RawPtr<OIS::ForceFeedback>> &forceFeedbackDevices )
    {
        m_forceFeedbackDevices = forceFeedbackDevices;
    }

    Array<RawPtr<OIS::ForceFeedback>> OISInputManager::getForceFeedbackDevices() const
    {
        return m_forceFeedbackDevices;
    }

    void OISInputManager::setJoySticks( const Array<RawPtr<OIS::JoyStick>> &joySticks )
    {
        m_oisJoysticks = joySticks;
    }

    Array<RawPtr<OIS::JoyStick>> OISInputManager::getJoySticks() const
    {
        return m_oisJoysticks;
    }

    void OISInputManager::setKeyboard( OIS::Keyboard *keyboard )
    {
        m_keyboard = keyboard;
    }

    OIS::Keyboard *OISInputManager::getKeyboard() const
    {
        return m_keyboard;
    }

    void OISInputManager::setMouse( OIS::Mouse *mouse )
    {
        m_mouse = mouse;
    }

    OIS::Mouse *OISInputManager::getMouse() const
    {
        return m_mouse;
    }

    void OISInputManager::setInputManager( OIS::InputManager *inputManager )
    {
        m_inputManager = inputManager;
    }

    OIS::InputManager *OISInputManager::getInputManager() const
    {
        return m_inputManager;
    }

    bool OISInputManager::isMouseButtonDown( u32 button ) const
    {
        if( m_mouse )
        {
            return m_mouse->getMouseState().buttonDown( static_cast<OIS::MouseButtonID>( button ) );
        }

        return false;
    }

    bool OISInputManager::isKeyPressed( KeyCodes keyCode ) const
    {
        ScopedLock lock( this );

        if( m_keyboard )
        {
            auto eKeyCode = getOISKeyCode( static_cast<u32>( keyCode ) );
            return m_keyboard->isKeyDown( eKeyCode );
        }

        return false;
    }

    bool OISInputManager::getCreateMouse() const
    {
        return m_createMouse;
    }

    void OISInputManager::setCreateMouse( bool createMouse )
    {
        m_createMouse = createMouse;
    }

    bool OISInputManager::getCreateKeyboard() const
    {
        return m_createKeyboard;
    }

    void OISInputManager::setCreateKeyboard( bool createKeyboard )
    {
        m_createKeyboard = createKeyboard;
    }

    bool OISInputManager::getCreateJoysticks() const
    {
        return m_createJoysticks;
    }

    void OISInputManager::setCreateJoysticks( bool createJoysticks )
    {
        m_createJoysticks = createJoysticks;
    }

    SmartPtr<render::IGraphicsWindow> OISInputManager::getWindow() const
    {
        return m_window;
    }

    void OISInputManager::setWindow( SmartPtr<render::IGraphicsWindow> window )
    {
        m_window = window;
    }

    Array<SmartPtr<IJoystick>> OISInputManager::getJoysticks() const
    {
        return m_joysticks;
    }

    void OISInputManager::setJoysticks( const Array<SmartPtr<IJoystick>> &joysticks )
    {
        m_joysticks = joysticks;
    }

    void OISInputManager::_getObject( void **ppObject )
    {
        *ppObject = m_inputManager;
    }

    OISInputManager::KeyListener::KeyListener() = default;

    OISInputManager::KeyListener::~KeyListener() = default;

    bool OISInputManager::KeyListener::keyPressed( const OIS::KeyEvent &arg )
    {
        auto owner = getOwner();
        return owner->keyPressed( arg );
    }

    bool OISInputManager::KeyListener::keyReleased( const OIS::KeyEvent &arg )
    {
        auto owner = getOwner();
        return owner->keyReleased( arg );
    }

    OISInputManager *OISInputManager::KeyListener::getOwner() const
    {
        return m_owner;
    }

    void OISInputManager::KeyListener::setOwner( OISInputManager *owner )
    {
        m_owner = owner;
    }

    OISInputManager::MouseListener::MouseListener() = default;

    OISInputManager::MouseListener::~MouseListener() = default;

    bool OISInputManager::MouseListener::mouseMoved( const OIS::MouseEvent &arg )
    {
        return m_owner->mouseMoved( arg );
    }

    bool OISInputManager::MouseListener::mousePressed( const OIS::MouseEvent &arg,
                                                       OIS::MouseButtonID id )
    {
        return m_owner->mousePressed( arg, id );
    }

    bool OISInputManager::MouseListener::mouseReleased( const OIS::MouseEvent &arg,
                                                        OIS::MouseButtonID id )
    {
        return m_owner->mouseReleased( arg, id );
    }

    OISInputManager *OISInputManager::MouseListener::getOwner() const
    {
        return m_owner;
    }

    void OISInputManager::MouseListener::setOwner( OISInputManager *owner )
    {
        m_owner = owner;
    }

    OISInputManager::JoyStickListener::JoyStickListener() = default;

    OISInputManager::JoyStickListener::~JoyStickListener() = default;

    bool OISInputManager::JoyStickListener::povMoved( const OIS::JoyStickEvent &arg, int index )
    {
        return m_owner->povMoved( arg, index );
    }

    bool OISInputManager::JoyStickListener::vector3Moved( const OIS::JoyStickEvent &arg, int index )
    {
        return m_owner->vector3Moved( arg, index );
    }

    bool OISInputManager::JoyStickListener::buttonPressed( const OIS::JoyStickEvent &arg, int button )
    {
        return m_owner->buttonPressed( arg, button );
    }

    bool OISInputManager::JoyStickListener::sliderMoved( const OIS::JoyStickEvent &arg, int index )
    {
        return m_owner->sliderMoved( arg, index );
    }

    bool OISInputManager::JoyStickListener::axisMoved( const OIS::JoyStickEvent &arg, int axis )
    {
        return m_owner->axisMoved( arg, axis );
    }

    bool OISInputManager::JoyStickListener::buttonReleased( const OIS::JoyStickEvent &arg, int button )
    {
        return m_owner->buttonReleased( arg, button );
    }

    OISInputManager *OISInputManager::JoyStickListener::getOwner() const
    {
        return m_owner;
    }

    void OISInputManager::JoyStickListener::setOwner( OISInputManager *owner )
    {
        m_owner = owner;
    }

    OISInputManager::WindowListener::WindowListener() = default;

    OISInputManager::WindowListener::~WindowListener() = default;

    void OISInputManager::WindowListener::handleEvent( SmartPtr<render::IGraphicsWindowEvent> event )
    {
    }

    Parameter OISInputManager::WindowListener::handleEvent( EventType eventType, hash_type eventValue,
                                                            const Array<Parameter> &arguments,
                                                            SmartPtr<ISharedObject> sender,
                                                            SmartPtr<ISharedObject> object,
                                                            SmartPtr<IEvent> event )
    {
        if( eventValue == windowClosingHash )
        {
            return Parameter( true );
        }

        return {};
    }

    void OISInputManager::WindowListener::setOwner( OISInputManager *owner )
    {
        m_owner = owner;
    }

    OISInputManager *OISInputManager::WindowListener::getOwner() const
    {
        return m_owner;
    }
}  // namespace workphone
