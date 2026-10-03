#ifndef AddPromptEvaluator_h__
#define AddPromptEvaluator_h__

#include "ai/PromptEvaluator.hpp"

namespace workphone
{
    namespace editor
    {
        class AddPromptEvaluator : public PromptEvaluator
        {
        public:
            AddPromptEvaluator();

            AddPromptEvaluator( AddActorCmd::ActorType actorType, const String &label,
                                const Array<String> &tags );

            ~AddPromptEvaluator() override;

            void activateGoal() override;

            f32 getRating() override;

            SmartPtr<ISharedObject> getOwner() const override;

            void setOwner( SmartPtr<ISharedObject> owner ) override;

            f32 getBias() const override;

            void setBias( f32 bias ) override;

            AddActorCmd::ActorType getActorType() const;

            void setActorType( AddActorCmd::ActorType actorType );

        protected:
            String m_className;
            SmartPtr<scene::IGameActor> m_owner;
            AddActorCmd::ActorType m_actorType = AddActorCmd::ActorType::Actor;
            f32 m_bias = 0.0f;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // AddPromptEvaluator_h__
