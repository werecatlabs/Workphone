#ifndef __GameInputState_h__
#define __GameInputState_h__

#include <Workphone/Interface/Input/IGameInputState.hpp>

/**
 * @file GameInputState.hpp
 * @brief Simple data container holding the event type and action of a game input event.
 */

namespace workphone
{
    /**
     * @brief Represents a high-level game input event.
     *
     * GameInputState stores a hashed event type and the hash of the action
     * that was triggered. It is used to pass abstracted input events (e.g.
     * "jump", "fire") through the input system after raw input has been
     * mapped to game actions.
     *
     * @note Instances are plain data holders and are not thread-safe.
     */
    class WPCore_API GameInputState : public IGameInputState
    {
    public:
        /**
         * @brief Create an empty GameInputState.
         *
         * Both the event type and the action hash are initialized to 0.
         */
        GameInputState();

        /**
         * @brief Virtual destructor.
         */
        ~GameInputState() override;

        /**
         * @brief Get the hashed event type for this state.
         *
         * @return hash32 Event type hash (default 0)
         */
        hash32 getEventType() const override;

        /**
         * @brief Set the hashed event type for this state.
         *
         * @param eventType Event type hash
         */
        void setEventType( hash32 eventType ) override;

        /**
         * @brief Get the hash of the game input action that was triggered.
         *
         * @return hash32 Action hash (default 0)
         */
        hash32 getAction() const override;

        /**
         * @brief Set the hash of the game input action that was triggered.
         *
         * @param action Action hash
         */
        void setAction( hash32 action ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Hash of the event type, initially 0.
        hash32 m_eventType = 0;
        /// Hash of the action that was triggered, initially 0.
        hash32 m_action = 0;
    };
}  // namespace workphone

#endif  // FBGameInputState_h__
