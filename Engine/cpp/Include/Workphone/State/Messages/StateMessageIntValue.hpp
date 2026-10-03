#ifndef StateMessageIntValue_h__
#define StateMessageIntValue_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageIntValue : public StateMessage
    {
    public:
        StateMessageIntValue() = default;
        ~StateMessageIntValue() override = default;

        s32 getValue() const;
        void setValue( s32 value );

        WP_CLASS_REGISTER_DECL;

    protected:
        s32 m_value = 0;
    };
}  // namespace workphone

#endif  // StateMessageIntValue_h__
