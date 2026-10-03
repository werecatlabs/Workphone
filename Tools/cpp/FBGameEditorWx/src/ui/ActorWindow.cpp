#include <GameEditorPCH.hpp>
#include <ui/ActorWindow.hpp>
#include <core/MessageManager.hpp>
#include <editor/EditorManager.hpp>
#include <editor/SceneViewManager.hpp>
#include <editor/Project.hpp>
#include <editor/ProjectManager.hpp>
#include <editor/ComponentTemplateMgr.hpp>
#include <editor/EditorManager.hpp>
#include <ui/GuiTypes.hpp>
#include <ui/AddNamedDialog.hpp>
#include <ui/AddEntityDialog.hpp>
#include <ui/AddFSMStateDialog.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/SceneWindow.hpp>
#include <ui/UIManager.hpp>
#include <commands/AddEntityEventCmd.hpp>
#include <commands/RemoveEntityEventCmd.hpp>
#include <FBCore/Interface/System/ISelectionManager.hpp>

#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplate.hpp>
#include <FBCore/FBCoreHeaders.hpp>
#include <FBApplication/Components/TransformComponent.hpp>
#include <FBWxWidgets/wxLabelCheckboxPair.hpp>
#include <FBWxWidgets/wxLabelTextInputPair.hpp>
#include <GameEditorTypes.hpp>
#include <wx/wx.hpp>
#include <ui/ObjectBrowserDialog.hpp>
#include <editor/FileSelection.hpp>



#define TREE_ITEM_STATE_NOT_FOUND        0
#define TREE_ITEM_STATE_EXPANDED        1
#define TREE_ITEM_STATE_NOT_EXPANDED    2



namespace fb
{
	namespace editor
	{



