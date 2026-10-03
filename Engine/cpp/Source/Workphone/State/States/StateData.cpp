#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/State/States/StateData.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, StateData, ISharedObject );

    StateData::StateData() : ISharedObject( StateData::typeInfo() )
    {
    }

    StateData::StateData( const StateData &other )
    {
    }

    StateData::StateData( u32 poolTypeId ) : ISharedObject( poolTypeId )
    {
    }

    StateData::~StateData()
    {
    }

    void StateData::lock()
    {
        m_mutex.lock();
    }

    void StateData::unlock()
    {
        m_mutex.unlock();
    }

    void StateData::lock_shared()
    {
        m_mutex.lock_shared();
    }

    void StateData::unlock_shared()
    {
        m_mutex.unlock_shared();
    }

}  // namespace workphone
