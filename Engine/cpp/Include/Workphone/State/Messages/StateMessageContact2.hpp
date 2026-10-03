#ifndef StateMessageContact2_h__
#define StateMessageContact2_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    class WPCore_API StateMessageContact2 : public StateMessage
    {
    public:
        StateMessageContact2();
        ~StateMessageContact2() override;

        hash32 getContactType() const;
        void setContactType( hash32 value );

        Vector2<real_Num> getPosition() const;
        void setPosition( const Vector2<real_Num> &value );

        Vector2<real_Num> getNormal() const;
        void setNormal( const Vector2<real_Num> &value );

        WP_CLASS_REGISTER_DECL;

    protected:
        hash32 m_contactType;
        Vector2<real_Num> m_position;
        Vector2<real_Num> m_normal;
    };
}  // namespace workphone

#endif  // StateMessageContact2_h__
