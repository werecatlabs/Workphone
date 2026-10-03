#ifndef StateMessageVisible_h__
#define StateMessageVisible_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageVisible : public StateMessage
    {
    public:
        StateMessageVisible() = default;
        ~StateMessageVisible() override = default;

        bool isVisible() const;
        void setVisible( bool value );

        bool getCascade() const;
        void setCascade( bool value );

        WP_CLASS_REGISTER_DECL;

    protected:
        bool m_isVisible = true;
        bool m_cascade = true;
    };
}  // namespace workphone

#endif  // StateMessageVisible_h__
