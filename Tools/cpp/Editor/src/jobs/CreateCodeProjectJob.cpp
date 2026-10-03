#include <EditorPCH.hpp>
#include <jobs/CreateCodeProjectJob.hpp>
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    CreateCodeProjectJob::CreateCodeProjectJob() = default;

    CreateCodeProjectJob::~CreateCodeProjectJob() = default;

    void CreateCodeProjectJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto editorManager = EditorManager::getSingletonPtr();
            auto projectManager = editorManager->getProjectManager();

            auto workingDirectory = Path::getWorkingDirectory();
            auto mediaPath = String();

#if WP_FINAL
            mediaPath += applicationManager->getMediaPath() + "/Engine";
#else
#    if defined WP_PLATFORM_WIN32
            mediaPath = String( "../../../../../../" );
#    elif defined WP_PLATFORM_APPLE
            mediaPath = String( "../../../" );
#    else
            mediaPath = String( "../../../Engine/cpp/Include" );
#    endif
#endif

            auto absoluteMediaPath = Path::getAbsolutePath( workingDirectory, mediaPath );

#if WP_FINAL
            auto proprietaryPath = Path::getAbsolutePath( workingDirectory, "../" );
#else
            auto proprietaryPath =
                Path::getAbsolutePath( applicationManager->getMediaPath(), "../../../" );
#endif

            proprietaryPath = StringUtil::cleanupPath( proprietaryPath );

            projectManager->clearLibraries();

            projectManager->setEnginePath( absoluteMediaPath );
            projectManager->addIncludeFolder( proprietaryPath +
                                              "/lioncat_proprietary/Libraries/cpp/Include" );
            projectManager->addLibraryFolder( proprietaryPath + "/lioncat_proprietary/libs" );
            projectManager->addLibrary( "FBApplication" );
            projectManager->addLibrary( "FBVehicle" );

            projectManager->generateProject();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone::editor
