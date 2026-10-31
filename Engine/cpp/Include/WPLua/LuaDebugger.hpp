#pragma once
#include <WPLua/WPLuaConfig.hpp>
#include <functional>
#include <memory>
#include <string>
struct lua_State;

namespace workphone
{
    // Transport owns sockets only. Every Lua API operation runs on the calling
    // VM executor, including inspection while stopped inside the debug hook.
    class WPLua_API LuaDebugger
    {
    public:
        LuaDebugger();
        ~LuaDebugger();
        bool start( lua_State *, unsigned short port, const std::string &token,
                    const std::string &sourceRoot, std::string &error );
        // Thread-safe signal only. The VM executor must finish stop() cleanup
        // after the interrupted Lua call has unwound and its owner lock is held.
        void requestStop() noexcept;
        void stop();
        void bind( lua_State * );
        void poll();
        void source( const std::string &name, const std::string &text );
        void exception( lua_State *, const char *message );
        unsigned short port() const;
        bool paused() const;
        void setPausePump( std::function<bool()> pump );
        void setPauseObserver( std::function<void(bool)> observer );
        void command( const std::string &command );
        std::string status() const;
        bool callbackAllowed( lua_State *state, int functionIndex ) const;
        void copySourcesFrom( const LuaDebugger &other );
    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}
