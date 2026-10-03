#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/ObjectBind.hpp"
#include <luabind/luabind.hpp>
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include <Workphone/Workphone.hpp>
#include <luabind/operator.hpp>

namespace workphone
{
    lua_Integer _getLoadingState( const ISharedObject *object )
    {
        return static_cast<lua_Integer>( object->getLoadingState() );
    }

    void _setLoadingState( ISharedObject *object, lua_Integer state )
    {
        object->setLoadingState( static_cast<LoadingState>( state ) );
    }

    SmartPtr<ISharedObject> _SharedObjectVector_at( const Array<SmartPtr<ISharedObject>> &objects,
                                                  size_t index )
    {
        // Return by value so SmartPtr's Lua converter preserves the selected object's interface.
        return objects.at( index );
    }

    void bindBaseObjects( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IObject, SmartPtr<IObject>>( "IObject" )
                        .def( "preUpdate", &IObject::preUpdate )
                        .def( "update", &IObject::update )
                        .def( "postUpdate", &IObject::postUpdate )
                        .def( "getNamePtr", &IObject::getNamePtr )
                        .def( "getId", &IObject::getId )
                        .def( "setId", &IObject::setId )
                        .def( "getName", &IObject::getName )
                        .def( "setName", &IObject::setName )
                        .def( "setObjectFlags", &IObject::setObjectFlags )
                        .def( "setObjectFlag", &IObject::setObjectFlag )
                        .def( "getObjectFlag", &IObject::getObjectFlag )
                        .def( "getHandle", static_cast<Handle *(IObject::*)()>( &IObject::getHandle ) )
                        .def( "isValid", &IObject::isValid )
                        .def( "getCreatorData", &IObject::getCreatorData )
                        .def( "setCreatorData", &IObject::setCreatorData )
                        .def( "getFactoryData", &IObject::getFactoryData )
                        .def( "setFactoryData", &IObject::setFactoryData )
                        .def( "toString", &IObject::toString )
                        .def( "getUserData",
                              static_cast<void *(IObject::*)() const>( &IObject::getUserData ) )
                        .def( "setUserData",
                              static_cast<void ( IObject::* )( void * )>( &IObject::setUserData ) )
                        .def( "getUserData", static_cast<void *(IObject::*)( hash_type ) const>(
                                                 &IObject::getUserData ) )
                        .def( "setUserData", static_cast<void ( IObject::* )( hash_type, void * )>(
                                                 &IObject::setUserData ) )
                        .def( "derived", &IObject::derived )
                        .def( "exactly", &IObject::exactly )
                        .def( "getTypeInfo", &IObject::getTypeInfo )
                        .def( "__tostring", &IObject::toString )
#if !WP_FINAL
                        .def( "getDebugStr", &IObject::getDebugStr )
                        .def( "setDebugStr", &IObject::setDebugStr )
#endif
                        .scope[def( "typeInfo", IObject::typeInfo )]];

        module( L )[class_<ISharedObject, IObject, SmartPtr<ISharedObject>>( "ISharedObject" )
                        .def( "getReferences", &ISharedObject::getReferences )
                        .def( "getWeakReferences", &ISharedObject::getWeakReferences )
                        .def( "load", &ISharedObject::load )
                        .def( "reload", &ISharedObject::reload )
                        .def( "unload", &ISharedObject::unload )
                        .def( "getLoadingState", _getLoadingState )
                        .def( "setLoadingState", _setLoadingState )
                        .def( "isLoading", &ISharedObject::isLoading )
                        .def( "isLoadingQueued", &ISharedObject::isLoadingQueued )
                        .def( "isLoaded", &ISharedObject::isLoaded )
                        .def( "isThreadSafe", &ISharedObject::isThreadSafe )
                        .def( "isAlive", &ISharedObject::isAlive )
                        .def( "setPoolElement", &ISharedObject::setPoolElement )
                        .def( "isPoolElement", &ISharedObject::isPoolElement )
                        .def( "toData", &ISharedObject::toData )
                        .def( "fromData", &ISharedObject::fromData )
                        .def( "getProperties", &ISharedObject::getProperties )
                        .def( "setProperties", &ISharedObject::setProperties )
                        .def( "getChildObjects", &ISharedObject::getChildObjects )
                        .def( "getObjectListeners", &ISharedObject::getObjectListeners )
                        .def( "getNumListeners", &ISharedObject::getNumListeners )
                        .def( "hasObjectListener", &ISharedObject::hasObjectListener )
                        .def( "addObjectListener", &ISharedObject::addObjectListener )
                        .def( "removeObjectListener", &ISharedObject::removeObjectListener )
                        .def( "removeObjectListeners", &ISharedObject::removeObjectListeners )
                        .def( "findObjectListener", &ISharedObject::findObjectListener )
                        .def( "getSharedObjectListener", &ISharedObject::getSharedObjectListener )
                        .def( "setSharedObjectListener", &ISharedObject::setSharedObjectListener )
                        .def( "isGarbageCollected", &ISharedObject::isGarbageCollected )
                        .def( "setGarbageCollected", &ISharedObject::setGarbageCollected )
                        .def( "getScriptDataPtr", &ISharedObject::getScriptDataPtr )
                        .def( "getScriptData", &ISharedObject::getScriptData )
                        .def( "setScriptData", &ISharedObject::setScriptData )
                        .def( "getEventTaskFlags", &ISharedObject::getEventTaskFlags )
                        .def( "setEventTaskFlags", &ISharedObject::setEventTaskFlags )
                        .def( "lockLoad", &ISharedObject::lockLoad )
                        .def( "unlockLoad", &ISharedObject::unlockLoad )
                        .def( "isLoadLocked", &ISharedObject::isLoadLocked )
                        .def( "lock", &ISharedObject::lock )
                        .def( "try_lock", &ISharedObject::try_lock )
                        .def( "unlock", &ISharedObject::unlock )
                        .def( "lock_shared", &ISharedObject::lock_shared )
                        .def( "unlock_shared", &ISharedObject::unlock_shared )
                        .scope[def( "typeInfo", ISharedObject::typeInfo )]];

        module( L )[class_<IData, ISharedObject, SmartPtr<IData>>( "IData" )
                        .def( "setData", &IData::setData )
                        .def( "getData", static_cast<void *(IData::*)()>( &IData::getData ) )
                        .scope[def( "typeInfo", IData::typeInfo )]];

        module( L )[class_<ISharedObjectListener, ISharedObject, SmartPtr<ISharedObjectListener>>(
                        "ISharedObjectListener" )
                        .def( "destroy", &ISharedObjectListener::destroy )];

        module( L )[class_<IObjectBuilder, ISharedObject, SmartPtr<IObjectBuilder>>( "IObjectBuilder" )
                        .def( "create", &IObjectBuilder::create )
                        .scope[def( "typeInfo", IObjectBuilder::typeInfo )]];

        using SharedObjectVector = Array<SmartPtr<ISharedObject>>;

        module(
            L )[class_<SharedObjectVector>( "SharedObjectVector" )
                    .def( "push_back",
                          static_cast<void ( SharedObjectVector::* )( const SmartPtr<ISharedObject> & )>(
                              &SharedObjectVector::push_back ) )
                    .def( "pop_back", &SharedObjectVector::pop_back )
                    .def( "clear", &SharedObjectVector::clear )
                    .def( "size", &SharedObjectVector::size )
                    .def( "empty", &SharedObjectVector::empty )
                    .def( "at", _SharedObjectVector_at )];
    }
} // namespace workphone
