#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <WPLua/LuaDebugger.hpp>
#include <WPLuabind/LuaCompat.hpp>
#include <atomic>
#include <chrono>
#include <deque>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <algorithm>
#include <cctype>
#include <climits>
#ifdef WP_LUA_BACKEND_LUAJIT
extern "C" {
#include <luajit.h>
}
#endif

namespace workphone
{
    namespace j = boost::json;
    using tcp = boost::asio::ip::tcp;
    namespace
    {
        char debuggerKey;
        constexpr size_t maxMessage = 64 * 1024;
        std::string stringValue( const j::object &o, const char *key )
        {
            auto p = o.if_contains( key );
            return p && p->is_string() ? std::string( p->as_string() ) : std::string();
        }
        int integer( const j::object &o, const char *key, int fallback = 0 )
        {
            auto p = o.if_contains( key );
            return p && p->is_int64() && p->as_int64() >= 0 && p->as_int64() <= INT_MAX ?
                static_cast<int>( p->as_int64() ) : fallback;
        }
        bool flag( const j::object &o, const char *key )
        {
            auto p = o.if_contains( key );
            return p && p->is_bool() && p->as_bool();
        }
        bool wouldBlock( const boost::system::error_code &e )
        {
            return e == boost::asio::error::would_block || e == boost::asio::error::try_again;
        }
        struct StackGuard
        {
            lua_State *state;
            int top;
            explicit StackGuard( lua_State *s ) : state( s ), top( lua_gettop( s ) ) {}
            ~StackGuard() { lua_settop( state, top ); }
        };
    }

    struct LuaDebugger::Impl
    {
        struct Source { std::string path, text; int id = 0, lines = 1; };
        struct Reference { int kind = 0, level = 0, registry = LUA_NOREF; };
        struct Packet { unsigned session; j::object request; };
        lua_State *main = nullptr, *stopped = nullptr;
        std::atomic<bool> stopping{ false }, connected{ false }, isPaused{ false }, closeClient{ false };
        std::atomic<unsigned> generation{ 0 };
        unsigned activeGeneration = 0;
        std::atomic<unsigned short> listeningPort{ 0 };
        std::thread network;
        std::mutex mutex;
        std::deque<Packet> requests;
        std::deque<std::pair<unsigned, std::string>> output;
        std::string token, root, exceptionText;
        bool initialized = false, attached = false, configured = false, pauseRequested = false, localMode = false;
        bool breakOnException = false, suppress = false, linesAtOne = true;
        int sequence = 0, nextHandle = 1, nextSource = 1, step = 0, stepDepth = 0;
        j::object pendingAttach;
        std::map<std::string, Source> sources;
        size_t sourceBytes = 0;
        std::map<std::string, std::set<int>> breakpoints;
        std::map<std::string, std::map<int, int>> breakpointIds;
        int nextBreakpoint = 1;
        std::map<int, int> frames;
        std::map<int, Reference> references;
        std::function<bool()> pump;
        std::atomic<int> localAction{ 0 };
        std::string snapshot = "{}";

