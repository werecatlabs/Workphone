#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/FSMBind.hpp"
#include <luabind/luabind.hpp>
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include "WPLuabind/Helpers/FSMContainerHelper.hpp"
#include "WPLuabind/Helpers/FSMHelper.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    void bindFSM( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IFSM, ISharedObject, SmartPtr<ISharedObject>>( "IFSM" )
                        .def( "getFsmManager", &IFSM::getFsmManager )
                        .def( "setFsmManager", &IFSM::setFsmManager )
                        .def( "getStateTime", &IFSM::getStateTime )
                        .def( "setStateTime", &IFSM::setStateTime )
                        .def( "getStateTimeElapsed", &IFSM::getStateTimeElapsed )
                        .def( "setState", &IFSM::setState<int> )
                        .def( "getState", &IFSM::getState<int> )
                        .def( "getPreviousState", &IFSM::getPreviousState )
                        .def( "getCurrentState", &IFSM::getCurrentState )
                        .def( "getNewState", &IFSM::getNewState )
                        .def( "setNewState", &IFSM::setNewState )
                        .def( "isPending", &IFSM::isPending )
                        .def( "addListener", &IFSM::addListener )
                        .def( "removeListener", &IFSM::removeListener )
                        .def( "getStateTicks",
                              static_cast<s32 ( IFSM::* )( TaskId ) const>( &IFSM::getStateTicks ) )
                        .def( "getStateTicks",
                              static_cast<s32 ( IFSM::* )() const>( &IFSM::getStateTicks ) )];

        module( L )[class_<IFSMManager, ISharedObject, SmartPtr<ISharedObject>>( "IFSMManager" )
                        .def( "createFSM", &IFSMManager::createFSM )
                        .def( "destroyFSM", &IFSMManager::destroyFSM )
                        .def( "getStateTime", &IFSMManager::getStateTime )
                        .def( "setStateTime", &IFSMManager::setStateTime )
                        .def( "getStateChangeTime", &IFSMManager::getStateChangeTime )
                        .def( "setStateChangeTime", &IFSMManager::setStateChangeTime )
                        .def( "getPreviousState", &IFSMManager::getPreviousState )
                        .def( "getCurrentState", &IFSMManager::getCurrentState )
                        .def( "getNewState", &IFSMManager::getNewState )
                        .def( "setNewState", &IFSMManager::setNewState )
                        .def( "addListener", &IFSMManager::addListener )
                        .def( "removeListener", &IFSMManager::removeListener )
                        .def( "removeListeners", &IFSMManager::removeListeners )
                        .def( "getListenerPriority", &IFSMManager::getListenerPriority )
                        .def( "setListenerPriority", &IFSMManager::setListenerPriority )
                        .def( "getFlagsPtr", &IFSMManager::getFlagsPtr )
                        .def( "getListeners", &IFSMManager::getListeners )];

        module( L )[class_<IFSMListener, IObject, SmartPtr<IObject>>( "IFSMListener" )
                        .def( "getFSM", &IFSMListener::getFSM )
                        .def( "setFSM", &IFSMListener::setFSM )];
    }
} // namespace workphone
