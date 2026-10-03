#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessage, IStateMessage );

    const hash_type StateMessage::SET_OBJECT = StringUtil::getHash( "set_object" );
    const hash_type StateMessage::SET_MESH = StringUtil::getHash( "set_mesh" );
    const hash_type StateMessage::SET_CUBEMAP = StringUtil::getHash( "set_cubemap" );
    const hash_type StateMessage::SET_TEXTURES = StringUtil::getHash( "set_textures" );

    StateMessage::StateMessage() = default;
    StateMessage::~StateMessage() = default;

    void StateMessage::unload( SmartPtr<ISharedObject> data )
    {
        m_sender = nullptr;
        m_stateContext = nullptr;
    }

    auto StateMessage::getType() const -> hash_type
    {
        return m_type;
    }

    void StateMessage::setType( hash_type type )
    {
        m_type = type;
    }

    auto StateMessage::getSender() const -> SmartPtr<ISharedObject>
    {
        return m_sender;
    }

    void StateMessage::setSender( SmartPtr<ISharedObject> object )
    {
        m_sender = object;
    }

    auto StateMessage::getStateContext() const -> SmartPtr<IStateContext>
    {
        return m_stateContext;
    }

    void StateMessage::setStateContext( SmartPtr<IStateContext> object )
    {
        m_stateContext = object;
    }

    void StateMessage::addSendCount()
    {
        ++m_sendCount;
    }

    void StateMessage::removeSendCount()
    {
        --m_sendCount;
    }

    u32 StateMessage::getSendCount() const
    {
        return m_sendCount;
    }

    void StateMessage::setSendCount( u32 sendCount )
    {
        m_sendCount = sendCount;
    }

}  // namespace workphone
