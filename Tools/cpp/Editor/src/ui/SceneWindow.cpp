#include <EditorPCH.hpp>
#include <ui/SceneWindow.hpp>
#include <editor/Project.hpp>
#include <editor/EditorManager.hpp>
#include <ui/ObjectWindow.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/UIManager.hpp>
#include <commands/DragDropActorCmd.hpp>
#include <commands/AddActorCmd.hpp>
#include <commands/RemoveSelectionCmd.hpp>
#include <commands/PromptCmd.hpp>
#include <jobs/SceneDropJob.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, SceneWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, SceneWindow::BuildTreeJob, Job );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, SceneWindow::ApplicationEventListener,
                               IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, SceneWindow::TreeCtrlListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, SceneWindow::SceneWindowListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, SceneWindow::PromptListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, SceneWindow::DragSource, ui::IUIDragSource );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, SceneWindow::DropTarget, ui::IUIDropTarget );

    using namespace scene;

    SceneWindow::SceneWindow()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    SceneWindow::SceneWindow( SmartPtr<ui::IUIWindow> parent )
    {
        try
        {
            setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
            setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
            setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto listener = workphone::make_ptr<SceneWindowListener>();
            listener->setOwner( this );
            m_menuListener = listener;

            auto window = ui->addElementByType<ui::IUIWindow>();
            if( window )
            {
                window->setLabel( "Scene" );
                window->setHasBorder( true );
                setParentWindow( window );

                if( parent )
                {
                    parent->addChild( window );
                }

                window->addObjectListener( m_menuListener );

#if 0
                auto sceneWindow = ui->addElementByType<ui::IUIWindow>();
                sceneWindow->setLabel( "Scene Hierarchy" );
                sceneWindow->setHasBorder( true );
                m_sceneWindow = sceneWindow;
                sceneWindow->addObjectListener( m_menuListener );
                window->addChild( sceneWindow );

                auto hierarchyToolbar = ui->addElementByType<ui::IUIWindow>();
                hierarchyToolbar->setLabel( "Hierarchy Toolbar | Create, Search, Focus, Visibility" );
                hierarchyToolbar->setHasBorder( true );
                sceneWindow->addChild( hierarchyToolbar );

                auto hierarchySearch = ui->addElementByType<ui::IUITextEntry>();
                hierarchySearch->setLabel( "Search Hierarchy" );
                hierarchySearch->setText( "" );
                hierarchyToolbar->addChild( hierarchySearch );

                auto addSceneButton = [&]( const String &label, MenuId menuId, bool sameLine ) {
                    auto button = ui->addElementByType<ui::IUIButton>();
                    button->setLabel( label );
                    button->setSameLine( sameLine );
                    button->setElementId( static_cast<hash_type>( menuId ) );
                    hierarchyToolbar->addChild( button );
                    button->addObjectListener( m_menuListener );
                    return button;
                };

                addSceneButton( "Add Actor", MenuId::ADD_NEW_ENTITY, false );
                addSceneButton( "Camera", MenuId::ADD_CAMERA, true );
                addSceneButton( "Render Target", MenuId::ADD_RENDER_TARGET, true );
                addSceneButton( "Cube", MenuId::ADD_CUBE, true );
                addSceneButton( "Light", MenuId::ADD_DIRECTIONAL_LIGHT, true );
                addSceneButton( "Particles", MenuId::ADD_PARTICLESYSTEM, true );
                addSceneButton( "Terrain", MenuId::ADD_NEW_TERRAIN, true );
                addSceneButton( "Refresh", MenuId::SCENE_REFRESH, true );
                addSceneButton( "Remove", MenuId::SCENE_REMOVE_ACTOR, true );

                auto promptPanel = ui->addElementByType<ui::IUIWindow>();
                promptPanel->setLabel( "Command Palette / AI Prompt" );
                promptPanel->setHasBorder( true );
                sceneWindow->addChild( promptPanel );

                auto inputText = ui->addElementByType<ui::IUITextEntry>();
                inputText->setLabel( "Prompt" );
                promptPanel->addChild( inputText );
                m_inputText = inputText;

                auto promptListener = workphone::make_ptr<PromptListener>();
                promptListener->setOwner( this );
                m_inputText->addObjectListener( promptListener );
                m_promptListener = promptListener;

                auto inputTextButton = ui->addElementByType<ui::IUIButton>();
                inputTextButton->setLabel( "Send" );
                inputTextButton->setSameLine( false );
                inputTextButton->setElementId( static_cast<hash_type>( MenuId::SEND_PROMPT ) );
                promptPanel->addChild( inputTextButton );
                inputTextButton->addObjectListener( m_menuListener );
                m_inputTextButton = inputTextButton;

                auto hierarchyPanel = ui->addElementByType<ui::IUIWindow>();
                hierarchyPanel->setLabel( "Hierarchy Tree | Multi-select, drag/drop, enabled toggles" );
                hierarchyPanel->setHasBorder( true );
                sceneWindow->addChild( hierarchyPanel );

                auto treeCtrl = ui->addElementByType<ui::IUITreeCtrl>();
                treeCtrl->setMultiSelect( true );
                hierarchyPanel->addChild( treeCtrl );
                m_tree = treeCtrl;

                auto dragSource = workphone::make_ptr<DragSource>();
                dragSource->setOwner( this );
                m_tree->setDragSource( dragSource );

                auto dropTarget = workphone::make_ptr<DropTarget>();
                dropTarget->setOwner( this );
                window->setDropTarget( dropTarget );
                hierarchyPanel->setDropTarget( dropTarget );
#else

                auto sceneWindow = ui->addElementByType<ui::IUIWindow>();
                sceneWindow->setLabel( "Hierarchy" );
                sceneWindow->setHasBorder( true );
                m_sceneWindow = sceneWindow;
                sceneWindow->addObjectListener( m_menuListener );
                window->addChild( sceneWindow );

                auto addSceneButton = [&]( const String &label, MenuId menuId, bool sameLine ) {
                    auto button = ui->addElementByType<ui::IUIButton>();
                    button->setLabel( label );
                    button->setSameLine( sameLine );
                    button->setElementId( static_cast<hash_type>( menuId ) );
                    button->addObjectListener( m_menuListener );
                    sceneWindow->addChild( button );
                    return button;
                };

                addSceneButton( "Create Actor", MenuId::ADD_NEW_ENTITY, false );
                addSceneButton( "Refresh", MenuId::SCENE_REFRESH, true );
                addSceneButton( "Delete", MenuId::SCENE_REMOVE_ACTOR, true );

                auto searchEntry = ui->addElementByType<ui::IUITextEntry>();
                searchEntry->setLabel( "Search##SceneHierarchySearch" );
                searchEntry->setPlaceholder( "Filter actors by name, layer, or tag..." );
                searchEntry->setText( "" );
                searchEntry->addObjectListener( m_menuListener );
                sceneWindow->addChild( searchEntry );
                m_searchEntry = searchEntry;

                auto inputText = ui->addElementByType<ui::IUITextEntry>();
                inputText->setLabel( "AI Command##ScenePrompt" );
                inputText->setPlaceholder( "Describe a scene change..." );
                sceneWindow->addChild( inputText );
                m_inputText = inputText;

                auto promptListener = workphone::make_ptr<PromptListener>();
                promptListener->setOwner( this );
                m_inputText->addObjectListener( promptListener );
                m_promptListener = promptListener;

                auto inputTextButton = ui->addElementByType<ui::IUIButton>();
                inputTextButton->setLabel( "Send" );
                inputTextButton->setSameLine( true );
                inputTextButton->setElementId( static_cast<hash_type>( MenuId::SEND_PROMPT ) );
                sceneWindow->addChild( inputTextButton );
                inputTextButton->addObjectListener( m_menuListener );
                m_inputTextButton = inputTextButton;

                auto treeCtrl = ui->addElementByType<ui::IUITreeCtrl>();
                treeCtrl->setMultiSelect( true );
                sceneWindow->addChild( treeCtrl );
                m_tree = treeCtrl;

                auto dragSource = workphone::make_ptr<DragSource>();
                dragSource->setOwner( this );
                m_tree->setDragSource( dragSource );

                auto dropTarget = workphone::make_ptr<DropTarget>();
                dropTarget->setOwner( this );
                window->setDropTarget( dropTarget );
#endif

                m_tree->setDropTarget( dropTarget );

                auto treeListener = workphone::make_ptr<TreeCtrlListener>();
                treeListener->setOwner( this );
                m_treeListener = treeListener;

                m_tree->addObjectListener( treeListener );

                m_applicationMenu = ui->addElementByType<ui::IUIMenu>();
                m_applicationMenu->setLabel( "Scene" );

                m_applicationAddMenu = ui->addElementByType<ui::IUIMenu>();
                m_applicationAddMenu->setLabel( "Add" );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_CAMERA ),
                                   "Camera", "Camera" );
                Util::addMenuItem( m_applicationAddMenu,
                                   static_cast<s32>( MenuId::ADD_RENDER_TARGET ), "Render Target",
                                   "Create a texture that cameras can render into" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_CAR ), "Car",
                                   "Car" );
                Util::addMenuItem( m_applicationAddMenu,
                                   static_cast<s32>( MenuId::ADD_HELICOPTER ), "Helicopter",
                                   "Create a default helicopter" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_PLANE ),
                                   "Plane", "Create a default aircraft plane" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_CUBE ), "Cube",
                                   "Cube" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_CUBE_MESH ),
                                   "Cube Mesh", "Cube Mesh" );
                Util::addMenuSeparator( m_applicationAddMenu );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_CUBEMAP ),
                                   "Cubemap", "Cubemap" );

                Util::addMenuSeparator( m_applicationAddMenu );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_CONSTRAINT ),
                                   "Constraint", "Constraint" );

                Util::addMenuItem( m_applicationAddMenu,
                                   static_cast<s32>( MenuId::ADD_DIRECTIONAL_LIGHT ),
                                   "Directional Light", "Directional Light" );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_POINT_LIGHT ),
                                   "Point Light", "Point Light" );
                Util::addMenuSeparator( m_applicationAddMenu );

                m_applicationParticleMenu = ui->addElementByType<ui::IUIMenu>();
                m_applicationParticleMenu->setLabel( "Particle System" );

                Util::addMenuItem( m_applicationParticleMenu,
                                   static_cast<s32>( MenuId::ADD_PARTICLESYSTEM ), "Default",
                                   "Create a balanced general-purpose particle system" );
                Util::addMenuItem( m_applicationParticleMenu,
                                   static_cast<s32>( MenuId::ADD_PARTICLESYSTEM_SMOKE ), "Smoke",
                                   "Create a slow, long-lived smoke effect" );
                Util::addMenuItem( m_applicationParticleMenu,
                                   static_cast<s32>( MenuId::ADD_PARTICLESYSTEM_SAND ), "Sand",
                                   "Create a small, fast sand or dust effect" );
                m_applicationAddMenu->addMenuItem( m_applicationParticleMenu );

                Util::addMenuSeparator( m_applicationAddMenu );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_PLANE_MESH ),
                                   "Plane Mesh", "Create a flat plane mesh" );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_PHYSICS_CUBE ),
                                   "Physics Cube", "Physics Cube" );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_SKYBOX ),
                                   "Skybox", "Skybox" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_NEW_TERRAIN ),
                                   "Terrain", "Terrain" );
                Util::addMenuSeparator( m_applicationAddMenu );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_BUTTON ),
                                   "Button", "Button" );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_CANVAS ),
                                   "Canvas", "Canvas" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_CHECKBOX ),
                                   "Checkbox", "Checkbox" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_DROPDOWN ),
                                   "Dropdown", "Dropdown" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_PANEL ), "Panel",
                                   "Panel" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_SCROLLBAR ),
                                   "ScrollBar", "ScrollBar" );
                Util::addMenuItem( m_applicationAddMenu,
                                   static_cast<s32>( MenuId::ADD_SCROLLBAR_VERTICAL ),
                                   "ScrollBar Vertical", "ScrollBar Vertical" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_SCROLLVIEW ),
                                   "ScrollView", "ScrollView" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_SLIDER ),
                                   "Slider", "Slider" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_SLIDER_VERTICAL ),
                                   "Slider Vertical", "Slider Vertical" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_TABVIEW ),
                                   "Tab View", "Tab View" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_TABLELAYOUT ),
                                   "Table Layout", "Table Layout" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_TEXT ), "Text",
                                   "Text" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::ADD_TOGGLE_BUTTON ),
                                   "Toggle Button", "Toggle Button" );
                Util::addMenuItem( m_applicationAddMenu,
                                   static_cast<s32>( MenuId::ADD_TOGGLE_WITH_TEXT ), "Toggle With Text",
                                   "Toggle With Text" );

                m_applicationMenu->addMenuItem( m_applicationAddMenu );

                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::ADD_NEW_ENTITY ),
                                   "Add Actor", "Add Actor" );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::SCENE_REMOVE_ACTOR ),
                                   "Remove", "Remove" );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::SCENE_REFRESH ),
                                   "Refresh", "Refresh" );

                m_applicationMenu->addObjectListener( m_menuListener );

                sceneWindow->setContextMenu( m_applicationMenu );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SceneWindow::~SceneWindow()
    {
        unload( nullptr );
    }

    void SceneWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loading ||
                getLoadingState() == LoadingState::Loaded )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            auto factoryManager = applicationManager->getFactoryManager();

            auto applicationEventListener = factoryManager->make_ptr<ApplicationEventListener>();
            applicationEventListener->setOwner( this );
            m_applicationEventListener = applicationEventListener;

            applicationManager->addObjectListener( applicationEventListener );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SceneWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto hasResources =
                m_applicationEventListener || m_window || m_sceneWindow || m_inputTextButton ||
                m_inputText || m_promptListener || m_searchEntry || m_tree || m_treeListener ||
                m_applicationMenu || m_applicationAddMenu || m_applicationParticleMenu ||
                m_menuListener || m_selectedObject || m_selectedEntity || m_dragDropActorCmd ||
                m_dropJob || m_buildJob || getParentWindow() || !m_dataArray.empty();

            if( getLoadingState() != LoadingState::Unloaded || hasResources )
            {
                setLoadingState( LoadingState::Unloading );

                if( auto buildJob = getBuildJob() )
                {
                    buildJob->setInterrupted( true );
                    setBuildJob( nullptr );
                }

                if( auto dropJob = getDropJob() )
                {
                    dropJob->setInterrupted( true );
                    setDropJob( nullptr );
                }

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto ui = applicationManager->getUI();
                WP_ASSERT( ui );

                if( m_applicationEventListener )
                {
                    applicationManager->removeObjectListener( m_applicationEventListener );
                    m_applicationEventListener->unload( nullptr );
                    m_applicationEventListener = nullptr;
                }

                if( m_inputText && m_promptListener )
                {
                    m_inputText->removeObjectListener( m_promptListener );
                }

                if( m_inputTextButton && m_menuListener )
                {
                    m_inputTextButton->removeObjectListener( m_menuListener );
                }

                if( m_searchEntry && m_menuListener )
                {
                    m_searchEntry->removeObjectListener( m_menuListener );
                }

                if( m_tree && m_treeListener )
                {
                    m_tree->removeObjectListener( m_treeListener );
                }

                if( m_applicationMenu && m_menuListener )
                {
                    m_applicationMenu->removeObjectListener( m_menuListener );
                }

                if( m_sceneWindow && m_menuListener )
                {
                    m_sceneWindow->removeObjectListener( m_menuListener );
                    m_sceneWindow->setContextMenu( nullptr );
                }

                if( auto parentWindow = getParentWindow() )
                {
                    parentWindow->removeObjectListener( m_menuListener );
                    parentWindow->setContextMenu( nullptr );
                }

                std::function<void( SmartPtr<workphone::ui::IUIMenu> )> removeMenuItems;
                removeMenuItems = [&ui, &removeMenuItems]( SmartPtr<workphone::ui::IUIMenu> menu ) {
                    if( !menu )
                    {
                        return;
                    }

                    auto menuItems = menu->getMenuItems();
                    for( auto menuItem : menuItems )
                    {
                        if( menuItem )
                        {
                            if( menuItem->isDerived<workphone::ui::IUIMenu>() )
                            {
                                auto subMenu =
                                    workphone::static_pointer_cast<workphone::ui::IUIMenu>( menuItem );
                                removeMenuItems( subMenu );
                            }

                            menu->removeMenuItem( menuItem );
                            ui->removeElement( menuItem );
                        }
                    }
                };

                if( m_applicationMenu )
                {
                    removeMenuItems( m_applicationMenu );
                    m_applicationAddMenu = nullptr;
                    m_applicationParticleMenu = nullptr;
                }
                else
                {
                    removeMenuItems( m_applicationAddMenu );
                    m_applicationParticleMenu = nullptr;
                }

                if( m_inputText )
                {
                    ui->removeElement( m_inputText );
                    m_inputText = nullptr;
                }

                if( m_inputTextButton )
                {
                    ui->removeElement( m_inputTextButton );
                    m_inputTextButton = nullptr;
                }

                if( m_searchEntry )
                {
                    ui->removeElement( m_searchEntry );
                    m_searchEntry = nullptr;
                }

                m_searchFilter.clear();

                if( m_tree )
                {
                    ui->removeElement( m_tree );
                    m_tree = nullptr;
                }

                m_promptListener = nullptr;
                m_treeListener = nullptr;

                if( m_sceneWindow )
                {
                    ui->removeElement( m_sceneWindow );
                    m_sceneWindow = nullptr;
                }

                if( m_applicationAddMenu )
                {
                    ui->removeElement( m_applicationAddMenu );
                    m_applicationAddMenu = nullptr;
                }

                if( m_applicationMenu )
                {
                    ui->removeElement( m_applicationMenu );
                    m_applicationMenu = nullptr;
                }

                if( m_window )
                {
                    ui->removeElement( m_window );
                    m_window = nullptr;
                }

                m_menuListener = nullptr;

                m_selectedObject = nullptr;
                m_selectedEntity = nullptr;
                m_dragDropActorCmd = nullptr;

                if( auto parentWindow = getParentWindow() )
                {
                    ui->removeElement( parentWindow );
                    setParentWindow( nullptr );
                }

                for( auto data : m_dataArray )
                {
                    data->unload( nullptr );
                }

                m_dataArray.clear();

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SceneWindow::buildTree()
    {
        try
        {
            if( getLoadingState() != LoadingState::Loaded || !getTree() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            if( auto job = getBuildJob() )
            {
                if( !job->isFinished() )
                {
                    return;
                }

                setBuildJob( nullptr );
            }

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            auto jobQueue = applicationManager->getJobQueuePtr();
            if( !factoryManager || !jobQueue )
            {
                return;
            }

            auto job = factoryManager->make_ptr<BuildTreeJob>();
            if( !job )
            {
                return;
            }

            job->setOwner( this );
            setBuildJob( job );
            jobQueue->addJob( job, TaskId::Primary );
        }
        catch( std::exception &e )
        {
            setBuildJob( nullptr );
            WP_LOG_EXCEPTION( e );
        }
    }

    SceneWindow::BuildTreeJob::BuildTreeJob()
    {
        setPrimary( true );
    }

    SceneWindow::BuildTreeJob::~BuildTreeJob() = default;

    void SceneWindow::BuildTreeJob::execute()
    {
        if( isInterrupted() )
        {
            return;
        }

        if( auto owner = getOwner() )
        {
            if( owner->getLoadingState() == LoadingState::Loaded )
            {
                owner->rebuildTree();
            }

            auto activeJob = owner->getBuildJob();
            if( activeJob && activeJob.get() == this )
            {
                owner->setBuildJob( nullptr );
            }
        }
    }

    SmartPtr<SceneWindow> SceneWindow::BuildTreeJob::getOwner() const
    {
        auto owner = m_owner.load();
        return owner.lock();
    }

    void SceneWindow::BuildTreeJob::setOwner( SmartPtr<SceneWindow> owner )
    {
        m_owner = owner;
    }

    void SceneWindow::rebuildTree()
    {
        try
        {
            RecursiveMutex::ScopedLock lock( m_buildTreeMutex );

            auto tree = getTree();
            if( !tree || getLoadingState() != LoadingState::Loaded )
            {
                return;
            }

            saveTreeState();
            tree->clear();

            for( auto data : m_dataArray )
            {
                if( data )
                {
                    data->unload( nullptr );
                }
            }

            m_dataArray.clear();

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto factoryManager = applicationManager ? applicationManager->getFactoryManagerPtr() : nullptr;
            auto editorManager = EditorManager::getSingletonPtr();
            auto sceneManager = applicationManager ? applicationManager->getGameManagerPtr() : nullptr;
            auto currentScene = sceneManager ? sceneManager->getCurrentScenePtr() : nullptr;

            if( !applicationManager || !factoryManager || !editorManager || !currentScene )
            {
                return;
            }

            auto sceneName = currentScene->getLabel();
            if( StringUtil::isNullOrEmpty( sceneName ) )
            {
                sceneName = "Untitled Scene";
            }

            auto rootNode = tree->addRoot();
            if( !rootNode )
            {
                return;
            }

            Util::setText( rootNode, sceneName );
            rootNode->setExpanded( true );

            static const String sceneStr = "scene";
            auto treeNodeData = factoryManager->make_ptr<ProjectTreeData>(
                sceneStr, sceneStr, currentScene, currentScene );
            rootNode->setNodeUserData( treeNodeData );
            m_dataArray.emplace_back( treeNodeData );

            auto actors = currentScene->getActors();
            for( auto actor : actors )
            {
                addActorToTree( actor, rootNode );
            }

            if( editorManager->getShowDebug() )
            {
                auto project = editorManager->getProject();
                if( project )
                {
                    auto projectRoot = tree->addNode();
                    if( projectRoot )
                    {
                        addObjectToTree( project, projectRoot );
                        Util::setText( projectRoot, "Project" );
                        rootNode->addChild( projectRoot );
                    }
                }

                auto engineRoot = tree->addNode();
                if( engineRoot )
                {
                    Util::setText( engineRoot, "Engine" );
                    rootNode->addChild( engineRoot );

                    auto children = applicationManager->getChildObjects();
                    for( auto child : children )
                    {
                        addObjectToTree( child, engineRoot );
                    }
                }
            }

            restoreTreeState();
            restoreSelection();
            applySearchFilter( m_searchFilter );
            tree->expand( rootNode );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SceneWindow::addObjectToTree( SmartPtr<ISharedObject> object,
                                       SmartPtr<ui::IUITreeNode> parentNode )
    {
        if( object && parentNode && m_tree )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto factoryManager = applicationManager ? applicationManager->getFactoryManagerPtr() : nullptr;
            if( !factoryManager )
            {
                return;
            }

            auto name = object->getName();
            if( StringUtil::isNullOrEmpty( name ) )
            {
                auto typeManager = TypeManager::instance();
                name = typeManager ? String( typeManager->getName( object->getTypeInfo() ) ) : String();
            }

            if( StringUtil::isNullOrEmpty( name ) )
            {
                name = "Untitled";
            }

            auto node = m_tree->addNode();
            if( !node )
            {
                return;
            }

            Util::setText( node, name );
            parentNode->addChild( node );

            auto data = factoryManager->make_ptr<ProjectTreeData>( name, name, object, object );
            node->setNodeUserData( data );
            m_dataArray.emplace_back( data );

            auto children = object->getChildObjects();
            for( auto child : children )
            {
                addObjectToTree( child, node );
            }
        }
    }

    void SceneWindow::addActorToTree( SmartPtr<IGameActor> actor, SmartPtr<ui::IUITreeNode> parentNode )
    {
        try
        {
            if( actor )
            {
                auto actorName = actor->getName();
                if( StringUtil::isNullOrEmpty( actorName ) )
                {
                    static const String untitledStr = "Untitled";
                    actorName = untitledStr;
                }

                auto applicationManager = core::IApplicationManager::instancePtr();
                auto factoryManager = applicationManager->getFactoryManagerPtr();
                auto ui = applicationManager->getUIPtr();

                if( !m_tree || !factoryManager )
                {
                    return;
                }

                auto node = m_tree->addNode();
                if( !node )
                {
                    return;
                }
                Util::setText( node, actorName );

                static const String actorStr = "actor";
                auto data =
                    factoryManager->make_ptr<ProjectTreeData>( actorStr, actorStr, actor, actor );
                node->setNodeUserData( data );

                if( parentNode )
                {
                    parentNode->addChild( node );
                }

                auto toggle = ui->addElementByType<ui::IUIToggle>();
                toggle->setSameLine( false );
                toggle->setScale( 0.5f );
                toggle->setToggled( actor->isEnabled() );
                node->addChild( toggle );

                auto children = actor->getChildren();
                for( auto &child : children )
                {
                    if( child )
                    {
                        addActorToTree( child, node );
                    }
                }

                m_dataArray.emplace_back( data );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void SceneWindow::applySearchFilter( const String &filter )
    {
        m_searchFilter = StringUtil::make_lower( filter );

        auto tree = getTree();
        auto root = tree ? tree->getRoot() : nullptr;
        if( !root )
        {
            return;
        }

        root->setVisible( true, false );
        for( auto child : root->getChildren() )
        {
            if( auto childNode = workphone::dynamic_pointer_cast<ui::IUITreeNode>( child ) )
            {
                updateNodeFilter( childNode, m_searchFilter );
            }
        }

        if( !m_searchFilter.empty() )
        {
            root->setExpanded( true );
        }
    }

    bool SceneWindow::updateNodeFilter( SmartPtr<ui::IUITreeNode> node, const String &filter )
    {
        if( !node )
        {
            return false;
        }

        auto searchable = StringUtil::make_lower( Util::getText( node ) );
        if( auto treeData =
                workphone::dynamic_pointer_cast<ProjectTreeData>( node->getNodeUserData() ) )
        {
            searchable += " " + StringUtil::make_lower( treeData->getObjectType() );
            if( auto actor = workphone::dynamic_pointer_cast<IGameActor>( treeData->getObjectData() ) )
            {
                searchable += " " + StringUtil::make_lower( actor->getLayer() );
                for( const auto &tag : actor->getTags() )
                {
                    searchable += " " + StringUtil::make_lower( tag );
                }
            }
        }

        auto matches = filter.empty() || searchable.find( filter ) != String::npos;
        auto childMatches = false;
        for( auto child : node->getChildren() )
        {
            if( auto childNode = workphone::dynamic_pointer_cast<ui::IUITreeNode>( child ) )
            {
                childMatches = updateNodeFilter( childNode, filter ) || childMatches;
            }
        }

        auto visible = matches || childMatches;
        node->setVisible( visible, false );
        if( !filter.empty() && childMatches )
        {
            node->setExpanded( true );
        }

        return visible;
    }

    void SceneWindow::handleWindowClicked()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto selectionManager = applicationManager->getSelectionManager();
        selectionManager->clearSelection();
    }

    void SceneWindow::handleTreeSelectionChanged( SmartPtr<ui::IUITreeNode> node )
    {
        if( !node )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return;
        }

        auto inputManager = applicationManager->getInputDeviceManager();

        auto timer = applicationManager->getTimerPtr();

        auto selectionManager = applicationManager->getSelectionManagerPtr();

        auto editorManager = EditorManager::getSingletonPtr();
        if( !timer || !selectionManager || !editorManager )
        {
            return;
        }

        auto data = node->getNodeUserData();
        if( data )
        {
            auto projectTreeData = workphone::dynamic_pointer_cast<ProjectTreeData>( data );
            if( !projectTreeData )
            {
                return;
            }

            auto ownerType = projectTreeData->getOwnerType();
            if( ownerType == "actor" || ownerType == "scene" )
            {
                auto object = projectTreeData->getObjectData();
                if( !object )
                {
                    return;
                }

                auto pObject = object->getSharedFromThis<ISharedObject>();

                if( inputManager && ( inputManager->isKeyPressed( KeyCodes::KEY_LSHIFT ) ||
                                      inputManager->isKeyPressed( KeyCodes::KEY_RSHIFT ) ||
                                      inputManager->isKeyPressed( KeyCodes::KEY_LCONTROL ) ||
                                      inputManager->isKeyPressed( KeyCodes::KEY_RCONTROL ) ) )
                {
                    //selectionManager->addSelectedObject( pObject );

                    selectionManager->clearSelection();

                    auto tree = getTree();
                    auto nodes = tree->getTreeNodes();
                    for( auto node : nodes )
                    {
                        if( node->isSelected() )
                        {
                            auto data = node->getNodeUserData();
                            if( data )
                            {
                                auto projectTreeData =
                                    workphone::dynamic_pointer_cast<ProjectTreeData>( data );
                                if( !projectTreeData )
                                {
                                    continue;
                                }

                                auto ownerType = projectTreeData->getOwnerType();
                                if( ownerType == "actor" || ownerType == "scene" )
                                {
                                    auto object = projectTreeData->getObjectData();
                                    selectionManager->addSelectedObject( object );
                                }
                            }
                        }
                    }
                }
                else
                {
                    selectionManager->clearSelection();
                    selectionManager->addSelectedObject( pObject );
                }
            }
            else
            {
                auto object = projectTreeData->getObjectData();

                if( inputManager && ( inputManager->isKeyPressed( KeyCodes::KEY_LSHIFT ) ||
                                      inputManager->isKeyPressed( KeyCodes::KEY_RSHIFT ) ||
                                      inputManager->isKeyPressed( KeyCodes::KEY_LCONTROL ) ||
                                      inputManager->isKeyPressed( KeyCodes::KEY_RCONTROL ) ) )
                {
                    selectionManager->addSelectedObject( object );
                }
                else
                {
                    selectionManager->clearSelection();
                    selectionManager->addSelectedObject( object );
                }
            }
        }

        auto uiManager = editorManager->getUI();
        if( uiManager )
        {
            uiManager->updateSelection();
        }

        m_nodeSelectTime = timer->now();

        if( auto editorUI = editorManager->getUI() )
        {
            if( auto objectWindow = editorUI->getObjectWindow() )
            {
                objectWindow->updateSelection();
            }
        }
    }

    void SceneWindow::saveTreeState()
    {
        RecursiveMutex::ScopedLock lock( m_treeStateMutex );

        // clear map
        auto treeState = workphone::make_shared<std::map<String, bool>>();
        setTreeState( treeState );

        if( auto tree = getTree() )
        {
            if( auto root = tree->getRoot() )
            {
                if( root->isDerived<ui::IUITreeNode>() )
                {
                    saveItemState( nullptr, root );
                }
            }
        }
    }

    void SceneWindow::saveItemState( SmartPtr<ui::IUITreeNode> parent, SmartPtr<ui::IUITreeNode> node )
    {
        if( !node )
        {
            return;
        }

        auto itemName = getTreeItemStateKey( parent, node );

        // get expanded state
        bool isExpanded = false;

        auto children = node->getChildren();
        if( !children.empty() )
        {
            isExpanded = node->isExpanded();
        }

        // add item to map
        if( !itemName.empty() )
        {
            if( auto p = getTreeState() )
            {
                auto &treeState = *p;
                treeState[itemName] = isExpanded;
            }
        }

        for( auto &child : children )
        {
            if( child->isDerived<ui::IUITreeNode>() )
            {
                saveItemState( node, child );
            }
        }
    }

    void SceneWindow::restoreTreeState()
    {
        RecursiveMutex::ScopedLock lock( m_treeStateMutex );

        if( auto tree = getTree() )
        {
            if( auto root = tree->getRoot() )
            {
                restoreItemState( nullptr, root, false );
            }
        }
    }

    void SceneWindow::restoreItemState( SmartPtr<ui::IUITreeNode> parent, SmartPtr<ui::IUITreeNode> node,
                                        bool parentWasNew )
    {
        if( !node )
        {
            return;
        }

        auto itemName = getTreeItemStateKey( parent, node );

        // get item state from map
        auto state = getItemState( itemName );

        auto isExpanded = false;
        if( state != TREE_ITEM_STATE_NOT_FOUND )
        {
            isExpanded = state == TREE_ITEM_STATE_EXPANDED;
            parentWasNew = false;
        }
        else
        {
            parentWasNew = true;
        }

        node->setExpanded( isExpanded );

        auto children = node->getChildren();
        for( auto &child : children )
        {
            if( child->isDerived<ui::IUITreeNode>() )
            {
                restoreItemState( node, child, parentWasNew );
            }
        }
    }

    String SceneWindow::getTreeItemStateKey( SmartPtr<ui::IUITreeNode> parent,
                                             SmartPtr<ui::IUITreeNode> node ) const
    {
        if( !node )
        {
            return {};
        }

        auto itemKey = String();
        if( auto nodeData =
                workphone::dynamic_pointer_cast<ProjectTreeData>( node->getNodeUserData() ) )
        {
            if( auto ownerData = nodeData->getOwnerData() )
            {
                if( auto handle = ownerData->getHandle() )
                {
                    itemKey = nodeData->getOwnerType() + ":" + handle->getUUIDAsString();
                }
            }
        }

        if( itemKey.empty() )
        {
            itemKey = Util::getText( node );
        }

        if( parent )
        {
            auto parentKey = getTreeItemStateKey( nullptr, parent );
            if( !parentKey.empty() )
            {
                itemKey = parentKey + "/" + itemKey;
            }
        }

        return itemKey;
    }

    void SceneWindow::restoreSelection()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
        auto tree = getTree();
        if( !selectionManager || !tree )
        {
            return;
        }

        tree->clearSelectedTreeNodes();
        if( auto root = tree->getRoot() )
        {
            restoreSelection( root, selectionManager->getSelection() );
        }
    }

    void SceneWindow::restoreSelection( SmartPtr<ui::IUITreeNode> node,
                                        const Array<SmartPtr<ISharedObject>> &selection )
    {
        if( !node )
        {
            return;
        }

        auto isSelected = false;
        if( auto nodeData =
                workphone::dynamic_pointer_cast<ProjectTreeData>( node->getNodeUserData() ) )
        {
            auto objectData = nodeData->getObjectData();
            for( const auto &selected : selection )
            {
                if( objectData && selected && objectData.get() == selected.get() )
                {
                    isSelected = true;
                    break;
                }
            }
        }

        node->setSelected( isSelected );
        for( auto child : node->getChildren() )
        {
            if( auto childNode = workphone::dynamic_pointer_cast<ui::IUITreeNode>( child ) )
            {
                restoreSelection( childNode, selection );
            }
        }
    }

    s32 SceneWindow::getItemState( const String &itemName ) const
    {
        if( auto p = getTreeState() )
        {
            auto &treeState = *p;
            auto it = treeState.find( itemName );
            if( it != treeState.end() )
            {
                auto value = it->second;
                return value ? TREE_ITEM_STATE_EXPANDED : TREE_ITEM_STATE_NOT_EXPANDED;
            }
        }

        return TREE_ITEM_STATE_NOT_FOUND;
    }

    SmartPtr<ISharedObject> SceneWindow::getSelectedObject() const
    {
        return m_selectedObject;
    }

    void SceneWindow::setSelectedObject( SmartPtr<ISharedObject> selectedObject )
    {
        m_selectedObject = selectedObject;
    }

    void SceneWindow::deselectAll()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto timer = applicationManager->getTimer();
        WP_ASSERT( timer );

        auto selectionManager = applicationManager->getSelectionManager();
        WP_ASSERT( selectionManager );

        if( ( m_nodeSelectTime + 0.2 ) < timer->now() )
        {
            if( m_tree )
            {
                m_tree->clearSelectedTreeNodes();
            }

            selectionManager->clearSelection();
        }
    }

    SceneWindow::TreeCtrlListener::TreeCtrlListener() = default;

    SceneWindow::TreeCtrlListener::~TreeCtrlListener() = default;

    Parameter SceneWindow::TreeCtrlListener::handleEvent( EventType eventType, hash_type eventValue,
                                                          const Array<Parameter> &arguments,
                                                          SmartPtr<ISharedObject> sender,
                                                          SmartPtr<ISharedObject> object,
                                                          SmartPtr<IEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto selectionManager = applicationManager ? applicationManager->getSelectionManagerPtr() : nullptr;
        auto jobQueue = applicationManager ? applicationManager->getJobQueuePtr() : nullptr;

        if( auto owner = getOwner(); owner && applicationManager )
        {
            if( eventValue == IEvent::handleToggle )
            {
                if( arguments.size() < 2 || !jobQueue )
                {
                    return {};
                }

                auto node = workphone::static_pointer_cast<ui::IUITreeNode>( arguments[0].object );
                auto toggle = workphone::static_pointer_cast<ui::IUIToggle>( arguments[1].object );
                if( node && toggle )
                {
                    auto data = node->getNodeUserData();
                    if( data )
                    {
                        auto projectTreeData = workphone::static_pointer_cast<ProjectTreeData>( data );
                        auto ownerType = projectTreeData->getOwnerType();
                        if( ownerType == "actor" )
                        {
                            auto actor = workphone::static_pointer_cast<IGameActor>(
                                projectTreeData->getObjectData() );

                            auto job = workphone::make_ptr<ActorEnableJob>();
                            job->setActor( actor );
                            job->setEnable( toggle->isToggled() );

                            jobQueue->addJob( job, TaskId::Application );
                        }
                    }
                }
            }
            else if( eventValue == IEvent::handleTreeSelectionActivated )
            {
                //if( sender == m_owner->m_tree )
                //{
                //    auto node = fb::static_pointer_cast<ui::IUITreeNode>( arguments[0].object );
                //    m_owner->handleTreeSelectionChanged( node );
                //}
            }
            else if( eventValue == IEvent::handleTreeSelectionRelease )
            {
                if( arguments.empty() )
                {
                    return {};
                }

                auto node = workphone::static_pointer_cast<ui::IUITreeNode>( arguments[0].object );
                owner->handleTreeSelectionChanged( node );
            }
            else if( eventValue == IEvent::handleTreeNodeDoubleClicked )
            {
                if( arguments.empty() )
                {
                    return {};
                }

                auto node = workphone::static_pointer_cast<ui::IUITreeNode>( arguments[0].object );

                if( node )
                {
                    if( auto userData = node->getNodeUserData() )
                    {
                        if( auto projectTreeData = workphone::static_pointer_cast<ProjectTreeData>( userData ) )
                        {
                            if( auto actor = projectTreeData->getObjectData() )
                            {
                                if( selectionManager )
                                {
                                    selectionManager->clearSelection();
                                    selectionManager->addSelectedObject( actor );
                                }
                            }
                        }
                    }
                }

                static const auto hashType = StringUtil::getHash( "focus_selection" );
                // SceneWindow opts into global event delivery; the listener itself does not.
                applicationManager->triggerEvent( EventType::UI, hashType, Array<Parameter>(), owner,
                                                  owner, nullptr );
            }
        }

        return {};
    }

    SmartPtr<SceneWindow> SceneWindow::TreeCtrlListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void SceneWindow::TreeCtrlListener::setOwner( SmartPtr<SceneWindow> owner )
    {
        m_owner = owner;
    }

    Parameter SceneWindow::SceneWindowListener::handleEvent( EventType eventType, hash_type eventValue,
                                                             const Array<Parameter> &arguments,
                                                             SmartPtr<ISharedObject> sender,
                                                             SmartPtr<ISharedObject> object,
                                                             SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return {};
            }

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            auto commandManager = applicationManager->getCommandManager();
            auto inputManager = applicationManager->getInputDeviceManager();

            auto eventObject = object ? object : sender;
            auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( eventObject );
            if( eventValue == IEvent::handleValueChanged && element == owner->m_searchEntry )
            {
                auto filter = arguments.empty() ? owner->m_searchEntry->getText() : arguments[0].getStr();
                owner->applySearchFilter( filter );
                return {};
            }

            if( eventValue == IEvent::handleSelection )
            {
                if( !element || !factoryManager || !commandManager )
                {
                    return {};
                }

                auto menuId = static_cast<MenuId>( element->getElementId() );

                switch( menuId )
                {
                case MenuId::ADD_NEW_ENTITY:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Actor );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_SKYBOX:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Skybox );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_NEW_TERRAIN:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Terrain );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_CAMERA:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Camera );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_RENDER_TARGET:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::RenderTarget );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_CAR:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Car );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_HELICOPTER:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Helicopter );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_PLANE:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Plane );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_PARTICLESYSTEM:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::ParticleSystem );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_PARTICLESYSTEM_SMOKE:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::ParticleSystemSmoke );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_PARTICLESYSTEM_SAND:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::ParticleSystemSand );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_PLANE_MESH:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::PlaneMesh );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_CUBE:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Cube );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_CUBE_MESH:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::CubeMesh );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_CUBEMAP:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Cubemap );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_PHYSICS_CUBE:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::PhysicsCube );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_CONSTRAINT:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Constraint );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_DIRECTIONAL_LIGHT:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::DirectionalLight );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_POINT_LIGHT:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::PointLight );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_BUTTON:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Button );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_SIMPLE_BUTTON:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Button );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_CANVAS:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Canvas );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_CHECKBOX:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Checkbox );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_DROPDOWN:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Dropdown );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_PANEL:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Panel );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_SCROLLBAR:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Scrollbar );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_SCROLLBAR_VERTICAL:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::ScrollbarVertical );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_SCROLLVIEW:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Scrollview );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_SLIDER:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Slider );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_SLIDER_VERTICAL:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::SliderVertical );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_TABVIEW:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::TabView );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_TABLELAYOUT:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::TableLayout );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_TEXT:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::Text );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_TOGGLE_BUTTON:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::ToggleButton );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::ADD_TOGGLE_WITH_TEXT:
                {
                    auto cmd = factoryManager->make_ptr<AddActorCmd>();
                    cmd->setActorType( AddActorCmd::ActorType::ToggleWithText );
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::SCENE_REMOVE_ACTOR:
                {
                    auto cmd = factoryManager->make_ptr<RemoveSelectionCmd>();
                    commandManager->addCommand( cmd );
                }
                break;
                case MenuId::SCENE_REFRESH:
                {
                    owner->buildTree();
                }
                break;
                case MenuId::SEND_PROMPT:
                {
                    if( !owner->m_inputText )
                    {
                        break;
                    }

                    auto str = owner->m_inputText->getText();
                    if( StringUtil::isNullOrEmpty( str ) )
                    {
                        break;
                    }

                    auto promptCmd = factoryManager->make_ptr<PromptCmd>();
                    promptCmd->setPrompt( str );
                    commandManager->addCommand( promptCmd );
                }
                break;
                default:
                {
                }
                }
            }
            else if( eventValue == IEvent::handleMouseClicked )
            {
                if( inputManager && ( inputManager->isKeyPressed( KeyCodes::KEY_LSHIFT ) ||
                                      inputManager->isKeyPressed( KeyCodes::KEY_RSHIFT ) ||
                                      inputManager->isKeyPressed( KeyCodes::KEY_LCONTROL ) ||
                                      inputManager->isKeyPressed( KeyCodes::KEY_RCONTROL ) ) )
                {
                }
                else
                {
                    owner->deselectAll();
                }
            }
            else if( eventValue == IComponent::childAdded || eventValue == IEvent::sceneChanged )
            {
                owner->buildTree();
            }
        }

        return {};
    }

    SmartPtr<SceneWindow> SceneWindow::SceneWindowListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void SceneWindow::SceneWindowListener::setOwner( SmartPtr<SceneWindow> owner )
    {
        m_owner = owner;
    }

    SceneWindow::SceneWindowListener::SceneWindowListener() = default;

    SceneWindow::SceneWindowListener::~SceneWindowListener() = default;

    Parameter SceneWindow::DropTarget::handleEvent( EventType eventType, hash_type eventValue,
                                                    const Array<Parameter> &arguments,
                                                    SmartPtr<ISharedObject> sender,
                                                    SmartPtr<ISharedObject> object,
                                                    SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleDrop )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto factoryManager = applicationManager ? applicationManager->getFactoryManagerPtr() : nullptr;
                auto jobQueue = applicationManager ? applicationManager->getJobQueuePtr() : nullptr;
                if( !factoryManager || !jobQueue || arguments.empty() )
                {
                    return {};
                }

                const auto &text = arguments[0].str;
                // Tree-node drops include a sibling index, but drops onto the scene window only
                // contain the drag payload. Use -1 for those drops so SceneDropJob appends the
                // actor instead of discarding the prefab drop.
                auto siblingIndex = arguments.size() > 1 ? arguments[1].getS32() : -1;

                auto owner = getOwner();
                if( !owner )
                {
                    return {};
                }

                auto tree = owner->m_tree;

                if( !owner->getDropJob() )
                {
                    auto dropJob = factoryManager->make_ptr<SceneDropJob>();
                    auto ownerWeak = WeakPtr<SceneWindow>( owner );
                    auto dropJobPtr = dropJob.get();

                    dropJob->setOwner( owner );
                    dropJob->setData( text );
                    dropJob->setSiblingIndex( siblingIndex );
                    dropJob->setTree( tree );
                    dropJob->setSender( sender );

                    dropJob->setCallbackFunction( [ownerWeak, dropJobPtr]( int state ) -> void {
                        if( state == static_cast<s32>( IJob::State::Finish ) )
                        {
                            if( auto owner = ownerWeak.lock() )
                            {
                                auto activeDropJob = owner->getDropJob();
                                if( activeDropJob && activeDropJob.get() == dropJobPtr )
                                {
                                    owner->setDropJob( nullptr );
                                }
                            }
                        }
                    } );

                    owner->setDropJob( dropJob );
                    jobQueue->addJob( dropJob );
                    if( dropJob->isFinished() )
                    {
                        if( owner->getDropJob() == dropJob )
                        {
                            owner->setDropJob( nullptr );
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        return {};
    }

    SmartPtr<SceneWindow> SceneWindow::DropTarget::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void SceneWindow::DropTarget::setOwner( SmartPtr<SceneWindow> owner )
    {
        m_owner = owner;
    }

    SceneWindow::DropTarget::DropTarget() = default;

    SceneWindow::DropTarget::~DropTarget() = default;

    Parameter SceneWindow::DragSource::handleEvent( EventType eventType, hash_type eventValue,
                                                    const Array<Parameter> &arguments,
                                                    SmartPtr<ISharedObject> sender,
                                                    SmartPtr<ISharedObject> object,
                                                    SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleDrag )
        {
            auto dataStr = handleDrag( Vector2I::zero(), sender );
            return Parameter( dataStr );
        }

        return {};
    }

    String SceneWindow::DragSource::handleDrag( const Vector2I &position,
                                                SmartPtr<ui::IUIElement> element )
    {
        if( element && element->isDerived<ui::IUITreeNode>() )
        {
            auto treeNode = workphone::static_pointer_cast<ui::IUITreeNode>( element );
            auto text = Util::getText( treeNode );

            auto userData = treeNode->getNodeUserData();
            if( userData )
            {
                auto projectTreeData = workphone::static_pointer_cast<ProjectTreeData>( userData );
                WP_ASSERT( projectTreeData );

                auto data = workphone::make_ptr<Properties>();

                data->setProperty( "sourceId", treeNode->getTreeNodeId() );

                auto treeData =
                    workphone::static_pointer_cast<ProjectTreeData>( treeNode->getNodeUserData() );
                auto actor = treeData->getObjectData();

                if( actor )
                {
                    if( auto handle = actor->getHandle() )
                    {
                        auto uuid = handle->getUUIDAsString();
                        data->setProperty( "actorId", handle->getInstanceId() );
                        data->setProperty( "actorUUID", uuid );
                    }
                }

                return DataUtil::toString( data.get(), true );
            }
        }

        return {};
    }

    SmartPtr<SceneWindow> SceneWindow::DragSource::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void SceneWindow::DragSource::setOwner( SmartPtr<SceneWindow> owner )
    {
        m_owner = owner;
    }

    SceneWindow::DragSource::DragSource() = default;

    SceneWindow::DragSource::~DragSource() = default;

    bool SceneWindow::isValid() const
    {
        return getLoadingState() == LoadingState::Loaded && getParentWindow() && m_sceneWindow &&
               m_tree && m_treeListener && m_menuListener;
    }

    SmartPtr<ui::IUITreeCtrl> SceneWindow::getTree() const
    {
        return m_tree;
    }

    void SceneWindow::setTree( SmartPtr<ui::IUITreeCtrl> tree )
    {
        m_tree = tree;
    }

    SharedPtr<std::map<String, bool>> SceneWindow::getTreeState() const
    {
        return m_treeState;
    }

    void SceneWindow::setTreeState( SharedPtr<std::map<String, bool>> treeState )
    {
        m_treeState = treeState;
    }

    SmartPtr<ICommand> SceneWindow::getDragDropActorCmd() const
    {
        return m_dragDropActorCmd;
    }

    void SceneWindow::setDragDropActorCmd( SmartPtr<ICommand> dragDropActorCmd )
    {
        m_dragDropActorCmd = dragDropActorCmd;
    }

    void SceneWindow::setDropJob( SmartPtr<IJob> dropJob )
    {
        m_dropJob = dropJob;
    }

    SmartPtr<IJob> SceneWindow::getDropJob() const
    {
        return m_dropJob;
    }

    void SceneWindow::setBuildJob( SmartPtr<IJob> buildJob )
    {
        m_buildJob = buildJob;
    }

    SmartPtr<IJob> SceneWindow::getBuildJob() const
    {
        return m_buildJob;
    }

    SceneWindow::PromptListener::PromptListener() = default;

    SceneWindow::PromptListener::~PromptListener() = default;

    Parameter SceneWindow::PromptListener::handleEvent( EventType eventType, hash_type eventValue,
                                                        const Array<Parameter> &arguments,
                                                        SmartPtr<ISharedObject> sender,
                                                        SmartPtr<ISharedObject> object,
                                                        SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handlePropertyChanged )
        {
            if( arguments.empty() )
            {
                return {};
            }

            auto str = arguments[0].str;
            if( StringUtil::isNullOrEmpty( str ) )
            {
                return {};
            }

            if( auto owner = getOwner() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto factoryManager = applicationManager ? applicationManager->getFactoryManagerPtr() : nullptr;
                auto commandManager = applicationManager ? applicationManager->getCommandManagerPtr() : nullptr;
                if( !factoryManager || !commandManager )
                {
                    return {};
                }

                auto promptCmd = factoryManager->make_ptr<PromptCmd>();
                promptCmd->setPrompt( str );
                commandManager->addCommand( promptCmd );
            }
        }

        return {};
    }

    SmartPtr<SceneWindow> SceneWindow::PromptListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void SceneWindow::PromptListener::setOwner( SmartPtr<SceneWindow> owner )
    {
        m_owner = owner;
    }

    SceneWindow::ApplicationEventListener::ApplicationEventListener() = default;

    SceneWindow::ApplicationEventListener::~ApplicationEventListener() = default;

    void SceneWindow::ApplicationEventListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
        setLoadingState( LoadingState::Unloaded );
    }

    Parameter SceneWindow::ApplicationEventListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto task = Thread::getCurrentTask();
        if( task == TaskId::Primary )
        {
            if( auto owner = getOwner() )
            {
                if( eventValue == IEvent::addActor )
                {
                    owner->buildTree();
                }
                else if( eventValue == IEvent::sceneChanged )
                {
                    owner->buildTree();
                }
            }
        }

        return {};
    }

    SmartPtr<SceneWindow> SceneWindow::ApplicationEventListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void SceneWindow::ApplicationEventListener::setOwner( SmartPtr<SceneWindow> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::editor
