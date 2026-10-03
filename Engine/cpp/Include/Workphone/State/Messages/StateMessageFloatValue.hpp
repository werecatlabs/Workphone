#ifndef StateMessageFloatValue_h__
#define StateMessageFloatValue_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageFloatValue : public StateMessage
    {
    public:
        static const hash_type LEFT_HASH;
        static const hash_type TOP_HASH;
        static const hash_type WIDTH_HASH;
        static const hash_type HEIGHT_HASH;

        StateMessageFloatValue() = default;
        ~StateMessageFloatValue() override = default;

        f32 getValue() const;
        void setValue( f32 value );

        WP_CLASS_REGISTER_DECL;

    protected:
        f32 m_value = 0.0f;
    };
}  // namespace workphone

#endif  // StateMessageFloatValue_h__
