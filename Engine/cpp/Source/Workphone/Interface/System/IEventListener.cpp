#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, IEventListener, ISharedObject );

    IEventListener::IEventListener()
    {
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    IEventListener::~IEventListener() = default;

    s32 IEventListener::getPriority() const
    {
        return m_eventListenerPriority;
    }

    void IEventListener::setPriority( s32 priority )
    {
        m_eventListenerPriority = priority;
    }
}  // namespace workphone
