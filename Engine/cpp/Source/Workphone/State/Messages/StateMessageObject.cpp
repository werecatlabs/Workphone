#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/Messages/StateMessageObject.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateMessageObject, StateMessage );

    StateMessageObject::StateMessageObject() = default;

    StateMessageObject::~StateMessageObject() = default;

    void StateMessageObject::unload( SmartPtr<ISharedObject> data )
    {
        std::fprintf( stderr, "TRACE StateMessageObject unload %p payload before %p\n", this,
                      m_object.get() );
        StateMessage::unload( data );

        // The pointee may already have been reclaimed by its factory by the time a
        // queued state message is drained. Clear the raw weak reference without
        // touching that potentially stale pooled object.
        m_object.forceReset();
        std::fprintf( stderr, "TRACE StateMessageObject unload %p payload after %p\n", this,
                      m_object.get() );
    }

    auto StateMessageObject::getObject() const -> SmartPtr<ISharedObject>
    {
        WP_ASSERT( workphone::dynamic_pointer_cast<State>( m_object.load().lock() ) == nullptr );
        return m_object.load().lock();
    }

    void StateMessageObject::setObject( SmartPtr<ISharedObject> object )
    {
        WP_ASSERT( workphone::dynamic_pointer_cast<State>( object ) == nullptr );
        m_object = WeakPtr<ISharedObject>( object );
    }
}  // namespace workphone
