#include <EditorPCH.hpp>
#include "jobs/FileSelectedJob.hpp"
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include "ui/UIManager.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    FileSelectedJob::FileSelectedJob()
    {
        setPrimary( true );
    }

    FileSelectedJob::~FileSelectedJob() = default;

    void FileSelectedJob::execute()
    {
        auto path = getFilePath();

        const auto sceneExt = ".fbscene";
        const auto sceneXmlExt = ".fbscenexml";
        const auto sceneBinaryExt = ".fbscenebin";

        auto ext = StringUtil::make_lower( Path::getFileExtension( path ) );
        auto editorManager = EditorManager::getSingletonPtr();
        if( !editorManager )
        {
            return;
        }
        if( ext == ".prefab" || ext == ".fbprefab" )
        {
            String error;
            editorManager->openPrefab( path, error );
            if( auto ui = editorManager->getUI() )
            {
                ui->updatePrefabEditingUI( error );
            }
            return;
        }

        if( StringUtil::isEqual( ext, sceneExt ) || StringUtil::isEqual( ext, sceneXmlExt ) ||
            StringUtil::isEqual( ext, sceneBinaryExt ) )
        {
            if( editorManager->isEditingPrefab() )
            {
                if( auto ui = editorManager->getUI() )
                    ui->updatePrefabEditingUI(
                        "Save & Return or Discard & Return before opening a scene." );
                return;
            }
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto taskManager = applicationManager->getTaskManager();
            WP_ASSERT( taskManager );

            auto editorManager = EditorManager::getSingletonPtr();
            WP_ASSERT( editorManager );

            auto project = editorManager->getProject();
            WP_ASSERT( project );

            auto uiManager = editorManager->getUI();
            WP_ASSERT( uiManager );

            if( !applicationManager->isPlaying() )
            {
                project->setCurrentScenePath( path );

                auto gameManager = applicationManager->getGameManager();
                gameManager->loadScene( path );
            }
        }
    }

    String FileSelectedJob::getFilePath() const
    {
        return m_filePath;
    }

    void FileSelectedJob::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }
}  // namespace workphone::editor
