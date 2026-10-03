#ifndef FileListener_h__
#define FileListener_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

#if WP_BUILD_FILEWATCHER
#    include <FileWatcher/FileWatcher.h>
#endif

/**
 * @file FileListener.hpp
 * @brief Listener that receives notifications from the file-watcher library.
 */

namespace workphone
{
    /**
     * @class FileListener
     * @brief Adapts the third-party file watcher callbacks to the Workphone codebase.
     *
     * This class implements FW::FileWatchListener from the FileWatcher library and
     * receives notifications when files in watched directories are created, modified,
     * deleted or moved. The implementation should translate the raw watcher events
     * into Workphone-specific actions (logging, reloading resources, etc.).
     *
     * Note: handleFileAction is invoked by the FileWatcher library and callers should
     * ensure thread-safety if the implementation touches shared Workphone state.
     */
    class WPCore_API FileListener
#if WP_BUILD_FILEWATCHER
        : public FW::FileWatchListener
#endif
    {
    public:
        /**
         * @brief Construct a new FileListener.
         *
         * The constructor performs minimal initialization. Actual watcher registration
         * is expected to be done by the code that owns this listener.
         */
        FileListener();

        /**
         * @brief Destroy the FileListener.
         *
         * Cleans up any resources owned by the listener. Declared virtual via override
         * to match the FW::FileWatchListener interface.
         */
        ~FileListener()
#if WP_BUILD_FILEWATCHER
            override
#endif
            ;

#if WP_BUILD_FILEWATCHER
        /**
         * @brief Handle a file-system action reported by the FileWatcher library.
         *
         * This method is called when a watched file or directory changes. The
         * implementation should inspect `action` to determine the type of change
         * (create, modify, delete, rename) and respond appropriately.
         *
         * @param watchid Identifier for the watch that triggered the event.
         * @param dirStr   Directory path where the event occurred.
         * @param filenameStr Filename (or relative path) that changed.
         * @param action   The FW::Action enum value describing the change.
         */
        void handleFileAction( FW::WatchID watchid, const FW::String &dirStr,
                               const FW::String &filenameStr, FW::Action action );
#endif
    };
}  // namespace workphone

#endif  // FileListener_h__
