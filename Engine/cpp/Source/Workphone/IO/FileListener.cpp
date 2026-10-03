#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/FileListener.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>

namespace workphone
{
    FileListener::FileListener() = default;

    FileListener::~FileListener() = default;

#if WP_BUILD_FILEWATCHER
    void FileListener::handleFileAction( FW::WatchID watchid, const FW::String &dirStr,
                                         const FW::String &filenameStr, FW::Action action )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( applicationManager->getQuit() )
        {
            return;
        }

        if( !applicationManager->isRunning() )
        {
            // If the application is not running, we don't need to handle file actions.
            return;
        }

        auto fileSystem = applicationManager->getFileSystemPtr();

        auto parameters = Parameters{ Parameter( (u32)watchid ), Parameter( dirStr.c_str() ),
                                      Parameter( filenameStr.c_str() ), Parameter( (u32)action ) };

        applicationManager->triggerEvent( EventType::IO, IEvent::fileAction, parameters, fileSystem,
                                          fileSystem, nullptr, false, Thread::Application_Flag );
    }
#endif

}  // namespace workphone
