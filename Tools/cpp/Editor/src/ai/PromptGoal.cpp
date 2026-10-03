#include <EditorPCH.hpp>
#include <ai/PromptGoal.hpp>
#include <Workphone/Workphone.hpp>
#include <utility>

namespace workphone::editor
{
    PromptGoal::PromptGoal( String label, const Array<String> &tags ) : m_tags( tags ), m_label( label )
    {
    }

    PromptGoal::PromptGoal() = default;

    void PromptGoal::start()
    {
    }

    void PromptGoal::finish()
    {
    }

    void PromptGoal::setState( u32 state )
    {
    }

    u32 PromptGoal::getState() const
    {
        return 0;
    }

    u32 PromptGoal::getType() const
    {
        return 0;
    }

    void PromptGoal::setType( u32 type )
    {
    }

    void PromptGoal::setLabel( const String &label )
    {
        m_label = label;
    }

    String PromptGoal::getLabel() const
    {
        return m_label;
    }

    void PromptGoal::setTags( const Array<String> &tags )
    {
        m_tags = tags;
    }

    Array<String> PromptGoal::getTags() const
    {
        return m_tags;
    }
}  // namespace workphone::editor
