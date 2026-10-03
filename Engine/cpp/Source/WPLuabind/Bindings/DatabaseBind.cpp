#include <WPLuabind/WPLuabindPCH.hpp>
#include <WPLuabind/Bindings/DatabaseBind.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    void bindDatabase( lua_State *L )
    {
        using namespace luabind;

        module(
            L )[class_<IDatabaseQuery, ISharedObject, SmartPtr<IDatabaseQuery>>( "IDatabaseQuery" )
                    .def( "getNumFields", &IDatabaseQuery::getNumFields )
                    .def( "getFieldName", &IDatabaseQuery::getFieldName )
                    .def( "getFieldValue", static_cast<String ( IDatabaseQuery::* )( u32 )>(
                                               &IDatabaseQuery::getFieldValue ) )
                    .def( "getFieldValue", static_cast<String ( IDatabaseQuery::* )( const String & )>(
                                               &IDatabaseQuery::getFieldValue ) )
                    .def( "getFieldValueAsInt", &IDatabaseQuery::getFieldValueAsInt )
                    .def( "getFieldValueAsFloat", &IDatabaseQuery::getFieldValueAsFloat )
                    .def( "isFieldValueNull", &IDatabaseQuery::isFieldValueNull )
                    .def( "nextRow", &IDatabaseQuery::nextRow )
                    .def( "eof", &IDatabaseQuery::eof )
                    .scope[def( "typeInfo", IDatabaseQuery::typeInfo )]];

        module(
            L )[class_<IDatabase, ISharedObject, SmartPtr<IDatabase>>( "IDatabase" )
                    .def( "loadFromFile", static_cast<void ( IDatabase::* )( const String & )>(
                                              &IDatabase::loadFromFile ) )
                    .def( "loadFromFile",
                          static_cast<void ( IDatabase::* )( const String &, const String & )>(
                              &IDatabase::loadFromFile ) )
                    .def( "close", &IDatabase::close )
                    .def( "setKey", &IDatabase::setKey )
                    .def( "query", static_cast<SmartPtr<IDatabaseQuery> ( IDatabase::* )(
                                       const String & )>( &IDatabase::query ) )
                    .def( "query", static_cast<SmartPtr<IDatabaseQuery> ( IDatabase::* )(
                                       const StringW & )>( &IDatabase::query ) )
                    .def( "queryDML",
                          static_cast<void ( IDatabase::* )( const String & )>( &IDatabase::queryDML ) )
                    .def( "queryDML",
                          static_cast<void ( IDatabase::* )( const StringW & )>( &IDatabase::queryDML ) )
                    .scope[def( "typeInfo", IDatabase::typeInfo )]];

        module(
            L )[class_<IDatabaseManager, ISharedObject, SmartPtr<IDatabaseManager>>( "IDatabaseManager" )
                    .def( "create", &IDatabaseManager::create )
                    .def( "destroy", &IDatabaseManager::destroy )
                    .def( "open", &IDatabaseManager::open )
                    .def( "close", &IDatabaseManager::close )
                    .def( "loadFromHandle", &IDatabaseManager::loadFromHandle )
                    .def( "loadFromFile", static_cast<void ( IDatabaseManager::* )( const String & )>(
                                              &IDatabaseManager::loadFromFile ) )
                    .def( "loadFromFile", static_cast<void ( IDatabaseManager::* )( const StringW & )>(
                                              &IDatabaseManager::loadFromFile ) )
                    .def( "attach", static_cast<void ( IDatabaseManager::* )( const String & )>(
                                        &IDatabaseManager::attach ) )
                    .def( "attach", static_cast<void ( IDatabaseManager::* )( const StringW & )>(
                                        &IDatabaseManager::attach ) )
                    .def( "executeQuery", &IDatabaseManager::executeQuery )
                    .def( "executeDML", static_cast<s32 ( IDatabaseManager::* )( const String & )>(
                                            &IDatabaseManager::executeDML ) )
                    .def( "executeDML", static_cast<s32 ( IDatabaseManager::* )( const StringW & )>(
                                            &IDatabaseManager::executeDML ) )
                    .def( "executeAsyncDML", &IDatabaseManager::executeAsyncDML )
                    .def( "runScript", &IDatabaseManager::runScript )
                    .def( "getDatabase", &IDatabaseManager::getDatabase )
                    .def( "setDatabase", &IDatabaseManager::setDatabase )
                    .def( "optimise", &IDatabaseManager::optimise )
                    .def( "getAttached", &IDatabaseManager::getAttached )
                    .def( "setAttached", &IDatabaseManager::setAttached )
                    .def( "addAttached", &IDatabaseManager::addAttached )
                    .def( "getDatabasePath", &IDatabaseManager::getDatabasePath )
                    .def( "setDatabasePath", &IDatabaseManager::setDatabasePath )
                    .scope[def( "typeInfo", IDatabaseManager::typeInfo )]];

        module( L )[class_<IResourceReference, ISharedObject, SmartPtr<IResourceReference>>(
                        "IResourceReference" )
                        .def( "getOwnerUUID", &IResourceReference::getOwnerUUID )
                        .def( "setOwnerUUID", &IResourceReference::setOwnerUUID )
                        .def( "getResourceUUID", &IResourceReference::getResourceUUID )
                        .def( "setResourceUUID", &IResourceReference::setResourceUUID )
                        .scope[def( "typeInfo", IResourceReference::typeInfo )]];
    }
} // namespace workphone
