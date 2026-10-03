#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageText.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageText, StateMessage );

    StateMessageText::StateMessageText() = default;
    StateMessageText::~StateMessageText() = default;

    auto StateMessageText::getText() const -> String
    {
        return m_text;
    }

    void StateMessageText::setText( const String &value )
    {
        m_text = value;
    }
}  // namespace workphone
