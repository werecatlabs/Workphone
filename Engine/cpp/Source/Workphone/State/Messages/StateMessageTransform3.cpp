#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageTransform3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageTransform3, StateMessage );

    StateMessageTransform3::StateMessageTransform3() = default;

    StateMessageTransform3::~StateMessageTransform3() = default;

    auto StateMessageTransform3::getTransform() const -> Transform3<real_Num>
    {
        return m_transform;
    }

    void StateMessageTransform3::setTransform( const Transform3<real_Num> &transform )
    {
        m_transform = transform;
    }
}  // namespace workphone