        static Impl *get( lua_State *state )
        {
            lua_pushlightuserdata( state, &debuggerKey );
            lua_rawget( state, LUA_REGISTRYINDEX );
            auto self = static_cast<Impl *>( lua_touserdata( state, -1 ) );
            lua_pop( state, 1 );
            return self;
        }
        static void hook( lua_State *state, lua_Debug *frame )
        {
            if( auto self = get( state ) )
            {
                if( self->suppress ) return;
                try { self->onHook( state, frame ); }
                catch( const std::exception & ) { self->closeClient = true; self->isPaused = false; }
            }
        }
        std::string path( std::string name ) const
        {
            if( !name.empty() && name[0] == '@' ) name.erase( 0, 1 );
            if( name.empty() || name[0] == '=' ) return name;
            auto p = std::filesystem::u8path( name );
            if( p.is_relative() ) p = std::filesystem::u8path( root ) / p;
            name = p.lexically_normal().generic_u8string();
#ifdef _WIN32
            for( auto &c : name ) if( c >= 'A' && c <= 'Z' ) c += 'a' - 'A';
#endif
            return name;
        }
        void send( j::object message )
        {
            if( !connected ) return;
            message["seq"] = ++sequence;
            auto body = j::serialize( message );
            if( body.size() > maxMessage ) { closeClient = true; return; }
            auto bytes = "Content-Length: " + std::to_string( body.size() ) + "\r\n\r\n" + body;
            std::lock_guard<std::mutex> lock( mutex );
            if( output.size() >= 128 ) { closeClient = true; return; }
            output.emplace_back( activeGeneration, std::move( bytes ) );
        }
        void response( const j::object &request, j::object body = {}, const std::string &error = {} )
        {
            j::object message{ { "type", "response" }, { "request_seq", integer( request, "seq" ) },
                { "command", stringValue( request, "command" ) }, { "success", error.empty() } };
            if( error.empty() ) message["body"] = std::move( body );
            else message["message"] = error;
            send( std::move( message ) );
        }
        void event( const char *name, j::object body = {} )
        {
            send( { { "type", "event" }, { "event", name }, { "body", std::move( body ) } } );
        }
        void releaseReferences()
        {
            if( stopped ) for( const auto &entry : references )
                if( entry.second.registry != LUA_NOREF ) luaL_unref( stopped, LUA_REGISTRYINDEX, entry.second.registry );
            references.clear(); frames.clear();
        }
        void reset()
        {
            releaseReferences();
            stopped = nullptr; isPaused = false; attached = initialized = configured = false;
            pauseRequested = breakOnException = false; step = 0;
            localMode = false;
            breakpoints.clear(); breakpointIds.clear(); pendingAttach.clear(); exceptionText.clear();
        }
        int depth( lua_State *state )
        {
            lua_Debug frame{}; int count = 0;
            while( count < 512 && lua_getstack( state, count, &frame ) ) ++count;
            return count;
        }
        void onHook( lua_State *state, lua_Debug *frame )
        {
            poll();
            if( ( !localMode && ( !attached || !configured || !connected ) ) || stopping ) return;
            bool stop = pauseRequested;
            const char *reason = "pause";
            if( frame->event == LUA_HOOKLINE )
            {
                lua_getinfo( state, "S", frame );
                auto source = path( frame->source ? frame->source : "" );
                auto entry = breakpoints.find( source );
                if( entry != breakpoints.end() && entry->second.count( frame->currentline ) )
                { stop = true; reason = "breakpoint"; }
                if( step && ( step == 1 || ( step == 2 && depth( state ) <= stepDepth ) ||
                                            ( step == 3 && depth( state ) < stepDepth ) ) )
                { stop = true; reason = "step"; }
            }
            if( stop ) pause( state, reason );
        }
        void pause( lua_State *state, const char *reason )
        {
            stopped = state; pauseRequested = false; step = 0; isPaused = true;
            j::array stack;
            lua_Debug frame{};
            for( int level = 0; level < 64 && lua_getstack( state, level, &frame ); ++level )
            {
                lua_getinfo( state, "Sln", &frame );
                stack.push_back( j::object{ { "name", frame.name ? frame.name : "Lua" },
                    { "source", path( frame.source ? frame.source : "" ) }, { "line", frame.currentline } } );
            }
            { std::lock_guard<std::mutex> lock( mutex ); snapshot = j::serialize( j::object{ { "reason", reason }, { "stack", std::move( stack ) } } ); }
            event( "stopped", { { "reason", reason }, { "threadId", 1 }, { "allThreadsStopped", true },
                                { "description", exceptionText } } );
            while( isPaused && ( connected || localMode ) && !stopping && !closeClient )
            {
                poll();
                if( !isPaused || ( !connected && !localMode ) || stopping || closeClient ) break;
                // Editor-only work may re-enter Lua through this pump. Suppress
                // debugger stops there, retain the interrupted frame and restore
                // all inspection temporaries before resuming it.
                if( pump )
                {
                    suppress = true;
                    bool keep = false;
                    try { keep = pump(); } catch( ... ) { suppress = false; throw; }
                    suppress = false;
                    if( !keep ) { closeClient = true; break; }
                }
                std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
            }
            releaseReferences(); stopped = nullptr; isPaused = false;
        }
        j::object sourceObject( const std::string &name )
        {
            auto found = sources.find( name );
            if( found == sources.end() )
            {
                if( !name.empty() && name[0] == '=' ) return { { "name", name.substr( 1 ) }, { "sourceReference", 0 } };
                return { { "name", name }, { "path", name } };
            }
            auto &s = found->second;
            j::object value{ { "name", std::filesystem::u8path( s.path ).filename().u8string() },
                            { "sourceReference", s.id } };
            if( !s.path.empty() && s.path[0] != '=' ) value["path"] = s.path;
            return value;
        }
        std::string display( lua_State *state, int index )
        {
            switch( lua_type( state, index ) )
            {
            case LUA_TNIL: return "nil";
            case LUA_TBOOLEAN: return lua_toboolean( state, index ) ? "true" : "false";
            case LUA_TNUMBER: { std::ostringstream text; text.precision( 17 ); text << lua_tonumber( state, index ); return text.str(); }
            case LUA_TSTRING:
            {
                size_t size = 0; auto text = lua_tolstring( state, index, &size );
                std::string result = "\"";
                for( size_t i = 0; i < size && i < 1024; ++i )
                {
                    const unsigned char c = text[i];
                    if( c < 32 || c >= 127 || c == '\\' || c == '"' )
                    { const char *hex = "0123456789abcdef"; result += "\\x"; result += hex[c >> 4]; result += hex[c & 15]; }
                    else result += char( c );
                }
                return result + ( size > 1024 ? "...\"" : "\"" );
            }
            default: return std::string( "<" ) + lua_typename( state, lua_type( state, index ) ) + ">";
            }
        }
        j::object variable( lua_State *state, const std::string &name, int index )
        {
            int reference = 0;
            if( lua_istable( state, index ) && references.size() < 4096 )
            {
                lua_pushvalue( state, index );
                reference = nextHandle++;
                references[reference] = { 3, 0, luaL_ref( state, LUA_REGISTRYINDEX ) };
            }
            return { { "name", name }, { "value", display( state, index ) },
                     { "type", lua_typename( state, lua_type( state, index ) ) }, { "variablesReference", reference } };
        }
        bool lookup( lua_State *state, int level, const std::string &expression )
        {
            // Inspection is read-only. No __index, __tostring, functions or Lua
            // evaluation can run arbitrary engine code while a frame is stopped.
            size_t at = 0;
            auto identifier = [&]() {
                const auto begin = at;
                while( at < expression.size() && ( std::isalnum( static_cast<unsigned char>( expression[at] ) ) || expression[at] == '_' ) ) ++at;
                return expression.substr( begin, at - begin );
            };
            auto name = identifier();
            if( name.empty() || std::isdigit( static_cast<unsigned char>( name[0] ) ) ) return false;
            bool found = false;
            lua_Debug frame{};
            if( lua_getstack( state, level, &frame ) )
            {
                for( int i = 1; const char *local = lua_getlocal( state, &frame, i ); ++i )
                {
                    if( name == local )
                    {
                        if( found ) lua_remove( state, -2 );
                        found = true;
                    }
                    else lua_pop( state, 1 );
                }
                if( !found )
                {
                    lua_getinfo( state, "f", &frame );
                    for( int i = 1; const char *upvalue = lua_getupvalue( state, -1, i ); ++i )
                    {
                        if( name == upvalue ) { lua_remove( state, -2 ); found = true; break; }
                        lua_pop( state, 1 );
                    }
                    if( !found ) lua_pop( state, 1 );
                }
            }
            if( !found ) { wpLuaPushGlobalTable( state ); lua_pushlstring( state, name.data(), name.size() ); lua_rawget( state, -2 ); lua_remove( state, -2 ); }
            while( at < expression.size() )
            {
                if( !lua_istable( state, -1 ) ) return false;
                if( expression[at] == '.' )
                { ++at; name = identifier(); if( name.empty() ) return false; lua_pushlstring( state, name.data(), name.size() ); }
                else if( expression[at] == '[' )
                {
                    ++at; const auto begin = at;
                    if( at < expression.size() && ( expression[at] == '"' || expression[at] == '\'' ) )
                    {
                        const auto quote = expression[at++]; const auto start = at;
                        while( at < expression.size() && expression[at] != quote ) ++at;
                        if( at == expression.size() ) return false;
                        lua_pushlstring( state, expression.data() + start, at - start ); ++at;
                    }
                    else
                    {
                        while( at < expression.size() && std::isdigit( static_cast<unsigned char>( expression[at] ) ) ) ++at;
                        if( at == begin || at - begin > 9 ) return false;
                        lua_pushinteger( state, std::stoi( expression.substr( begin, at - begin ) ) );
                    }
                    if( at == expression.size() || expression[at++] != ']' ) return false;
                }
                else return false;
                lua_rawget( state, -2 ); lua_remove( state, -2 );
            }
            return true;
        }
        void handle( const j::object &request )
        {
            const auto command = stringValue( request, "command" );
            j::object empty;
            auto rawArgs = request.if_contains( "arguments" );
            const auto &args = rawArgs && rawArgs->is_object() ? rawArgs->as_object() : empty;
            if( command == "initialize" )
            {
                if( initialized ) { response( request, {}, "Already initialized" ); return; }
                initialized = true;
                if( auto p = args.if_contains( "linesStartAt1" ); p && p->is_bool() ) linesAtOne = p->as_bool();
                response( request, { { "supportsConfigurationDoneRequest", true }, { "supportsEvaluateForHovers", true },
                    { "supportsExceptionInfoRequest", true }, { "supportsLoadedSourcesRequest", true },
                    { "exceptionBreakpointFilters", j::array{ j::object{ { "filter", "uncaught" }, { "label", "Uncaught Lua errors" }, { "default", false } } } } } );
                return;
            }
            if( !initialized ) { response( request, {}, "Initialize first" ); return; }
            if( command == "attach" )
            {
                const auto supplied = stringValue( args, "token" );
                size_t difference = supplied.size() ^ token.size();
                for( size_t i = 0; i < token.size(); ++i ) difference |= token[i] ^ ( i < supplied.size() ? supplied[i] : 0 );
                if( difference || attached ) { response( request, {}, "Attach denied" ); return; }
                attached = true; pendingAttach = request;
                pauseRequested = flag( args, "stopOnEntry" );
                event( "initialized" ); return;
            }
            if( !attached ) { response( request, {}, "Authenticate with attach first" ); return; }
            if( command == "configurationDone" )
            { configured = true; response( request ); if( !pendingAttach.empty() ) { response( pendingAttach ); pendingAttach.clear(); } return; }
            if( command == "disconnect" )
            { response( request ); event( "terminated" ); attached = false; isPaused = false; step = 0; return; }
            if( command == "threads" ) { response( request, { { "threads", j::array{ j::object{ { "id", 1 }, { "name", "Lua executor" } } } } } ); return; }
            if( command == "setBreakpoints" )
            {
                auto s = args.if_contains( "source" );
                if( !s || !s->is_object() ) { response( request, {}, "Source is required" ); return; }
                auto source = path( stringValue( s->as_object(), "path" ) );
                if( source.empty() ) { response( request, {}, "A source path is required" ); return; }
                if( ( !breakpoints.count( source ) && breakpoints.size() >= 256 ) || nextBreakpoint > INT_MAX - 1024 )
                { response( request, {}, "Breakpoint source limit reached" ); return; }
                auto &lines = breakpoints[source]; lines.clear(); j::array result;
                if( auto b = args.if_contains( "breakpoints" ); b && b->is_array() )
                {
                    if( b->as_array().size() > 1024 ) { response( request, {}, "Too many breakpoints" ); return; }
                    for( const auto &item : b->as_array() )
                    {
                        if( !item.is_object() ) continue;
                        const auto &o = item.as_object(); const int line = integer( o, "line", -1 ) + ( linesAtOne ? 0 : 1 );
                        bool valid = line > 0 && !o.if_contains( "condition" ) && !o.if_contains( "hitCondition" ) && !o.if_contains( "logMessage" );
                        int id = 0;
                        if( valid )
                        {
                            lines.insert( line );
                            auto &assigned = breakpointIds[source][line];
                            if( !assigned ) assigned = nextBreakpoint++;
                            id = assigned;
                        }
                        const auto known = sources.find( source );
                        result.push_back( j::object{ { "id", id }, { "verified", valid && known != sources.end() && line <= known->second.lines },
                            { "line", line - ( linesAtOne ? 0 : 1 ) }, { "message", valid ? "Bound when this source executes" : "Invalid line or unsupported conditional breakpoint" } } );
                    }
                }
                response( request, { { "breakpoints", std::move( result ) } } ); return;
            }
            if( command == "setExceptionBreakpoints" )
            {
                breakOnException = false;
                if( auto filters = args.if_contains( "filters" ); filters && filters->is_array() )
                    for( auto &filter : filters->as_array() ) if( filter.is_string() && filter.as_string() == "uncaught" ) breakOnException = true;
                response( request ); return;
            }
            if( command == "pause" ) { pauseRequested = true; response( request ); return; }
            if( command == "loadedSources" )
            { j::array result; for( auto &s : sources ) result.push_back( sourceObject( s.first ) ); response( request, { { "sources", std::move( result ) } } ); return; }
            if( command == "source" )
            {
                for( auto &s : sources ) if( s.second.id == integer( args, "sourceReference" ) )
                { if( s.second.text.size() > 48000 ) response( request, {}, "Source exceeds transfer limit; open its file" ); else response( request, { { "content", s.second.text }, { "mimeType", "text/x-lua" } } ); return; }
                response( request, {}, "Unknown source" ); return;
            }
            if( !isPaused || !stopped ) { response( request, {}, "Lua is running" ); return; }
            StackGuard guard( stopped );
            if( command == "continue" || command == "next" || command == "stepIn" || command == "stepOut" )
            {
                stepDepth = depth( stopped );
                step = command == "stepIn" ? 1 : command == "next" ? 2 : command == "stepOut" ? 3 : 0;
                response( request, { { "allThreadsContinued", true } } );
                event( "continued", { { "threadId", 1 }, { "allThreadsContinued", true } } ); isPaused = false; return;
            }
            if( command == "stackTrace" )
            {
                if( frames.size() + references.size() > 4096 || nextHandle > INT_MAX - 512 )
                { response( request, {}, "Inspection handle limit reached; continue to release handles" ); return; }
                j::array result; lua_Debug frame{};
                const int start = integer( args, "startFrame" ); const int count = std::min( 128, integer( args, "levels", 128 ) );
                const int total = depth( stopped );
                for( int level = start; level < total && result.size() < static_cast<size_t>( count ? count : 128 ); ++level )
                {
                    lua_getstack( stopped, level, &frame ); lua_getinfo( stopped, "Sln", &frame );
                    const int id = nextHandle++; frames[id] = level;
                    result.push_back( j::object{ { "id", id }, { "name", frame.name ? frame.name : frame.what ? frame.what : "Lua" },
                        { "source", sourceObject( path( frame.source ? frame.source : "" ) ) },
                        { "line", std::max( 0, frame.currentline - ( linesAtOne ? 0 : 1 ) ) }, { "column", 1 } } );
                }
                response( request, { { "stackFrames", std::move( result ) }, { "totalFrames", total } } ); return;
            }
            if( command == "scopes" )
            {
                if( references.size() > 4090 || nextHandle > INT_MAX - 512 )
                { response( request, {}, "Inspection handle limit reached" ); return; }
                auto frame = frames.find( integer( args, "frameId" ) );
                if( frame == frames.end() ) { response( request, {}, "Stale frame" ); return; }
                j::array scopes;
                for( int kind = 0; kind < 3; ++kind )
                { const int id = nextHandle++; references[id] = { kind, frame->second, LUA_NOREF }; scopes.push_back( j::object{ { "name", kind == 0 ? "Locals" : kind == 1 ? "Upvalues" : "Globals" }, { "variablesReference", id }, { "expensive", kind == 2 } } ); }
                response( request, { { "scopes", std::move( scopes ) } } ); return;
            }
            if( command == "variables" )
            {
                auto ref = references.find( integer( args, "variablesReference" ) );
                if( ref == references.end() ) { response( request, {}, "Stale variable reference" ); return; }
                auto scope = ref->second; j::array values; lua_Debug frame{};
                const int start = integer( args, "start" ); const int count = std::min( 256, integer( args, "count", 256 ) ); int seen = 0;
                auto add = [&]( const std::string &name ) { if( seen++ >= start && values.size() < static_cast<size_t>( count ? count : 256 ) ) values.push_back( variable( stopped, name, -1 ) ); };
                if( scope.kind <= 1 && lua_getstack( stopped, scope.level, &frame ) )
                {
                    if( scope.kind == 1 ) lua_getinfo( stopped, "f", &frame );
                    for( int i = 1; i <= 4096; ++i )
                    {
                        auto name = scope.kind == 0 ? lua_getlocal( stopped, &frame, i ) : lua_getupvalue( stopped, -1, i );
                        if( !name ) break;
                        if( name[0] != '(' ) add( name ); lua_pop( stopped, 1 );
                    }
                }
                else if( scope.kind >= 2 )
                {
                    if( scope.kind == 2 ) wpLuaPushGlobalTable( stopped ); else lua_rawgeti( stopped, LUA_REGISTRYINDEX, scope.registry );
                    lua_pushnil( stopped ); int visited = 0;
                    while( visited++ < 4096 && lua_next( stopped, -2 ) )
                    {
                        auto name = lua_type( stopped, -2 ) == LUA_TSTRING ? std::string( lua_tostring( stopped, -2 ) ) : display( stopped, -2 );
                        add( name ); lua_pop( stopped, 1 );
                        if( values.size() >= static_cast<size_t>( count ? count : 256 ) ) break;
                    }
                }
                response( request, { { "variables", std::move( values ) } } ); return;
            }
            if( command == "evaluate" )
            {
                auto expression = stringValue( args, "expression" );
                int level = 0;
                if( auto id = args.if_contains( "frameId" ) )
                { auto frame = frames.find( integer( args, "frameId" ) ); if( frame == frames.end() ) { response( request, {}, "Stale frame" ); return; } level = frame->second; }
                if( expression.size() > 1024 || !lookup( stopped, level, expression ) )
                { response( request, {}, "Read-only evaluation supports names and table paths, e.g. actor.position[1]" ); return; }
                auto value = variable( stopped, expression, -1 ); auto result = value.at( "value" );
                value["result"] = std::move( result ); value.erase( "name" ); value.erase( "value" ); response( request, std::move( value ) ); return;
            }
            if( command == "exceptionInfo" ) { response( request, { { "exceptionId", "LuaError" }, { "description", exceptionText }, { "breakMode", "unhandled" } } ); return; }
            response( request, {}, "Unsupported debugger request" );
        }
        void poll()
        {
            const int action = localAction.exchange( 0 );
            if( action == 1 ) { pauseRequested = true; if( !attached ) localMode = true; }
            else if( action >= 2 && isPaused )
            {
                stepDepth = depth( stopped ); step = action - 2;
                event( "continued", { { "threadId", 1 }, { "allThreadsContinued", true } } );
                isPaused = false;
            }
            if( activeGeneration != generation )
            { reset(); activeGeneration = generation; }
            for( int count = 0; count < 64; ++count )
            {
                Packet packet;
                { std::lock_guard<std::mutex> lock( mutex ); if( requests.empty() ) break; packet = std::move( requests.front() ); requests.pop_front(); }
                if( packet.session != activeGeneration || !connected ) continue;
                try { handle( packet.request ); }
                catch( const std::exception &error ) { response( packet.request, {}, error.what() ); }
            }
        }
        void serve( std::unique_ptr<tcp::acceptor> listener )
        {
            tcp::socket socket( listener->get_executor() );
            std::string input, sending; size_t sent = 0; unsigned session = 0;
            char buffer[4096];
            while( !stopping )
            {
                boost::system::error_code error;
                if( !socket.is_open() )
                {
                    listener->accept( socket, error );
                    if( !error )
                    { socket.non_blocking( true ); input.clear(); sending.clear(); sent = 0; closeClient = false; session = ++generation; connected = true; }
                    else if( !wouldBlock( error ) ) break;
                }
                else
                {
                    auto count = socket.read_some( boost::asio::buffer( buffer ), error );
                    if( error && !wouldBlock( error ) ) closeClient = true;
                    if( !error ) input.append( buffer, count );
                    while( !closeClient )
                    {
                        auto end = input.find( "\r\n\r\n" );
                        if( end == std::string::npos ) { if( input.size() > 256 ) closeClient = true; break; }
                        if( end > 256 || input.compare( 0, 16, "Content-Length: " ) != 0 ) { closeClient = true; break; }
                        const auto header = input.substr( 16, end - 16 );
                        if( header.empty() || header.size() > 6 || header.find_first_not_of( "0123456789" ) != std::string::npos ) { closeClient = true; break; }
                        const size_t length = std::stoul( header );
                        if( !length || length > maxMessage ) { closeClient = true; break; }
                        if( input.size() < end + 4 + length ) break;
                        boost::system::error_code parseError;
                        auto value = j::parse( j::string_view( input.data() + end + 4, length ), parseError );
                        input.erase( 0, end + 4 + length );
                        if( parseError || !value.is_object() || stringValue( value.as_object(), "type" ) != "request" || integer( value.as_object(), "seq" ) <= 0 ) { closeClient = true; break; }
                        std::lock_guard<std::mutex> lock( mutex );
                        if( requests.size() >= 256 ) { closeClient = true; break; }
                        requests.push_back( { session, std::move( value.as_object() ) } );
                    }
                    if( sending.empty() )
                    {
                        std::lock_guard<std::mutex> lock( mutex );
                        while( !output.empty() )
                        { auto item = std::move( output.front() ); output.pop_front(); if( item.first == session ) { sending = std::move( item.second ); sent = 0; break; } }
                    }
                    if( !sending.empty() )
                    {
                        sent += socket.write_some( boost::asio::buffer( sending.data() + sent, sending.size() - sent ), error );
                        if( error && !wouldBlock( error ) ) closeClient = true;
                        if( sent == sending.size() ) sending.clear();
                    }
                    if( closeClient )
                    { socket.close( error ); connected = false; ++generation; std::lock_guard<std::mutex> lock( mutex ); requests.clear(); output.clear(); closeClient = false; }
                }
                std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
            }
            boost::system::error_code ignored; socket.close( ignored ); listener->close( ignored ); connected = false; ++generation;
        }
    };

