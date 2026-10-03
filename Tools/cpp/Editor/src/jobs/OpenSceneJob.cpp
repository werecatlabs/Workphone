#include <EditorPCH.hpp>
#include "jobs/OpenSceneJob.hpp"
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
#include "ui/ProjectWindow.hpp"

#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    OpenSceneJob::OpenSceneJob() = default;

    OpenSceneJob::~OpenSceneJob() = default;

    void OpenSceneJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabase();

        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto uiManager = editorManager->getUI();
        WP_ASSERT( uiManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        if( auto fileDialog = fileSystem->openFileDialog() )
        {
            auto projectPath = applicationManager->getProjectPath();
            if( !fileSystem->isExistingFolder( projectPath ) )
            {
                projectPath = "";
            }

            fileDialog->setDialogMode( INativeFileDialog::DialogMode::Open );
            fileDialog->setFileExtension( ApplicationUtil::builtinSceneExt + ";" +
                                          ApplicationUtil::builtinXmlSceneExt + ";" +
                                          ApplicationUtil::builtinBinarySceneExt );
            fileDialog->setFilePath( projectPath );

            //auto result = fileDialog->openDialog();
            //if( result == INativeFileDialog::Result::Dialog_Okay )
            {
                auto project = editorManager->getProject();
                WP_ASSERT( project );

                auto filePath = getFilePath();
                if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    auto fileSystem = applicationManager->getFileSystem();
                    WP_ASSERT( fileSystem );

                    auto path = Path::getFilePath( filePath );
                    path = StringUtil::cleanupPath( path );

                    auto projectPath = applicationManager->getProjectPath();
                    if( StringUtil::isNullOrEmpty( projectPath ) )
                    {
                        projectPath = Path::getWorkingDirectory();
                    }

                    auto scenePath = Path::getRelativePath( projectPath, filePath );
                    scenePath = StringUtil::cleanupPath( scenePath );

                    project->setCurrentScenePath( filePath );

                    auto sceneManager = applicationManager->getGameManager();
                    if( auto scene = sceneManager->getCurrentScene() )
                    {
                        scene->clear();
                        scene->loadScene( scenePath );
                    }

                    resourceDatabase->refresh();
                    uiManager->rebuildSceneTree();

                    if( auto projectWindow = uiManager->getProjectWindow() )
                    {
                        projectWindow->buildTree();
                    }
                }
            }
        }
    }

    String OpenSceneJob::getFilePath() const
    {
        return m_filePath;
    }

    void OpenSceneJob::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }
}  // namespace workphone::editor
