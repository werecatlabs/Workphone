#include <EditorPCH.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/ProjectWindow.hpp>
#include <ui/UIManager.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/TransformWindow.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, EditorManager, IEditorManager );

    SmartPtr<EditorManager> EditorManager::m_singleton = nullptr;

    EditorManager::EditorManager() :
        m_editTerrain( false ),
        m_editFoliage( false ),
        m_fileSaved( true ),
        m_enablePhysics( false )
    {
    }

    EditorManager::~EditorManager()
    {
        unload( nullptr );
    }

    void EditorManager::load( SmartPtr<ISharedObject> data )
    {
    }

    void EditorManager::unload( SmartPtr<ISharedObject> data )
    {
        clearPlayModeScene();

        if( auto ui = getUI() )
        {
            ui->unload( nullptr );
            setUI( nullptr );
        }

        if( m_translateManipulator )
        {
            m_translateManipulator->unload( data );
            m_translateManipulator = nullptr;
        }

        if( m_rotateManipulator )
        {
            m_rotateManipulator->unload( data );
            m_rotateManipulator = nullptr;
        }

        if( m_scaleManipulator )
        {
            m_scaleManipulator->unload( data );
            m_scaleManipulator = nullptr;
        }
    }

    void EditorManager::importAssets()
    {
    }

    void EditorManager::loadProject( const String &filePath )
    {
        WP_ASSERT( Path::isExistingFile( filePath ) );

        if( Path::isExistingFile( filePath ) )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto timer = applicationManager->getTimer();
            WP_ASSERT( timer );

            auto editorManager = getSingletonPtr();
            auto project = editorManager->getProject();
            WP_ASSERT( project );

            auto uiManager = editorManager->getUI();
            WP_ASSERT( uiManager );

            project->loadFromFile( filePath );
            setProjectPath( applicationManager->getProjectPath() );
            setCachePath( applicationManager->getCachePath() );

            if( auto editorSettings = applicationManager->getEditorSettings() )
            {
                editorSettings->setProperty( "Project Path", filePath );
            }

            uiManager->rebuildSceneTree();

            auto projectWindow = uiManager->getProjectWindow();
            if( projectWindow )
            {
                projectWindow->buildTree();
            }
        }
        else
        {
            WP_LOG_ERROR( "EditorManager::loadProject: Project file not found" );
            MessageBoxUtil::show( "Project file not found" );
        }
    }

    String EditorManager::getProjectPath() const
    {
        return m_projectPath;
    }

    void EditorManager::setProjectPath( const String &path )
    {
        m_projectPath = path;
    }

    String EditorManager::getCachePath() const
    {
        return m_cachePath;
    }

    void EditorManager::setCachePath( const String &path )
    {
        m_cachePath = path;
    }

    SmartPtr<ProjectManager> EditorManager::getProjectManager() const
    {
        return m_projectManager;
    }

    void EditorManager::setProjectManager( SmartPtr<ProjectManager> projectManager )
    {
        m_projectManager = projectManager;
    }

    SmartPtr<Project> EditorManager::getProject() const
    {
        return m_project;
    }

    void EditorManager::setProject( SmartPtr<Project> project )
    {
        m_project = project;
    }

    SmartPtr<UIManager> EditorManager::getUI() const
    {
        return m_ui;
    }

    void EditorManager::setUI( SmartPtr<UIManager> ui )
    {
        m_ui = ui;
    }

    SmartPtr<render::IDecalCursor> EditorManager::getDecalCursor() const
    {
        return m_decalCursor;
    }

    void EditorManager::setDecalCursor( SmartPtr<render::IDecalCursor> decalCursor )
    {
        m_decalCursor = decalCursor;
    }

    bool EditorManager::getEditTerrain() const
    {
        return m_editTerrain;
    }

    void EditorManager::setEditTerrain( bool editTerrain )
    {
        m_editTerrain = editTerrain;
    }

    bool EditorManager::getEditFoliage() const
    {
        return m_editFoliage;
    }

    void EditorManager::setEditFoliage( bool editFoliage )
    {
        m_editFoliage = editFoliage;
    }

    bool EditorManager::getFileSaved() const
    {
        return m_fileSaved;
    }

    void EditorManager::setFileSaved( bool fileSaved )
    {
        m_fileSaved = fileSaved;
    }

    bool EditorManager::capturePlayModeScene( SmartPtr<scene::IGameScene> scene )
    {
        if( !scene || m_playModeSceneData )
        {
            return false;
        }

        auto sceneData = scene->toData();
        if( !sceneData )
        {
            return false;
        }

        m_playModeScene = scene;
        m_playModeSceneData = sceneData;
        m_playModeSceneFilePath = scene->getFilePath();
        m_playModeSceneLabel = scene->getLabel();
        m_playModeFileSaved = getFileSaved();
        return true;
    }

    SmartPtr<scene::IGameScene> EditorManager::getPlayModeScene() const
    {
        return m_playModeScene;
    }

    SmartPtr<ISharedObject> EditorManager::getPlayModeSceneData() const
    {
        return m_playModeSceneData;
    }

    SmartPtr<scene::IGameScene> EditorManager::restorePlayModeScene()
    {
        if( !m_playModeScene || !m_playModeSceneData )
        {
            return nullptr;
        }

        // Clearing a scene in State::None does nothing. Establish edit state so
        // runtime content is removed even if the game stopped or switched scenes.
        m_playModeScene->setState( scene::IGameScene::State::Edit );
        m_playModeScene->clear( true );
        m_playModeScene->fromData( m_playModeSceneData );
        m_playModeScene->setFilePath( m_playModeSceneFilePath );
        m_playModeScene->setLabel( m_playModeSceneLabel );
        m_playModeScene->setSceneLoadingState( scene::IGameScene::SceneLoadingState::Loaded );
        setFileSaved( m_playModeFileSaved );
        return m_playModeScene;
    }

    void EditorManager::clearPlayModeScene()
    {
        m_playModeScene = nullptr;
        m_playModeSceneData = nullptr;
        m_playModeSceneFilePath.clear();
        m_playModeSceneLabel.clear();
        m_playModeFileSaved = false;
    }

    void EditorManager::previewAsset( const String &path )
    {
        try
        {
            if( ApplicationUtil::isSupportedMesh( path ) )
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto sceneManager = applicationManager->getGameManager();
                auto scene = sceneManager->getCurrentScene();
                auto prefabManager = applicationManager->getPrefabManager();

                auto resource = prefabManager->loadPrefab( path );
                if( resource )
                {
                    auto actor = resource->createActor();
                    if( actor )
                    {
                        scene->addActor( actor );
                        scene->registerAllUpdates( actor );
                    }
                }

                auto uiManager = getUI();
                if( uiManager )
                {
                    uiManager->rebuildSceneTree();
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<EditorManager> EditorManager::getSingleton()
    {
        return m_singleton;
    }

    EditorManager *EditorManager::getSingletonPtr()
    {
        return m_singleton.get();
    }

    void EditorManager::setSingletonPtr( SmartPtr<EditorManager> editorManager )
    {
        m_singleton = editorManager;
    }

    SmartPtr<TranslateManipulator> EditorManager::getTranslateManipulator() const
    {
        return m_translateManipulator;
    }

    void EditorManager::setTranslateManipulator( SmartPtr<TranslateManipulator> translateManipulator )
    {
        m_translateManipulator = translateManipulator;
    }

    SmartPtr<RotateManipulator> EditorManager::getRotateManipulator() const
    {
        return m_rotateManipulator;
    }

    void EditorManager::setRotateManipulator( SmartPtr<RotateManipulator> rotateManipulator )
    {
        m_rotateManipulator = rotateManipulator;
    }

    SmartPtr<ScaleManipulator> EditorManager::getScaleManipulator() const
    {
        return m_scaleManipulator;
    }

    void EditorManager::setScaleManipulator( SmartPtr<ScaleManipulator> scaleManipulator )
    {
        m_scaleManipulator = scaleManipulator;
    }

    void EditorManager::setShowDebug( bool showDebug )
    {
        m_showDebug = showDebug;
    }

    void EditorManager::setTransformLocal( bool transformLocal )
    {
        m_isTransformLocal = transformLocal;
    }

    void EditorManager::refreshTransformUI()
    {
        if( auto ui = getUI() )
        {
            if( auto actorWindow = ui->getActorWindow() )
            {
                if( auto transformWindow = actorWindow->getTransformWindow() )
                {
                    transformWindow->updateSelection();
                }
            }
        }
    }

    bool EditorManager::isTransformLocal() const
    {
        return m_isTransformLocal;
    }

    bool EditorManager::getDrawSceneDebug() const
    {
        return m_drawSceneDebug;
    }

    void EditorManager::setDrawSceneDebug( bool drawSceneDebug )
    {
        m_drawSceneDebug = drawSceneDebug;
    }

    bool EditorManager::getDrawUiDebug() const
    {
        return m_drawUiDebug;
    }

    void EditorManager::setDrawUiDebug( bool drawUiDebug )
    {
        m_drawUiDebug = drawUiDebug;
    }

}  // namespace workphone::editor
