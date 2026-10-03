#include "WPPythonBind/WPPythonBindPCH.hpp"
#include "WPPythonBind/Helpers\FSMHelper.hpp"
#include <Workphone/Workphone.hpp>

namespace fb
{

    void FSMHelper::setFSMState( SmartPtr<IFSM> fsm, python_Integer state )
    {
        fsm->setState( (u32)state );
    }

    fb::python_Integer FSMHelper::getFSMState( SmartPtr<IFSM> fsm )
    {
        return 0;//(python_Integer)fsm->getState();
    }

    void FSMHelper::setFSMInitialState( SmartPtr<IFSM> fsm, python_Integer state )
    {
        //fsm->setInitialState( (u32)state );
    }

    //fb::SmartPtr<IFSM> FSMHelper::_getFSM( FSMContainerPtr container, python_Integer id )
    //{
    //    return container->getFSM( id );
    //}

    //fb::SmartPtr<IFSM> FSMHelper::_getFSMByName( FSMContainerPtr container, const char *name )
    //{
    //    hash32 hash = StringUtil::getHash( name );
    //    return container->getFSM( hash );
    //}

}  // namespace fb
