#ifndef IProcessManager_h__
#define IProcessManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @class IProcessManager
     * @brief Interface for managing processes.
     */
    class WPCore_API IProcessManager : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IProcessManager() override;

        /**
         * @brief Creates a new process with the specified application name.
         * @param applicationName The name of the application to run.
         */
        virtual void createProcess( const String &applicationName ) = 0;

        /**
         * @brief Creates a new process with the specified application name.
         * @param applicationName The name of the application to run.
         */
        virtual void createProcess( const StringW &applicationName ) = 0;

        /**
         * @brief Creates a new process with the specified application name.
         * @param applicationName The name of the application to run.
         * @param args The arguments to pass to the application.
         */
        virtual void createProcess( const StringW &applicationName, const Array<StringW> &args ) = 0;

        /**
         * @brief Executes the specified application using the default shell.
         * @param applicationName The name of the application to execute.
         */
        virtual void shellExecute( const String &applicationName ) = 0;

        /**
         * @brief Executes the specified application using the default shell.
         * @param applicationName The name of the application to execute.
         */
        virtual void shellExecute( const StringW &applicationName ) = 0;

        /**
         * @brief Executes the specified application with the specified arguments using the default
         * shell.
         * @param applicationName The name of the application to execute.
         * @param args The arguments to pass to the application.
         */
        virtual void shellExecute( const StringW &applicationName, const Array<StringW> &args ) = 0;

        /**
         * @brief Executes the specified application with the specified arguments using the default
         * shell.
         * @param applicationName The name of the application to execute.
         * @param directory The working directory for the application.
         * @param args The arguments to pass to the application.
         */
        virtual void shellExecute( const StringW &applicationName, const StringW &directory,
                                   const Array<StringW> &args ) = 0;

        /**
         * @brief Checks if the specified process is currently running.
         * @param processName The name of the process to check.
         * @return true if the process is running, false otherwise.
         */
        virtual bool isProcessRunning( const String &processName ) = 0;

        /**
         * @brief Terminates the specified process.
         * @param processName The name of the process to terminate.
         * @return true if the process was terminated successfully, false otherwise.
         */
        virtual bool terminateProcess( const String &processName ) = 0;

        /**
         * @brief Checks if the specified process is currently running.
         * @param processName The name of the process to check.
         * @return true if the process is running, false otherwise.
         */
        virtual bool isProcessRunning( const StringW &processName ) = 0;

        /**
         * @brief Terminates the specified process.
         * @param processName The name of the process to terminate.
         * @return true if the process was terminated successfully, false otherwise.
         */
        virtual bool terminateProcess( const StringW &processName ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IProcessManager_h__
