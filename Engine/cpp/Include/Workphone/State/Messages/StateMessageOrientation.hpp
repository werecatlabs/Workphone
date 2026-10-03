#ifndef StateMessageOrientation_h__
#define StateMessageOrientation_h__

#include <Workphone/State/Messages/StateMessage.hpp>
#include "Workphone/Math/Quaternion.hpp"

namespace workphone
{

    class WPCore_API StateMessageOrientation : public StateMessage
    {
    public:
        StateMessageOrientation() = default;
        ~StateMessageOrientation() override = default;

        Quaternion<real_Num> getOrientation() const;
        void setOrientation( const Quaternion<real_Num> &value );

        WP_CLASS_REGISTER_DECL;

    protected:
        Quaternion<real_Num> m_orientation;
    };
}  // namespace workphone

#endif  // StateMessageOrientation_h__
