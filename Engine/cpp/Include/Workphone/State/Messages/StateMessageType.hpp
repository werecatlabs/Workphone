#ifndef StateMessageType_h__
#define StateMessageType_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageType : public StateMessage
    {
    public:
        StateMessageType();
        ~StateMessageType() override;

        u32 getTypeValue() const;
        void setTypeValue( u32 typeValue );

        WP_CLASS_REGISTER_DECL;

    protected:
        u32 m_typeValue = 0;
    };
}  // namespace workphone

#endif  // StateMessageType_h__
