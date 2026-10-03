#include <GameEditorPCH.hpp>
#include <ui/FileWindow.hpp>
#include <editor/ComponentTemplateMgr.hpp>
#include <editor/FileSelection.hpp>
#include <editor/EditorMessages.hpp>
#include <editor/EditorManager.hpp>
#include <editor/SceneViewManager.hpp>

#include <FBCore/Base/DataUtil.hpp>
#include <FBData/FBData.hpp>
#include <FBApplication/Script/ScriptGenerator.hpp>

#include <ui/AddNamedDialog.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/UIManager.hpp>
#include <ui/NewEventDialog.hpp>
#include <commands/AddNewScriptCmd.hpp>
#include <commands/AddResourceCmd.hpp>
#include <GameEditorTypes.hpp>
#include <core/MessageManager.hpp>
#include <editor/Project.hpp>
#include <editor/ProjectManager.hpp>
#include <FBState/FBState.hpp>

#include <FBCore/FBCoreHeaders.hpp>
#include <wx/scrolwin.hpp>
#include <wx/treectrl.hpp>



#define TREE_ITEM_STATE_NOT_FOUND		0
#define TREE_ITEM_STATE_EXPANDED		1
#define TREE_ITEM_STATE_NOT_EXPANDED	2



namespace fb
{
	namespace editor
	{

	
	
		//-------------------------------------------------
		FileWindow::FileWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
			const wxSize& size, long style, const wxValidator& validator, const wxString& name)
		{
			m_lastSelectedItem = 0;

			auto window = new wxScrolledWindow(parent, id, pos, size, style);
			setWindow(window);
	
			auto baseSizer = new wxBoxSizer( wxVERTICAL );
			window->SetSizer(baseSizer);
	
			m_tree = new wxTreeCtrl(window, ENTITY_TREE_ID);
			baseSizer->Add(m_tree, 1, wxEXPAND);

			m_tree->SetDropTarget(new DnDText(this));
			//m_tree->SetDropTarget(new DnDFile(this));

			m_tree->Bind(wxEVT_TREE_SEL_CHANGED, &FileWindow::handleTreeSelectionChanged, this, -1);
			m_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &FileWindow::handleTreeSelectionActivated, this, -1);
			m_tree->Bind(wxEVT_TREE_BEGIN_DRAG, &FileWindow::handleTreeDragStart, this, -1);

			m_defaultMenu = new wxMenu();
			m_defaultAddMenu = new wxMenu();

			m_defaultAddMenu->Append(ADD_SCRIPT, "Add Script");
			m_defaultAddMenu->Append(ADD_MATERIAL, "Add Material");
			m_defaultAddMenu->Append(ADD_SCENE, "Add Empty Scene");
			m_defaultMenu->AppendSubMenu(m_defaultAddMenu, "Add");
			m_defaultMenu->Append(REMOVE_COMPONENT, "Remove");
			m_defaultMenu->AppendSeparator();
			m_defaultMenu->Append(REFRESH, "Refresh");

			m_tree->Bind(wxEVT_RIGHT_DOWN, &FileWindow::handleContextMenu, this, -1);

