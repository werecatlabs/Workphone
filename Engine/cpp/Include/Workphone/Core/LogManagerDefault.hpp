#ifndef __WP_Application_LogManager_h__
#define __WP_Application_LogManager_h__

#include <Workphone/Interface/System/ILogManager.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <fstream>
#include <sstream>
#include <map>

namespace workphone
{
    /**
     * @class LogManagerDefault
     * @brief Default implementation of the ILogManager interface.
     *
     * This class implements a simple file-backed logger with an optional
     * queuing mode. Messages can be logged using narrow or wide strings.
     * When queuing is enabled messages are pushed to an internal thread-safe
     * queue and can later be flushed to the output file.
     */
    class WPCore_API LogManagerDefault : public ILogManager
    {
    public:
        /**
         * @brief Construct a new LogManagerDefault instance.
         *
         * The constructor initializes internal flags to their default values.
         */
        LogManagerDefault();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of the output stream and any queued messages.
         */
        ~LogManagerDefault() override;

        /**
         * @brief Log a narrow (UTF-8 / platform-encoding) message.
         * @param message The message text to log.
         * @param type The message severity / type.
         */
        void logMessage( const String &message, Type type ) override;

        /**
         * @brief Log a wide (UTF-16/UTF-32) message.
         * @param message The wide message text to log.
         * @param type The message severity / type.
         */
        void logMessage( const StringW &message, Type type ) override;

        /**
         * @brief Query whether message queuing is enabled.
         * @return true if queuing is enabled, false if messages are written
         *         immediately to the output stream.
         */
        bool getEnableQueue() const override;

        /**
         * @brief Enable or disable message queuing.
         * @param queue True to enable queuing, false to disable.
         */
        void setEnableQueue( bool queue ) override;

        /**
         * @brief Open the log file for writing using a narrow path string.
         * @param filePath Path to the log file to open.
         */
        void open( const String &filePath ) override;

        /**
         * @brief Open the log file for writing using a wide path string.
         * @param filePath Wide-character path to the log file to open.
         */
        void open( const StringW &filePath ) override;

        /**
         * @brief Close the current log file and flush any remaining messages.
         */
        void close() override;

        /**
         * @brief Flush any buffered or queued messages to the output file.
         */
        void flush() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Output file stream used for writing log messages.
         */
        std::ofstream m_log;

        /**
         * @brief Whether queued logging is enabled. When true messages are
         *        pushed to m_messages and require an explicit flush to write
         *        them to disk.
         */
        atomic_bool m_queue = false;

        /**
         * @brief When true the log should be cleared on open/rotate.
         */
        atomic_bool m_clearLog = true;

        /**
         * @brief Thread-safe queue holding pending log messages when queuing
         *        is enabled.
         */
        ConcurrentQueue<String> m_messages;

        /**
         * @brief Singleton instance of the default log manager.
         */
        static SmartPtr<LogManagerDefault> m_instance;
    };
}  // namespace workphone

#endif  // __WP_LogManager_h__
