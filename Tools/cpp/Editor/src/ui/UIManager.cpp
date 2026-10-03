#include <EditorPCH.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <commands/DuplicateSelectionCmd.hpp>
#include <commands/FileMenuCmd.hpp>
#include <commands/PasteSelectionCmd.hpp>
#include <commands/RemoveSelectionCmd.hpp>
#include <commands/ToggleEditorCamera.hpp>
#include <jobs/AssetDatabaseBuildJob.hpp>
#include <jobs/AssetImportJob.hpp>
#include <jobs/CopyEngineFilesJob.hpp>
#include <jobs/CreatePluginCodeJob.hpp>
#include <jobs/GenerateSkybox.hpp>
#include <jobs/OptimiseDatabasesJob.hpp>
#include <jobs/ProjectCleanJob.hpp>
#include <jobs/SetupMaterialJob.hpp>
#include <jobs/ReloadScriptsJob.hpp>
#include <jobs/PlaymodeJob.hpp>
#include <jobs/LeavePlaymodeJob.hpp>
#include <jobs/ImportUnityYaml.hpp>
#include <jobs/SaveSceneJob.hpp>
#include <jobs/ShowDebugJob.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/AnimationWindow.hpp>
#include <ui/AnimationGraphWindow.hpp>
#include <ui/AssetDatabaseWindow.hpp>
#include <ui/CutsceneWindow.hpp>
#include <ui/CollisionMaskDialog.hpp>
#include <ui/CollisionMaskManager.hpp>
#include <ui/FileWindow.hpp>
#include <ui/MaterialWindow.hpp>
#include <ui/ObjectWindow.hpp>
#include <ui/ObjectBrowserDialog.hpp>
#include <ui/ProjectWindow.hpp>
#include <ui/ProfilerWindow.hpp>
#include <ui/PropertiesWindow.hpp>
#include <ui/ResourceDatabaseDialog.hpp>
#include <ui/InputManagerWindow.hpp>
#include <ui/LayerDialog.hpp>
#include <ui/LayerManager.hpp>
#include <ui/SceneWindow.hpp>
#include <ui/ScriptWindow.hpp>
#include <ui/TagDialog.hpp>
#include <ui/TagManager.hpp>
#include <ui/TerrainWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <WPImGui/WPImGui.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, UIManager, ISharedObject );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, UIManager::UpdateSelectionJob, Job );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, UIManager::MenuBarListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, UIManager::ToolbarListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, UIManager::EventListener, IEventListener );

    Array<SmartPtr<Properties>> UIManager::s_clipboardActorData;

    void UIManager::clearClipboardActorData()
    {
        s_clipboardActorData.clear();
    }

    void UIManager::addClipboardActorData( SmartPtr<Properties> data )
    {
        if( data )
        {
            s_clipboardActorData.push_back( data );
        }
    }

    const Array<SmartPtr<Properties>> &UIManager::getClipboardActorData()
    {
        return s_clipboardActorData;
    }

    UIManager::UIManager()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
    }

    UIManager::~UIManager()
    {
        unload( nullptr );
    }

    void UIManager::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto scriptManager = applicationManager->getScriptManager();

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto ui = applicationManager->getUI();
        if( !ui )
        {
            WP_LOG_ERROR( "UIManager::load: UI is null" );
            return;
        }

        auto editorManager = EditorManager::getSingletonPtr();
        auto project = editorManager->getProject();

        m_frameStatistics = factoryManager->make_ptr<FrameStatistics>();
        //m_frameStatistics->load( nullptr );
        m_frameStatistics->setVisible( false );

        if( graphicsSystem )
        {
            graphicsSystem->loadObject( m_frameStatistics, true );
        }

        auto menuBar = ui->addElementByType<ui::IUIMenubar>();
        if( menuBar )
        {
            auto fileMenu = ui->addElementByType<ui::IUIMenu>();
            fileMenu->setLabel( "File" );

            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::NewProjectId ),
                               ICON_FA_FILE " New Project", "Create a project",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::OpenProjectId ),
                               ICON_FA_FOLDER_OPEN " Open Project", "Open a project",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::SaveProjectId ),
                               ICON_FA_FILE " Save Project", "Save a project",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( fileMenu );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::NewSceneId ),
                               ICON_FA_FILE " New Scene", "New Scene", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::OpenSceneId ),
                               ICON_FA_FOLDER_OPEN " Open Scene", "Open Scene",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::SaveId ),
                               ICON_FA_FILM " Save Scene", "Save Scene", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::SaveSceneAsId ),
                               ICON_FA_FILE " Save Scene As", "Save Scene As",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::SaveAllId ),
                               ICON_FA_FILE " Save All", "Save All", ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( fileMenu );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::GenerateCMakeProjectId ),
                               ICON_FA_FILM " Generate CMake Project", "Generate CMake Project",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::CompileId ),
                               ICON_FA_FILE " Compile", "Compile", ui::IUIMenuItem::Type::Normal );

            Util::addMenuSeparator( fileMenu );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::ProjectSettingsId ),
                               ICON_FA_FILE " Settings", "Settings", ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( fileMenu );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::CreatePackageId ),
                               ICON_FA_FILE " Create Package", "Create Package",
                               ui::IUIMenuItem::Type::Normal );
            //Util::addMenuSeparator( fileMenu );

            //Util::addMenuItem(
            //    fileMenu, static_cast<s32>( UIManager::WidgetId::CreateUnityBindings ),
            //    "Create Unity Bindings", "Create Unity Bindings", ui::IUIMenuItem::Type::Normal );
            //Util::addMenuSeparator( fileMenu );

            //auto recentFilesMenu = ui->addElementByType<ui::IUIMenu>();
            //recentFilesMenu->setLabel( "Recent Files" );
            //Util::addMenuItem( recentFilesMenu,
            //                              static_cast<s32>( UIManager::WidgetId::LoadProceduralSceneId ),
            //                              "File", "File", ui::IUIMenuItem::Type::Normal );
            //fileMenu->addMenuItem( recentFilesMenu );

            //auto recentProjectsMenu = ui->addElementByType<ui::IUIMenu>();
            //recentProjectsMenu->setLabel( "Recent Projects" );
            //Util::addMenuItem( recentProjectsMenu,
            //                              static_cast<s32>( UIManager::WidgetId::LoadProceduralSceneId ),
            //                              "File", "File", ui::IUIMenuItem::Type::Normal );
            //fileMenu->addMenuItem( recentProjectsMenu );

            Util::addMenuSeparator( fileMenu );
            Util::addMenuItem( fileMenu, static_cast<s32>( WidgetId::Exit ), ICON_FA_POWER_OFF " Exit",
                               "Quit this program", ui::IUIMenuItem::Type::Normal );

            menuBar->addMenu( fileMenu );

            auto editMenu = ui->addElementByType<ui::IUIMenu>();
            editMenu->setLabel( "Edit" );
            Util::addMenuItem( editMenu, static_cast<s32>( WidgetId::UndoId ),
                               ICON_FA_ROTATE_LEFT " Undo", "Undo command",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( editMenu, static_cast<s32>( WidgetId::RedoId ), ICON_FA_REPEAT " Redo",
                               "Redo command", ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( editMenu );
            Util::addMenuItem( editMenu, static_cast<s32>( WidgetId::CutId ), ICON_FA_SCISSORS " Cut",
                               "Cut selected objects", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( editMenu, static_cast<s32>( WidgetId::CopyId ), ICON_FA_COPY " Copy",
                               "Copy selected objects", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( editMenu, static_cast<s32>( WidgetId::PasteId ), ICON_FA_PASTE " Paste",
                               "Paste clipboard objects", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( editMenu, static_cast<s32>( WidgetId::DuplicateId ),
                               ICON_FA_CLONE " Duplicate", "Duplicate selected objects",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( editMenu );
            Util::addMenuItem( editMenu, static_cast<s32>( WidgetId::DeleteId ), ICON_FA_TRASH " Delete",
                               "Delete selected objects", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( editMenu, static_cast<s32>( WidgetId::SelectAllId ),
                               ICON_FA_SQUARE_CHECK " Select All", "Select all objects",
                               ui::IUIMenuItem::Type::Normal );
            menuBar->addMenu( editMenu );

            auto assetsMenu = ui->addElementByType<ui::IUIMenu>();
            assetsMenu->setLabel( "Assets" );

            Util::addMenuItem( assetsMenu, static_cast<s32>( WidgetId::AssetImportId ),
                               ICON_FA_DOWNLOAD " Import", "Import", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( assetsMenu, static_cast<s32>( WidgetId::AssetReimportId ),
                               ICON_FA_FILE " Reimport", "Reimport", ui::IUIMenuItem::Type::Normal );

            Util::addMenuSeparator( assetsMenu );

            Util::addMenuItem( assetsMenu, static_cast<s32>( WidgetId::AssetDatabaseBuildId ),
                               ICON_FA_FILE " Build Database", "Build Database",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( assetsMenu, static_cast<s32>( WidgetId::AssetDatabaseImportCacheId ),
                               ICON_FA_DOWNLOAD " Import Cache", "Import Cache",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( assetsMenu, static_cast<s32>( WidgetId::AssetDatabaseDeleteCacheId ),
                               ICON_FA_FILM " Delete Cache", "Delete Cache",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuSeparator( assetsMenu );

            auto resourcesMenu = ui->addElementByType<ui::IUIMenu>();
            resourcesMenu->setLabel( "Resources" );

            Util::addMenuItem( resourcesMenu, static_cast<s32>( WidgetId::AddResourceNodeId ),
                               ICON_FA_FILM " Add Resource Node", "Adds a node to the resource tree",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( resourcesMenu, static_cast<s32>( WidgetId::DeleteResourceNodeId ),
                               ICON_FA_FILM " Delete Resource Node",
                               "Deletes a node to the resource tree", ui::IUIMenuItem::Type::Normal );

            Util::addMenuSeparator( resourcesMenu );

            Util::addMenuItem( resourcesMenu, static_cast<s32>( WidgetId::ResourcesAddComponentId ),
                               ICON_FA_FILM " Add Component", "Adds a component group",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( resourcesMenu, static_cast<s32>( WidgetId::ResourcesRemoveComponentId ),
                               ICON_FA_FILM " Remove Component", "Adds a component group",
                               ui::IUIMenuItem::Type::Normal );

            assetsMenu->addMenuItem( resourcesMenu );

            auto assetsGroupMenu = ui->addElementByType<ui::IUIMenu>();
            assetsGroupMenu->setLabel( "Group" );

            Util::addMenuItem( assetsGroupMenu, static_cast<s32>( WidgetId::AddGroupId ),
                               ICON_FA_FILM " Add Group", "Adds a component group",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( assetsGroupMenu, static_cast<s32>( WidgetId::RemoveGroupId ),
                               ICON_FA_FILM " Remove Group", "Adds a component group",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuSeparator( assetsGroupMenu );

            Util::addMenuItem( assetsGroupMenu, static_cast<s32>( WidgetId::GroupAddComponentId ),
                               ICON_FA_FILM " Add Component", "Adds a component to a group",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( assetsGroupMenu, static_cast<s32>( WidgetId::GroupRemoveComponentId ),
                               ICON_FA_FILM " Remove Component", "Removes a component from a group",
                               ui::IUIMenuItem::Type::Normal );

            assetsMenu->addMenuItem( assetsGroupMenu );

            menuBar->addMenu( assetsMenu );

            auto utilMenu = ui->addElementByType<ui::IUIMenu>();
            utilMenu->setLabel( "Util" );

            auto utilImportMenu = ui->addElementByType<ui::IUIMenu>();
            utilImportMenu->setLabel( "Import" );
            Util::addMenuItem( utilImportMenu, static_cast<s32>( WidgetId::ImportJsonSceneId ),
                               ICON_FA_FILE " Import Json Scene", "Import Json Scene",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( utilImportMenu, static_cast<s32>( WidgetId::ImportUnityYamlId ),
                               ICON_FA_FILE " Import Unity Yaml", "Import Json Scene",
                               ui::IUIMenuItem::Type::Normal );
            utilMenu->addMenuItem( utilImportMenu );

            auto utilDebugMenu = ui->addElementByType<ui::IUIMenu>();
            utilDebugMenu->setLabel( "Debug" );
            Util::addMenuItem( utilDebugMenu, static_cast<s32>( WidgetId::ShowAllOverlaysId ),
                               ICON_FA_EYE " showAllOverlays", "showAllOverlays",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( utilDebugMenu, static_cast<s32>( WidgetId::HideAllOverlaysId ),
                               ICON_FA_EYE_SLASH " hideAllOverlays", "hideAllOverlays",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( utilDebugMenu, static_cast<s32>( WidgetId::CreateOverlayTestId ),
                               ICON_FA_FILE " createOverlayPanelTest", "createOverlayPanelTest",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( utilDebugMenu, static_cast<s32>( WidgetId::CreateOverlayTextTestId ),
                               ICON_FA_FONT " createOverlayTextTest", "createOverlayTextTest",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( utilDebugMenu, static_cast<s32>( WidgetId::CreateOverlayButtonTestId ),
                               ICON_FA_SQUARE " createOverlayButtonTest", "createOverlayButtonTest",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( utilDebugMenu, static_cast<s32>( WidgetId::CreateBoxTestId ),
                               ICON_FA_CUBE " createBoxTest", "createBoxTest",
                               ui::IUIMenuItem::Type::Normal );
            utilMenu->addMenuItem( utilDebugMenu );

            auto utilProceduralMenu = ui->addElementByType<ui::IUIMenu>();
            utilProceduralMenu->setLabel( "Procedural" );
            Util::addMenuItem( utilProceduralMenu, static_cast<s32>( WidgetId::CreateProceduralTestId ),
                               ICON_FA_FILE " Create Procedural Test", "Create Procedural Test",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( utilProceduralMenu, static_cast<s32>( WidgetId::LoadProceduralSceneId ),
                               ICON_FA_FOLDER_OPEN " Load Procedural Scene", "Load Procedural Scene",
                               ui::IUIMenuItem::Type::Normal );
            utilMenu->addMenuItem( utilProceduralMenu );

            auto utilSceneMenu = ui->addElementByType<ui::IUIMenu>();
            utilSceneMenu->setLabel( "Scene" );
            Util::addMenuItem( utilSceneMenu,
                               static_cast<s32>( WidgetId::CreateProceduralTestSceneMenuId ),
                               ICON_FA_FILE " Create Procedural Test", "Create Procedural Test",
                               ui::IUIMenuItem::Type::Normal );
            utilMenu->addMenuItem( utilSceneMenu );

            auto utilDatabaseMenu = ui->addElementByType<ui::IUIMenu>();
            utilDatabaseMenu->setLabel( "Database" );
            Util::addMenuItem( utilDatabaseMenu, static_cast<s32>( WidgetId::OptimiseDatabasesId ),
                               ICON_FA_FILE " Optimise Databases", "Optimise Databases",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( utilDatabaseMenu, static_cast<s32>( WidgetId::CleanDatabasesId ),
                               ICON_FA_TRASH " Clean Databases", "Clean Databases",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem(
                utilDatabaseMenu, static_cast<s32>( WidgetId::CreateAssetFromDatabasesId ),
                ICON_FA_PLUS " Create asset", "Create asset", ui::IUIMenuItem::Type::Normal );
            utilMenu->addMenuItem( utilDatabaseMenu );

            auto utilMaterialMenu = ui->addElementByType<ui::IUIMenu>();
            utilMaterialMenu->setLabel( "Materials" );
            Util::addMenuItem( utilMaterialMenu, static_cast<s32>( WidgetId::GenerateSkyboxMaterialsId ),
                               ICON_FA_CLOUD " Generate Skybox Materials", "Generate Skybox Materials",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( utilMaterialMenu, static_cast<s32>( WidgetId::SetupMaterialsId ),
                               ICON_FA_FILE " Setup Materials", "Setup Materials",
                               ui::IUIMenuItem::Type::Normal );
            utilMenu->addMenuItem( utilMaterialMenu );

            auto utilPhysicsMenu = ui->addElementByType<ui::IUIMenu>();
            utilPhysicsMenu->setLabel( "Physics" );

            //Util::addMenuItem( utilPhysicsMenu, static_cast<s32>( WidgetId::CreateRigidBodies ),
            //                   "Create Rigid Bodies", "Create Rigid Bodies",
            //                   ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( utilPhysicsMenu, static_cast<s32>( WidgetId::CreateRigidStaticMeshId ),
                               ICON_FA_CUBE " Create Rigid Static", "Create Rigid Static",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( utilPhysicsMenu, static_cast<s32>( WidgetId::CreateRigidDynamicMeshId ),
                               ICON_FA_CUBE " Create Rigid Dynamic", "Create Rigid Dynamic",
                               ui::IUIMenuItem::Type::Normal );

            utilMenu->addMenuItem( utilPhysicsMenu );

            auto utilProjectMenu = ui->addElementByType<ui::IUIMenu>();
            utilProjectMenu->setLabel( "Project" );

            Util::addMenuItem( utilProjectMenu, static_cast<s32>( WidgetId::CreatePluginCodeId ),
                               ICON_FA_CODE " Create Plugin Code", "Create Plugin Code",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( utilProjectMenu, static_cast<s32>( WidgetId::LoadPluginId ),
                               ICON_FA_PLUG " Load Plugin", "Load Plugin",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( utilProjectMenu, static_cast<s32>( WidgetId::UnloadPluginId ),
                               ICON_FA_PLUG " Unload Plugin", "Unload Plugin",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( utilProjectMenu, static_cast<s32>( WidgetId::CopyEngineFilesId ),
                               ICON_FA_COPY " Copy Engine Files", "Copy Engine Files",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( utilProjectMenu, static_cast<s32>( WidgetId::CleanProjectId ),
                               ICON_FA_TRASH " Clean", "Clean", ui::IUIMenuItem::Type::Normal );

            utilMenu->addMenuItem( utilProjectMenu );

            auto utilSystemMenu = ui->addElementByType<ui::IUIMenu>();
            utilSystemMenu->setLabel( "System" );

            Util::addMenuItem( utilSystemMenu, static_cast<s32>( WidgetId::MakeAllStateContextsDirtyId ),
                               ICON_FA_FILE " Make all contexts dirty", "Make all contexts dirty",
                               ui::IUIMenuItem::Type::Normal );
            utilMenu->addMenuItem( utilSystemMenu );

            menuBar->addMenu( utilMenu );

            auto utilMeshMenu = ui->addElementByType<ui::IUIMenu>();
            utilMeshMenu->setLabel( "Mesh" );

            Util::addMenuItem( utilMeshMenu, static_cast<s32>( WidgetId::ConvertFbx ),
                               ICON_FA_CUBE " Convert Fbx", "Convert Fbx",
                               ui::IUIMenuItem::Type::Normal );

            Util::addMenuItem( utilMeshMenu, static_cast<s32>( WidgetId::ConvertAllFbx ),
                               ICON_FA_CUBES " Convert All Fbx", "Convert All Fbx",
                               ui::IUIMenuItem::Type::Normal );

            utilMenu->addMenuItem( utilMeshMenu );

            //auto proceduralMenu = ui->addElementByType<ui::IUIMenu>();
            //proceduralMenu->setLabel( "Procedural" );
            //Util::addMenuItem( proceduralMenu, static_cast<s32>( WidgetId::LoadProceduralSceneId ),
            //                   "Load Procedural Scene", "Load Procedural Scene",
            //                   ui::IUIMenuItem::Type::Normal );
            //menuBar->addMenu( proceduralMenu );

            auto windowMenu = ui->addElementByType<ui::IUIMenu>();
            windowMenu->setLabel( "Window" );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::AnimationEditorId ),
                               ICON_FA_LIST " Animation Editor", "Animation Editor",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::AnimationGraphEditorId ),
                               ICON_FA_SITEMAP " Animation Graph Editor", "Animation Graph Editor",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::ProceduralModelEditorId ),
                               ICON_FA_LIST " Procedural Model", "Procedural Model",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::ComponentsId ),
                               ICON_FA_LIST " Components", "Components", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::ResourcesId ),
                               ICON_FA_FILE " Resources", "Resources", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::InputWindowId ),
                               ICON_FA_FILE " Input", "Input", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::LayersWindowId ),
                               ICON_FA_LIST " Layers", "Layers", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::CollisionMasksWindowId ),
                               ICON_FA_LIST " Collision Masks", "Collision Masks",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::TagsWindowId ),
                               ICON_FA_LIST " Tags", "Tags", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::ProfilerWindowId ),
                               ICON_FA_FILE " Profiler", "Profiler", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::GraphicsPipelineId ),
                               "Editor Graphics", "Editor Graphics", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::GameGraphicsId ),
                               "Game Graphics", "Game Graphics", ui::IUIMenuItem::Type::Normal );
            Util::addMenuSeparator( windowMenu );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::ObjectWindowId ),
                               ICON_FA_CUBE " Object", "Object", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::ProjectWindowId ),
                               ICON_FA_FOLDER_OPEN " Project", "Project",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::SceneWindowId ),
                               ICON_FA_FILM " Scene", "Scene", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::SoundWindowId ),
                               ICON_FA_MUSIC " Sound", "Sound", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::ParticleWindowId ),
                               ICON_FA_FILE " Particle", "Particle", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::ShaderWindowId ),
                               ICON_FA_CODE " Shader", "Shader", ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::CutsceneWindowId ),
                               ICON_FA_FILM " Cutscene Editor", "Cutscene Editor",
                               ui::IUIMenuItem::Type::Normal );
            Util::addMenuItem( windowMenu, static_cast<s32>( WidgetId::AssetDatabaseWindowId ),
                               ICON_FA_FILE " Asset Database", "Asset Database",
                               ui::IUIMenuItem::Type::Normal );

            menuBar->addMenu( windowMenu );

            auto helpMenu = ui->addElementByType<ui::IUIMenu>();
            helpMenu->setLabel( "Help" );
            Util::addMenuItem( helpMenu, static_cast<s32>( WidgetId::AboutId ), ICON_FA_FILE " About",
                               "About", ui::IUIMenuItem::Type::Normal );
            menuBar->addMenu( helpMenu );

            auto menubarListener = workphone::make_ptr<MenuBarListener>();
            menubarListener->setOwner( this );
            m_menubarListener = menubarListener;

            menuBar->addObjectListener( m_menubarListener );
        }

        auto toolbar = ui->addElementByType<ui::IUIToolbar>();
        if( toolbar )
        {
            auto toolbarListener = workphone::make_ptr<ToolbarListener>();
            toolbarListener->setOwner( this );
            toolbar->addObjectListener( toolbarListener );

            // Play/Stop controls
            auto toolbarPlayButton = ui->addElementByType<ui::IUILabelTogglePair>();
            toolbarPlayButton->setElementId( static_cast<s32>( WidgetId::RunId ) );
            toolbarPlayButton->setLabel( ICON_FA_PLAY " Play" );
            toolbar->addChild( toolbarPlayButton );
            toolbarPlayButton->setValue( false );
            m_playmodeToggle = toolbarPlayButton;

            // Add separator
            if( auto separator = ui->addElementByType<ui::IUISeparator>() )
            {
                toolbar->addChild( separator );
            }

            // Editor camera controls
            auto toggleEditorCameraButton = ui->addElementByType<ui::IUILabelTogglePair>();
            toggleEditorCameraButton->setElementId( static_cast<s32>( WidgetId::ToggleEditorCameraId ) );
            toggleEditorCameraButton->setLabel( ICON_FA_VIDEO " Editor Camera" );
            toolbar->addChild( toggleEditorCameraButton );
            toggleEditorCameraButton->setValue( true );
            m_editorCameraToggle = toggleEditorCameraButton;

            // Add separator
            if( auto separator = ui->addElementByType<ui::IUISeparator>() )
            {
                toolbar->addChild( separator );
            }

            // Transform controls
            auto toggleTranslateButton = ui->addElementByType<ui::IUILabelTogglePair>();
            toggleTranslateButton->setElementId( static_cast<s32>( WidgetId::TranslateEditorCameraId ) );
            toggleTranslateButton->setLabel( ICON_FA_VIDEO " Translate" );
            toolbar->addChild( toggleTranslateButton );
            toggleTranslateButton->setValue( false );
            m_toolbarTranslateManipulatorButton = toggleTranslateButton;

            auto toggleRotateButton = ui->addElementByType<ui::IUILabelTogglePair>();
            toggleRotateButton->setElementId( static_cast<s32>( WidgetId::RotateEditorCameraId ) );
            toggleRotateButton->setLabel( ICON_FA_VIDEO " Rotate" );
            toolbar->addChild( toggleRotateButton );
            toggleRotateButton->setValue( false );
            m_toolbarRotateManipulatorButton = toggleRotateButton;

            auto toggleScaleButton = ui->addElementByType<ui::IUILabelTogglePair>();
            toggleScaleButton->setElementId( static_cast<s32>( WidgetId::ScaleEditorCameraId ) );
            toggleScaleButton->setLabel( ICON_FA_VIDEO " Scale" );
            toolbar->addChild( toggleScaleButton );
            toggleScaleButton->setValue( false );
            m_toolbarScaleManipulatorButton = toggleScaleButton;

            auto toolbarLocalTransformButton = ui->addElementByType<ui::IUILabelTogglePair>();
            toolbarLocalTransformButton->setElementId( static_cast<s32>( WidgetId::LocalTransformId ) );
            toolbarLocalTransformButton->setLabel( ICON_FA_COMPASS " Local" );
            toolbar->addChild( toolbarLocalTransformButton );
            toolbarLocalTransformButton->setValue( editorManager->isTransformLocal() );
            m_toolbarLocalTransformButton = toolbarLocalTransformButton;

            // Add separator
            if( auto separator = ui->addElementByType<ui::IUISeparator>() )
            {
                toolbar->addChild( separator );
            }

            // Debug controls
            auto toolbarShowDebugButton = ui->addElementByType<ui::IUILabelTogglePair>();
            toolbarShowDebugButton->setElementId( static_cast<s32>( WidgetId::ShowDebugId ) );
            toolbarShowDebugButton->setLabel( ICON_FA_BUG " Debug" );
            toolbar->addChild( toolbarShowDebugButton );
            toolbarShowDebugButton->setValue( false );
            m_toolbarShowDebugButton = toolbarShowDebugButton;

            auto toolbarShowSceneDebugButton = ui->addElementByType<ui::IUILabelTogglePair>();
            toolbarShowSceneDebugButton->setElementId( static_cast<s32>( WidgetId::ShowSceneDebugId ) );
            toolbarShowSceneDebugButton->setLabel( ICON_FA_CUBE " Scene Debug" );
            toolbar->addChild( toolbarShowSceneDebugButton );
            toolbarShowSceneDebugButton->setValue( false );
            m_toolbarShowSceneDebugButton = toolbarShowSceneDebugButton;

            auto toolbarShowUiDebugButton = ui->addElementByType<ui::IUILabelTogglePair>();
            toolbarShowUiDebugButton->setElementId( static_cast<s32>( WidgetId::ShowUiDebugId ) );
            toolbarShowUiDebugButton->setLabel( ICON_FA_WINDOW_MAXIMIZE " UI Debug" );
            toolbar->addChild( toolbarShowUiDebugButton );
            toolbarShowUiDebugButton->setValue( false );
            m_toolbarShowUiDebugButton = toolbarShowUiDebugButton;

            // Add separator
            if( auto separator = ui->addElementByType<ui::IUISeparator>() )
            {
                toolbar->addChild( separator );
            }

            // Utility controls
            auto toolbarStatsButton = ui->addElementByType<ui::IUIButton>();
            toolbarStatsButton->setElementId( static_cast<s32>( WidgetId::StatsId ) );
            toolbarStatsButton->setLabel( ICON_FA_CHART_BAR " Stats" );
            toolbar->addChild( toolbarStatsButton );

            auto reloadScriptsButton = ui->addElementByType<ui::IUIButton>();
            reloadScriptsButton->setElementId( static_cast<s32>( WidgetId::ReloadScriptsId ) );
            reloadScriptsButton->setLabel( ICON_FA_CHART_BAR " Reload" );
            toolbar->addChild( reloadScriptsButton );

            setToolbar( toolbar );
        }

        auto application = ui->getApplication();
        if( application )
        {
            application->setMenubar( menuBar );
            application->setToolbar( toolbar );
        }

        auto sceneWindow = factoryManager->make_ptr<SceneWindow>( nullptr );
        sceneWindow->load( nullptr );
        setSceneWindow( sceneWindow );

        auto layerManager = workphone::make_ptr<LayerManager>();
        layerManager->refreshFromScene();
        setLayerManager( layerManager );

        auto collisionMaskManager = workphone::make_ptr<CollisionMaskManager>();
        collisionMaskManager->refreshFromScene();
        setCollisionMaskManager( collisionMaskManager );

        auto tagManager = workphone::make_ptr<TagManager>();
        tagManager->refreshFromScene();
        setTagManager( tagManager );

        auto objectWindow = factoryManager->make_ptr<ObjectWindow>();
        objectWindow->load( nullptr );
        setObjectWindow( objectWindow );

        auto projectWindow = factoryManager->make_ptr<ProjectWindow>();
        projectWindow->load( nullptr );
        setProjectWindow( projectWindow );

        auto objectBrowserDialog = factoryManager->make_ptr<ObjectBrowserDialog>();
        objectBrowserDialog->load( nullptr );
        setObjectBrowserDialog( objectBrowserDialog );
        objectBrowserDialog->setWindowVisible( false );

        auto layerDialog = workphone::make_ptr<LayerDialog>();
        layerDialog->load( nullptr );
        setLayerDialog( layerDialog );
        layerDialog->setWindowVisible( false );

        auto collisionMaskDialog = workphone::make_ptr<CollisionMaskDialog>();
        collisionMaskDialog->load( nullptr );
        setCollisionMaskDialog( collisionMaskDialog );
        collisionMaskDialog->setWindowVisible( false );

        auto tagDialog = workphone::make_ptr<TagDialog>();
        tagDialog->load( nullptr );
        setTagDialog( tagDialog );
        tagDialog->setWindowVisible( false );

        auto resourceDatabaseDialog = factoryManager->make_ptr<ResourceDatabaseDialog>();
        resourceDatabaseDialog->load( nullptr );
        setResourceDatabaseDialog( resourceDatabaseDialog );
        resourceDatabaseDialog->setWindowVisible( false );

        //auto inputManagerWindow = factoryManager->make_ptr<InputManagerWindow>();
        //inputManagerWindow->load( nullptr );
        //setInputManagerWindow( inputManagerWindow );
        //inputManagerWindow->setWindowVisible( false );

        auto inputManagerWindow = factoryManager->make_ptr<ScriptWindow>();
        inputManagerWindow->setClassName( "InputManager" );
        inputManagerWindow->load( nullptr );
        setInputManagerWindow( inputManagerWindow );
        inputManagerWindow->setWindowVisible( false );

        auto profilerWindow = factoryManager->make_ptr<ProfilerWindow>();
        profilerWindow->load( nullptr );
        setProfilerWindow( profilerWindow );
        profilerWindow->setWindowVisible( false );

        auto aboutDialog = factoryManager->make_ptr<ScriptWindow>();
        aboutDialog->setClassName( "AboutDialog" );
        aboutDialog->load( nullptr );
        setAboutDialog( aboutDialog );
        aboutDialog->setWindowVisible( false );

        auto soundWindow = factoryManager->make_ptr<ScriptWindow>();
        soundWindow->setClassName( "SoundEditor" );
        soundWindow->load( nullptr );
        setSoundWindow( soundWindow );
        soundWindow->setWindowVisible( false );

        auto particleSystemWindow = factoryManager->make_ptr<ScriptWindow>();
        particleSystemWindow->setClassName( "ParticleEditor" );
        particleSystemWindow->load( nullptr );
        setParticleSystemWindow( particleSystemWindow );
        particleSystemWindow->setWindowVisible( false );

        auto shaderWindow = factoryManager->make_ptr<ScriptWindow>();
        shaderWindow->setClassName( "ShaderEditor" );
        shaderWindow->load( nullptr );
        setShaderWindow( shaderWindow );
        shaderWindow->setWindowVisible( false );

        auto packageWindow = factoryManager->make_ptr<ScriptWindow>();
        packageWindow->setClassName( "PackageEditor" );
        packageWindow->load( nullptr );
        setPackageWindow( packageWindow );
        packageWindow->setWindowVisible( false );

        auto projectSettingsWindow = factoryManager->make_ptr<ScriptWindow>();
        projectSettingsWindow->setClassName( "ProjectSettings" );
        projectSettingsWindow->load( nullptr );
        setProjectSettingsWindow( projectSettingsWindow );
        projectSettingsWindow->setWindowVisible( false );

        auto proceduralModelEditorWindow = factoryManager->make_ptr<ScriptWindow>();
        proceduralModelEditorWindow->setClassName( "ProceduralModelEditor" );
        proceduralModelEditorWindow->load( nullptr );
        setProceduralModelEditorWindow( proceduralModelEditorWindow );
        proceduralModelEditorWindow->setWindowVisible( false );

        auto animationEditorWindow = factoryManager->make_ptr<AnimationWindow>();
        animationEditorWindow->load( nullptr );
        setAnimationWindow( animationEditorWindow );
        animationEditorWindow->setWindowVisible( false );

        auto animationGraphEditorWindow = factoryManager->make_ptr<AnimationGraphWindow>();
        animationGraphEditorWindow->load( nullptr );
        setAnimationGraphWindow( animationGraphEditorWindow );
        animationGraphEditorWindow->setWindowVisible( false );

        auto cutsceneWindow = factoryManager->make_ptr<CutsceneWindow>();
        cutsceneWindow->load( nullptr );
        setCutsceneWindow( cutsceneWindow );
        cutsceneWindow->setWindowVisible( false );

        if( project )
        {
            projectSettingsWindow->setProperties( project->getProperties() );
        }

        auto assetDatabaseWindow = factoryManager->make_ptr<AssetDatabaseWindow>();
        assetDatabaseWindow->setClassName( "AssetDatabase" );
        assetDatabaseWindow->load( nullptr );
        setAssetDatabaseWindow( assetDatabaseWindow );
        assetDatabaseWindow->setWindowVisible( false );

        //auto addDialog = factoryManager->make_ptr<EditorWindow>();
        //addDialog->load( nullptr );
        //setProjectWindow( addDialog );

        m_updateSelectionJob = factoryManager->make_ptr<UpdateSelectionJob>();

        auto eventListener = factoryManager->make_ptr<EventListener>();
        eventListener->setOwner( this );
        applicationManager->addObjectListener( eventListener );

        m_eventListener = eventListener;

        setClassName( "EditorUI" );

        auto invoker = factoryManager->make_ptr<ScriptInvoker>( this );
        setScriptInvoker( invoker );

        if( scriptManager )
        {
            WP_ASSERT( scriptManager->isValid() );

            if( !StringUtil::isNullOrEmpty( m_className ) )
            {
                scriptManager->createObject( m_className, this );
            }
        }

        if( invoker )
        {
            invoker->callObjectMember( scene::IGameEditor::loadStr );
        }

        applicationManager->triggerEvent( EventType::Scene, IEvent::createUI, Array<Parameter>(), this,
                                          this, nullptr );

        setLoadingState( LoadingState::Loaded );
    }

    void UIManager::unload( SmartPtr<ISharedObject> data )
    {
        if( !isLoaded() )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto editorManager = EditorManager::getSingletonPtr();
        auto project = editorManager->getProject();

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        if( m_frameStatistics )
        {
            m_frameStatistics->unload( nullptr );
            m_frameStatistics = nullptr;
        }

        if( auto resourceDatabaseDialog = getResourceDatabaseDialog() )
        {
            resourceDatabaseDialog->unload( nullptr );
            setResourceDatabaseDialog( nullptr );
        }

        if( auto sceneWindow = getSceneWindow() )
        {
            sceneWindow->unload( nullptr );
            setSceneWindow( nullptr );
        }

        if( auto propertiesWindow = getPropertiesWindow() )
        {
            propertiesWindow->unload( nullptr );
            setPropertiesWindow( nullptr );
        }

        if( auto projectWindow = getProjectWindow() )
        {
            projectWindow->unload( nullptr );
            setProjectWindow( nullptr );
        }

        if( auto objectBrowserDialog = getObjectBrowserDialog() )
        {
            objectBrowserDialog->unload( nullptr );
            setObjectBrowserDialog( nullptr );
        }

        if( auto layerDialog = getLayerDialog() )
        {
            layerDialog->unload( nullptr );
            setLayerDialog( nullptr );
        }

        setLayerManager( nullptr );

        if( auto collisionMaskDialog = getCollisionMaskDialog() )
        {
            collisionMaskDialog->unload( nullptr );
            setCollisionMaskDialog( nullptr );
        }

        setCollisionMaskManager( nullptr );

        if( auto tagDialog = getTagDialog() )
        {
            tagDialog->unload( nullptr );
            setTagDialog( nullptr );
        }

        setTagManager( nullptr );

        if( auto objectWindow = getObjectWindow() )
        {
            objectWindow->unload( nullptr );
            setObjectWindow( nullptr );
        }

        if( auto actorWindow = getActorWindow() )
        {
            actorWindow->unload( nullptr );
            setActorWindow( nullptr );
        }

        if( auto terrainWindow = getTerrainWindow() )
        {
            terrainWindow->unload( nullptr );
            setTerrainWindow( nullptr );
        }

        if( auto fileWindow = getFileWindow() )
        {
            fileWindow->unload( nullptr );
            setFileWindow( nullptr );
        }

        if( auto materialWindow = getMaterialWindow() )
        {
            materialWindow->unload( nullptr );
            setMaterialWindow( nullptr );
        }

        if( auto profilerWindow = getProfilerWindow() )
        {
            profilerWindow->unload( nullptr );
            setProfilerWindow( nullptr );
        }

        if( auto soundWindow = getSoundWindow() )
        {
            soundWindow->unload( nullptr );
            setSoundWindow( nullptr );
        }

        if( auto particleSystemWindow = getParticleSystemWindow() )
        {
            particleSystemWindow->unload( nullptr );
            setParticleSystemWindow( nullptr );
        }

        if( auto shaderWindow = getShaderWindow() )
        {
            shaderWindow->unload( nullptr );
            setShaderWindow( nullptr );
        }

        if( auto packageWindow = getPackageWindow() )
        {
            packageWindow->unload( nullptr );
            setPackageWindow( nullptr );
        }

        if( auto projectSettingsWindow = getProjectSettingsWindow() )
        {
            auto properties = projectSettingsWindow->getProperties();
            // Hidden script windows do not expose project properties. Closing the
            // editor must still release the window and continue application teardown.
            if( properties )
            {
                project->setProperties( properties );
                project->save();
            }

            projectSettingsWindow->unload( nullptr );
            setProjectSettingsWindow( nullptr );
        }

        if( auto assetDatabaseWindow = getAssetDatabaseWindow() )
        {
            assetDatabaseWindow->unload( nullptr );
            setAssetDatabaseWindow( nullptr );
        }

        if( auto animationWindow = getAnimationWindow() )
        {
            animationWindow->unload( nullptr );
            setAnimationWindow( nullptr );
        }

        if( auto cutsceneWindow = getCutsceneWindow() )
        {
            cutsceneWindow->unload( nullptr );
            setCutsceneWindow( nullptr );
        }

        if( auto proceduralModelEditorWindow = getProceduralModelEditorWindow() )
        {
            proceduralModelEditorWindow->unload( nullptr );
            setProceduralModelEditorWindow( nullptr );
        }

        if( auto aboutDialog = getAboutDialog() )
        {
            aboutDialog->unload( nullptr );
            setAboutDialog( nullptr );
        }

        if( auto inputManagerWindow = getInputManagerWindow() )
        {
            inputManagerWindow->unload( nullptr );
            setInputManagerWindow( nullptr );
        }

        if( auto eventListener = m_eventListener )
        {
            applicationManager->removeObjectListener( eventListener );
            m_eventListener = nullptr;
        }

        if( auto menubarListener = m_menubarListener )
        {
            if( auto application = ui->getApplication() )
            {
                application->setMenubar( nullptr );
            }

            m_menubarListener = nullptr;
        }

        if( auto application = ui->getApplication() )
        {
            application->setToolbar( nullptr );
        }

        if( auto selectionJob = m_updateSelectionJob )
        {
            selectionJob->unload( nullptr );
            m_updateSelectionJob = nullptr;
        }

        if( auto toolbar = getToolbar() )
        {
            ui->removeElement( toolbar );
            setToolbar( nullptr );
        }

        m_toolbarShowDebugButton = nullptr;
        m_toolbarShowSceneDebugButton = nullptr;
        m_toolbarShowUiDebugButton = nullptr;
        m_toolbarLocalTransformButton = nullptr;
        m_toolbarTranslateManipulatorButton = nullptr;
        m_toolbarRotateManipulatorButton = nullptr;
        m_toolbarScaleManipulatorButton = nullptr;
        m_editorCameraToggle = nullptr;
        m_playmodeToggle = nullptr;
        m_scriptClass = nullptr;
        m_invoker = nullptr;

        applicationManager->triggerEvent( EventType::Scene, IEvent::destroyUI, Array<Parameter>(), this,
                                          this, nullptr, true );

        // The application owns the root UI container. Unload it only after every editor and
        // extension window has released its elements and listeners.
        if( auto application = ui->getApplication() )
        {
            application->unload( nullptr );
        }

        setLoadingState( LoadingState::Unloaded );
    }

    void UIManager::rebuildResourceTree()
    {
        if( auto projectWindow = getProjectWindow() )
        {
            projectWindow->buildTree();
        }
    }

    void UIManager::rebuildSceneTree()
    {
        try
        {
            if( auto sceneWindow = getSceneWindow() )
            {
                sceneWindow->buildTree();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void UIManager::rebuildActorTree()
    {
        try
        {
            auto actorWindow = getActorWindow();
            if( actorWindow )
            {
                actorWindow->buildTree();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<SceneWindow> UIManager::getSceneWindow() const
    {
        return m_sceneWindow;
    }

    void UIManager::setSceneWindow( SmartPtr<SceneWindow> sceneWindow )
    {
        m_sceneWindow = sceneWindow;
    }

    SmartPtr<ActorWindow> UIManager::getActorWindow() const
    {
        return m_actorWindow;
    }

    void UIManager::updateSelection()
    {
        //if( m_updateSelectionJob->isFinished() )
        //{
        //    auto applicationManager = core::IApplicationManager::instance();
        //    auto jobQueue = applicationManager->getJobQueue();

        //    jobQueue->addJob( m_updateSelectionJob );
        //    //m_updateSelectionJob->execute();
        //}
    }

    void UIManager::updateActorSelection()
    {
        if( auto propertiesWindow = getPropertiesWindow() )
        {
            propertiesWindow->updateSelection();
        }
    }

    void UIManager::updateComponentSelection()
    {
        if( auto propertiesWindow = getPropertiesWindow() )
        {
            propertiesWindow->updateSelection();
        }

        if( auto objectWindow = getObjectWindow() )
        {
            objectWindow->updateSelection();
        }
    }

    PropertiesWindow *UIManager::getPropertiesWindow() const
    {
        return m_propertiesWindow;
    }

    void UIManager::setPropertiesWindow( PropertiesWindow *val )
    {
        m_propertiesWindow = val;
    }

    ApplicationFrame *UIManager::getApplicationFrame() const
    {
        return m_appFrame;
    }

    void UIManager::setApplicationFrame( ApplicationFrame *val )
    {
        m_appFrame = val;
    }

    void UIManager::setActorWindow( SmartPtr<ActorWindow> actorWindow )
    {
        m_actorWindow = actorWindow;
    }

    TerrainWindow *UIManager::getTerrainWindow() const
    {
        return m_terrainWindow;
    }

    void UIManager::setTerrainWindow( TerrainWindow *val )
    {
        m_terrainWindow = val;
    }

    FileWindow *UIManager::getFileWindow() const
    {
        return m_fileWindow;
    }

    void UIManager::setFileWindow( FileWindow *val )
    {
        m_fileWindow = val;
    }

    SmartPtr<ProjectWindow> UIManager::getProjectWindow() const
    {
        return m_projectWindow;
    }

    void UIManager::setProjectWindow( SmartPtr<ProjectWindow> val )
    {
        m_projectWindow = val;
    }

    MaterialWindow *UIManager::getMaterialWindow() const
    {
        return m_materialWindow;
    }

    void UIManager::setMaterialWindow( MaterialWindow *materialWindow )
    {
        m_materialWindow = materialWindow;
    }

    SmartPtr<ObjectWindow> UIManager::getObjectWindow() const
    {
        return m_objectWindow;
    }

    void UIManager::setObjectWindow( SmartPtr<ObjectWindow> objectWindow )
    {
        m_objectWindow = objectWindow;
    }

    SmartPtr<ObjectBrowserDialog> UIManager::getObjectBrowserDialog() const
    {
        return m_objectBrowserDialog;
    }

    void UIManager::setObjectBrowserDialog( SmartPtr<ObjectBrowserDialog> objectBrowserDialog )
    {
        m_objectBrowserDialog = objectBrowserDialog;
    }

    SmartPtr<LayerManager> UIManager::getLayerManager() const
    {
        return m_layerManager;
    }

    void UIManager::setLayerManager( SmartPtr<LayerManager> layerManager )
    {
        m_layerManager = layerManager;
    }

    SmartPtr<LayerDialog> UIManager::getLayerDialog() const
    {
        return m_layerDialog;
    }

    void UIManager::setLayerDialog( SmartPtr<LayerDialog> layerDialog )
    {
        m_layerDialog = layerDialog;
    }

    SmartPtr<CollisionMaskManager> UIManager::getCollisionMaskManager() const
    {
        return m_collisionMaskManager;
    }

    void UIManager::setCollisionMaskManager( SmartPtr<CollisionMaskManager> collisionMaskManager )
    {
        m_collisionMaskManager = collisionMaskManager;
    }

    SmartPtr<CollisionMaskDialog> UIManager::getCollisionMaskDialog() const
    {
        return m_collisionMaskDialog;
    }

    void UIManager::setCollisionMaskDialog( SmartPtr<CollisionMaskDialog> collisionMaskDialog )
    {
        m_collisionMaskDialog = collisionMaskDialog;
    }

    SmartPtr<TagManager> UIManager::getTagManager() const
    {
        return m_tagManager;
    }

    void UIManager::setTagManager( SmartPtr<TagManager> tagManager )
    {
        m_tagManager = tagManager;
    }

    SmartPtr<TagDialog> UIManager::getTagDialog() const
    {
        return m_tagDialog;
    }

    void UIManager::setTagDialog( SmartPtr<TagDialog> tagDialog )
    {
        m_tagDialog = tagDialog;
    }

    SmartPtr<ResourceDatabaseDialog> UIManager::getResourceDatabaseDialog() const
    {
        return m_resourceDatabaseDialog;
    }

    void UIManager::setResourceDatabaseDialog( SmartPtr<ResourceDatabaseDialog> resourceDatabaseDialog )
    {
        m_resourceDatabaseDialog = resourceDatabaseDialog;
    }

    SmartPtr<EditorWindow> UIManager::getAboutDialog() const
    {
        return m_aboutDialog;
    }

    UIManager::MenuBarListener::MenuBarListener() = default;

    UIManager::MenuBarListener::~MenuBarListener() = default;

    Parameter UIManager::MenuBarListener::handleEvent( EventType eventType, hash_type eventValue,
                                                       const Array<Parameter> &arguments,
                                                       SmartPtr<ISharedObject> sender,
                                                       SmartPtr<ISharedObject> object,
                                                       SmartPtr<IEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto stateManager = applicationManager->getStateManagerPtr();
        auto fileSystem = applicationManager->getFileSystemPtr();

        auto application = applicationManager->getApplication();

        auto editorManager = EditorManager::getSingletonPtr();

        auto resourceDatabase = applicationManager->getResourceDatabase();
        auto factoryManager = applicationManager->getFactoryManager();
        auto jobQueue = applicationManager->getJobQueue();
        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto uiManager = editorManager->getUI();
        WP_ASSERT( uiManager );

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

        auto commandManager = applicationManager->getCommandManager();

        if( eventValue == IEvent::handleSelection )
        {
            auto element = workphone::static_pointer_cast<ui::IUIElement>( object );

            auto widgetID = static_cast<WidgetId>( element->getElementId() );
            switch( widgetID )
            {
            case WidgetId::SaveProjectId:
            {
                if( auto project = editorManager->getProject() )
                {
                    auto filePath = project->getFilePath();
                    if( !StringUtil::isNullOrEmpty( filePath ) )
                    {
                        project->saveToFile( filePath );
                    }
                    else
                    {
                        WP_LOG_ERROR( "Project file path empty" );
                    }
                }
                else
                {
                    WP_LOG_ERROR( "Project null" );
                }
            }
            break;
            case WidgetId::SaveId:
            {
                auto job = workphone::make_ptr<SaveSceneJob>();
                job->setPrimary( true );  // fix for macos
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::SaveSceneAsId:
            {
                auto job = workphone::make_ptr<SaveSceneJob>();
                job->setPrimary( true );  // fix for macos
                job->setSaveAs( true );
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::SaveAllId:
            {
                if( auto scene = sceneManager->getCurrentScene() )
                {
                    scene->saveScene();
                }

                if( auto project = editorManager->getProject() )
                {
                    project->save();
                }
            }
            break;
            case WidgetId::ProjectSettingsId:
            {
                if( auto projectSettingsWindow = uiManager->getProjectSettingsWindow() )
                {
                    projectSettingsWindow->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::GraphicsPipelineId:
            case WidgetId::GameGraphicsId:
            {
                const auto item = widgetID;
                SmartPtr<ISharedObject> settings;
                if( item == WidgetId::GraphicsPipelineId )
                    settings = applicationManager->getApplication();
                else if( auto project = editorManager->getProject() )
                    settings = project->getGraphicsSettingsDirector();
                if( settings )
                {
                    auto selectionManager = applicationManager->getSelectionManager();
                    selectionManager->clearSelection();
                    selectionManager->addSelectedObject( settings );
                    if( auto window = uiManager->getPropertiesWindow() )
                    {
                        window->setWindowVisible( true );
                        window->setSelected( nullptr );
                        window->updateSelection();
                    }
                }
                else
                {
                    WP_LOG_INFO( "Open a project to edit its game graphics options." );
                }
            }
            break;
            case WidgetId::CreatePackageId:
            {
                if( auto packageWindow = uiManager->getPackageWindow() )
                {
                    packageWindow->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::Exit:
            {
                applicationManager->setRunning( false );
                applicationManager->setQuit( true );
            }
            break;
            case WidgetId::AssetImportId:
            {
                auto job = workphone::make_ptr<AssetImportJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::AssetReimportId:
            {
                resourceDatabase->reimportAssets();
            }
            break;
            case WidgetId::AssetDatabaseBuildId:
            {
                auto job = workphone::make_ptr<AssetDatabaseBuildJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::AssetDatabaseImportCacheId:
            {
                resourceDatabase->importCache();
            }
            break;
            case WidgetId::AssetDatabaseDeleteCacheId:
            {
                resourceDatabase->deleteCache();
            }
            break;
            case WidgetId::ImportJsonSceneId:
            {
                if( auto fileDialog = fileSystem->openFileDialog() )
                {
                    auto projectPath = Path::getWorkingDirectory();
                    if( !fileSystem->isExistingFolder( projectPath ) )
                    {
                        projectPath = "";
                    }

                    fileDialog->setDialogMode( INativeFileDialog::DialogMode::Open );
                    fileDialog->setFileExtension( ".json" );
                    fileDialog->setFilePath( projectPath );

                    auto result = fileDialog->openDialog();
                    if( result == INativeFileDialog::Result::Dialog_Okay )
                    {
                        auto filePath = fileDialog->getFilePath();
                        if( !StringUtil::isNullOrEmpty( filePath ) )
                        {
                            filePath = StringUtil::cleanupPath( filePath );

                            auto project = editorManager->getProject();
                            WP_ASSERT( project );

                            auto uiManager = editorManager->getUI();
                            WP_ASSERT( uiManager );

                            auto fileSystem = applicationManager->getFileSystem();
                            WP_ASSERT( fileSystem );

                            auto sceneData = application->importScene( filePath );

                            auto sceneManager = applicationManager->getGameManager();
                            auto scene = sceneManager->getCurrentScene();
                            if( scene )
                            {
                                scene->fromData( sceneData );
                            }

                            uiManager->rebuildSceneTree();
                        }
                    }
                }
            }
            break;
            case WidgetId::ImportUnityYamlId:
            {
                auto job = workphone::make_ptr<ImportUnityYaml>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::UndoId:
            {
                auto cmd = commandManager->getPreviousCommand();
                if( cmd != nullptr )
                {
                    cmd->undo();
                }
            }
            break;
            case WidgetId::RedoId:
            {
                auto cmd = commandManager->getNextCommand();
                if( cmd != nullptr )
                {
                    cmd->redo();
                }
            }
            break;
            case WidgetId::CutId:
            {
                auto selectionManager = applicationManager->getSelectionManager();
                UIManager::clearClipboardActorData();

                for( auto selected : selectionManager->getSelection() )
                {
                    auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected );
                    if( actor )
                    {
                        auto data = workphone::static_pointer_cast<Properties>( actor->toData() );
                        if( data )
                        {
                            UIManager::addClipboardActorData( data );
                        }
                    }
                }

                if( !selectionManager->getSelection().empty() )
                {
                    auto cmd = workphone::make_ptr<RemoveSelectionCmd>();
                    commandManager->addCommand( cmd );
                }
            }
            break;
            case WidgetId::CopyId:
            {
                auto selectionManager = applicationManager->getSelectionManager();
                UIManager::clearClipboardActorData();

                for( auto selected : selectionManager->getSelection() )
                {
                    auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( selected );
                    if( actor )
                    {
                        auto data = workphone::static_pointer_cast<Properties>( actor->toData() );
                        if( data )
                        {
                            UIManager::addClipboardActorData( data );
                        }
                    }
                }
            }
            break;
            case WidgetId::PasteId:
            {
                if( !UIManager::s_clipboardActorData.empty() )
                {
                    auto cmd = workphone::make_ptr<PasteSelectionCmd>();
                    commandManager->addCommand( cmd );
                }
            }
            break;
            case WidgetId::DuplicateId:
            {
                auto selectionManager = applicationManager->getSelectionManager();
                if( !selectionManager->getSelection().empty() )
                {
                    auto cmd = workphone::make_ptr<DuplicateSelectionCmd>();
                    commandManager->addCommand( cmd );
                }
            }
            break;
            case WidgetId::DeleteId:
            {
                auto selectionManager = applicationManager->getSelectionManager();
                if( !selectionManager->getSelection().empty() )
                {
                    auto cmd = workphone::make_ptr<RemoveSelectionCmd>();
                    commandManager->addCommand( cmd );
                }
            }
            break;
            case WidgetId::SelectAllId:
            {
                auto selectionManager = applicationManager->getSelectionManager();
                selectionManager->clearSelection();

                auto sceneManager = applicationManager->getGameManager();
                if( auto scene = sceneManager->getCurrentScene() )
                {
                    for( auto actor : scene->getActors() )
                    {
                        if( actor )
                        {
                            selectionManager->addSelectedObject( actor );
                        }
                    }
                }

                auto editorManager = EditorManager::getSingletonPtr();
                auto uiManager = editorManager->getUI();
                if( uiManager )
                {
                    uiManager->rebuildSceneTree();
                }
            }
            break;
            case WidgetId::ProceduralModelEditorId:
            {
                auto owner = getOwner();
                auto window = owner->getProceduralModelEditorWindow();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::CutsceneWindowId:
            {
                auto owner = getOwner();
                auto window = owner->getCutsceneWindow();
                if( window )
                {
                    window->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::AnimationEditorId:
            {
                auto owner = getOwner();
                auto window = owner->getAnimationWindow();
                if( window )
                {
                    window->updateSelection();
                    window->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::AnimationGraphEditorId:
            {
                auto owner = getOwner();
                auto window = owner->getAnimationGraphWindow();
                if( window )
                {
                    window->updateSelection();
                    window->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::ComponentsId:
            {
                auto owner = getOwner();
                auto window = owner->getObjectBrowserDialog();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::ResourcesId:
            {
                auto owner = getOwner();
                auto window = owner->getResourceDatabaseDialog();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::InputWindowId:
            {
                auto owner = getOwner();
                auto window = owner->getInputManagerWindow();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::LayersWindowId:
            {
                auto owner = getOwner();
                auto window = owner->getLayerDialog();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::CollisionMasksWindowId:
            {
                auto owner = getOwner();
                auto window = owner->getCollisionMaskDialog();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::TagsWindowId:
            {
                auto owner = getOwner();
                auto window = owner->getTagDialog();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::ProfilerWindowId:
            {
                auto owner = getOwner();
                auto window = owner->getProfilerWindow();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::ObjectWindowId:
            {
                auto owner = getOwner();
                auto window = owner->getObjectWindow();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::ProjectWindowId:
            {
                auto owner = getOwner();
                auto window = owner->getProjectWindow();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::SceneWindowId:
            {
                auto owner = getOwner();
                auto window = owner->getSceneWindow();
                window->setWindowVisible( true );
            }
            break;
            case WidgetId::GenerateSkyboxMaterialsId:
            {
                auto job = workphone::make_ptr<GenerateSkybox>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::CreateBoxTestId:
            {
                auto cube = application->createDefaultCube( true );
                auto ground = application->createDefaultGround();

                uiManager->rebuildSceneTree();
            }
            break;
            case WidgetId::CreatePluginCodeId:
            {
                auto job = workphone::make_ptr<CreatePluginCodeJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::LoadPluginId:
            {
                auto pluginFileName = String( "Plugin.dll" );
                auto pluginPath = applicationManager->getProjectPath() + "/bin/windows/RelWithDebInfo/" +
                                  pluginFileName;

                auto job = workphone::make_ptr<LoadPluginJob>();
                job->setPluginPath( pluginPath );
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::UnloadPluginId:
            {
                auto job = workphone::make_ptr<UnloadPluginJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::CopyEngineFilesId:
            {
                auto job = workphone::make_ptr<CopyEngineFilesJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::CleanProjectId:
            {
                auto job = workphone::make_ptr<ProjectCleanJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::MakeAllStateContextsDirtyId:
            {
                stateManager->makeAllDirty();
            }
            break;
            case WidgetId::SetupMaterialsId:
            {
                auto job = workphone::make_ptr<SetupMaterialJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::OptimiseDatabasesId:
            {
                auto job = workphone::make_ptr<OptimiseDatabasesJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::CleanDatabasesId:
            {
                resourceDatabase->clean();
            }
            break;
            case WidgetId::CreateAssetFromDatabasesId:
            {
                //SmartPtr<scene::IPrefab> prefab = resourceDatabase->loadResource( 3 );
                //if( prefab )
                //{
                //    auto actor = prefab->createActor();
                //    scene->addActor( actor );
                //}

                uiManager->rebuildSceneTree();
            }
            break;
            case WidgetId::SoundWindowId:
            {
                if( auto soundWindow = uiManager->getSoundWindow() )
                {
                    soundWindow->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::ParticleWindowId:
            {
                if( auto particleSystemWindow = uiManager->getParticleSystemWindow() )
                {
                    particleSystemWindow->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::ShaderWindowId:
            {
                if( auto shaderWindow = uiManager->getShaderWindow() )
                {
                    shaderWindow->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::AssetDatabaseWindowId:
            {
                if( auto assetDatabaseWindow = uiManager->getAssetDatabaseWindow() )
                {
                    assetDatabaseWindow->setWindowVisible( true );
                }
            }
            break;
            case WidgetId::AboutId:
            {
                if( auto aboutDialog = uiManager->getAboutDialog() )
                {
                    aboutDialog->setWindowVisible( true );
                }
            }
            break;
            default:
            {
                auto primary = false;

                if( widgetID == WidgetId::OpenProjectId || widgetID == WidgetId::OpenSceneId )
                {
                    primary = true;
                }

                auto cmd = workphone::make_ptr<FileMenuCmd>();
                cmd->setItemId( widgetID );
                cmd->setPrimary( primary );
                commandManager->addCommand( cmd );
            }
            break;
            }
        }

        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            if( sender->isDerived<ui::IUIElement>() )
            {
                if( auto owner = getOwner() )
                {
                    if( auto invoker = owner->getScriptInvoker() )
                    {
                        Parameters args;
                        args.reserve( 8 );

                        Parameters result;

                        args.push_back( Parameter( static_cast<s32>( eventType ) ) );
                        args.push_back( Parameter( eventValue ) );
                        args.push_back( Parameter( arguments ) );
                        args.push_back( Parameter( sender ) );
                        args.push_back( Parameter( object ) );
                        args.push_back( Parameter( event.get() ) );

                        invoker->callObjectMember( "handleEvent", args, result );
                    }
                }
            }
        }
        break;
        };

        return {};
    }

    UIManager *UIManager::MenuBarListener::getOwner() const
    {
        return m_owner;
    }

    void UIManager::MenuBarListener::setOwner( UIManager *owner )
    {
        m_owner = owner;
    }

    void UIManager::setAboutDialog( SmartPtr<EditorWindow> aboutDialog )
    {
        m_aboutDialog = aboutDialog;
    }

    SmartPtr<EditorWindow> UIManager::getInputManagerWindow() const
    {
        return m_inputManagerWindow;
    }

    void UIManager::setInputManagerWindow( SmartPtr<EditorWindow> inputManagerWindow )
    {
        m_inputManagerWindow = inputManagerWindow;
    }

    SmartPtr<ProfilerWindow> UIManager::getProfilerWindow() const
    {
        return m_profilerWindow;
    }

    void UIManager::setProfilerWindow( SmartPtr<ProfilerWindow> profilerWindow )
    {
        m_profilerWindow = profilerWindow;
    }

    bool UIManager::isValid() const
    {
        bool valid = true;

        if( m_projectWindow )
        {
            valid = valid && m_projectWindow->isValid();
        }

        if( m_projectWindow )
        {
            valid = valid && m_projectWindow->isValid();
        }

        if( m_sceneWindow )
        {
            valid = valid && m_sceneWindow->isValid();
        }

        return valid;
    }

    void UIManager::setSoundWindow( SmartPtr<EditorWindow> soundWindow )
    {
        m_soundWindow = soundWindow;
    }

    SmartPtr<EditorWindow> UIManager::getSoundWindow() const
    {
        return m_soundWindow;
    }

    void UIManager::setParticleSystemWindow( SmartPtr<EditorWindow> particleSystemWindow )
    {
        m_particleSystemWindow = particleSystemWindow;
    }

    SmartPtr<EditorWindow> UIManager::getParticleSystemWindow() const
    {
        return m_particleSystemWindow;
    }

    void UIManager::setShaderWindow( SmartPtr<EditorWindow> shaderWindow )
    {
        m_shaderWindow = shaderWindow;
    }

    SmartPtr<EditorWindow> UIManager::getShaderWindow() const
    {
        return m_shaderWindow;
    }

    void UIManager::setPackageWindow( SmartPtr<EditorWindow> packageWindow )
    {
        m_packageWindow = packageWindow;
    }

    SmartPtr<EditorWindow> UIManager::getPackageWindow() const
    {
        return m_packageWindow;
    }

    void UIManager::setProjectSettingsWindow( SmartPtr<EditorWindow> projectSettingsWindow )
    {
        m_projectSettingsWindow = projectSettingsWindow;
    }

    SmartPtr<EditorWindow> UIManager::getProjectSettingsWindow() const
    {
        return m_projectSettingsWindow;
    }

    void UIManager::setToolbarShowDebugButton( SmartPtr<ui::IUILabelTogglePair> toolbarShowDebugButton )
    {
        m_toolbarShowDebugButton = toolbarShowDebugButton;
    }

    SmartPtr<ui::IUILabelTogglePair> UIManager::getToolbarShowDebugButton() const
    {
        return m_toolbarShowDebugButton;
    }

    void UIManager::setEditorCameraToggle( SmartPtr<ui::IUILabelTogglePair> editorCameraToggle )
    {
        m_editorCameraToggle = editorCameraToggle;
    }

    SmartPtr<ui::IUILabelTogglePair> UIManager::getEditorCameraToggle() const
    {
        return m_editorCameraToggle;
    }

    void UIManager::setPlaymodeToggle( SmartPtr<ui::IUILabelTogglePair> playmodeToggle )
    {
        m_playmodeToggle = playmodeToggle;
    }

    SmartPtr<ui::IUILabelTogglePair> UIManager::getPlaymodeToggle() const
    {
        return m_playmodeToggle;
    }

    void UIManager::setToolbar( SmartPtr<ui::IUIToolbar> toolbar )
    {
        m_toolbar = toolbar;
    }

    SmartPtr<ui::IUIToolbar> UIManager::getToolbar() const
    {
        return m_toolbar;
    }

    void UIManager::setAssetDatabaseWindow( SmartPtr<EditorWindow> assetDatabaseWindow )
    {
        m_assetDatabaseWindow = assetDatabaseWindow;
    }

    SmartPtr<EditorWindow> UIManager::getAssetDatabaseWindow() const
    {
        return m_assetDatabaseWindow;
    }

    // Script System Accessors
    String UIManager::getClassName() const
    {
        return m_className;
    }

    void UIManager::setClassName( const String &className )
    {
        m_className = className;
    }

    SmartPtr<IScriptClass> UIManager::getScriptClass() const
    {
        return m_scriptClass;
    }

    void UIManager::setScriptClass( SmartPtr<IScriptClass> scriptClass )
    {
        m_scriptClass = scriptClass;
    }

    SmartPtr<IScriptInvoker> UIManager::getScriptInvoker() const
    {
        return m_invoker;
    }

    void UIManager::setScriptInvoker( SmartPtr<IScriptInvoker> invoker )
    {
        m_invoker = invoker;
    }

    SmartPtr<EditorWindow> UIManager::getProceduralModelEditorWindow() const
    {
        return m_proceduralModelEditorWindow;
    }

    void UIManager::setProceduralModelEditorWindow( SmartPtr<EditorWindow> proceduralModelEditorWindow )
    {
        m_proceduralModelEditorWindow = proceduralModelEditorWindow;
    }

    void UIManager::setAnimationWindow( SmartPtr<EditorWindow> animationWindow )
    {
        m_animationWindow = animationWindow;
    }

    SmartPtr<EditorWindow> UIManager::getAnimationWindow() const
    {
        return m_animationWindow;
    }

    void UIManager::setCutsceneWindow( SmartPtr<EditorWindow> cutsceneWindow )
    {
        m_cutsceneWindow = cutsceneWindow;
    }

    SmartPtr<EditorWindow> UIManager::getCutsceneWindow() const
    {
        return m_cutsceneWindow;
    }

    void UIManager::setAnimationGraphWindow( SmartPtr<EditorWindow> animationGraphWindow )
    {
        m_animationGraphWindow = animationGraphWindow;
    }

    SmartPtr<EditorWindow> UIManager::getAnimationGraphWindow() const
    {
        return m_animationGraphWindow;
    }

    Parameter UIManager::ToolbarListener::handleEvent( EventType eventType, hash_type eventValue,
                                                       const Array<Parameter> &arguments,
                                                       SmartPtr<ISharedObject> sender,
                                                       SmartPtr<ISharedObject> object,
                                                       SmartPtr<IEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto jobQueue = applicationManager->getJobQueuePtr();
        auto commandManager = applicationManager->getCommandManagerPtr();
        auto cameraManager = applicationManager->getCameraManager();

        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( object );
        auto iElementId = element->getElementId();

        auto id = static_cast<WidgetId>( iElementId );

        auto owner = getOwner();

        if( eventValue == IEvent::loadScene )
        {
            auto editorCameraToggle = owner->getEditorCameraToggle();
            auto toggled = cameraManager->isEditorCameraEnabled();
            editorCameraToggle->setValue( toggled );
        }
        else if( eventValue == IEvent::handleValueChanged )
        {
            switch( id )
            {
            case WidgetId::RunId:
            {
                auto toggled = m_owner->m_playmodeToggle->getValue();
                if( toggled )
                {
                    auto job = workphone::make_ptr<PlaymodeJob>();
                    jobQueue->addJob( job );
                }
                else
                {
                    auto job = workphone::make_ptr<LeavePlaymodeJob>();
                    jobQueue->addJob( job );
                }
            }
            break;
            case WidgetId::ToggleEditorCameraId:
            {
                auto toggled = m_owner->m_editorCameraToggle->getValue();
                auto cmd = workphone::make_ptr<ToggleEditorCamera>();
                cmd->setToggleValue( toggled );
                commandManager->addCommand( cmd );
            }
            break;
            case WidgetId::ShowDebugId:
            {
                auto toggled = m_owner->m_toolbarShowDebugButton->getValue();
                editorManager->setShowDebug( toggled );

                auto job = workphone::make_ptr<ShowDebugJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::ShowSceneDebugId:
            {
                auto toggled = m_owner->m_toolbarShowSceneDebugButton->getValue();
                editorManager->setDrawSceneDebug( toggled );
            }
            break;
            case WidgetId::ShowUiDebugId:
            {
                auto toggled = m_owner->m_toolbarShowUiDebugButton->getValue();
                editorManager->setDrawUiDebug( toggled );
            }
            break;
            case WidgetId::LocalTransformId:
            {
                auto toggled = m_owner->m_toolbarLocalTransformButton->getValue();
                editorManager->setTransformLocal( toggled );
            }
            break;
            case WidgetId::TranslateEditorCameraId:
            {
                const auto toggled = m_owner->m_toolbarTranslateManipulatorButton->getValue();
                if( auto translateManipulator = editorManager->getTranslateManipulator() )
                {
                    translateManipulator->setEnabled( toggled );
                }

                if( toggled )
                {
                    m_owner->m_toolbarRotateManipulatorButton->setValue( false );
                    m_owner->m_toolbarScaleManipulatorButton->setValue( false );

                    if( auto rotateManipulator = editorManager->getRotateManipulator() )
                    {
                        rotateManipulator->setEnabled( false );
                    }

                    if( auto scaleManipulator = editorManager->getScaleManipulator() )
                    {
                        scaleManipulator->setEnabled( false );
                    }
                }
            }
            break;
            case WidgetId::RotateEditorCameraId:
            {
                const auto toggled = m_owner->m_toolbarRotateManipulatorButton->getValue();
                if( auto rotateManipulator = editorManager->getRotateManipulator() )
                {
                    rotateManipulator->setEnabled( toggled );
                }

                if( toggled )
                {
                    m_owner->m_toolbarTranslateManipulatorButton->setValue( false );
                    m_owner->m_toolbarScaleManipulatorButton->setValue( false );

                    if( auto translateManipulator = editorManager->getTranslateManipulator() )
                    {
                        translateManipulator->setEnabled( false );
                    }

                    if( auto scaleManipulator = editorManager->getScaleManipulator() )
                    {
                        scaleManipulator->setEnabled( false );
                    }
                }
            }
            break;
            case WidgetId::ScaleEditorCameraId:
            {
                const auto toggled = m_owner->m_toolbarScaleManipulatorButton->getValue();
                if( auto scaleManipulator = editorManager->getScaleManipulator() )
                {
                    scaleManipulator->setEnabled( toggled );
                }

                if( toggled )
                {
                    m_owner->m_toolbarTranslateManipulatorButton->setValue( false );
                    m_owner->m_toolbarRotateManipulatorButton->setValue( false );

                    if( auto translateManipulator = editorManager->getTranslateManipulator() )
                    {
                        translateManipulator->setEnabled( false );
                    }

                    if( auto rotateManipulator = editorManager->getRotateManipulator() )
                    {
                        rotateManipulator->setEnabled( false );
                    }
                }
            }
            break;
            };
        }
        else if( eventValue == IEvent::handleSelection )
        {
            switch( id )
            {
            case WidgetId::RunId:
            {
                auto job = workphone::make_ptr<PlaymodeJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::StopId:
            {
                auto job = workphone::make_ptr<LeavePlaymodeJob>();
                jobQueue->addJob( job );
            }
            break;
            case WidgetId::StatsId:
            {
                auto frameStatistics = m_owner->m_frameStatistics;
                if( frameStatistics )
                {
                    auto visible = !frameStatistics->isVisible();
                    frameStatistics->setVisible( visible );
                }
            }
            break;
            case WidgetId::ReloadScriptsId:
            {
                auto job = workphone::make_ptr<ReloadScriptsJob>();
                jobQueue->addJob( job );
            }
            break;

            default:
            {
            }
            break;
            }
        }

        return {};
    }

    UIManager *UIManager::ToolbarListener::getOwner() const
    {
        return m_owner;
    }

    void UIManager::ToolbarListener::setOwner( UIManager *owner )
    {
        m_owner = owner;
    }

    UIManager::ToolbarListener::ToolbarListener() = default;

    UIManager::ToolbarListener::~ToolbarListener() = default;

    void UIManager::UpdateSelectionJob::execute()
    {
        auto editorManager = EditorManager::getSingletonPtr();
        auto ui = editorManager->getUI();

        auto objectWindow = ui->getObjectWindow();
        if( objectWindow )
        {
            objectWindow->updateSelection();
        }

        auto propertiesWindow = ui->getPropertiesWindow();
        if( propertiesWindow )
        {
            propertiesWindow->updateSelection();
        }

        auto terrainWindow = ui->getTerrainWindow();
        if( terrainWindow )
        {
            terrainWindow->updateSelection();
        }

        if( auto animationWindow = ui->getAnimationWindow() )
        {
            animationWindow->updateSelection();
        }
    }

    UIManager::UpdateSelectionJob::UpdateSelectionJob() = default;

    UIManager::UpdateSelectionJob::~UpdateSelectionJob() = default;

    Parameter UIManager::EventListener::handleEvent( EventType eventType, hash_type eventValue,
                                                     const Array<Parameter> &arguments,
                                                     SmartPtr<ISharedObject> sender,
                                                     SmartPtr<ISharedObject> object,
                                                     SmartPtr<IEvent> event )
    {
        if( eventType == EventType::Loading )
        {
            if( eventValue == scene::IGameManager::sceneLoadedHash )
            {
                if( auto owner = getOwner() )
                {
                    owner->rebuildSceneTree();
                }
            }
        }

        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            if( sender )
            {
                if( sender->isDerived<ui::IUIElement>() )
                {
                    if( auto owner = getOwner() )
                    {
                        if( auto invoker = owner->getScriptInvoker() )
                        {
                            Parameters args;
                            args.reserve( 8 );

                            Parameters result;

                            args.push_back( Parameter( static_cast<s32>( eventType ) ) );
                            args.push_back( Parameter( eventValue ) );
                            args.push_back( Parameter( arguments ) );
                            args.push_back( Parameter( sender ) );
                            args.push_back( Parameter( object ) );
                            args.push_back( Parameter( event.get() ) );

                            static const String handleEventStr = "handleEvent";
                            invoker->callObjectMember( handleEventStr, args, result );
                        }
                    }
                }
            }
        }
        break;
        }

        return {};
    }

    SmartPtr<UIManager> UIManager::EventListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void UIManager::EventListener::setOwner( SmartPtr<UIManager> owner )
    {
        m_owner = owner;
    }

    UIManager::EventListener::EventListener() = default;

    UIManager::EventListener::~EventListener() = default;
}  // namespace workphone::editor