    LuaDebugger::LuaDebugger() : m_impl( std::make_unique<Impl>() ) {}
    LuaDebugger::~LuaDebugger() { stop(); }
    bool LuaDebugger::start( lua_State *state, unsigned short port, const std::string &token,
                             const std::string &root, std::string &error )
    {
        stop();
        if( !state || token.size() < 16 || root.empty() ) { error = "A loaded VM, source root and token of at least 16 characters are required"; return false; }
        try
        {
            // io_context outlives its acceptor/socket through the network closure.
            auto io = std::make_shared<boost::asio::io_context>();
            auto listener = std::make_unique<tcp::acceptor>( *io, tcp::endpoint( boost::asio::ip::address_v4::loopback(), port ) );
            listener->non_blocking( true ); m_impl->listeningPort = listener->local_endpoint().port();
            m_impl->token = token; m_impl->root = root; m_impl->stopping = false;
            auto registered = std::move( m_impl->sources );
            m_impl->sources.clear(); m_impl->sourceBytes = 0;
            for( const auto &entry : registered ) source( entry.second.path, entry.second.text );
            bind( state );
            m_impl->network = std::thread( [self = m_impl.get(), io, listener = std::move( listener )]() mutable {
                try { self->serve( std::move( listener ) ); } catch( ... ) { self->connected = false; ++self->generation; }
            } );
            return true;
        }
        catch( const std::exception &e ) { error = e.what(); stop(); return false; }
    }
    void LuaDebugger::bind( lua_State *state )
    {
        if( m_impl->main )
        {
            lua_sethook( m_impl->main, nullptr, 0, 0 );
            lua_pushlightuserdata( m_impl->main, &debuggerKey ); lua_pushnil( m_impl->main ); lua_rawset( m_impl->main, LUA_REGISTRYINDEX );
        }
        m_impl->main = state;
        if( state )
        {
            lua_pushlightuserdata( state, &debuggerKey ); lua_pushlightuserdata( state, m_impl.get() ); lua_rawset( state, LUA_REGISTRYINDEX );
            lua_sethook( state, Impl::hook, LUA_MASKLINE | LUA_MASKCOUNT, 1000 );
#ifdef WP_LUA_BACKEND_LUAJIT
            luaJIT_setmode( state, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF );
#endif
        }
    }
    void LuaDebugger::stop()
    {
        m_impl->stopping = true;
        if( m_impl->network.joinable() ) m_impl->network.join();
        m_impl->reset(); bind( nullptr ); m_impl->listeningPort = 0;
        std::lock_guard<std::mutex> lock( m_impl->mutex ); m_impl->requests.clear(); m_impl->output.clear();
    }
    void LuaDebugger::poll() { m_impl->poll(); }
    void LuaDebugger::source( const std::string &name, const std::string &text )
    {
        auto key = m_impl->path( name );
        const auto existing = m_impl->sources.find( key );
        if( existing != m_impl->sources.end() && existing->second.text == text ) return;
        const auto previousSize = existing == m_impl->sources.end() ? 0 : existing->second.text.size();
        if( text.size() > 8 * 1024 * 1024 ||
            ( existing == m_impl->sources.end() && m_impl->sources.size() >= 256 ) ||
            m_impl->sourceBytes - previousSize + text.size() > 64 * 1024 * 1024 ) return;
        m_impl->sourceBytes = m_impl->sourceBytes - previousSize + text.size();
        auto &source = m_impl->sources[key];
        source.path = key; source.text = text; source.lines = 1 + static_cast<int>( std::count( text.begin(), text.end(), '\n' ) );
        if( !source.id ) source.id = m_impl->nextSource++;
        if( m_impl->attached ) m_impl->event( "loadedSource", { { "reason", "new" }, { "source", m_impl->sourceObject( key ) } } );
        if( m_impl->attached )
            for( int line : m_impl->breakpoints[key] )
                m_impl->event( "breakpoint", { { "reason", "changed" }, { "breakpoint", j::object{
                    { "id", m_impl->breakpointIds[key][line] }, { "verified", line <= source.lines },
                    { "line", line - ( m_impl->linesAtOne ? 0 : 1 ) }, { "source", m_impl->sourceObject( key ) } } } } );
    }
    void LuaDebugger::exception( lua_State *state, const char *message )
    {
        if( m_impl->suppress ) return;
        m_impl->poll();
        if( m_impl->attached && m_impl->configured && m_impl->breakOnException && !m_impl->suppress )
        {
            m_impl->exceptionText = message ? message : "Lua error";
            try { m_impl->pause( state, "exception" ); }
            catch( ... )
            {
                m_impl->closeClient = true; m_impl->isPaused = false;
                m_impl->releaseReferences(); m_impl->stopped = nullptr;
            }
            m_impl->exceptionText.clear();
        }
    }
    unsigned short LuaDebugger::port() const { return m_impl->listeningPort; }
    bool LuaDebugger::paused() const { return m_impl->isPaused; }
    void LuaDebugger::setPausePump( std::function<bool()> pump ) { m_impl->pump = std::move( pump ); }
    void LuaDebugger::command( const std::string &command )
    {
        const int action = command == "pause" ? 1 : command == "continue" ? 2 : command == "stepIn" ? 3 : command == "next" ? 4 : command == "stepOut" ? 5 : 0;
        if( action ) m_impl->localAction = action;
    }
    std::string LuaDebugger::status() const
    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        return j::serialize( j::object{ { "port", port() }, { "paused", paused() },
            { "connected", m_impl->connected.load() }, { "stop", j::parse( m_impl->snapshot ) } } );
    }
    bool LuaDebugger::callbackAllowed( lua_State *state, int index ) const
    {
        if( !paused() ) return true;
        if( !m_impl->suppress ) return false;
        lua_Debug info{};
        lua_pushvalue( state, index );
        if( !lua_getinfo( state, ">S", &info ) ) return false;
        const auto source = m_impl->path( info.source ? info.source : "" );
        // Paused input/UI dispatch may run Editor scripts on the same executor;
        // gameplay listeners must remain suspended along with simulation ticks.
        return source.find( "/editor/" ) != std::string::npos ||
               source.find( "/Editor/" ) != std::string::npos;
    }
    void LuaDebugger::copySourcesFrom( const LuaDebugger &other )
    {
        for( const auto &entry : other.m_impl->sources ) source( entry.second.path, entry.second.text );
    }
}
