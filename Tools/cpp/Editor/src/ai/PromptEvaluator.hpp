#ifndef PromptEvaluator_h__
#define PromptEvaluator_h__

#include <Workphone/Interface/AI/IAiGoalEvaluator.hpp>
#include "commands/AddActorCmd.hpp"
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace editor
    {
        class PromptEvaluator : public IAiGoalEvaluator
        {
        public:
            PromptEvaluator();

            PromptEvaluator( const String &label, const Array<String> &tags );

            ~PromptEvaluator() override;

            void activateGoal() override;

            f32 getRating() override;

            SmartPtr<ISharedObject> getOwner() const override;

            void setOwner( SmartPtr<ISharedObject> owner ) override;

            f32 getBias() const override;

            void setBias( f32 bias ) override;

            Array<String> getNamedEntities() const;

            void setNamedEntities( const Array<String> &namedEntities );

            Array<String> getTags() const;

            void setTags( const Array<String> &tags );

            String getLabel() const;

            void setLabel( const String &label );

        protected:
            Array<String> m_namedEntities;
            Array<String> m_tags;
            String m_label;

            String m_className;
            SmartPtr<scene::IGameActor> m_owner;
            f32 m_bias = 0.0f;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // PromptEvaluator_h__
