#ifndef IAiGoalEvaluator_h__
#define IAiGoalEvaluator_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for an AI goal evaluator. Inherits from ISharedObject.
     */
    class WPCore_API IAiGoalEvaluator : public ISharedObject
    {
    public:
        /**
         * @brief Destroy the IAiGoalEvaluator object.
         */
        ~IAiGoalEvaluator() override;

        /**
         * @brief Activate the AI goal associated with this evaluator.
         */
        virtual void activateGoal() = 0;

        /**
         * @brief Get the rating of the AI goal.
         * @return f32 The rating of the AI goal.
         */
        virtual f32 getRating() = 0;

        /**
         * @brief Get the owner of the AI goal evaluator.
         * @return A smart pointer to the owner.
         */
        virtual SmartPtr<ISharedObject> getOwner() const = 0;

        /**
         * @brief Set the owner of the AI goal evaluator.
         * @param owner A smart pointer to the owner.
         */
        virtual void setOwner( SmartPtr<ISharedObject> owner ) = 0;

        /**
         * @brief Get the bias value of the AI goal evaluator.
         * @return f32 The bias value.
         */
        virtual f32 getBias() const = 0;

        /**
         * @brief Set the bias value of the AI goal evaluator.
         * @param bias The bias value to set.
         */
        virtual void setBias( f32 bias ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAiGoalEvaluator_h__
