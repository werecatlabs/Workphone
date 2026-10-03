#ifndef StateMessageText_h__
#define StateMessageText_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    class WPCore_API StateMessageText : public StateMessage
    {
    public:
        StateMessageText();
        ~StateMessageText() override;

        String getText() const;
        void setText( const String &value );

        WP_CLASS_REGISTER_DECL;

    protected:
        String m_text;
    };

}  // namespace workphone

#endif  // StateMessageText_h__