			m_defaultAddMenu->Bind(wxEVT_MENU, &FileWindow::addScript, this, ADD_SCRIPT);
			m_defaultAddMenu->Bind(wxEVT_MENU, &FileWindow::addMaterial, this, ADD_MATERIAL);
			m_defaultAddMenu->Bind(wxEVT_MENU, &FileWindow::addScene, this, ADD_SCENE);
			m_defaultMenu->Bind(wxEVT_MENU, &FileWindow::refresh, this, REFRESH);
		}
	
	
	
		//-------------------------------------------------
		FileWindow::~FileWindow()
		{
			FB_SAFE_DELETE(m_defaultMenu);
		}



		//-------------------------------------------------
		void FileWindow::update(time_interval t, time_interval dt)
		{
		}
	


		//-------------------------------------------------
		void FileWindow::handleMessage( SmartPtr<IMessage> message )
		{
			String type = message->getType();
			if(type==("SelectEntityTemplate"))
			{
				buildTree();
			}
			else if(type==("LoadProjectMsg"))
			{
				buildTree();
			}
		}



		//-------------------------------------------------
		void FileWindow::addFolderToTree(wxTreeItemId parent, SmartPtr<IFolderListing> listing)
		{
			auto editorManager = EditorManager::getSingletonPtr();
			auto project = editorManager->getProject();
			ProjectTreeData data("project", "project", project, project);

			auto folderName = listing->getFolderName();
			auto folderTreeId = m_tree->AppendItem(parent, folderName, -1, -1, new ProjectTreeData(data));
			
			auto subFolders = listing->getSubFolders();
			for (auto& subFolder : subFolders)
			{
				addFolderToTree(folderTreeId, subFolder);
			}
		}
	


		//-------------------------------------------------
		void FileWindow::buildTree()
		{
			try
			{
				m_tree->DeleteAllItems();
	
				auto applicationManager = IApplicationManager::instance();
				auto fileSystem = applicationManager->getFileSystem();
	
				auto editorManager = EditorManager::getSingletonPtr();
				auto project = editorManager->getProject();
				auto appTemplate = project->getApplicationTemplate();	
				
				ProjectTreeData scriptsData(m_defaultMenu, "project", "filter", project, m_parentFilter);
	
				auto rootId = m_tree->AddRoot("Project", -1, -1, new ProjectTreeData(scriptsData));
	
				auto projectFolder = applicationManager->getProjectPath();
				if (StringUtil::isNullOrEmpty(projectFolder))
				{
					projectFolder = Path::getWorkingDirectory();
				}

				auto selectedProjectFolder = project->getSelectedProjectPath();
				if (StringUtil::isNullOrEmpty(selectedProjectFolder))
				{
					selectedProjectFolder = Path::getWorkingDirectory();
				}

				auto folderListing = fileSystem->getDirectoryListing(selectedProjectFolder);
				if (folderListing)
				{
					auto files = folderListing->getFiles();
					for (auto& file : files)
					{
						auto filePath = Path::getRelativePath(projectFolder, file);
						filePath = StringUtil::cleanupPath(filePath);

						ProjectTreeData data("file", filePath, project, project);

						auto fileName = Path::getFileName(filePath);
						wxTreeItemId folderTreeId = m_tree->AppendItem(rootId, fileName, -1, -1, new ProjectTreeData(data));
					}
				}

				m_tree->Expand(rootId);
			}
			catch (Exception& e)
			{
				wxMessageBox(e.what());
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}


	
		//-------------------------------------------------
		void FileWindow::handleTreeSelectionChanged( wxTreeEvent& event )
		{
			//if(m_tree->HasFocus())
			{
				auto selectedId = event.GetItem();
				if (!selectedId)
				{
					return;
				}
	
				if (selectedId.IsOk())
				{
					m_lastSelectedItem = selectedId;
				}
	
				auto applicationManager = IApplicationManager::instance();
				auto selectionManager = applicationManager->getSelectionManager();

				auto editorManager = EditorManager::getSingletonPtr();
				auto project = editorManager->getProject();
				auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
				if(data)
				{
					auto ownerType = data->getOwnerType();
					if(ownerType == "entity")
					{
						SmartPtr<EntityTemplate> entityTemplate = data->getOwnerData();
						editorManager->getProject()->setSelectedEntityTemplate(entityTemplate);
					}
					else if (ownerType == "file")
					{
						auto path = data->getObjectType();

						auto fileSelection = fb::make_ptr<FileSelection>();
						fileSelection->setFilePath(path);

						auto ext = Path::getFileExtension(path);
						ext = StringUtil::cleanupPath(ext);
						ext = StringUtil::make_lower(ext);

						if (ext == ".fbx" || ext == ".fbmeshbin")
						{
							auto graphicsSystem = applicationManager->getGraphicsSystem();
							auto meshManager = applicationManager->getMeshManager();
							auto meshResource = meshManager->loadFromFile(path);

							selectionManager->clearSelection();
							selectionManager->addSelectedObject(meshResource);
						}
						else
						{
							selectionManager->clearSelection();
							selectionManager->addSelectedObject(fileSelection);
						}

						//editorManager->previewAsset(path);
					}
					else if(ownerType == "mesh")
					{
						SmartPtr<MeshTemplate> meshTemplate = data->getObjectData();

						SceneViewManagerPtr meshViewManager = editorManager->getSceneViewManager();
						meshViewManager->setMeshTemplate(meshTemplate);
					}
					else
					{
						editorManager->getProject()->setSelectedEntityTemplate(nullptr);
					}
				}
	
				//wxTreeItemId parentId = m_tree->GetItemParent(selectedId);
				//if(!parentId)
				//	return;
			
				//wxString parentType = m_tree->GetItemText(parentId);
				//wxString componentName = m_tree->GetItemText(selectedId);	
	
				EntityTreeItemSelectedPtr msg(new EntityTreeItemSelected);
	
				//msg->setComponentType(parentType.mb_str().data());
				//msg->setComponentName(componentName.mb_str().data());
	
				if(data)
				{
					m_selectedObject = data->getObjectData();
					if (m_selectedObject)
					{
						const String& editableType = m_selectedObject->getEditableType();
						if (editableType == ("EntityTemplate"))
						{
							m_selectedEntity = m_selectedObject;
						}

						msg->setSelectedObject(m_selectedObject);
					}
				}
				else
				{
					m_selectedObject = nullptr;
				}
	
				auto messageManager = editorManager->getMessageManager();
				messageManager->postMessage(msg);

				auto ui = editorManager->getUI();
				ui->updateSelection();
			}
		}
	


		//-------------------------------------------------
		void FileWindow::handleTreeSelectionActivated(wxTreeEvent& event)
		{
			//if(m_tree->HasFocus())
			{
				auto selectedId = event.GetItem();
				if (!selectedId)
				{
					return;
				}

				if (selectedId.IsOk())
				{
					m_lastSelectedItem = selectedId;
				}

				auto applicationManager = IApplicationManager::instance();
				auto selectionManager = applicationManager->getSelectionManager();
				auto editorManager = EditorManager::getSingletonPtr();
				auto project = editorManager->getProject();
				auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
				if (data)
				{
					auto ownerType = data->getOwnerType();
					if (ownerType == "entity")
					{
						SmartPtr<EntityTemplate> entityTemplate = data->getOwnerData();
						editorManager->getProject()->setSelectedEntityTemplate(entityTemplate);
					}
					else if (ownerType == "file")
					{
						auto path = data->getObjectType();

						//auto fileSelection = fb::make_ptr<FileSelection>();
						//fileSelection->setFilePath(path);

						//selectionManager->clearSelection();
						//selectionManager->addSelectedObject(fileSelection);

						editorManager->previewAsset(path);
					}
					else if (ownerType == "mesh")
					{
						SmartPtr<MeshTemplate> meshTemplate = data->getObjectData();

						SceneViewManagerPtr meshViewManager = editorManager->getSceneViewManager();
						meshViewManager->setMeshTemplate(meshTemplate);
					}
					else
					{
						editorManager->getProject()->setSelectedEntityTemplate(nullptr);
					}
				}

				//wxTreeItemId parentId = m_tree->GetItemParent(selectedId);
				//if(!parentId)
				//	return;

				//wxString parentType = m_tree->GetItemText(parentId);
				//wxString componentName = m_tree->GetItemText(selectedId);	

				EntityTreeItemSelectedPtr msg(new EntityTreeItemSelected);

				//msg->setComponentType(parentType.mb_str().data());
				//msg->setComponentName(componentName.mb_str().data());

				if (data)
				{
					m_selectedObject = data->getObjectData();
					if (m_selectedObject)
					{
						const String& editableType = m_selectedObject->getEditableType();
						if (editableType == ("EntityTemplate"))
						{
							m_selectedEntity = m_selectedObject;
						}

						msg->setSelectedObject(m_selectedObject);
					}
				}
				else
				{
					m_selectedObject = nullptr;
				}

				auto messageManager = editorManager->getMessageManager();
				messageManager->postMessage(msg);

				auto ui = editorManager->getUI();
				ui->updateSelection();
			}
		}
	

		//-------------------------------------------------
		void FileWindow::handleTreeDragStart(wxTreeEvent& event)
		{
			auto selectedId = event.GetItem();
			if (!selectedId)
			{
				return;
			}

			auto applicationManager = IApplicationManager::instance();
			auto selectionManager = applicationManager->getSelectionManager();

			data::drag_drop_data data;

			auto itemData = (ProjectTreeData*)m_tree->GetItemData(selectedId);
			if (itemData)
			{
				auto ownerType = itemData->getOwnerType();
				if (ownerType == "file")
				{
					auto path = itemData->getObjectType();				
					data.filePath = path;

					auto fileSelection = fb::make_ptr<FileSelection>();
					fileSelection->setFilePath(path);

					selectionManager->clearSelection();
					selectionManager->addSelectedObject(fileSelection);
				}
			}

			//auto selection = selectionManager->getSelection();
			//for (auto selected : selection)
			//{
			//	if (selected->isExactly<FileSelection>())
			//	{
			//		auto fileSelection = fb::static_pointer_cast<FileSelection>(selected);
			//	}
			//}

			auto dataStr = DataUtil::toString(&data, true);

			wxTextDataObject my_data(dataStr.c_str());

			wxDropSource dragSource(m_tree);
			dragSource.SetData(my_data);
			wxDragResult result = dragSource.DoDragDrop(true);
		}



		//-------------------------------------------------
		void FileWindow::handleContextMenu(wxMouseEvent& event )
		{		
			m_tree->PopupMenu(m_defaultMenu);
			event.Skip();
		}
	
	
			
		//-------------------------------------------------
		void FileWindow::saveTreeState()
		{


			// clear map
			treeState.clear();
	
			// reset selected item
			m_newSelectedItem = NULL;
	
			wxTreeItemId itemId = m_tree->GetRootItem();
			String parent = "";
			if (itemId)
			{
				saveItemState(parent, itemId);
			}
		}
	
	
	
		//-------------------------------------------------
		void FileWindow::saveItemState(String parent, wxTreeItemId itemId)
		{


			// make item name
			String itemName = String(m_tree->GetItemText(itemId));
			if(parent != "")
				itemName = parent + "/" + itemName;
	
			// get expanded state
			bool isExpanded = false;
			if(m_tree->ItemHasChildren(itemId))
				isExpanded = m_tree->IsExpanded(itemId);
	
			//get selected state
			if(m_tree->IsSelected(itemId))
				m_newSelectedItem = itemId;
	
			// add item to map
			treeState.insert(std::map<String, bool>::value_type(itemName, isExpanded));
			
			// parse childes
			wxTreeItemIdValue cookie;
			wxTreeItemId childrenItem = m_tree->GetFirstChild(itemId, cookie);
			while(childrenItem.IsOk())
			{
				saveItemState(itemName, childrenItem);
				childrenItem = m_tree->GetNextChild(itemId, cookie);
			}
		}
	
	
	
		//-------------------------------------------------
		void FileWindow::restoreTreeState()
		{


			wxTreeItemId itemId = m_tree->GetRootItem();
	
			// restore items 
			String parent = "";
			if (itemId)
			{
				restoreItemState(parent, itemId, false);
			}
	
			if(m_newSelectedItem)
			{
				m_tree->EnsureVisible(m_newSelectedItem);
				m_tree->SelectItem(m_newSelectedItem);
			}
		}
	
	
	
		//-------------------------------------------------
		void FileWindow::restoreItemState(String parent, wxTreeItemId itemId, bool parentWasNew)
		{


			// make item name
			String itemName = String(m_tree->GetItemText(itemId));
			if(parent != "")
				itemName = parent + "/" + itemName;
	
			// get item state from map
			int state = getItemState(itemName);
	
			bool isExpanded = false;
			bool showItem = false;
			if(state != TREE_ITEM_STATE_NOT_FOUND)
			{
				isExpanded = (state == TREE_ITEM_STATE_EXPANDED) ? true : false;
				parentWasNew = false;
			}
			else
			{
				showItem = parentWasNew ? false : true;
				parentWasNew = true;
			}
	
			// set item state
			if(isExpanded)
				m_tree->Expand(itemId);
	
			// show item
			if(showItem)
			{
				m_newSelectedItem = itemId;
			}
	
			// parse childes
			wxTreeItemIdValue cookie;
			wxTreeItemId childrenItem = m_tree->GetFirstChild(itemId, cookie);
			while(childrenItem.IsOk())
			{
				restoreItemState(itemName, childrenItem, parentWasNew);
				childrenItem = m_tree->GetNextChild(itemId, cookie);
			}
		}
	

	
		//-------------------------------------------------
		void FileWindow::addScript(wxCommandEvent& event)
		{
			auto window = getWindow();

			AddNamedDialog dialog(window);
			if (dialog.ShowModal() == wxID_OK)
			{
				auto label = dialog.getLabel();

				auto applicationManager = IApplicationManager::instance();
				auto commandManager = applicationManager->getCommandManager();

				auto editorManager = EditorManager::getSingletonPtr();
				auto project = editorManager->getProject();
				auto projectFolder = project->getSelectedProjectPath();

				auto cmd = fb::make_ptr<AddNewScriptCmd>();
				cmd->setPath(projectFolder);
				cmd->setFileName(label);
				commandManager->addCommand(cmd);
			}
		}



		void FileWindow::addMaterial(wxCommandEvent& event)
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto factoryManager = applicationManager->getFactoryManager();
				FB_ASSERT(factoryManager);

				auto cmd = factoryManager->make_ptr<AddResourceCmd>();

				auto materialFileName = String("NewMaterial.mat");
				auto currentPath = getPath();
				auto filePath = currentPath + materialFileName;

				cmd->setResourceType(AddResourceCmd::ResourceType::Material);
				cmd->setFilePath(filePath);
				cmd->execute();

				this->buildTree();
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//-------------------------------------------------
		void FileWindow::addScene(wxCommandEvent& event)
		{

		}



		//-------------------------------------------------
		void FileWindow::refresh(wxCommandEvent& event)
		{
			buildTree();
		}



		//-------------------------------------------------
		int FileWindow::getItemState(String itemName)
		{
			std::map<String, bool>::iterator it;
			for(it = treeState.begin(); it != treeState.end(); it++)
			{
				String name = (*it).first;
				if(name == itemName)
					return (*it).second ? TREE_ITEM_STATE_EXPANDED : TREE_ITEM_STATE_NOT_EXPANDED;
			}
	
			return TREE_ITEM_STATE_NOT_FOUND;
		}



		//-------------------------------------------------
		SmartPtr<IEditableObject> FileWindow::getSelectedObject() const
		{
			return m_selectedObject;
		}



		//-------------------------------------------------
		void FileWindow::setSelectedObject( SmartPtr<IEditableObject> val )
		{
			m_selectedObject = val;
		}



		//-------------------------------------------------
		String FileWindow::getPath() const
		{
			return m_path;
		}



		//-------------------------------------------------
		void FileWindow::setPath(const String& val)
		{
			m_path = val;
		}




		//-------------------------------------------------
		FileWindow::DnDFile::DnDFile(FileWindow* pOwner)
		{
			m_pOwner = pOwner;
		}



		//-------------------------------------------------
		bool FileWindow::DnDFile::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
		{
			m_pOwner->buildTree();
			return false;
		}


		//-------------------------------------------------
		FileWindow::DnDText::DnDText(FileWindow* pOwner) 
			: m_pOwner(pOwner)
		{

		}



		//-------------------------------------------------
		bool FileWindow::DnDText::OnDropText(wxCoord x, wxCoord y, const wxString& text) 
		{
			return false;
		}



	} // end namespace editor
} // end namespace fb


