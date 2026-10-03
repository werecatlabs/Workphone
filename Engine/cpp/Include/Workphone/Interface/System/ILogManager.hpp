#ifndef ILogManager_h__
#define ILogManager_h__

#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for a log manager, used to manage logging messages and bind script functions.
     * Inherits from ISharedObject.
     */
    class WPCore_API ILogManager : public ISharedObject
    {
    public:
        /**
         * @brief Enum class representing the different types of log messages.
         */
        enum class Type
        {
            Error,
            Warning,
            Info,
            Exception,

            Count
        };

        /**
         * @brief Destroy the ILogManager object.
         */
        ~ILogManager() override;

        /**
         * @brief Log a message with the specified type.
         *
         * @param message The message to log.
         * @param type The type of the log message.
         */
        virtual void logMessage( const String &message, Type type ) = 0;

        /**
         * @brief Log a wide string message with the specified type.
         *
         * @param message The wide string message to log.
         * @param type The type of the log message.
         */
        virtual void logMessage( const StringW &message, Type type ) = 0;

        /**
         * @brief Get the current state of the message queue.
         *
         * @return bool True if the message queue is enabled, false otherwise.
         */
        virtual bool getEnableQueue() const = 0;

        /**
         * @brief Enable or disable the message queue.
         *
         * @param queue Set to true to enable the message queue, false to disable it.
         */
        virtual void setEnableQueue( bool queue ) = 0;

        /**
         * @brief Open a log file for writing.
         *
         * @param filePath The path of the file to open for logging.
         */
        virtual void open( const String &filePath ) = 0;

        /**
         * @brief Open a wide string log file for writing.
         *
         * @param filePath The wide string path of the file to open for logging.
         */
        virtual void open( const StringW &filePath ) = 0;

        /**
         * @brief Close the log file.
         */
        virtual void close() = 0;

        /**
         * @brief Flush the message queue, writing any pending messages to the log file.
         */
        virtual void flush() = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ILogManager_h__
