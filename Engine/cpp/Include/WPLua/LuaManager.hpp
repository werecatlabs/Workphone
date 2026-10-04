#ifndef LuaManager_h__
#define LuaManager_h__

#include <WPLua/WPLuaPrerequisites.hpp>
#include <Workphone/Interface/Script/IScriptManager.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentMap.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Memory/RawPtr.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <luabind/object.hpp>

namespace workphone
{

    /**
     * @class LuaManager
     * @brief Manages integration between the engine and Lua scripts.
     *
     * The LuaManager is responsible for creating and owning the Lua state, loading and
     * executing scripts, creating script-backed objects, invoking script functions and
     * methods, and providing utilities for debugging and profiling Lua code used by the
     * engine.
     *
     * Responsibilities:
     * - Maintain a single (or thread-safe) Lua state accessible via @c getLuaState / @c setLuaState.
     * - Load scripts from files or strings and keep track of loaded script file names.
     * - Create / destroy script objects and low-level class instances.
     * - Call global functions, class member functions and object member functions with
     *   optional parameter and result marshalling via the engine's @c Parameters type.
     * - Support reloading of scripts, optional delayed creation of script objects, and
     *   basic script-level profiling and breakpoints.
     *
     * Thread-safety notes:
     * - Public lock/try_lock/unlock methods are provided for callers to ensure thread-safe
     *   access when interacting with the manager from multiple threads.
     *
     * @note LuaManager implements IScriptManager.
     */
    class LuaManager : public IScriptManager
    {
    public:
        /**
         * @class ScriptProfile
         * @brief Holds profiling information for a single script call or function.
         *
         * Instances of ScriptProfile store the function/class name and time taken
         * for execution. This structure is used by the manager's profiling map to
         * aggregate or inspect performance characteristics of script code.
         */
        class ScriptProfile
        {
        public:
            /**
             * @brief Compute or return a precomputed hash for this profile entry.
             *
             * The hash may be used as a key in maps to identify a unique script
             * (for example by class + function combination).
             *
             * @return Computed hash value for the profile (stable for the lifetime of the profile).
             */
            hash_type getHash() const;

            String m_className;  ///< Lua class name associated with the profile entry.
            String m_function;   ///< Lua function name associated with the profile entry.
            time_interval
                m_timeTaken;  ///< Accumulated time taken by the function or last measured duration.
        };

        /**
         * @brief Create a new LuaManager.
         *
         * Constructor initializes internal state but does not necessarily create the
         * Lua state until @c load or @c createLuaState is called.
         */
        LuaManager();

        /**
         * @brief Virtual destructor.
         *
         * Ensures resources are released and the Lua state is cleaned up on shutdown.
         */
        ~LuaManager() override;

        /**
         * @brief Initialize the script manager using shared object initialization data.
         *
         * Typical use is to register the manager with the engine and create/prepare
         * any resources required by the scripts.
         *
         * @param data Optional initialization data provided by the engine.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload the script manager and release resources.
         *
         * This will attempt to unload scripts, destroy script objects and close the
         * Lua state if owned.
         *
         * @param data Optional cleanup data provided by the engine.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Called each frame / tick to update manager state.
         *
         * Performs maintenance tasks such as processing creation/unload queues,
         * running delayed object creation, and periodic garbage collection.
         */
        void update() override;

        /**
         * @brief Load and execute a Lua script from a file.
         *
         * If the script has already been loaded this call may reload it (behavior
         * depends on implementation). Errors during loading are handled via
         * @c handleLuaError and the manager's error flag.
         *
         * @param filename Path or resource name of the Lua script file to load.
         */
        void loadScript( const String &filename ) override;

        /**
         * @brief Load and execute a Lua script provided as a string.
         *
         * Useful for runtime-generated scripts or scripts embedded in other assets.
         *
         * @param str Lua source code to run.
         */
        void loadScriptFromString( const String &str ) override;

        /**
         * @brief Load multiple Lua scripts.
         *
         * Iterates over the provided array and loads each script file.
         *
         * @param scripts Array of script file paths/names to load.
         */
        void loadScripts( const Array<String> &scripts ) override;

        /**
         * @brief Call a global Lua function with no parameters and no results.
         *
         * Safe to call only when the function exists; missing functions are handled
         * by the manager and may set the error flag.
         *
         * @param functionName Fully-qualified or global function name in Lua.
         */
        void callFunction( const String &functionName ) override;

