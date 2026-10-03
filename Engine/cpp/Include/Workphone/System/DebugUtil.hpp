#ifndef DebugUtil_h__
#define DebugUtil_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <exception>

namespace workphone
{

    /**
     * @file DebugUtil.hpp
     * @brief Utilities for obtaining runtime debug information (stack traces).
     *
     * Provides helpers to capture and format the current call stack and to
     * format exception-related stack information for logging and diagnostics.
     *
     * Implementations are platform-specific (see corresponding .cpp). If stack
     * trace capture is not supported on a platform the functions should return
     * an empty string or a small diagnostic message describing the limitation.
     */

    /**
     * @class DebugUtil
     * @brief Static helper functions to collect and format stack traces.
     *
     * The methods in this class provide a convenient way to obtain a human
     * readable representation of the current call stack or the stack-related
     * information for an exception. All members are static and the class is
     * not intended to be instantiated.
     *
     * @note Thread-safety: callers may invoke these functions concurrently;
     *       however actual thread-safety depends on the platform-specific
     *       implementation. Implementations should avoid global mutable state
     *       where possible.
     *
     * @ingroup DebugUtilities
     */
    class WPCore_API DebugUtil
    {
    public:
        /**
         * @brief Capture and return the current call stack as a formatted string.
         *
         * The returned string is intended for logging or display in diagnostics.
         * The level of detail (function names, source file paths, line numbers)
         * depends on platform support and availability of symbol information.
         *
         * Typical uses:
         * - Logging a stack trace when an error condition is detected.
         * - Including a trace in a crash report or diagnostic dump.
         *
         * @return A formatted stack trace as an Workphone::String. If stack trace
         *         capture is unavailable, returns an empty string or a small
         *         diagnostic message.
         */
        static String getStackTrace();

        /**
         * @brief Produce a formatted message combining exception information and a stack trace.
         *
         * This function formats information from the provided exception (for
         * example the result of `e.what()`) together with a stack trace if
         * available. Depending on the implementation it may capture the stack
         * at the time the exception was handled (current stack) or include
         * additional exception-specific context if available.
         *
         * @param e A reference to the exception to describe. Note: the function
         *          does not take ownership of the exception object.
         * @return A formatted string containing the exception description and
         *         associated stack trace. If stack capture is not supported,
         *         returns at least the exception description.
         */
        static String getStackTraceForException( std::exception &e );
    };

}  // namespace workphone

#endif  // DebugUtil_h__
