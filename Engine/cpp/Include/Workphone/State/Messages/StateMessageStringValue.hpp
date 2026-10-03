#ifndef StateMessageStringValue_h__
#define StateMessageStringValue_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include "Workphone/Core/StringTypes.hpp"

namespace workphone
{

    class WPCore_API StateMessageStringValue : public StateMessage
    {
    public:
        StateMessageStringValue();
        ~StateMessageStringValue() override;

        String getValue() const;
        void setValue( const String &value );

        WP_CLASS_REGISTER_DECL;

    protected:
        String m_value;
    };
}  // namespace workphone

#endif  // StateMessageStringValue_h__
