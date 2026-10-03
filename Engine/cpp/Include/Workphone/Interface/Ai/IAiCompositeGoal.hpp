#ifndef IAiCompositeGoal_h__
#define IAiCompositeGoal_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Ai/IAiGoal.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @brief Interface for a composite AI goal, which can contain multiple subgoals. Inherits from
     * IAiGoal.
     */
    class WPCore_API IAiCompositeGoal : public IAiGoal
    {
    public:
        /**
         * @brief Destroy the IAiCompositeGoal object.
         */
        ~IAiCompositeGoal() override;

        /**
         * @brief Add a subgoal to the composite goal.
         * @param goal A smart pointer to the subgoal to be added.
         */
        virtual void addGoal( SmartPtr<IAiGoal> goal ) = 0;

        /**
         * @brief Remove a subgoal from the composite goal.
         * @param goal A smart pointer to the subgoal to be removed.
         * @return bool True if the subgoal was removed, false otherwise.
         */
        virtual bool removeGoal( SmartPtr<IAiGoal> goal ) = 0;

        /**
         * @brief Get an array of subgoals in the composite goal.
         * @return Array<SmartPtr<IAiGoal>> An array of smart pointers to subgoals.
         */
        virtual Array<SmartPtr<IAiGoal>> getGoals() const = 0;

        /**
         * @brief Remove all subgoals from the composite goal.
         */
        virtual void removeAllGoals() = 0;

        /**
         * @brief Remove subgoals of a specific type from the composite goal.
         * @param goalType The type of subgoals to remove.
         */
        virtual void removeGoalsByType( u32 goalType ) = 0;

        /**
         * @brief Check if the composite goal has any subgoals of a specific type.
         * @param goalType The type of subgoals to check for.
         * @return bool True if the composite goal has subgoals of the specified type, false otherwise.
         */
        virtual bool hasGoalType( u32 goalType ) const = 0;

        /**
         * @brief Add a goal evaluator to the composite goal.
         * @param evaluator A smart pointer to the goal evaluator to be added.
         */
        virtual void addGoalEvaluator( SmartPtr<IAiGoalEvaluator> evaluator ) = 0;

        /**
         * @brief Remove a goal evaluator from the composite goal.
         * @param evaluator A smart pointer to the goal evaluator to be removed.
         * @return bool True if the goal evaluator was removed, false otherwise.
         */
        virtual bool removeGoalEvaluator( SmartPtr<IAiGoalEvaluator> evaluator ) = 0;

        /**
         * @brief Get an array of goal evaluators associated with the composite goal.
         * @return An array of smart pointers to goal evaluators.
         */
        virtual Array<SmartPtr<IAiGoalEvaluator>> getEvaluators() const = 0;

        /**
         * @brief Check if the composite goal has any subgoals.
         * @return bool True if the composite goal has subgoals, false otherwise.
         */
        virtual bool hasGoals() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IAiCompositeGoal_h__
