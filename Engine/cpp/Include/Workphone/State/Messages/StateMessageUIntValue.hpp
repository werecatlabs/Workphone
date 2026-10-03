#ifndef StateMessageUIntValue_h__
#define StateMessageUIntValue_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageUIntValue : public StateMessage
    {
    public:
        StateMessageUIntValue() = default;
        ~StateMessageUIntValue() override = default;

        u32 getValue() const;
        void setValue( u32 value );

        WP_CLASS_REGISTER_DECL;

    protected:
        u32 m_value = 0;
    };
}  // namespace workphone

#endif  // StateMessageUIntValue_h__
