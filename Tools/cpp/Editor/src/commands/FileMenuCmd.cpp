#include <EditorPCH.hpp>
#include <commands/FileMenuCmd.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <jobs/OpenSceneJob.hpp>
#include <jobs/CreateCodeProjectJob.hpp>
#include "jobs/CompileProjectJob.hpp"
#include <ui/UIManager.hpp>
#include <ui/ProjectWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, FileMenuCmd, Command );

    FileMenuCmd::FileMenuCmd() = default;

    FileMenuCmd::~FileMenuCmd() = default;

    void FileMenuCmd::undo()
    {
    }

    void FileMenuCmd::redo()
    {
    }

    void FileMenuCmd::execute()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto processManager = applicationManager->getProcessManager();

        auto taskManager = applicationManager->getTaskManager();
        auto application = applicationManager->getApplication();

        auto sceneManager = applicationManager->getGameManagerPtr();
        auto scene = sceneManager->getCurrentScenePtr();

        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto uiManager = editorManager->getUI();
        WP_ASSERT( uiManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto jobQueue = applicationManager->getJobQueue();
        auto selectionManager = applicationManager->getSelectionManager();

        switch( auto eventId = getItemId() )
        {
        case UIManager::WidgetId::None:
        {
        }
        break;
        case UIManager::WidgetId::About:
        {
        }
        break;
        case UIManager::WidgetId::NewProjectId:
        {
            auto project = editorManager->getProject();

            if( auto fileDialog = fileSystem->openFileDialog() )
            {
                auto projectPath = Path::getWorkingDirectory();
                if( !fileSystem->isExistingFolder( projectPath ) )
                {
                    projectPath = "";
                }

                fileDialog->setDialogMode( INativeFileDialog::DialogMode::Select );
                fileDialog->setFileExtension( ".fbproject" );
                fileDialog->setFilePath( projectPath );

                auto result = fileDialog->openDialog();
                if( result == INativeFileDialog::Result::Dialog_Okay )
                {
                    auto filePath = fileDialog->getFilePath();
                    if( !StringUtil::isNullOrEmpty( filePath ) )
                    {
                        project->create( filePath );

                        scene->clear();

                        const auto projectFileName = String( "project.fbproject" );
                        auto projectFilePath = filePath + "/" + projectFileName;
                        editorManager->loadProject( projectFilePath );
                    }

                    ApplicationUtil::createDefaultScene();
                }
            }
        }
        break;
        case UIManager::WidgetId::OpenProjectId:
        {
            if( auto fileDialog = fileSystem->openFileDialog() )
            {
                auto projectPath = applicationManager->getProjectPath();
                if( !fileSystem->isExistingFolder( projectPath ) )
                {
                    projectPath = "";
                }

                fileDialog->setDialogMode( INativeFileDialog::DialogMode::Open );
                fileDialog->setFileExtension( ".fbproject" );
                fileDialog->setFilePath( projectPath );

                auto result = fileDialog->openDialog();
                if( result == INativeFileDialog::Result::Dialog_Okay )
                {
                    auto filePath = fileDialog->getFilePath();
                    if( !StringUtil::isNullOrEmpty( filePath ) )
                    {
                        editorManager->loadProject( filePath );
                    }
                }
            }
        }
        break;
        case UIManager::WidgetId::NewSceneId:
        {
            auto renderLock = taskManager->lockTask( TaskId::Render );
            auto physicsLock = taskManager->lockTask( TaskId::Physics );
            auto applicationLock = taskManager->lockTask( TaskId::Application );

            auto editorManager = EditorManager::getSingletonPtr();
            WP_ASSERT( editorManager );

            auto uiManager = editorManager->getUI();
            WP_ASSERT( uiManager );

            scene->setLabel( "Untitled" );
            scene->setFilePath( String() );
            scene->clear( true );

            application->createDefaultSky();
            application->createDirectionalLight();

            uiManager->rebuildSceneTree();
        }
        break;
        case UIManager::WidgetId::OpenSceneId:
        {
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

                auto result = fileDialog->openDialog();
                if( result == INativeFileDialog::Result::Dialog_Okay )
                {
                    auto filePath = fileDialog->getFilePath();
                    if( !StringUtil::isNullOrEmpty( filePath ) )
                    {
                        sceneManager->loadScene( filePath );
                    }
                }
            }
        }
        break;
        case UIManager::WidgetId::BatchAllBtnId:
        {
        }
        break;
        case UIManager::WidgetId::AppPropertiesId:
        {
        }
        break;
        case UIManager::WidgetId::CreateOverlayTestId:
        {
            //ApplicationUtil::createOverlayPanelTest();

            auto editorManager = EditorManager::getSingletonPtr();
            auto ui = editorManager->getUI();
            ui->rebuildSceneTree();
        }
        break;
        case UIManager::WidgetId::CreateOverlayTextTestId:
        {
            //ApplicationUtil::createOverlayTextTest();

            auto editorManager = EditorManager::getSingletonPtr();
            auto ui = editorManager->getUI();
            ui->rebuildSceneTree();
        }
        break;
        case UIManager::WidgetId::CreateOverlayButtonTestId:
        {
            //ApplicationUtil::createOverlayButtonTest();

            auto editorManager = EditorManager::getSingletonPtr();
            auto ui = editorManager->getUI();
            ui->rebuildSceneTree();
        }
        break;
        case UIManager::WidgetId::CreateProceduralTestId:
        {
            //auto applicationManager = core::IApplicationManager::instance();
            //WP_ASSERT( applicationManager );

            //auto actor = ApplicationUtil::createProceduralTest();
            //WP_ASSERT( actor );

            //if( applicationManager->isPlaying() )
            //{
            //    actor->setState( scene::IGameActor::State::Play );
            //}
            //else
            //{
            //    actor->setState( scene::IGameActor::State::Edit );
            //}

            auto editorManager = EditorManager::getSingletonPtr();
            auto ui = editorManager->getUI();
            ui->rebuildSceneTree();
        }
        break;
        case UIManager::WidgetId::GenerateCMakeProjectId:
        {
            auto job = workphone::make_ptr<CreateCodeProjectJob>();
            jobQueue->addJob( job );
        }
        break;
        case UIManager::WidgetId::CompileId:
        {
            auto job = workphone::make_ptr<CompileProjectJob>();
            jobQueue->addJob( job );
        }
        break;
        case UIManager::WidgetId::CreatePackageId:
            break;
        case UIManager::WidgetId::ProjectSettingsId:
            break;
        case UIManager::WidgetId::LoadProceduralSceneId:
        {
            if( auto fileDialog = fileSystem->openFileDialog() )
            {
                auto projectPath = applicationManager->getProjectPath();
                if( !fileSystem->isExistingFolder( projectPath ) )
                {
                    projectPath = "";
                }

                fileDialog->setDialogMode( INativeFileDialog::DialogMode::Open );
                fileDialog->setFileExtension( ".osm" );
                fileDialog->setFilePath( projectPath );

                auto result = fileDialog->openDialog();
                if( result == INativeFileDialog::Result::Dialog_Okay )
                {
                }
            }
        }
        break;
        case UIManager::WidgetId::SaveProceduralSceneId:
        {
            auto editorManager = EditorManager::getSingletonPtr();
            WP_ASSERT( editorManager );

            auto uiManager = editorManager->getUI();
            WP_ASSERT( uiManager );

            //auto fileBrowser = uiManager->getFileBrowser();
            //WP_ASSERT( fileBrowser );

            //fileBrowser->setElementId( (s32)UIManager::WidgetId::SaveProceduralSceneDialog );
            //fileBrowser->setFileExtension( ".osm" );
            //fileBrowser->show();
        }
        break;
        case UIManager::WidgetId::LuaEditConfigDialogId:
        {
        }
        break;
        case UIManager::WidgetId::GotoId:
        {
        }
        break;
        case UIManager::WidgetId::ShowAllOverlaysId:
            break;
        case UIManager::WidgetId::HideAllOverlaysId:
            break;
        case UIManager::WidgetId::CreateRigidBodies:
        {
            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<scene::IGameActor>() )
                {
                    auto actor = workphone::static_pointer_cast<scene::IGameActor>( selected );

                    auto job = workphone::make_ptr<CreateRigidBodies>();
                    job->setActor( actor );
                    job->setMakeStatic( true );
                    job->setConvex( false );
                    job->setCascade( true );

                    jobQueue->addJob( job );
                }
            }
        }
        break;
        case UIManager::WidgetId::CreateRigidStaticMeshId:
        {
            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<scene::IGameActor>() )
                {
                    auto actor = workphone::static_pointer_cast<scene::IGameActor>( selected );

                    auto job = workphone::make_ptr<CreateRigidBodies>();
                    job->setActor( actor );
                    job->setMakeStatic( true );
                    job->setConvex( false );
                    job->setCascade( true );

                    jobQueue->addJob( job );
                }
            }
        }
        break;
        case UIManager::WidgetId::CreateRigidDynamicMeshId:
        {
            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<scene::IGameActor>() )
                {
                    auto actor = workphone::static_pointer_cast<scene::IGameActor>( selected );

                    auto job = workphone::make_ptr<CreateRigidBodies>();
                    job->setActor( actor );
                    job->setMakeStatic( false );
                    job->setConvex( true );
                    job->setCascade( true );

                    jobQueue->addJob( job );
                }
            }
        }
        break;
        case UIManager::WidgetId::CreateConstraintId:
        {
        }
        break;
        case UIManager::WidgetId::CreateDefaultCarId:
            break;
        case UIManager::WidgetId::CreateDefaultTruckId:
            break;
        case UIManager::WidgetId::ConvertCSharpId:
            break;
        case UIManager::WidgetId::PhysicsEnableId:
            break;
        case UIManager::WidgetId::ID_CustomizeToolbar:
            break;
        case UIManager::WidgetId::RunId:
            break;
        case UIManager::WidgetId::StopId:
            break;
        case UIManager::WidgetId::FileBrowserId:
            break;
        case UIManager::WidgetId::ID_SampleItem:
            break;
        case UIManager::WidgetId::ConvertFbx:
        {
            auto processName = StringW( L"FBX2glTF-windows-x64.exe" );

            Array<StringW> args;
            args.resize( 5 );

            StringW meshPath;
            StringW outputPath;

            auto selection = selectionManager->getSelection();
            for( auto selected : selection )
            {
                if( selected->isDerived<FileSelection>() )
                {
                    auto fileSelection = workphone::static_pointer_cast<FileSelection>( selected );
                    meshPath = StringUtil::toStringW( fileSelection->getFilePath() );
                }
                else if( selected->isDerived<IResource>() )
                {
                    auto resource = workphone::static_pointer_cast<IResource>( selected );
                    meshPath = StringUtil::toStringW( resource->getFilePath() );
                }
            }

            auto projectPath = StringUtil::toStringW( applicationManager->getProjectPath() );

            args[0] = L"-i";
            args[1] = L"\"" +
                      ( PathW::isPathAbsolute( meshPath ) ? meshPath : projectPath + L"/" + meshPath ) +
                      L"\"";
            args[2] = L"-o";
            args[3] =
                L"\"" +
                ( PathW::isPathAbsolute( meshPath )
                      ? PathW::getFilePathWithoutExtension( meshPath ) + L".glb"
                      : projectPath + L"/" + PathW::getFilePathWithoutExtension( meshPath ) + L".glb" ) +
                L"\"";
            args[4] = L"--pbr-metallic-roughness";

            //auto searchPaths = Array<StringW>();
            //searchPaths.reserve( 4 );

            //auto meshFolderPath = PathW::getFilePath( meshPath );
            //searchPaths.push_back( meshFolderPath );

            //for( auto searchPath : searchPaths )
            //{
            //    args.push_back( L"--search-path" );
            //    args.push_back( projectPath + L"/" + searchPath );
            //    args.push_back( L"--recursive" );
            //}

            processManager->createProcess( processName, args );
        }
        break;
        case UIManager::WidgetId::ConvertAllFbx:
        {
            auto projectPath = StringUtil::toStringW( applicationManager->getProjectPath() );

            auto processName = StringW( L"FBX2glTF-windows-x64.exe" );

            Array<StringW> args;
            args.resize( 5 );

            StringW meshPath;
            StringW outputPath;

            auto files = fileSystem->getFilesWithExtension( ".fbx" );
            auto fileUppercase = fileSystem->getFilesWithExtension( ".FBX" );
            files.insert( files.end(), fileUppercase.begin(), fileUppercase.end() );

            for( auto file : files )
            {
                meshPath = StringUtil::toStringW( file.absolutePath.c_str() );

                args[0] = L"-i";
                args[1] =
                    L"\"" +
                    ( PathW::isPathAbsolute( meshPath ) ? meshPath : projectPath + L"/" + meshPath ) +
                    L"\"";
                args[2] = L"-o";
                args[3] = L"\"" +
                          ( PathW::isPathAbsolute( meshPath )
                                ? PathW::getFilePathWithoutExtension( meshPath ) + L".glb"
                                : projectPath + L"/" + PathW::getFilePathWithoutExtension( meshPath ) +
                                      L".glb" ) +
                          L"\"";
                args[4] = L"--pbr-metallic-roughness";

                //auto searchPaths = Array<StringW>();
                //searchPaths.reserve( 4 );

                //auto meshFolderPath = PathW::getFilePath( meshPath );
                //searchPaths.push_back( meshFolderPath );

                //args.push_back( L"--search-path" );

                //for( auto searchPath : searchPaths )
                //{
                //    args.push_back( projectPath + L"/" + searchPath );
                //}

                //args.push_back( L"--recursive" );

                processManager->createProcess( processName, args );
            }
        }
        break;
        default:
        {
        }
        break;
        }
    }

    UIManager::WidgetId FileMenuCmd::getItemId() const
    {
        return m_itemId;
    }

    void FileMenuCmd::setItemId( UIManager::WidgetId itemId )
    {
        m_itemId = itemId;
    }
}  // namespace workphone::editor
