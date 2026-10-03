#include <WPLua/LuaManager.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <WPLua/NullScriptObject.hpp>
#include <WPLua/LuaObjectData.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/System/IPlugin.hpp>
#include <Workphone/Interface/System/IPluginManager.hpp>
#include <WPLuabind/WPLuabind.hpp>
#include <sstream>
#include <stdarg.h>
#include <iostream>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <lobject.h>
}

#include <luabind/luabind.hpp>
#include <luabind/detail/call_member.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/function_introspection.hpp>
#include <luabind/class_info.hpp>

namespace luabind
{
    std::string get_function_name( argument const &fn );
}

extern "C" {
int luaopen_cjson( lua_State *l );
}

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, LuaManager, IScriptManager );

    TValue *index2value( lua_State *L, int idx );

    LuaManager::LuaManager() = default;

    LuaManager::~LuaManager() = default;

    void LuaManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            ScopedLock lock( this );

            createLuaState();
            m_bReload = false;

            m_timeTaken = 0.0f;
            m_callCounter = 0;

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void LuaManager::unload( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            setLoadingState( LoadingState::Unloading );

            ScopedLock lock( this );

            for( auto objectData : m_objectData )
            {
                if( objectData )
                {
                    // Any surviving script data belongs to an object that has already gone
                    // through script finalization. Do not resolve its weak owner during the
                    // Lua-state teardown, because the pooled owner slot may already be stale.
                    objectData->setOwner( nullptr );
                    objectData->unload( nullptr );
                }
            }

            m_objectData.clear();
            m_creationList.clear();

            for( u32 i = 0; i < m_instances.size(); ++i )
            {
                luabind::object *pObject = m_instances[i];
                delete pObject;
            }

            m_instances.clear();

            lua_gc( m_luaState, LUA_GCCOLLECT, 0 );

            // closes the lua state
            lua_close( m_luaState );
            m_luaState = nullptr;

            auto nullScriptObject = m_nullScriptObject.load();
            WP_SAFE_DELETE( nullScriptObject );

            setLoadingState( LoadingState::Unloaded );
        }
    }

    void LuaManager::setClassNames( const Array<String> &classNames )
    {
        m_classNames = { classNames.begin(), classNames.end() };
    }

    void LuaManager::handleLuaError( lua_State *luaState )
    {
        auto errorStr = String( lua_tostring( luaState, -1 ) );
        auto message = String( "Error: " ) + errorStr + String( "\n" );

        String debugStr;

        lua_pop( luaState, 1 );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto pScriptManager = applicationManager->getScriptManager();
        auto scriptMgr = workphone::static_pointer_cast<LuaManager>( pScriptManager );
        if( scriptMgr )
        {
            debugStr = scriptMgr->getDebugInfo();
            scriptMgr->setError( true );
        }

        auto logStr = message + debugStr;
        WP_LOG_ERROR( logStr );
    }

    void LuaManager::lock()
    {
        m_mutex.lock();
    }

    void LuaManager::lock_shared()
    {
        m_mutex.lock_shared();
    }

    bool LuaManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void LuaManager::unlock()
    {
        m_mutex.unlock();
    }

    void LuaManager::unlock_shared()
    {
        m_mutex.unlock_shared();
    }

    s32 handleLuaPCallError( lua_State *luaState )
    {
        String debugStr;

        lua_Debug d;

        int level = LUA_MINSTACK;
        int success;

        while( level >= 0 )
        {
            success = lua_getstack( luaState, level, &d );
            if( success != 0 )
            {
                lua_getinfo( luaState, "Sln", &d );

                std::stringstream msg;
                msg << d.short_src << ":" << d.currentline;

                if( d.name != nullptr )
                {
                    msg << "(" << d.namewhat << " " << d.name << ")";
                }

                static const auto newline = std::string( "\n" );
                debugStr += msg.str() + newline;

                auto outputStr = XmlUtil::createErrorXML( d.currentline, d.short_src );
                WP_LOG_ERROR( outputStr );
            }

            --level;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto scriptManager = static_cast<LuaManager *>( applicationManager->getScriptManagerPtr() );
        scriptManager->setError( true );

        return 1;
    }

    void handleCastFailed( lua_State *luaState, const luabind::type_id &id )
    {
        String debugStr;

        lua_Debug d;

        int level = LUA_MINSTACK;
        int success;

        while( level >= 0 )
        {
            success = lua_getstack( luaState, level, &d );
            if( success != 0 )
            {
                lua_getinfo( luaState, "Sln", &d );

                std::stringstream msg;
                msg << d.short_src << ":" << d.currentline;

                if( d.name != nullptr )
                {
                    msg << "(" << d.namewhat << " " << d.name << ")";
                }

                debugStr += String( msg.str().c_str() ) + String( "\n" );
            }

            --level;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto scriptManager = (LuaManager *)applicationManager->getScriptManagerPtr();
        scriptManager->setError( true );

        WP_LOG_ERROR( debugStr );
    }

    void LuaManager::loadScript( const String &filename )
    {
        try
        {
            if( isLoaded() )
            {
                ScopedLock lock( this );

                auto it = std::find( m_scripts.begin(), m_scripts.end(), filename );
                if( it != m_scripts.end() )
                {
                    return;
                }

                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystemPtr();
                WP_ASSERT( fileSystem );

                auto stream = fileSystem->open( filename, true, false, false, false, false );
                if( !stream )
                {
                    stream = fileSystem->open( filename, true, false, false, true, true );
                }

                if( stream )
                {
                    auto script = stream->getAsString();

                    auto luaState = getLuaState();
                    auto error = luaL_dostring( luaState, script.c_str() );
                    if( error )
                    {
                        auto message = String( "ScriptMgr::loadScript - error - couldn't open " ) +
                                       filename + String( " errorcode = " ) +
                                       String( lua_tostring( luaState, -1 ) );
                        WP_LOG_ERROR( message );
                    }
                    else
                    {
                        auto message = String( "Loaded: " ) + filename;
                        WP_LOG_INFO( message );
                    }

                    // add filename
                    m_scripts.push_back( filename );
                }

                //updateClassNames();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void LuaManager::loadScriptFromString( const String &str )
    {
        try
        {
            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto luaState = getLuaState();
            auto error = luaL_dostring( luaState, str.c_str() );
            if( error )
            {
                auto message = String( "ScriptMgr::loadScript - error - couldn't open " ) + str +
                               String( " errorcode = " ) + String( lua_tostring( luaState, -1 ) );
                WP_LOG_ERROR( message );
            }
            else
            {
                auto message = String( "Loaded: " ) + str;
                WP_LOG_INFO( message );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void LuaManager::print_lua_stack()
    {
        auto L = getLuaState();
        print_lua_stack( L );
    }

    void LuaManager::print_lua_stack( lua_State *L )
    {
        std::cout << "Lua stack contents:\n";
        int top = lua_gettop( L );
        for( int i = 1; i <= top; i++ )
        {
            int type = lua_type( L, i );
            const char *type_name = lua_typename( L, type );
            std::cout << "  [" << i << "]: " << type_name;
            switch( type )
            {
            case LUA_TBOOLEAN:
                std::cout << " " << ( lua_toboolean( L, i ) ? "true" : "false" );
                break;
            case LUA_TNUMBER:
                std::cout << " " << lua_tonumber( L, i );
                break;
            case LUA_TSTRING:
                std::cout << " \"" << lua_tostring( L, i ) << "\"";
                break;
            case LUA_TTABLE:
                std::cout << " {table}";
                break;
            case LUA_TFUNCTION:
                std::cout << " {function}";
                break;
            case LUA_TUSERDATA:
                std::cout << " {userdata}";
                break;
            default:
                std::cout << " {unknown}";
                break;
            }
            std::cout << "\n";
        }
    }

    void LuaManager::updateClassNames()
    {
        ScopedLock lock( this );

        m_classNames.clear();

        auto L = getLuaState();

        auto classNames = luabind::get_class_names( L );

        for( luabind::iterator i( classNames ), e; i != e; ++i )
        {
            luabind::object obj = *i;

            std::string className = luabind::object_cast<std::string>( obj );
        }

        // Push the global environment table onto the stack
        lua_pushglobaltable( L );

        // Push a nil value onto the stack to start the iteration
        lua_pushnil( L );

        while( lua_next( L, -2 ) != 0 )
        {
            auto type = lua_type( L, -1 );
            if( type == LUA_TUSERDATA )
            {
                // Get the class name
                std::string className = lua_tostring( L, -2 );

                if( std::find( m_bindingClassNames.begin(), m_bindingClassNames.end(),
                               className.c_str() ) == m_bindingClassNames.end() )
                {
                    m_classNames.push_back( className.c_str() );
                }
            }

            lua_pop( L, 1 );
        }
    }

    void LuaManager::updateBindClassNames()
    {
        ScopedLock lock( this );

        m_bindingClassNames.clear();

        auto L = getLuaState();

        auto classNames = luabind::get_class_names( L );

        for( luabind::iterator i( classNames ), e; i != e; ++i )
        {
            luabind::object obj = *i;

            auto className = luabind::object_cast<std::string>( obj );
            m_bindingClassNames.push_back( className.c_str() );
        }
    }

    void LuaManager::updateScriptData()
    {
        ScopedLock lock( this );

        m_scriptData.clear();

        auto L = getLuaState();
        // Push the global environment table onto the stack
        lua_pushglobaltable( L );
        // Push a nil value onto the stack to start the iteration
        lua_pushnil( L );
        while( lua_next( L, -2 ) != 0 )
        {
            auto type = lua_type( L, -1 );
            if( type == LUA_TFUNCTION )
            {
                // Get the function name
                std::string functionName = lua_tostring( L, -2 );
                // Get the function's environment
                lua_getuservalue( L, -1 );
                // Get the function's environment table
                lua_pushstring( L, "__name" );
                lua_rawget( L, -2 );
                std::string className = lua_tostring( L, -1 );
                lua_pop( L, 2 );

                // Get the function's source file
                lua_pushstring( L, "__source" );
                lua_rawget( L, -2 );
                std::string source = lua_tostring( L, -1 );
                lua_pop( L, 2 );
                // Get the function's line number
                lua_pushstring( L, "__line" );
                lua_rawget( L, -2 );
                int line = (int)lua_tointeger( L, -1 );
                lua_pop( L, 2 );
                // Get the function's column number
                lua_pushstring( L, "__column" );
                lua_rawget( L, -2 );
                int column = (int)lua_tointeger( L, -1 );
                lua_pop( L, 2 );

                // Get the function's source code
                auto code = getSourceCode( source.c_str(), line, column );
            }

            lua_pop( L, 1 );
        }
    }

    void LuaManager::updateScriptDataVariables()
    {
        ScopedLock lock( this );
        auto L = getLuaState();
        // Push the global environment table onto the stack
        lua_pushglobaltable( L );
        // Push a nil value onto the stack to start the iteration
        lua_pushnil( L );
        while( lua_next( L, -2 ) != 0 )
        {
            auto type = lua_type( L, -1 );
            if( type == LUA_TTABLE )
            {
                // Get the function name
                std::string functionName = lua_tostring( L, -2 );
                // Get the function's environment
                lua_getuservalue( L, -1 );
                // Get the function's environment table
                lua_pushstring( L, "__name" );
                lua_rawget( L, -2 );
                std::string className = lua_tostring( L, -1 );
                lua_pop( L, 2 );
                // Get the function's source file
                lua_pushstring( L, "__source" );
                lua_rawget( L, -2 );
                std::string source = lua_tostring( L, -1 );
                lua_pop( L, 2 );
                // Get the function's line number
                lua_pushstring( L, "__line" );
                lua_rawget( L, -2 );
                auto line = lua_tointeger( L, -1 );
                lua_pop( L, 2 );
                // Get the function's column number
                lua_pushstring( L, "__column" );
                lua_rawget( L, -2 );
                auto column = lua_tointeger( L, -1 );
                lua_pop( L, 2 );
                // Get the function's source code
                auto code = getSourceCode( source.c_str(), (s32)line, (s32)column );
            }
            lua_pop( L, 1 );
        }
    }

    String LuaManager::getSourceCode( const String &source, s32 line, s32 column )
    {
        ScopedLock lock( this );

        String code;
        code.clear();

        return code;
    }

    Array<String> LuaManager::getSupportedFileExtensions() const
    {
        return { ".lua" };
    }

    void LuaManager::loadObject( SmartPtr<ISharedObject> scriptObject, bool forceQueue )
    {
        if( forceQueue )
        {
            m_loadQueue.push( scriptObject );
        }
        else
        {
            scriptObject->load( nullptr );
        }
    }

    void LuaManager::unloadObject( SmartPtr<ISharedObject> scriptObject, bool forceQueue )
    {
        if( forceQueue )
        {
            m_unloadQueue.push( scriptObject );
        }
        else
        {
            scriptObject->unload( nullptr );
        }
    }

    void LuaManager::loadScripts( const Array<String> &scripts )
    {
        for( auto &script : scripts )
        {
            loadScript( script );
        }

        updateClassNames();
    }

    void LuaManager::executeScript( const String &script )
    {
        auto error = luaL_loadbuffer( m_luaState, script.c_str(), script.length(), "script" ) ||
                     lua_pcall( m_luaState, 0, 0, 0 );

        if( error )
        {
            String message = String( "Error: " ) + String( lua_tostring( m_luaState, -1 ) );
            WP_LOG_INFO( message );
            lua_pop( m_luaState, 1 );
        }
    }

    void LuaManager::callFunction( const String &functionName )
    {
        if( isLoaded() )
        {
            ScopedLock lock( this );

            Parameters parameters;
            Parameters results;
            callFunction( functionName, parameters, results );
        }
    }

    void LuaManager::callFunction( const String &functionName, const Parameters &parameters )
    {
        ScopedLock lock( this );

        Parameters results;
        callFunction( functionName, parameters, results );
    }

    void LuaManager::callFunction( const String &functionNameStr, const Parameters &parameters,
                                   Parameters &results )
    {
        try
        {
            ScopedLock lock( this );
        }
        catch( std::exception &e )
        {
            String msg = String( "lua exception: " ) + String( e.what() );
            WP_LOG_INFO( msg.c_str() );
        }
    }

    s32 LuaManager::callMember( const String &className, const String &functionName )
    {
        try
        {
            ScopedLock lock( this );

            auto returnValue = 0;  //default value

            auto object = luabind::globals( m_luaState )[className.c_str()];
            if( object )
            {
                if( m_enableFullDebug )
                {
                    m_curClass = className;
                    m_curFunction = functionName;
                }

                luabind::call_member<void>( object, functionName.c_str() );
            }
            else
            {
                auto msg = String( "Object not found: " ) + className;
                WP_LOG_INFO( msg );
            }

            return returnValue;
        }
        catch( std::exception &e )
        {
            auto msg = String( "Error calling function: " ) + className + String( ":" ) + functionName +
                       String( " lua exception: " ) + String( e.what() );
            WP_LOG_INFO( msg );
        }

        return 0;
    }

    s32 LuaManager::callMember( const String &className, const String &functionName,
                                const Parameters &parameters )
    {
        try
        {
            ScopedLock lock( this );

            auto returnValue = 0;  //default value

            auto object = luabind::globals( m_luaState )[className.c_str()];
            if( object )
            {
                if( m_enableFullDebug )
                {
                    m_curClass = className;
                    m_curFunction = functionName;
                }

                luabind::call_member<void>( object, functionName.c_str(), parameters );
            }
            else
            {
                auto msg = String( "Object not found: " ) + className;
                WP_LOG_INFO( msg );
            }

            return returnValue;
        }
        catch( std::exception &e )
        {
            auto msg = String( "Error calling function: " ) + className + String( ":" ) + functionName +
                       String( " lua exception: " ) + String( e.what() );
            WP_LOG_INFO( msg );

            throw;
        }

        return 0;
    }

    s32 LuaManager::callMember( const String &className, const String &functionName,
                                const Parameters &parameters, Parameters &results )
    {
        try
        {
            ScopedLock lock( this );

            auto returnValue = 0;  //default value

            auto object = luabind::globals( m_luaState )[className.c_str()];
            if( object )
            {
                if( m_enableFullDebug )
                {
                    m_curClass = className;
                    m_curFunction = functionName;
                }

                luabind::call_member<void>( object, functionName.c_str(), parameters,
                                            boost::ref( results ) );
            }
            else
            {
                auto msg = String( "Object not found: " ) + className;
                WP_LOG_INFO( msg );
            }

            return returnValue;
        }
        catch( std::exception &e )
        {
            auto msg = String( "Error calling function: " ) + className + String( ":" ) + functionName +
                       String( " lua exception: " ) + String( e.what() );
            WP_LOG_INFO( msg );

            throw;
        }

        return 0;
    }

    void LuaManager::callObjectMember( SmartPtr<ISharedObject> object, const String &functionName )
    {
        if( isLoaded() )
        {
            try
            {
                ScopedLock lock( this, true );
                _callObjectMember( object, functionName );
            }
            catch( ScriptException &e )
            {
                auto msg = String( "Error calling function: " ) + m_curClass + String( ":" ) +
                           functionName + String( " lua exception: " ) + String( e.what() ) +
                           String( " Lua Debug: " ) + getDebugInfo();

                WP_LOG_INFO( msg.c_str() );
                WP_LOG_INFO( e.what() );

                throw;
            }
            catch( std::exception &e )
            {
                String msg = String( "Error calling function: " ) + m_curClass + String( ":" ) +
                             functionName + String( " lua exception: " ) + String( e.what() ) +
                             String( " Lua Debug: " ) + getDebugInfo();

                WP_LOG_INFO( msg.c_str() );

                std::stringstream strStream;

                strStream << DebugUtil::getStackTraceForException( e );
                WP_LOG_INFO( strStream.str().c_str() );

                throw;
            }
            catch( ... )
            {
                String msg = String( "Error calling function: " ) + m_curClass + String( ":" ) +
                             functionName + String( " Lua Debug: " ) + getDebugInfo();

                WP_LOG_INFO( msg.c_str() );

                throw;
            }
        }
    }

    void LuaManager::callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                       const Parameters &parameters )
    {
        if( isLoaded() )
        {
            m_callCounter++;

            try
            {
                ScopedLock lock( this, true );
                _callObjectMember( object, functionName, parameters );
            }
            catch( Exception &e )
            {
                auto msg = String( "Error calling function: " ) + m_curClass + String( ":" ) +
                           functionName + String( " lua exception: " ) + String( e.what() ) +
                           String( " Lua Debug: " ) + getDebugInfo();

                WP_LOG_INFO( msg.c_str() );
                WP_LOG_INFO( e.what() );

                throw;
            }
            catch( std::exception &e )
            {
                String msg = String( "Error calling function: " ) + m_curClass + String( ":" ) +
                             functionName + String( " lua exception: " ) + String( e.what() ) +
                             String( " Lua Debug: " ) + getDebugInfo();

                WP_LOG_INFO( msg.c_str() );

                std::stringstream strStream;

                strStream << DebugUtil::getStackTraceForException( e );
                WP_LOG_INFO( strStream.str().c_str() );

                throw;
            }
            catch( ... )
            {
                String msg = String( "Error calling function: " ) + m_curClass + String( ":" ) +
                             functionName + String( " Lua Debug: " ) + getDebugInfo();

                WP_LOG_INFO( msg.c_str() );

                throw;
            }

            --m_callCounter;
        }
    }

    void LuaManager::callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                       const Parameters &parameters, Parameters &results )
    {
        if( isLoaded() )
        {
            try
            {
                ScopedLock lock( this, true );
                _callObjectMember( object, functionName, parameters, results );
            }
            catch( Exception &e )
            {
                auto msg = String( "Error calling function: " ) + m_curClass + String( ":" ) +
                           functionName + String( " lua exception: " ) + String( e.what() ) +
                           String( " Lua Debug: " ) + getDebugInfo();

                WP_LOG_INFO( msg.c_str() );
                WP_LOG_INFO( e.what() );

                throw;
            }
            catch( std::exception &e )
            {
                String msg = String( "Error calling function: " ) + m_curClass + String( ":" ) +
                             functionName + String( " lua exception: " ) + String( e.what() ) +
                             String( " Lua Debug: " ) + getDebugInfo();

                WP_LOG_INFO( msg.c_str() );

                std::stringstream strStream;

                strStream << DebugUtil::getStackTraceForException( e );
                WP_LOG_INFO( strStream.str().c_str() );

                throw;
            }
            catch( ... )
            {
                String msg = String( "Error calling function: " ) + m_curClass + String( ":" ) +
                             functionName + String( " Lua Debug: " ) + getDebugInfo();

                WP_LOG_INFO( msg.c_str() );

                throw;
            }
        }
    }

    void LuaManager::reloadScripts()
    {
        m_bReload = true;
    }

    bool LuaManager::reloadPending() const
    {
        return m_bReload;
    }

    void LuaManager::clearStack()
    {
        auto luaState = getLuaState();
        lua_settop( luaState, 0 );
    }

    void LuaManager::createLuaState()
    {
        // create the lua state

#ifdef _FINAL_
        auto luaState = lua_open();
#else
        auto luaState = luaL_newstate();
#endif

        luaL_openlibs( luaState );

        // Load the Lua CJSON library
        luaL_requiref( luaState, "cjson", luaopen_cjson, 1 );
        lua_pop( luaState, 1 );

        luabind::open( luaState );

        //bindings
        bindParam( luaState );
        bindParamList( luaState );

        bindMath( luaState );

        bindString( luaState );

        bindBaseObjects( luaState );
        bindCore( luaState );
        bindIO( luaState );
        bindSystem( luaState );

        bindScript( luaState );

        bindAi( luaState );
        bindFSM( luaState );

        bindSceneManager( luaState );
        bindScene( luaState );

        bindInput( luaState );
        bindVideo( luaState );
        bindSound( luaState );
        bindComponent( luaState );
        bindComponentUI( luaState );
        bindGraphicsSystem( luaState );
        bindUI( luaState );
        bindPhysics( luaState );
        bindDatabase( luaState );
        bindProcedural( luaState );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( applicationManager )
        {
            if( auto pluginManager = applicationManager->getPluginManager() )
            {
                using BindLibrary = void ( * )( lua_State * );

                for( const auto &pluginObject : pluginManager->getPlugins() )
                {
                    auto plugin = workphone::dynamic_pointer_cast<IPlugin>( pluginObject );
                    if( plugin && plugin->getLibraryHandle() )
                    {
                        auto bindLibrary =
                            reinterpret_cast<BindLibrary>( plugin->getFunction( "bindLibrary" ) );
                        if( bindLibrary )
                        {
                            bindLibrary( luaState );
                        }
                    }
                }
            }
        }

        //{
        //    using namespace luabind;

        //    module( luaState )[class_<IScriptData, SmartPtr<IScriptData>>( "IScriptData" )];

        //    module(
        //        luaState )[class_<LuaObjectData, IScriptData, SmartPtr<IScriptData>>( "LuaObjectData" )
        //                       .def( "getInstance", _getLuaInstance )];
        //}

        setLuaState( luaState );

        //lua_gc(luaState, LUA_GCSETPAUSE, 200);
        //lua_gc(luaState, LUA_GCSETSTEPMUL, 300);

        updateBindClassNames();

#if defined LUABIND_NO_EXCEPTIONS
        luabind::set_error_callback( handleLuaError );
        luabind::set_pcall_callback( handleLuaPCallError );
        set_cast_failed_callback( handleCastFailed );
#endif
    }

    String LuaManager::getDebugInfo()
    {
        auto debugStr = String( "Class: " ) + m_curClass + String( " Function: " ) + m_curFunction;

        lua_Debug d;

        auto luaState = getLuaState();

        auto top = lua_gettop( luaState );
        auto level = top;  //LUA_MINSTACK;
        auto success = lua_getstack( luaState, top, &d );

        while( level >= 0 )
        {
            success = lua_getstack( luaState, level, &d );
            if( success != 0 )
            {
                lua_getinfo( luaState, "Sln", &d );

                std::stringstream msg;
                msg << d.short_src << ":" << d.currentline;

                if( d.name != nullptr )
                {
                    msg << "(" << d.namewhat << " " << d.name << ")";
                }

                debugStr += String( msg.str().c_str() ) + String( "\n" );
            }

            --level;
        }

        return debugStr;
    }

    SmartPtr<IScriptClass> LuaManager::createObject( const String &className,
                                                     SmartPtr<ISharedObject> object )
    {
        ScopedLock lock( this );

        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );
            WP_ASSERT( factoryManager->isValid() );

            auto scriptData = object->getScriptData();
            auto luaScriptData = workphone::static_pointer_cast<LuaObjectData>( scriptData );
            if( !luaScriptData )
            {
                luaScriptData = factoryManager->make_ptr<LuaObjectData>();
                luaScriptData->setClassName( className );
                luaScriptData->setLuaState( m_luaState );
                luaScriptData->setOwner( object );
                object->setScriptData( luaScriptData );

                m_objectData.push_back( luaScriptData );

                createLuaInstance( luaScriptData );
                luaScriptData->load( nullptr );

                object->setScriptData( luaScriptData );
            }
            else
            {
                m_objectData.push_back( luaScriptData );

                createLuaInstance( luaScriptData );
                object->setScriptData( luaScriptData );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void LuaManager::destroyObject( SmartPtr<ISharedObject> object )
    {
        if( isLoaded() )
        {
            ScopedLock lock( this );

            if( auto luaState = getLuaState() )
            {
                lua_gc( luaState, LUA_GCCOLLECT, 0 );
            }

            if( object )
            {
                if( auto objectData = object->getScriptData() )
                {
                    objectData->unload( nullptr );

                    m_objectData.erase(
                        std::remove( m_objectData.begin(), m_objectData.end(), objectData ),
                        m_objectData.end() );

                    object->setScriptData( nullptr );
                }
            }

            if( auto luaState = getLuaState() )
            {
                lua_gc( luaState, LUA_GCCOLLECT, 0 );
            }
        }
    }

    void LuaManager::garbageCollect()
    {
        ScopedLock lock( this );

        if( auto luaState = getLuaState() )
        {
            lua_gc( luaState, LUA_GCCOLLECT, 0 );
        }
    }

    void LuaManager::registerClass( void *ptr )
    {
    }

    Array<String> LuaManager::getClassNames() const
    {
        auto array = m_classNames.snapshot();
        return { array.begin(), array.end() };
    }

    s32 LuaManager::_callObjectMember( SmartPtr<ISharedObject> object, const String &functionName )
    {
        auto returnValue = 0;  //default value

        try
        {
            ScopedLock lock( this );

            setError( false );

            auto pScriptData = object->getScriptData();
            auto data = workphone::static_pointer_cast<LuaObjectData>( pScriptData );
            if( data )
            {
                if( data->hasMemberFunction( functionName ) )
                {
                    if( m_enableFullDebug )
                    {
                        m_curClass = data->getClassName();
                        m_curFunction = functionName;
                    }

                    auto &luaObject = data->getObject();
                    if( !luaObject )
                    {
                        if( !createLuaInstance( data ) )
                        {
                            errorObjectNotFound();

                            returnValue = 0;
                        }
                    }

                    if( luaObject )
                    {
                        luabind::call_member<void>( luaObject, functionName.c_str() );
                    }
                }
            }
            else
            {
                errorObjectNotFound();

                returnValue = 0;
            }

            if( getError() )
            {
                returnValue = 0;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return returnValue;
    }

    s32 LuaManager::_callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                       const Parameters &parameters )
    {
        auto returnValue = 0;  //default value

        try
        {
            ScopedLock lock( this );

            setError( false );

            auto pScriptData = object->getScriptData();
            auto data = workphone::static_pointer_cast<LuaObjectData>( pScriptData );
            if( data )
            {
                if( data->hasMemberFunction( functionName ) )
                {
                    if( m_enableFullDebug )
                    {
                        m_curClass = data->getClassName();
                        m_curFunction = functionName;
                    }

                    luabind::object &luaObject = data->getObject();
                    if( !luaObject )
                    {
                        if( !createLuaInstance( data ) )
                        {
                            errorObjectNotFound();

                            returnValue = 0;

                            auto msg = String( "Object not found: " );
                            WP_EXCEPTION( msg.c_str() );
                        }
                    }

                    if( luaObject )
                    {
#if WP_PROFILE_LUA_CALLS
                        auto engine = core::ApplicationManager::instance();
                        auto profiler = engine->getProfiler();

                        String profileClass = data->getClassName();
                        String profileFunction = functionName;

                        if( m_callCounter == 1 && profiler )
                        {
                            WP_PROFILE_START( profileClass + String( ":" ) + profileFunction );
                        }
#endif

                        luabind::call_member<void>( luaObject, functionName.c_str(), parameters );

#if WP_PROFILE_LUA_CALLS
                        if( m_callCounter == 1 && profiler )
                        {
                            WP_PROFILE_END( profileClass + String( ":" ) + profileFunction );
                        }
#endif
                    }
                }
            }
            else
            {
                errorObjectNotFound();

                returnValue = 0;

                auto msg = String( "Object not found: " );
                WP_EXCEPTION( msg.c_str() );
            }

            if( getError() )
            {
                returnValue = 0;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return returnValue;
    }

    s32 LuaManager::_callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                       const Parameters &parameters, Parameters &results )
    {
        auto returnValue = 0;  //default value

        try
        {
            ScopedLock lock( this );

            setError( false );

            auto pScriptData = object->getScriptData();
            auto data = workphone::static_pointer_cast<LuaObjectData>( pScriptData );
            if( data )
            {
                if( data->hasMemberFunction( functionName ) )
                {
                    if( m_enableFullDebug )
                    {
                        m_curClass = data->getClassName();
                        m_curFunction = functionName;
                    }

                    auto &luaObject = data->getObject();
                    if( !luaObject )
                    {
                        if( !createLuaInstance( data ) )
                        {
                            errorObjectNotFound();

                            returnValue = 0;

                            auto msg = String( "Object not found: " );
                            WP_LOG_INFO( msg.c_str() );
                        }
                    }

                    if( luaObject )
                    {
                        luabind::call_member<void>( luaObject, functionName.c_str(), parameters,
                                                    boost::ref( results ) );
                    }
                }
            }
            else
            {
                auto msg = String( "Object not found: " );
                WP_LOG_INFO( msg.c_str() );

                errorObjectNotFound();

                returnValue = 0;
            }

            if( getError() )
            {
                returnValue = 0;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return returnValue;
    }

    void LuaManager::update()
    {
        if( isLoaded() )
        {
            auto task = Thread::getCurrentTask();

            switch( task )
            {
            case TaskId::Application:
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto timer = applicationManager->getTimerPtr();

                auto t = timer->getTime();

                if( m_nextGcUpdate < t )
                {
                    ScopedLock lock( this );

                    if( m_bReload )
                    {
                        _reloadScripts();
                        m_bReload = false;
                    }

                    for( u32 i = 0; i < m_creationList.size(); ++i )
                    {
                        auto &objectData = m_creationList[i];
                        createLuaInstance( objectData );
                    }

                    m_creationList.clear();

                    if( auto luaState = getLuaState() )
                    {
                        lua_gc( luaState, LUA_GCCOLLECT, 0 );
                        lua_gc( luaState, LUA_GCSTOP, 0 );
                    }

                    m_nextGcUpdate = t + time_interval( 3.0 );
                }
            }
            break;
            }
        }
    }

    void LuaManager::_reloadScripts()
    {
        try
        {
            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();

            for( auto &data : m_objectData )
            {
                if( data )
                {
                    auto nilValue = luabind::object();
                    data->setObject( nilValue );
                }
            }

            if( auto luaState = getLuaState() )
            {
                lua_gc( luaState, LUA_GCCOLLECT, 0 );
                lua_gc( luaState, LUA_GCSTOP, 0 );

                lua_close( luaState );
                setLuaState( nullptr );
            }

            createLuaState();

            auto luaState = getLuaState();
            WP_ASSERT( luaState );

            for( u32 i = 0; i < m_scripts.size(); ++i )
            {
                auto filename = m_scripts[i];
                if( filename.empty() )
                    continue;

                auto stream = fileSystem->open( filename, true, false, false, false, false );
                if( !stream )
                {
                    stream = fileSystem->open( filename, true, false, false, true, true );
                }

                if( stream )
                {
                    auto script = stream->getAsString();
                    auto error = luaL_dostring( luaState, script.c_str() );
                    if( error )
                    {
                        auto message = String( "LuaScriptMgr::loadScript - error - couldn't open " ) +
                                       filename.str() + String( " errorcode = " ) +
                                       String( lua_tostring( luaState, -1 ) );
                        WP_LOG_INFO( message );
                    }
                    else
                    {
                        auto message = String( "LuaScriptMgr loaded script: " ) + filename.str();
                        WP_LOG_INFO( message );
                    }
                }
            }

            for( u32 i = 0; i < m_objectData.size(); ++i )
            {
                auto objectData = m_objectData[i];
                if( objectData )
                {
                    auto className = objectData->getClassName();
                    if( !className.empty() )
                    {
                        luabind::object _LuaObject = luabind::globals( luaState )[className.c_str()];
                        if( _LuaObject )
                        {
                            auto pObject = objectData->getOwner();
                            objectData->getObject() = _LuaObject( pObject );
                        }
                        else
                        {
                            auto msg = String( "Error : " ) + getDebugInfo();
                            WP_LOG_ERROR( msg );
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            auto msg = String( "Error : " ) + String( e.what() );
            WP_LOG_ERROR( msg );
        }
    }

    bool LuaManager::createLuaInstance( SmartPtr<LuaObjectData> objectData )
    {
        ScopedLock lock( this );

        if( objectData )
        {
            auto className = objectData->getClassName();

            auto luaState = getLuaState();
            auto globalObject = luabind::globals( luaState );

            auto classObject = globalObject[className.c_str()];
            if( classObject )
            {
                // SmartPtr conversion exposes the host's bound interface (for example IEditor).
                // getOwner() returns a raw pointer, which only exposes ISharedObject to Lua.
                SmartPtr<ISharedObject> owner = objectData->getOwner();
                luabind::object instance = classObject( owner );
                objectData->setObject( instance );

                return true;
            }
        }
        else
        {
            WP_EXCEPTION( "No object data." );
        }

        return false;
    }

    void LuaManager::errorObjectNotFound()
    {
        auto outputStr = XmlUtil::createErrorXML( 0, m_curClass, m_curFunction );
        WP_LOG_ERROR( outputStr );
    }

    void LuaManager::removeBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint )
    {
        if( !breakpoint )
        {
            WP_LOG_ERROR( "LuaManager::removeBreakpoint - Invalid breakpoint pointer" );
            return;
        }

        ScopedLock lock( this );

        // Find and remove the breakpoint from our collection
        auto filePath = breakpoint->getFilePath();
        auto lineNumber = breakpoint->getLineNumber();

        auto it = std::find_if(
            m_breakpoints.begin(), m_breakpoints.end(),
            [&filePath, lineNumber]( const SmartPtr<IScriptBreakpoint> &existingBreakpoint ) {
                return existingBreakpoint && existingBreakpoint->getFilePath() == filePath &&
                       existingBreakpoint->getLineNumber() == lineNumber;
            } );

        if( it != m_breakpoints.end() )
        {
            // Remove the breakpoint
            m_breakpoints.erase( it );

            auto msg = String( "Removed breakpoint at " ) + filePath + String( ":" ) +
                       String( std::to_string( lineNumber ).c_str() );
            WP_LOG_INFO( msg );
        }
        else
        {
            // Breakpoint not found
            auto msg = String( "Breakpoint not found at " ) + filePath + String( ":" ) +
                       String( std::to_string( lineNumber ).c_str() );
            WP_LOG_INFO( msg );
        }
    }

    void LuaManager::addBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint )
    {
        if( !breakpoint )
        {
            WP_LOG_ERROR( "LuaManager::addBreakpoint - Invalid breakpoint pointer" );
            return;
        }

        ScopedLock lock( this );

        // Check if breakpoint already exists
        auto filePath = breakpoint->getFilePath();
        auto lineNumber = breakpoint->getLineNumber();

        for( const auto &existingBreakpoint : m_breakpoints )
        {
            if( existingBreakpoint && existingBreakpoint->getFilePath() == filePath &&
                existingBreakpoint->getLineNumber() == lineNumber )
            {
                // Breakpoint already exists at this location
                auto msg = String( "Breakpoint already exists at " ) + filePath + String( ":" ) +
                           StringUtil::toString( lineNumber );
                WP_LOG_INFO( msg );
                return;
            }
        }

        // Add the breakpoint to our collection
        m_breakpoints.push_back( breakpoint );

        auto msg = String( "Added breakpoint at " ) + filePath + String( ":" ) +
                   StringUtil::toString( lineNumber );
        WP_LOG_INFO( msg );
    }

    Array<SmartPtr<IScriptBreakpoint>> LuaManager::getBreakpoints() const
    {
        return m_breakpoints.snapshot();
    }

    void LuaManager::_getObject( void **object )
    {
        *object = getLuaState();
    }

    void *LuaManager::createInstance( const String &className )
    {
        ScopedLock lock( this );

        auto luaState = getLuaState();
        auto globalObject = luabind::globals( luaState );

        auto luaClass = globalObject[className.c_str()];
        if( luaClass )
        {
            auto instance = new luabind::object( luaClass() );
            m_instances.push_back( instance );
            return instance;
        }

        return nullptr;
    }

    void LuaManager::destroyInstance( void *instance )
    {
        ScopedLock lock( this );

        auto object = static_cast<luabind::object *>( instance );
        m_instances.erase( std::remove( m_instances.begin(), m_instances.end(), object ),
                           m_instances.end() );
        WP_SAFE_DELETE( object );
    }

    void LuaManager::setError( bool error )
    {
        m_bError = error;
    }

    bool LuaManager::getError() const
    {
        return m_bError;
    }

    void LuaManager::setDebugEnabled( bool enable )
    {
        m_isDebugEnabled = enable;
    }

    bool LuaManager::isDebugEnabled() const
    {
        return m_isDebugEnabled;
    }

    void LuaManager::setDelayedCreation( bool delayedCreation )
    {
        m_delayedCreation = delayedCreation;
    }

    bool LuaManager::getDelayedCreation() const
    {
        return m_delayedCreation;
    }

    void LuaManager::setLuaState( lua_State *luaState )
    {
        m_luaState = luaState;
    }

    lua_State *LuaManager::getLuaState() const
    {
        return m_luaState.load();
    }

    void LuaManager::setEnableFullDebug( bool enableFullDebug )
    {
        m_enableFullDebug = enableFullDebug;
    }

    bool LuaManager::getEnableFullDebug() const
    {
        return m_enableFullDebug;
    }

    hash_type LuaManager::ScriptProfile::getHash() const
    {
        auto str = m_className + m_function;
        return StringUtil::getHash( str );
    }

}  // namespace workphone
