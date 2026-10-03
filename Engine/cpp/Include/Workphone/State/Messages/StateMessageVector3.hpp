#ifndef StateMessagePosition3_h__
#define StateMessagePosition3_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{

    class WPCore_API StateMessageVector3 : public StateMessage
    {
    public:
        StateMessageVector3();
        explicit StateMessageVector3( const Vector3<real_Num> &value );
        ~StateMessageVector3() override;

        Vector3<real_Num> getValue() const;
        void setValue( const Vector3<real_Num> &value );

        WP_CLASS_REGISTER_DECL;

    protected:
        Vector3<real_Num> m_value;
    };
}  // namespace workphone

#endif  // StateMessagePosition3_h__
