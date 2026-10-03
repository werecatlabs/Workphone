#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/ScriptBind.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    namespace
    {
        int getScriptFunctionReturnType( IScriptFunction *function )
        {
            return static_cast<int>( function->getReturnType() );
        }

        void setScriptFunctionReturnType( IScriptFunction *function, int type )
        {
            function->setReturnType( static_cast<ParameterType>( type ) );
        }

        int getScriptVariableType( IScriptVariable *variable )
        {
            return static_cast<int>( variable->getType() );
        }

        void setScriptVariableType( IScriptVariable *variable, int type )
        {
            variable->setType( static_cast<ParameterType>( type ) );
        }

        SmartPtr<IScriptInvoker> getScriptInvoker( IScriptObject *object )
        {
            return object->getInvoker();
        }

        SmartPtr<IScriptReceiver> getScriptReceiver( IScriptObject *object )
        {
            return object->getReceiver();
        }

        String getScriptReceiverString( IScriptReceiver *receiver, hash_type hash )
        {
            String value;
            receiver->getProperty( hash, value );
            return value;
        }

        Parameter getScriptReceiverParameter( IScriptReceiver *receiver, hash_type hash )
        {
            Parameter value;
            receiver->getProperty( hash, value );
            return value;
        }

        Parameters getScriptReceiverParameters( IScriptReceiver *receiver, hash_type hash )
        {
            Parameters values;
            receiver->getProperty( hash, values );
            return values;
        }
    } // namespace

    void bindScript( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IScript, ISharedObject, SmartPtr<IScript>>( "IScript" )
                        .def( "getClasses", &IScript::getClasses )
                        .def( "setClasses", &IScript::setClasses )
                        .scope[def( "typeInfo", IScript::typeInfo )]];

        module( L )[class_<IScriptBreakpoint, ISharedObject, SmartPtr<IScriptBreakpoint>>(
                        "IScriptBreakpoint" )
                        .def( "getLineNumber", &IScriptBreakpoint::getLineNumber )
                        .def( "setLineNumber", &IScriptBreakpoint::setLineNumber )
                        .def( "getFilePath", &IScriptBreakpoint::getFilePath )
                        .def( "setFilePath", &IScriptBreakpoint::setFilePath )
                        .scope[def( "typeInfo", IScriptBreakpoint::typeInfo )]];

        module( L )[class_<IScriptClass, ISharedObject, SmartPtr<IScriptClass>>( "IScriptClass" )
                        .def( "getClassName", &IScriptClass::getClassName )
                        .def( "setClassName", &IScriptClass::setClassName )
                        .def( "getNamespaceNames", &IScriptClass::getNamespaceNames )
                        .def( "setNamespaceNames", &IScriptClass::setNamespaceNames )
                        .def( "getFunctions", &IScriptClass::getFunctions )
                        .def( "setFunctions", &IScriptClass::setFunctions )
                        .def( "getParentClasses", &IScriptClass::getParentClasses )
                        .def( "setParentClasses", &IScriptClass::setParentClasses )
                        .def( "getHeaderIncludes", &IScriptClass::getHeaderIncludes )
                        .def( "setHeaderIncludes", &IScriptClass::setHeaderIncludes )
                        .def( "getSourceIncludes", &IScriptClass::getSourceIncludes )
                        .def( "setSourceIncludes", &IScriptClass::setSourceIncludes )
                        .scope[def( "typeInfo", IScriptClass::typeInfo )]];

        module( L )[class_<IScriptData, ISharedObject, SmartPtr<IScriptData>>( "IScriptData" )
                        .def( "getOwner", &IScriptData::getOwner )
                        .def( "setOwner", &IScriptData::setOwner )
                        .scope[def( "typeInfo", IScriptData::typeInfo )]];

        module( L )[class_<IScriptEvent, IEvent, SmartPtr<IScriptEvent>>( "IScriptEvent" )
                        .def( "getClassName", &IScriptEvent::getClassName )
                        .def( "setClassName", &IScriptEvent::setClassName )
                        .def( "getFunction", &IScriptEvent::getFunction )
                        .def( "setFunction", &IScriptEvent::setFunction )
                        .scope[def( "typeInfo", IScriptEvent::typeInfo )]];

        module(
            L )[class_<IScriptFunction, ISharedObject, SmartPtr<IScriptFunction>>( "IScriptFunction" )
                    .def( "getClassName", &IScriptFunction::getClassName )
                    .def( "setClassName", &IScriptFunction::setClassName )
                    .def( "getFunctionName", &IScriptFunction::getFunctionName )
                    .def( "setFunctionName", &IScriptFunction::setFunctionName )
                    .def( "getReturnType", getScriptFunctionReturnType )
                    .def( "setReturnType", setScriptFunctionReturnType )
                    .def( "getArguments", &IScriptFunction::getArguments )
                    .def( "setArguments", &IScriptFunction::setArguments )
                    .def( "isConstructor", &IScriptFunction::isConstructor )
                    .def( "setConstructor", &IScriptFunction::setConstructor )
                    .def( "isDestructor", &IScriptFunction::isDestructor )
                    .def( "setDestructor", &IScriptFunction::setDestructor )
                    .scope[def( "typeInfo", IScriptFunction::typeInfo )]];

        module(
            L )[class_<IScriptGenerator, ISharedObject, SmartPtr<IScriptGenerator>>( "IScriptGenerator" )
                    .scope[def( "typeInfo", IScriptGenerator::typeInfo )]];

        module(
            L )[class_<IScriptInvoker, ISharedObject, SmartPtr<IScriptInvoker>>( "IScriptInvoker" )
                    .def( "getOwner", &IScriptInvoker::getOwner )
                    .def( "setOwner", &IScriptInvoker::setOwner )
                    .def( "callObjectMember", static_cast<void ( IScriptInvoker::* )( const String & )>(
                                                  &IScriptInvoker::callObjectMember ) )
                    .def( "callObjectMemberWithParameters",
                          static_cast<void ( IScriptInvoker::* )( const String &, const Parameters & )>(
                              &IScriptInvoker::callObjectMember ) )
                    .def( "event", static_cast<void ( IScriptInvoker::* )( hash_type )>(
                                       &IScriptInvoker::event ) )
                    .def( "eventWithParameters",
                          static_cast<void ( IScriptInvoker::* )( hash_type, const Parameters & )>(
                              &IScriptInvoker::event ) )
                    .def( "hasEvent", &IScriptInvoker::hasEvent )
                    .def( "setEventFunction", &IScriptInvoker::setEventFunction )
                    .def( "getEventFunction", &IScriptInvoker::getEventFunction )
                    .def( "getNumEvents", &IScriptInvoker::getNumEvents )
                    .def( "set", static_cast<void ( IScriptInvoker::* )( hash_type, const Parameter & )>(
                                     &IScriptInvoker::set ) )
                    .def( "setByName", static_cast<void ( IScriptInvoker::* )(
                                           const String &, const Parameter & )>( &IScriptInvoker::set ) )
                    .def( "get", static_cast<Parameter ( IScriptInvoker::* )( hash_type )>(
                                     &IScriptInvoker::get ) )
                    .def( "getByName", static_cast<Parameter ( IScriptInvoker::* )( const String & )>(
                                           &IScriptInvoker::get ) )
                    .scope[def( "typeInfo", IScriptInvoker::typeInfo )]];

        module(
            L )[class_<IScriptManager, ISharedObject, SmartPtr<IScriptManager>>( "IScriptManager" )
                    .def( "loadScript", &IScriptManager::loadScript )
                    .def( "loadScripts", &IScriptManager::loadScripts )
                    .def( "loadScriptFromString", &IScriptManager::loadScriptFromString )
                    .def( "callFunction", static_cast<void ( IScriptManager::* )( const String & )>(
                                              &IScriptManager::callFunction ) )
                    .def( "callFunctionWithParameters",
                          static_cast<void ( IScriptManager::* )( const String &, const Parameters & )>(
                              &IScriptManager::callFunction ) )
                    .def( "callMember",
                          static_cast<s32 ( IScriptManager::* )( const String &, const String & )>(
                              &IScriptManager::callMember ) )
                    .def( "callMemberWithParameters",
                          static_cast<s32 ( IScriptManager::* )( const String &, const String &,
                                                                 const Parameters & )>(
                              &IScriptManager::callMember ) )
                    .def( "callObjectMember", static_cast<void ( IScriptManager::* )(
                                                  SmartPtr<ISharedObject>, const String & )>(
                                                  &IScriptManager::callObjectMember ) )
                    .def( "callObjectMemberWithParameters",
                          static_cast<void ( IScriptManager::* )( SmartPtr<ISharedObject>,
                                                                  const String &, const Parameters & )>(
                              &IScriptManager::callObjectMember ) )
                    .def( "createObject", &IScriptManager::createObject )
                    .def( "destroyObject", &IScriptManager::destroyObject )
                    .def( "reloadScripts", &IScriptManager::reloadScripts )
                    .def( "reloadPending", &IScriptManager::reloadPending )
                    .def( "getDebugInfo", &IScriptManager::getDebugInfo )
                    .def( "getDelayedCreation", &IScriptManager::getDelayedCreation )
                    .def( "setDelayedCreation", &IScriptManager::setDelayedCreation )
                    .def( "isDebugEnabled", &IScriptManager::isDebugEnabled )
                    .def( "setDebugEnabled", &IScriptManager::setDebugEnabled )
                    .def( "removeBreakpoint", &IScriptManager::removeBreakpoint )
                    .def( "addBreakpoint", &IScriptManager::addBreakpoint )
                    .def( "getBreakpoints", &IScriptManager::getBreakpoints )
                    .def( "garbageCollect", &IScriptManager::garbageCollect )
                    .def( "getClassNames", &IScriptManager::getClassNames )
                    .def( "setClassNames", &IScriptManager::setClassNames )
                    .def( "getSupportedFileExtensions", &IScriptManager::getSupportedFileExtensions )
                    .def( "loadObject", &IScriptManager::loadObject )
                    .def( "unloadObject", &IScriptManager::unloadObject )
                    .scope[def( "typeInfo", IScriptManager::typeInfo )]];

        module( L )[class_<IScriptObject, ISharedObject, SmartPtr<IScriptObject>>( "IScriptObject" )
                        .def( "getInvoker", getScriptInvoker )
                        .def( "setInvoker", &IScriptObject::setInvoker )
                        .def( "getReceiver", getScriptReceiver )
                        .def( "setReceiver", &IScriptObject::setReceiver )
                        .scope[def( "typeInfo", IScriptObject::typeInfo )]];

        module(
            L )[class_<IScriptReceiver, ISharedObject, SmartPtr<IScriptReceiver>>( "IScriptReceiver" )
                    .def( "setString",
                          static_cast<s32 ( IScriptReceiver::* )( hash_type, const String & )>(
                              &IScriptReceiver::setProperty ) )
                    .def( "getString", getScriptReceiverString )
                    .def( "setParameter",
                          static_cast<s32 ( IScriptReceiver::* )( hash_type, const Parameter & )>(
                              &IScriptReceiver::setProperty ) )
                    .def( "getParameter", getScriptReceiverParameter )
                    .def( "setParameters",
                          static_cast<s32 ( IScriptReceiver::* )( hash_type, const Parameters & )>(
                              &IScriptReceiver::setProperty ) )
                    .def( "getParameters", getScriptReceiverParameters )
                    .def( "callFunction", static_cast<s32 ( IScriptReceiver::* )(
                                              hash_type, const Parameters &, Parameters & )>(
                                              &IScriptReceiver::callFunction ) )
                    .def( "callObjectFunction", static_cast<s32 ( IScriptReceiver::* )(
                                                    hash_type, SmartPtr<ISharedObject>, Parameters & )>(
                                                    &IScriptReceiver::callFunction ) )
                    .scope[def( "typeInfo", IScriptReceiver::typeInfo )]];

        module(
            L )[class_<IScriptUserData, ISharedObject, SmartPtr<IScriptUserData>>( "IScriptUserData" )
                    .scope[def( "typeInfo", IScriptUserData::typeInfo )]];

        module(
            L )[class_<IScriptVariable, ISharedObject, SmartPtr<IScriptVariable>>( "IScriptVariable" )
                    .def( "getType", getScriptVariableType )
                    .def( "setType", setScriptVariableType )
                    .scope[def( "typeInfo", IScriptVariable::typeInfo )]];
    }

} // namespace workphone
