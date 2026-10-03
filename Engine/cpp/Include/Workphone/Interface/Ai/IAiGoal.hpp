#ifndef IAiGoal_h__
#define IAiGoal_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for an AI goal. Inherits from ISharedObject.
     */
    class WPCore_API IAiGoal : public ISharedObject
    {
    public:
        /**
         * @brief Enum representing the possible states of an AI goal.
         */
        enum class State
        {
            None,
            Ready,
            Executing,
            Finished,
            Failed,

            Count
        };

        /**
         * @brief Destroy the IAiGoal object.
         */
        ~IAiGoal() override;

        /**
         * @brief Start the AI goal. Called when the AI begins using this goal.
         */
        virtual void start() = 0;

        /**
         * @brief Finish the AI goal. Called when the goal has been completed or terminated.
         */
        virtual void finish() = 0;

        /**
         * @brief Get the FSM for the AI goal.
         * @return SmartPtr<IFSM> The FSM for the AI goal.
         */
        virtual const SmartPtr<IFSM> getFSM() const = 0;

        /**
         * @brief Set the FSM for the AI goal.
         * @param fsm The FSM to set for the AI goal.
         */
        virtual void setFSM( SmartPtr<IFSM> fsm ) = 0;

        /**
         * @brief Set the state of the AI goal.
         * @param state The desired state for the AI goal.
         */
        virtual void setState( u32 state ) = 0;

        /**
         * @brief Get the current state of the AI goal.
         * @return u32 The current state of the AI goal.
         */
        virtual u32 getState() const = 0;

        /**
         * @brief Get the type of the AI goal.
         * @return u32 An integer representing the goal type.
         */
        virtual u32 getType() const = 0;

        /**
         * @brief Set the type of the AI goal.
         * @param type An integer representing the goal type.
         */
        virtual void setType( u32 type ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAiGoal_h__
