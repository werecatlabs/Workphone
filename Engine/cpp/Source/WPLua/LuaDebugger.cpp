#include <WPLua/LuaDebugger.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
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
#include <cstdlib>
#ifdef _MSC_VER
// Engine math uses /fp:fast; debugger expressions must retain Lua's NaN and
// infinity comparison semantics regardless of renderer optimization flags.
#pragma float_control(precise, on, push)
#endif
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
        struct StopRule
        {
            std::string condition, message;
            unsigned long long hits = 0, target = 0;
            char comparison = '=';
            bool log = false, disabled = false;
        };
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
        std::map<std::string, std::map<int, StopRule>> stopRules;
        int nextBreakpoint = 1;
        std::map<int, int> frames;
        std::map<int, Reference> references;
        std::function<bool()> pump;
        int hookedThreads = LUA_NOREF;
        std::atomic<int> localAction{ 0 };
        bool abortExecution = false;
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
                if( self->hookedThreads != LUA_NOREF )
                {
                    lua_rawgeti( state, LUA_REGISTRYINDEX, self->hookedThreads );
                    lua_pushthread( state ); lua_pushboolean( state, 1 ); lua_rawset( state, -3 ); lua_pop( state, 1 );
                }
                try { self->onHook( state, frame ); }
                catch( const std::exception & ) { self->closeClient = true; self->isPaused = false; }
                if( self->abortExecution )
                {
                    self->abortExecution = false;
                    // All C++ frames with non-trivial locals have returned.
                    lua_pushliteral( state, "Lua execution cancelled by the debugger host" );
                    lua_error( state );
                }
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
            if( body.size() > maxMessage )
            {
                if( stringValue( message, "type" ) != "response" ) return;
                message.erase( "body" ); message["success"] = false;
                message["message"] = "Debugger response exceeds 64 KiB; request fewer variables or frames";
                body = j::serialize( message );
            }
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
            abortExecution = false;
            breakpoints.clear(); breakpointIds.clear(); stopRules.clear(); pendingAttach.clear(); exceptionText.clear();
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
                {
                    auto &rule = stopRules[source][frame->currentline];
                    StackGuard guard( state );
                    if( rule.hits != std::numeric_limits<unsigned long long>::max() ) ++rule.hits;
                    bool hit = !rule.disabled && ( !rule.target || ( rule.comparison == '=' ? rule.hits == rule.target :
                        rule.comparison == '%' ? rule.hits % rule.target == 0 : rule.comparison == '>' ? rule.hits > rule.target : rule.hits >= rule.target ) );
                    try
                    {
                        if( hit && !rule.condition.empty() ) hit = Expression( *this, state, 0, rule.condition ).parse().truth();
                        if( hit && rule.log ) event( "output", { { "category", "console" },
                            { "output", logText( state, rule.message, true ) + "\n" }, { "source", sourceObject( source ) },
                            { "line", frame->currentline - ( linesAtOne ? 0 : 1 ) } } );
                        else if( hit ) { stop = true; reason = "breakpoint"; }
                    }
                    catch( const std::exception &error )
                    {
                        rule.disabled = true;
                        event( "breakpoint", { { "reason", "changed" }, { "breakpoint", j::object{
                            { "id", breakpointIds[source][frame->currentline] }, { "verified", false }, { "message", error.what() } } } } );
                        event( "output", { { "category", "stderr" }, { "output", std::string( "Breakpoint disabled: " ) + error.what() + "\n" } } );
                    }
                }
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
                    if( !keep ) { closeClient = true; abortExecution = true; break; }
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
            if( !lua_checkstack( state, 8 ) ) throw std::runtime_error( "Lua inspection stack limit reached" );
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
        struct Value
        {
            enum Kind { Nil, Boolean, Number, String, Object } kind = Nil;
            bool boolean = false;
            double number = 0;
            std::string text;
            int index = 0;
            const void *pointer = nullptr;
            bool truth() const { return kind != Nil && ( kind != Boolean || boolean ); }
        };
        // A bounded observer expression language. Never executes Lua functions,
        // metamethods, assignments or native bindings in a stopped program.
        struct Expression
        {
            Impl &owner; lua_State *state; int level; const std::string &input;
            size_t at = 0; int recursion = 0;
            struct Token { std::string text; int kind = 0; } token;
            Expression( Impl &o, lua_State *s, int l, const std::string &i ) : owner( o ), state( s ), level( l ), input( i ) { next(); }
            [[noreturn]] void error() { throw std::runtime_error( "Unsupported read-only expression near byte " + std::to_string( at ) ); }
            void next()
            {
                while( at < input.size() && std::isspace( static_cast<unsigned char>( input[at] ) ) ) ++at;
                token = {};
                if( at == input.size() ) return;
                const auto begin = at; const char c = input[at++];
                if( std::isalpha( static_cast<unsigned char>( c ) ) || c == '_' )
                {
                    while( at < input.size() && ( std::isalnum( static_cast<unsigned char>( input[at] ) ) || input[at] == '_' ) ) ++at;
                    token = { input.substr( begin, at - begin ), 1 }; return;
                }
                if( std::isdigit( static_cast<unsigned char>( c ) ) ||
                    ( c == '.' && at < input.size() && std::isdigit( static_cast<unsigned char>( input[at] ) ) ) )
                {
                    while( at < input.size() && ( std::isdigit( static_cast<unsigned char>( input[at] ) ) ||
                        ( input[at] == '.' && ( at + 1 == input.size() || input[at + 1] != '.' ) ) ) ) ++at;
                    if( at < input.size() && ( input[at] == 'e' || input[at] == 'E' ) )
                    { ++at; if( at < input.size() && ( input[at] == '+' || input[at] == '-' ) ) ++at; while( at < input.size() && std::isdigit( static_cast<unsigned char>( input[at] ) ) ) ++at; }
                    token = { input.substr( begin, at - begin ), 2 }; return;
                }
                if( c == '\'' || c == '"' )
                {
                    token.kind = 3;
                    while( at < input.size() && input[at] != c )
                    {
                        char value = input[at++];
                        if( value == '\\' )
                        {
                            if( at == input.size() ) error(); value = input[at++];
                            if( value == 'n' ) value = '\n'; else if( value == 'r' ) value = '\r'; else if( value == 't' ) value = '\t';
                            else if( value != '\\' && value != '\'' && value != '"' ) error();
                        }
                        token.text += value;
                    }
                    if( at == input.size() ) error(); ++at; return;
                }
                token = { std::string( 1, c ), 4 };
                if( at < input.size() && ( ( ( c == '=' || c == '~' || c == '<' || c == '>' ) && input[at] == '=' ) || ( c == '.' && input[at] == '.' ) ) ) token.text += input[at++];
            }
            int precedence() const
            {
                if( token.kind != 1 && token.kind != 4 ) return 0;
                const auto &s = token.text;
                if( token.kind == 1 && s != "and" && s != "or" ) return 0;
                return s == "or" ? 1 : s == "and" ? 2 : s == "==" || s == "~=" || s == "<" || s == ">" || s == "<=" || s == ">=" ? 3 :
                    s == ".." ? 4 : s == "+" || s == "-" ? 5 : s == "*" || s == "/" || s == "%" ? 6 : s == "^" ? 8 : 0;
            }
            Value scalar( const std::string &text )
            {
                Value value; value.kind = Value::Number; char *end = nullptr;
                value.number = std::strtod( text.c_str(), &end ); if( !end || *end ) error(); return value;
            }
            Value primary( bool run )
            {
                Value value;
                if( token.kind == 4 && token.text == "(" ) { next(); value = expression( 1, run ); if( token.text != ")" || token.kind != 4 ) error(); next(); return value; }
                if( ( token.kind == 4 && ( token.text == "-" || token.text == "#" ) ) || ( token.kind == 1 && token.text == "not" ) )
                {
                    const auto op = token.text; next(); value = expression( 7, run ); if( !run ) return {};
                    if( op == "not" ) { value.boolean = !value.truth(); value.kind = Value::Boolean; }
                    else if( op == "-" && value.kind == Value::Number ) value.number = -value.number;
                    else if( op == "#" && value.kind == Value::String ) { value.number = static_cast<double>( value.text.size() ); value.kind = Value::Number; }
                    else error(); return value;
                }
                if( token.kind == 2 ) { value = scalar( token.text ); next(); return value; }
                if( token.kind == 3 ) { value.kind = Value::String; value.text = token.text; next(); return value; }
                if( token.kind != 1 ) error();
                auto name = token.text; next();
                if( name == "nil" ) return value;
                if( name == "true" || name == "false" ) { value.kind = Value::Boolean; value.boolean = name == "true"; return value; }
                while( token.kind == 4 && ( token.text == "." || token.text == "[" ) )
                {
                    const auto op = token.text; next();
                    if( op == "." ) { if( token.kind != 1 ) error(); name += "." + token.text; next(); }
                    else
                    {
                        if( token.kind == 2 && token.text.find_first_not_of( "0123456789" ) == std::string::npos ) name += "[" + token.text + "]";
                        else if( token.kind == 3 && token.text.find_first_of( "\"\\" ) == std::string::npos ) name += "[\"" + token.text + "\"]";
                        else error();
                        next(); if( token.text != "]" ) error(); next();
                    }
                }
                if( !run ) return {};
                if( !owner.lookup( state, level, name ) ) error();
                const int index = lua_gettop( state );
                switch( lua_type( state, -1 ) )
                {
                case LUA_TNIL: break;
                case LUA_TBOOLEAN: value.kind = Value::Boolean; value.boolean = lua_toboolean( state, -1 ) != 0; break;
                case LUA_TNUMBER: value.kind = Value::Number; value.number = lua_tonumber( state, -1 ); break;
                case LUA_TSTRING: { value.kind = Value::String; size_t size; auto p = lua_tolstring( state, -1, &size ); if( size > 4096 ) throw std::runtime_error( "String operand exceeds debugger limit" ); value.text.assign( p, size ); break; }
                default: value.kind = Value::Object; value.index = index; value.pointer = lua_topointer( state, -1 ); return value;
                }
                lua_pop( state, 1 ); return value;
            }
            Value expression( int minimum, bool run )
            {
                if( ++recursion > 64 ) error();
                auto left = primary( run );
                for( int p; ( p = precedence() ) >= minimum; )
                {
                    const auto op = token.text; next();
                    auto right = expression( p + ( op == "^" || op == ".." ? 0 : 1 ), run && !( op == "and" && !left.truth() ) && !( op == "or" && left.truth() ) );
                    if( !run ) continue;
                    if( op == "and" ) { if( left.truth() ) left = std::move( right ); continue; }
                    if( op == "or" ) { if( !left.truth() ) left = std::move( right ); continue; }
                    if( op == "==" || op == "~=" )
                    {
                        bool equal = left.kind == right.kind && ( left.kind == Value::Nil || ( left.kind == Value::Boolean && left.boolean == right.boolean ) ||
                            ( left.kind == Value::Number && left.number == right.number ) || ( left.kind == Value::String && left.text == right.text ) || ( left.kind == Value::Object && left.pointer == right.pointer ) );
                        left.kind = Value::Boolean; left.boolean = op == "==" ? equal : !equal; continue;
                    }
                    if( op == "<" || op == ">" || op == "<=" || op == ">=" )
                    {
                        if( left.kind == Value::Number && right.kind == Value::Number )
                            left.boolean = op == "<" ? left.number < right.number : op == "<=" ? left.number <= right.number : op == ">" ? left.number > right.number : left.number >= right.number;
                        else if( left.kind == Value::String && right.kind == Value::String )
                            left.boolean = op == "<" ? left.text < right.text : op == "<=" ? left.text <= right.text : op == ">" ? left.text > right.text : left.text >= right.text;
                        else error();
                        left.kind = Value::Boolean;
                        continue;
                    }
                    if( op == ".." )
                    {
                        if( left.kind != Value::String || right.kind != Value::String || left.text.size() + right.text.size() > 4096 ) error();
                        left.text += right.text; continue;
                    }
                    if( left.kind != Value::Number || right.kind != Value::Number ) error();
                    if( op == "+" ) left.number += right.number; else if( op == "-" ) left.number -= right.number;
                    else if( op == "*" ) left.number *= right.number; else if( op == "/" ) left.number /= right.number;
                    else if( op == "%" ) left.number -= std::floor( left.number / right.number ) * right.number;
                    else if( op == "^" ) left.number = std::pow( left.number, right.number );
                }
                --recursion; return left;
            }
            Value parse( bool run = true )
            {
                if( input.empty() || input.size() > 1024 ) error();
                auto value = expression( 1, run ); if( token.kind ) error(); return value;
            }
            void push( const Value &value )
            {
                switch( value.kind )
                {
                case Value::Nil: lua_pushnil( state ); break;
                case Value::Boolean: lua_pushboolean( state, value.boolean ); break;
                case Value::Number: lua_pushnumber( state, value.number ); break;
                case Value::String: lua_pushlstring( state, value.text.data(), value.text.size() ); break;
                case Value::Object: lua_pushvalue( state, value.index ); break;
                }
            }
        };
        std::string logText( lua_State *state, const std::string &message, bool run )
        {
            if( message.size() > 4096 ) throw std::runtime_error( "Logpoint message exceeds limit" );
            std::string text;
            for( size_t at = 0; at < message.size(); )
            {
                const char c = message[at++];
                if( ( c == '{' || c == '}' ) && at < message.size() && message[at] == c ) { text += c; ++at; continue; }
                if( c == '}' ) throw std::runtime_error( "Unmatched logpoint brace" );
                if( c != '{' ) { text += c; continue; }
                const auto begin = at; char quote = 0;
                for( ; at < message.size(); ++at )
                {
                    const char current = message[at];
                    if( quote ) { if( current == '\\' ) ++at; else if( current == quote ) quote = 0; }
                    else if( current == '\'' || current == '"' ) quote = current;
                    else if( current == '}' ) break;
                }
                if( at >= message.size() ) throw std::runtime_error( "Unclosed logpoint expression" );
                const auto expression = message.substr( begin, at++ - begin );
                Expression evaluator( *this, state, 0, expression ); auto result = evaluator.parse( run );
                if( run )
                { evaluator.push( result ); text += display( state, -1 ); lua_pop( state, 1 ); }
                if( text.size() > 8192 ) throw std::runtime_error( "Logpoint output exceeds limit" );
            }
            return text;
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
                    { "supportsConditionalBreakpoints", true }, { "supportsHitConditionalBreakpoints", true }, { "supportsLogPoints", true },
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
                auto &lines = breakpoints[source]; lines.clear(); stopRules[source].clear(); j::array result;
                if( auto b = args.if_contains( "breakpoints" ); b && b->is_array() )
                {
                    if( b->as_array().size() > 1024 ) { response( request, {}, "Too many breakpoints" ); return; }
                    for( const auto &item : b->as_array() )
                    {
                        if( !item.is_object() ) continue;
                        const auto &o = item.as_object(); const int line = integer( o, "line", -1 ) + ( linesAtOne ? 0 : 1 );
                        bool valid = line > 0; std::string problem; StopRule rule;
                        try
                        {
                            for( const char *name : { "condition", "hitCondition", "logMessage" } )
                                if( auto value = o.if_contains( name ); value && !value->is_string() ) throw std::runtime_error( "Breakpoint options must be strings" );
                            rule.condition = stringValue( o, "condition" );
                            if( !rule.condition.empty() ) Expression( *this, main, 0, rule.condition ).parse( false );
                            rule.log = o.if_contains( "logMessage" ) != nullptr; rule.message = stringValue( o, "logMessage" );
                            if( rule.log ) logText( main, rule.message, false );
                            auto hits = stringValue( o, "hitCondition" );
                            hits.erase( std::remove_if( hits.begin(), hits.end(), []( unsigned char c ) { return std::isspace( c ) != 0; } ), hits.end() );
                            if( !hits.empty() )
                            {
                                if( hits.compare( 0, 2, ">=" ) == 0 ) { rule.comparison = '+'; hits.erase( 0, 2 ); }
                                else if( hits.compare( 0, 2, "==" ) == 0 ) hits.erase( 0, 2 );
                                else if( hits[0] == '%' || hits[0] == '>' ) { rule.comparison = hits[0]; hits.erase( 0, 1 ); }
                                if( hits.empty() || hits.size() > 9 || hits.find_first_not_of( "0123456789" ) != std::string::npos ) throw std::runtime_error( "Hit condition must be N, ==N, >=N, >N or %N" );
                                rule.target = std::stoull( hits ); if( !rule.target ) throw std::runtime_error( "Hit count must be positive" );
                            }
                        }
                        catch( const std::exception &error ) { valid = false; problem = error.what(); }
                        int id = 0;
                        if( valid )
                        {
                            lines.insert( line );
                            auto &assigned = breakpointIds[source][line];
                            if( !assigned ) assigned = nextBreakpoint++;
                            id = assigned;
                            stopRules[source][line] = std::move( rule );
                        }
                        const auto known = sources.find( source );
                        result.push_back( j::object{ { "id", id }, { "verified", valid && known != sources.end() && line <= known->second.lines },
                            { "line", line - ( linesAtOne ? 0 : 1 ) }, { "message", valid ? "Bound when this source executes" : problem.empty() ? "Invalid breakpoint line" : problem } } );
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
                Expression evaluator( *this, stopped, level, expression );
                const auto evaluated = evaluator.parse(); evaluator.push( evaluated );
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
                        if( end > 256 ) { closeClient = true; break; }
                        size_t length = 0, offset = 0; bool hasLength = false;
                        while( offset < end )
                        {
                            auto lineEnd = input.find( "\r\n", offset );
                            if( lineEnd == std::string::npos || lineEnd > end ) lineEnd = end;
                            const auto colon = input.find( ':', offset );
                            if( colon == std::string::npos || colon >= lineEnd ) { closeClient = true; break; }
                            auto name = input.substr( offset, colon - offset );
                            for( auto &c : name ) c = static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
                            if( name == "content-length" )
                            {
                                auto value = input.substr( colon + 1, lineEnd - colon - 1 );
                                const auto first = value.find_first_not_of( " \t" ), last = value.find_last_not_of( " \t" );
                                value = first == std::string::npos ? "" : value.substr( first, last - first + 1 );
                                if( hasLength || value.empty() || value.size() > 6 || value.find_first_not_of( "0123456789" ) != std::string::npos ) { closeClient = true; break; }
                                length = std::stoul( value ); hasLength = true;
                            }
                            offset = lineEnd + 2;
                        }
                        if( closeClient || !hasLength ) { closeClient = true; break; }
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
            if( m_impl->hookedThreads != LUA_NOREF )
            {
                lua_rawgeti( m_impl->main, LUA_REGISTRYINDEX, m_impl->hookedThreads );
                lua_pushnil( m_impl->main );
                while( lua_next( m_impl->main, -2 ) )
                {
                    auto thread = lua_tothread( m_impl->main, -2 );
                    if( thread && lua_gethook( thread ) == Impl::hook ) lua_sethook( thread, nullptr, 0, 0 );
                    lua_pop( m_impl->main, 1 );
                }
                lua_pop( m_impl->main, 1 );
                luaL_unref( m_impl->main, LUA_REGISTRYINDEX, m_impl->hookedThreads );
                m_impl->hookedThreads = LUA_NOREF;
            }
            lua_pushlightuserdata( m_impl->main, &debuggerKey ); lua_pushnil( m_impl->main ); lua_rawset( m_impl->main, LUA_REGISTRYINDEX );
        }
        m_impl->main = state;
        if( state )
        {
            lua_newtable( state ); lua_newtable( state ); lua_pushliteral( state, "k" );
            lua_setfield( state, -2, "__mode" ); lua_setmetatable( state, -2 );
            lua_pushthread( state ); lua_pushboolean( state, 1 ); lua_rawset( state, -3 );
            m_impl->hookedThreads = luaL_ref( state, LUA_REGISTRYINDEX );
            lua_pushlightuserdata( state, &debuggerKey ); lua_pushlightuserdata( state, m_impl.get() ); lua_rawset( state, LUA_REGISTRYINDEX );
            lua_sethook( state, Impl::hook, LUA_MASKLINE | LUA_MASKCOUNT, 1000 );
#ifdef WP_LUA_BACKEND_LUAJIT
            luaJIT_setmode( state, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF );
            luaJIT_setmode( state, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_FLUSH );
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
#ifdef _MSC_VER
#pragma float_control(pop)
#endif
