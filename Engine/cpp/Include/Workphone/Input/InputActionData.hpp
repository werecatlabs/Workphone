#ifndef InputActionData_h__
#define InputActionData_h__

#include <Workphone/Interface/Input/IInputAction.hpp>

/**
 * @file InputActionData.hpp
 * @brief Simple value container for one input binding (primary/secondary input and action id).
 */

namespace workphone
{

    /**
     * @class InputActionData
     * @brief Plain-data implementation of IInputAction.
     *
     * Describes the binding of one game action: the action id and up to two inputs that trigger
     * it. The primary and secondary inputs are stored as numeric values: for keyboard bindings the
     * hash of the key name (the readable names can additionally be stored with setPrimaryName()
     * and setSecondaryName()); for joystick bindings the button index. A value of 0 means unset.
     *
     * @see GameInputMap
     */
    class InputActionData : public IInputAction
    {
    public:
        /**
         * @brief Constructs an empty InputActionData (all ids 0, empty names).
         */
        InputActionData();

        /**
         * @brief Constructs with initial primary and secondary inputs and an action id.
         *
         * @param first Primary input (key-name hash or joystick button index)
         * @param second Secondary input (key-name hash or joystick button index)
         * @param actionId Id of the action triggered by these inputs
         */
        InputActionData( u32 first, u32 second, u32 actionId );

        /**
         * @copydoc IInputAction::getPrimaryAction
         */
        hash_type getPrimaryAction() const override;

        /**
         * @copydoc IInputAction::setPrimaryAction
         * @param primary Primary input (key-name hash or joystick button index)
         */
        void setPrimaryAction( hash_type primary ) override;

        /**
         * @copydoc IInputAction::getSecondaryAction
         */
        hash_type getSecondaryAction() const override;

        /**
         * @copydoc IInputAction::setSecondaryAction
         * @param secondary Secondary input (key-name hash or joystick button index)
         */
        void setSecondaryAction( hash_type secondary ) override;

        /**
         * @copydoc IInputAction::getActionId
         */
        hash_type getActionId() const override;

        /**
         * @copydoc IInputAction::setActionId
         * @param actionId Id of the action triggered by the primary/secondary inputs
         */
        void setActionId( hash_type actionId ) override;

        /**
         * @brief Gets the stored primary key name (keyboard bindings).
         * @return The primary key name; empty if none was set.
         */
        String getPrimaryName() const;

        /**
         * @brief Sets the stored primary key name.
         * @param name Key name to store. This does not update the primary input value.
         */
        void setPrimaryName( const String &name );

        /**
         * @brief Gets the stored secondary key name (keyboard bindings).
         * @return The secondary key name; empty if none was set.
         */
        String getSecondaryName() const;

        /**
         * @brief Sets the stored secondary key name.
         * @param name Key name to store. This does not update the secondary input value.
         */
        void setSecondaryName( const String &name );

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Primary input (key-name hash or joystick button index); 0 if unset.
        hash_type m_primary = 0;
        /// Secondary input (key-name hash or joystick button index); 0 if unset.
        hash_type m_secondary = 0;
        /// Id of the action triggered by the primary/secondary inputs.
        hash_type m_actionId = 0;
        /// Optional readable primary key name (keyboard bindings only).
        String m_primaryName;
        /// Optional readable secondary key name (keyboard bindings only).
        String m_secondaryName;
    };
}  // namespace workphone

#endif  // InputActionData_h__
