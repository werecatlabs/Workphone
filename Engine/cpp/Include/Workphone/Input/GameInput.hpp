#ifndef GameInput_h__
#define GameInput_h__

#include <Workphone/Interface/Input/IGameInput.hpp>

/**
 * @file GameInput.hpp
 * @brief Represents a player's input source and readiness state.
 */

namespace workphone
{

    /**
     * @brief Concrete implementation of IGameInput representing a player's
     *        input interface (keyboard, joystick, dongle, etc.).
     *
     * GameInput holds identification information (player index, joystick id)
     * and exposes atomic flags describing the readiness of various input
     * subsystems (dongle, general input, keyboard). This allows thread-safe
     * queries and updates from different engine systems.
     */
    class WPCore_API GameInput : public IGameInput
    {
    public:
        /**
         * @brief Construct a new GameInput object.
         */
        GameInput();

        /**
         * @brief Virtual destructor.
         */
        ~GameInput() override;

        /**
         * @brief Returns whether this GameInput is currently assigned to a
         *        player or input source.
         *
         * @return true if assigned, false otherwise
         */
        bool isAssigned() const override;

        /**
         * @brief Retrieve the input mapping for this GameInput.
         *
         * @return SmartPtr<IGameInputMap> Pointer to the associated input map
         */
        SmartPtr<IGameInputMap> getGameInputMap() const override;

        /**
         * @brief Set the player index associated with this input.
         *
         * @param playerIndex Index of the player (usually 0-based)
         */
        void setPlayerIndex( u32 playerIndex ) override;

        /**
         * @brief Get the player index associated with this input.
         *
         * @return u32 Player index
         */
        u32 getPlayerIndex() const override;

        /**
         * @brief Get the joystick identifier for this GameInput.
         *
         * @return u32 Joystick id
         */
        u32 getJoystickId() const override;

        /**
         * @brief Set the joystick identifier for this GameInput.
         *
         * @param joystickId Joystick id to associate with this input
         */
        void setJoystickId( u32 joystickId ) override;

        /**
         * @brief Query whether the dongle (wireless receiver) is ready.
         *
         * @return true if the dongle is ready and connected
         */
        bool isDongleReady() const;

        /**
         * @brief Set the dongle readiness flag.
         *
         * @param ready True when the dongle is ready/connected
         */
        void setDongleReady( bool ready );

        /**
         * @brief Query whether input processing is ready for this device.
         *
         * @return true if input is ready
         */
        bool isInputReady() const;

        /**
         * @brief Set the general input readiness flag.
         *
         * @param ready True when input processing for this device is enabled
         */
        void setInputReady( bool ready );

        /**
         * @brief Check if keyboard input is enabled for this GameInput.
         *
         * @return true if keyboard input is allowed
         */
        bool isKeyboardInputEnabled() const;

        /**
         * @brief Enable or disable keyboard input for this GameInput.
         *
         * @param enabled True to enable keyboard input, false to disable
         */
        void setKeyboardInputEnabled( bool enabled );

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Atomic flag indicating whether a wireless dongle/receiver is ready.
        atomic_bool m_dongleReady;

        /// Atomic flag indicating whether input processing is ready/enabled.
        atomic_bool m_inputReady;

        /// Atomic flag that controls whether keyboard input should be accepted.
        atomic_bool m_keyboardInputEnabled;
    };

}  // namespace workphone

#endif  // GameInput_h__
