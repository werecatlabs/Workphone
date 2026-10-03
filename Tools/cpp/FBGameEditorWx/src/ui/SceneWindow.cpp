#include <GameEditorPCH.hpp>
#include "ui/SceneWindow.hpp"

#include "core/MessageManager.hpp"
#include "editor/ComponentTemplateMgr.hpp"
#include "editor/Project.hpp"
#include "editor/ProjectManager.hpp"
#include "editor/EditorMessages.hpp"
#include "editor/EditorManager.hpp"
#include "editor/SceneViewManager.hpp"
#include <FBApplication/Script/ScriptGenerator.hpp>
#include "ui/ProjectTreeData.hpp"
#include "ui/UIManager.hpp"
#include "ui/GuiTypes.hpp"
#include "ui/AddComponentDialog.hpp"
#include "ui/AddNamedDialog.hpp"
#include "ui/AddEntityDialog.hpp"
#include "ui/AddFSMStateDialog.hpp"

#include "NewEventDialog.hpp"
#include "AddGfxObjDialog.hpp"
#include "AddSceneNodeDialog.hpp"
#include "AddSoundDialog.hpp"
#include "AddShapeDialog.hpp"
#include "AddBodyDialog.hpp"

#include "commands/AddEntityEventCmd.hpp"
#include "commands/RemoveEntityEventCmd.hpp"
#include "commands/AddEntityComponentCmd.hpp"
#include "commands/RemoveEntityComponentCmd.hpp"
#include "commands/AddFSMEventCmd.hpp"
#include "commands/RemoveFSMEventCmd.hpp"
#include "commands/AddFSMStateCmd.hpp"
#include "commands/RemoveFSMStateCmd.hpp"
#include "commands/AddEntityGfxObjCmd.hpp"
#include "commands/RemoveEntityGfxObjCmd.hpp"
#include "commands/AddEntityNodeCmd.hpp"
#include "commands/RemoveEntityNodeCmd.hpp"
#include "commands/AddEntitySoundCmd.hpp"
#include "commands/RemoveEntitySoundCmd.hpp"
#include "commands/AddEntityShapeCmd.hpp"
#include "commands/RemoveEntityShapeCmd.hpp"
#include "commands/AddEntityBodyCmd.hpp"
#include "commands/RemoveEntityBodyCmd.hpp"
#include "commands/AddEntityScriptCmd.hpp"
#include "commands/RemoveEntityScriptCmd.hpp"
#include "commands/AddEntityFSMCmd.hpp"
#include "commands/RemoveEntityFSMCmd.hpp"
#include "commands/AddActorCmd.hpp"
#include "commands/RemoveSelectionCmd.hpp"
#include "commands/AddNewScriptCmd.hpp"
#include "commands/AddExistingScriptCmd.hpp"
#include "commands/RemoveSelectionCmd.hpp"

#include "GameEditorTypes.hpp"

#include <FBCore/Interface/System/ISelectionManager.hpp>
#include <FBState/FBState.hpp>
#include <FBCore/FBCoreHeaders.hpp>


#include <FBCore/Base/DataUtil.hpp>
#include <FBCore/Memory/PointerUtil.hpp>
#include <FBData/FBData.hpp>
#include <FBCore/Interface/Resource/IPrefab.hpp>
#include <FBCore/Interface/Resource/IPrefabManager.hpp>
#include <FBApplication/ApplicationUtil.hpp>

#define TREE_ITEM_STATE_NOT_FOUND		0
#define TREE_ITEM_STATE_EXPANDED		1
#define TREE_ITEM_STATE_NOT_EXPANDED	2



namespace fb
{	
	namespace editor
	{


	
		class DnDFile : public wxDropTarget
		{
		public:
			DnDFile(SceneWindow* pOwner) { m_pOwner = pOwner; }

			bool OnDropFiles(wxCoord x, wxCoord y,
				const wxArrayString& filenames)
			{
				return true;
			}

