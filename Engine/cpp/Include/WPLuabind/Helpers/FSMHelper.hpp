#ifndef FSMHelper_h__
#define FSMHelper_h__

#include "WPLuabind/WPLuabindPrerequisites.hpp"

namespace workphone
{
    class FSMHelper
    {
    public:
        static void        setFSMState( IFSM *fsm, lua_Integer state );
        static lua_Integer getFSMState( IFSM *fsm );
        static void        setFSMInitialState( IFSM *fsm, lua_Integer state );
        static void        _updateObject( IFSM *object, lua_Number t, lua_Number dt );
        static void        setFSMStateChangeNow( IFSM *fsm, lua_Integer state, bool changeNow );
    };
} // namespace workphone

#endif // FSMHelper_h__
