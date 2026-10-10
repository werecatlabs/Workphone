#ifndef _WP_IScriptManager_H
#define _WP_IScriptManager_H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Parameter.hpp>

namespace workphone
{

    /**
     * @brief Interface for a script manager responsible for loading, executing, and managing scripts and
     * script objects.
     *
     * This interface provides methods to load scripts from files or strings, execute script functions,
     * manage script objects and instances, handle debugging (breakpoints), and perform garbage
     * collection. It is designed to be implemented by classes that integrate scripting capabilities into
     * the engine.
     */
    class WPCore_API IScriptManager : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        ~IScriptManager() override;

        /**
         * @brief Loads a script file.
         * @param filename The name of the script file to load.
         */
        virtual void loadScript( const String &filename ) = 0;

        /**
         * @brief Loads multiple script files.
         * @param scripts The list of script file names to load.
         */
        virtual void loadScripts( const Array<String> &scripts ) = 0;

        /**
         * @brief Loads a script from a string.
         * @param str The script source code as a string.
         */
        virtual void loadScriptFromString( const String &str ) = 0;

        /**
         * @brief Executes a global script function with no parameters.
         * @param functionName The name of the function to execute.
         */
        virtual void callFunction( const String &functionName ) = 0;

        /**
         * @brief Executes a global script function with parameters.
         * @param functionName The name of the function to execute.
         * @param parameters The parameters to pass to the function.
         */
        virtual void callFunction( const String &functionName, const Parameters &parameters ) = 0;

        /**
         * @brief Executes a global script function with parameters and retrieves results.
         * @param functionName The name of the function to execute.
         * @param parameters The parameters to pass to the function.
         * @param results The results returned by the function.
         */
        virtual void callFunction( const String &functionName, const Parameters &parameters,
                                   Parameters &results ) = 0;

        /**
         * @brief Executes a member function of a script class with no parameters.
         * @param className The name of the class.
         * @param functionName The name of the member function to execute.
         * @return Status code or result of the function call.
         */
        virtual s32 callMember( const String &className, const String &functionName ) = 0;

        /**
         * @brief Executes a member function of a script class with parameters.
         * @param className The name of the class.
         * @param functionName The name of the member function to execute.
         * @param parameters The parameters to pass to the function.
         * @return Status code or result of the function call.
         */
        virtual s32 callMember( const String &className, const String &functionName,
                                const Parameters &parameters ) = 0;

        /**
         * @brief Executes a member function of a script class with parameters and retrieves results.
         * @param className The name of the class.
         * @param functionName The name of the member function to execute.
         * @param parameters The parameters to pass to the function.
         * @param results The results returned by the function.
         * @return Status code or result of the function call.
         */
        virtual s32 callMember( const String &className, const String &functionName,
                                const Parameters &parameters, Parameters &results ) = 0;

        /**
         * @brief Executes a member function on a script object with no parameters.
         * @param object The script object.
         * @param functionName The name of the member function to execute.
         */
        virtual void callObjectMember( SmartPtr<ISharedObject> object, const String &functionName ) = 0;

