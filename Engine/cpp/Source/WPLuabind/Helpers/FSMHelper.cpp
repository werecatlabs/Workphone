#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Helpers/FSMHelper.hpp"
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    void FSMHelper::setFSMState( IFSM *fsm, lua_Integer state )
    {
        fsm->setState( static_cast<u32>( state ) );
    }

    void FSMHelper::setFSMStateChangeNow( IFSM *fsm, lua_Integer state, bool changeNow )
    {
        fsm->setState( static_cast<u32>( state ), changeNow );
    }

    lua_Integer FSMHelper::getFSMState( IFSM *fsm )
    {
        return 0; //(lua_Integer)fsm->getState();
    }

    void FSMHelper::setFSMInitialState( IFSM *fsm, lua_Integer state )
    {
        // fsm->setInitialState( (u32)state );
    }

    void FSMHelper::_updateObject( IFSM *object, lua_Number t, lua_Number dt )
    {
        // object->update( Thread::TASK_ID_APPLICATION_LOGIC, t, dt );
    }
} // namespace workphone
