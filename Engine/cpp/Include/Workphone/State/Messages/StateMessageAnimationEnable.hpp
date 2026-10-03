#ifndef StateMessageAnimationEnable_h__
#define StateMessageAnimationEnable_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{

    class WPCore_API StateMessageAnimationEnable : public StateMessage
    {
    public:
        StateMessageAnimationEnable();
        ~StateMessageAnimationEnable() override;

        String getName() const override;
        void setName( const String &name ) override;

        f32 getTime() const;
        void setTime( f32 time );

        bool getEnabled() const;
        void setEnabled( bool enabled );

        WP_CLASS_REGISTER_DECL;

    protected:
        String m_name;
        f32 m_time = 0.0f;
        bool m_isEnabled;
    };
}  // namespace workphone

#endif  // StateMessageAnimationEnable_h__
