#include <WPLua/LuaManager.hpp>
#include <WPLua/LuaObjectData.hpp>
#include <WPLuabind/SmartPtrConverter.hpp>
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>
#include <cstdio>
#include <stdexcept>
#ifdef WP_LUA_ASSET_TESTS
#include "LuaAssetContracts.hpp"
#endif
extern "C" {
#include <lauxlib.h>
}

namespace
{
    void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    class QueuedObject : public workphone::Properties
    {
    public:
        int loads = 0;
        int unloads = 0;
        void load( workphone::SmartPtr<workphone::ISharedObject> ) override { ++loads; }
        void unload( workphone::SmartPtr<workphone::ISharedObject> ) override { ++unloads; }
    };
}

int main()
{
    using namespace workphone;
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    int result = 0;
    {
        LuaManager manager;
        try
        {
            require( !manager.executeSource( "return 1", "=unloaded" ), "unloaded execution" );
            manager.clearStack();
            require( manager.getDebugInfo().empty(), "unloaded debug info" );
            manager.load( nullptr );
            auto state = manager.getLuaState();
            require( state != nullptr, "state allocation" );
            manager.load( nullptr );
            require( state == manager.getLuaState(), "double load replaced the state" );
            lua_pushinteger( state, 713 );
            const int top = lua_gettop( state );
            auto source = [&]( const char *text ) {
                require( manager.executeSource( text, "@Tests/Runtime.lua" ),
                         manager.getLastDiagnostic().c_str() );
                require( lua_gettop( state ) == top, "source leaked stack values" );
            };
            source( "return 1, 2, 3" );
            for( int i = 0; i < 64; ++i )
            {
                require( !manager.executeSource( "local function nested() error('failure') end; nested()",
                                                "@Tests/Failure.lua" ), "runtime failure accepted" );
                require( manager.getLastDiagnostic().find( "Tests/Failure.lua" ) != String::npos,
                         "source identifier missing" );
                require( manager.getLastDiagnostic().find( "stack traceback" ) != String::npos,
                         "traceback missing" );
                require( lua_gettop( state ) == top, "failure leaked stack values" );
            }
            require( !manager.executeSource( "local =", "@syntax.lua" ), "syntax failure accepted" );
            require( !manager.executeSource( "error({})", "@table-error.lua" ), "table error accepted" );
            require( !manager.executeSource( String( "return 1\0error('hidden')", 24 ), "@nul.lua" ),
                     "embedded NUL truncated source" );
            source( R"(
                calls = 0
                function noargs(...) assert(select('#', ...) == 0); calls = calls + 1 end
                function params(p) assert(p:at(0) == 'hello'); calls = calls + 1 end
                function outputs(p, r) r:push_back(p:at(0)); calls = calls + 1 end
                Methods = { count = 0 }
                function Methods:tick() self.count = self.count + 1 end
                function Methods:bad() error('member failure') end
                function broken() error('global failure') end
                function Good() return {} end
                function Bad() error('constructor failure') end
            )" );
            manager.callFunction( "noargs" );
            require( !manager.getError(), "global noargs call" );
            Parameters parameters( 1 );
            parameters[0].setStr( "hello" );
            manager.callFunction( "params", parameters );
            require( !manager.getError(), "global Parameters call" );
            Parameters outputs;
            manager.callFunction( "outputs", parameters, outputs );
            require( !manager.getError() && outputs.size() == 1 && outputs[0].getStr() == "hello",
                     "global mutable results ABI" );
            require( manager.callMember( "Methods", "tick" ) == 0, "class call" );
            require( manager.callMember( "Methods", "bad" ) == -1, "class failure status" );
            manager.callFunction( "broken" );
            require( manager.getError(), "global failure status" );
            manager.callFunction( "missing" );
            require( manager.getError(), "missing global status" );
            source( "assert(calls == 3 and Methods.count == 1)" );
            require( lua_gettop( state ) == top, "calls leaked stack values" );
            require( !manager.createInstance( "Bad" ), "constructor failure accepted" );
            require( !manager.createInstance( "MissingClass" ), "missing class accepted" );
            auto instance = manager.createInstance( "Good" );
            require( instance != nullptr, "valid constructor rejected" );
            manager.destroyInstance( instance );
            manager.destroyInstance( instance );
            manager.destroyInstance( nullptr );
            source( R"(
                class 'RuntimeOwnedBase'
                function RuntimeOwnedBase:inherited() self.owner:setName('inherited callback') end
                class 'RuntimeOwned' (RuntimeOwnedBase)
                function RuntimeOwned:__init(owner) self.owner = owner end
                function RuntimeOwned:bad() error('object failure') end
                function RuntimeOwned:typed(p) p:at(0):setProperty('typed', 'yes') end
            )" );
            auto owner = make_ptr<Properties>();
            auto metadata = manager.createObject( "RuntimeOwned", owner );
            require( metadata != nullptr, "constructed object metadata" );
            auto ownedData = owner->getScriptData();
            require( manager.createObject( "RuntimeOwned", owner ) == metadata &&
                     owner->getScriptData() == ownedData, "duplicate create replaced the instance" );
            manager.callObjectMember( owner, "inherited" );
            require( !manager.getError() && owner->getName() == "inherited callback", "inherited callback" );
            manager.callObjectMember( owner, "optionalAbsent" );
            require( !manager.getError(), "absent optional callback" );
            manager.callObjectMember( owner, "bad" );
            require( manager.getError(), "object error lost" );
            auto properties = make_ptr<Properties>();
            Parameters typed( 1 );
            typed[0].setObject( properties );
            manager.callObjectMember( owner, "typed", typed );
            require( properties->getProperty( "typed" ) == "yes", "typed Properties conversion" );
            require( !manager.createObject( "Bad", owner ) && owner->getScriptData() == ownedData,
                     "failed replacement damaged live instance" );
            manager.destroyObject( owner );
            manager.destroyObject( owner );
            require( !owner->getScriptData(), "owner retained destroyed data" );
            require( !static_pointer_cast<LuaObjectData>( ownedData )->getLuaState(), "destroyed data retained state" );
            require( lua_gettop( state ) == top, "constructors leaked stack values" );
            for( int i = 0; i < 16; ++i ) manager.updateClassNames();
            require( lua_gettop( state ) == top && lua_tointeger( state, -1 ) == 713,
                     "class enumeration changed caller stack" );
            auto queued = make_ptr<QueuedObject>();
            manager.loadObject( queued, true );
            Thread::setCurrentTask( TaskId::Application );
            manager.update();
            require( queued->loads == 1, "load queue was not drained" );
            manager.unloadObject( queued, true );
            manager.update();
            require( queued->unloads == 1, "unload queue was not drained" );
            manager.loadScript( "missing.lua" );
            require( manager.getError(), "missing file silently accepted" );
            auto retained = make_ptr<LuaObjectData>();
            retained->setLuaState( state );
            luabind::object value( luabind::from_stack( state, -1 ) );
            retained->setObject( value );
            retained->unload( nullptr );
            require( !retained->getObject() && !retained->getLuaState(), "retained data kept VM references" );
            value = luabind::object();
            manager.unload( nullptr );
            manager.unload( nullptr );
            require( !manager.getLuaState(), "state survived unload" );
            retained = nullptr;
            manager.load( nullptr );
            require( manager.executeSource( "assert(calls == nil)", "=fresh" ), "fresh VM" );
#ifdef WP_LUA_ASSET_TESTS
            runLuaAssetContracts( manager );
#endif
            manager.unload( nullptr );
            std::puts( "Lua production runtime tests: PASS" );
        }
        catch( const std::exception &error )
        {
            std::fprintf( stderr, "Lua production runtime tests: FAIL: %s\n", error.what() );
            result = 1;
        }
    }
    TypeManager::setInstance( nullptr );
    types.unload();
    return result;
}