        /**
         * @brief Executes a member function on a script object with parameters.
         * @param object The script object.
         * @param functionName The name of the member function to execute.
         * @param parameters The parameters to pass to the function.
         */
        virtual void callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                       const Parameters &parameters ) = 0;

        /**
         * @brief Executes a member function on a script object with parameters and retrieves results.
         * @param object The script object.
         * @param functionName The name of the member function to execute.
         * @param parameters The parameters to pass to the function.
         * @param results The results returned by the function.
         */
        virtual void callObjectMember( SmartPtr<ISharedObject> object, const String &functionName,
                                       const Parameters &parameters, Parameters &results ) = 0;

        /**
         * @brief Creates an object instance defined in the script and stores the data in ScriptObject
         * for future use.
         * @param className The name of the script class.
         * @param object The shared object to associate with the script object.
         * @return A smart pointer to the created script class object.
         */
        virtual SmartPtr<IScriptClass> createObject( const String &className,
                                                     SmartPtr<ISharedObject> object ) = 0;

        /**
         * @brief Creates an instance of a script class.
         * @param className The name of the script class.
         * @return A pointer to the created instance.
         */
        virtual void *createInstance( const String &className ) = 0;

        /**
         * @brief Destroys an instance of a script class.
         * @param instance The pointer to the instance to destroy.
         */
        virtual void destroyInstance( void *instance ) = 0;

        /**
         * @brief Destroys the script object data associated with a shared object.
         * @param object The shared object whose script data should be destroyed.
         */
        virtual void destroyObject( SmartPtr<ISharedObject> object ) = 0;

        /**
         * @brief Reloads all scripts managed by the script manager.
         */
        virtual void reloadScripts() = 0;

        /**
         * @brief Checks if a script reload is pending.
         * @return True if a reload is pending, false otherwise.
         */
        virtual bool reloadPending() const = 0;

        /**
         * @brief Gets a string containing debug information about the script manager.
         * @return A string with debug information.
         */
        virtual String getDebugInfo() = 0;

        /**
         * @brief Checks if script objects are created with delayed creation.
         * @return True if delayed creation is enabled, false otherwise.
         */
        virtual bool getDelayedCreation() const = 0;

        /**
         * @brief Sets whether script objects are created with delayed creation.
         * @param delayedCreation True to enable delayed creation, false otherwise.
         */
        virtual void setDelayedCreation( bool delayedCreation ) = 0;

        /**
         * @brief Checks if debugging is enabled for the script manager.
         * @return True if debugging is enabled, false otherwise.
         */
        virtual bool isDebugEnabled() const = 0;

        /**
         * @brief Enables or disables debugging for the script manager.
         * @param enable True to enable debugging, false to disable.
         */
        virtual void setDebugEnabled( bool enable ) = 0;

        /**
         * @brief Removes a breakpoint from the script manager.
         * @param breakpoint The breakpoint to remove.
         */
        virtual void removeBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint ) = 0;

        /**
         * @brief Adds a breakpoint to the script manager.
         * @param breakpoint The breakpoint to add.
         */
        virtual void addBreakpoint( SmartPtr<IScriptBreakpoint> breakpoint ) = 0;

        /**
         * @brief Gets the list of breakpoints currently set in the script manager.
         * @return An array of smart pointers to the breakpoints.
         */
        virtual Array<SmartPtr<IScriptBreakpoint>> getBreakpoints() const = 0;

        /**
         * @brief Performs garbage collection for the scripting environment.
         */
        virtual void garbageCollect() = 0;

        /**
         * @brief Gets the internal scripting object pointer.
         * @param object Output parameter to receive the internal object pointer.
         */
        virtual void _getObject( void **object ) = 0;

        /**
         * @brief Registers a class with the scripting environment.
         * @param ptr A pointer to the class to register.
         */
        virtual void registerClass( void *ptr ) = 0;

        /**
         * @brief Gets the list of registered script class names.
         * @return An array of class names.
         */
        virtual Array<String> getClassNames() const = 0;

        /**
         * @brief Sets the list of script class names.
         * @param classNames The array of class names to set.
         */
        virtual void setClassNames( const Array<String> &classNames ) = 0;

        /**
         * @brief Gets the list of supported file extensions for scripts.
         * @return An array of supported file extensions.
         */
        virtual Array<String> getSupportedFileExtensions() const = 0;

        /** Loads an object via the script system.
         * @param scriptObject The object to be loaded.
         * @param forceQueue Forces the object to be queued for deferred loading.
         */
        virtual void loadObject( SmartPtr<ISharedObject> scriptObject, bool forceQueue = false ) = 0;

        /** Unloads an object via the script system.
         * @param scriptObject The object to be unloaded.
         * @param forceQueue Forces the object to be queued for deferred unloading.
         */
        virtual void unloadObject( SmartPtr<ISharedObject> scriptObject, bool forceQueue = false ) = 0;

        /// Load a script by durable catalog UUID. Legacy managers may not support assets.
        virtual bool loadScriptAsset( const String &uuid ) { return false; }

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