			bool OnDrop(wxCoord x, wxCoord y) wxOVERRIDE
			{
				return true;
			}

			bool GetData() wxOVERRIDE
			{
				return true;
			}


			bool OnDrop(long x, long y, const void* data, size_t size) 
			{
				return true;
			}


			size_t GetFormatCount() const 
			{
				return 0;
			}


			wxDataFormat GetFormat(size_t n) const 
			{
				return wxDataFormat("");
			}

			wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def) wxOVERRIDE
			{
				return wxDragResult();
			}

		private:
			SceneWindow* m_pOwner;
		};
	
		//-------------------------------------------------
		SceneWindow::SceneWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
			const wxSize& size, long style, const wxValidator& validator, const wxString& name)
		{
			try
			{
				m_lastSelectedItem = 0;
		
				auto window = new wxScrolledWindow(parent, id, pos, size, style);
				setWindow(window);

				auto baseSizer = new wxBoxSizer( wxVERTICAL );
				window->SetSizer(baseSizer);
		
				long treeStyle = wxTR_DEFAULT_STYLE | wxTR_TWIST_BUTTONS;
				m_tree = new wxTreeCtrl(window, ENTITY_TREE_ID, wxDefaultPosition, wxDefaultSize, treeStyle);
				baseSizer->Add(m_tree, 1, wxEXPAND);

				m_tree->SetDropTarget(new DnDText(this));

				//m_tree->Hide();
		
				m_applicationMenu = new wxMenu();
				m_applicationAddMenu = new wxMenu();	

				m_applicationAddMenu->Append(PW_ADD_CAMERA, "Camera");
				m_applicationAddMenu->Append(PW_ADD_CAR, "Car");
				m_applicationAddMenu->Append(PW_ADD_CUBE, "Cube");
				m_applicationAddMenu->Append(PW_ADD_PLANE, "Plane");
				m_applicationAddMenu->Append(PW_ADD_PHYSICS_CUBE, "Physics Cube");
				m_applicationAddMenu->Append(PW_ADD_NEW_TERRAIN, "Terrain");
				m_applicationAddMenu->Append(PW_ADD_DIRECTIONAL_LIGHT, "Directional Light");
				m_applicationAddMenu->Append(PW_ADD_POINT_LIGHT, "Point Light");
				m_applicationAddMenu->AppendSeparator();
				m_applicationAddMenu->Append(PW_ADD_BUTTON, "Button");
				m_applicationAddMenu->Append(PW_ADD_CANVAS, "Canvas");
				m_applicationAddMenu->Append(PW_ADD_PANEL, "Panel");
				m_applicationAddMenu->Append(PW_ADD_TEXT, "Text");
				m_applicationMenu->AppendSubMenu(m_applicationAddMenu, "Add");
				m_applicationMenu->AppendSeparator();
				m_applicationMenu->Append(PW_ADD_NEW_ENTITY, "Add Actor");
				m_applicationMenu->Append(SCENE_REMOVE_ACTOR, "Remove" );
				
				EditorManager* editorManager = EditorManager::getSingletonPtr();
				//appRoot->getMessageManager()->addListener(m_messageListener);
				editorManager->getUI()->setSceneWindow(this);
		
				ProjectPtr project = editorManager->getProject();
				m_parentFilter = SmartPtr<TemplateFilter>(new TemplateFilter);
				m_parentFilter->setLabel("Scripts");
				project->addFilter(m_parentFilter);

				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addCamera, this, PW_ADD_CAMERA);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addCar, this, PW_ADD_CAR);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addPlane, this, PW_ADD_PLANE);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addCube, this, PW_ADD_CUBE);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addCube, this, PW_ADD_PHYSICS_CUBE);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addNewTerrain, this, PW_ADD_NEW_TERRAIN);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addDirectionalLight, this, PW_ADD_DIRECTIONAL_LIGHT);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addPointLight, this, PW_ADD_POINT_LIGHT);

				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addButton, this, PW_ADD_BUTTON);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addCanvas, this, PW_ADD_CANVAS);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addPanel, this, PW_ADD_PANEL);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addText, this, PW_ADD_TEXT);

				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::addNewActor, this, PW_ADD_NEW_ENTITY);
				m_applicationMenu->Bind(wxEVT_MENU, &SceneWindow::removeActor, this, SCENE_REMOVE_ACTOR);

				m_tree->Bind(wxEVT_RIGHT_DOWN, &SceneWindow::handleContextMenu, this, -1);

				// todo deselect
				//m_tree->Bind(wxEVT_TREE_STATE_IMAGE_CLICK, &SceneWindow::handleWindowClicked, this, -1);
				//m_tree->Bind(wxEVT_LEFT_DOWN, &SceneWindow::handleWindowClicked, this, -1);
				//m_tree->Bind(wxEVT_COMMAND_LEFT_CLICK, &SceneWindow::addNewTerrain, this, -1);

				m_tree->Bind(wxEVT_TREE_SEL_CHANGED, &SceneWindow::handleTreeSelectionChanged, this, -1);
				m_tree->Bind(wxEVT_TREE_BEGIN_DRAG, &SceneWindow::beginDrag, this, -1);
				m_tree->Bind(wxEVT_TREE_END_DRAG, &SceneWindow::endDrag, this, -1);

				buildTree();
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}
	
	
	
		//-------------------------------------------------
		SceneWindow::~SceneWindow()
		{
			try
			{
		
				//ApplicationManager* appRoot = IApplicationManager::instance();
				//appRoot->getMessageManager()->removeListener(m_messageListener);
				//FB_SAFE_DELETE(m_messageListener);
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//-------------------------------------------------
		void SceneWindow::handleMessage( SmartPtr<IMessage> message )
		{
			try
			{
				FB_ASSERT(message);

				auto type = message->getType();
				FB_ASSERT(!StringUtil::isNullOrEmpty(type));
				
				if(type == "SelectEntityTemplate")
				{
					buildTree();
				}
				else if(type == "LoadProjectMsg")
				{
					buildTree();
				}
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}
	
	
	
		//-------------------------------------------------
		void SceneWindow::buildTree()
		{
			try
			{
				FB_ASSERT(m_tree);
				m_tree->DeleteAllItems();
		
				auto applicationManager = IApplicationManager::instance();
				auto editorManager = EditorManager::getSingletonPtr();

				auto sceneManager = applicationManager->getSceneManager();
				FB_ASSERT(sceneManager);

				auto currentScene = sceneManager->getCurrentScene();
				FB_ASSERT(currentScene);

				if (currentScene)
				{
					auto project = editorManager->getProject();
					FB_ASSERT(project);

					auto appTemplate = project->getApplicationTemplate();

					auto sceneName = currentScene->getName();

					ProjectTreeData data("project", "project", project, project);
					ProjectTreeData scriptsData(m_applicationMenu, "project", "filter", project, m_parentFilter);

					auto rootId = m_tree->AddRoot(sceneName.c_str(), -1, -1, new ProjectTreeData(data));

					auto actors = currentScene->getActors();
					for (auto actor : actors)
					{
						addActorToTree(actor, rootId);
					}

					m_tree->Expand(rootId);
				}
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//-------------------------------------------------
		void SceneWindow::addActorToTree(SmartPtr<IActor> actor, wxTreeItemId rootId)
		{
			auto editorManager = EditorManager::getSingletonPtr();

			auto project = editorManager->getProject();
			FB_ASSERT(project);

			ProjectTreeData data("actor", "actor", actor, actor);

			auto actorName = actor->getName();
			if (StringUtil::isNullOrEmpty(actorName))
			{
				//actorName = "Untitled";
			}

			auto treeId = m_tree->AppendItem(rootId, actorName, -1, -1, new ProjectTreeData(data));

			auto children = actor->getChildren();
			for (auto child : children)
			{
				addActorToTree(child, treeId);
			}
		}

		
	
		//-------------------------------------------------
		void SceneWindow::handleWindowClicked(wxTreeEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto selectionManager = applicationManager->getSelectionManager();
			selectionManager->clearSelection();

			//m_tree->UnselectAll();
		}



		//-------------------------------------------------
		void SceneWindow::handleTreeSelectionChanged( wxTreeEvent& event )
		{
			try
			{
				FB_ASSERT(m_tree);

				if(m_tree->HasFocus())
				{
					auto selectedId = event.GetItem ();
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

					auto appRoot = EditorManager::getSingletonPtr();
					auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
					if(data)
					{
						if(data->getOwnerType()==("entity"))
						{
							SmartPtr<EntityTemplate> entityTemplate = data->getOwnerData();
							appRoot->getProject()->setSelectedEntityTemplate(entityTemplate);
						}
						else if(data->getObjectType()==("mesh"))
						{
							SmartPtr<MeshTemplate> meshTemplate = data->getObjectData();
	
							SceneViewManagerPtr meshViewManager = appRoot->getSceneViewManager();
							meshViewManager->setMeshTemplate(meshTemplate);
						}
						else if (data->getObjectType() == "actor")
						{
							auto object = data->getObjectData();
							auto pObject = object->getSharedFromThis<ISharedObject>();
							
							selectionManager->clearSelection();
							selectionManager->addSelectedObject(pObject);
						}
						else
						{
							selectionManager->clearSelection();
						}
					}
					else
					{
						selectionManager->clearSelection();
					}
		
					wxTreeItemId parentId = m_tree->GetItemParent(selectedId);
					if (!parentId)
						return;

					wxString parentType = m_tree->GetItemText(parentId);
					wxString componentName = m_tree->GetItemText(selectedId);
		
					EntityTreeItemSelectedPtr msg(new EntityTreeItemSelected);
		
					//msg->setComponentType(parentType.mb_str().data());
					//msg->setComponentName(componentName.mb_str().data());
		
					if(data)
					{
						if (m_selectedObject)
						{
							//m_selectedObject = (IEditableObject*)data->getObjectData();

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
		
					auto messageManager = appRoot->getMessageManager();
					if (messageManager)
					{
						messageManager->postMessage(msg);
					}

					auto uiManager = appRoot->getUI();
					if (uiManager)
					{
						uiManager->updateSelection();
					}
				}
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}
	
		//-------------------------------------------------
		void SceneWindow::handleTreeDragStart(wxTreeEvent& event)
		{
			auto selectedId = event.GetItem();
			if (!selectedId)
			{
				return;
			}

			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto selectionManager = applicationManager->getSelectionManager();
			FB_ASSERT(selectionManager);

			data::drag_drop_data data;

			auto itemData = (ProjectTreeData*)m_tree->GetItemData(selectedId);
			if (itemData)
			{
				//auto ownerType = itemData->getOwnerType();
				//if (ownerType == "file")
				//{
				//	auto path = itemData->getObjectType();
				//	data.filePath = path;

				//	auto fileSelection = fb::make_ptr<FileSelection>();
				//	fileSelection->setFilePath(path);

				//	selectionManager->clearSelection();
				//	selectionManager->addSelectedObject(fileSelection);
				//}
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
		void SceneWindow::handleContextMenu(wxMouseEvent& event )
		{		
			try
			{
				m_tree->PopupMenu(m_applicationMenu);				
				event.Skip();
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}
	

	
		//-------------------------------------------------
		void SceneWindow::OnActivateItem( wxTreeEvent& event )
		{
			auto applicationManager = IApplicationManager::instance();
			auto selectionManager = applicationManager->getSelectionManager();

			if(m_tree->HasFocus())
			{
				wxTreeItemId selectedId = m_tree->GetSelection();
				if(!selectedId)
					return;

				wxTreeItemId parentId = m_tree->GetItemParent(selectedId);
				if(!parentId)
					return;

				wxString parentType = m_tree->GetItemText(parentId);
				wxString componentName = m_tree->GetItemText(selectedId);

				if(parentType.length() == 0 || componentName.length() == 0)
					return;

				ProjectTreeData* projectTreeData = (ProjectTreeData*)m_tree->GetItemData(selectedId);
				if(projectTreeData)
				{
					selectionManager->clearSelection();

					if (projectTreeData->getOwnerType() == "Actor" || projectTreeData->getOwnerType() == "actor")
					{
						auto actor = fb::static_pointer_cast<IActor>(projectTreeData->getOwnerData());
						selectionManager->addSelectedObject(actor);
					}

					EntityTreeItemActivatedPtr msg(new EntityTreeItemActivated);

					String componentType;

					String ext = Path::getFileExtension(componentName.mb_str().data());
					if(ext==(".lua"))
					{
						componentType = "Script";
					}
					else if(ext==(".gui"))
					{
						componentType = "GUI";

						//appRoot->getSceneViewManager()->addOverlay(componentName.mb_str().data());
					}
					else
					{
						componentType = parentType.mb_str().data();
					}

					msg->setComponentType(componentType);
					msg->setComponentName(componentName.mb_str().data());
					msg->setObject(projectTreeData->getObjectData());
					//appRoot->getMessageManager()->postMessage(msg);
				}
			}
		}
	
	
	
		//-------------------------------------------------
		void SceneWindow::addNewActor(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto commandManager = applicationManager->getCommandManager();
			auto cmd = fb::make_ptr<AddActorCmd>();
			commandManager->addCommand(cmd);
		}



		//-------------------------------------------------
		void SceneWindow::removeActor(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto commandManager = applicationManager->getCommandManager();

			auto cmd = fb::make_ptr<RemoveSelectionCmd>();
			commandManager->addCommand(cmd);
		}



		//-------------------------------------------------
		void SceneWindow::addNewTerrain(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto commandManager = applicationManager->getCommandManager();
			auto cmd = fb::make_ptr<AddActorCmd>();
			cmd->setActorType(AddActorCmd::ActorType::Terrain);
			commandManager->addCommand(cmd);
		}



		//-------------------------------------------------
		void SceneWindow::addDirectionalLight(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto projectManager = editorManager->getProjectManager();
			auto project = editorManager->getProject();

			ApplicationUtil::createDirectionalLight();

			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}



		//-------------------------------------------------
		void SceneWindow::addPointLight(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto projectManager = editorManager->getProjectManager();
			auto project = editorManager->getProject();

			ApplicationUtil::createDirectionalLight();

			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}



		//-------------------------------------------------
		void SceneWindow::addCube(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto projectManager = editorManager->getProjectManager();
			auto project = editorManager->getProject();
			
			ApplicationUtil::createDefaultCube();

			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}



		//-------------------------------------------------
		void SceneWindow::addPhysicsCube(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto projectManager = editorManager->getProjectManager();
			auto project = editorManager->getProject();

			ApplicationUtil::createDefaultGround();

			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}



		//-------------------------------------------------
		void SceneWindow::addPlane(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto projectManager = editorManager->getProjectManager();
			auto project = editorManager->getProject();

			ApplicationUtil::createDefaultPlane();

			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}



		//-------------------------------------------------
		void SceneWindow::addCamera(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto projectManager = editorManager->getProjectManager();
			auto project = editorManager->getProject();

			ApplicationUtil::createCamera();

			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}



		//-------------------------------------------------
		void SceneWindow::addCar(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto projectManager = editorManager->getProjectManager();
			auto project = editorManager->getProject();

			ApplicationUtil::createDefaultVehicle();

			auto uiManager = editorManager->getUI();
			uiManager->rebuildSceneTree();
		}


	
		//-------------------------------------------------
		void SceneWindow::addButton(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto commandManager = applicationManager->getCommandManager();
			auto cmd = fb::make_ptr<AddActorCmd>();
			cmd->setActorType(AddActorCmd::ActorType::Button);
			commandManager->addCommand(cmd);
		}



		//-------------------------------------------------
		void SceneWindow::addCanvas(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto commandManager = applicationManager->getCommandManager();
			auto cmd = fb::make_ptr<AddActorCmd>();
			cmd->setActorType(AddActorCmd::ActorType::Canvas);
			commandManager->addCommand(cmd);
		}



		//-------------------------------------------------
		void SceneWindow::addPanel(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto commandManager = applicationManager->getCommandManager();
			auto cmd = fb::make_ptr<AddActorCmd>();
			cmd->setActorType(AddActorCmd::ActorType::Panel);
			commandManager->addCommand(cmd);
		}



		//-------------------------------------------------
		void SceneWindow::addText(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto commandManager = applicationManager->getCommandManager();
			auto cmd = fb::make_ptr<AddActorCmd>();
			cmd->setActorType(AddActorCmd::ActorType::Text);
			commandManager->addCommand(cmd);
		}



		//-------------------------------------------------
		void SceneWindow::beginDrag(wxTreeEvent& event)
		{
			// need to explicitly allow drag
			if (event.GetItem() != m_tree->GetRootItem())
			{
				m_draggedItem = event.GetItem();

				wxPoint clientpt = event.GetPoint();
				wxPoint screenpt = m_tree->ClientToScreen(clientpt);

				event.Allow();
			}
			else
			{
				wxLogMessage("OnBeginDrag: this item can't be dragged.");
			}
		}



		//-------------------------------------------------
		void SceneWindow::endDrag(wxTreeEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto sceneManager = applicationManager->getSceneManager();
			FB_ASSERT(applicationManager);

			auto scene = sceneManager->getCurrentScene();
			FB_ASSERT(scene);

			auto itemSrc = m_draggedItem;
			auto itemDst = event.GetItem();

			if (itemSrc && itemDst)
			{
				m_draggedItem = (wxTreeItemId)0l;

				//// where to copy the item?
				//if (itemDst.IsOk() && !m_tree->ItemHasChildren(itemDst))
				//{
				//	// copy to the parent then
				//	itemDst = m_tree->GetItemParent(itemDst);
				//}

				//if (!itemDst.IsOk())
				//{
				//	wxLogMessage("OnEndDrag: can't drop here.");

				//	return;
				//}

				wxString text = m_tree->GetItemText(itemSrc);
				//wxLogMessage("OnEndDrag: '%s' copied to '%s'.",
				//	text, GetItemText(itemDst));

				// just do append here - we could also insert it just before/after the item
				// on which it was dropped, but this requires slightly more work... we also
				// completely ignore the client data and icon of the old item but could
				// copy them as well.
				//
				// Finally, we only copy one item here but we might copy the entire tree if
				// we were dragging a folder.
				//int image = wxGetApp().ShowImages() ? TreeCtrlIcon_File : -1;
				wxTreeItemId id = m_tree->AppendItem(itemDst, text);

				//if (wxGetApp().ShowStates())
				//	SetItemState(id, GetItemState(itemSrc));

				auto srcData = (ProjectTreeData*)m_tree->GetItemData(itemSrc);
				auto dstData = (ProjectTreeData*)m_tree->GetItemData(itemDst);

				if (srcData && dstData)
				{
					auto objectTypeSrc = srcData->getObjectType();
					auto objectTypeDst = dstData->getObjectType();

					if (objectTypeSrc == "actor" || objectTypeDst == "actor")
					{
						auto objectSrc = srcData->getObjectData();
						auto pObjectSrc = objectSrc->getSharedFromThis<ISharedObject>();
						auto actorSrc = fb::static_pointer_cast<IActor>(pObjectSrc);

						auto objectDst = dstData->getObjectData();
						auto pObjectDst = objectDst->getSharedFromThis<ISharedObject>();
						auto actorDst = fb::static_pointer_cast<IActor>(pObjectDst);
						auto projectDst = fb::static_pointer_cast<Project>(pObjectDst);

						if (actorSrc && actorDst)
						{
							auto parent = actorSrc->getParent();
							if (parent != nullptr)
							{
								parent->removeChild(actorSrc);
							}
							else
							{
								scene->removeActor(actorSrc);
							}

							actorDst->addChild(actorSrc);
							this->buildTree();
						}
						else if (actorSrc && projectDst)
						{
							auto parent = actorSrc->getParent();
							if (parent != nullptr)
							{
								parent->removeChild(actorSrc);
								scene->addActor(actorSrc);
								this->buildTree();
							}
						}
					}
				}
			}
		}


		
		//-------------------------------------------------
		void SceneWindow::saveTreeState()
		{


			// clear map
			treeState.clear();
	
			// reset selected item
			m_newSelectedItem = NULL;
	
			wxTreeItemId itemId = m_tree->GetRootItem();
			String parent = "";
			if(itemId)
				saveItemState(parent, itemId);
		}
	
	
	
		//-------------------------------------------------
		void SceneWindow::saveItemState(String parent, wxTreeItemId itemId)
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
		void SceneWindow::restoreTreeState()
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
		void SceneWindow::restoreItemState(String parent, wxTreeItemId itemId, bool parentWasNew)
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
		int SceneWindow::getItemState(String itemName)
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
		SmartPtr<IEditableObject> SceneWindow::getSelectedObject() const
		{
			return m_selectedObject;
		}



		//-------------------------------------------------
		void SceneWindow::setSelectedObject( SmartPtr<IEditableObject> val )
		{
			m_selectedObject = val;
		}



		SceneWindow::DnDText::DnDText(SceneWindow* pOwner)
		{
			m_pOwner = pOwner;
		}

		bool SceneWindow::DnDText::OnDropText(wxCoord x, wxCoord y, const wxString& text) 
		{
			try
			{
				data::drag_drop_data data;
				auto dataStr = String(text.c_str());
				if (!StringUtil::isNullOrEmpty(dataStr))
				{
					DataUtil::parse(dataStr, &data);

					auto filePath = StringUtil::cleanupPath(data.filePath);

					if (!StringUtil::isNullOrEmpty(filePath))
					{
						auto fileExt = Path::getFileExtension(filePath);
						fileExt = StringUtil::make_lower(fileExt);

						if (fileExt == ".fbx" || fileExt == ".fbmeshbin")
						{
							auto applicationManager = IApplicationManager::instance();
							FB_ASSERT(applicationManager);

							auto prefabManager = applicationManager->getPrefabManager();
							auto sceneManager = applicationManager->getSceneManager();
							auto scene = sceneManager->getCurrentScene();

							auto selectionManager = applicationManager->getSelectionManager();

							auto prefabResource = prefabManager->load(filePath);
							if (prefabResource)
							{
								auto prefab = fb::static_pointer_cast<IPrefab>(prefabResource);
								if (prefab)
								{
									auto actor = prefab->createActor();

									auto name = Path::getFileNameWithoutExtension(filePath);
									auto handle = actor->getHandle();
									if (handle)
									{
										handle->setName(name);
									}

									//auto selection = selectionManager->getSelection();
									//if (!selection.empty())
									//{
									//	for (auto selected : selection)
									//	{
									//		if (selected->isDerived<IActor>())
									//		{
									//			auto selectedActor = fb::static_pointer_cast<IActor>(selected);
									//			selectedActor->addChild(actor);
									//		}
									//	}
									//}
									//else
									{
										scene->addActor(actor);
										scene->registerAllUpdates(actor);
									}

									//if (applicationManager->isPlaying())
									//{
									//	actor->start();
									//}
									//else
									//{
									//	actor->edit();
									//}
								}
							}
						}
					}
				}

				m_pOwner->buildTree();
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}

			return false;
		}

	} // end namespace editor
} // end namespace fb