		//-------------------------------------------------
		ActorWindow::ActorWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos,
				const wxSize& size, long style, const wxValidator& validator, const wxString& name)
				:m_messageListener(nullptr)
		{
			try
			{
				m_lastSelectedItem = 0;

				//m_messageListener = new ProjectWindowMessageListener(this);
				auto parentWindow = new wxWindow(parent, id, pos, size, style);
				setWindow(parentWindow);

				auto baseSizer = new wxBoxSizer(wxVERTICAL);
				parentWindow->SetSizer(baseSizer);

				auto actorWindow = new wxWindow(parentWindow, -1, pos, size, style);
				//actorWindow->SetBackgroundColour(wxColour(*wxRED));

				auto baseSizerFlags = wxSizerFlags().Expand().Proportion(50);
				baseSizer->Add(actorWindow, baseSizerFlags);

				auto actorSizer = new wxBoxSizer(wxVERTICAL);
				actorWindow->SetSizer(actorSizer);

				auto checkBoxPanel = new wxWindow(actorWindow, -1, pos, size, style);
				//checkBoxPanel->SetBackgroundColour(wxColour(*wxBLUE));
				actorSizer->Add(checkBoxPanel, wxSizerFlags().Expand().Proportion(5));

				auto checkBoxPanelSizer = new wxBoxSizer(wxVERTICAL);
				checkBoxPanel->SetSizer(checkBoxPanelSizer);

				auto actorNamePanel = new wxWindow(actorWindow, -1, pos, size, style);
				actorSizer->Add(actorNamePanel, wxSizerFlags().Expand().Proportion(5));
				//actorNamePanel->SetBackgroundColour(wxColour(*wxRED));

				auto actorNamePanelSizer = new wxBoxSizer(wxVERTICAL);
				actorNamePanel->SetSizer(actorNamePanelSizer);

				//auto dropDownPanel = new wxWindow(actorWindow, -1, pos, size, style);
				//actorSizer->Add(dropDownPanel, wxSizerFlags().Expand().Proportion(10));

				auto visibleCheckbox = new wxCheckBox(checkBoxPanel, -1, "&Visible");
				visibleCheckbox->Bind(wxEVT_CHECKBOX, &ActorWindow::visibleCheckboxState, this, -1);
				checkBoxPanelSizer->Add(visibleCheckbox, wxSizerFlags().Expand().Proportion(10));
				m_visibleCheckbox = visibleCheckbox;

				auto staticCheckbox = new wxCheckBox(checkBoxPanel, -1, "&Static");
				staticCheckbox->Bind(wxEVT_CHECKBOX, &ActorWindow::staticCheckboxState, this, -1);
				checkBoxPanelSizer->Add(staticCheckbox, wxSizerFlags().Expand().Proportion(10));
				m_staticCheckbox = staticCheckbox;

				//auto actorNameText = new wxTextCtrl(actorNamePanel, -1, "&Actor Name");
				//actorNamePanelSizer->Add(actorNameText, 0, wxEXPAND);

				m_actorNamePair = fb::make_ptr<ui::wxLabelTextInputPair>();
				m_actorNamePair->setParent(actorNamePanel);
				m_actorNamePair->setLabel("Actor Name");
				m_actorNamePair->setValue("Empty");
				m_actorNamePair->load(nullptr);

				auto valueText = m_actorNamePair->getInputText();
				valueText->Bind(wxEVT_TEXT, &ActorWindow::OnText, this, -1);
				//valueText->Bind(wxEVT_TEXT_ENTER, &ActorWindow::OnTextEnter, this, -1);
				valueText->Bind(wxEVT_TEXT_COPY, &ActorWindow::OnTextCopy, this, -1);
				valueText->Bind(wxEVT_TEXT_PASTE, &ActorWindow::OnTextPaste, this, -1);

				//auto radioKind = new wxRadioBox(actorWindow, -1);

				long componentWindowStyle = wxVSCROLL | wxHSCROLL;
				auto componentWindow = new wxWindow(actorWindow, -1, pos, size, componentWindowStyle);
				actorSizer->Add(componentWindow, wxSizerFlags().Expand().Proportion(30));
				componentWindow->SetBackgroundColour(wxColour(*wxYELLOW));
				componentWindow->SetAutoLayout(true);
				componentWindow->Layout();

				auto componentSizer = new wxBoxSizer(wxVERTICAL);
				componentWindow->SetSizer(componentSizer);

				wxSize treeSize = wxSize(300, 200);
				treeSize.SetWidth(wxDefaultSize.GetWidth());

				long treeStyle = wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT | wxTR_TWIST_BUTTONS;
				m_tree = new wxTreeCtrl(componentWindow, ENTITY_TREE_ID, pos, treeSize, treeStyle);
				m_tree->SetAutoLayout(true);

				auto componentsSizerFlags = wxSizerFlags().Expand();
				componentSizer->Add(m_tree, componentsSizerFlags);

				componentWindow->Bind(wxEVT_TREE_SEL_CHANGED, &ActorWindow::handleTreeSelectionChanged, this,
						ENTITY_TREE_ID);

				auto editorManager = EditorManager::getSingletonPtr();
				//appRoot->getMessageManager()->addListener(m_messageListener);
				auto uiManager = editorManager->getUI();
				uiManager->setActorWindow(this);

				auto project = editorManager->getProject();
				m_parentFilter = SmartPtr<TemplateFilter>(new TemplateFilter);
				m_parentFilter->setLabel("Scripts");
				project->addFilter(m_parentFilter);

				auto data = new ScriptTreeItemData;
				data->m_itemId = 0;
				m_parentFilter->setUserData(data);

				m_defaultMenu = new wxMenu();

				m_defaultMenu->Append(ADD_COMPONENT, "Add Component");
				m_defaultMenu->Append(REMOVE_COMPONENT, "Remove Component");

				m_defaultMenu->Bind(wxEVT_MENU, &ActorWindow::addComponent, this, ADD_COMPONENT);
				m_defaultMenu->Bind(wxEVT_MENU, &ActorWindow::removeComponent, this, REMOVE_COMPONENT);

				m_tree->Bind(wxEVT_RIGHT_DOWN, &ActorWindow::handleContextMenu, this, -1);
				//parentWindow->Bind(wxEVT_SIZE , &ActorWindow::OnSize, this, wxID_ANY);

				buildTree();

				//baseSizer->Layout();
				m_baseSizer = baseSizer;
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//-------------------------------------------------
		void ActorWindow::OnSize(wxSizeEvent& event)
		{
			//m_baseSizer->Layout();
		}



		//-------------------------------------------------
		void ActorWindow::OnText(wxCommandEvent& event)
		{		
			auto inputText = m_actorNamePair->getInputText();
			auto text = inputText->GetValue();
			auto textStr = String(text.c_str());

			setActorName(textStr);

		}



		//-------------------------------------------------
		void ActorWindow::setActorName(const String& textStr)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto selectionManager = applicationManager->getSelectionManager();
			FB_ASSERT(selectionManager);

			auto selection = selectionManager->getSelection();
			for (auto selected : selection)
			{
				if (selected->isDerived<IActor>())
				{
					auto actor = fb::static_pointer_cast<IActor>(selected);
					if (actor)
					{
						auto handle = actor->getHandle();
						if (handle)
						{
							auto name = handle->getName();
							if (name != textStr)
							{
								handle->setName(textStr);

								auto editorManager = EditorManager::getSingletonPtr();
								auto ui = editorManager->getUI();
								auto sceneWindow = ui->getSceneWindow();
								if (sceneWindow)
								{
									sceneWindow->buildTree();
								}
							}
						}
					}
				}
			}
		}



		//-------------------------------------------------
		void ActorWindow::OnTextEnter(wxCommandEvent& event)
		{
		}



		//-------------------------------------------------
		void ActorWindow::OnTextURL(wxTextUrlEvent& event)
		{

		}



		//-------------------------------------------------
		void ActorWindow::OnTextMaxLen(wxCommandEvent& event)
		{

		}



		//-------------------------------------------------
		void ActorWindow::OnTextCut(wxClipboardTextEvent& event)
		{

		}



		//-------------------------------------------------
		void ActorWindow::OnTextCopy(wxClipboardTextEvent& event)
		{

		}



		//-------------------------------------------------
		void ActorWindow::OnTextPaste(wxClipboardTextEvent& event)
		{
			auto inputText = m_actorNamePair->getInputText();
			auto text = inputText->GetValue();

			//auto str = event.GetString();
			auto textStr = String(text.c_str());
			setActorName(textStr);
		}



		//-------------------------------------------------
		ActorWindow::~ActorWindow()
		{
			try
			{
				FB_SAFE_DELETE(m_defaultMenu);

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
		void ActorWindow::unload(SmartPtr<ISharedObject> data)
		{
			if (m_actorNamePair)			
			{
				m_actorNamePair->unload(data);
				m_actorNamePair = nullptr;
			}

			m_selectedObject = nullptr;
			m_selectedEntity = nullptr;
			m_parentFilter = nullptr;
		}



		//-------------------------------------------------
		void ActorWindow::staticCheckboxState(wxCommandEvent& event)
		{
			auto isStatic = m_staticCheckbox->GetValue();

			auto applicationManager = IApplicationManager::instance();
			auto selectionManager = applicationManager->getSelectionManager();

			auto selection = selectionManager->getSelection();
			for (auto selected: selection)
			{
				auto actor = fb::dynamic_pointer_cast<IActor>(selected);
				if (actor)
				{
					actor->setStatic(isStatic);
				}
			}
		}



		//-------------------------------------------------
		void ActorWindow::visibleCheckboxState(wxCommandEvent& event)
		{
			auto isVisible = m_visibleCheckbox->GetValue();

			auto applicationManager = IApplicationManager::instance();
			auto selectionManager = applicationManager->getSelectionManager();

			auto selection = selectionManager->getSelection();
			for (auto selected: selection)
			{
				auto actor = fb::dynamic_pointer_cast<IActor>(selected);
				if (actor)
				{
					actor->setVisible(isVisible);
				}
			}
		}



		//-------------------------------------------------
		void ActorWindow::addComponent(wxCommandEvent& event)
		{
			auto window = getWindow();
			ObjectBrowserDialog dlg(window);
			auto returnValue = dlg.ShowModal();

			if (returnValue == wxID_OK)
			{
				auto selectedComponentType = dlg.getSelectedObject();

				auto applicationManager = IApplicationManager::instance();
				auto factoryManager = applicationManager->getFactoryManager();
				auto selectionManager = applicationManager->getSelectionManager();

				auto selection = selectionManager->getSelection();
				for (auto selected: selection)
				{
					auto actor = fb::dynamic_pointer_cast<IActor>(selected);
					if (actor)
					{
						auto component = factoryManager->createObjectFromType<component::IComponent>(
								selectedComponentType);
						actor->addComponentInstance(component);
					}
				}

				buildTree();
			}
		}



		//-------------------------------------------------
		void ActorWindow::removeComponent(wxCommandEvent& event)
		{
			auto selectedItem = m_tree->GetSelection();
			auto data = (ProjectTreeData*)m_tree->GetItemData(selectedItem);
			if (data)
			{
				if (data->getObjectType() == "component")
				{
					auto applicationManager = IApplicationManager::instance();
					auto selectionManager = applicationManager->getSelectionManager();

					auto selection = selectionManager->getSelection();
					for (auto selected: selection)
					{
						auto component = fb::dynamic_pointer_cast<component::IComponent>(selected);
						if (component)
						{
							//m_selectedObject->removeComponent(component);
						}
					}

					buildTree();
				}
			}
		}



		//-------------------------------------------------
		void ActorWindow::handleMessage(SmartPtr<IMessage> message)
		{
			try
			{
				FB_ASSERT(message);

				auto type = message->getType();
				FB_ASSERT(!StringUtil::isNullOrEmpty(type));

				if (type == "SelectEntityTemplate")
				{
					buildTree();
				}
				else if (type == "LoadProjectMsg")
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
		void ActorWindow::buildTree()
		{
			try
			{
				FB_ASSERT(m_tree);

				if (m_tree)
				{
					m_tree->DeleteAllItems();

					auto applicationManager = IApplicationManager::instance();
					FB_ASSERT(applicationManager);

					auto graphicsSystem = applicationManager->getGraphicsSystem();
					FB_ASSERT(graphicsSystem);
					FB_ASSERT(graphicsSystem->isValid());

					auto materialManager = graphicsSystem->getMaterialManager();
					auto meshManager = applicationManager->getMeshManager();

					FB_ASSERT(materialManager);
					FB_ASSERT(meshManager);

					auto editorManager = EditorManager::getSingletonPtr();
					auto selectionManager = applicationManager->getSelectionManager();

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

						auto selection = selectionManager->getSelection();
						if (selection.size() == 1)
						{
							m_selectedObject = selection.front();
							if (m_selectedObject)
							{
								auto actor = fb::dynamic_pointer_cast<IActor>(selection.front());
								auto material = fb::dynamic_pointer_cast<render::IMaterial>(selection.front());
								auto fileSelection = fb::dynamic_pointer_cast<FileSelection>(selection.front());

								if (actor)
								{
									auto actorName = actor->getName();
									m_actorNamePair->setValue(actorName);

									auto isStatic = actor->isStatic();
									m_staticCheckbox->SetValue(isStatic);

									auto isVisible = actor->isVisible();
									m_visibleCheckbox->SetValue(isVisible);

									ProjectTreeData data("project", "project", project, project);
									ProjectTreeData scriptsData(m_defaultMenu, "project", "filter", project, m_parentFilter);

									auto rootId = m_tree->AddRoot(actorName.c_str(), -1, -1, new ProjectTreeData(data));
									addActorToTree(actor, rootId);

									//m_tree->Expand(rootId);
								}
								else if (m_selectedObject->isDerived<IMeshResource>())
								{
									ProjectTreeData data("project", "project", project, project);
									auto rootId = m_tree->AddRoot("Mesh", -1, -1, new ProjectTreeData(data));
									addObjectToTree(m_selectedObject, rootId);
								}
								else if (fileSelection)
								{
									auto filePath = fileSelection->getFilePath();
									auto ext = Path::getFileExtension(filePath);
									ext = StringUtil::make_lower(ext);

									ProjectTreeData data("project", "project", project, project);
									ProjectTreeData scriptsData(m_defaultMenu, "project", "filter", project, m_parentFilter);

									if (ext == ".mat")
									{
										material = materialManager->loadFromFile(filePath);
										if (material)
										{
											auto rootId = m_tree->AddRoot("Material", -1, -1, new ProjectTreeData(data));
											addObjectToTree(material, rootId);
										}
									}
									else if (ext == ".fbx" || ext == ".fbmeshbin")
									{
										auto mesh = meshManager->loadFromFile(filePath);
										if (mesh)
										{
											auto rootId = m_tree->AddRoot("Mesh", -1, -1, new ProjectTreeData(data));
											addObjectToTree(mesh, rootId);
										}
									}
								}
								else if (material)
								{

									ProjectTreeData data("project", "project", project, project);
									ProjectTreeData scriptsData(m_defaultMenu, "project", "filter", project, m_parentFilter);

									auto rootId = m_tree->AddRoot("Material", -1, -1, new ProjectTreeData(data));
									addMaterialToTree(material, rootId);
								}
								else if (m_selectedObject->isDerived<render::IMaterialNode>())
								{
									ProjectTreeData data("project", "project", project, project);
									ProjectTreeData scriptsData(m_defaultMenu, "project", "filter", project, m_parentFilter);

									auto rootId = m_tree->AddRoot("Object", -1, -1, new ProjectTreeData(data));
									addObjectToTree(m_selectedObject, rootId);
								}
								else
								{
									ProjectTreeData data("project", "project", project, project);
									ProjectTreeData scriptsData(m_defaultMenu, "project", "filter", project, m_parentFilter);

									auto rootId = m_tree->AddRoot("Object", -1, -1, new ProjectTreeData(data));
									addObjectToTree(m_selectedObject, rootId);
								}
							}
						}
						else
						{
							ProjectTreeData data("project", "project", project, project);
							ProjectTreeData scriptsData(m_defaultMenu, "project", "filter", project,m_parentFilter);

							auto rootId = m_tree->AddRoot(sceneName.c_str(), -1, -1, new ProjectTreeData(data));

							for (auto selected : selection)
							{
								auto actor = fb::dynamic_pointer_cast<IActor>(selected);
								if (actor)
								{
									addActorToTree(actor, rootId);
								}
							}

							//m_tree->Expand(rootId);
						}
					}
				}
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//-------------------------------------------------
		void ActorWindow::addTransformToTree(SmartPtr<ITransform> component, String label, wxTreeItemId rootId)
		{
			try
			{
				if (component)
				{
					auto editorManager = EditorManager::getSingletonPtr();

					auto project = editorManager->getProject();
					FB_ASSERT(project);

					ProjectTreeData data("transform", "transform", component, component);

					auto treeId = m_tree->AppendItem(rootId, label, -1, -1, new ProjectTreeData(data));
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//-------------------------------------------------
		void ActorWindow::addComponentToTree(SmartPtr<component::IComponent> component, wxTreeItemId rootId)
		{
			try
			{
				if (component)
				{
					auto editorManager = EditorManager::getSingletonPtr();

					auto project = editorManager->getProject();
					FB_ASSERT(project);

					ProjectTreeData data("component", "component", component, component);

					auto typeinfo = component->getTypeInfo();
					FB_ASSERT(typeinfo);

					auto typeManager = TypeManager::instance();
					FB_ASSERT(typeManager);

					auto className = typeManager->getName(typeinfo);
					if (StringUtil::isNullOrEmpty(className))
					{
						className = "Untitled";
					}

					auto treeId = m_tree->AppendItem(rootId, className, -1, -1, new ProjectTreeData(data));

					auto children = component->getChildObjects();
					for (auto child: children)
					{
						if (child)
						{
							addObjectToTree(child, treeId);
						}
					}

					if (!children.empty())
					{
						m_tree->Expand(treeId);
					}
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//-------------------------------------------------
		void ActorWindow::addObjectToTree(SmartPtr<ISharedObject> object, wxTreeItemId rootId)
		{
			try
			{
				if (object)
				{
					auto editorManager = EditorManager::getSingletonPtr();

					auto project = editorManager->getProject();
					FB_ASSERT(project);

					ProjectTreeData data("object", "object", object, object);

					auto typeinfo = object->getTypeInfo();
					FB_ASSERT(typeinfo != 0);

					auto typeManager = TypeManager::instance();
					FB_ASSERT(typeManager);

					auto className = typeManager->getName(typeinfo);
					if (StringUtil::isNullOrEmpty(className))
					{
						className = "Untitled";
					}

					/*if (object->isDerived<IStateObject>())
					{
						className = String("StateObject");
					}
					else 
					*/
					if (object->isDerived<IStateObject>())
					{
						className = String("StateObject");
					}
					else if (object->isDerived<IStateListener>())
					{
						className = String("StateListener");
					}
					else if (object->isDerived<render::ISceneManager>())
					{
						className = String("SceneManager");
					}
					else if (object->isDerived<render::ISceneNode>())
					{
						className = String("SceneNode");
					}
					else if (object->isDerived<render::IGraphicsMesh>())
					{
						className = String("Mesh");
					}
					else if (object->isDerived<render::IMaterial>())
					{
						className = String("Material");
					}
					else if (object->isDerived<render::IMaterialTechnique>())
					{
						className = String("Technique");
					}
					else if (object->isDerived<render::IMaterialPass>())
					{
						className = String("Pass");
					}
					else if (object->isDerived<render::IMaterialTexture>())
					{
						auto textureHandle = object->getHandle();
						className = textureHandle->getName();
					}

					auto treeId = m_tree->AppendItem(rootId, className, -1, -1, new ProjectTreeData(data));

					auto children = object->getChildObjects();
					for (auto child: children)
					{
						if (child)
						{
							addObjectToTree(child, treeId);
						}
					}

					//if (!children.empty())
					//{
					//	m_tree->Expand(treeId);
					//}
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//-------------------------------------------------
		void ActorWindow::addActorToTree(SmartPtr<IActor> actor, wxTreeItemId rootId)
		{
			try
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

				//auto treeId = m_tree->AppendItem(rootId, actorName, -1, -1, new ProjectTreeData(data));

				auto transform = actor->getTransform();
				addTransformToTree(transform, "Transform", rootId);

				auto components = actor->getComponentsByType<component::IComponent>();
				for (auto component: components)
				{
					addComponentToTree(component, rootId);
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//-------------------------------------------------
		void ActorWindow::addResourceToTree(SmartPtr<IResource> resource, wxTreeItemId rootId)
		{
			try
			{
				auto editorManager = EditorManager::getSingletonPtr();

				auto project = editorManager->getProject();
				FB_ASSERT(project);

				ProjectTreeData data("resource", "resource", resource, resource);

				auto handle = resource->getHandle();
				auto actorName = handle->getName();
				if (StringUtil::isNullOrEmpty(actorName))
				{
					//actorName = "Untitled";
				}

				auto treeId = m_tree->AppendItem(rootId, actorName, -1, -1, new ProjectTreeData(data));

				auto children = resource->getChildObjects();
				for (auto child: children)
				{
					if (child)
					{
						addObjectToTree(child, treeId);
					}
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//-------------------------------------------------
		void ActorWindow::addMaterialToTree(SmartPtr<render::IMaterial> material, wxTreeItemId rootId)
		{
			try
			{
				auto editorManager = EditorManager::getSingletonPtr();

				auto project = editorManager->getProject();
				FB_ASSERT(project);

				ProjectTreeData data("material", "material", material, material);

				auto matreialHandle = material->getHandle();
				auto actorName = matreialHandle->getName();
				if (StringUtil::isNullOrEmpty(actorName))
				{
					//actorName = "Untitled";
				}

				auto treeId = m_tree->AppendItem(rootId, actorName, -1, -1, new ProjectTreeData(data));

				addObjectToTree(material, rootId);
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//-------------------------------------------------
		void ActorWindow::handleTreeSelectionChanged(wxTreeEvent& event)
		{
			try
			{
				FB_ASSERT(m_tree);

				if (m_tree->HasFocus())
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
					auto ui = editorManager->getUI();
					ui->hideComponentEditWindows();

					auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
					if (data)
					{
						auto pObject = data->getObjectData();
						auto ownerType = data->getOwnerType();

						if (ownerType == ("entity"))
						{
							SmartPtr<EntityTemplate> entityTemplate = data->getOwnerData();
							editorManager->getProject()->setSelectedEntityTemplate(entityTemplate);
						}
						else if (ownerType == ("mesh"))
						{
							SmartPtr<MeshTemplate> meshTemplate = data->getObjectData();

							SceneViewManagerPtr meshViewManager = editorManager->getSceneViewManager();
							meshViewManager->setMeshTemplate(meshTemplate);
						}
						else if (ownerType == "actor")
						{
							auto object = data->getObjectData();
							auto pObject = object->getSharedFromThis<ISharedObject>();

							selectionManager->clearSelection();
							selectionManager->addSelectedObject(pObject);

							auto uiManager = editorManager->getUI();
							if (uiManager)
							{
								uiManager->updateActorSelection();
							}
						}
						else if (ownerType == "transform")
						{
							auto object = data->getObjectData();
							auto pObject = object->getSharedFromThis<ISharedObject>();

							selectionManager->clearSelection();
							selectionManager->addSelectedObject(pObject);

							auto uiManager = editorManager->getUI();
							if (uiManager)
							{
								uiManager->updateActorSelection();
							}
						}
						else if (ownerType == "component")
						{
							auto object = data->getObjectData();
							auto pObject = object->getSharedFromThis<ISharedObject>();

							selectionManager->clearSelection();
							selectionManager->addSelectedObject(pObject);

							auto uiManager = editorManager->getUI();
							if (uiManager)
							{
								uiManager->updateComponentSelection();
							}
						}
						else if (pObject->isDerived<render::IMaterial>())
						{
							auto object = data->getObjectData();
							auto pObject = object->getSharedFromThis<ISharedObject>();

							selectionManager->clearSelection();
							selectionManager->addSelectedObject(pObject);

							auto uiManager = editorManager->getUI();
							if (uiManager)
							{
								uiManager->updateActorSelection();
							}
						}
						else if (pObject->isDerived<render::IMaterialNode>())
						{
							auto object = data->getObjectData();
							auto pObject = object->getSharedFromThis<ISharedObject>();

							selectionManager->clearSelection();
							selectionManager->addSelectedObject(pObject);

							auto uiManager = editorManager->getUI();
							if (uiManager)
							{
								uiManager->updateComponentSelection();
							}
						}
						else if (ownerType == "object")
						{
							auto object = data->getObjectData();
							auto pObject = object->getSharedFromThis<ISharedObject>();

							selectionManager->clearSelection();
							selectionManager->addSelectedObject(pObject);

							auto propertiesWindow = ui->getPropertiesWindow();
							if (propertiesWindow)
							{
								propertiesWindow->updateSelection();
							}
						}
						else
						{
							auto project = editorManager->getProject();
							project->setSelectedEntityTemplate(nullptr);
						}
					}
				}
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//-------------------------------------------------
		void ActorWindow::handleContextMenu(wxMouseEvent& event)
		{
			try
			{
				m_tree->PopupMenu(m_defaultMenu);
				event.Skip();
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//-------------------------------------------------
		void ActorWindow::saveTreeState()
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
		void ActorWindow::saveItemState(String parent, wxTreeItemId itemId)
		{
			// make item name
			String itemName = String(m_tree->GetItemText(itemId));
			if (parent != "")
				itemName = parent + "/" + itemName;

			// get expanded state
			bool isExpanded = false;
			if (m_tree->ItemHasChildren(itemId))
				isExpanded = m_tree->IsExpanded(itemId);

			//get selected state
			if (m_tree->IsSelected(itemId))
				m_newSelectedItem = itemId;

			// add item to map
			treeState.insert(std::map<String, bool>::value_type(itemName, isExpanded));

			// parse childes
			wxTreeItemIdValue cookie;
			wxTreeItemId childrenItem = m_tree->GetFirstChild(itemId, cookie);
			while (childrenItem.IsOk())
			{
				saveItemState(itemName, childrenItem);
				childrenItem = m_tree->GetNextChild(itemId, cookie);
			}
		}



		//-------------------------------------------------
		void ActorWindow::restoreTreeState()
		{
			wxTreeItemId itemId = m_tree->GetRootItem();

			// restore items 
			String parent = "";
			if (itemId)
				restoreItemState(parent, itemId, false);

			if (m_newSelectedItem)
			{
				m_tree->EnsureVisible(m_newSelectedItem);
				m_tree->SelectItem(m_newSelectedItem);
			}
		}



		//-------------------------------------------------
		void ActorWindow::updateSelection()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				auto selectionManager = applicationManager->getSelectionManager();

				auto selection = selectionManager->getSelection();
				for (auto object: selection)
				{
					if (object)
					{
						auto actor = fb::dynamic_pointer_cast<IActor>(object);
						auto tranform = fb::dynamic_pointer_cast<ITransform>(object);
						auto component = fb::dynamic_pointer_cast<component::IComponent>(object);
						auto resource = fb::dynamic_pointer_cast<IResource>(object);
						auto fileSelection = fb::dynamic_pointer_cast<FileSelection>(object);

						ActorWindow::ObjectType objectType = ActorWindow::ObjectType::ACTOR;

						if (actor)
						{
							objectType = ActorWindow::ObjectType::ACTOR;
						}
						else if (fileSelection)
						{
							auto filePath = fileSelection->getFilePath();
							auto ext = Path::getFileExtension(filePath);
							ext = StringUtil::make_lower(ext);

							if (ext == ".fbx")
							{
								objectType = ActorWindow::ObjectType::MESH;
							}
							else
							{
								objectType = ActorWindow::ObjectType::MATERIAL;
							}
						}

						m_objectType = objectType;
					}
				}

				buildTree();
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//-------------------------------------------------
		void ActorWindow::restoreItemState(String parent, wxTreeItemId itemId, bool parentWasNew)
		{
			// make item name
			String itemName = String(m_tree->GetItemText(itemId));
			if (parent != "")
				itemName = parent + "/" + itemName;

			// get item state from map
			int state = getItemState(itemName);

			bool isExpanded = false;
			bool showItem = false;
			if (state != TREE_ITEM_STATE_NOT_FOUND)
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
			if (isExpanded)
			{
				m_tree->Expand(itemId);
			}

			// show item
			if (showItem)
			{
				m_newSelectedItem = itemId;
			}

			// parse childes
			wxTreeItemIdValue cookie;
			wxTreeItemId childrenItem = m_tree->GetFirstChild(itemId, cookie);
			while (childrenItem.IsOk())
			{
				restoreItemState(itemName, childrenItem, parentWasNew);
				childrenItem = m_tree->GetNextChild(itemId, cookie);
			}
		}



		//-------------------------------------------------
		int ActorWindow::getItemState(String itemName)
		{
			std::map<String, bool>::iterator it;
			for (it = treeState.begin(); it != treeState.end(); it++)
			{
				String name = (*it).first;
				if (name == itemName)
					return (*it).second ? TREE_ITEM_STATE_EXPANDED : TREE_ITEM_STATE_NOT_EXPANDED;
			}

			return TREE_ITEM_STATE_NOT_FOUND;
		}



	} // end namespace editor
} // end namespace fb


