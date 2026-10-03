#ifndef StateMessagePosition4_h__
#define StateMessagePosition4_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Math/Vector4.hpp>

namespace workphone
{

    class WPCore_API StateMessageVector4 : public StateMessage
    {
    public:
        StateMessageVector4();
        explicit StateMessageVector4( const Vector4F &position );
        ~StateMessageVector4() override;

        Vector4F getValue() const;
        void setValue( const Vector4F &value );

        WP_CLASS_REGISTER_DECL;

    protected:
        Vector4F m_position;
    };
}  // namespace workphone

#endif  // StateMessagePosition4_h__
