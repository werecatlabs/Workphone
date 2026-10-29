#include <Workphone/WorkphoneConfig.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <WPLua/LuaManager.hpp>
#include <WPLua/LuaDebugger.hpp>
#include <WPLuabind/LuaCompat.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <thread>

namespace j = boost::json;
using tcp = boost::asio::ip::tcp;
using Clock = std::chrono::steady_clock;
static void check( bool value, const char *message ) { if( !value ) throw std::runtime_error( message ); }

class Client
{
    boost::asio::io_context io;
    tcp::socket socket{ io };
    int sequence = 0;
    std::string input;
public:
    explicit Client( unsigned short port )
    {
        socket.connect( tcp::endpoint( boost::asio::ip::address_v4::loopback(), port ) );
        socket.non_blocking( true );
    }
    int request( const char *command, j::object args = {} )
    {
        const int seq = ++sequence;
        const auto body = j::serialize( j::object{ { "seq", seq }, { "type", "request" }, { "command", command }, { "arguments", std::move( args ) } } );
        const auto packet = "Content-Length: " + std::to_string( body.size() ) + "\r\n\r\n" + body;
        // Deliberately fragment headers and UTF-8 JSON payloads.
        for( size_t pos = 0; pos < packet.size(); )
        {
            boost::system::error_code error;
            pos += socket.write_some( boost::asio::buffer( packet.data() + pos, std::min<size_t>( 7, packet.size() - pos ) ), error );
            if( error == boost::asio::error::would_block ) std::this_thread::yield();
            else if( error ) throw boost::system::system_error( error );
        }
        return seq;
    }
    j::object read()
    {
        const auto deadline = Clock::now() + std::chrono::seconds( 15 );
        while( Clock::now() < deadline )
        {
            const auto end = input.find( "\r\n\r\n" );
            if( end != std::string::npos )
            {
                const auto size = std::stoul( input.substr( 16, end - 16 ) );
                if( input.size() >= end + 4 + size )
                {
                    auto message = j::parse( input.substr( end + 4, size ) ).as_object();
                    input.erase( 0, end + 4 + size ); return message;
                }
            }
            char buffer[4096]; boost::system::error_code error;
            const auto count = socket.read_some( boost::asio::buffer( buffer ), error );
            if( !error ) input.append( buffer, count );
            else if( error != boost::asio::error::would_block && error != boost::asio::error::try_again ) throw boost::system::system_error( error );
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
        throw std::runtime_error( "DAP response timed out" );
    }
    j::object reply( int id, bool success = true )
    {
        for( ;; )
        {
            auto message = read();
            if( message.at( "type" ) == "response" && message.at( "request_seq" ).as_int64() == id )
            { check( message.at( "success" ).as_bool() == success, j::serialize( message ).c_str() ); auto body = message.if_contains( "body" ); return body && body->is_object() ? body->as_object() : j::object{}; }
        }
    }
    j::object event( const char *name )
    { for( ;; ) { auto message = read(); if( message.at( "type" ) == "event" && message.at( "event" ) == name ) return message.at( "body" ).as_object(); } }
    void close() { boost::system::error_code ignored; socket.close( ignored ); }
};

#ifdef WP_LUA_BACKEND_LUAJIT
static void retainedDisconnectContract( const std::string &root )
{
    workphone::LuaDebugger debugger;
    auto state = luaL_newstate();
    check( state != nullptr, "Cannot create retained-disconnect VM" );
    luaL_openlibs( state );
    struct Context
    {
        workphone::LuaDebugger *debugger;
        lua_State *other;
        Clock::time_point until;
        std::atomic<bool> ready{ false }, failed{ false }, acknowledged{ false }, finish{ false };
        int before = 0, after = 0, otherAfter = 0;
        bool socketRetained = false;
        std::string clientError;
    } context{ &debugger, lua_newthread( state ), Clock::now() + std::chrono::seconds( 15 ) };
    const auto source = root + "/debug-jit-retained-disconnect.lua";
    const std::string code = "local n=0\nstepDisconnectGate()\nn=n+1\n";
    std::thread client;
    std::exception_ptr failure;
    try
    {
        std::string error;
        check( debugger.start( state, 0, "retained-disconnect-contract-token", root, error ), error.c_str() );
        debugger.source( "@" + source, code );
        debugger.setPausePump( [&]() { return Clock::now() < context.until && !context.failed; } );
        lua_pushlightuserdata( state, &context );
        lua_pushcclosure( state, []( lua_State *vm ) -> int {
            auto &probe = *static_cast<Context *>( lua_touserdata( vm, lua_upvalueindex( 1 ) ) );
            // This gate runs on the VM owner before another Lua line can stop
            // the active step and restore its count interval incidentally.
            probe.before = lua_gethookcount( vm );
            while( !probe.acknowledged && !probe.failed && Clock::now() < probe.until )
            {
                probe.debugger->poll();
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            }
            probe.after = lua_gethookcount( vm );
            probe.otherAfter = lua_gethookcount( probe.other );
            probe.socketRetained = j::parse( probe.debugger->status() ).as_object().at( "connected" ).as_bool();
            return 0;
        }, 1 );
        lua_setglobal( state, "stepDisconnectGate" );
        client = std::thread( [&]() {
            try
            {
                Client connection( debugger.port() );
                connection.reply( connection.request( "initialize" ) );
                const auto attach = connection.request( "attach", { { "token", "retained-disconnect-contract-token" } } );
                connection.event( "initialized" );
                connection.reply( connection.request( "setBreakpoints", {
                    { "source", j::object{ { "path", source } } },
                    { "breakpoints", j::array{ j::object{ { "line", 2 } } } } } ) );
                connection.reply( connection.request( "configurationDone" ) ); connection.reply( attach );
                context.ready = true;
                connection.event( "stopped" );
                connection.reply( connection.request( "next", { { "threadId", 1 } } ) );
                connection.reply( connection.request( "disconnect" ) );
                context.acknowledged = true;
                // Keep the physical connection open until the owner has read
                // both hook intervals; network-generation cleanup cannot help.
                while( !context.finish && Clock::now() < context.until )
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                connection.close();
            }
            catch( const std::exception &error )
            {
                context.clientError = error.what(); context.failed = true; context.ready = true;
            }
        } );
        while( !context.ready && Clock::now() < context.until )
        {
            debugger.poll(); std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
        check( context.ready && !context.failed, "Retained-disconnect client did not configure" );
        check( luaL_loadbufferx( state, code.data(), code.size(), ( "@" + source ).c_str(), "t" ) == LUA_OK &&
               lua_pcall( state, 0, 0, 0 ) == LUA_OK, "Retained-disconnect script failed" );
        check( context.acknowledged && context.socketRetained && context.before == 1,
               "Disconnect fixture must retain TCP while a step owns COUNT1" );
        check( context.after == 1000 && context.otherAfter == 1000,
               "Explicit disconnect must restore normal hook counts globally before TCP closes" );
    }
    catch( ... ) { failure = std::current_exception(); }
    context.finish = true;
    debugger.stop();
    if( client.joinable() ) client.join();
    lua_close( state );
    if( failure ) std::rethrow_exception( failure );
    check( !context.failed, context.clientError.c_str() );
}
#endif

int main( int argc, char **argv )
{
    using namespace workphone;
    TypeManager types; types.load(); TypeManager::setInstance( &types );
    int result = 0;
    {
        LuaManager manager;
        std::atomic<bool> ready{ false }, failed{ false }, done{ false };
        std::atomic<int> pumps{ 0 };
        std::string clientError;
        std::thread client;
        try
        {
            manager.load( nullptr );
            // Optional manual IDE fixture uses the production host and service.
            // Own a fresh directory; never point this at a user's project.
            if( argc == 3 && std::string( argv[1] ) == "--ide-fixture" )
            {
                const auto fixture = std::filesystem::absolute( std::filesystem::u8path( argv[2] ) );
                check( !std::filesystem::exists( fixture ), "IDE fixture requires a new directory" );
                std::filesystem::create_directories( fixture );
                const auto source = fixture / "DebugFixture.lua";
                const std::string code = "local data={value=41}\nlocal function sample()\n local answer=data.value+1\n return answer\nend\nfixtureResult=sample()\n";
                { std::ofstream output( source ); output << code; }
                const auto token = "ide-fixture-" + std::to_string( Clock::now().time_since_epoch().count() );
                check( manager.startDebugger( 0, token.c_str(), fixture.generic_u8string().c_str() ), "Cannot start IDE fixture" );
                const auto until = Clock::now() + std::chrono::minutes( 5 );
                manager.setDebuggerPausePump( [&]() { return Clock::now() < until && !std::filesystem::exists( fixture / "stop" ); } );
                const auto endpoint = fixture / "lua-debug.json";
                { std::ofstream output( endpoint ); output << j::serialize( j::object{ { "port", manager.getDebuggerPort() }, { "token", token }, { "sourceRoot", fixture.generic_u8string() } } ); }
                while( Clock::now() < until && !std::filesystem::exists( fixture / "stop" ) )
                {
                    manager.executeSource( code.c_str(), ( "@" + source.generic_u8string() ).c_str() );
                    manager.update(); std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
                }
                manager.stopDebugger(); manager.unload( nullptr );
                std::filesystem::remove( endpoint );
                TypeManager::setInstance( nullptr ); types.unload();
                return 0;
            }
            auto timer = make_ptr<TimerMT>(); timer->load( nullptr );
            Thread::setCurrentTask( TaskId::Application ); timer->update();
            // A paused redraw can query every task's clock, but cannot advance
            // native consumers through a retained positive frame delta.
            const auto originalDelta = timer->getDeltaTime();
            const auto originalTime = timer->getTime();
            const auto originalTicks = timer->getTickCount();
            timer->beginSuspension();
            const auto frozenNow = timer->now();
            const auto frozenFixedNow = timer->getFixedTimeNow();
            timer->beginSuspension();
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
            for( auto task : { TaskId::Application, TaskId::Render, TaskId::Physics } )
            {
                Thread::setCurrentTask( task );
                const auto taskTime = timer->getTime();
                const auto taskTicks = timer->getTickCount();
                const auto taskFixed = timer->getFixedTime();
                check( timer->getDeltaTime() == 0.0 && timer->getDeltaTime( task ) == 0.0 &&
                       timer->getSmoothDeltaTime() == 0.0 && timer->getSmoothDeltaTime( task ) == 0.0,
                       "Suspension exposed a native simulation delta" );
                timer->update(); timer->update( 0.5 ); timer->updateFixed();
                check( timer->getTime() == taskTime && timer->getTickCount() == taskTicks &&
                       timer->getFixedTime() == taskFixed, "Suspended timer update advanced simulation" );
            }
            Thread::setCurrentTask( TaskId::Application );
            check( timer->now() == frozenNow && timer->now128() == frozenNow &&
                   timer->getFixedTimeNow() == frozenFixedNow &&
                   timer->getFixedTimeNow( static_cast<u32>( TaskId::Application ) ) == frozenFixedNow,
                   "Suspended engine clock advanced with wall time" );
            timer->endSuspension();
            check( timer->isSuspended() && timer->getDeltaTime() == 0.0,
                   "Nested suspension resumed the timer early" );
            const auto resumeStart = Clock::now();
            timer->endSuspension();
            const auto resumedNow = timer->now();
            const auto resumeElapsed = std::chrono::duration<double>( Clock::now() - resumeStart ).count();
            check( !timer->isSuspended() && timer->getDeltaTime() == originalDelta &&
                   timer->getTime() == originalTime && timer->getTickCount() == originalTicks,
                   "Suspension lost the interrupted frame clock" );
            check( resumedNow >= frozenNow && resumedNow - frozenNow <= resumeElapsed + 0.001,
                   "Resuming introduced paused wall time into the engine clock" );
            int boundaries = 0;
            manager.setDebuggerPauseObserver( [&]( bool paused ) {
                ++boundaries;
                if( paused ) timer->beginSuspension();
                else timer->endSuspension();
            } );
            const auto root = std::filesystem::current_path().generic_u8string();
            check( !manager.startDebugger( 0, "short", root.c_str() ), "Short authentication token accepted" );
            check( manager.startDebugger( 0, "workphone-debug-test-token", root.c_str() ), "Debugger did not listen" );
            struct PhysicalSource
            {
                std::filesystem::path path = std::filesystem::temp_directory_path() /
                    ( "workphone-debug-source-" + std::to_string( Clock::now().time_since_epoch().count() ) + ".lua" );
                PhysicalSource() { std::ofstream output( path ); output << "diskSourceValue=42"; }
                ~PhysicalSource() { std::error_code ignored; std::filesystem::remove( path, ignored ); }
            } physicalSource;
            check( manager.executeSource( "diskSourceValue=42", ( "@" + physicalSource.path.generic_u8string() ).c_str() ), "Physical source fixture" );
            { std::ofstream changed( physicalSource.path ); changed << "diskSourceValue=999"; }
            check( manager.executeSource( "assert(jit == nil or not jit.status())", "=debug-backend" ), "JIT compilation stayed enabled while debugging" );
            check( manager.executeSource( "gameplayCalls=0; function gameplayCallback() gameplayCalls=gameplayCalls+1 end", "@gameplay.lua" ), "Gameplay fixture" );
            check( manager.executeSource( "editorCalls=0; function editorCallback() editorCalls=editorCalls+1 end", "@Editor/DebuggerFixture.lua" ), "Editor fixture" );
            check( manager.executeSource( "largeId=9007199254740993; huge={}; for i=1,160 do huge['field'..i]=string.rep('x',1200) end", "@values.lua" ), "Inspection budgets fixture" );
            check( manager.executeSource( "beyond={}; for i=1,5000 do beyond[i]=i end", "=large-table" ), "Large table fixture" );
            check( manager.executeSource( "pendingCo=coroutine.create(function()\nlocal marker=21\nfor i=1,3 do\n marker=marker+1\nend\nreturn marker\nend)", "@debug-coroutine.lua" ), "Pre-attach coroutine fixture" );
            manager.setDebuggerPausePump( [&]() {
                ++pumps;
                manager.callFunction( "gameplayCallback" );
                manager.callFunction( "editorCallback" );
                return !failed.load();
            } );
            const auto port = manager.getDebuggerPort();
            client = std::thread( [&]() {
                try
                {
                    Client connection( port );
                    connection.reply( connection.request( "initialize", { { "clientID", "contracts" } } ) );
                    connection.reply( connection.request( "attach", { { "token", "incorrect-session-token" } } ), false );
                    const int attach = connection.request( "attach", { { "token", "workphone-debug-test-token" } } );
                    connection.event( "initialized" );
                    connection.reply( connection.request( "setBreakpoints", { { "source", j::object{ { "path", root + "/debug-contract.lua" } } },
                        { "breakpoints", j::array{
                            j::object{ { "line", 2 }, { "condition", "false" } },
                            j::object{ { "line", 4 }, { "logMessage", "captured={data.value}" }, { "condition", "data.value >= 12" }, { "hitCondition", ">=1" } },
                            j::object{ { "line", 5 }, { "condition", "inner == 19 and not false" } } } } } ) );
                    connection.reply( connection.request( "setExceptionBreakpoints", { { "filters", j::array{ "uncaught" } } } ) );
                    j::array coroutineBreakpoints{ j::object{ { "line", 4 }, { "hitCondition", "2" }, { "condition", "marker >= 21" } } };
                    connection.reply( connection.request( "setBreakpoints", {
                        { "source", j::object{ { "path", root + "/debug-coroutine.lua" } } },
                        { "breakpoints", std::move( coroutineBreakpoints ) } } ) );
                    connection.reply( connection.request( "configurationDone" ) ); connection.reply( attach );
                    auto loaded = connection.reply( connection.request( "loadedSources" ) ).at( "sources" ).as_array();
                    bool physical = false;
                    for( const auto &source : loaded )
                        if( std::string( source.as_object().at( "name" ).as_string() ) == physicalSource.path.filename().generic_u8string() )
                        {
                            physical = true;
                            check( source.as_object().at( "sourceReference" ).as_int64() == 0,
                                   "Physical source was advertised as a virtual document" );
                        }
                    check( physical, "Physical source disappeared from loadedSources" );
                    const auto physicalText = connection.reply( connection.request( "source", {
                        { "sourceReference", 0 }, { "source", j::object{ { "path", physicalSource.path.generic_u8string() } } } } ) );
                    check( physicalText.at( "content" ) == "diskSourceValue=42", "Editor cannot retrieve a registered physical source snapshot" );
                    connection.reply( connection.request( "source", { { "sourceReference", 0 },
                        { "source", j::object{ { "path", root + "/unregistered.lua" } } } } ), false );
                    ready = true;
                    check( std::string( connection.event( "output" ).at( "output" ).as_string() ).find( "debug output\t23" ) != std::string::npos, "Lua print was not forwarded intact" );
                    check( std::string( connection.event( "output" ).at( "output" ).as_string() ).find( "captured=12" ) != std::string::npos, "Logpoint did not evaluate its expression" );
                    check( connection.event( "stopped" ).at( "reason" ) == "breakpoint", "Conditional breakpoint was not hit" );
                    std::this_thread::sleep_for( std::chrono::milliseconds( 1100 ) );
                    auto stack = connection.reply( connection.request( "stackTrace" ) ).at( "stackFrames" ).as_array();
                    check( !stack.empty() && stack.front().as_object().at( "line" ).as_int64() == 5, "Wrong paused line" );
                    const auto frame = stack.front().as_object().at( "id" ).as_int64();
                    auto scopes = connection.reply( connection.request( "scopes", { { "frameId", frame } } ) ).at( "scopes" ).as_array();
                    const auto locals = scopes.front().as_object().at( "variablesReference" ).as_int64();
                    auto values = connection.reply( connection.request( "variables", { { "variablesReference", locals } } ) ).at( "variables" ).as_array();
                    bool inner = false;
                    for( const auto &value : values ) if( value.as_object().at( "name" ) == "inner" ) inner = value.as_object().at( "value" ) == "19";
                    check( inner, "Local value lost at breakpoint" );
                    auto evaluated = connection.reply( connection.request( "evaluate", { { "frameId", frame }, { "expression", "data.value" } } ) );
                    check( evaluated.at( "result" ) == "12", "Upvalue/table evaluation failed" );
                    evaluated = connection.reply( connection.request( "evaluate", { { "frameId", frame }, { "expression", "(false and missing.value) or (inner + data.value * 2)" } } ) );
                    check( evaluated.at( "result" ) == "43", "Read-only expression or short-circuit evaluation failed" );
                    evaluated = connection.reply( connection.request( "evaluate", { { "frameId", frame }, { "expression", "-2^2 + #'abc'" } } ) );
                    check( evaluated.at( "result" ) == "-1", "Expression precedence failed" );
                    evaluated = connection.reply( connection.request( "evaluate", { { "frameId", frame }, { "expression", "(0/0) >= 0" } } ) );
                    check( evaluated.at( "result" ) == "false", "NaN comparison changed Lua semantics" );
#ifndef WP_LUA_BACKEND_LUAJIT
                    evaluated = connection.reply( connection.request( "evaluate", { { "frameId", frame }, { "expression", "largeId + 0" } } ) );
                    check( evaluated.at( "result" ) == "9007199254740993", "64-bit integer watch lost precision" );
                    evaluated = connection.reply( connection.request( "evaluate", { { "frameId", frame }, { "expression", "largeId == 9007199254740992.0" } } ) );
                    check( evaluated.at( "result" ) == "false", "Integer/float comparison lost precision" );
#endif
                    evaluated = connection.reply( connection.request( "evaluate", { { "frameId", frame }, { "expression", "huge" } } ) );
                    auto page = connection.reply( connection.request( "variables", { { "variablesReference", evaluated.at( "variablesReference" ) } } ) ).at( "variables" ).as_array();
                    check( !page.empty() && page.back().as_object().at( "name" ) == "[more variables]", "Large inspection was not paged" );
                    auto nextPage = connection.reply( connection.request( "variables", { { "variablesReference", page.back().as_object().at( "variablesReference" ) } } ) ).at( "variables" ).as_array();
                    check( !nextPage.empty(), "Inspection continuation lost its table" );
                    evaluated = connection.reply( connection.request( "evaluate", { { "frameId", frame }, { "expression", "beyond" } } ) );
                    page = connection.reply( connection.request( "variables", { { "variablesReference", evaluated.at( "variablesReference" ) }, { "start", 4090 }, { "count", 20 } } ) ).at( "variables" ).as_array();
                    check( !page.empty() && page.back().as_object().at( "name" ) == "[inspection limit]", "Table budget silently hid remaining keys" );
                    connection.reply( connection.request( "evaluate", { { "frameId", frame }, { "expression", "os.exit()" } } ), false );
                    connection.reply( connection.request( "stepIn", { { "threadId", 1 } } ) );
                    connection.event( "stopped" );
                    connection.reply( connection.request( "stepOut", { { "threadId", 1 } } ) );
                    connection.event( "stopped" );
                    connection.reply( connection.request( "variables", { { "variablesReference", locals } } ), false );
                    connection.reply( connection.request( "setBreakpoints", { { "source", j::object{ { "path", root + "/debug-contract.lua" } } }, { "breakpoints", j::array{} } } ) );
                    int steppedLine = 0;
                    // Lua 5.1 and 5.5 report the caller's return line differently.
                    // Both must remain in the caller when stepping over child().
                    for( int i = 0; i < 5 && steppedLine != 11; ++i )
                    {
                        connection.reply( connection.request( "next", { { "threadId", 1 } } ) );
                        connection.event( "stopped" );
                        auto stepped = connection.reply( connection.request( "stackTrace" ) ).at( "stackFrames" ).as_array();
                        steppedLine = static_cast<int>( stepped.front().as_object().at( "line" ).as_int64() );
                        check( stepped.size() == 1 && steppedLine >= 8, "Step over entered a child function" );
                    }
                    check( steppedLine == 11, "Step over did not reach the next statement" );
                    connection.reply( connection.request( "continue", { { "threadId", 1 } } ) );
                    check( connection.event( "stopped" ).at( "reason" ) == "breakpoint", "Coroutine did not retain its pre-attach hook" );
                    auto coroutineStack = connection.reply( connection.request( "stackTrace" ) ).at( "stackFrames" ).as_array();
                    const auto coroutineFrame = coroutineStack.front().as_object().at( "id" );
                    evaluated = connection.reply( connection.request( "evaluate", { { "frameId", coroutineFrame }, { "expression", "marker" } } ) );
                    check( evaluated.at( "result" ) == "22", "Coroutine local inspection or hit count failed" );
                    auto sourceReference = coroutineStack.front().as_object().at( "source" ).as_object().at( "sourceReference" );
                    auto loadedSource = connection.reply( connection.request( "source", { { "sourceReference", sourceReference } } ) );
                    check( std::string( loadedSource.at( "content" ).as_string() ).find( "coroutine.create" ) != std::string::npos, "Loaded coroutine source is unavailable" );
                    connection.reply( connection.request( "continue", { { "threadId", 1 } } ) );
                    auto error = connection.event( "stopped" ); check( error.at( "reason" ) == "exception", "Uncaught error did not stop" );
                    auto info = connection.reply( connection.request( "exceptionInfo", { { "threadId", 1 } } ) );
                    check( std::string( info.at( "description" ).as_string() ).find( "debug exception" ) != std::string::npos, "Exception text lost" );
                    connection.close(); // Disconnect must release the suspended VM.
                    done = true;
                }
                catch( const std::exception &error ) { clientError = error.what(); failed = true; ready = true; done = true; }
            } );
            Thread::setCurrentTask( TaskId::Application );
            auto deadline = Clock::now() + std::chrono::seconds( 20 );
            while( !ready && Clock::now() < deadline ) { manager.update(); std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ); }
            check( ready && !failed, "DAP configuration failed" );
            timer->update(); const auto ticks = timer->getTickCount(); const auto fixed = timer->getFixedTimeNow();
            check( manager.executeSource( "print('debug output',23)", "=debug-print" ), "Debug print failed" );
            check( manager.executeSource( "local outer=7\nlocal data={value=12}\nlocal function child()\n local inner=outer+data.value\n inner=inner+1\n return inner\nend\nresult=child()\nresult=result+1\nresult=child()\nresult=result+1", "@debug-contract.lua" ), "Debugged execution failed" );
            check( manager.executeSource( "assert(result==21)", "=result" ), "Stepping changed program behavior" );
            check( manager.executeSource( "local ok,value=coroutine.resume(pendingCo); assert(ok and value==24)", "@resume-coroutine.lua" ), "Debugged coroutine failed" );
            check( !manager.executeSource( "local marker=42\nerror(string.char(255)..'debug exception')", "@debug-error.lua" ), "Lua error unexpectedly succeeded" );
            check( pumps > 0, "Paused Editor pump did not run" );
            check( done && !failed, "Client contracts failed" );
            if( client.joinable() ) client.join();
            check( clientError.empty(), clientError.c_str() );
            timer->update();
            check( timer->getDeltaTime() < 1.0 && timer->getFixedTimeNow() - fixed < 1.0,
                   "Debugger suspension inflated frame or fixed time" );
            check( timer->getTickCount() == ticks + 1 && boundaries >= 2 && boundaries % 2 == 0,
                   "Pause observer reset ticks or missed a boundary" );
            const bool dispatch = manager.executeSource( "assert(gameplayCalls==0, 'gameplayCalls='..tostring(gameplayCalls)); assert(editorCalls>0, 'editorCalls='..tostring(editorCalls))", "=paused-dispatch" );
            check( dispatch, manager.getLastDiagnostic().c_str() );
            int sequence = 1000;
            auto localRequest = [&]( const char *command, j::object arguments = {} ) {
                const int id = ++sequence;
                manager.debuggerCommand( j::serialize( j::object{ { "seq", id }, { "type", "request" }, { "command", command }, { "arguments", std::move( arguments ) } } ).c_str() );
                return id;
            };
            auto localReply = [&]( int id ) {
                const auto status = j::parse( manager.getDebuggerStatus().c_str() ).as_object();
                for( const auto &reply : status.at( "responses" ).as_array() )
                    if( reply.as_object().at( "request_seq" ).as_int64() == id ) return reply.as_object();
                return j::object{};
            };
            const auto preferencesRoot = std::filesystem::temp_directory_path() / ( "workphone-lua-debug-" + std::to_string( Clock::now().time_since_epoch().count() ) );
            check( manager.startDebugger( 0, "workphone-debug-test-token", preferencesRoot.generic_u8string().c_str() ), "Cannot rebase debugger" );
            auto requestId = localRequest( "saveBreakpointPreferences", { { "files", j::array{ j::object{ { "source", j::object{ { "path", root + "/local-debug.lua" } } }, { "breakpoints", j::array{ j::object{ { "line", 2 } } } } } } }, { "breakOnException", true } } );
            manager.update(); check( localReply( requestId ).at( "success" ).as_bool(), "Cannot save Editor breakpoint preferences" );
            requestId = localRequest( "loadBreakpointPreferences" ); manager.update();
            auto saved = localReply( requestId ).at( "body" ).as_object();
            check( saved.at( "files" ).as_array().size() == 1 && saved.at( "breakOnException" ).as_bool(), "Editor breakpoint preferences did not round trip" );
            requestId = localRequest( "saveBreakpointPreferences", { { "files", j::array{ 42 } }, { "breakOnException", false } } );
            manager.update(); check( !localReply( requestId ).at( "success" ).as_bool(), "Malformed Editor preferences accepted" );
            std::filesystem::remove( preferencesRoot / ".workphone/lua-breakpoints.json" );
            std::filesystem::remove( preferencesRoot / ".workphone" ); std::filesystem::remove( preferencesRoot );
            int localStops = 0, phase = 0, pending = 0;
            manager.setDebuggerPausePump( [&]() {
                ++localStops;
                if( phase == 0 ) { pending = localRequest( "stackTrace" ); phase = 1; }
                else if( phase == 1 )
                {
                    auto reply = localReply( pending ); if( reply.empty() ) return true;
                    const auto frame = reply.at( "body" ).as_object().at( "stackFrames" ).as_array().front().as_object().at( "id" );
                    pending = localRequest( "evaluate", { { "frameId", frame }, { "expression", "6 * 7" } } ); phase = 2;
                }
                else if( phase == 2 )
                {
                    auto reply = localReply( pending ); if( reply.empty() ) return true;
                    check( reply.at( "body" ).as_object().at( "result" ) == "42", "Local Editor inspection failed" );
                    phase = 3; manager.debuggerCommand( "continue" );
                }
                return true;
            } );
            manager.debuggerCommand( "pause" );
            check( manager.executeSource( "localPaused=1\nlocalPaused=2", "@local-debug.lua" ), "Editor-only pause failed" );
            check( localStops > 0 && phase == 3, "Editor-only pause or asynchronous inspection failed" );
#ifdef WP_LUA_BACKEND_LUAJIT
            struct StepStop
            {
                int line;
                const char *command;
                const char *expression = nullptr, *value = nullptr;
                size_t frames = 1;
                bool editorAction = false;
            };
            const auto stepContract = [&]( const char *name, const char *code, int breakpoint,
                                           std::initializer_list<StepStop> expected ) {
                const auto source = root + "/" + name + ".lua";
                const auto configured = localRequest( "setBreakpoints", {
                    { "source", j::object{ { "path", source } } },
                    { "breakpoints", j::array{ j::object{ { "line", breakpoint } } } } } );
                manager.update();
                check( localReply( configured ).at( "success" ).as_bool(), "Cannot configure LuaJIT stepping fixture" );
                size_t at = 0;
                int inspection = 0, beforeContinue = 0;
                std::string problem;
                const auto until = Clock::now() + std::chrono::seconds( 10 );
                const auto advance = [&]() {
                    const auto &stop = *( expected.begin() + at++ );
                    if( at == expected.size() && std::string( stop.command ) == "continue" )
                    {
                        // Loop contracts deliberately keep this breakpoint
                        // active during every Next assertion. Remove it only
                        // for teardown so Continue can finish later iterations.
                        beforeContinue = localRequest( "setBreakpoints", {
                            { "source", j::object{ { "path", source } } }, { "breakpoints", j::array{} } } );
                    }
                    // Cover both the native Editor action path and the same
                    // structured request handler used by the remote DAP client.
                    if( stop.editorAction ) manager.debuggerCommand( stop.command );
                    else localRequest( stop.command, { { "threadId", 1 } } );
                    inspection = 0;
                };
                manager.setDebuggerPausePump( [&]() {
                    try
                    {
                        check( Clock::now() < until, "LuaJIT stepping fixture timed out" );
                        if( at >= expected.size() )
                        {
                            const auto status = j::parse( manager.getDebuggerStatus().c_str() ).as_object();
                            const auto stack = status.at( "stop" ).as_object().at( "stack" ).as_array();
                            const auto actual = stack.empty() ? -1 : static_cast<int>( stack.front().as_object().at( "line" ).as_int64() );
                            throw std::runtime_error( std::string( name ) + ": extra stop at line " + std::to_string( actual ) );
                        }
                        const auto &stop = *( expected.begin() + at );
                        if( !inspection )
                        {
                            const auto status = j::parse( manager.getDebuggerStatus().c_str() ).as_object();
                            const auto stack = status.at( "stop" ).as_object().at( "stack" ).as_array();
                            const auto actual = static_cast<int>( stack.front().as_object().at( "line" ).as_int64() );
                            check( actual == stop.line && stack.size() == stop.frames,
                                   ( std::string( name ) + ": unexpected step position " + std::to_string( actual ) ).c_str() );
                            if( stop.expression ) inspection = localRequest( "evaluate", { { "expression", stop.expression } } );
                            else advance();
                        }
                        else
                        {
                            const auto reply = localReply( inspection );
                            if( reply.empty() ) return true;
                            check( reply.at( "success" ).as_bool() &&
                                   reply.at( "body" ).as_object().at( "result" ) == stop.value,
                                   ( std::string( name ) + ": step did not execute exactly the expected work" ).c_str() );
                            advance();
                        }
                    }
                    catch( const std::exception &error )
                    {
                        problem = error.what();
                        manager.debuggerCommand( "continue" );
                    }
                    return true;
                } );
                check( manager.executeSource( code, ( "@" + source ).c_str() ), "LuaJIT stepping fixture execution failed" );
                check( problem.empty(), problem.c_str() );
                check( at == expected.size(), ( std::string( name ) + ": missing step stop" ).c_str() );
                if( beforeContinue ) check( localReply( beforeContinue ).at( "success" ).as_bool(),
                                            "Cannot clear stepping breakpoint before Continue" );
                const auto cleared = localRequest( "setBreakpoints", {
                    { "source", j::object{ { "path", source } } }, { "breakpoints", j::array{} } } );
                manager.update(); check( localReply( cleared ).at( "success" ).as_bool(), "Cannot clear stepping fixture" );
                manager.setDebuggerPausePump( {} );
            };
            // The current-line breakpoint deliberately remains active: a
            // redundant return hook must not immediately hit it again either.
            stepContract( "debug-jit-sum", "local function f() return 3 end\nlocal value=f()+f()\nstepSumResult=value\nstepSumResult=stepSumResult+1", 2,
                { { 2, "next" }, { 3, "continue", "value", "6" } } );
            stepContract( "debug-jit-pcall", "local function f() local n=3; return n end\nlocal ok,value=pcall(f)\nstepCResult=value", 2,
                { { 2, "next", nullptr, nullptr, 1, true }, { 3, "continue", "value", "3" } } );
            stepContract( "debug-jit-tail-c", "local function f() return tostring('{}') end\nlocal ok,value=pcall(tostring,f())\nstepTailResult=value", 2,
                { { 2, "next" }, { 3, "continue", "value", "\"{}\"" } } );
            stepContract( "debug-jit-loop", "local n=0\nwhile n<3 do n=n+1 end\nstepLoopResult=n", 2,
                { { 2, "next", "n", "0" }, { 2, "next", "n", "1" }, { 2, "continue", "n", "2" } } );
            stepContract( "debug-jit-call-loop", "local function f() return 1 end\nlocal n=0\nwhile n<3 do n=n+f() end\nstepCallLoopResult=n", 3,
                { { 3, "next", "n", "0", 1, true }, { 3, "next", "n", "1", 1, true }, { 3, "continue", "n", "2" } } );
            stepContract( "debug-jit-tail-c-loop", "local function f() return tonumber('1') end\nlocal n=0\nwhile n<3 do n=n+f() end\nstepTailLoopResult=n", 3,
                { { 3, "next", "n", "0" }, { 3, "next", "n", "1" }, { 3, "continue", "n", "2" } } );
            stepContract( "debug-jit-in-out", "local function f()\n local value=3\n value=value+1\n return value\nend\nlocal result=f()+f()\nstepInResult=result", 6,
                { { 6, "stepIn" }, { 2, "stepOut", nullptr, nullptr, 2 }, { 6, "next" }, { 7, "continue", "result", "8" } } );
            stepContract( "debug-jit-coroutine-step", "local function f() return 2 end\nlocal co=coroutine.create(function()\n local value=f()+f()\n stepCoResult=value\nend)\nassert(coroutine.resume(co))", 3,
                { { 3, "next" }, { 4, "continue", "value", "4" } } );
            check( manager.executeSource( "assert(stepSumResult==7 and stepCResult==3 and stepTailResult=='{}' and stepLoopResult==3 and stepCallLoopResult==3 and stepTailLoopResult==3 and stepInResult==8 and stepCoResult==4)", "=jit-step-results" ),
                   "LuaJIT stepping changed script results" );
            retainedDisconnectContract( root );
#endif
            manager.stopDebugger(); manager.unload( nullptr );
            std::puts( "Lua debugger integration: PASS" );
        }
        catch( const std::exception &error )
        {
            failed = true; manager.stopDebugger();
            if( client.joinable() ) client.join();
            std::fprintf( stderr, "Lua debugger integration: FAIL: %s %s\n", error.what(), clientError.c_str() );
            manager.unload( nullptr ); result = 1;
        }
    }
    TypeManager::setInstance( nullptr ); types.unload(); return result;
}
