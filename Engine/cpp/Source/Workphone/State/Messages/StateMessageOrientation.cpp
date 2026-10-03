#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageOrientation.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageOrientation, StateMessage );

    void StateMessageOrientation::setOrientation( const Quaternion<real_Num> &value )
    {
        m_orientation = value;
    }

    auto StateMessageOrientation::getOrientation() const -> Quaternion<real_Num>
    {
        return m_orientation;
    }
}  // namespace workphone
