#ifndef __GameInputMap_h__
#define __GameInputMap_h__

#include <Workphone/Interface/Input/IGameInputMap.hpp>
#include <Workphone/Interface/Input/IInputAction.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/HashMap.hpp>

/**
 * @file GameInputMap.hpp
 * @brief Maps raw input (keys/buttons) to high-level game actions.
 */

namespace workphone
{

    /**
     * @brief Stores mappings between keyboard/joystick inputs and game actions.
     *
     * GameInputMap maintains lookup tables that associate physical key or
     * button identifiers with higher-level action identifiers (IInputAction
     * instances). It supports querying by key/button and retrieving or
     * setting mappings for keyboard and joystick inputs.
     */
    class GameInputMap : public IGameInputMap
    {
    public:
        /**
         * @brief Default constructor.
         */
        GameInputMap();

        /**
         * @brief Copy constructor.
         *
         * Performs a deep copy of mapping tables.
         */
        GameInputMap( const GameInputMap &other );

        /**
         * @brief Virtual destructor.
         */
        ~GameInputMap() override;

        /**
         * @brief Lookup the action id associated with a keyboard key code.
         *
         * @param key Key code to query
         * @return u32 Mapped action id or 0 if none
         */
        u32 getActionFromKey( u32 key ) const override;

        /**
         * @brief Assign a keyboard mapping for the given action id using two
         *        optional key strings.
         *
         * @param id Action id to map
         * @param key0 Primary key name or identifier
         * @param key1 Secondary key name or identifier
         */
        void setKeyboardAction( u32 id, const String &key0, const String &key1 ) override;

        /**
         * @brief Retrieve the two key strings mapped to the given action id.
         *
         * @param id Action id to query
         * @param key0 Output primary key string
         * @param key1 Output secondary key string
         */
        void getKeyboardAction( u32 id, String &key0, String &key1 ) override;

        /**
         * @brief Lookup the action id associated with a joystick button.
         *
         * @param button Button index to query
         * @return u32 Mapped action id or 0 if none
         */
        u32 getActionFromButton( u32 button ) const override;

        /**
         * @brief Retrieve joystick button mappings for a given action id.
         *
         * @param id Action id to query
         * @param button0 Output primary button index
         * @param button1 Output secondary button index
         */
        void getJoystickAction( u32 id, u32 &button0, u32 &button1 ) override;

        /**
         * @brief Set joystick button mappings for a given action id.
         *
         * @param id Action id to map
         * @param button0 Primary button index
         * @param button1 Secondary button index
         */
        void setJoystickAction( u32 id, u32 button0, u32 button1 ) override;

        /**
         * @brief Access the internal keyboard mapping table.
         *
         * @return const Map<u32, SmartPtr<IInputAction>>&
         */
        const Map<u32, SmartPtr<IInputAction>> &getKeyboardMap() const;

        /**
         * @brief Access the internal joystick mapping table.
         *
         * @return const Map<u32, SmartPtr<IInputAction>>&
         */
        const Map<u32, SmartPtr<IInputAction>> &getJoystickMap() const;

        /**
         * @brief Try to retrieve the IInputAction data for the provided
         *        button id.
         *
         * @param button Button id to query
         * @param data Output parameter filled with the action data if found
         * @return true if action data was found
         */
        bool getInputActionData( u32 button, SmartPtr<IInputAction> &data );

    protected:
        /**
         * @brief Internal helper to set keyboard mapping using an IInputAction
         *        instance.
         */
        void setKeyboardAction( u32 id, const SmartPtr<IInputAction> &actionData );

        /**
         * @brief Internal helper to set joystick mapping using an IInputAction
         *        instance.
         */
        void setJoystickAction( u32 id, const SmartPtr<IInputAction> &actionData );

        /// Mapping table from keyboard key id to input action data.
        Map<u32, SmartPtr<IInputAction>> m_keyboardMap;

        /// Mapping table from joystick button id to input action data.
        Map<u32, SmartPtr<IInputAction>> m_joystickMap;
    };
}  // namespace workphone

#endif  // GameInputMap_h__
