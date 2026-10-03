#include <Workphone/WorkphonePCH.hpp>
#include "Workphone/State/Messages/StateMessageBuffer.hpp"
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageBuffer, StateMessage );

    StateMessageBuffer::StateMessageBuffer()
    {
    }

    StateMessageBuffer::~StateMessageBuffer()
    {
        if( m_buffer )
        {
            delete m_buffer;
            m_buffer = nullptr;
        }
    }

    auto StateMessageBuffer::getBuffer() const -> u8 *
    {
        return m_buffer;
    }

    void StateMessageBuffer::setBuffer( u8 *value )
    {
        m_buffer = value;
    }
}  // namespace workphone
