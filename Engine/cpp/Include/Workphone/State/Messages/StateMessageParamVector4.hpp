#ifndef StateMessageParamVector4_h__
#define StateMessageParamVector4_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Math/Vector4.hpp>

namespace workphone
{

    class WPCore_API StateMessageParamVector4 : public StateMessage
    {
    public:
        StateMessageParamVector4();
        ~StateMessageParamVector4() override;

        hash32 getId() const;
        void setId( hash32 value );

        Vector4F getValue() const;
        void setValue( const Vector4F &value );

        WP_CLASS_REGISTER_DECL;

    protected:
        hash32 m_id;
        Vector4F m_value;
    };
}  // namespace workphone

#endif  // StateMessageParamVector4_h__
