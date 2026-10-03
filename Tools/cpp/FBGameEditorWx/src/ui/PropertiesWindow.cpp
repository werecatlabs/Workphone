#include <GameEditorPCH.hpp>
#include <ui/PropertiesWindow.hpp>
#include <editor/EditorMessages.hpp>
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <ui/wxGUIUtil.hpp>
#include <ui/ActorWindow.hpp>
#include <FBCore/Memory/PointerUtil.hpp>
#include <FBCore/Interface/IApplicationManager.hpp>
#include <FBCore/Interface/System/ISelectionManager.hpp>
#include <FBCore/Interface/System/IEditableObject.hpp>
#include <FBCore/Interface/Actor/IActor.hpp>
#include <FBCore/Interface/Actor/IComponent.hpp>
#include <FBCore/Interface/Mesh/IMeshResource.hpp>
#include <FBCore/FBCoreHeaders.hpp>

#include <FBApplication/Components/TransformComponent.hpp>
#include <editor/FileSelection.hpp>
#include <wx/dialog.hpp>
#include <wx/sizer.hpp>



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		PropertiesWindow::PropertiesWindow(
			wxWindow* parent, wxWindowID id,
			const wxPoint& pos, const wxSize& size, long style,
			const wxValidator& validator, const wxString& name)
		{
			auto window = new wxScrolledWindow(parent, id, pos, size, style);
			setWindow(window);

			window->Bind(wxEVT_PG_CHANGED, &PropertiesWindow::propertyChange, this, PropertiesId);

			auto topSizer = new wxBoxSizer(wxVERTICAL);
			window->SetSizer(topSizer);

			auto pgman = new wxPropertyGridManager(window,
				PropertiesId,
				wxDefaultPosition,
				wxDefaultSize,
				wxPG_SPLITTER_AUTO_CENTER | wxPG_BOLD_MODIFIED);
			m_pg = pgman->GetGrid();
			topSizer->Add(pgman, 1, wxEXPAND);

			m_messageListener = new MessageListener(this);

			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			//editorManager->getMessageManager()->addListener(m_messageListener);

			auto uiManager = editorManager->getUI();
			if (uiManager)
			{
				uiManager->setPropertiesWindow(this);
			}
		}



		//--------------------------------------------
		PropertiesWindow::~PropertiesWindow()
		{
			//ApplicationManager* appRoot = IApplicationManager::instance();
			//appRoot->getMessageManager()->removeListener(m_messageListener);

			if (m_messageListener)
			{
				delete m_messageListener;
				m_messageListener = nullptr;
			}
		}



		//--------------------------------------------
		void PropertiesWindow::update()
		{
			if (isDirty())
			{
				updateSelection();
				setDirty(false);
			}
		}



		//--------------------------------------------
		void PropertiesWindow::objectSelected()
		{

		}



		//--------------------------------------------
		void PropertiesWindow::handleMessage(SmartPtr<IMessage> message)
		{
			String messageType = message->getType();

			if (messageType == ("EntityTreeItemSelected"))
			{
				EntityTreeItemSelectedPtr msg;// = message;

				SmartPtr<IEditableObject> editable = msg->getSelectedObject();
				if (editable)
				{
					actorSelected(editable);

				}
			}

			if (messageType == ("ComponentItemSelected"))
			{
				ComponentItemSelectedPtr msg = message;

				SmartPtr<IEditableObject> editable = msg->getSelectedObject();
				if (editable)
				{
					m_selectedEditable = editable;

					Properties propertyGroup;
					editable->getProperties(propertyGroup);
					wxGUIUtil::populateProperties(propertyGroup, m_pg);
				}
			}

			if (messageType == ("ComponentDeleteCurrentProperty"))
			{
				ComponentDeleteCurrentPropertyPtr msg = message;

				SmartPtr<IEditableObject> editable = msg->getSelectedObject();
				if (editable)
				{
					m_selectedEditable = editable;

					// get selected property in properties grid
					wxPGProperty* wxProperty = m_pg->GetSelectedProperty();
					if (wxProperty)
					{
						// get property name
						String name = String(wxProperty->GetName().c_str());

						// get property group for this component
						Properties propertyGroup;
						editable->getProperties(propertyGroup);
						// get property that we want to delete
						if (propertyGroup.hasProperty(name))
						{
							// delete property from property group
							propertyGroup.removeProperty(name);
							// set modified property group
							editable->setProperties(propertyGroup);
							// populate properties
							wxGUIUtil::populateProperties(propertyGroup, m_pg);
						}
					}
				}
			}
		}



		//--------------------------------------------
		void PropertiesWindow::actorSelected(SmartPtr<IEditableObject> editable)
		{
			m_selectedEditable = editable;

			Properties propertyGroup;
			editable->getProperties(propertyGroup);
			wxGUIUtil::populateProperties(propertyGroup, m_pg);
		}



		//--------------------------------------------
		void PropertiesWindow::updateSelection()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto selectionManager = applicationManager->getSelectionManager();
				FB_ASSERT(selectionManager);

				auto selection = selectionManager->getSelection();
				for (auto object : selection)
				{
					if (object)
					{
						if (object->isDerived<IActor>())
						{
							auto actor = fb::dynamic_pointer_cast<IActor>(object);

							auto properties = actor->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<ITransform>())
						{
							auto transform = fb::dynamic_pointer_cast<ITransform>(object);

							auto properties = transform->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<render::ISceneNode>())
						{
							auto sceneNode = fb::dynamic_pointer_cast<render::ISceneNode>(object);

							auto properties = sceneNode->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<render::IGraphicsObject>())
						{
							auto graphicsObject = fb::dynamic_pointer_cast<render::IGraphicsObject>(object);

							auto properties = graphicsObject->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<IStateObject>())
						{
							auto stateObject = fb::dynamic_pointer_cast<IStateObject>(object);

							auto properties = stateObject->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<IState>())
						{
							auto state = fb::dynamic_pointer_cast<IState>(object);

							auto properties = state->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<IMeshResource>())
						{
							auto meshResource = fb::static_pointer_cast<IMeshResource>(object);

							auto properties = meshResource->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<render::IMaterial>())
						{
							auto material = fb::static_pointer_cast<render::IMaterial>(object);

							auto properties = material->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<render::IMaterialNode>())
						{
							auto materialNode = fb::static_pointer_cast<render::IMaterialNode>(object);

							auto properties = materialNode->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<component::IComponent>())
						{
							auto component = fb::dynamic_pointer_cast<component::IComponent>(object);

							auto properties = component->getProperties();
							if (properties)
							{
								wxGUIUtil::populateProperties(*properties, m_pg);
							}
						}
						else if (object->isDerived<FileSelection>())
						{
							auto fileSelection = fb::dynamic_pointer_cast<FileSelection>(object);

							auto filePath = fileSelection->getFilePath();
							auto ext = Path::getFileExtension(filePath);

							static const auto materialExt = String(".mat");
							static const auto fbxExt = String(".fbx");

							if (ext == materialExt)
							{
								auto graphicsSystem = applicationManager->getGraphicsSystem();
								auto materialManager = graphicsSystem->getMaterialManager();
								auto material = materialManager->loadFromFile(filePath);
								if (material)
								{
									auto properties = material->getProperties();
									if (properties)
									{
										wxGUIUtil::populateProperties(*properties, m_pg);
									}
								}
							}
							else if (ext == fbxExt)
							{
								auto graphicsSystem = applicationManager->getGraphicsSystem();
								auto meshManager = applicationManager->getMeshManager();
								auto mesh = meshManager->loadFromFile(filePath);
								if (mesh)
								{
									auto properties = mesh->getProperties();
									if (properties)
									{
										wxGUIUtil::populateProperties(*properties, m_pg);
									}
								}
							}
						}
					}
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//--------------------------------------------
		bool PropertiesWindow::isDirty() const
		{
			return m_isDirty;
		}



		//--------------------------------------------
		void PropertiesWindow::setDirty(bool val)
		{
			m_isDirty = val;
		}



		//--------------------------------------------
		void PropertiesWindow::propertyChange(wxPropertyGridEvent& event)
		{
			auto p = event.GetProperty();

			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto selectionManager = applicationManager->getSelectionManager();
			FB_ASSERT(selectionManager);

			auto editorManager = EditorManager::getSingletonPtr();
			FB_ASSERT(editorManager);

			auto ui = editorManager->getUI();
			FB_ASSERT(ui);

			auto selection = selectionManager->getSelection();
			for (auto object : selection)
			{
				if (object)
				{
					auto actor = fb::dynamic_pointer_cast<IActor>(object);
					auto transform = fb::dynamic_pointer_cast<ITransform>(object);
					auto component = fb::dynamic_pointer_cast<component::IComponent>(object);
					auto sceneNode = fb::dynamic_pointer_cast<render::ISceneNode>(object);
					auto graphicsObject = fb::dynamic_pointer_cast<render::IGraphicsObject>(object);
					auto stateObject = fb::dynamic_pointer_cast<IStateObject>(object);
					auto state = fb::dynamic_pointer_cast<IState>(object);

					if (actor)
					{
						auto properties = actor->getProperties();
						if (properties)
						{
							wxGUIUtil::setPropertyValue(*properties, m_pg, p);
							actor->setProperties(properties);
						}
					}
					else if (transform)
					{
						auto properties = transform->getProperties();
						if (properties)
						{
							wxGUIUtil::setPropertyValue(*properties, m_pg, p);
							transform->setProperties(properties);
						}
					}
					else if (component)
					{
						auto properties = component->getProperties();
						if (properties)
						{
							wxGUIUtil::setPropertyValue(*properties, m_pg, p);
							component->setProperties(properties);
						}
					}
					else if (sceneNode)
					{
						auto properties = sceneNode->getProperties();
						if (properties)
						{
							wxGUIUtil::setPropertyValue(*properties, m_pg, p);
							sceneNode->setProperties(properties);
						}
					}
					else if (graphicsObject)
					{
						auto properties = graphicsObject->getProperties();
						if (properties)
						{
							wxGUIUtil::setPropertyValue(*properties, m_pg, p);
							graphicsObject->setProperties(properties);
						}
					}
					else if (stateObject)
					{
						auto properties = stateObject->getProperties();
						if (properties)
						{
							wxGUIUtil::setPropertyValue(*properties, m_pg, p);
							stateObject->setProperties(properties);
						}
					}
					else if (state)
					{
						auto properties = state->getProperties();
						if (properties)
						{
							wxGUIUtil::setPropertyValue(*properties, m_pg, p);
							state->setProperties(properties);
						}
					}
					else if (object->isDerived<IMeshResource>())
					{
						auto meshResource = fb::static_pointer_cast<IMeshResource>(object);
						if (meshResource)
						{
							auto properties = meshResource->getProperties();
							if (properties)
							{
								wxGUIUtil::setPropertyValue(*properties, m_pg, p);
								meshResource->setProperties(properties);
							}

							meshResource->save();
						}
					}
					else if (object->isDerived<render::IMaterial>())
					{
						auto material = fb::static_pointer_cast<render::IMaterial>(object);
						if (material)
						{
							auto properties = material->getProperties();
							if (properties)
							{
								wxGUIUtil::setPropertyValue(*properties, m_pg, p);
								material->setProperties(properties);
							}

							material->save();

							ui->rebuildActorTree();
						}
					}
					else if (object->isDerived<render::IMaterialPass>())
					{
						auto materialPass = fb::static_pointer_cast<render::IMaterialPass>(object);
						if (materialPass)
						{
							auto properties = materialPass->getProperties();
							if (properties)
							{
								wxGUIUtil::setPropertyValue(*properties, m_pg, p);
								materialPass->setProperties(properties);
							}
						}

						auto technique = fb::static_pointer_cast<render::IMaterialTechnique>(materialPass->getParent());
						if (technique)
						{
							auto material = technique->getOwner();
							if (material)
							{
								material->save();
							}
						}
					}
					else if (object->isDerived<render::IMaterialTexture>())
					{
						auto materialTexture = fb::static_pointer_cast<render::IMaterialTexture>(object);
						if (materialTexture)
						{
							auto properties = materialTexture->getProperties();
							if (properties)
							{
								wxGUIUtil::setPropertyValue(*properties, m_pg, p);
								materialTexture->setProperties(properties);
							}
						}

						auto materialPass = fb::static_pointer_cast<render::IMaterialTechnique>(materialTexture->getParent());
						if (materialPass)
						{
							auto technique = fb::static_pointer_cast<render::IMaterialTechnique>(materialPass->getParent());
							if (technique)
							{
								auto material = technique->getOwner();
								if (material)
								{
									material->save();
								}
							}
						}
					}
					/*
					else if (object->isDerived<FileSelection>())
					{
						auto fileSelection = fb::static_pointer_cast<FileSelection>(object);
						if (fileSelection)
						{
							auto filePath = fileSelection->getFilePath();
							auto fileExt = Path::getFileExtension(filePath);

							if (fileExt == ".mat")
							{
								auto graphicsSystem = applicationManager->getGraphicsSystem();
								FB_ASSERT(graphicsSystem);

								auto materialManager = graphicsSystem->getMaterialManager();
								FB_ASSERT(materialManager);

								auto material = materialManager->loadFromFile(filePath);
								if (material)
								{
									auto properties = material->getProperties();
									if (properties)
									{
										wxGUIUtil::setPropertyValue(*properties, m_pg, p);
										material->setProperties(properties);
									}

									material->save();
								}
							}
						}
					}
					*/
				}
			}

			//auto editorManager = EditorManager::getSingletonPtr();
			//auto ui = editorManager->getUI();
			//auto actorWindow = ui->getActorWindow();
			//if (actorWindow)
			//{
			//	actorWindow->buildTree();
			//}

			setDirty(true);
		}



		//--------------------------------------------
		PropertiesWindow::MessageListener::~MessageListener()
		{

		}



		//--------------------------------------------
		PropertiesWindow::MessageListener::MessageListener(PropertiesWindow* propertiesWindow)
			: m_propertiesWindow(propertiesWindow)
		{

		}



		//--------------------------------------------
		void PropertiesWindow::MessageListener::handleMessage(SmartPtr<IMessage> message)
		{
			m_propertiesWindow->handleMessage(message);
		}



	} // end namespace editor	
} // end namespace fb