        /**
         * @brief Call a global Lua function with parameters.
         *
         * Parameters are marshalled from the engine's @c Parameters type into Lua.
         *
         * @param functionName Name of the Lua function to call.
         * @param parameters Parameters to pass to the Lua function.
         */
        void callFunction( const String &functionName, const Parameters &parameters ) override;

        /**
         * @brief Call a global Lua function with parameters and collect results.
         *
         * Results returned by the Lua function are marshalled back into @c results.
         *
         * @param functionName Name of the Lua function to call.
         * @param parameters Parameters to pass to the Lua function.
         * @param[out] results Container that will receive returned values from Lua.
         */
        void callFunction( const String &functionName, const Parameters &parameters,
                           Parameters &results ) override;

        /**
         * @brief Call a static/member function defined on a Lua class (no params).
         *
         * The return value is used as a status or result code by the engine.
         *
         * @param className Lua class name.
         * @param functionName Member function name to call.
         * @return Integer status or result code from the script call.
         */
        s32 callMember( const String &className, const String &functionName ) override;

        /**
         * @brief Call a class member function with parameters.
         *
         * @param className Name of the Lua class containing the function.
         * @param functionName Member function to call.
         * @param parameters Parameters to pass to the function.
         * @return Integer status or result code from the script call.
         */
        s32 callMember( const String &className, const String &functionName,
                        const Parameters &parameters ) override;

        /**
         * @brief Call a class member function with parameters and collect results.
         *
         * @param className Name of the Lua class containing the function.
         * @param functionName Member function to call.
         * @param parameters Parameters to pass to the function.
         * @param[out] results Container that will receive returned values from Lua.
         * @return Integer status or result code from the script call.
         */
        s32 callMember( const String &className, const String &functionName,
                        const Parameters &parameters, Parameters &results ) override;

        /**
         * @brief Call a member function on a specific script object (no params).
         *
         * @param object Script object instance to call into.
         * @param functionName Name of the member function on the object to invoke.
         */
        void callObjectMember( SmartPtr<ISharedObject> object, const String &functionName ) override;

        /**
         * @brief Call a member function on a specific script object with parameters.
         *
         * @param object Script object instance to call into.
         * @param functionName Function name to invoke on the object.
         * @param parameters Parameters to pass to the function.
         */
        void callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                               const Parameters &parameters ) override;

