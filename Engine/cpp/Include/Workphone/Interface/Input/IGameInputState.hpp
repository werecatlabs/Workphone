#ifndef IGameInputState_h__
#define IGameInputState_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * Interface for a game input state.
     */
    class WPCore_API IGameInputState : public ISharedObject
    {
    public:
        /** Enumeration for the types of game input actions. */
        enum class Action
        {
            Chord,
            Tap,
            Sequence,
            Pressed,
            Released,
            Count
        };

        /** Virtual destructor. */
        ~IGameInputState() override;

        /** Gets the hash code for the game input event type. */
        virtual hash32 getEventType() const = 0;

        /** Sets the hash code for the game input event type. */
        virtual void setEventType( hash32 eventType ) = 0;

        /** Gets the hash code for the game input action that was triggered. */
        virtual hash32 getAction() const = 0;

        /** Sets the hash code for the game input action that was triggered. */
        virtual void setAction( hash32 action ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IGameInputState_h__
