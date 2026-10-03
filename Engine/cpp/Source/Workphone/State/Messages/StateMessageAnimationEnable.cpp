#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageAnimationEnable.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageAnimationEnable, StateMessage );

    StateMessageAnimationEnable::StateMessageAnimationEnable() = default;

    StateMessageAnimationEnable::~StateMessageAnimationEnable() = default;

    auto StateMessageAnimationEnable::getName() const -> String
    {
        return m_name;
    }

    void StateMessageAnimationEnable::setName( const String &name )
    {
        m_name = name;
    }

    auto StateMessageAnimationEnable::getTime() const -> f32
    {
        return m_time;
    }

    void StateMessageAnimationEnable::setTime( f32 time )
    {
        m_time = time;
    }

    auto StateMessageAnimationEnable::getEnabled() const -> bool
    {
        return m_isEnabled;
    }

    void StateMessageAnimationEnable::setEnabled( bool enabled )
    {
        m_isEnabled = enabled;
    }
}  // namespace workphone
