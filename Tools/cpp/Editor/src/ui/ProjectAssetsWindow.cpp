#include <EditorPCH.hpp>
#include <ui/ProjectAssetsWindow.hpp>
#include <commands/AddResourceCmd.hpp>
#include <commands/DragDropActorCmd.hpp>
#include <commands/AddActorCmd.hpp>
#include <commands/AddNewScriptCmd.hpp>
#include <commands/RemoveResourceCmd.hpp>
#include <ui/ObjectWindow.hpp>
#include <ui/FileWindow.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <jobs/FileSelectedJob.hpp>
#include <Workphone/Workphone.hpp>

#include <set>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, ProjectAssetsWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone, ProjectAssetsWindow::BuildTreeJob, Job );
    WP_CLASS_REGISTER_DERIVED( workphone, ProjectAssetsWindow::TreeCtrlListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone, ProjectAssetsWindow::WindowListener, IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone, ProjectAssetsWindow::DragSource, ui::IUIDragSource );
    WP_CLASS_REGISTER_DERIVED( workphone, ProjectAssetsWindow::DropTarget, ui::IUIDropTarget );

    const String ProjectAssetsWindow::fileExt = String( "file" );

    ProjectAssetsWindow::ProjectAssetsWindow()
    {
    }

    ProjectAssetsWindow::~ProjectAssetsWindow()
    {
        try
        {
            unload( nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ProjectAssetsWindow::load( SmartPtr<ISharedObject> data )
    {
        auto shouldBuild = false;

        try
        {
            {
                ScopedLock lock( this );

                setLoadingState( LoadingState::Loading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto factoryManager = applicationManager->getFactoryManager();

                auto ui = applicationManager->getUI();
                WP_ASSERT( ui );

                auto parent = getParent();

                auto parentWindow = ui->addElementByType<ui::IUIWindow>();
                parentWindow->setLabel( "Assets" );
                parentWindow->setHasBorder( true );
                setParentWindow( parentWindow );

                if( parent )
                {
                    parent->addChild( parentWindow );
                }

                auto listener = workphone::make_ptr<WindowListener>();
                listener->setOwner( this );
                m_menuListener = listener;

                auto addNavigationButton = [&]( const String &label, MenuId menuId, bool sameLine ) {
                    auto button = ui->addElementByType<ui::IUIButton>();
                    button->setLabel( label );
                    button->setSameLine( sameLine );
                    button->setElementId( static_cast<s32>( menuId ) );
                    button->addObjectListener( listener );
                    parentWindow->addChild( button );
                    return button;
                };

                m_backButton = addNavigationButton( ICON_FA_ARROW_LEFT "##AssetsBack",
                                                    MenuId::NavigateBack, false );
                m_forwardButton = addNavigationButton( ICON_FA_ARROW_RIGHT "##AssetsForward",
                                                       MenuId::NavigateForward, true );
                m_upButton =
                    addNavigationButton( ICON_FA_ARROW_UP "##AssetsUp", MenuId::NavigateUp, true );
                m_rootButton =
                    addNavigationButton( ICON_FA_HOUSE "##AssetsRoot", MenuId::NavigateRoot, true );
                m_refreshButton =
                    addNavigationButton( ICON_FA_ROTATE "##AssetsRefresh", MenuId::Refresh, true );

                m_breadcrumbText = ui->addElementByType<ui::IUIText>();
                m_breadcrumbText->setText( ICON_FA_FOLDER_OPEN " Assets" );
                //Util::setText( m_breadcrumbText, ICON_FA_FOLDER_OPEN " Assets" );
                parentWindow->addChild( m_breadcrumbText );

                m_searchEntry = ui->addElementByType<ui::IUITextEntry>();
                m_searchEntry->setLabel( ICON_FA_MAGNIFYING_GLASS " Search##AssetSearch" );
                m_searchEntry->setPlaceholder( "Filter by name, type, or path..." );
                m_searchEntry->setText( "" );
                m_searchEntry->addObjectListener( listener );
                parentWindow->addChild( m_searchEntry );

                m_thumbnailCaption = ui->addElementByType<ui::IUIText>();
                m_thumbnailCaption->setText( "" );
                //Util::setText( m_thumbnailCaption, "" );
                m_thumbnailCaption->setVisible( false );
                parentWindow->addChild( m_thumbnailCaption );

                m_thumbnailPreview = ui->addElementByType<ui::IUIImage>();
                m_thumbnailPreview->setSize( Vector2F( 160.0f, 100.0f ) );
                m_thumbnailPreview->setVisible( false );
                parentWindow->addChild( m_thumbnailPreview );

                auto treeCtrl = ui->addElementByType<ui::IUITreeCtrl>();
                parentWindow->addChild( treeCtrl );
                m_tree = treeCtrl;

                auto dragSource = workphone::make_ptr<DragSource>();
                dragSource->setOwner( this );
                treeCtrl->setDragSource( dragSource );

                auto dropTarget = workphone::make_ptr<DropTarget>();
                dropTarget->setOwner( this );
                parentWindow->setDropTarget( dropTarget );
                m_tree->setDropTarget( dropTarget );

                auto treeListener = workphone::make_ptr<TreeCtrlListener>();
                treeListener->setOwner( this );
                m_treeListener = treeListener;

                m_tree->addObjectListener( treeListener );

                m_applicationMenu = ui->addElementByType<ui::IUIMenu>();
                m_applicationMenu->setLabel( "Assets" );

                m_applicationAddMenu = ui->addElementByType<ui::IUIMenu>();
                m_applicationAddMenu->setLabel( "Add" );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::AddLightingPreset ),
                                   "Lighting Preset", "Creates a lighting preset file." );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::AddMaterial ),
                                   "Material", "Material" );

                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::AddScript ), "Script",
                                   "Add a script" );

                auto factories = factoryManager->getFactories();
                for( auto &factory : factories )
                {
                    if( factory->isObjectDerivedFrom<IBuildDirector>() )
                    {
                        if( factory->isObjectDerivedFrom<scene::ResourceDirector>() )
                        {
                            continue;
                        }

                        auto factoryName = String( factory->getObjectTypeName() );
                        auto menuItem = Util::addMenuItem(
                            m_applicationAddMenu, static_cast<s32>( MenuId::AddDirector ), factoryName,
                            String( "Creates a " ) + factoryName + " director" );
                        menuItem->setUserData( factory.get() );
                    }
                }

                Util::addMenuSeparator( m_applicationAddMenu );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::AddTerrainDirector ),
                                   "Terrain Resource", "Terrain Resource" );
                Util::addMenuItem( m_applicationAddMenu, static_cast<s32>( MenuId::AddFolder ), "Folder",
                                   "Create a new asset folder" );

                m_applicationMenu->addMenuItem( m_applicationAddMenu );

                Util::addMenuSeparator( m_applicationMenu );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::Cut ), "Cut",
                                   "Move this asset when Paste is selected" );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::Copy ), "Copy",
                                   "Copy this asset to the asset clipboard" );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::Paste ), "Paste",
                                   "Paste into the selected folder" );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::Duplicate ), "Duplicate",
                                   "Duplicate this asset" );
                Util::addMenuSeparator( m_applicationMenu );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::Remove ), "Delete",
                                   "Delete this asset" );
                Util::addMenuSeparator( m_applicationMenu );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::Import ), "Import",
                                   "Import" );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::Reimport ), "Reimport",
                                   "Reimport" );
                Util::addMenuSeparator( m_applicationMenu );
                Util::addMenuItem( m_applicationMenu, static_cast<s32>( MenuId::Refresh ), "Refresh",
                                   "Refresh" );

                parentWindow->setContextMenu( m_applicationMenu );

                m_applicationMenu->addObjectListener( m_menuListener );

                setupListeners();
                setupApplicationListeners();

                setLoadingState( LoadingState::Loaded );
                shouldBuild = true;
            }

            if( shouldBuild )
            {
                build();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ProjectAssetsWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto unloadedState = LoadingState::Unloaded;

            auto loadingState = getLoadingState();
            if( loadingState != unloadedState )
            {
                setLoadingState( LoadingState::Unloading );

                if( auto buildTreeJob = getBuildTreeJob() )
                {
                    buildTreeJob->setInterrupted( true );
                    setBuildTreeJob( nullptr );
                }

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto ui = applicationManager->getUI();
                WP_ASSERT( ui );

                if( m_tree )
                {
                    if( m_treeListener )
                    {
                        m_tree->removeObjectListener( m_treeListener );
                    }

                    ui->removeElement( m_tree );
                    m_tree = nullptr;
                }

                if( m_treeListener )
                {
                    m_treeListener->unload( nullptr );
                    m_treeListener = nullptr;
                }

                if( m_applicationMenu )
                {
                    ui->removeElement( m_applicationMenu );
                    m_applicationMenu = nullptr;
                }

                if( auto parentWindow = getParentWindow() )
                {
                    parentWindow->setContextMenu( nullptr );
                    ui->removeElement( parentWindow );
                    setParentWindow( nullptr );
                }

                m_backButton = nullptr;
                m_forwardButton = nullptr;
                m_upButton = nullptr;
                m_rootButton = nullptr;
                m_refreshButton = nullptr;
                m_searchEntry = nullptr;
                m_breadcrumbText = nullptr;
                if( m_thumbnailPreview )
                {
                    m_thumbnailPreview->setTexture( nullptr );
                    m_thumbnailPreview = nullptr;
                }
                m_thumbnailCaption = nullptr;
                m_navigationHistory.clear();
                m_navigationHistoryIndex = -1;
                m_currentFolder.clear();
                m_searchFilter.clear();

                auto dataArray = getData();
                for( auto data : dataArray )
                {
                    data->unload( nullptr );
                }

                clearData();

                EditorWindow::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<ui::IUITreeNode> ProjectAssetsWindow::addFileToTree( SmartPtr<ui::IUITreeNode> parent,
                                                                  const String &filePath )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager || !parent )
        {
            return nullptr;
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        auto tree = getTree();
        if( !factoryManager || !tree )
        {
            return nullptr;
        }

        auto projectFolder = applicationManager->getProjectPath();
        auto path = StringUtil::cleanupPath( filePath );
        if( Path::isPathAbsolute( path ) )
        {
            path = Path::getRelativePath( projectFolder, path );
        }

        auto data = factoryManager->make_ptr<ProjectTreeData>( fileExt, path, nullptr, nullptr );
        if( auto fileNode = tree->addNode() )
        {
            Util::setText( fileNode, getAssetDisplayName( path, false ) );
            fileNode->setNodeUserData( data );
            parent->addChild( fileNode );
            addData( data );
            return fileNode;
        }

        return nullptr;
    }

    SmartPtr<ui::IUITreeNode> ProjectAssetsWindow::addFolderToTree( SmartPtr<ui::IUITreeNode> parent,
                                                                    SmartPtr<IFolderExplorer> listing,
                                                                    SmartPtr<IJob> buildTreeJob )
    {
        try
        {
            if( !listing )
            {
                return nullptr;
            }

            auto activeBuildTreeJob = buildTreeJob ? buildTreeJob : getBuildTreeJob();
            if( activeBuildTreeJob && activeBuildTreeJob->isInterrupted() )
            {
                return nullptr;
            }

            if( auto tree = getTree() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                if( !applicationManager )
                {
                    return nullptr;
                }

                if( !applicationManager->isRunning() )
                {
                    return nullptr;
                }

                if( applicationManager->getQuit() )
                {
                    return nullptr;
                }

                auto factoryManager = applicationManager->getFactoryManagerPtr();
                if( !factoryManager )
                {
                    return nullptr;
                }

                auto fileSystem = applicationManager->getFileSystemPtr();

                auto folderPath = listing->getFolderName();

                auto editorManager = EditorManager::getSingletonPtr();
                if( !editorManager )
                {
                    return nullptr;
                }

                auto project = editorManager->getProject();

                static const String folderStr = String( "folder" );
                auto data =
                    factoryManager->make_ptr<ProjectTreeData>( folderStr, folderPath, project, project );

                auto folderName = Path::getFileName( folderPath );

                if( auto node = tree->addNode() )
                {
                    Util::setText( node, getAssetDisplayName( folderName, true ) );

                    node->setNodeUserData( data );

                    if( parent )
                    {
                        parent->addChild( node );
                    }

                    auto projectFolder = applicationManager->getProjectPath();
                    if( StringUtil::isNullOrEmpty( projectFolder ) )
                    {
                        projectFolder = Path::getWorkingDirectory();
                    }

                    auto subFolders = listing->getSubFolders();
                    std::sort( subFolders.begin(), subFolders.end(),
                               []( const auto &left, const auto &right ) {
                                   return StringUtil::make_lower( left->getFolderName() ) <
                                          StringUtil::make_lower( right->getFolderName() );
                               } );
                    for( auto &subFolder : subFolders )
                    {
                        addFolderToTree( node, subFolder, activeBuildTreeJob );

                        if( activeBuildTreeJob && activeBuildTreeJob->isInterrupted() )
                        {
                            return nullptr;
                        }
                    }

                    auto files = listing->getFiles();
                    std::sort( files.begin(), files.end(), []( const auto &left, const auto &right ) {
                        return StringUtil::make_lower( left ) < StringUtil::make_lower( right );
                    } );
                    for( const auto &file : files )
                    {
                        addFileToTree( node, file );

                        if( activeBuildTreeJob && activeBuildTreeJob->isInterrupted() )
                        {
                            return nullptr;
                        }
                    }

                    return node;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<ui::IUITreeNode> ProjectAssetsWindow::addMediaFolderToTree(
        SmartPtr<ui::IUITreeNode> parent, SmartPtr<IFolderExplorer> listing,
        SmartPtr<IJob> buildTreeJob )
    {
        try
        {
            if( !listing )
            {
                return nullptr;
            }

            auto activeBuildTreeJob = buildTreeJob ? buildTreeJob : getBuildTreeJob();
            if( activeBuildTreeJob && activeBuildTreeJob->isInterrupted() )
            {
                return nullptr;
            }

            if( auto tree = getTree() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                if( !applicationManager )
                {
                    return nullptr;
                }

                if( !applicationManager->isRunning() )
                {
                    return nullptr;
                }

                if( applicationManager->getQuit() )
                {
                    return nullptr;
                }

                auto factoryManager = applicationManager->getFactoryManager();
                if( !factoryManager )
                {
                    return nullptr;
                }

                auto fileSystem = applicationManager->getFileSystem();

                auto folderPath = listing->getFolderName();

                auto editorManager = EditorManager::getSingletonPtr();
                if( !editorManager )
                {
                    return nullptr;
                }

                auto project = editorManager->getProject();
                auto data =
                    factoryManager->make_ptr<ProjectTreeData>( "folder", folderPath, project, project );

                auto folderName = Path::getFileName( folderPath );

                if( auto node = tree->addNode() )
                {
                    Util::setText( node, getAssetDisplayName( folderName, true ) );

                    node->setNodeUserData( data );

                    if( parent )
                    {
                        parent->addChild( node );
                    }

                    auto projectFolder = applicationManager->getProjectPath();
                    auto subFolders = listing->getSubFolders();
                    std::sort( subFolders.begin(), subFolders.end(),
                               []( const auto &left, const auto &right ) {
                                   return StringUtil::make_lower( left->getFolderName() ) <
                                          StringUtil::make_lower( right->getFolderName() );
                               } );
                    for( auto &subFolder : subFolders )
                    {
                        addMediaFolderToTree( node, subFolder, activeBuildTreeJob );

                        if( activeBuildTreeJob && activeBuildTreeJob->isInterrupted() )
                        {
                            return nullptr;
                        }
                    }

                    auto files = listing->getFiles();
                    std::sort( files.begin(), files.end(), []( const auto &left, const auto &right ) {
                        return StringUtil::make_lower( left ) < StringUtil::make_lower( right );
                    } );
                    for( const auto &file : files )
                    {
                        auto filePath = StringUtil::cleanupPath( file );
                        filePath = Path::getRelativePath( projectFolder, filePath );
                        auto pathHash = StringUtil::getUUID( filePath );

                        //auto pFileData = factoryManager->make_ptr<Data<FileInfo>>();

                        //FileInfo fileInfo;
                        //if( fileSystem->findFileInfo( pathHash, fileInfo ) )
                        //{
                        //    pFileData->setData( &fileInfo );
                        //}

                        auto data = factoryManager->make_ptr<ProjectTreeData>( "file", filePath, nullptr,
                                                                               nullptr );

                        auto fileName = Path::getFileName( filePath );

                        if( auto fileNode = tree->addNode() )
                        {
                            Util::setText( fileNode, getAssetDisplayName( fileName, false ) );

                            fileNode->setNodeUserData( data );

                            node->addChild( fileNode );
                        }

                        addData( data );

                        if( activeBuildTreeJob && activeBuildTreeJob->isInterrupted() )
                        {
                            return nullptr;
                        }
                    }

                    return node;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void ProjectAssetsWindow::build()
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager || applicationManager->getQuit() || !applicationManager->isRunning() )
        {
            return;
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        auto jobQueue = applicationManager->getJobQueuePtr();
        if( !factoryManager || !jobQueue || !jobQueue->isRunning() )
        {
            return;
        }

        if( auto existingBuildTreeJob = getBuildTreeJob() )
        {
            existingBuildTreeJob->setInterrupted( true );
            setBuildTreeJob( nullptr );
        }

        auto job = factoryManager->make_ptr<BuildTreeJob>();
        job->setPrimary( true );

        auto owner = WeakPtr<ProjectAssetsWindow>( this );
        auto jobPtr = job.get();

        job->setCallbackFunction( [owner, jobPtr]( int state ) {
            if( state == static_cast<int>( Job::State::Finish ) )
            {
                if( auto ownerPtr = owner.lock() )
                {
                    auto buildTreeJob = ownerPtr->getBuildTreeJob();
                    if( buildTreeJob && buildTreeJob.get() == jobPtr )
                    {
                        ownerPtr->setBuildTreeJob( nullptr );
                    }
                }
            }
        } );

        job->setOwner( this );
        setBuildTreeJob( job );

        jobQueue->addJob( job );
        if( job->isFinished() )
        {
            if( getBuildTreeJob() == job )
            {
                setBuildTreeJob( nullptr );
            }
        }
    }

    void ProjectAssetsWindow::buildTree( SmartPtr<IJob> buildTreeJob )
    {
        try
        {
            ScopedLock lock( this );

            if( buildTreeJob && buildTreeJob->isInterrupted() )
            {
                return;
            }

            if( auto tree = getTree() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                if( !applicationManager )
                {
                    return;
                }

                if( !applicationManager->isRunning() )
                {
                    return;
                }

                if( applicationManager->getQuit() )
                {
                    return;
                }

                auto projectFolder = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectFolder ) )
                {
                    WP_LOG_ERROR( "ProjectAssetsWindow::buildTree: Project path is empty" );
                    return;
                }

                auto fileSystem = applicationManager->getFileSystemPtr();
                if( !fileSystem )
                {
                    return;
                }

                auto editorManager = EditorManager::getSingletonPtr();
                if( !editorManager )
                {
                    return;
                }

                auto project = editorManager->getProject();

                if( StringUtil::isNullOrEmpty( projectFolder ) && project )
                {
                    projectFolder = Path::getFilePath( project->getPath() );
                }

                if( StringUtil::isNullOrEmpty( projectFolder ) )
                {
                    return;
                }

                projectFolder = StringUtil::cleanupPath( projectFolder );
                if( !Path::isPathAbsolute( projectFolder ) )
                {
                    projectFolder = Path::lexically_normal( Path::getWorkingDirectory(), projectFolder );
                }

                if( !fileSystem->isExistingFolder( projectFolder ) )
                {
                    return;
                }

                auto assetsFolder = Path::lexically_normal( projectFolder, "Assets" );
                if( !fileSystem->isExistingFolder( assetsFolder ) )
                {
                    assetsFolder = projectFolder;
                }

                if( buildTreeJob && buildTreeJob->isInterrupted() )
                {
                    return;
                }

                auto folderListing = fileSystem->getFolderListing( assetsFolder );
                if( !folderListing )
                {
                    return;
                }

                auto folderToRestore = m_currentFolder;
                if( StringUtil::isNullOrEmpty( folderToRestore ) ||
                    !fileSystem->isExistingFolder( folderToRestore ) )
                {
                    folderToRestore = assetsFolder;
                }

                tree->clear();
                clearData();

                auto rootNode = tree->addRoot();
                WP_ASSERT( rootNode );
                if( !rootNode )
                {
                    return;
                }

                Util::setText( rootNode, ICON_FA_DATABASE " Project Content" );
                rootNode->setExpanded( true );

                auto node = addFolderToTree( rootNode, folderListing, buildTreeJob );
                if( node )
                {
                    node->setExpanded( true );
                }

                if( editorManager->getShowDebug() )
                {
                    auto mediaFolder = applicationManager->getMediaPath();
                    if( !StringUtil::isNullOrEmpty( mediaFolder ) )
                    {
                        if( !Path::isPathAbsolute( mediaFolder ) )
                        {
                            mediaFolder =
                                Path::lexically_normal( Path::getWorkingDirectory(), mediaFolder );
                        }

                        if( fileSystem->isExistingFolder( mediaFolder ) )
                        {
                            if( auto folderListing = fileSystem->getFolderListing( mediaFolder ) )
                            {
                                auto node =
                                    addMediaFolderToTree( rootNode, folderListing, buildTreeJob );
                                if( node )
                                {
                                    node->setExpanded( false );
                                }
                            }
                        }
                    }
                }

                tree->expand( rootNode );

                m_currentFolder = Path::lexically_normal( folderToRestore );
                if( auto selectedFolderNode = findNodeByPath( m_currentFolder ) )
                {
                    tree->setSelectedTreeNode( selectedFolderNode );
                    tree->expand( selectedFolderNode );
                }

                if( m_navigationHistory.empty() )
                {
                    m_navigationHistory.push_back( m_currentFolder );
                    m_navigationHistoryIndex = 0;
                }

                updateNavigationControls();
                applySearchFilter( m_searchFilter );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ProjectAssetsWindow::handleTreeSelectionActivated( SmartPtr<ui::IUITreeNode> node )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto selectionManager = applicationManager->getSelectionManagerPtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();
        auto fileSystem = applicationManager->getFileSystemPtr();
        auto jobQueue = applicationManager->getJobQueuePtr();

        auto editorManager = EditorManager::getSingletonPtr();
        auto project = editorManager->getProject();

        auto data = node->getNodeUserData();
        if( data )
        {
            auto projectTreeData = workphone::static_ptr_cast<ProjectTreeData>( data );
            const auto ownerType = projectTreeData->getOwnerType();

            if( ownerType == fileExt )
            {
                const auto path = projectTreeData->getObjectType();

                auto fileSelection = factoryManager->make_ptr<FileSelection>();
                fileSelection->setFilePath( path );

                auto pObjectData = projectTreeData->getObjectData();
                auto objectData = workphone::static_pointer_cast<IData>( pObjectData );
                if( !objectData )
                {
                    auto pathHash = StringUtil::getUUID( path );

                    objectData = factoryManager->make_ptr<Data<FileInfo>>();

                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( pathHash, fileInfo ) )
                    {
                        objectData->setData( &fileInfo );
                    }
                }

                if( objectData )
                {
                    auto fileInfo = objectData->getDataAsType<FileInfo>();
                    if( fileInfo )
                    {
                        fileSelection->setFileInfo( *fileInfo );
                    }
                }

                selectionManager->clearSelection();
                selectionManager->addSelectedObject( fileSelection );
            }
        }

        auto ui = editorManager->getUI();
        WP_ASSERT( ui );

        ui->updateSelection();

        jobQueue->startJob( []() {
            auto applicationManager = core::IApplicationManager::instancePtr();

            auto editorManager = EditorManager::getSingletonPtr();
            auto editorUI = editorManager->getUI();

            if( auto objectWindow = editorUI->getObjectWindow() )
            {
                objectWindow->updateSelection();
            }
        } );
    }

    void ProjectAssetsWindow::handleTreeNodeDoubleClicked( SmartPtr<ui::IUITreeNode> node )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto taskManager = applicationManager->getTaskManager();
        auto factoryManager = applicationManager->getFactoryManager();

        auto jobQueue = applicationManager->getJobQueue();

        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto project = editorManager->getProject();
        WP_ASSERT( project );

        auto uiManager = editorManager->getUI();
        WP_ASSERT( uiManager );

        auto data = node->getNodeUserData();
        if( data )
        {
            auto projectTreeData = workphone::static_pointer_cast<ProjectTreeData>( data );
            auto ownerType = projectTreeData->getOwnerType();

            if( ownerType == fileExt )
            {
                auto path = projectTreeData->getObjectType();

                auto job = factoryManager->make_ptr<FileSelectedJob>();
                job->setFilePath( path );
                jobQueue->addJob( job );
            }
            else if( ownerType == "folder" )
            {
                // ImGui already toggled the folder on double-click.
                navigateToFolder( projectTreeData->getObjectType(), true, false );
            }
        }
    }

    SmartPtr<ui::IUIWindow> ProjectAssetsWindow::getParentWindow() const
    {
        return m_parentWindow;
    }

    void ProjectAssetsWindow::setParentWindow( SmartPtr<ui::IUIWindow> parentWindow )
    {
        m_parentWindow = parentWindow;
    }

    String ProjectAssetsWindow::getSelectedPath() const
    {
        if( auto tree = getTree() )
        {
            auto node = tree->getSelectedTreeNode();
            if( node )
            {
                auto userData = node->getNodeUserData();
                auto projectTreeData = workphone::static_pointer_cast<ProjectTreeData>( userData );

                if( projectTreeData )
                {
                    return projectTreeData->getObjectType();
                }
            }
        }

        return {};
    }

    String ProjectAssetsWindow::getSelectedFolderPath() const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return {};
        }

        if( auto tree = getTree() )
        {
            if( auto node = tree->getSelectedTreeNode() )
            {
                auto treeData =
                    workphone::dynamic_pointer_cast<ProjectTreeData>( node->getNodeUserData() );
                if( treeData )
                {
                    auto path = treeData->getObjectType();
                    if( treeData->getOwnerType() == fileExt )
                    {
                        path = Path::getFilePath( path );
                    }

                    return Path::isPathAbsolute( path )
                               ? Path::lexically_normal( path )
                               : Path::lexically_normal( applicationManager->getProjectPath(), path );
                }
            }
        }

        auto projectPath = applicationManager->getProjectPath();
        auto assetsPath = Path::lexically_normal( projectPath, "Assets" );
        auto fileSystem = applicationManager->getFileSystemPtr();
        return fileSystem && fileSystem->isExistingFolder( assetsPath ) ? assetsPath : projectPath;
    }

    bool ProjectAssetsWindow::copyAsset( const String &sourcePath, const String &targetFolder,
                                         bool generateUniqueName )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return false;
        }

        auto fileSystem = applicationManager->getFileSystemPtr();
        if( !fileSystem || StringUtil::isNullOrEmpty( sourcePath ) ||
            StringUtil::isNullOrEmpty( targetFolder ) )
        {
            return false;
        }

        auto projectPath = applicationManager->getProjectPath();
        auto source = Path::isPathAbsolute( sourcePath )
                          ? Path::lexically_normal( sourcePath )
                          : Path::lexically_normal( projectPath, sourcePath );
        auto target = Path::isPathAbsolute( targetFolder )
                          ? Path::lexically_normal( targetFolder )
                          : Path::lexically_normal( projectPath, targetFolder );

        auto assetsRoot = Path::lexically_normal( projectPath, "Assets" );
        if( !fileSystem->isExistingFolder( assetsRoot ) )
        {
            assetsRoot = Path::lexically_normal( projectPath );
        }

        auto isInAssets = [&assetsRoot]( const String &path ) {
            auto root = StringUtil::make_lower( StringUtil::cleanupPath( assetsRoot ) );
            auto candidate = StringUtil::make_lower( StringUtil::cleanupPath( path ) );
            auto prefix = root + "/";
            return candidate == root || ( candidate.size() >= prefix.size() &&
                                          candidate.compare( 0, prefix.size(), prefix ) == 0 );
        };
        if( !isInAssets( source ) || !isInAssets( target ) || !fileSystem->isExistingFolder( target ) )
        {
            WP_LOG_ERROR( "Assets can only be copied between project asset folders." );
            return false;
        }

        const auto isFolder = fileSystem->isExistingFolder( source );
        if( !isFolder && !fileSystem->isExistingFile( source ) )
        {
            WP_LOG_ERROR( "Cannot copy missing asset: " + source );
            return false;
        }

        auto sourceLower = StringUtil::make_lower( StringUtil::cleanupPath( source ) );
        auto targetLower = StringUtil::make_lower( StringUtil::cleanupPath( target ) );
        auto descendantPrefix = sourceLower + "/";
        if( isFolder && targetLower.size() >= descendantPrefix.size() &&
            targetLower.compare( 0, descendantPrefix.size(), descendantPrefix ) == 0 )
        {
            WP_LOG_ERROR( "Cannot copy an asset folder into itself: " + source );
            return false;
        }

        auto name = Path::getFileName( source );
        auto destination = Path::lexically_normal( target, name );
        auto destinationExists = [&fileSystem]( const String &path ) {
            return fileSystem->isExistingFile( path ) || fileSystem->isExistingFolder( path );
        };

        if( generateUniqueName )
        {
            auto extension = isFolder ? String() : Path::getFileExtension( name );
            auto baseName = isFolder ? name : Path::getFileNameWithoutExtension( name );
            auto copyNumber = 1;
            while( destinationExists( destination ) )
            {
                auto suffix = copyNumber == 1 ? String( " Copy" )
                                              : String( " Copy " ) + StringUtil::toString( copyNumber );
                destination = Path::lexically_normal( target, baseName + suffix + extension );
                ++copyNumber;
            }
        }
        else if( destinationExists( destination ) )
        {
            WP_LOG_ERROR( "An asset already exists at: " + destination );
            return false;
        }

        auto resources = applicationManager->getResourceDatabase();
        auto catalog = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() ) : nullptr;
        if( !catalog )
            return false;
        const auto result = catalog->performFileOperation( AssetDatabaseManager::FileOperation::Copy, source, destination );
        if( !result.succeeded )
        {
            WP_LOG_ERROR( "Cannot copy asset: " + result.error );
            catalog->recoverFileOperations();
            refreshFolders( { target } );
            return false;
        }
        refreshFolders( { target } );
        return true;
    }

    bool ProjectAssetsWindow::moveAsset( const String &sourcePath, const String &targetFolder )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return false;
        }

        auto fileSystem = applicationManager->getFileSystemPtr();
        if( !fileSystem || StringUtil::isNullOrEmpty( sourcePath ) ||
            StringUtil::isNullOrEmpty( targetFolder ) )
        {
            return false;
        }

        auto projectPath = applicationManager->getProjectPath();
        auto source = Path::isPathAbsolute( sourcePath )
                          ? Path::lexically_normal( sourcePath )
                          : Path::lexically_normal( projectPath, sourcePath );
        auto target = Path::isPathAbsolute( targetFolder )
                          ? Path::lexically_normal( targetFolder )
                          : Path::lexically_normal( projectPath, targetFolder );

        auto assetsRoot = Path::lexically_normal( projectPath, "Assets" );
        if( !fileSystem->isExistingFolder( assetsRoot ) )
        {
            assetsRoot = Path::lexically_normal( projectPath );
        }

        auto isInAssets = [&assetsRoot]( const String &path ) {
            auto root = StringUtil::make_lower( StringUtil::cleanupPath( assetsRoot ) );
            auto candidate = StringUtil::make_lower( StringUtil::cleanupPath( path ) );
            auto prefix = root + "/";
            return candidate == root || ( candidate.size() >= prefix.size() &&
                                          candidate.compare( 0, prefix.size(), prefix ) == 0 );
        };
        if( !isInAssets( source ) || !isInAssets( target ) || !fileSystem->isExistingFolder( target ) )
        {
            WP_LOG_ERROR( "Assets can only be moved between project asset folders." );
            return false;
        }

        const auto isFolder = fileSystem->isExistingFolder( source );
        if( !isFolder && !fileSystem->isExistingFile( source ) )
        {
            WP_LOG_ERROR( "Cannot move missing asset: " + source );
            return false;
        }

        auto destination = Path::lexically_normal( target, Path::getFileName( source ) );
        auto sourceLower = StringUtil::make_lower( StringUtil::cleanupPath( source ) );
        auto targetLower = StringUtil::make_lower( StringUtil::cleanupPath( target ) );
        auto destinationLower = StringUtil::make_lower( StringUtil::cleanupPath( destination ) );

        if( sourceLower == destinationLower )
        {
            return false;
        }

        auto descendantPrefix = sourceLower + "/";
        if( isFolder && targetLower.size() >= descendantPrefix.size() &&
            targetLower.compare( 0, descendantPrefix.size(), descendantPrefix ) == 0 )
        {
            WP_LOG_ERROR( "Cannot move an asset folder into itself: " + source );
            return false;
        }

        if( fileSystem->isExistingFile( destination ) || fileSystem->isExistingFolder( destination ) )
        {
            WP_LOG_ERROR( "An asset already exists at: " + destination );
            return false;
        }

        auto sourceFolder = Path::getFilePath( source );
        auto resources = applicationManager->getResourceDatabase();
        auto catalog = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() ) : nullptr;
        if( !catalog )
            return false;
        const auto result = catalog->performFileOperation( AssetDatabaseManager::FileOperation::Move, source, destination );
        if( !result.succeeded )
        {
            WP_LOG_ERROR( "Cannot move asset: " + result.error );
            catalog->recoverFileOperations();
            refreshFolders( { sourceFolder, target } );
            return false;
        }
        refreshFolders( { sourceFolder, target } );
        return true;
    }

    bool ProjectAssetsWindow::pasteClipboard()
    {
        if( StringUtil::isNullOrEmpty( m_clipboardPath ) )
        {
            return false;
        }

        auto targetFolder = getSelectedFolderPath();
        if( m_clipboardCut )
        {
            if( moveAsset( m_clipboardPath, targetFolder ) )
            {
                m_clipboardPath.clear();
                m_clipboardCut = false;
                return true;
            }

            return false;
        }

        return copyAsset( m_clipboardPath, targetFolder, true );
    }

    bool ProjectAssetsWindow::duplicateSelectedAsset()
    {
        auto selectedPath = getSelectedPath();
        if( StringUtil::isNullOrEmpty( selectedPath ) )
        {
            return false;
        }

        return copyAsset( selectedPath, Path::getFilePath( selectedPath ), true );
    }

    String ProjectAssetsWindow::getAssetsRootPath() const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return {};
        }

        auto projectPath = Path::lexically_normal( applicationManager->getProjectPath() );
        auto assetsPath = Path::lexically_normal( projectPath, "Assets" );
        auto fileSystem = applicationManager->getFileSystemPtr();
        return fileSystem && fileSystem->isExistingFolder( assetsPath ) ? assetsPath : projectPath;
    }

    bool ProjectAssetsWindow::navigateToFolder( const String &folderPath, bool addToHistory,
                                               bool expandNode )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto tree = getTree();
        if( !applicationManager || !tree || StringUtil::isNullOrEmpty( folderPath ) )
        {
            return false;
        }

        auto path = Path::isPathAbsolute( folderPath )
                        ? Path::lexically_normal( folderPath )
                        : Path::lexically_normal( applicationManager->getProjectPath(), folderPath );
        auto fileSystem = applicationManager->getFileSystemPtr();
        auto node = findNodeByPath( path );
        if( !fileSystem || !fileSystem->isExistingFolder( path ) || !node )
        {
            return false;
        }

        auto pathKey = StringUtil::make_lower( StringUtil::cleanupPath( path ) );
        auto currentKey = StringUtil::make_lower( StringUtil::cleanupPath( m_currentFolder ) );
        if( addToHistory && pathKey != currentKey )
        {
            if( m_navigationHistoryIndex + 1 < static_cast<s32>( m_navigationHistory.size() ) )
            {
                m_navigationHistory.erase( m_navigationHistory.begin() + m_navigationHistoryIndex + 1,
                                           m_navigationHistory.end() );
            }

            m_navigationHistory.push_back( path );
            m_navigationHistoryIndex = static_cast<s32>( m_navigationHistory.size() ) - 1;
        }

        m_currentFolder = path;
        tree->setSelectedTreeNode( node );
        if( expandNode )
        {
            tree->expand( node );
        }
        updateThumbnailPreview( {} );
        updateNavigationControls();
        return true;
    }

    void ProjectAssetsWindow::navigateBack()
    {
        if( m_navigationHistoryIndex > 0 )
        {
            --m_navigationHistoryIndex;
            navigateToFolder( m_navigationHistory[m_navigationHistoryIndex], false );
        }
    }

    void ProjectAssetsWindow::navigateForward()
    {
        if( m_navigationHistoryIndex >= 0 &&
            m_navigationHistoryIndex + 1 < static_cast<s32>( m_navigationHistory.size() ) )
        {
            ++m_navigationHistoryIndex;
            navigateToFolder( m_navigationHistory[m_navigationHistoryIndex], false );
        }
    }

    void ProjectAssetsWindow::navigateUp()
    {
        if( StringUtil::isNullOrEmpty( m_currentFolder ) )
        {
            return;
        }

        auto root = getAssetsRootPath();
        auto currentKey = StringUtil::make_lower( StringUtil::cleanupPath( m_currentFolder ) );
        auto rootKey = StringUtil::make_lower( StringUtil::cleanupPath( root ) );
        if( currentKey != rootKey )
        {
            navigateToFolder( Path::getFilePath( m_currentFolder ) );
        }
    }

    void ProjectAssetsWindow::updateNavigationControls()
    {
        if( m_backButton )
        {
            m_backButton->setEnabled( m_navigationHistoryIndex > 0, false );
        }
        if( m_forwardButton )
        {
            m_forwardButton->setEnabled(
                m_navigationHistoryIndex >= 0 &&
                    m_navigationHistoryIndex + 1 < static_cast<s32>( m_navigationHistory.size() ),
                false );
        }

        auto root = getAssetsRootPath();
        auto currentKey = StringUtil::make_lower( StringUtil::cleanupPath( m_currentFolder ) );
        auto rootKey = StringUtil::make_lower( StringUtil::cleanupPath( root ) );
        if( m_upButton )
        {
            m_upButton->setEnabled( !currentKey.empty() && currentKey != rootKey, false );
        }
        if( m_rootButton )
        {
            m_rootButton->setEnabled( !currentKey.empty() && currentKey != rootKey, false );
        }

        if( m_breadcrumbText )
        {
            auto displayPath = String( "Assets" );
            auto rootPrefix = rootKey + "/";
            if( !currentKey.empty() && currentKey != rootKey )
            {
                if( currentKey.size() > rootPrefix.size() &&
                    currentKey.compare( 0, rootPrefix.size(), rootPrefix ) == 0 )
                {
                    auto relativePath = Path::getRelativePath( root, m_currentFolder );
                    relativePath = StringUtil::replaceAll( relativePath, "\\", " / " );
                    relativePath = StringUtil::replaceAll( relativePath, "/", " / " );
                    displayPath += " / " + relativePath;
                }
                else
                {
                    displayPath = Path::getFileName( m_currentFolder );
                }
            }

            //Util::setText( m_breadcrumbText, ICON_FA_FOLDER_OPEN " " + displayPath );
        }
    }

    void ProjectAssetsWindow::applySearchFilter( const String &filter )
    {
        m_searchFilter = StringUtil::make_lower( filter );

        auto tree = getTree();
        if( !tree )
        {
            return;
        }

        if( auto root = tree->getRoot() )
        {
            root->setVisible( true, false );
            for( auto child : root->getChildren() )
            {
                if( auto childNode = workphone::dynamic_pointer_cast<ui::IUITreeNode>( child ) )
                {
                    updateNodeFilter( childNode, m_searchFilter );
                }
            }
        }
    }

    bool ProjectAssetsWindow::updateNodeFilter( SmartPtr<ui::IUITreeNode> node, const String &filter )
    {
        if( !node )
        {
            return false;
        }

        auto matches = filter.empty();
        if( auto treeData = workphone::dynamic_pointer_cast<ProjectTreeData>( node->getNodeUserData() ) )
        {
            auto searchable = StringUtil::make_lower( treeData->getObjectType() );
            matches = matches || searchable.find( filter ) != String::npos;
        }

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

    void ProjectAssetsWindow::updateThumbnailPreview( const String &filePath )
    {
        if( !m_thumbnailPreview || !m_thumbnailCaption )
        {
            return;
        }

        m_thumbnailPreview->setTexture( nullptr );
        m_thumbnailPreview->setVisible( false );
        m_thumbnailCaption->setVisible( false );
        if( StringUtil::isNullOrEmpty( filePath ) || !ApplicationUtil::isSupportedTexture( filePath ) )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager ? applicationManager->getGraphicsSystemPtr() : nullptr;
        auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;
        if( !textureManager )
        {
            return;
        }

        auto absolutePath =
            Path::isPathAbsolute( filePath )
                ? Path::lexically_normal( filePath )
                : Path::lexically_normal( applicationManager->getProjectPath(), filePath );
        if( auto texture = textureManager->loadFromFile( absolutePath ) )
        {
            m_thumbnailPreview->setTexture( texture );
            m_thumbnailPreview->setVisible( true );
            m_thumbnailCaption->setText( ICON_FA_FILE_IMAGE " Preview: " +
                                         Path::getFileName( filePath ) );
            m_thumbnailCaption->setVisible( true );
        }
    }

    String ProjectAssetsWindow::getAssetIcon( const String &path, bool isFolder )
    {
        if( isFolder )
        {
            return ICON_FA_FOLDER;
        }
        if( ApplicationUtil::isSupportedTexture( path ) )
        {
            return ICON_FA_FILE_IMAGE;
        }
        if( ApplicationUtil::isSupportedMesh( path ) )
        {
            return ICON_FA_CUBE;
        }
        if( ApplicationUtil::isSupportedSound( path ) )
        {
            return ICON_FA_FILE_AUDIO;
        }
        if( ApplicationUtil::isSupportedFont( path ) )
        {
            return ICON_FA_FONT;
        }

        auto extension = StringUtil::make_lower( Path::getFileExtension( path ) );
        if( extension == ".mp4" || extension == ".mov" || extension == ".avi" || extension == ".webm" )
        {
            return ICON_FA_FILE_VIDEO;
        }
        if( extension == ".lua" || extension == ".py" || extension == ".js" || extension == ".cpp" ||
            extension == ".h" || extension == ".hpp" || extension == ".shader" || extension == ".glsl" ||
            extension == ".hlsl" )
        {
            return ICON_FA_FILE_CODE;
        }
        if( extension == ".mat" || extension == ".material" || extension == ".lightingpreset" )
        {
            return ICON_FA_PALETTE;
        }
        if( extension == ".scene" || extension == ".prefab" || extension == ".resource" )
        {
            return ICON_FA_CUBES;
        }
        if( extension == ".db" || extension == ".sqlite" || extension == ".sqlite3" )
        {
            return ICON_FA_DATABASE;
        }
        if( extension == ".zip" || extension == ".7z" || extension == ".pak" )
        {
            return ICON_FA_BOX_ARCHIVE;
        }
        if( extension == ".json" || extension == ".xml" || extension == ".yaml" || extension == ".yml" ||
            extension == ".txt" || extension == ".md" )
        {
            return ICON_FA_FILE_LINES;
        }

        return ICON_FA_FILE;
    }

    String ProjectAssetsWindow::getAssetDisplayName( const String &path, bool isFolder )
    {
        return getAssetIcon( path, isFolder ) + "  " + Path::getFileName( path );
    }

    SmartPtr<ui::IUITreeNode> ProjectAssetsWindow::findNodeByPath( const String &path ) const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto tree = getTree();
        if( !applicationManager || !tree || StringUtil::isNullOrEmpty( path ) )
        {
            return nullptr;
        }

        auto projectPath = applicationManager->getProjectPath();
        auto absolutePath = Path::isPathAbsolute( path ) ? Path::lexically_normal( path )
                                                         : Path::lexically_normal( projectPath, path );
        auto pathKey = StringUtil::make_lower( StringUtil::cleanupPath( absolutePath ) );

        for( auto node : tree->getTreeNodes() )
        {
            if( !node )
            {
                continue;
            }

            auto treeData = workphone::dynamic_pointer_cast<ProjectTreeData>( node->getNodeUserData() );
            if( treeData )
            {
                auto nodePath = treeData->getObjectType();
                nodePath = Path::isPathAbsolute( nodePath )
                               ? Path::lexically_normal( nodePath )
                               : Path::lexically_normal( projectPath, nodePath );
                if( StringUtil::make_lower( StringUtil::cleanupPath( nodePath ) ) == pathKey )
                {
                    return node;
                }
            }
        }

        return nullptr;
    }

    void ProjectAssetsWindow::removeTreeNode( SmartPtr<ui::IUITreeNode> node )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto tree = getTree();
        auto ui = applicationManager ? applicationManager->getUIPtr() : nullptr;
        if( !node || !tree || !ui )
        {
            return;
        }

        Array<SmartPtr<ui::IUITreeNode>> removedNodes;
        std::function<void( SmartPtr<ui::IUITreeNode> )> collectNodes;
        collectNodes = [&removedNodes, &collectNodes]( SmartPtr<ui::IUITreeNode> current ) {
            if( !current )
            {
                return;
            }

            removedNodes.push_back( current );
            for( auto child : current->getChildren() )
            {
                collectNodes( workphone::dynamic_pointer_cast<ui::IUITreeNode>( child ) );
            }
        };
        collectNodes( node );

        if( auto parent = node->getParent() )
        {
            parent->removeChild( node );
        }

        auto selectedNode = tree->getSelectedTreeNode();
        auto isRemoved = [&removedNodes]( SmartPtr<ui::IUITreeNode> candidate ) {
            return std::find( removedNodes.begin(), removedNodes.end(), candidate ) !=
                   removedNodes.end();
        };
        if( isRemoved( selectedNode ) )
        {
            tree->setSelectedTreeNode( nullptr );
        }

        auto selectedNodes = tree->getSelectedTreeNodes();
        selectedNodes.erase( std::remove_if( selectedNodes.begin(), selectedNodes.end(), isRemoved ),
                             selectedNodes.end() );
        tree->setSelectedTreeNodes( selectedNodes );

        if( isRemoved(
                workphone::dynamic_pointer_cast<ui::IUITreeNode>( tree->getDragSourceElement() ) ) )
        {
            tree->setDragSourceElement( nullptr );
        }
        if( isRemoved(
                workphone::dynamic_pointer_cast<ui::IUITreeNode>( tree->getDropDestinationElement() ) ) )
        {
            tree->setDropDestinationElement( nullptr );
        }

        auto treeNodes = tree->getTreeNodes();
        treeNodes.erase( std::remove_if( treeNodes.begin(), treeNodes.end(), isRemoved ),
                         treeNodes.end() );
        tree->setTreeNodes( treeNodes );

        for( auto removedNode : removedNodes )
        {
            if( auto data = removedNode->getNodeUserData() )
            {
                removeData( data );
            }
            removedNode->setNodeUserData( nullptr );
            ui->removeElement( removedNode );
        }
    }

    bool ProjectAssetsWindow::refreshFolder( const String &folderPath )
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto tree = getTree();
        if( !applicationManager || !tree || StringUtil::isNullOrEmpty( folderPath ) )
        {
            return false;
        }

        auto fileSystem = applicationManager->getFileSystemPtr();
        auto projectPath = applicationManager->getProjectPath();
        auto absoluteFolder = Path::isPathAbsolute( folderPath )
                                  ? Path::lexically_normal( folderPath )
                                  : Path::lexically_normal( projectPath, folderPath );
        if( !fileSystem || !fileSystem->isExistingFolder( absoluteFolder ) )
        {
            return false;
        }

        auto folderNode = findNodeByPath( absoluteFolder );
        if( !folderNode )
        {
            return false;
        }

        // Read only this directory. getFolderListing() recursively scans every descendant,
        // which is unnecessarily expensive when reconciling an already-built subtree.
        auto files = Path::getFiles( absoluteFolder );
        auto folders = Path::getFolders( absoluteFolder );

        auto makePathKey = [&projectPath]( const String &path ) {
            auto absolutePath = Path::isPathAbsolute( path )
                                    ? Path::lexically_normal( path )
                                    : Path::lexically_normal( projectPath, path );
            return StringUtil::make_lower( StringUtil::cleanupPath( absolutePath ) );
        };

        std::set<String> desiredPaths;
        for( const auto &file : files )
        {
            desiredPaths.insert( makePathKey( file ) );
        }
        for( const auto &subFolder : folders )
        {
            desiredPaths.insert( makePathKey( subFolder ) );
        }

        auto children = folderNode->getChildren();
        for( auto child : children )
        {
            auto childNode = workphone::dynamic_pointer_cast<ui::IUITreeNode>( child );
            auto treeData =
                childNode
                    ? workphone::dynamic_pointer_cast<ProjectTreeData>( childNode->getNodeUserData() )
                    : nullptr;
            if( treeData &&
                desiredPaths.find( makePathKey( treeData->getObjectType() ) ) == desiredPaths.end() )
            {
                removeTreeNode( childNode );
            }
        }

        std::set<String> existingPaths;
        for( auto child : folderNode->getChildren() )
        {
            auto childNode = workphone::dynamic_pointer_cast<ui::IUITreeNode>( child );
            auto treeData =
                childNode
                    ? workphone::dynamic_pointer_cast<ProjectTreeData>( childNode->getNodeUserData() )
                    : nullptr;
            if( treeData )
            {
                existingPaths.insert( makePathKey( treeData->getObjectType() ) );
            }
        }

        for( const auto &file : files )
        {
            if( existingPaths.find( makePathKey( file ) ) == existingPaths.end() )
            {
                addFileToTree( folderNode, file );
            }
        }
        for( const auto &subFolder : folders )
        {
            if( existingPaths.find( makePathKey( subFolder ) ) == existingPaths.end() )
            {
                addFolderToTree( folderNode, fileSystem->getFolderListing( subFolder ) );
            }
        }

        applySearchFilter( m_searchFilter );
        return true;
    }

    void ProjectAssetsWindow::refreshFolders( const Array<String> &folderPaths )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto fileSystem = applicationManager ? applicationManager->getFileSystemPtr() : nullptr;

        std::set<String> refreshedPaths;
        for( const auto &folderPath : folderPaths )
        {
            if( StringUtil::isNullOrEmpty( folderPath ) )
            {
                continue;
            }

            auto key = StringUtil::make_lower( StringUtil::cleanupPath( folderPath ) );
            if( refreshedPaths.insert( key ).second )
            {
                refreshFolder( folderPath );
                if( fileSystem )
                {
                    fileSystem->refreshPath( folderPath, true );
                }
            }
        }
    }

    Parameter ProjectAssetsWindow::TreeCtrlListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( auto owner = getOwnerPtr() )
        {
            if( eventValue == IEvent::handleTreeSelectionRelease )
            {
                ScopedLock lock( owner );

                owner->m_selectedObject = object;

                auto &obj = arguments[0].object;
                if( obj && obj->isDerived<ui::IUITreeNode>() )
                {
                    auto node = workphone::static_ptr_cast<ui::IUITreeNode>( obj );

                    auto nodeUserData = node->getNodeUserData();
                    auto treeData = workphone::static_pointer_cast<ProjectTreeData>( nodeUserData );
                    if( treeData )
                    {
                        static const auto folderType = String( "folder" );
                        if( treeData->getOwnerType() == folderType )
                        {
                            // Keep the expansion state chosen by the tree click.
                            owner->navigateToFolder( treeData->getObjectType(), true, false );
                        }
                        else
                        {
                            owner->updateThumbnailPreview( treeData->getObjectType() );
                            owner->handleTreeSelectionActivated( node );
                        }
                    }
                }
            }
            else if( eventValue == IEvent::handleTreeNodeDoubleClicked )
            {
                ScopedLock lock( owner );

                owner->m_selectedObject = object;

                auto node = workphone::static_ptr_cast<ui::IUITreeNode>( arguments[0].object );
                owner->handleTreeNodeDoubleClicked( node );
            }
        }

        return {};
    }

    SmartPtr<ProjectAssetsWindow> ProjectAssetsWindow::TreeCtrlListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void ProjectAssetsWindow::TreeCtrlListener::setOwner( SmartPtr<ProjectAssetsWindow> owner )
    {
        m_owner = owner;
    }

    ProjectAssetsWindow::TreeCtrlListener::TreeCtrlListener() = default;

    ProjectAssetsWindow::TreeCtrlListener::~TreeCtrlListener() = default;

    Parameter ProjectAssetsWindow::WindowListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        auto owner = getOwner();
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !owner || !applicationManager )
        {
            return {};
        }

        auto eventObject = object ? object : sender;
        auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( eventObject );
        if( !element )
        {
            return {};
        }

        if( eventValue == IEvent::handleValueChanged && element == owner->m_searchEntry )
        {
            auto filter = arguments.empty() ? owner->m_searchEntry->getText() : arguments[0].getStr();
            owner->applySearchFilter( filter );
            return {};
        }

        auto menuId = static_cast<MenuId>( element->getElementId() );
        switch( menuId )
        {
        case MenuId::NavigateBack:
            owner->navigateBack();
            return {};
        case MenuId::NavigateForward:
            owner->navigateForward();
            return {};
        case MenuId::NavigateUp:
            owner->navigateUp();
            return {};
        case MenuId::NavigateRoot:
            owner->navigateToFolder( owner->getAssetsRootPath() );
            return {};
        default:
            break;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        auto jobQueue = applicationManager->getJobQueuePtr();
        auto fileSystem = applicationManager->getFileSystem();
        auto selectedPath = owner->getSelectedPath();
        auto targetFolder = owner->getSelectedFolderPath();

        if( menuId == MenuId::Refresh )
        {
            owner->refreshFolder( targetFolder );
            fileSystem->refreshPath( targetFolder, true );
            return {};
        }

        auto commandManager = applicationManager->getCommandManager();
        if( !commandManager )
        {
            WP_LOG_ERROR( "Command Manager not found" );
            return {};
        }

        switch( menuId )
        {
        case MenuId::AddLightingPreset:
        {
            const auto materialName = String( "NewLightingPreset.lightingpreset" );

            auto cmd = factoryManager->make_ptr<AddResourceCmd>();

            auto materialPath = targetFolder + "/" + materialName;
            cmd->setFilePath( materialPath );

            cmd->setResourceType( AddResourceCmd::ResourceType::LightingPreset );
            commandManager->addCommand( cmd );
        }
        break;
        case MenuId::AddMaterial:
        {
            const auto materialName = String( "NewMaterial.mat" );

            auto cmd = factoryManager->make_ptr<AddResourceCmd>();

            auto materialPath = targetFolder + "/" + materialName;
            cmd->setFilePath( materialPath );

            cmd->setResourceType( AddResourceCmd::ResourceType::Material );
            commandManager->addCommand( cmd );
        }
        break;
        case MenuId::AddScript:
        {
            const auto materialName = String( "NewScript.lua" );

            auto cmd = factoryManager->make_ptr<AddNewScriptCmd>();

            auto materialPath = targetFolder + "/" + materialName;
            cmd->setPath( materialPath );

            commandManager->addCommand( cmd );
        }
        break;
        case MenuId::AddDirector:
        {
            auto pFactory = static_cast<IFactory *>( element->getUserData() );
            auto factory = SmartPtr<IFactory>( pFactory );
            auto factoryName = factory->getObjectTypeName();

            auto names = StringUtil::split( factoryName, "::" );
            auto last = names.empty() ? "Untitled" : names.back();
            const auto directorFileName = String( last + ".resource" );

            auto director = factory->make_ptr<IBuildDirector>();
            auto properties = director->getProperties();

            auto dataStr = DataUtil::toString( properties.get(), true );
            fileSystem->writeAllText( targetFolder + "/" + directorFileName, dataStr );
            owner->refreshFolders( { targetFolder } );
        }
        break;
        case MenuId::AddTerrainDirector:
        {
            const auto materialName = String( "NewTerrain.resource" );

            auto cmd = factoryManager->make_ptr<AddResourceCmd>();

            auto materialPath = targetFolder + "/" + materialName;
            cmd->setFilePath( materialPath );

            cmd->setResourceType( AddResourceCmd::ResourceType::Director );
            commandManager->addCommand( cmd );
        }
        break;
        case MenuId::AddFolder:
        {
            auto folderPath = Path::lexically_normal( targetFolder, "New Folder" );
            auto folderNumber = 2;
            while( fileSystem->isExistingFolder( folderPath ) ||
                   fileSystem->isExistingFile( folderPath ) )
            {
                folderPath = Path::lexically_normal(
                    targetFolder, String( "New Folder " ) + StringUtil::toString( folderNumber++ ) );
            }

            fileSystem->createDirectories( folderPath );
            owner->refreshFolders( { targetFolder } );
        }
        break;
        case MenuId::Cut:
        {
            owner->m_clipboardPath = selectedPath;
            owner->m_clipboardCut = !StringUtil::isNullOrEmpty( selectedPath );
        }
        break;
        case MenuId::Copy:
        {
            owner->m_clipboardPath = selectedPath;
            owner->m_clipboardCut = false;
        }
        break;
        case MenuId::Paste:
        {
            owner->pasteClipboard();
        }
        break;
        case MenuId::Duplicate:
        {
            owner->duplicateSelectedAsset();
        }
        break;
        case MenuId::Remove:
        {
            if( StringUtil::isNullOrEmpty( selectedPath ) )
            {
                break;
            }

            auto cmd = factoryManager->make_ptr<RemoveResourceCmd>();
            cmd->setFilePath( selectedPath );
            commandManager->addCommand( cmd );
        }
        break;
        case MenuId::Import:
        {
            auto job = factoryManager->make_ptr<ImportResourceJob>();
            job->setFilePath( selectedPath );
            jobQueue->addJob( job );
        }
        break;
        case MenuId::Reimport:
        {
            auto job = factoryManager->make_ptr<ImportResourceJob>();
            job->setFilePath( selectedPath );
            job->setReimport( true );
            jobQueue->addJob( job );
        }
        break;
        default:
        {
        }
        }

        return {};
    }

    SmartPtr<ProjectAssetsWindow> ProjectAssetsWindow::WindowListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void ProjectAssetsWindow::WindowListener::setOwner( SmartPtr<ProjectAssetsWindow> owner )
    {
        m_owner = owner;
    }

    ProjectAssetsWindow::WindowListener::WindowListener() = default;

    ProjectAssetsWindow::WindowListener::~WindowListener() = default;

    Parameter ProjectAssetsWindow::DragSource::handleEvent( EventType eventType, hash_type eventValue,
                                                            const Array<Parameter> &arguments,
                                                            SmartPtr<ISharedObject> sender,
                                                            SmartPtr<ISharedObject> object,
                                                            SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleDrag )
        {
            auto str = handleDrag( Vector2I::zero(), sender );
            return Parameter( str );
        }

        return {};
    }

    String ProjectAssetsWindow::DragSource::handleDrag( const Vector2I &position,
                                                        SmartPtr<ui::IUIElement> element )
    {
        if( element->isDerived<ui::IUITreeNode>() )
        {
            auto treeNode = workphone::static_pointer_cast<ui::IUITreeNode>( element );

            auto userData = treeNode->getNodeUserData();
            auto projectTreeData = workphone::dynamic_pointer_cast<ProjectTreeData>( userData );
            if( projectTreeData )
            {
                auto properties = workphone::make_ptr<Properties>();

                auto ownerType = projectTreeData->getOwnerType();
                auto path = projectTreeData->getObjectType();

                properties->setProperty( "assetPath", path );
                properties->setProperty( "assetType", ownerType );
                if( ownerType == fileExt )
                {
                    properties->setProperty( "filePath", path );
                }

                return DataUtil::toString( properties.get(), true );
            }
        }

        return String( "" );
    }

    SmartPtr<ProjectAssetsWindow> ProjectAssetsWindow::DragSource::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void ProjectAssetsWindow::DragSource::setOwner( SmartPtr<ProjectAssetsWindow> owner )
    {
        m_owner = owner;
    }

    ProjectAssetsWindow::DragSource::DragSource() = default;

    ProjectAssetsWindow::DragSource::~DragSource() = default;

    bool ProjectAssetsWindow::isValid() const
    {
        bool valid = true;
        return valid;
    }

    SmartPtr<ui::IUITreeCtrl> ProjectAssetsWindow::getTree() const
    {
        return m_tree;
    }

    void ProjectAssetsWindow::setTree( SmartPtr<ui::IUITreeCtrl> tree )
    {
        m_tree = tree;
    }

    Parameter ProjectAssetsWindow::handleApplicationEvent( EventType eventType, hash_type eventValue,
                                                           const Array<Parameter> &arguments,
                                                           SmartPtr<ISharedObject> sender,
                                                           SmartPtr<ISharedObject> object,
                                                           SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::refreshAll )
        {
            build();
        }
        else if( eventValue == IEvent::refreshPath && !arguments.empty() )
        {
            refreshFolder( arguments[0].getStr() );
        }
        else if( eventValue == IEvent::fileAction && arguments.size() >= 3 )
        {
            auto changedPath = Path::lexically_normal( arguments[1].getStr(), arguments[2].getStr() );
            auto parentPath = Path::getFilePath( changedPath );
            refreshFolder( parentPath );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto fileSystem = applicationManager ? applicationManager->getFileSystemPtr() : nullptr;
            if( fileSystem && fileSystem->isExistingFolder( changedPath ) )
            {
                refreshFolder( changedPath );
            }
        }

        return {};
    }

    void ProjectAssetsWindow::setBuildTreeJob( SmartPtr<IJob> buildTreeJob )
    {
        m_buildTreeJob = buildTreeJob;
    }

    SmartPtr<IJob> ProjectAssetsWindow::getBuildTreeJob() const
    {
        return m_buildTreeJob;
    }

    Parameter ProjectAssetsWindow::DropTarget::handleEvent( EventType eventType, hash_type eventValue,
                                                            const Array<Parameter> &arguments,
                                                            SmartPtr<ISharedObject> sender,
                                                            SmartPtr<ISharedObject> object,
                                                            SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleDrop )
        {
            if( arguments.empty() )
            {
                return {};
            }

            Vector2I position;
            auto src = workphone::dynamic_pointer_cast<ui::IUIElement>( sender );
            auto dst = workphone::dynamic_pointer_cast<ui::IUIElement>( object );
            String dataText = arguments[0].str;

            handleDrop( position, src, dst, dataText );
        }

        return {};
    }

    bool ProjectAssetsWindow::DropTarget::handleDrop( const Vector2I &position,
                                                      SmartPtr<ui::IUIElement> src,
                                                      SmartPtr<ui::IUIElement> dst, const String &text )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );

            auto prefabManager = applicationManager->getPrefabManager();
            WP_ASSERT( prefabManager );

            auto properties = workphone::make_ptr<Properties>();
            if( !StringUtil::isNullOrEmpty( text ) )
            {
                DataUtil::parse( text, properties.get() );

                auto owner = getOwner();
                if( !owner )
                {
                    return false;
                }

                auto tree = owner->getTree();
                if( !dst && tree )
                {
                    dst = tree->getDropDestinationElement();
                }

                auto getTargetPath = []( SmartPtr<ui::IUITreeNode> node ) {
                    if( node )
                    {
                        auto userData = node->getNodeUserData();
                        auto projectTreeData =
                            workphone::dynamic_pointer_cast<ProjectTreeData>( userData );
                        if( projectTreeData )
                        {
                            auto path = projectTreeData->getObjectType();
                            if( projectTreeData->getOwnerType() == fileExt )
                            {
                                path = Path::getFilePath( path );
                            }

                            return path;
                        }
                    }

                    return String();
                };

                auto treeDropDst = workphone::dynamic_pointer_cast<ui::IUITreeNode>( dst );
                auto targetPath = getTargetPath( treeDropDst );
                if( StringUtil::isNullOrEmpty( targetPath ) && tree )
                {
                    targetPath = getTargetPath( tree->getSelectedTreeNode() );
                }
                if( StringUtil::isNullOrEmpty( targetPath ) )
                {
                    targetPath = owner->getSelectedFolderPath();
                }
                else if( !Path::isPathAbsolute( targetPath ) )
                {
                    targetPath =
                        Path::lexically_normal( applicationManager->getProjectPath(), targetPath );
                }

                auto assetPath = properties->getProperty( "assetPath" );
                if( !StringUtil::isNullOrEmpty( assetPath ) )
                {
                    return owner->moveAsset( assetPath, targetPath );
                }

                u32 actorId = 0;
                if( !properties->getPropertyValue( "actorId", actorId ) )
                {
                    return false;
                }

                if( auto actor = sceneManager->getActor( actorId ) )
                {
                    auto name = actor->getName();

                    if( StringUtil::isNullOrEmpty( targetPath ) )
                    {
                        targetPath = applicationManager->getProjectPath();
                        auto assetsPath = Path::lexically_normal( targetPath, "Assets" );
                        if( fileSystem->isExistingFolder( assetsPath ) )
                        {
                            targetPath = assetsPath;
                        }
                    }

                    auto prefabFilePath = targetPath + "/" + name + ".prefab";
                    prefabFilePath = StringUtil::cleanupPath( prefabFilePath );

                    prefabManager->savePrefab( prefabFilePath, actor );
                    owner->refreshFolders( { targetPath } );
                    return true;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    SmartPtr<ProjectAssetsWindow> ProjectAssetsWindow::DropTarget::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void ProjectAssetsWindow::DropTarget::setOwner( SmartPtr<ProjectAssetsWindow> owner )
    {
        m_owner = owner;
    }

    ProjectAssetsWindow::DropTarget::DropTarget() = default;

    ProjectAssetsWindow::DropTarget::~DropTarget() = default;

    void ProjectAssetsWindow::BuildTreeJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( isInterrupted() )
        {
            return;
        }

        if( auto owner = getOwner() )
        {
            if( applicationManager->isRunning() && !applicationManager->getQuit() )
            {
                auto buildTreeJob = owner->getBuildTreeJob();
                if( buildTreeJob && buildTreeJob.get() == this )
                {
                    owner->buildTree( buildTreeJob );
                }
            }
        }
    }

    SmartPtr<ProjectAssetsWindow> ProjectAssetsWindow::BuildTreeJob::getOwner() const
    {
        auto p = m_owner.lock();
        return p;
    }

    void ProjectAssetsWindow::BuildTreeJob::setOwner( SmartPtr<ProjectAssetsWindow> owner )
    {
        m_owner = owner;
    }

    ProjectAssetsWindow::BuildTreeJob::BuildTreeJob() = default;

    ProjectAssetsWindow::BuildTreeJob::~BuildTreeJob() = default;
}  // namespace workphone::editor
