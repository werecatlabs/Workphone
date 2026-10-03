#include <EditorPCH.hpp>
#include <jobs/CreatePluginCodeJob.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    CreatePluginCodeJob::CreatePluginCodeJob() = default;

    CreatePluginCodeJob::~CreatePluginCodeJob() = default;

    void CreatePluginCodeJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto editorManager = EditorManager::getSingletonPtr();
            auto projectManager = editorManager->getProjectManager();

            auto path = applicationManager->getProjectPath();

            auto pluginPath = path + "/Plugin";
            if( !Path::isExistingFolder( pluginPath ) )
            {
                Path::createDirectories( pluginPath );
            }

            auto project = editorManager->getProject();
            auto pluginHeaderStr = project->getPluginHeader();
            auto pluginSourceStr = project->getPluginSource();

            Path::writeAllText( pluginPath + "/Plugin.hpp", pluginHeaderStr );
            Path::writeAllText( pluginPath + "/Plugin.cpp", pluginSourceStr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone::editor
