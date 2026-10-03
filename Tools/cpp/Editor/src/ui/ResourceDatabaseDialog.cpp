#include <EditorPCH.hpp>
#include <ui/ResourceDatabaseDialog.hpp>
#include <ui/ProjectTreeData.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ResourceDatabaseDialog, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ResourceDatabaseDialog::UIElementListener,
                               IEventListener );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ResourceDatabaseDialog::DragSource,
                               ui::IUIDragSource );

    ResourceDatabaseDialog::ResourceDatabaseDialog() = default;

    ResourceDatabaseDialog::~ResourceDatabaseDialog()
    {
        unload( nullptr );
    }

    void ResourceDatabaseDialog::load( SmartPtr<ISharedObject> data )
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
            if( parentWindow )
            {
                setParentWindow( parentWindow );
                parentWindow->setLabel( "ResourceDatabaseDialog" );

                if( parent )
                {
                    parent->addChild( parentWindow );
                }

                m_buttonWindow = ui->addElementByType<ui::IUIWindow>();
                parentWindow->addChild( m_buttonWindow );

                m_buttonWindow->setSize( Vector2F( 0.0f, 50.0f ) );

                m_treeWindow = ui->addElementByType<ui::IUIWindow>();
                parentWindow->addChild( m_treeWindow );

                auto uiListener = workphone::make_ptr<UIElementListener>();
                uiListener->setOwner( this );
                m_uiListener = uiListener;

                auto tree = ui->addElementByType<ui::IUITreeCtrl>();
                m_treeWindow->addChild( tree );
                tree->setMultiSelect( false );
                tree->addObjectListener( uiListener );
                tree->setElementId( Tree );
                m_tree = tree;

                auto addComponentButton = ui->addElementByType<ui::IUIButton>();
                WP_ASSERT( addComponentButton );

                addComponentButton->setLabel( "Refresh" );
                m_buttonWindow->addChild( addComponentButton );
                addComponentButton->addObjectListener( uiListener );
                addComponentButton->setElementId( AddComponent );
                m_addComponentButton = addComponentButton;

                auto selectButton = ui->addElementByType<ui::IUIButton>();
                WP_ASSERT( selectButton );

                selectButton->setSameLine( true );
                selectButton->setLabel( "Select" );
                m_buttonWindow->addChild( selectButton );
                selectButton->addObjectListener( uiListener );
                selectButton->setElementId( Select );
                m_selectButton = selectButton;

                auto dragSource = workphone::make_ptr<DragSource>();
                dragSource->setOwner( this );
                m_tree->setDragSource( dragSource );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabaseDialog::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();

            if( m_tree )
            {
                if( m_uiListener )
                {
                    m_tree->removeObjectListener( m_uiListener );
                }

                m_tree->clear();
                m_treeNodes.clear();

                if( ui )
                {
                    ui->removeElement( m_tree );
                }

                m_tree = nullptr;
            }

            if( m_addComponentButton )
            {
                if( ui )
                {
                    ui->removeElement( m_addComponentButton );
                }

                m_addComponentButton = nullptr;
            }

            if( m_selectButton )
            {
                if( ui )
                {
                    ui->removeElement( m_selectButton );
                }

                m_selectButton = nullptr;
            }

            if( m_buttonWindow )
            {
                if( ui )
                {
                    ui->removeElement( m_buttonWindow );
                }

                m_buttonWindow = nullptr;
            }

            if( m_uiListener )
            {
                m_uiListener->unload( nullptr );
                m_uiListener = nullptr;
            }

            if( m_treeWindow )
            {
                if( ui )
                {
                    ui->removeElement( m_treeWindow );
                }

                m_treeWindow = nullptr;
            }

            if( auto parentWindow = getParentWindow() )
            {
                if( ui )
                {
                    ui->removeElement( parentWindow );
                }

                setParentWindow( nullptr );
            }

            for( auto data : m_dataArray )
            {
                data->unload( nullptr );
            }

            m_dataArray.clear();

            EditorWindow::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabaseDialog::addResourceToTree( SmartPtr<ui::IUITreeNode> rootNode,
                                                    SmartPtr<IBuildDirector> resource )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto resourceType = getResourceType();

        WP_ASSERT( resource->isDerived<IBuildDirector>() );

        auto handle = resource->getHandle();
        auto name = resource->getName();

        auto uuid = handle->getUUIDAsString();
        //WP_ASSERT( !StringUtil::isNullOrEmpty( uuid ) );

        auto label = Path::getFileName( name );

        auto treeNode = m_tree->addNode();
        WP_ASSERT( treeNode );

        if( resourceType == ISharedObject::typeInfo() )
        {
            Util::setText( treeNode, label );

            auto data =
                factoryManager->make_ptr<ProjectTreeData>( "resource", "resource", resource, resource );
            treeNode->setNodeUserData( data );

            rootNode->addChild( treeNode );

            m_dataArray.emplace_back( data );
        }
        else if( resourceType == render::IMaterial::typeInfo() )
        {
            if( auto resourceProperties = resource->getProperties() )
            {
                String value;
                resourceProperties->getPropertyValue( "type", value );

                if( value == "Material" )
                {
                    Util::setText( treeNode, label );

                    auto data = factoryManager->make_ptr<ProjectTreeData>( "resource", "resource",
                                                                           resource, resource );
                    treeNode->setNodeUserData( data );

                    rootNode->addChild( treeNode );

                    m_dataArray.emplace_back( data );
                }
            }
        }
        else if( resourceType == scene::IGameActor::typeInfo() )
        {
            if( resource->isDerived<scene::IGameActor>() )
            {
                Util::setText( treeNode, name );

                auto data = factoryManager->make_ptr<ProjectTreeData>( "resource", "resource", resource,
                                                                       resource );
                treeNode->setNodeUserData( data );

                rootNode->addChild( treeNode );

                m_dataArray.emplace_back( data );
            }
        }
        else if( resourceType == scene::IComponent::typeInfo() )
        {
            if( resource->isDerived<scene::IComponent>() )
            {
                Util::setText( treeNode, name );

                auto data = factoryManager->make_ptr<ProjectTreeData>( "resource", "resource", resource,
                                                                       resource );
                treeNode->setNodeUserData( data );

                rootNode->addChild( treeNode );

                m_dataArray.emplace_back( data );
            }

            if( auto resourceProperties = resource->getProperties() )
            {
                String value;
                resourceProperties->getPropertyValue( "type", value );

                if( value == "Component" )
                {
                    Util::setText( treeNode, name );

                    auto data = factoryManager->make_ptr<ProjectTreeData>( "resource", "resource",
                                                                           resource, resource );
                    treeNode->setNodeUserData( data );

                    rootNode->addChild( treeNode );

                    m_dataArray.emplace_back( data );
                }
            }
        }
        else if( resourceType == render::ITexture::typeInfo() )
        {
            if( auto resourceProperties = resource->getProperties() )
            {
                String value;
                resourceProperties->getPropertyValue( "type", value );

                if( value == "Texture" )
                {
                    Util::setText( treeNode, label );

                    auto imageElement = ui->addElementByType<ui::IUIImage>();
                    WP_ASSERT( imageElement );

                    auto data = factoryManager->make_ptr<ProjectTreeData>( "resource", "resource",
                                                                           resource, resource );
                    treeNode->setNodeUserData( data );

                    rootNode->addChild( treeNode );

                    m_dataArray.emplace_back( data );
                }
            }
        }
    }

    void ResourceDatabaseDialog::updateSelection()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto jobQueue = applicationManager->getJobQueue();

        auto populateFunction = std::bind( &ResourceDatabaseDialog::populate, this );
        jobQueue->startJob( populateFunction );
    }

    void ResourceDatabaseDialog::populate()
    {
        if( !m_tree )
        {
            return;
        }

        if( m_tree )
        {
            m_tree->clear();
        }

        m_treeNodes.clear();

        for( auto data : m_dataArray )
        {
            data->unload( nullptr );
        }

        m_dataArray.clear();

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto resourceDatabase = applicationManager->getResourceDatabase();

        auto rootNode = m_tree->addRoot();
        WP_ASSERT( rootNode );
        rootNode->setExpanded( true );

        auto resourceType = getResourceType();

        auto resources = resourceDatabase->getResourceData();
        for( auto resource : resources )
        {
            auto parent = rootNode;

            auto name = resource->getName();

            auto path = Path::getFilePath( name );
            if( !StringUtil::isNullOrEmpty( path ) )
            {
                auto it = m_treeNodes.find( path );
                if( it != m_treeNodes.end() )
                {
                    parent = it->second;
                }
                else
                {
                    //parent = m_tree->addNode();
                    //WP_ASSERT( parent );

                    //Util::setText( parent, path );

                    //auto data = factoryManager->make_ptr<ProjectTreeData>( "resource", "resource",
                    //                                                       nullptr, nullptr );
                    //parent->setNodeUserData( data );

                    //rootNode->addChild( parent );

                    //m_treeNodes[path] = parent;

                    auto splitPaths = StringUtil::split( path, "/" );

                    auto previousNode = SmartPtr<ui::IUITreeNode>();
                    auto currentPath = String();
                    for( auto &splitPath : splitPaths )
                    {
                        if( splitPath == "." || splitPath == ".." )
                        {
                            continue;
                        }

                        if( !currentPath.empty() )
                        {
                            currentPath += "/";
                        }

                        currentPath += splitPath;

                        auto it = m_treeNodes.find( currentPath );
                        if( it == m_treeNodes.end() )
                        {
                            auto splitNode = m_tree->addNode();
                            WP_ASSERT( splitNode );

                            Util::setText( splitNode, splitPath );

                            auto data = factoryManager->make_ptr<ProjectTreeData>(
                                "resource", "resource", nullptr, nullptr );
                            splitNode->setNodeUserData( data );

                            if( !previousNode )
                            {
                                rootNode->addChild( splitNode );
                            }
                            else
                            {
                                previousNode->addChild( splitNode );
                            }

                            m_treeNodes[currentPath] = splitNode;

                            previousNode = splitNode;
                        }
                        else
                        {
                            previousNode = it->second;
                        }
                    }

                    auto it = m_treeNodes.find( currentPath );
                    if( it != m_treeNodes.end() )
                    {
                        parent = it->second;
                    }
                }
            }

            addResourceToTree( parent, resource );
        }
    }

    void ResourceDatabaseDialog::select()
    {
        WP_ASSERT( m_tree );

        auto treeNode = m_tree->getSelectedTreeNode();
        if( treeNode )
        {
            auto data = treeNode->getNodeUserData();
            if( data )
            {
                auto uuid = String();

                auto projectData = workphone::static_pointer_cast<ProjectTreeData>( data );
                if( auto resourceObject = projectData->getObjectData() )
                {
                    if( auto handle = resourceObject->getHandle() )
                    {
                        uuid = handle->getUUIDAsString();
                    }

                    if( StringUtil::isNullOrEmpty( uuid ) )
                    {
                        if( resourceObject->isDerived<IBuildDirector>() )
                        {
                            auto director =
                                workphone::static_pointer_cast<IBuildDirector>( resourceObject );
                            auto directorProperties = director->getProperties();
                            directorProperties->getPropertyValue( "uuid", uuid );
                        }
                    }
                }

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto selectionManager = applicationManager->getSelectionManager();
                WP_ASSERT( selectionManager );

                if( auto selected = getCurrentObject() )
                {
                    selectObject( selected, uuid );
                }
                else
                {
                    auto selection = selectionManager->getSelection();
                    for( auto selected : selection )
                    {
                        selectObject( selected, uuid );
                    }
                }
            }
        }
    }

    void ResourceDatabaseDialog::selectObject( SmartPtr<ISharedObject> selected, const String &uuid )
    {
        //WP_ASSERT(selected->isDerived<core::IPrototype>());

        if( selected->isDerived<scene::Material>() )
        {
            auto node = workphone::static_pointer_cast<scene::Material>( selected );

            auto properties = node->getProperties();
            if( properties )
            {
                properties->setProperty( m_propertyName, uuid );
                node->setProperties( properties );
            }
        }
        else if( selected->isDerived<render::IMaterialNode>() )
        {
            auto node = workphone::static_pointer_cast<render::IMaterialNode>( selected );

            auto properties = node->getProperties();
            if( properties )
            {
                properties->setProperty( m_propertyName, uuid );
                node->setProperties( properties );
            }

            auto material = node->getMaterial();
            if( material )
            {
                material->save();
            }
        }
        else if( selected->isDerived<IResource>() )
        {
            auto resource = workphone::static_pointer_cast<IResource>( selected );

            auto properties = resource->getProperties();
            if( properties )
            {
                properties->setProperty( m_propertyName, uuid );
                resource->setProperties( properties );
            }
        }
        else if( selected->isDerived<core::IPrototype>() )
        {
            auto prototype = workphone::static_pointer_cast<core::IPrototype>( selected );

            auto properties = prototype->getProperties();
            if( properties )
            {
                properties->setProperty( m_propertyName, uuid );
                prototype->setProperties( properties );
            }
        }
        else if( selected->isDerived<scene::IComponentEventListener>() )
        {
            auto prototype = workphone::static_pointer_cast<scene::IComponentEventListener>( selected );

            auto properties = prototype->getProperties();
            if( properties )
            {
                properties->setProperty( m_propertyName, uuid );
                prototype->setProperties( properties );
            }
        }
    }

    String ResourceDatabaseDialog::getSelectedObject() const
    {
        return m_selectedObject;
    }

    void ResourceDatabaseDialog::setSelectedObject( const String &selectedObject )
    {
        m_selectedObject = selectedObject;
    }

    SmartPtr<ui::IUITreeCtrl> ResourceDatabaseDialog::getTree() const
    {
        return m_tree;
    }

    void ResourceDatabaseDialog::setTree( SmartPtr<ui::IUITreeCtrl> tree )
    {
        m_tree = tree;
    }

    SmartPtr<ISharedObject> ResourceDatabaseDialog::getCurrentObject() const
    {
        return m_currentObject;
    }

    void ResourceDatabaseDialog::setCurrentObject( SmartPtr<ISharedObject> currentObject )
    {
        m_currentObject = currentObject;
    }

    String ResourceDatabaseDialog::getPropertyName() const
    {
        return m_propertyName;
    }

    void ResourceDatabaseDialog::setPropertyName( const String &propertyName )
    {
        m_propertyName = propertyName;
    }

    void ResourceDatabaseDialog::handleTreeSelectionChanged()
    {
        // auto selectedId = event.GetItem();
        // auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
        // if (data)
        //{
        //	auto factory = fb::static_pointer_cast<IFactory>(data->getOwnerData());
        //	auto selectedObject = factory->getObjectType();
        //	setSelectedObject(selectedObject);
        // }
    }

    void ResourceDatabaseDialog::handleTreeSelectionActivated()
    {
        // auto selectedId = event.GetItem();
        // auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
        // if (data)
        //{
        //	auto factory = fb::static_pointer_cast<IFactory>(data->getOwnerData());
        //	auto selectedObject = factory->getObjectType();
        //	setSelectedObject(selectedObject);
        // }

        // EndModal(wxID_OK);
    }

    Parameter ResourceDatabaseDialog::UIElementListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleSelection )
        {
            auto owner = getOwner();
            auto tree = owner->getTree();
            auto selectedNode = tree->getSelectedTreeNode();

            auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( sender );
            auto elementId = static_cast<WidgetId>( element->getElementId() );
            switch( elementId )
            {
            case AddComponent:
            {
                owner->populate();
            }
            break;
            case Select:
            {
                owner->select();
            }
            break;
            }
        }

        return {};
    }

    ResourceDatabaseDialog *ResourceDatabaseDialog::UIElementListener::getOwner() const
    {
        return m_owner;
    }

    void ResourceDatabaseDialog::UIElementListener::setOwner( ResourceDatabaseDialog *owner )
    {
        m_owner = owner;
    }

    ResourceDatabaseDialog::UIElementListener::UIElementListener() = default;

    ResourceDatabaseDialog::UIElementListener::~UIElementListener() = default;

    Parameter ResourceDatabaseDialog::DragSource::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::handleDrag )
        {
            auto dataStr = handleDrag( Vector2I::zero(), sender );
            return Parameter( dataStr );
        }

        return {};
    }

    String ResourceDatabaseDialog::DragSource::handleDrag( const Vector2I &position,
                                                           SmartPtr<ui::IUIElement> element )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto selectionManager = applicationManager->getSelectionManager();

        if( element->isDerived<ui::IUITreeNode>() )
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
                        data->setProperty( "resourceId", handle->getInstanceId() );
                        data->setProperty( "resourceUUID", uuid );
                    }
                }

                return DataUtil::toString( data.get(), true );
            }
        }

        return {};
    }

    ResourceDatabaseDialog *ResourceDatabaseDialog::DragSource::getOwner() const
    {
        return m_owner;
    }

    void ResourceDatabaseDialog::DragSource::setOwner( ResourceDatabaseDialog *owner )
    {
        m_owner = owner;
    }

    ResourceDatabaseDialog::DragSource::DragSource() = default;

    ResourceDatabaseDialog::DragSource::~DragSource() = default;

}  // namespace workphone::editor
