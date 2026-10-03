#ifndef __GameInputState_h__
#define __GameInputState_h__

#include <Workphone/Interface/Input/IGameInputState.hpp>

/**
 * @file GameInputState.hpp
 * @brief Simple data container for game input events and mapped actions.
 */

namespace workphone
{
    /**
     * @brief Represents a high-level game input event mapping.
     *
     * GameInputState stores a hashed event type and the corresponding action
     * hash. It is used to represent abstracted input events (e.g., "jump",
     * "fire") after raw input has been mapped by the input system.
     */
    class WPCore_API GameInputState : public IGameInputState
    {
    public:
        /**
         * @brief Create an empty GameInputState.
         */
        GameInputState();

        /**
         * @brief Virtual destructor.
         */
        ~GameInputState() override;

        /**
         * @brief Get the hashed event type for this state.
         *
         * @return hash32 Event type hash
         */
        hash32 getEventType() const override;

        /**
         * @brief Set the hashed event type for this state.
         *
         * @param eventType Event type hash
         */
        void setEventType( hash32 eventType ) override;

        /**
         * @brief Get the action hash mapped to this event.
         *
         * @return hash32 Action hash
         */
        hash32 getAction() const override;

        /**
         * @brief Set the action hash to associate with this event.
         *
         * @param action Action hash
         */
        void setAction( hash32 action ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Hashed event type identifier.
        hash32 m_eventType = 0;
        /// Hashed action identifier associated with the event.
        hash32 m_action = 0;
    };
}  // namespace workphone

#endif  // FBGameInputState_h__
