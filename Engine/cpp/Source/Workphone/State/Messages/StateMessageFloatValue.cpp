#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageFloatValue.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageFloatValue, StateMessage );

    const hash_type StateMessageFloatValue::LEFT_HASH = StringUtil::getHash( "left" );
    const hash_type StateMessageFloatValue::TOP_HASH = StringUtil::getHash( "top" );
    const hash_type StateMessageFloatValue::WIDTH_HASH = StringUtil::getHash( "width" );
    const hash_type StateMessageFloatValue::HEIGHT_HASH = StringUtil::getHash( "height" );

    //--------------------------------------------
    auto StateMessageFloatValue::getValue() const -> f32
    {
        return m_value;
    }

    //--------------------------------------------
    void StateMessageFloatValue::setValue( f32 value )
    {
        m_value = value;
    }
}  // namespace workphone
