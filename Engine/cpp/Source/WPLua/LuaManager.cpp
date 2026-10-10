#include <WPLua/LuaManager.hpp>
#include <WPLuabind/ParamConverter.hpp>
#include <WPLua/NullScriptObject.hpp>
#include <WPLua/LuaObjectData.hpp>
#include <WPLua/LuaScriptCompiler.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/System/IPlugin.hpp>
#include <Workphone/Interface/System/IPluginManager.hpp>
#include <WPLuabind/WPLuabind.hpp>
#include <sstream>
#include <stdarg.h>
#include <iostream>
#include <stdexcept>

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
    namespace
    {
        int scriptTraceback( lua_State *state )
        {
            // Do not invoke an arbitrary error object's __tostring in the handler.
            const char *message = lua_tostring( state, 1 );
            if( !message ) message = "Lua raised a non-string error";
            luaL_traceback( state, state, message, 1 );
            return 1;
        }

        struct ScriptInvocation
        {
            luabind::object *receiver;
            const char *globalName;
            const char *functionName;
            const Parameters *parameters;
            Parameters *results;
            bool optional;
        };

        struct ScriptConstruction
        {
            const char *className;
            SmartPtr<ISharedObject> *owner;
        };

        int constructScript( lua_State *state )
        {
            auto *call = static_cast<ScriptConstruction *>( lua_touserdata( state, 1 ) );
            lua_getglobal( state, call->className );
            if( lua_isnil( state, -1 ) )
                return luaL_error( state, "Lua class '%s' was not found", call->className );
            if( call->owner ) luabind::detail::convert_to_lua( state, *call->owner );
            lua_call( state, call->owner ? 1 : 0, 1 );
            if( lua_isnil( state, -1 ) )
                return luaL_error( state, "Lua class '%s' returned nil", call->className );
            return 1;
        }

        int invokeScript( lua_State *state )
        {
            auto *call = static_cast<ScriptInvocation *>( lua_touserdata( state, 1 ) );
            int arguments = 0;
            if( call->receiver || call->globalName )
            {
                if( call->receiver ) call->receiver->push( state );
                else lua_getglobal( state, call->globalName );
                lua_getfield( state, -1, call->functionName );
                lua_insert( state, -2 ); // function, self
                ++arguments;
            }
            else lua_getglobal( state, call->functionName );

            if( call->optional && lua_isnil( state, -( arguments + 1 ) ) ) return 0;
            if( !lua_isfunction( state, -( arguments + 1 ) ) )
                return luaL_error( state, "Lua function '%s' was not found", call->functionName );

            // Keep the established Parameters and mutable results-container ABI.
            if( call->parameters )
            {
                luabind::detail::convert_to_lua( state, *call->parameters );
                ++arguments;
            }
            if( call->results )
            {
                luabind::detail::convert_to_lua( state, boost::ref( *call->results ) );
                ++arguments;
            }
            lua_call( state, arguments, 0 );
            return 0;
        }
    }

    WP_CLASS_REGISTER_DERIVED( workphone, LuaManager, IScriptManager );

    TValue *index2value( lua_State *L, int idx );

    LuaManager::LuaManager() = default;

    LuaManager::~LuaManager()
    {
        unload( nullptr );
    }

    void LuaManager::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() ) return;
        try
        {
            setLoadingState( LoadingState::Loading );

            ScopedLock lock( this );

            createLuaState();
            m_bReload = false;

            m_timeTaken = 0.0f;
            m_callCounter = 0;
            m_bError = false;
            m_lastDiagnostic.clear();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            if( auto state = getLuaState() ) lua_close( state );
            m_luaState = nullptr;
            m_bError = true;
            m_lastDiagnostic = e.what();
            setLoadingState( LoadingState::Unloaded );
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
            m_loadQueue.clear();
            m_unloadQueue.clear();
            m_scripts.clear();
            m_loadedScriptAssets.clear();
            m_classNames.clear();
            m_bindingClassNames.clear();
            m_scriptData.clear();
            m_bReload = false;

            lua_gc( m_luaState, LUA_GCCOLLECT, 0 );

            // closes the lua state
            lua_close( m_luaState );
            m_luaState = nullptr;

            auto nullScriptObject = m_nullScriptObject.load();
            WP_SAFE_DELETE( nullScriptObject );
            m_nullScriptObject = nullptr;

            setLoadingState( LoadingState::Unloaded );
        }
    }

    void LuaManager::setClassNames( const Array<String> &classNames )
    {
        m_classNames = { classNames.begin(), classNames.end() };
    }

    void LuaManager::handleLuaError( lua_State *luaState )
    {
        const auto text = lua_tostring( luaState, -1 );
        auto manager = *static_cast<LuaManager **>( lua_getextraspace( luaState ) );
        if( manager ) manager->setError( true );
        WP_LOG_ERROR( text ? text : "Lua raised a non-string error" );
        lua_pop( luaState, 1 );
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
        auto manager = *static_cast<LuaManager **>( lua_getextraspace( luaState ) );
        if( manager ) manager->setError( true );
        return scriptTraceback( luaState );
    }

    void handleCastFailed( lua_State *luaState, const luabind::type_id &id )
    {
        auto manager = *static_cast<LuaManager **>( lua_getextraspace( luaState ) );
        if( manager ) manager->setError( true );
        WP_LOG_ERROR( "Lua native value conversion failed" );
    }

    void LuaManager::loadScript( const String &filename )
    {
        ScopedLock lock( this );
        if( !isLoaded() ) return;
        if( std::find( m_scripts.begin(), m_scripts.end(), filename ) != m_scripts.end() ) return;
        auto app = core::IApplicationManager::instancePtr();
        auto fs = app ? app->getFileSystemPtr() : nullptr;
        if( !fs )
        {
            m_lastDiagnostic = "No filesystem available for Lua source: " + filename;
            setError( true );
            return;
        }
        auto stream = fs->open( filename, true, false, false, false, false );
        if( !stream ) stream = fs->open( filename, true, false, false, true, true );
        if( !stream )
        {
            m_lastDiagnostic = "Lua source not found: " + filename;
            setError( true );
            WP_LOG_ERROR( m_lastDiagnostic );
            return;
        }
        if( executeSource( stream->getAsString(), "@" + filename ) )
            m_scripts.push_back( filename );
    }

    void LuaManager::loadScriptFromString( const String &str )
    {
        executeSource( str, "=inline" );
    }

    void LuaManager::print_lua_stack()
    {
        auto L = getLuaState();
        if( !L ) return;
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
        lua_pop( L, 1 ); // global environment
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
        executeSource( script, "=script" );
    }

    String LuaManager::getLastDiagnostic() const
    {
        ScopedLock lock( const_cast<LuaManager *>( this ) );
        return m_lastDiagnostic;
    }

    void LuaManager::configureScriptResources( SmartPtr<AssetDatabaseManager> catalog,
                                               std::shared_ptr<resource::IResourceSystem> resources,
                                               bool compiledOnly )
    {
        ScopedLock lock( this );
        m_scriptCatalog = catalog;
        m_scriptResources = resources;
        m_compiledScriptsOnly = compiledOnly;
    }

    bool LuaManager::loadScriptResource( std::shared_ptr<const resource::RuntimeResource> resource )
    {
        ScopedLock lock( this );
        LuaScriptCompiler compiler;
        if( !resource || resource->header.resourceType != resource::ResourceTypeID( "lua" ) ||
            resource->header.compilerVersion != compiler.versionFor( resource::ResourceTypeID( "lua" ) ) ||
            resource->payload.size() > LuaScriptCompiler::maxSourceBytes ||
            resource->header.payloadSize != resource->payload.size() ||
            resource->header.payloadHash != resource::hashBytes( resource->payload.data(), resource->payload.size() ) )
        {
            m_lastDiagnostic = "Invalid or incompatible compiled Lua resource";
            setError( true );
            return false;
        }
        const String source( reinterpret_cast<const char *>( resource->payload.data() ), resource->payload.size() );
        return executeSource( source, "@" + resource->header.resourceId.sourceRelativePath() );
    }

    bool LuaManager::loadScriptAsset( const String &uuid )
    {
        ScopedLock lock( this );
        auto fail = [&]( const String &message ) {
            m_lastDiagnostic = message;
            setError( true );
            return false;
        };
        auto catalog = m_scriptCatalog;
        if( !catalog )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto database = app ? app->getResourceDatabase() : nullptr;
            if( database ) catalog = dynamic_pointer_cast<AssetDatabaseManager>( database->getDatabaseManager() );
        }
        if( !catalog ) return fail( "No script asset catalog is configured" );
        AssetDatabaseManager::EntrySnapshot entry;
        if( !catalog->tryGetEntry( uuid, entry ) || entry.kind != AssetDatabaseManager::EntryKind::File ||
            entry.type != "script" ) return fail( "Script asset UUID is missing or has the wrong type: " + uuid );
        const auto loaded = m_loadedScriptAssets.find( uuid );
        if( loaded != m_loadedScriptAssets.end() ) return true;
        if( m_scriptResources )
        {
            CatalogResourceAdapter adapter( catalog, m_scriptResources, catalog->getProjectRoot(),
                                            { { "script", resource::ResourceTypeID( "lua" ) } } );
            CatalogResourceAdapter::Request request;
            String error;
            if( !adapter.resolve( uuid, request, error ) ) return fail( error );
            if( !m_compiledScriptsOnly )
            {
                auto report = adapter.compile( request );
                if( !report.succeeded() ) return fail( report.messages.empty() ? "Lua compilation failed" : report.messages[0] );
            }
            auto compiled = adapter.load( request, error );
            if( !compiled || !adapter.isCurrent( request, error ) ) return fail( error );
            if( !loadScriptResource( compiled ) ) return false;
            m_loadedScriptAssets[uuid] = compiled->header.sourceHash;
            return true;
        }
        if( m_compiledScriptsOnly ) return fail( "Compiled Lua resource service is required" );
        auto app = core::IApplicationManager::instancePtr();
        auto fs = app ? app->getFileSystemPtr() : nullptr;
        if( !fs ) return fail( "No filesystem available for script asset" );
        const String path = catalog->getProjectRoot() + "/" + entry.path;
        auto stream = fs->open( path, true, false, false, false, false );
        if( !stream || !catalog->isEntryCurrent( entry ) ) return fail( "Script asset source is missing or stale" );
        auto source = stream->getAsString();
        if( !executeSource( source, "@" + entry.path ) ) return false;
        m_loadedScriptAssets[uuid] = resource::hashString( source );
        return true;
    }

    bool LuaManager::executeSource( const String &source, const String &sourceName )
    {
        ScopedLock lock( this );
        auto state = getLuaState();
        m_lastDiagnostic.clear();
        setError( false );
        if( !state )
        {
            m_lastDiagnostic = "Lua state is not loaded";
            setError( true );
            return false;
        }
        const int top = lua_gettop( state );
        lua_pushcfunction( state, scriptTraceback );
        const int handler = top + 1;
        int status = luaL_loadbufferx( state, source.data(), source.size(),
                                      sourceName.c_str(), "t" );
        if( status == LUA_OK ) status = lua_pcall( state, 0, 0, handler );
        if( status != LUA_OK )
        {
            const auto message = lua_tostring( state, -1 );
            m_lastDiagnostic = message ? message : "Lua raised a non-string error";
            setError( true );
            WP_LOG_ERROR( m_lastDiagnostic );
        }
        lua_settop( state, top );
        return status == LUA_OK;
    }

    bool LuaManager::invokeLua( luabind::object *receiver, const String &globalName,
                                const String &functionName, const Parameters *parameters,
                                Parameters *results, bool optional )
    {
        ScopedLock lock( this );
        auto state = getLuaState();
        setError( false );
        m_lastDiagnostic.clear();
        if( !state )
        {
            m_lastDiagnostic = "Lua state is not loaded";
            setError( true );
            return false;
        }
        const int top = lua_gettop( state );
        ScriptInvocation call{ receiver, globalName.empty() ? nullptr : globalName.c_str(),
                               functionName.c_str(), parameters, results, optional };
        lua_pushcfunction( state, scriptTraceback );
        lua_pushcfunction( state, invokeScript );
        lua_pushlightuserdata( state, &call );
        int status = lua_pcall( state, 1, 0, top + 1 );
        if( status != LUA_OK )
        {
            const auto message = lua_tostring( state, -1 );
            m_lastDiagnostic = message ? message : "Lua invocation failed";
            setError( true );
            WP_LOG_ERROR( m_lastDiagnostic );
        }
        lua_settop( state, top );
        return status == LUA_OK;
    }

    void LuaManager::callFunction( const String &functionName )
    {
        invokeLua( nullptr, "", functionName, nullptr, nullptr );
    }

    void LuaManager::callFunction( const String &functionName, const Parameters &parameters )
    {
        invokeLua( nullptr, "", functionName, &parameters, nullptr );
    }

    void LuaManager::callFunction( const String &functionNameStr, const Parameters &parameters,
                                   Parameters &results )
    {
        invokeLua( nullptr, "", functionNameStr, &parameters, &results );
    }

    s32 LuaManager::callMember( const String &className, const String &functionName )
    {
        return invokeLua( nullptr, className, functionName, nullptr, nullptr ) ? 0 : -1;
    }

    s32 LuaManager::callMember( const String &className, const String &functionName,
                               const Parameters &parameters )
    {
        return invokeLua( nullptr, className, functionName, &parameters, nullptr ) ? 0 : -1;
    }

    s32 LuaManager::callMember( const String &className, const String &functionName,
                               const Parameters &parameters, Parameters &results )
    {
        return invokeLua( nullptr, className, functionName, &parameters, &results ) ? 0 : -1;
    }

    void LuaManager::callObjectMember( SmartPtr<ISharedObject> object, const String &functionName )
    {
        _callObjectMember( object, functionName );
    }

    void LuaManager::callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                     const Parameters &parameters )
    {
        _callObjectMember( object, functionName, parameters );
    }

    void LuaManager::callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                     const Parameters &parameters, Parameters &results )
    {
        _callObjectMember( object, functionName, parameters, results );
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
        ScopedLock lock( this );
        auto luaState = getLuaState();
        if( luaState ) lua_settop( luaState, 0 );
    }

    void LuaManager::createLuaState()
    {
        // create the lua state

        auto luaState = luaL_newstate();
        if( !luaState ) throw std::runtime_error( "Could not allocate Lua state" );
        setLuaState( luaState );
        *static_cast<LuaManager **>( lua_getextraspace( luaState ) ) = this;

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
        // Component callbacks derive from the vehicle interfaces.
        bindVehicle( luaState );
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
        ScopedLock lock( this );
        String result;
        auto state = getLuaState();
        if( !state ) return result;
        lua_Debug frame{};
        for( int level = 0; lua_getstack( state, level, &frame ); ++level )
        {
            lua_getinfo( state, "Sln", &frame );
            std::stringstream text;
            text << frame.short_src << ":" << frame.currentline;
            if( frame.name ) text << " (" << frame.name << ")";
            result += text.str() + "\n";
        }
        return result;
    }

    SmartPtr<IScriptClass> LuaManager::createObject( const String &className,
                                                    SmartPtr<ISharedObject> object )
    {
        ScopedLock lock( this );
        if( !object || !isLoaded() ) return nullptr;
        auto existing = dynamic_pointer_cast<LuaObjectData>( object->getScriptData() );
        if( existing && existing->getLuaState() == getLuaState() &&
            existing->getClassName() == className && existing->getObject() )
            return existing->getClassData();
        try
        {
            auto data = make_ptr<LuaObjectData>();
            data->setClassName( className );
            data->setLuaState( getLuaState() );
            data->setOwner( object );
            if( !createLuaInstance( data ) ) return nullptr;
            data->load( nullptr );
            // Publish only a fully constructed and inspected instance.
            if( existing ) destroyObject( object );
            object->setScriptData( data );
            m_objectData.push_back( data );
            return data->getClassData();
        }
        catch( const std::exception &error )
        {
            m_lastDiagnostic = error.what();
            setError( true );
            WP_LOG_EXCEPTION( error );
            return nullptr;
        }
    }

    void LuaManager::destroyObject( SmartPtr<ISharedObject> object )
    {
        if( isLoaded() )
        {
            ScopedLock lock( this );

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

    s32 LuaManager::invokeObject( SmartPtr<ISharedObject> object, const String &functionName,
                                 const Parameters *parameters, Parameters *results )
    {
        ScopedLock lock( this );
        auto data = object ? dynamic_pointer_cast<LuaObjectData>( object->getScriptData() ) : nullptr;
        if( !isLoaded() || !data || data->getLuaState() != getLuaState() || !data->getObject() )
        {
            m_lastDiagnostic = "Lua callback has no live instance: " + functionName;
            setError( true );
            return -1;
        }
        // Resolve dynamically so inherited callbacks and reloaded methods work.
        // An absent lifecycle callback is optional; an invalid present value is an error.
        return invokeLua( &data->getObject(), "", functionName, parameters, results, true ) ? 0 : -1;
    }

    s32 LuaManager::_callObjectMember( SmartPtr<ISharedObject> object, const String &functionName )
    {
        return invokeObject( object, functionName, nullptr, nullptr );
    }

    s32 LuaManager::_callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                     const Parameters &parameters )
    {
        return invokeObject( object, functionName, &parameters, nullptr );
    }

    s32 LuaManager::_callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                     const Parameters &parameters, Parameters &results )
    {
        return invokeObject( object, functionName, &parameters, &results );
    }

    void LuaManager::update()
    {
        if( !isLoaded() || Thread::getCurrentTask() != TaskId::Application ) return;
        ScopedLock lock( this );
        // Drain a snapshot, so a callback that enqueues work cannot starve the frame.
        SmartPtr<ISharedObject> object;
        auto unloadCount = m_unloadQueue.size();
        while( unloadCount-- && m_unloadQueue.try_pop( object ) )
            if( object ) object->unload( nullptr );
        auto loadCount = m_loadQueue.size();
        while( loadCount-- && m_loadQueue.try_pop( object ) )
            if( object ) object->load( nullptr );
        if( m_bReload )
        {
            _reloadScripts();
            m_bReload = false;
        }
        for( auto &data : m_creationList )
            if( data && createLuaInstance( data ) ) data->load( nullptr );
        m_creationList.clear();
        // Leave the collector running; avoid periodic full-heap pauses.
        if( auto state = getLuaState() ) lua_gc( state, LUA_GCSTEP, 32 );
    }

    void LuaManager::_reloadScripts()
    {
        ScopedLock lock( this );
        // Legacy raw pointers cannot be rebound without invalidating their callers.
        if( !m_instances.empty() )
        {
            m_lastDiagnostic = "Reload requires releasing legacy raw Lua instances first";
            setError( true );
            return;
        }
        auto app = core::IApplicationManager::instancePtr();
        auto fs = app ? app->getFileSystemPtr() : nullptr;
        if( !m_scripts.empty() && !fs )
        {
            m_lastDiagnostic = "Reload has no source filesystem";
            setError( true );
            return;
        }
        // Stage on this executor. Luabind is not safe for concurrent state execution.
        // Native side effects in module bodies/constructors cannot be rolled back;
        // modules must keep initialization declarative until promotion.
        LuaManager candidate;
        candidate.configureScriptResources( m_scriptCatalog, m_scriptResources, m_compiledScriptsOnly );
        candidate.load( nullptr );
        if( !candidate.isLoaded() )
        {
            m_lastDiagnostic = candidate.getLastDiagnostic();
            setError( true );
            return;
        }
        Array<luabind::object> instances;
        for( const auto &asset : m_loadedScriptAssets )
        {
            if( !candidate.loadScriptAsset( asset.first ) )
            {
                m_lastDiagnostic = candidate.getLastDiagnostic();
                setError( true );
                return;
            }
        }
        for( const auto &filename : m_scripts )
        {
            auto stream = fs->open( filename.str(), true, false, false, false, false );
            if( !stream ) stream = fs->open( filename.str(), true, false, false, true, true );
            if( !stream || !candidate.executeSource( stream->getAsString(), "@" + filename.str() ) )
            {
                m_lastDiagnostic = stream ? candidate.getLastDiagnostic() : "Reload source missing: " + filename.str();
                setError( true );
                return;
            }
        }
        for( auto &data : m_objectData )
        {
            luabind::object value;
            auto owner = data->getOwner();
            if( owner && !candidate.constructLua( data->getClassName(), &owner, value ) )
            {
                m_lastDiagnostic = candidate.getLastDiagnostic();
                setError( true );
                return;
            }
            instances.push_back( value );
        }
        auto previous = getLuaState();
        auto replacement = candidate.getLuaState();
        for( auto &data : m_objectData ) data->getObject() = luabind::object();
        setLuaState( replacement );
        *static_cast<LuaManager **>( lua_getextraspace( replacement ) ) = this;
        candidate.setLuaState( nullptr );
        candidate.setLoadingState( LoadingState::Unloaded );
        m_bindingClassNames = candidate.m_bindingClassNames;
        m_loadedScriptAssets = candidate.m_loadedScriptAssets;
        for( size_t i = 0; i < m_objectData.size(); ++i )
        {
            auto &data = m_objectData[i];
            data->setLuaState( replacement );
            data->setObject( instances[i] );
            data->load( nullptr ); // Refresh callback metadata against the new class.
        }
        instances.clear();
        if( previous ) lua_close( previous );
        updateClassNames();
        m_lastDiagnostic.clear();
        setError( false );
    }

    bool LuaManager::constructLua( const String &className, SmartPtr<ISharedObject> *owner,
                                   luabind::object &result )
    {
        ScopedLock lock( this );
        auto state = getLuaState();
        if( !state ) return false;
        const int top = lua_gettop( state );
        ScriptConstruction call{ className.c_str(), owner };
        lua_pushcfunction( state, scriptTraceback );
        lua_pushcfunction( state, constructScript );
        lua_pushlightuserdata( state, &call );
        int status = lua_pcall( state, 1, 1, top + 1 );
        if( status == LUA_OK )
        {
            result = luabind::object( luabind::from_stack( state, -1 ) );
            m_lastDiagnostic.clear();
            setError( false );
        }
        else
        {
            const auto text = lua_tostring( state, -1 );
            m_lastDiagnostic = text ? text : "Lua construction failed";
            setError( true );
            WP_LOG_ERROR( m_lastDiagnostic );
        }
        lua_settop( state, top );
        return status == LUA_OK;
    }

    bool LuaManager::createLuaInstance( SmartPtr<LuaObjectData> data )
    {
        ScopedLock lock( this );
        if( !data ) return false;
        SmartPtr<ISharedObject> owner = data->getOwner();
        if( !owner ) return false;
        luabind::object instance;
        if( !constructLua( data->getClassName(), &owner, instance ) ) return false;
        data->setLuaState( getLuaState() );
        data->setObject( instance );
        return true;
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
        luabind::object value;
        if( !constructLua( className, nullptr, value ) ) return nullptr;
        auto instance = new luabind::object( value );
        m_instances.push_back( instance );
        return instance;
    }

    void LuaManager::destroyInstance( void *instance )
    {
        ScopedLock lock( this );

        auto object = static_cast<luabind::object *>( instance );
        if( std::find( m_instances.begin(), m_instances.end(), object ) == m_instances.end() )
            return;
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
