#include <EditorPCH.hpp>
#include <ui/FileWindow.hpp>
#include <editor/EditorManager.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/UIManager.hpp>
#include <commands/AddNewScriptCmd.hpp>
#include <commands/AddResourceCmd.hpp>
#include <EditorTypes.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>

enum
{
    TREE_ITEM_STATE_NOT_FOUND = 0,
    TREE_ITEM_STATE_EXPANDED = 1,
    TREE_ITEM_STATE_NOT_EXPANDED = 2
};

namespace workphone::editor
{
    FileWindow::FileWindow() = default;

    FileWindow::~FileWindow() = default;

    void FileWindow::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto parent = getParent();

        auto parentWindow = ui->addElementByType<ui::IUIWindow>();
        setParentWindow( parentWindow );

        if( parent )
        {
            parent->addChild( parentWindow );
        }

        auto treeCtrl = ui->addElementByType<ui::IUITreeCtrl>();
        parentWindow->addChild( treeCtrl );
        m_tree = treeCtrl;

        setLoadingState( LoadingState::Loaded );
    }

    void FileWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            if( m_tree )
            {
                ui->removeElement( m_tree );
                m_tree = nullptr;
            }

            if( auto parentWindow = getParentWindow() )
            {
                ui->removeElement( parentWindow );
                setParentWindow( nullptr );
            }

            EditorWindow::unload( nullptr );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FileWindow::update( time_interval t, time_interval dt )
    {
    }

    void FileWindow::addFolderToTree( SmartPtr<ui::IUITreeNode> parent,
                                      SmartPtr<IFolderExplorer> listing )
    {
    }

    void FileWindow::buildTree()
    {
        try
        {
            WP_ASSERT( m_tree );

            if( m_tree )
            {
                m_tree->clear();
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();

            auto editorManager = EditorManager::getSingletonPtr();
            WP_ASSERT( editorManager );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );

            auto currentScene = sceneManager->getCurrentScene();
            WP_ASSERT( currentScene );

            if( currentScene )
            {
                auto project = editorManager->getProject();
                WP_ASSERT( project );

                //auto appTemplate = project->getApplicationTemplate();

                auto sceneName = currentScene->getLabel();

                // ProjectTreeData data("project", "project", project, project);
                // ProjectTreeData scriptsData(m_applicationMenu, "project", "filter", project,
                // m_parentFilter);

                auto rootNode = m_tree->addRoot();
                WP_ASSERT( rootNode );

                auto projectFolder = applicationManager->getProjectPath();
                if( !StringUtil::isNullOrEmpty( projectFolder ) )
                {
                    auto folderListing = fileSystem->getFolderListing( projectFolder + "/Assets" );
                    if( folderListing )
                    {
                        auto folderName = folderListing->getFolderName();

                        auto node = m_tree->addNode();
                        WP_ASSERT( node );
                        Util::setText( node, folderName );

                        rootNode->addChild( node );

                        auto subFolders = folderListing->getSubFolders();
                        for( auto &subFolder : subFolders )
                        {
                            addFolderToTree( rootNode, subFolder );
                        }
                    }
                }

                m_tree->expand( rootNode );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FileWindow::handleTreeSelectionChanged()
    {
        ////if(m_tree->HasFocus())
        //{
        //	auto selectedId = event.GetItem();
        //	if (!selectedId)
        //	{
        //		return;
        //	}

        //	if (selectedId.IsOk())
        //	{
        //		m_lastSelectedItem = selectedId;
        //	}

        //	auto applicationManager = core::IApplicationManager::instance();
        //	auto selectionManager = applicationManager->getSelectionManager();

        //	auto editorManager = EditorManager::getSingletonPtr();
        //	auto project = editorManager->getProject();
        //	auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
        //	if(data)
        //	{
        //		auto ownerType = data->getOwnerType();
        //		if(ownerType == "entity")
        //		{
        //			SmartPtr<EntityTemplate> entityTemplate = data->getOwnerData();
        //			editorManager->getProject()->setSelectedEntityTemplate(entityTemplate);
        //		}
        //		else if (ownerType == "file")
        //		{
        //			auto path = data->getObjectType();

        //			auto fileSelection = fb::make_ptr<FileSelection>();
        //			fileSelection->setFilePath(path);

        //			auto ext = Path::getFileExtension(path);
        //			ext = StringUtil::cleanupPath(ext);
        //			ext = StringUtil::make_lower(ext);

        //			if (ext == ".fbx" || ext == ".fbmeshbin")
        //			{
        //				auto graphicsSystem = applicationManager->getGraphicsSystem();
        //				auto meshManager = applicationManager->getMeshManager();
        //				auto meshResource = meshManager->loadFromFile(path);

        //				selectionManager->clearSelection();
        //				selectionManager->addSelectedObject(meshResource);
        //			}
        //			else
        //			{
        //				selectionManager->clearSelection();
        //				selectionManager->addSelectedObject(fileSelection);
        //			}

        //			//editorManager->previewAsset(path);
        //		}
        //		else if(ownerType == "mesh")
        //		{
        //			SmartPtr<MeshTemplate> meshTemplate = data->getObjectData();

        //			SmartPtr<SceneViewManager> meshViewManager =
        // editorManager->getSceneViewManager();
        // meshViewManager->setMeshTemplate(meshTemplate);
        //		}
        //		else
        //		{
        //			editorManager->getProject()->setSelectedEntityTemplate(nullptr);
        //		}
        //	}

        //	//wxTreeItemId parentId = m_tree->GetItemParent(selectedId);
        //	//if(!parentId)
        //	//	return;
        //
        //	//wxString parentType = m_tree->GetItemText(parentId);
        //	//wxString componentName = m_tree->GetItemText(selectedId);

        //	EntityTreeItemSelectedPtr msg(new EntityTreeItemSelected);

        //	//msg->setComponentType(parentType.mb_str().data());
        //	//msg->setComponentName(componentName.mb_str().data());

        //	if(data)
        //	{
        //		m_selectedObject = data->getObjectData();
        //		if (m_selectedObject)
        //		{
        //			const String& editableType = m_selectedObject->getEditableType();
        //			if (editableType == ("EntityTemplate"))
        //			{
        //				m_selectedEntity = m_selectedObject;
        //			}

        //			msg->setSelectedObject(m_selectedObject);
        //		}
        //	}
        //	else
        //	{
        //		m_selectedObject = nullptr;
        //	}

        //	auto messageManager = editorManager->getMessageManager();
        //	messageManager->postMessage(msg);

        //	auto ui = editorManager->getUI();
        //	ui->updateSelection();
        //}
    }

    void FileWindow::handleTreeSelectionActivated()
    {
        // if(m_tree->HasFocus())
        //{
        //	auto selectedId = event.GetItem();
        //	if (!selectedId)
        //	{
        //		return;
        //	}

        //	if (selectedId.IsOk())
        //	{
        //		m_lastSelectedItem = selectedId;
        //	}

        //	auto applicationManager = core::IApplicationManager::instance();
        //	auto selectionManager = applicationManager->getSelectionManager();
        //	auto editorManager = EditorManager::getSingletonPtr();
        //	auto project = editorManager->getProject();
        //	auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
        //	if (data)
        //	{
        //		auto ownerType = data->getOwnerType();
        //		if (ownerType == "entity")
        //		{
        //			SmartPtr<EntityTemplate> entityTemplate = data->getOwnerData();
        //			editorManager->getProject()->setSelectedEntityTemplate(entityTemplate);
        //		}
        //		else if (ownerType == "file")
        //		{
        //			auto path = data->getObjectType();

        //			//auto fileSelection = fb::make_ptr<FileSelection>();
        //			//fileSelection->setFilePath(path);

        //			//selectionManager->clearSelection();
        //			//selectionManager->addSelectedObject(fileSelection);

        //			editorManager->previewAsset(path);
        //		}
        //		else if (ownerType == "mesh")
        //		{
        //			SmartPtr<MeshTemplate> meshTemplate = data->getObjectData();

        //			SmartPtr<SceneViewManager> meshViewManager =
        // editorManager->getSceneViewManager();
        // meshViewManager->setMeshTemplate(meshTemplate);
        //		}
        //		else
        //		{
        //			editorManager->getProject()->setSelectedEntityTemplate(nullptr);
        //		}
        //	}

        //	//wxTreeItemId parentId = m_tree->GetItemParent(selectedId);
        //	//if(!parentId)
        //	//	return;

        //	//wxString parentType = m_tree->GetItemText(parentId);
        //	//wxString componentName = m_tree->GetItemText(selectedId);

        //	EntityTreeItemSelectedPtr msg(new EntityTreeItemSelected);

        //	//msg->setComponentType(parentType.mb_str().data());
        //	//msg->setComponentName(componentName.mb_str().data());

        //	if (data)
        //	{
        //		m_selectedObject = data->getObjectData();
        //		if (m_selectedObject)
        //		{
        //			const String& editableType = m_selectedObject->getEditableType();
        //			if (editableType == ("EntityTemplate"))
        //			{
        //				m_selectedEntity = m_selectedObject;
        //			}

        //			msg->setSelectedObject(m_selectedObject);
        //		}
        //	}
        //	else
        //	{
        //		m_selectedObject = nullptr;
        //	}

        //	auto messageManager = editorManager->getMessageManager();
        //	messageManager->postMessage(msg);

        //	auto ui = editorManager->getUI();
        //	ui->updateSelection();
        //}
    }

    void FileWindow::handleTreeDragStart()
    {
        // auto selectedId = event.GetItem();
        // if (!selectedId)
        //{
        //	return;
        // }

        // auto applicationManager = core::IApplicationManager::instance();
        // auto selectionManager = applicationManager->getSelectionManager();

        // data::drag_drop_data data;

        // auto itemData = (ProjectTreeData*)m_tree->GetItemData(selectedId);
        // if (itemData)
        //{
        //	auto ownerType = itemData->getOwnerType();
        //	if (ownerType == "file")
        //	{
        //		auto path = itemData->getObjectType();
        //		data.filePath = path;

        //		auto fileSelection = fb::make_ptr<FileSelection>();
        //		fileSelection->setFilePath(path);

        //		selectionManager->clearSelection();
        //		selectionManager->addSelectedObject(fileSelection);
        //	}
        //}

        // auto selection = selectionManager->getSelected();
        // for (auto selected : selection)
        //{
        //	if (selected->isExactly<FileSelection>())
        //	{
        //		auto fileSelection = fb::static_pointer_cast<FileSelection>(selected);
        //	}
        // }

        // auto dataStr = DataUtil::toString(&data, true);

        // wxTextDataObject my_data(dataStr.c_str());

        // wxDropSource dragSource(m_tree);
        // dragSource.SetData(my_data);
        // wxDragResult result = dragSource.DoDragDrop(true);
    }

    void FileWindow::handleContextMenu()
    {
        // m_tree->PopupMenu(m_defaultMenu);
        // event.Skip();
    }

    void FileWindow::saveTreeState()
    {
        //// clear map
        // treeState.clear();

        //// reset selected item
        // m_newSelectedItem = NULL;

        // wxTreeItemId itemId = m_tree->GetRootItem();
        // String parent = "";
        // if (itemId)
        //{
        //	saveItemState(parent, itemId);
        // }
    }

    void FileWindow::saveItemState()
    {
        //// make item name
        // String itemName = String(m_tree->GetItemText(itemId));
        // if(parent != "")
        //	itemName = parent + "/" + itemName;

        //// get expanded state
        // bool isExpanded = false;
        // if(m_tree->ItemHasChildren(itemId))
        //	isExpanded = m_tree->IsExpanded(itemId);

        ////get selected state
        // if(m_tree->IsSelected(itemId))
        //	m_newSelectedItem = itemId;

        //// add item to map
        // treeState.insert(std::map<String, bool>::value_type(itemName, isExpanded));
        //
        //// parse childes
        // wxTreeItemIdValue cookie;
        // wxTreeItemId childrenItem = m_tree->GetFirstChild(itemId, cookie);
        // while(childrenItem.IsOk())
        //{
        //	saveItemState(itemName, childrenItem);
        //	childrenItem = m_tree->GetNextChild(itemId, cookie);
        // }
    }

    void FileWindow::restoreTreeState()
    {
        // wxTreeItemId itemId = m_tree->GetRootItem();

        //// restore items
        // String parent = "";
        // if (itemId)
        //{
        //	restoreItemState(parent, itemId, false);
        // }

        // if(m_newSelectedItem)
        //{
        //	m_tree->EnsureVisible(m_newSelectedItem);
        //	m_tree->SelectItem(m_newSelectedItem);
        // }
    }

    void FileWindow::restoreItemState()
    {
        //// make item name
        // String itemName = String(m_tree->GetItemText(itemId));
        // if(parent != "")
        //	itemName = parent + "/" + itemName;

        //// get item state from map
        // int state = getItemState(itemName);

        // bool isExpanded = false;
        // bool showItem = false;
        // if(state != TREE_ITEM_STATE_NOT_FOUND)
        //{
        //	isExpanded = (state == TREE_ITEM_STATE_EXPANDED) ? true : false;
        //	parentWasNew = false;
        // }
        // else
        //{
        //	showItem = parentWasNew ? false : true;
        //	parentWasNew = true;
        // }

        //// set item state
        // if(isExpanded)
        //	m_tree->Expand(itemId);

        //// show item
        // if(showItem)
        //{
        //	m_newSelectedItem = itemId;
        // }

        //// parse childes
        // wxTreeItemIdValue cookie;
        // wxTreeItemId childrenItem = m_tree->GetFirstChild(itemId, cookie);
        // while(childrenItem.IsOk())
        //{
        //	restoreItemState(itemName, childrenItem, parentWasNew);
        //	childrenItem = m_tree->GetNextChild(itemId, cookie);
        // }
    }

    void FileWindow::addScript()
    {
        // auto window = getWindow();

        // AddNamedDialog dialog(window);
        // if (dialog.ShowModal() == wxID_OK)
        //{
        //	auto label = dialog.getLabel();

        //	auto applicationManager = core::IApplicationManager::instance();
        //	auto commandManager = applicationManager->getCommandManager();

        //	auto editorManager = EditorManager::getSingletonPtr();
        //	auto project = editorManager->getProject();
        //	auto projectFolder = project->getSelectedProjectPath();

        //	auto cmd = fb::make_ptr<AddNewScriptCmd>();
        //	cmd->setPath(projectFolder);
        //	cmd->setFileName(label);
        //	commandManager->add(cmd);
        //}
    }

    void FileWindow::addMaterial()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto cmd = factoryManager->make_ptr<AddResourceCmd>();

            auto materialFileName = String( "NewMaterial.mat" );
            auto currentPath = getPath();
            auto filePath = currentPath + materialFileName;

            cmd->setResourceType( AddResourceCmd::ResourceType::Material );
            cmd->setFilePath( filePath );
            cmd->execute();

            this->buildTree();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FileWindow::addScene()
    {
    }

    void FileWindow::refresh()
    {
        buildTree();
    }

    int FileWindow::getItemState( String itemName )
    {
        std::map<String, bool>::iterator it;
        for( it = treeState.begin(); it != treeState.end(); ++it )
        {
            String name = ( *it ).first;
            if( name == itemName )
            {
                return ( *it ).second ? TREE_ITEM_STATE_EXPANDED : TREE_ITEM_STATE_NOT_EXPANDED;
            }
        }

        return TREE_ITEM_STATE_NOT_FOUND;
    }

    SmartPtr<ISharedObject> FileWindow::getSelectedObject() const
    {
        return m_selectedObject;
    }

    void FileWindow::setSelectedObject( SmartPtr<ISharedObject> val )
    {
        m_selectedObject = val;
    }

    String FileWindow::getPath() const
    {
        return m_path;
    }

    void FileWindow::setPath( const String &val )
    {
        m_path = val;
    }
}  // namespace workphone::editor
