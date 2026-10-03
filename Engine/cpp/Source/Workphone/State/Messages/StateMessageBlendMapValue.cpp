#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageBlendMapValue.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageBlendMapValue, StateMessage );

    StateMessageBlendMapValue::StateMessageBlendMapValue() :
        m_coordinates( Vector2I::zero() ),
        m_blendValue( 0.f )
    {
    }

    StateMessageBlendMapValue::~StateMessageBlendMapValue() = default;

    auto StateMessageBlendMapValue::getCoordinates() const -> Vector2I
    {
        return m_coordinates;
    }

    void StateMessageBlendMapValue::setCoordinates( const Vector2I &value )
    {
        m_coordinates = value;
    }

    auto StateMessageBlendMapValue::getBlendValue() const -> f32
    {
        return m_blendValue;
    }

    void StateMessageBlendMapValue::setBlendValue( f32 value )
    {
        m_blendValue = value;
    }
}  // namespace workphone