        /**
         * @brief Call a member function on a specific script object with parameters and results.
         *
         * @param object Script object instance to call into.
         * @param functionName Function name to invoke on the object.
         * @param parameters Parameters to pass to the function.
         * @param[out] results Container that will receive returned values from Lua.
         */
        void callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                               const Parameters &parameters, Parameters &results ) override;

        /**
         * @brief Create a script-backed object of the given class and associate it with the engine object.
         *
         * The created object implements IScriptClass and will be usable from both C++ and Lua.
         * Behaviour on failure is implementation-defined (may return null).
         *
         * @param className Name of the Lua class to instantiate.
         * @param object Engine-side shared object to associate with the script instance.
         * @return Smart pointer to the created IScriptClass instance or null on failure.
         */
        SmartPtr<IScriptClass> createObject( const String &className,
                                             SmartPtr<ISharedObject> object ) override;

        /**
         * @brief Destroy a script object previously created with @c createObject.
         *
         * Cleans up Lua-side resources and removes internal bookkeeping.
         *
         * @param object Engine-side shared object whose script object should be destroyed.
         */
        void destroyObject( SmartPtr<ISharedObject> object ) override;

        /**
         * @brief Create a raw (unmanaged) instance of a Lua class.
         *
         * Returns a raw pointer to an instance that must be explicitly destroyed using
         * @c destroyInstance to avoid leaks.
         *
         * @param className Lua class name to instantiate.
         * @return Raw pointer to the created instance or nullptr on failure.
         */
        void *createInstance( const String &className ) override;

        /**
         * @brief Destroy a raw instance created via @c createInstance.
         *
         * @param instance Pointer previously returned by @c createInstance.
         */
        void destroyInstance( void *instance ) override;

        /**
         * @brief Mark all loaded scripts to be reloaded on next update.
         *
         * This triggers a reload of script files (useful for hot-reload during development).
         */
        void reloadScripts() override;

        /**
         * @brief Query whether a reload has been requested and is pending.
         *
         * @return True if a reload is pending; otherwise false.
         */
        bool reloadPending() const override;

        /**
         * @brief Clear the current Lua stack for the active Lua state.
         *
         * This is a low-level utility for ensuring stack consistency after errors or
         * cross-boundary calls.
         */
        void clearStack();

        /**
         * @brief Retrieve debugging information from the Lua state.
         *
         * Returns a human-readable summary useful for logging or editor display.
         *
         * @return Formatted debug information string.
         */
        String getDebugInfo() override;

        /**
         * @brief Execute the provided Lua script text on the manager's Lua state.
         *
         * This is similar to @c loadScriptFromString but named to emphasise immediate execution.
         *
         * @param script Lua code to execute.
         */
        void executeScript( const String &script );

        /**
         * @brief Get whether the manager is running in full debug mode.
         *
         * Full debug mode may enable additional checks, verbose logging or detailed
         * stack traces when errors occur.
         *
         * @return True when full debug is enabled; otherwise false.
         */
        bool getEnableFullDebug() const;

        /**
         * @brief Enable or disable full debug mode.
         *
         * @param enableFullDebug True to enable extended debug logging and checks.
         */
        void setEnableFullDebug( bool enableFullDebug );

        /**
         * @brief Get raw pointer to the underlying lua_State.
         *
         * @return Pointer to the Lua state managed by this manager (may be nullptr).
         */
        lua_State *getLuaState() const;

        /**
         * @brief Set the underlying lua_State pointer.
         *
         * Ownership semantics depend on the caller/implementation; typically used
         * to inject an externally-created state.
         *
         * @param luaState Pointer to a lua_State to use.
         */
        void setLuaState( lua_State *luaState );

        /**
         * @brief Get whether delayed creation of script objects is enabled.
         *
         * When enabled, creation of script objects may be queued and executed on the
         * next update cycle.
         *
         * @return True when delayed creation is enabled.
         */
        bool getDelayedCreation() const override;

        /**
         * @brief Enable or disable delayed creation of script objects.
         *
         * @param delayedCreation True to queue object creation until the next update.
         */
        void setDelayedCreation( bool delayedCreation ) override;

        /**
         * @brief Query whether debug mode (lighter-weight than full debug) is enabled.
         *
         * @return True if debug checks and logging are enabled.
         */
        bool isDebugEnabled() const override;

        /**
         * @brief Enable or disable debug mode.
         *
         * @param enable True to enable debug mode; false to disable.
         */
        void setDebugEnabled( bool enable ) override;

        /**
         * @brief Get the current error state for the manager.
         *
         * @return True if the manager has recorded an error condition; otherwise false.
         */
        bool getError() const;

        /**
         * @brief Set or clear the manager's recorded error state.
         *
         * @param error True to mark an error state; false to clear it.
         */
        void setError( bool error );

        /**
         * @brief Remove a script breakpoint.
         *
         * Breakpoints are used by the engine/editor to pause or inspect script execution.
         *
         * @param breakpoint Breakpoint object to remove.
         */
        void removeBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint ) override;

        /**
         * @brief Add a script breakpoint.
         *
         * @param breakpoint Breakpoint object to register with the manager.
         */
        void addBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint ) override;

        /**
         * @brief Return a copy of all registered script breakpoints.
         *
         * @return Array containing smart pointers to registered breakpoints.
         */
        Array<SmartPtr<IScriptBreakpoint>> getBreakpoints() const override;

        /**
         * @brief Retrieve the underlying script object pointer.
         *
         * Implementation detail: used by systems that require raw access to the
         * script runtime object. The pointer is returned via the out parameter.
         *
         * @param[out] object Address to receive a void* pointer to the underlying script object.
         */
        void _getObject( void **object ) override;

        /**
         * @brief Run a full garbage collection pass on the Lua state.
         *
         * Useful to force cleanup at predictable times (for example during level unload).
         */
        void garbageCollect() override;

        /**
         * @brief Register a native C++ class or pointer with the Lua state.
         *
         * The @p ptr is typically a type descriptor or registration helper that allows
         * the binding library to expose C++ types to Lua.
         *
         * @param ptr Pointer to type registration data or class descriptor.
         */
        void registerClass( void *ptr ) override;

        /**
         * @brief Get the names of all script classes currently known to the manager.
         *
         * @return Array of class name strings.
         */
        Array<String> getClassNames() const override;

        /**
         * @brief Replace the internal list of script class names.
         *
         * @param classNames New list of class names to store.
         */
        void setClassNames( const Array<String> &classNames ) override;

        /**
         * @brief Print the active Lua stack for the manager's Lua state.
         *
         * Convenience helper that prints stack contents to the logging facility or console.
         */
        void print_lua_stack();

        /**
         * @brief Print the Lua stack for an arbitrary lua_State.
         *
         * @param L Lua state whose stack should be printed.
         */
        void print_lua_stack( lua_State *L );

        /**
         * @brief Refresh the internal list of class names discovered from the binding registry.
         *
         * Typically called after registering new bindings or reloading scripts.
         */
        void updateClassNames();

        /**
         * @brief Refresh the list of classes registered via binding code (luabind / custom bindings).
         */
        void updateBindClassNames();

        /**
         * @brief Update script-side metadata for all registered script classes.
         *
         * Called when script definitions change and the engine needs to resync script metadata.
         */
        void updateScriptData();

        /**
         * @brief Update value/variable members for all script classes from engine state.
         */
        void updateScriptDataVariables();

        /**
         * @brief Retrieve a portion of source code around a specific line/column.
         *
         * Useful for editor features such as showing the source context for errors or breakpoints.
         *
         * @param source Full source text.
         * @param line 1-based line number to query.
         * @param column 1-based column number to query.
         * @return Substring containing the requested source context (may be empty on invalid input).
         */
        String getSourceCode( const String &source, s32 line, s32 column );

        /**
         * @brief Get supported script file extensions (for file dialogs / asset importers).
         *
         * @return Array of extension strings (for example ".lua").
         */
        Array<String> getSupportedFileExtensions() const override;

        /**
         * @brief Enqueue or immediately load a script object instance.
         *
         * When @p forceQueue is false the object may be loaded immediately; when true it is placed
         * on an internal load queue for processing during update().
         *
         * @param scriptObject Engine-side object to load the script for.
         * @param forceQueue If true, force queuing the object for deferred loading.
         */
        void loadObject( SmartPtr<ISharedObject> scriptObject, bool forceQueue = false ) override;

        /**
         * @brief Enqueue or immediately unload a script object instance.
         *
         * @param scriptObject Engine-side object to unload the script for.
         * @param forceQueue If true, force queuing the object for deferred unloading.
         */
        void unloadObject( SmartPtr<ISharedObject> scriptObject, bool forceQueue = false ) override;

        /**
         * @brief Global C-style error handler for Lua errors.
         *
         * This static function can be registered with Lua as an error handler; it
         * should route Lua error information into the manager's logging and error state.
         *
         * @param luaState Lua state where the error occurred.
         */
        static void handleLuaError( lua_State *luaState );

        /**
         * @brief Acquire the manager lock for thread-safe operations.
         *
         * Use this when performing multiple operations on the manager from different threads.
         * This method blocks until the lock is acquired.
         */
        void lock() override;
        void lock_shared() override;

        /**
         * @brief Try to acquire the manager lock without blocking.
         *
         * @return True if the lock was successfully acquired, false if it is already held.
         */
        bool try_lock() override;

        /**
         * @brief Release the manager lock previously acquired with @c lock or @c try_lock.
         */
        void unlock() override;
        void unlock_shared() override;

        WP_CLASS_REGISTER_DECL;

    private:
        /**
         * @brief Create a Lua-side instance for the supplied LuaObjectData.
         *
         * Allocates and binds a luabind::object (or equivalent) to the engine-side owner.
         *
         * @param objectData Data describing the script instance to create.
         * @return True when the Lua-side instance was created successfully.
         */
        bool createLuaInstance( SmartPtr<LuaObjectData> objectData );

        /**
         * @brief Internal implementation for calling an object's member function (no params).
         *
         * @param object Engine-side object to call into.
         * @param functionName Member function name.
         * @return Integer status or result code returned by the script invocation.
         */
        s32 _callObjectMember( SmartPtr<ISharedObject> object, const String &functionName );

        /**
         * @brief Internal implementation for calling an object's member function with parameters and results.
         *
         * Marshals parameters into Lua and writes returned values into @p results.
         *
         * @param object Engine-side object to call into.
         * @param functionName Name of the function to call.
         * @param parameters Parameters to pass to the function.
         * @param[out] results Container to receive results from the call.
         * @return Integer status or result code returned by the script invocation.
         */
        s32 _callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                               const Parameters &parameters, Parameters &results );

        /**
         * @brief Internal implementation for calling an object's member function with parameters (no results).
         *
         * @param object Engine-side object to call into.
         * @param functionName Name of the function to call.
         * @param parameters Parameters to pass to the function.
         * @return Integer status or result code returned by the script invocation.
         */
        s32 _callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                               const Parameters &parameters );

        /**
         * @brief Handle the situation where a script object is not found or invalid.
         *
         * Typically logs an error and may set internal error state.
         */
        void errorObjectNotFound();

        /**
         * @brief Create and initialise the lua_State used by this manager.
         *
         * Sets up bindings, registers library functions and configures the state for use.
         */
        void createLuaState();

        /**
         * @brief Internal implementation that performs the actual script reload.
         *
         * Called when a reload is pending to iterate loaded scripts, reload them and
         * refresh bindings and script metadata.
         */
        void _reloadScripts();

        ///< Thread-safe pointer to the active Lua state.
        AtomicRawPtr<lua_State> m_luaState;

        ///< Singleton null script object used as a safe fallback.
        AtomicRawPtr<NullScriptObject> m_nullScriptObject;

        ///< Time taken by the last measured operation (profiling).
        time_interval m_timeTaken = 0;

        ///< Timestamp (or delta accumulator) for scheduling next GC pass.
        time_interval m_nextGcUpdate = 0;

        ///< Simple counter of script function calls (used for profiling/metrics).
        atomic_s32 m_callCounter = 0;

        ///< True when a script reload has been requested.
        atomic_bool m_bReload = false;

        ///< True when full debug mode (verbose) is enabled.
        atomic_bool m_enableFullDebug = false;

        ///< True when script object creation should be delayed/queued.
        atomic_bool m_delayedCreation = false;

        ///< True when debug mode (lighter weight) is enabled.
        atomic_bool m_isDebugEnabled = false;

        ///< Manager-level error flag that callers can query.
        atomic_bool m_bError = false;

        String m_curClass;         ///< Current class being invoked (used for profiling/debug).
        String m_curFunction;      ///< Current function being invoked (used for profiling/debug).
        String m_profileClass;     ///< Class name used for profiling aggregation.
        String m_profileFunction;  ///< Function name used for profiling aggregation.

        ///< List of loaded script filenames.
        ConcurrentArray<FixedString<512>> m_scripts;

        ///< Data for all Lua-backed objects.
        ConcurrentArray<SmartPtr<LuaObjectData>> m_objectData;

        ///< Objects pending creation (deferred).
        ConcurrentArray<SmartPtr<LuaObjectData>> m_creationList;

        ///< Registered script breakpoints.
        ConcurrentArray<SmartPtr<IScriptBreakpoint>> m_breakpoints;

        ///< Raw pointers to luabind objects / instances.
        ConcurrentArray<RawPtr<luabind::object>> m_instances;

        ///< Class names registered via binding code.
        ConcurrentArray<FixedString<128>> m_bindingClassNames;

        ///< All discovered script class names.
        ConcurrentArray<FixedString<128>> m_classNames;

        ///< Metadata for script classes managed by the engine.
        ConcurrentArray<IScriptClass> m_scriptData;

        ///< Queue of objects to load on update().
        ConcurrentQueue<SmartPtr<ISharedObject>> m_loadQueue;

        ///< Queue of objects to unload on update().
        ConcurrentQueue<SmartPtr<ISharedObject>> m_unloadQueue;

        using ScriptProfiles =
            ConcurrentMap<u32, ScriptProfile>;  ///< Map keyed by hash to profiling entries.
        ScriptProfiles m_scriptProfiles;        ///< Aggregated profiling data for scripts.

        mutable RecursiveSpinMutex m_mutex;  ///< Mutex for thread-safe access to the manager.
    };

}  // namespace workphone

#endif  // LuaManager_h__
