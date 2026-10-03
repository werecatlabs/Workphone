#ifndef IGameInputMap_h__
#define IGameInputMap_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * Interface for a game input mapping. Used to map input events (such as keys or joystick buttons) to
     * game actions.
     */
    class WPCore_API IGameInputMap : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IGameInputMap() override;

        /** Sets the keyboard action corresponding to the given id.
         * @param id The action id.
         */
        virtual void setKeyboardAction( u32 id, const String &key0, const String &key1 ) = 0;

        /** Gets the keyboard action corresponding to the given id. */
        virtual void getKeyboardAction( u32 id, String &key0, String &key1 ) = 0;

        /** Sets the joystick action corresponding to the given id. */
        virtual void setJoystickAction( u32 id, u32 button0, u32 button1 ) = 0;

        /** Gets the joystick action corresponding to the given id. */
        virtual void getJoystickAction( u32 id, u32 &button0, u32 &button1 ) = 0;

        /** Gets the action ID corresponding to the given joystick button. */
        virtual u32 getActionFromButton( u32 button ) const = 0;

        /** Gets the action ID corresponding to the given key. */
        virtual u32 getActionFromKey( u32 key ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IGameInputMap_h__
