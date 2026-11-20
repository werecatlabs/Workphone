#ifndef GameInput_h__
#define GameInput_h__

#include <Workphone/Interface/Input/IGameInput.hpp>

/**
 * @file GameInput.hpp
 * @brief Concrete game input: player/joystick assignment, input map and readiness flags.
 */

namespace workphone
{

    /**
     * @class GameInput
     * @brief Concrete IGameInput describing one player's input source (keyboard, joystick, dongle).
     *
     * GameInput stores the player index, the joystick id and the GameInputMap that translates raw
     * keys/buttons into game actions. It also holds atomic readiness flags (dongle, general input,
     * keyboard) that can be read and written from different threads.
     *
     * Defaults: player index 0, joystick id 0, input ready, keyboard input enabled, dongle not
     * ready.
     */
    class WPCore_API GameInput : public IGameInput
    {
    public:
        /**
         * @brief Constructs a GameInput with an empty GameInputMap, input ready, keyboard input
         *        enabled and the dongle not ready.
         */
        GameInput();

        /**
         * @brief Virtual destructor.
         */
        ~GameInput() override;

        /**
         * @copydoc IGameInput::isAssigned
         * @note This implementation reports the input-ready flag (see isInputReady()).
         */
        bool isAssigned() const override;

        /**
         * @copydoc IGameInput::getGameInputMap
         */
        SmartPtr<IGameInputMap> getGameInputMap() const override;

        /**
         * @copydoc IGameInput::setPlayerIndex
         */
        void setPlayerIndex( u32 playerIndex ) override;

        /**
         * @copydoc IGameInput::getPlayerIndex
         */
        u32 getPlayerIndex() const override;

        /**
         * @copydoc IGameInput::getJoystickId
         */
        u32 getJoystickId() const override;

        /**
         * @copydoc IGameInput::setJoystickId
         */
        void setJoystickId( u32 joystickId ) override;

        /**
         * @brief Queries whether the wireless dongle (receiver) is ready.
         *
         * @return true if the dongle is ready and connected
         * @note Thread-safe (backed by an atomic flag).
         */
        bool isDongleReady() const;

        /**
         * @brief Sets the dongle readiness flag.
         *
         * @param ready true when the dongle is ready/connected
         * @note Thread-safe (backed by an atomic flag).
         */
        void setDongleReady( bool ready );

        /**
         * @brief Queries whether input processing is ready for this device.
         *
         * @return true if input is ready (the default)
         * @note Thread-safe (backed by an atomic flag).
         */
        bool isInputReady() const;

        /**
         * @brief Sets the general input readiness flag.
         *
         * @param ready true when input processing for this device is enabled
         * @note Thread-safe (backed by an atomic flag).
         */
        void setInputReady( bool ready );

        /**
         * @brief Queries whether keyboard input is enabled for this GameInput.
         *
         * @return true if keyboard input is accepted (the default)
         * @note Thread-safe (backed by an atomic flag).
         */
        bool isKeyboardInputEnabled() const;

        /**
         * @brief Enables or disables keyboard input for this GameInput.
         *
         * @param enabled true to enable keyboard input, false to disable
         * @note Thread-safe (backed by an atomic flag).
         */
        void setKeyboardInputEnabled( bool enabled );

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Atomic flag indicating whether the wireless dongle/receiver is ready.
        atomic_bool m_dongleReady;

        /// Atomic flag indicating whether input processing is ready/enabled.
        atomic_bool m_inputReady;

        /// Atomic flag that controls whether keyboard input is accepted.
        atomic_bool m_keyboardInputEnabled;
        /// Input map translating keys/buttons into game actions (created in the constructor).
        SmartPtr<IGameInputMap> m_gameInputMap;
        /// Player index (m_playerIndex) and joystick id (m_joystickId); both default to 0.
        u32 m_playerIndex = 0, m_joystickId = 0;
    };

}  // namespace workphone

#endif  // GameInput_h__
