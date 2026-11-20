#ifndef __GameInputMap_h__
#define __GameInputMap_h__

#include <Workphone/Interface/Input/IGameInputMap.hpp>
#include <Workphone/Interface/Input/IInputAction.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/HashMap.hpp>

/**
 * @file GameInputMap.hpp
 * @brief Maps raw keyboard keys and joystick buttons to game action ids.
 */

namespace workphone
{

    /**
     * @class GameInputMap
     * @brief Stores the keyboard and joystick bindings of the game actions.
     *
     * Two tables, keyed by action id, hold an IInputAction (primary and secondary binding) per
     * action: one for keyboard keys and one for joystick buttons. Keyboard bindings are set by key
     * name (hashed and also stored as text); joystick bindings are button indices. The reverse
     * lookups getActionFromKey() and getActionFromButton() return IGameInput::UNASSIGNED when
     * nothing matches.
     */
    class GameInputMap : public IGameInputMap
    {
    public:
        /**
         * @brief Default constructor. Creates empty tables with rebinding enabled.
         */
        GameInputMap();

        /**
         * @brief Copy constructor.
         *
         * Deep-copies the keyboard and joystick tables (entries that are not InputActionData are
         * skipped). The rebinding flag is not copied and is enabled in the new object.
         *
         * @param other The map to copy.
         */
        GameInputMap( const GameInputMap &other );

        /**
         * @brief Virtual destructor.
         */
        ~GameInputMap() override;

        /**
         * @brief Enables or disables rebinding of already mapped actions.
         *
         * While disabled, setKeyboardAction() and setJoystickAction() ignore calls for an action id
         * that already has a binding; unmapped ids can still be bound.
         *
         * @param enabled true to allow existing bindings to be replaced (the default).
         */
        void setRebindingEnabled( bool enabled ) { m_rebindingEnabled = enabled; }

        /**
         * @brief Looks up the action bound to a keyboard key.
         *
         * A binding matches if the key equals its primary or secondary hash, or, for a non-zero
         * key, if the key equals the native key code of its stored key name (a single character,
         * or Space, Enter, Escape, Tab, Left, Up, Right, Down, Shift, Control, Alt).
         *
         * @param key Key code to query.
         * @return The action id of the first matching binding, or IGameInput::UNASSIGNED if none.
         */
        u32 getActionFromKey( u32 key ) const override;

        /**
         * @brief Binds an action id to up to two keyboard keys, replacing any existing binding.
         *
         * The key names are stored and hashed with StringUtil::getHash().
         *
         * @param id Action id to bind.
         * @param key0 Primary key name.
         * @param key1 Secondary key name (may be empty).
         * @note Ignored if rebinding is disabled and the action id is already bound.
         */
        void setKeyboardAction( u32 id, const String &key0, const String &key1 ) override;

        /**
         * @brief Retrieves the key names bound to an action id.
         *
         * @param id Action id to query.
         * @param[out] key0 Receives the primary key name; cleared if the action is not bound.
         * @param[out] key1 Receives the secondary key name; cleared if the action is not bound.
         */
        void getKeyboardAction( u32 id, String &key0, String &key1 ) override;

        /**
         * @brief Looks up the action bound to a joystick button.
         *
         * @param button Button index to query.
         * @return The action id of the first binding whose primary or secondary button matches, or
         *         IGameInput::UNASSIGNED if none.
         */
        u32 getActionFromButton( u32 button ) const override;

        /**
         * @brief Retrieves the joystick buttons bound to an action id.
         *
         * @param id Action id to query.
         * @param[out] button0 Receives the primary button index; IGameInput::UNASSIGNED if the
         *             action is not bound.
         * @param[out] button1 Receives the secondary button index; IGameInput::UNASSIGNED if the
         *             action is not bound.
         */
        void getJoystickAction( u32 id, u32 &button0, u32 &button1 ) override;

        /**
         * @brief Binds an action id to up to two joystick buttons, replacing any existing binding.
         *
         * @param id Action id to bind.
         * @param button0 Primary button index.
         * @param button1 Secondary button index.
         * @note Ignored if rebinding is disabled and the action id is already bound.
         */
        void setJoystickAction( u32 id, u32 button0, u32 button1 ) override;

        /**
         * @brief Gets the keyboard binding table.
         *
         * @return Read-only table mapping action id to its keyboard binding.
         */
        const Map<u32, SmartPtr<IInputAction>> &getKeyboardMap() const;

        /**
         * @brief Gets the joystick binding table.
         *
         * @return Read-only table mapping action id to its joystick binding.
         */
        const Map<u32, SmartPtr<IInputAction>> &getJoystickMap() const;

        /**
         * @brief Finds the joystick binding that contains the given button.
         *
         * @param button Button index to look for as primary or secondary button.
         * @param[out] data Receives the matching binding; left unchanged if none is found.
         * @return true if a binding was found, false otherwise.
         */
        bool getInputActionData( u32 button, SmartPtr<IInputAction> &data );

    protected:
        /**
         * @brief Stores a keyboard binding for an action id, replacing any existing one.
         *
         * Unlike the public overload, this does not honor the rebinding flag.
         *
         * @param id Action id to bind.
         * @param actionData The binding to store.
         */
        void setKeyboardAction( u32 id, const SmartPtr<IInputAction> &actionData );

        /**
         * @brief Stores a joystick binding for an action id, replacing any existing one.
         *
         * Unlike the public overload, this does not honor the rebinding flag.
         *
         * @param id Action id to bind.
         * @param actionData The binding to store.
         */
        void setJoystickAction( u32 id, const SmartPtr<IInputAction> &actionData );

        /// Keyboard bindings, keyed by action id.
        Map<u32, SmartPtr<IInputAction>> m_keyboardMap;

        /// Joystick bindings, keyed by action id.
        Map<u32, SmartPtr<IInputAction>> m_joystickMap;
        /// Whether existing bindings may be replaced (see setRebindingEnabled()).
        bool m_rebindingEnabled = true;
    };
}  // namespace workphone

#endif  // GameInputMap_h__
