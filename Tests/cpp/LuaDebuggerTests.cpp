#include <Workphone/WorkphoneConfig.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <WPLua/LuaManager.hpp>
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
            auto pauseStart = Clock::now(); int boundaries = 0;
            manager.setDebuggerPauseObserver( [&]( bool paused ) {
                ++boundaries;
                if( paused ) pauseStart = Clock::now();
                else timer->excludeElapsed( std::chrono::duration<double>( Clock::now() - pauseStart ).count() );
            } );
            const auto root = std::filesystem::current_path().generic_u8string();
            check( !manager.startDebugger( 0, "short", root.c_str() ), "Short authentication token accepted" );
            check( manager.startDebugger( 0, "workphone-debug-test-token", root.c_str() ), "Debugger did not listen" );
            check( manager.executeSource( "gameplayCalls=0; function gameplayCallback() gameplayCalls=gameplayCalls+1 end", "@gameplay.lua" ), "Gameplay fixture" );
            check( manager.executeSource( "editorCalls=0; function editorCallback() editorCalls=editorCalls+1 end", "@Editor/DebuggerFixture.lua" ), "Editor fixture" );
            check( manager.executeSource( "largeId=9007199254740993; huge={}; for i=1,160 do huge['field'..i]=string.rep('x',1200) end", "@values.lua" ), "Inspection budgets fixture" );
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
                    connection.reply( connection.request( "configurationDone" ) ); connection.reply( attach ); ready = true;
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
