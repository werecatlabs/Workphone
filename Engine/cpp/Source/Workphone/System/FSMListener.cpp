#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/FSMListener.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, FSMListener, IFSMListener );

    FSMListener::FSMListener() = default;

    FSMListener::~FSMListener()
    {
        unload( nullptr );
    }

    void FSMListener::load( SmartPtr<ISharedObject> data )
    {
    }

    void FSMListener::unload( SmartPtr<ISharedObject> data )
    {
        m_fsm = nullptr;
    }

    auto FSMListener::handleEvent( [[maybe_unused]] u32 state, [[maybe_unused]] FSMEvent eventType )
        -> FSMReturnType
    {
        return FSMReturnType::Ok;
    }

    auto FSMListener::getFSM() const -> SmartPtr<IFSM>
    {
        auto fsm = m_fsm.lock();
        return fsm;
    }

    void FSMListener::setFSM( SmartPtr<IFSM> fsm )
    {
        m_fsm = fsm;
    }

}  // namespace workphone
