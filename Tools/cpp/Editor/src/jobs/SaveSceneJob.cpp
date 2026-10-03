#include <EditorPCH.hpp>
#include <jobs/SaveSceneJob.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, SaveSceneJob, Job );

    SaveSceneJob::SaveSceneJob() = default;

    SaveSceneJob::~SaveSceneJob() = default;

    void SaveSceneJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        if( applicationManager->getQuit() )
        {
            return;
        }

        if( applicationManager->isRunning() == false )
        {
            return;
        }

        auto fileSystem = applicationManager->getFileSystemPtr();
        WP_ASSERT( fileSystem );

        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto sceneManager = applicationManager->getGameManagerPtr();
        WP_ASSERT( sceneManager );

        if( auto scene = sceneManager->getCurrentScenePtr() )
        {
            auto filePath = scene->getFilePath();
            if( !StringUtil::isNullOrEmpty( filePath ) )
            {
                if( !getSaveAs() )
                {
                    if( fileSystem->isExistingFile( filePath ) )
                    {
                        scene->saveScene();
                    }
                    else
                    {
                        saveScene( filePath );
                    }
                }
                else
                {
                    saveScene( filePath );
                }

                filePath = getFilePath();
                if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    saveScene( filePath );
                }
            }
            else
            {
                auto filePath = applicationManager->getProjectPath();
                saveScene( filePath );
            }
        }
    }

    void SaveSceneJob::saveScene( const String &filePath )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        if( applicationManager->getQuit() )
        {
            return;
        }

        if( applicationManager->isRunning() == false )
        {
            return;
        }

        auto fileSystem = applicationManager->getFileSystemPtr();
        WP_ASSERT( fileSystem );

        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto sceneManager = applicationManager->getGameManagerPtr();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScenePtr();
        WP_ASSERT( scene );

        if( auto fileDialog = fileSystem->openFileDialog() )
        {
            auto projectPath = filePath;

            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = applicationManager->getProjectPath();
            }

            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            if( !fileSystem->isExistingFolder( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            fileDialog->setDialogMode( INativeFileDialog::DialogMode::Save );
            fileDialog->setFileExtension( getSceneFileDialogExtensions() );
            fileDialog->setFilePath( projectPath );

            auto result = fileDialog->openDialog();
            if( result == INativeFileDialog::Result::Dialog_Okay )
            {
                auto sceneFilePath = fileDialog->getFilePath();
                auto ext = Path::getFileExtension( sceneFilePath );
                if( StringUtil::isNullOrEmpty( ext ) || !isSupportedSceneFileExtension( ext ) )
                {
                    sceneFilePath += ApplicationUtil::builtinSceneExt;
                }

                scene->saveScene( sceneFilePath );
            }
        }
    }

    String SaveSceneJob::getFilePath() const
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        return m_filePath;
    }

    void SaveSceneJob::setFilePath( const String &filePath )
    {
        SpinRWMutex::ScopedLock lock( m_mutex, true );
        m_filePath = filePath;
    }

    bool SaveSceneJob::getSaveAs() const
    {
        SpinRWMutex::ScopedLock lock( m_mutex, false );
        return m_saveAs;
    }

    void SaveSceneJob::setSaveAs( bool saveAs )
    {
        SpinRWMutex::ScopedLock lock( m_mutex, true );
        m_saveAs = saveAs;
    }

    bool SaveSceneJob::isSupportedSceneFileExtension( const String &ext )
    {
        return ext == ApplicationUtil::builtinSceneExt || ext == ApplicationUtil::builtinXmlSceneExt ||
               ext == ApplicationUtil::builtinBinarySceneExt ||
               ext == ApplicationUtil::builtinUsdSceneExt;
    }

    String SaveSceneJob::getSceneFileDialogExtensions()
    {
        return ApplicationUtil::builtinSceneExt + ";" + ApplicationUtil::builtinXmlSceneExt + ";" +
               ApplicationUtil::builtinBinarySceneExt;
    }

}  // namespace workphone::editor
