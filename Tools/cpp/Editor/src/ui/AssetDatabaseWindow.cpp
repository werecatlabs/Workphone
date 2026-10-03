#include <EditorPCH.hpp>
#include "ui/AssetDatabaseWindow.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, AssetDatabaseWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, AssetDatabaseWindow::EventListener, IEventListener );

    AssetDatabaseWindow::AssetDatabaseWindow()
    {
    }

    AssetDatabaseWindow::~AssetDatabaseWindow()
    {
    }

    void AssetDatabaseWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            setParentWindow( parentWindow );

            auto eventListener = workphone::make_ptr<EventListener>();
            eventListener->setOwner( this );
            applicationManager->addObjectListener( eventListener );
            setEventListener( eventListener );

            auto tabBar = ui->addElementByType<ui::IUITabBar>();
            setTabBar( tabBar );
            parentWindow->addChild( tabBar );

            auto tabItemObjects = tabBar->addTabItem();
            tabItemObjects->setLabel( "Resources" );

            //auto tabItemResourceTrees = tabBar->addTabItem();
            //tabItemResourceTrees->setLabel( "Resource Tree" );

            auto tabItemStaticComponents = tabBar->addTabItem();
            tabItemStaticComponents->setLabel( "Static Components" );

            auto tabItemReferenceComponents = tabBar->addTabItem();
            tabItemReferenceComponents->setLabel( "Reference Components" );

            //auto tabItemComponents = tabBar->addTabItem();
            //tabItemComponents->setLabel( "Configurable Components" );

            auto modelTreeWindow = ui->addElementByType<ui::IUIWindow>();
            tabItemObjects->addChild( modelTreeWindow );

            modelTreeWindow->setSize( Vector2F( 400, 400 ) );

            auto resourcesTree = ui->addElementByType<ui::IUITreeCtrl>();
            modelTreeWindow->addChild( resourcesTree );
            resourcesTree->addObjectListener( eventListener );
            setResourceTreeCtrl( resourcesTree );

            auto resourceAttribsWindow = ui->addElementByType<ui::IUIWindow>();
            resourceAttribsWindow->setSameLine( true );
            tabItemObjects->addChild( resourceAttribsWindow );

            auto grid = ui->addElementByType<ui::IUIDataGrid>();
            //grid->setSameLine( true );
            resourceAttribsWindow->addChild( grid );
            setResourceAttribsGrid( grid );

            auto staticComponentsWindow = ui->addElementByType<ui::IUIWindow>();
            //staticComponentsWindow->setSameLine( true );
            tabItemStaticComponents->addChild( staticComponentsWindow );
            staticComponentsWindow->setSize( Vector2F( 400, 400 ) );

            auto staticComponentsTree = ui->addElementByType<ui::IUITreeCtrl>();
            staticComponentsWindow->addChild( staticComponentsTree );
            staticComponentsTree->addObjectListener( eventListener );
            setStaticComponentsTreeCtrl( staticComponentsTree );

            auto staticComponentsGridWindow = ui->addElementByType<ui::IUIWindow>();
            staticComponentsGridWindow->setSameLine( true );
            tabItemStaticComponents->addChild( staticComponentsGridWindow );

            auto staticComponentsGrid = ui->addElementByType<ui::IUIDataGrid>();
            //staticComponentsGrid->setSameLine( true );
            staticComponentsGridWindow->addChild( staticComponentsGrid );
            setStaticComponentsAttribsGrid( staticComponentsGrid );

            auto referenceTreeWindow = ui->addElementByType<ui::IUIWindow>();
            tabItemReferenceComponents->addChild( referenceTreeWindow );
            referenceTreeWindow->setSize( Vector2F( 400, 400 ) );

            auto modelTreeComponents = ui->addElementByType<ui::IUITreeCtrl>();
            modelTreeComponents->addObjectListener( eventListener );
            referenceTreeWindow->addChild( modelTreeComponents );
            setComponentsTreeCtrl( modelTreeComponents );

            auto referenceComponentsWindow = ui->addElementByType<ui::IUIWindow>();
            referenceComponentsWindow->setSameLine( true );
            tabItemReferenceComponents->addChild( referenceComponentsWindow );

            auto referenceComponentsGrid = ui->addElementByType<ui::IUIDataGrid>();
            referenceComponentsWindow->addChild( referenceComponentsGrid );
            setComponentsAttribsGrid( referenceComponentsGrid );

            //buildResourceTree( resourcesTree );
            //buildStaticComponentsTree( staticComponentsTree );
            //buildComponentsTree( modelTreeComponents );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AssetDatabaseWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto eventListener = getEventListener();
            if( eventListener )
            {
                applicationManager->removeObjectListener( eventListener );

                if( m_resourceTreeCtrl )
                {
                    m_resourceTreeCtrl->removeObjectListener( eventListener );
                }

                if( m_staticComponentsTreeCtrl )
                {
                    m_staticComponentsTreeCtrl->removeObjectListener( eventListener );
                }

                if( m_componentsTreeCtrl )
                {
                    m_componentsTreeCtrl->removeObjectListener( eventListener );
                }
            }

            if( auto parentWindow = getParentWindow() )
            {
                ui->removeElement( parentWindow );
                setParentWindow( nullptr );
            }

            m_assetDatabaseManager = nullptr;
            m_tabBar = nullptr;
            m_resourceTreeCtrl = nullptr;
            m_resourceAttribsGrid = nullptr;
            m_staticComponentsTreeCtrl = nullptr;
            m_staticComponentsAttribsGrid = nullptr;
            m_componentsTreeCtrl = nullptr;
            m_componentsAttribsGrid = nullptr;

            EditorWindow::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AssetDatabaseWindow::setAssetDatabaseManager(
        SmartPtr<AssetDatabaseManager> assetDatabaseManager )
    {
        m_assetDatabaseManager = assetDatabaseManager;
    }

    SmartPtr<AssetDatabaseManager> AssetDatabaseWindow::getAssetDatabaseManager() const
    {
        if( !m_assetDatabaseManager )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto workingDirectory = Path::getWorkingDirectory();
            auto mediaRelativePath = applicationManager->getMediaPath();

            auto mediaPath = Path::lexically_normal( workingDirectory, mediaRelativePath );

            auto databaseManager = workphone::make_ptr<AssetDatabaseManager>();
            databaseManager->loadFromFile( mediaPath + "/resources.db" );

            m_assetDatabaseManager = databaseManager;
        }

        return m_assetDatabaseManager;
    }

    void AssetDatabaseWindow::setTabBar( SmartPtr<ui::IUITabBar> tabBar )
    {
        m_tabBar = tabBar;
    }

    SmartPtr<ui::IUITabBar> AssetDatabaseWindow::getTabBar() const
    {
        return m_tabBar;
    }

    void AssetDatabaseWindow::buildResourceTree( SmartPtr<ui::IUITreeCtrl> treeCtrl )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto root = treeCtrl->addRoot();
        Util::setText( root, "Resources" );

        auto vehiclesNode = treeCtrl->addNode();

        Util::setText( vehiclesNode, "Vehicles" );
        root->addChild( vehiclesNode );

        auto databaseManager = getAssetDatabaseManager();
        auto sql = "select * from configured_models";
        auto result = databaseManager->executeQuery( sql );
        if( result )
        {
            while( !result->eof() )
            {
                auto id = result->getFieldValueAsInt( "id" );
                auto parentId = result->getFieldValueAsInt( "parent_id" );
                auto name = result->getFieldValue( "name" );

                auto treeNode = treeCtrl->addNode();
                Util::setText( treeNode, name );
                treeNode->setId( id );

                if( auto parent = treeCtrl->getNodeById( parentId ) )
                {
                    parent->addChild( treeNode );
                }
                else
                {
                    root->addChild( treeNode );
                }

                result->nextRow();
            }
        }
    }

    void AssetDatabaseWindow::buildStaticComponentsTree( SmartPtr<ui::IUITreeCtrl> treeCtrl )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto root = treeCtrl->addRoot();
        Util::setText( root, "Resources" );

        auto databaseManager = getAssetDatabaseManager();
        auto sql = "select * from configured_models";
        auto result = databaseManager->executeQuery( sql );
        if( result )
        {
            while( !result->eof() )
            {
                auto id = result->getFieldValueAsInt( "id" );
                auto parentId = result->getFieldValueAsInt( "parent_id" );
                auto name = result->getFieldValue( "name" );
                auto standard = result->getFieldValue( "standard" );

                if( !StringUtil::isNullOrEmpty( standard ) )
                {
                    auto treeNode = treeCtrl->addNode();
                    Util::setText( treeNode, name );
                    treeNode->setId( id );

                    if( auto parent = treeCtrl->getNodeById( parentId ) )
                    {
                        parent->addChild( treeNode );
                    }
                    else
                    {
                        root->addChild( treeNode );
                    }

                    auto componentsSql =
                        "select * from model_objects where model_id = " + StringUtil::toString( id );
                    auto componentsResult = databaseManager->executeQuery( componentsSql );
                    if( componentsResult )
                    {
                        while( !componentsResult->eof())
                        {
                            auto componentId = componentsResult->getFieldValueAsInt( "id" );
                            auto componentName = componentsResult->getFieldValue( "ident" );

                            auto componentTreeNode = treeCtrl->addNode();
                            Util::setText( componentTreeNode, componentName );
                            componentTreeNode->setId( componentId );
                            treeNode->addChild( componentTreeNode );

                            componentsResult->nextRow();
                        }
                    }
                }

                result->nextRow();
            }
        }
    }

    void AssetDatabaseWindow::buildComponentsTree( SmartPtr<ui::IUITreeCtrl> treeCtrl )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto databaseManager = getAssetDatabaseManager();

        auto root = treeCtrl->addRoot();
        Util::setText( root, "Configurable Components" );

        auto sql = "select * from ref_components";
        auto result = databaseManager->executeQuery( sql );
        if( result )
        {
            while( !result->eof() )
            {
                auto id = result->getFieldValueAsInt( "id" );
                auto name = result->getFieldValue( "title" );

                auto treeNode = treeCtrl->addNode();

                Util::setText( treeNode, name );
                treeNode->setId( id );

                root->addChild( treeNode );

                result->nextRow();
            }
        }
    }

    void AssetDatabaseWindow::buildResourceAttribs( SmartPtr<ui::IUIDataGrid> grid, s32 id )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto databaseManager = getAssetDatabaseManager();

        auto sqlStr = "select * from attribs where configured_model_id=" + StringUtil::toString( id );
        auto queryResult = databaseManager->executeQuery( sqlStr );
        if( queryResult )
        {
            if( !queryResult->eof() )
            {
                auto properties = queryResult->getProperties();
                if( properties )
                {
                    if( grid )
                    {
                        grid->setProperties( properties );
                    }
                }
            }
        }
    }

    void AssetDatabaseWindow::buildStaticComponentAttribs( SmartPtr<ui::IUIDataGrid> grid, s32 id )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto databaseManager = getAssetDatabaseManager();
        WP_ASSERT( databaseManager );

        auto sqlStr = "select * from " + m_staticAttribsTableName +
                      " where object_id=" + StringUtil::toString( id );
        auto queryResult = databaseManager->executeQuery( sqlStr );
        if( queryResult )
        {
            if( !queryResult->eof() )
            {
                auto properties = queryResult->getProperties();
                if( properties )
                {
                    if( grid )
                    {
                        grid->setProperties( properties );
                    }
                }
            }
        }
    }

    void AssetDatabaseWindow::buildComponentAttribs( SmartPtr<ui::IUIDataGrid> grid, s32 id )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto databaseManager = getAssetDatabaseManager();

        auto sqlStr = "select * from ref_attribs where ref_component_id=" + StringUtil::toString( id );
        auto queryResult = databaseManager->executeQuery( sqlStr );

        if( queryResult )
        {
            if( !queryResult->eof() )
            {
                auto properties = queryResult->getProperties();
                if( properties )
                {
                    if( grid )
                    {
                        grid->setProperties( properties );
                    }
                }
            }
        }
    }

    Parameter AssetDatabaseWindow::handleEvent( EventType eventType, hash_type eventValue,
                                                const Array<Parameter> &arguments,
                                                SmartPtr<ISharedObject> sender,
                                                SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleTreeSelectionActivated )
        {
            //if( sender == m_owner->m_tree )
            //{
            //    auto node = fb::static_pointer_cast<ui::IUITreeNode>( arguments[0].object );
            //    m_owner->handleTreeSelectionChanged( node );
            //}
        }
        else if( eventValue == IEvent::handleTreeSelectionRelease )
        {
            if( auto treeCtrl = getResourceTreeCtrl() )
            {
                if( sender == treeCtrl )
                {
                    auto selectedNode = treeCtrl->getSelectedTreeNode();
                    if( selectedNode )
                    {
                        auto id = selectedNode->getId();
                        buildResourceAttribs( getResourceAttribsGrid(), static_cast<s32>( id ) );
                    }
                }
            }

            if( auto staticComponentsTreeCtrl = getStaticComponentsTreeCtrl() )
            {
                if( sender == staticComponentsTreeCtrl )
                {
                    auto selectedNode = staticComponentsTreeCtrl->getSelectedTreeNode();
                    if( selectedNode )
                    {
                        auto id = selectedNode->getId();
                        buildStaticComponentAttribs( getStaticComponentsAttribsGrid(),
                                                     static_cast<s32>( id ) );
                    }
                }
            }

            if( auto componentsTreeCtrl = getComponentsTreeCtrl() )
            {
                if( sender == componentsTreeCtrl )
                {
                    auto selectedNode = componentsTreeCtrl->getSelectedTreeNode();
                    if( selectedNode )
                    {
                        auto id = selectedNode->getId();
                        buildComponentAttribs( getComponentsAttribsGrid(), static_cast<s32>( id ) );
                    }
                }
            }
        }
        return {};
    }

    void AssetDatabaseWindow::setResourceAttribsGrid( SmartPtr<ui::IUIDataGrid> resourceAttribsGrid )
    {
        m_resourceAttribsGrid = resourceAttribsGrid;
    }

    SmartPtr<ui::IUIDataGrid> AssetDatabaseWindow::getResourceAttribsGrid() const
    {
        return m_resourceAttribsGrid;
    }

    void AssetDatabaseWindow::setResourceTreeCtrl( SmartPtr<ui::IUITreeCtrl> resourceTreeCtrl )
    {
        m_resourceTreeCtrl = resourceTreeCtrl;
    }

    SmartPtr<ui::IUITreeCtrl> AssetDatabaseWindow::getResourceTreeCtrl() const
    {
        return m_resourceTreeCtrl;
    }

    void AssetDatabaseWindow::setStaticComponentsAttribsGrid(
        SmartPtr<ui::IUIDataGrid> staticComponentsAttribsGrid )
    {
        m_staticComponentsAttribsGrid = staticComponentsAttribsGrid;
    }

    SmartPtr<ui::IUIDataGrid> AssetDatabaseWindow::getStaticComponentsAttribsGrid() const
    {
        return m_staticComponentsAttribsGrid;
    }

    void AssetDatabaseWindow::setStaticComponentsTreeCtrl(
        SmartPtr<ui::IUITreeCtrl> staticComponentsTreeCtrl )
    {
        m_staticComponentsTreeCtrl = staticComponentsTreeCtrl;
    }

    SmartPtr<ui::IUITreeCtrl> AssetDatabaseWindow::getStaticComponentsTreeCtrl() const
    {
        return m_staticComponentsTreeCtrl;
    }

    void AssetDatabaseWindow::setComponentsAttribsGrid( SmartPtr<ui::IUIDataGrid> componentsAttribsGrid )
    {
        m_componentsAttribsGrid = componentsAttribsGrid;
    }

    SmartPtr<ui::IUIDataGrid> AssetDatabaseWindow::getComponentsAttribsGrid() const
    {
        return m_componentsAttribsGrid;
    }

    void AssetDatabaseWindow::setComponentsTreeCtrl( SmartPtr<ui::IUITreeCtrl> componentsTreeCtrl )
    {
        m_componentsTreeCtrl = componentsTreeCtrl;
    }

    SmartPtr<ui::IUITreeCtrl> AssetDatabaseWindow::getComponentsTreeCtrl() const
    {
        return m_componentsTreeCtrl;
    }

    void AssetDatabaseWindow::EventListener::setOwner( SmartPtr<AssetDatabaseWindow> owner )
    {
        m_owner = owner;
    }

    SmartPtr<AssetDatabaseWindow> AssetDatabaseWindow::EventListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    Parameter AssetDatabaseWindow::EventListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            return owner->handleEvent( eventType, eventValue, arguments, sender, object, event );
        }

        return {};
    }

    void AssetDatabaseWindow::EventListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    AssetDatabaseWindow::EventListener::EventListener() = default;

    AssetDatabaseWindow::EventListener::~EventListener() = default;
}  // namespace workphone::editor
