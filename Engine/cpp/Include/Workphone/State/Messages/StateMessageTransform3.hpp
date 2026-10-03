#ifndef StateMessageTransform3_h__
#define StateMessageTransform3_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    class WPCore_API StateMessageTransform3 : public StateMessage
    {
    public:
        StateMessageTransform3();
        ~StateMessageTransform3() override;

        Transform3<real_Num> getTransform() const;
        void setTransform( const Transform3<real_Num> &transform );

        WP_CLASS_REGISTER_DECL;

    private:
        Transform3<real_Num> m_transform;
    };
}  // namespace workphone

#endif  // StateMessageTransform3_h__
