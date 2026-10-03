#ifndef PromptGoal_h__
#define PromptGoal_h__

#include <Workphone/Interface/AI/IAiGoal.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace editor
    {
        class PromptGoal : public IAiGoal
        {
        public:
            PromptGoal();

            PromptGoal( String label, const Array<String> &tags );

            void start() override;

            void finish() override;

            void setState( u32 state ) override;

            u32 getState() const override;

            u32 getType() const override;

            void setType( u32 type ) override;

            Array<String> getTags() const;

            void setTags( const Array<String> &tags );

            String getLabel() const;

            void setLabel( const String &label );

        protected:
            Array<String> m_tags;
            String m_label;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // PromptGoal_h__
