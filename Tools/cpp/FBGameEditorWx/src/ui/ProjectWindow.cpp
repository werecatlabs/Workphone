#include <GameEditorPCH.hpp>

#include "ui/ProjectWindow.hpp"
#include "ui/GuiTypes.hpp"

#include "ui/AddComponentDialog.hpp"
#include "ui/AddNamedDialog.hpp"
#include "ui/AddEntityDialog.hpp"
#include "ui/AddFSMStateDialog.hpp"
#include "ui/FileWindow.hpp"

#include "editor/EditorMessages.hpp"
#include "editor/EditorManager.hpp"
#include "editor/SceneViewManager.hpp"
#include <FBApplication/Script/ScriptGenerator.hpp>

#include "ui/ProjectTreeData.hpp"
#include "ui/UIManager.hpp"

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

#include "GameEditorTypes.hpp"
#include "core/MessageManager.hpp"
#include <FBState/FBState.hpp>
#include <FBCore/FBCoreHeaders.hpp>

#include "editor/Project.hpp"
#include "editor/ProjectManager.hpp"
#include "editor/ComponentTemplateMgr.hpp"

#define TREE_ITEM_STATE_NOT_FOUND		0
#define TREE_ITEM_STATE_EXPANDED		1
#define TREE_ITEM_STATE_NOT_EXPANDED	2






namespace fb
{
	namespace editor
	{



		//-------------------------------------------------
		ProjectWindow::ProjectWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos,
			const wxSize& size, long style, const wxValidator& validator, const wxString& name)
		{
			try
			{
				m_lastSelectedItem = 0;

				auto window = new wxScrolledWindow(parent, id, pos, size, style);
				setWindow(window);

				window->Bind(wxEVT_TREE_SEL_CHANGED, &ProjectWindow::handleTreeSelectionChanged, this, ENTITY_TREE_ID);

				wxBoxSizer* baseSizer = new wxBoxSizer(wxVERTICAL);
				window->SetSizer(baseSizer);

				m_tree = new wxTreeCtrl(window, ENTITY_TREE_ID);
				baseSizer->Add(m_tree, 1, wxEXPAND);

				m_applicationMenu = new wxMenu();
				m_applicationAddMenu = new wxMenu();
				m_applicationAddMenu->Append(APPLICATION_MENU_ADD_EVENT, "Add Component");
				m_applicationAddMenu->Append(APPLICATION_MENU_ADD_EVENT, "Add Event");
				m_applicationAddMenu->Append(APPLICATION_MENU_ADD_EVENT, "Add Script");
				m_applicationAddMenu->Append(APPLICATION_MENU_ADD_EVENT, "Add Task");
				m_applicationMenu->AppendSubMenu(m_applicationAddMenu, "Add");
				m_applicationMenu->AppendSeparator();
				m_applicationMenu->Append(APPLICATION_MENU_ADD_EVENT, "Remove");

				m_entityMenu = new wxMenu();
				m_entitySubMenu = new wxMenu();
				m_entitySubMenu->Append(MENU_ADD_BUILDER_ID, "Builder");
				m_entitySubMenu->Append(MENU_ADD_COMPONENT_ID, "Component");
				m_entitySubMenu->Append(PW_EVENT_ADD, "Event");
				m_entitySubMenu->Append(MENU_ADD_FSM_ID, "FSM");
				m_entityMenu->AppendSubMenu(m_entitySubMenu, "Add");
				m_entityMenu->AppendSeparator();
				m_entityMenu->Append(PW_ENTITY_REMOVE, "Remove Entity", "Removes an entity from the project.");
				m_entityMenu->AppendSeparator();
				m_entityMenu->Append(PW_GENERATE_SCRIPT, "Generate Script", "Generates a script from the object properties.");

				m_entitiesMenu = new wxMenu();
				m_entitiesAddMenu = new wxMenu();
				m_entitiesAddMenu->Append(PW_ADD_NEW_ENTITY, "Add New");
				m_entitiesAddMenu->Append(PW_ADD_EXISTING_ENTITY, "Add Existing");
				m_entitiesMenu->AppendSubMenu(m_entitiesAddMenu, "Add Object");

				m_entityGfxLabelMenu = new wxMenu();
				m_entityGfxLabelMenu->Append(PW_ADD_ENT_SCENE_NODE, "Add Scene Node");
				m_entityGfxLabelMenu->Append(PW_ADD_ENT_GFX_OBJ, "Add Graphics Object");
				m_entityGfxLabelMenu->AppendSeparator();
				m_entityGfxLabelMenu->Append(1, "Cancel");

				m_entityGfxObjLabelMenu = new wxMenu();
				m_entityGfxObjLabelMenu->Append(PW_ADD_ENT_GFX_OBJ, "Add Graphics Object");
				m_entityGfxObjLabelMenu->AppendSeparator();
				m_entityGfxObjLabelMenu->Append(1, "Cancel");

				m_entityGfxObjMenu = new wxMenu();
				m_entityGfxObjMenu->Append(PW_REMOVE_ENT_GFX_OBJ, "Remove Graphics Object");
				m_entityGfxObjMenu->AppendSeparator();
				m_entityGfxObjMenu->Append(1, "Cancel");

				m_entityNodeLabelMenu = new wxMenu();
				m_entityNodeLabelMenu->Append(PW_ADD_ENT_SCENE_NODE, "Add Scene Node");
				m_entityNodeLabelMenu->AppendSeparator();
				m_entityNodeLabelMenu->Append(1, "Cancel");

				m_entityNodeMenu = new wxMenu();
				m_entityNodeMenu->Append(PW_REMOVE_ENT_SCENE_NODE, "Remove Scene Node");
				m_entityNodeMenu->AppendSeparator();
				m_entityNodeMenu->Append(1, "Cancel");

				m_entityPhysicsLabelMenu = new wxMenu();
				m_entityPhysicsLabelMenu->Append(PW_ADD_ENT_SHAPE, "Add Shape");
				m_entityPhysicsLabelMenu->Append(PW_ADD_ENT_BODY, "Add Body");
				m_entityPhysicsLabelMenu->AppendSeparator();
				m_entityPhysicsLabelMenu->Append(1, "Cancel");

				m_entityShapeLabelMenu = new wxMenu();
				m_entityShapeLabelMenu->Append(PW_ADD_ENT_SHAPE, "Add Shape");
				m_entityShapeLabelMenu->AppendSeparator();
				m_entityShapeLabelMenu->Append(1, "Cancel");

				m_entityShapeMenu = new wxMenu();
				m_entityShapeMenu->Append(PW_REMOVE_ENT_SHAPE, "Remove Shape");
				m_entityShapeMenu->AppendSeparator();
				m_entityShapeMenu->Append(1, "Cancel");

				m_entityBodyLabelMenu = new wxMenu();
				m_entityBodyLabelMenu->Append(PW_ADD_ENT_BODY, "Add Body");
				m_entityBodyLabelMenu->AppendSeparator();
				m_entityBodyLabelMenu->Append(1, "Cancel");

				m_entityBodyMenu = new wxMenu();
				m_entityBodyMenu->Append(PW_REMOVE_ENT_BODY, "Remove Body");
				m_entityBodyMenu->AppendSeparator();
				m_entityBodyMenu->Append(1, "Cancel");

				m_entitySoundLabelMenu = new wxMenu();
				m_entitySoundLabelMenu->Append(PW_ADD_ENT_SOUND, "Add Sound");
				m_entitySoundLabelMenu->AppendSeparator();
				m_entitySoundLabelMenu->Append(1, "Cancel");

				m_entitySoundMenu = new wxMenu();
				m_entitySoundMenu->Append(PW_REMOVE_ENT_SOUND, "Remove Sound");
				m_entitySoundMenu->AppendSeparator();
				m_entitySoundMenu->Append(1, "Cancel");

				m_entityScriptLabelMenu = new wxMenu();
				m_entityScriptLabelMenu->Append(MENU_ADD_SCRIPT_ID, "Add Script");
				m_entityScriptLabelMenu->AppendSeparator();
				m_entityScriptLabelMenu->Append(1, "Cancel");

				m_entityScriptMenu = new wxMenu();
				m_entityScriptMenu->Append(PW_ENT_SCRIPT_REMOVE, "Remove Script");
				m_entityScriptMenu->AppendSeparator();
				m_entityScriptMenu->Append(1, "Cancel");

				m_entityResLabelMenu = new wxMenu();
				m_entityResLabelMenu->Append(PW_ADD_ENT_SCENE_NODE, "Add Scene Node");
				m_entityResLabelMenu->Append(PW_ADD_ENT_GFX_OBJ, "Add Graphics Object");
				m_entityResLabelMenu->AppendSeparator();
				m_entityResLabelMenu->Append(PW_ADD_ENT_SHAPE, "Add Shape");
				m_entityResLabelMenu->Append(PW_ADD_ENT_BODY, "Add Body");
				m_entityResLabelMenu->AppendSeparator();
				m_entityResLabelMenu->Append(PW_ADD_ENT_SOUND, "Add Sound");
				m_entityResLabelMenu->AppendSeparator();
				m_entityResLabelMenu->Append(MENU_ADD_SCRIPT_ID, "Add Script");
				m_entityResLabelMenu->AppendSeparator();
				m_entityResLabelMenu->Append(1, "Cancel");

				m_guiMenu = new wxMenu();
				m_guiAddMenu = new wxMenu();
				m_guiAddMenu->Append(PW_ADD_NEW_GUI, "Add New");
				m_guiAddMenu->Append(PW_ADD_EXISTING_GUI, "Add Existing");
				m_guiMenu->AppendSubMenu(m_guiAddMenu, "Add GUI");
				m_guiMenu->AppendSeparator();

				m_componentMenu = new wxMenu();
				m_componentMenu->Append(PW_COMPONENT_REMOVE, "Remove Component");
				m_componentMenu->AppendSeparator();
				m_componentMenu->Append(1, "Cancel");

				m_labelComponentMenu = new wxMenu();
				m_labelComponentMenu->Append(MENU_ADD_COMPONENT_ID, "Add Components");
				m_labelComponentMenu->AppendSeparator();
				m_labelComponentMenu->Append(1, "Cancel");

				m_labelFSMMenu = new wxMenu();
				m_labelFSMMenu->Append(MENU_ADD_FSM_ID, "Add FSM");
				m_labelFSMMenu->AppendSeparator();
				m_labelFSMMenu->Append(1, "Cancel");

				m_fsmMenu = new wxMenu();
				m_fsmMenu->Append(PW_FSM_REMOVE, "Remove FSM");
				m_fsmMenu->AppendSeparator();
				m_fsmMenu->Append(1, "Cancel");

				m_labelEventMenu = new wxMenu();
				m_labelEventMenu->Append(PW_EVENT_ADD, "Add Event");
				m_labelEventMenu->AppendSeparator();
				m_labelEventMenu->Append(1, "Cancel");

				m_eventMenu = new wxMenu();
				m_eventMenu->Append(PW_EVENT_MODIFY, "Modify Event");
				m_eventMenu->Append(PW_EVENT_REMOVE, "Remove Event");
				m_eventMenu->AppendSeparator();
				m_eventMenu->Append(1, "Cancel");

				m_fsmLabelEventMenu = new wxMenu();
				m_fsmLabelEventMenu->Append(PW_FSM_EVENT_ADD, "Add Event");
				m_fsmLabelEventMenu->AppendSeparator();
				m_fsmLabelEventMenu->Append(1, "Cancel");

				m_fsmEventMenu = new wxMenu();
				m_fsmEventMenu->Append(PW_FSM_EVENT_MODIFY, "Modify Event");
				m_fsmEventMenu->Append(PW_FSM_EVENT_REMOVE, "Remove Event");
				m_fsmEventMenu->AppendSeparator();
				m_fsmEventMenu->Append(1, "Cancel");

				m_fsmLabelStateMenu = new wxMenu();
				m_fsmLabelStateMenu->Append(PW_FSM_STATE_ADD, "Add State");
				m_fsmLabelStateMenu->AppendSeparator();
				m_fsmLabelStateMenu->Append(1, "Cancel");

				m_fsmStateMenu = new wxMenu();
				m_fsmStateMenu->Append(PW_FSM_STATE_REMOVE, "Remove State");
				m_fsmStateMenu->AppendSeparator();
				m_fsmStateMenu->Append(1, "Cancel");

				m_defaultMenu = new wxMenu();
				m_defaultMenu->Append(1, "Help");

				m_scenesMenu = new wxMenu();
				m_scenesAddMenu = new wxMenu();
				m_scenesMenu->AppendSubMenu(m_scenesAddMenu, "Add");
				m_scenesAddMenu->Append(-1, "Add New");
				m_scenesAddMenu->Append(-1, "Add Existing");
				m_scenesMenu->Append(-1, "Remove");
				m_scenesMenu->Append(-1, "Rename");

				m_sceneMenu = new wxMenu();
				m_sceneMenu->Append(-1, "Add Map");
				m_sceneMenu->Append(-1, "Add Overlay");
				m_sceneMenu->Append(-1, "Remove");
				m_sceneMenu->Append(-1, "Rename");

				m_scriptsMenu = new wxMenu();
				m_scriptsAddMenu = new wxMenu();
				m_scriptsMenu->AppendSubMenu(m_scriptsAddMenu, "Add");
				m_scriptsAddMenu->Append(PW_ADD_NEW_SCRIPT, "Add New");
				m_scriptsAddMenu->Append(PW_ADD_EXISTING_SCRIPT, "Add Existing");
				m_scriptsAddMenu->Append(PW_ADD_SCRIPT_FOLDER, "Add Folder");
				m_scriptsMenu->Append(PW_REMOVE_SCRIPT, "Remove");
				m_scriptsMenu->Append(PW_RENAME_SCRIPT, "Rename");

				m_scriptMenu = new wxMenu();
				m_scriptMenu->Append(1, "Open");
				m_scriptMenu->Append(PW_RENAME_SCRIPT, "Rename");
				m_scriptMenu->Append(PW_REMOVE_SCRIPT, "Remove");

				m_resourceMenu = new wxMenu();

				m_resourceGfxMenu = new wxMenu();
				m_resourceGfxMenu->Append(PW_ADD_TERRAIN, "Add Terrain");
				m_resourceGfxMenu->Append(PW_ADD_MESH, "Add Mesh");
				m_resourceGfxMenu->Append(PW_ADD_DESTRUCTIBLE_MESH, "Add Destructible");
				m_resourceGfxMenu->Append(PW_ADD_PARTICLE_SYSTEM, "Add Particle");

				m_resourcePhysicsMenu = new wxMenu();
				m_resourceSoundMenu = new wxMenu();

				m_vehiclesMenu = new wxMenu();
				m_vehiclesMenu->Append(PW_ADD_VEHICLE_4_WHEEL, "Add 4 Wheel");
				m_vehiclesMenu->Append(PW_ADD_VEHICLE_AIRPLANE, "Add Airplane");
				m_vehiclesMenu->Append(PW_ADD_VEHICLE_HELICOPTER, "Add Helicopter");
				m_vehiclesMenu->Append(PW_ADD_VEHICLE_TANK, "Add Tank");

				m_vehicleMenu = new wxMenu();
				m_vehicleSubMenu = new wxMenu();
				m_vehicleSubMenu->Append(MENU_ADD_FSM_ID, "FSM");
				m_vehicleSubMenu->Append(MENU_ADD_COMPONENT_ID, "Component");
				m_vehicleSubMenu->Append(PW_EVENT_ADD, "Event");
				m_vehicleSubMenu->Append(PW_EVENT_ADD, "Vehicle Component");
				m_vehicleMenu->AppendSubMenu(m_vehicleSubMenu, "Add");
				m_vehicleMenu->AppendSeparator();
				m_vehicleMenu->Append(PW_ENTITY_REMOVE, "Remove Entity", "Removes an entity from the project.");
				m_vehicleMenu->AppendSeparator();
				m_vehicleMenu->Append(PW_GENERATE_SCRIPT, "Generate Script", "Generates a script from the object properties.");

				m_dotSceneMenu = new wxMenu;
				m_dotSceneMenu->Append(PW_ADD_SCENE, "Add Scene");

				m_aiMenu = new wxMenu;
				m_aiMenu->Append(PW_ADD_GOAL, "Add Goal");
				m_aiMenu->Append(PW_ADD_EVALUATOR, "Add Evaluator");

				m_mapsMenu = new wxMenu;
				m_mapsMenu->Append(PW_ADD_MAP, "Add Map");
				m_mapsMenu->Append(PW_MAP_REFRESH, "Refresh Map");

				m_camerasMenu = new wxMenu;
				m_camerasMenu->Append(MENU_ADD_CAMERA, "Add Camera");
				m_camerasMenu->Append(MENU_REMOVE_CAMERA, "Remove Camera");

				m_materialsMenu = new wxMenu();
				m_materialsMenu->Append(PW_ADD_MATERIAL, "Add Material");

				m_materialMenu = new wxMenu();
				m_materialMenu->Append(PW_ADD_TECHNIQUE, "Add Technique");

				EditorManager* appRoot = EditorManager::getSingletonPtr();

				m_messageListener = new ProjectWindowMessageListener(this);
				appRoot->getMessageManager()->addListener(m_messageListener);
				appRoot->getUI()->setProjectWindow(this);

				ProjectPtr project = appRoot->getProject();
				m_parentFilter = SmartPtr<TemplateFilter>(new TemplateFilter);
				m_parentFilter->setLabel("Scripts");
				project->addFilter(m_parentFilter);

				//m_applicationMenu->Connect(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ProjectWindow::OnRightClick), NULL, this);
				//PopupMenu(m_applicationMenu);
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//-------------------------------------------------
		ProjectWindow::~ProjectWindow()
		{
			try
			{
				FB_SAFE_DELETE(m_defaultMenu);

				FB_SAFE_DELETE(m_entitiesMenu);

				FB_SAFE_DELETE(m_entityResLabelMenu);
				FB_SAFE_DELETE(m_entityGfxLabelMenu);
				FB_SAFE_DELETE(m_entityGfxObjLabelMenu);
				FB_SAFE_DELETE(m_entityGfxObjMenu);
				FB_SAFE_DELETE(m_entityNodeLabelMenu);
				FB_SAFE_DELETE(m_entityNodeMenu);

				FB_SAFE_DELETE(m_entitySoundLabelMenu);
				FB_SAFE_DELETE(m_entitySoundMenu);

				FB_SAFE_DELETE(m_entityMenu);

				FB_SAFE_DELETE(m_guiMenu);

				FB_SAFE_DELETE(m_scenesMenu);

				FB_SAFE_DELETE(m_componentMenu);
				FB_SAFE_DELETE(m_labelComponentMenu);

				FB_SAFE_DELETE(m_fsmMenu);
				FB_SAFE_DELETE(m_labelFSMMenu);

				FB_SAFE_DELETE(m_eventMenu);
				FB_SAFE_DELETE(m_labelEventMenu);

				FB_SAFE_DELETE(m_fsmEventMenu);
				FB_SAFE_DELETE(m_fsmLabelEventMenu);

				FB_SAFE_DELETE(m_fsmStateMenu);
				FB_SAFE_DELETE(m_fsmLabelStateMenu);

				EditorManager* appRoot = EditorManager::getSingletonPtr();
				//appRoot->getMessageManager()->removeListener(m_messageListener);
				//FB_SAFE_DELETE(m_messageListener);
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//-------------------------------------------------
		void ProjectWindow::update(time_interval t, time_interval dt)
		{
		}



		//-------------------------------------------------
		void ProjectWindow::handleMessage(SmartPtr<IMessage> message)
		{
			String type = message->getType();
			if (type == ("SelectEntityTemplate"))
			{
				buildTree();
			}
			else if (type == ("LoadProjectMsg"))
			{
				buildTree();
			}
		}



		//-------------------------------------------------
		void ProjectWindow::addFolderToTree(wxTreeItemId parent, SmartPtr<IFolderListing> listing)
		{
			try
			{
				auto folderPath = listing->getFolderName();

				auto editorManager = EditorManager::getSingletonPtr();
				auto project = editorManager->getProject();
				ProjectTreeData data("folder", folderPath, project, project);

				auto folderName = Path::getFileName(folderPath);
				auto folderTreeId = m_tree->AppendItem(parent, folderName, -1, -1, new ProjectTreeData(data));

				auto subFolders = listing->getSubFolders();
				for (auto& subFolder : subFolders)
				{
					addFolderToTree(folderTreeId, subFolder);
				}
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//-------------------------------------------------
		void ProjectWindow::buildTree()
		{
			try
			{
				m_tree->DeleteAllItems();

				auto applicationManager = IApplicationManager::instance();
				auto fileSystem = applicationManager->getFileSystem();

				auto editorManager = EditorManager::getSingletonPtr();
				auto project = editorManager->getProject();
				auto appTemplate = project->getApplicationTemplate();

				ProjectTreeData data("project", "project", project, project);
				ProjectTreeData scriptsData(m_scriptsMenu, "project", "filter", project, m_parentFilter);

				auto rootId = m_tree->AddRoot("Project", -1, -1, new ProjectTreeData(data));

				auto projectFolder = applicationManager->getProjectPath();
				if (!StringUtil::isNullOrEmpty(projectFolder))
				{
					auto folderListing = fileSystem->getDirectoryListing(projectFolder + "/Assets");
					if (folderListing)
					{
						//auto folderName = folderListing->getFolderName();
						//wxTreeItemId folderTreeId = m_tree->AppendItem(rootId, folderName, -1, -1, new ProjectTreeData(data));

						auto subFolders = folderListing->getSubFolders();
						for (auto& subFolder : subFolders)
						{
							addFolderToTree(rootId, subFolder);
						}
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
		void ProjectWindow::populateProjectResources(wxTreeItemId resourcesId, SmartPtr<ResourceSetTemplate> resourcesetTemplate, ProjectPtr project)
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//ProjectPtr project = appRoot->getProject();

			ProjectTreeData gfxData(m_resourceGfxMenu, "project", "resourceset", project, resourcesetTemplate);

			wxTreeItemId graphicsId = m_tree->AppendItem(resourcesId, "Graphics", -1, -1, new ProjectTreeData(gfxData));

			ProjectTreeData materialsData(m_materialsMenu, "project", "project", project, project);
			wxTreeItemId graphicsMaterialsId = m_tree->AppendItem(graphicsId, "Materials", -1, -1, new ProjectTreeData(materialsData));

			Array<SmartPtr<MaterialTemplate>> materialTemplates = project->getMaterials();
			for (u32 i = 0; i < materialTemplates.size(); ++i)
			{
				SmartPtr<MaterialTemplate> materialTemplate = materialTemplates[i];

				ProjectTreeData materialData(m_materialMenu, "project", "material", project, materialTemplate);
				wxTreeItemId materialId = m_tree->AppendItem(graphicsMaterialsId, materialTemplate->getLabel().c_str(), -1, -1, new ProjectTreeData(materialData));
			}

			m_graphicsMeshId = m_tree->AppendItem(graphicsId, "Mesh", -1, -1, new ProjectTreeData(gfxData));
			m_particleId = m_tree->AppendItem(graphicsId, "Particle", -1, -1, new ProjectTreeData(gfxData));

			ProjectTreeData gfxDotSceneData(m_dotSceneMenu, "project", "resourceset", project, resourcesetTemplate);
			wxTreeItemId graphicsSceneId = m_tree->AppendItem(graphicsId, "Scene", -1, -1, new ProjectTreeData(gfxDotSceneData));

			wxTreeItemId physicssId = m_tree->AppendItem(resourcesId, "Physics", -1, -1);


			wxTreeItemId soundsId = m_tree->AppendItem(resourcesId, "Sounds", -1, -1);

			/*
			ProjectTreeData data(m_entityMenu, "entity", "entity", (void*)entityTemplate.get(), (void*)entityTemplate.get());
			ProjectTreeData gfxData(m_entityGfxLabelMenu, "entity", "entity", (void*)entityTemplate.get(), (void*)entityTemplate.get());
			ProjectTreeData gfxObjData(m_entityGfxObjLabelMenu, "entity", "entity", (void*)entityTemplate.get(), (void*)entityTemplate.get());
			ProjectTreeData gfxNodeData(m_entityNodeLabelMenu, "entity", "entity", (void*)entityTemplate.get(), (void*)entityTemplate.get());
			ProjectTreeData soundData(m_entitySoundLabelMenu, "entity", "entity", (void*)entityTemplate.get(), (void*)entityTemplate.get());
			ProjectTreeData shapeLabelData(m_entityShapeLabelMenu, "entity", "entity", (void*)entityTemplate.get(), (void*)entityTemplate.get());
			ProjectTreeData bodyLabelData(m_entityBodyLabelMenu, "entity", "entity", (void*)entityTemplate.get(), (void*)entityTemplate.get());
			ProjectTreeData physicsLabelData(m_entityPhysicsLabelMenu, "entity", "entity", (void*)entityTemplate.get(), (void*)entityTemplate.get());
			ProjectTreeData scriptLabelData(m_entityScriptLabelMenu, "entity", "entity", (void*)entityTemplate.get(), (void*)entityTemplate.get());

			wxTreeItemId graphicsId = m_tree->AppendItem(resourcesId, "Graphics", -1, -1, new ProjectTreeData(gfxData));
			wxTreeItemId physicssId = m_tree->AppendItem(resourcesId, "Physics", -1, -1, new ProjectTreeData(physicsLabelData));
			//wxTreeItemId scriptsId = m_tree->AppendItem(resourcesId, "Scripts", -1, -1, new ProjectTreeData(scriptLabelData));
			wxTreeItemId soundsId = m_tree->AppendItem(resourcesId, "Sounds", -1, -1, new ProjectTreeData(soundData));

			wxTreeItemId graphicsObjectsId = m_tree->AppendItem(graphicsId, "GraphicsObjects", -1, -1, new ProjectTreeData(gfxObjData));
			wxTreeItemId sceneNodesId = m_tree->AppendItem(graphicsId, "SceneNodes", -1, -1, new ProjectTreeData(gfxNodeData));

			wxTreeItemId collisionShapesId = m_tree->AppendItem(physicssId, "Shapes", -1, -1, new ProjectTreeData(shapeLabelData));
			wxTreeItemId bodiesId = m_tree->AppendItem(physicssId, "Bodies", -1, -1, new ProjectTreeData(bodyLabelData));

			//scene nodes
			Array<SmartPtr<SceneNodeTemplate>> nodeTemplates = resourcesetTemplate->getNodes();
			for(u32 i=0; i<nodeTemplates.size(); ++i)
			{
				SmartPtr<SceneNodeTemplate>& nodeTemplate = nodeTemplates[i];

				ProjectTreeData nodeData(m_entityNodeMenu, "entity", "node", (void*)entityTemplate.get(), (void*)nodeTemplate.get());
				m_tree->AppendItem(sceneNodesId, nodeTemplate->getLabel().c_str(), -1, -1, new ProjectTreeData(nodeData));
			}

			//graphic objects
			Array<SmartPtr<GfxObjectTemplate>> gfxObjectTemplates = entityTemplate->getResourceSetTemplate()->getGfxObjects();
			for(u32 i=0; i<gfxObjectTemplates.size(); ++i)
			{
				SmartPtr<GfxObjectTemplate>& gfxObjectTemplate = gfxObjectTemplates[i];
				ProjectTreeData gfxObjData(m_entityGfxObjMenu, "entity", "gfxObject", (void*)entityTemplate.get(), (void*)gfxObjectTemplate.get());
				m_tree->AppendItem(graphicsObjectsId, gfxObjectTemplate->getLabel().c_str(), -1, -1, new ProjectTreeData(gfxObjData));
			}

			//
			//Physics
			//

			//collision shapes
			Array<CollisionShapeTemplatePtr> shapeTemplates = entityTemplate->getResourceSetTemplate()->getShapes();
			for(u32 i=0; i<shapeTemplates.size(); ++i)
			{
				CollisionShapeTemplatePtr& shapeTemplate = shapeTemplates[i];
				ProjectTreeData shapeData(m_entityShapeMenu, "entity", "shape", (void*)entityTemplate.get(), (void*)shapeTemplate.get());
				m_tree->AppendItem(collisionShapesId, shapeTemplate->getLabel().c_str(), -1, -1, new ProjectTreeData(shapeData));
			}

			//physics bodies
			Array<SmartPtr<PhysicsBodyTemplate>> bodyTemplates = entityTemplate->getResourceSetTemplate()->getBodies();
			for(u32 i=0; i<bodyTemplates.size(); ++i)
			{
				SmartPtr<PhysicsBodyTemplate>& bodyTemplate = bodyTemplates[i];
				ProjectTreeData bodyData(m_entityBodyMenu, "entity", "body", (void*)entityTemplate.get(), (void*)bodyTemplate.get());
				m_tree->AppendItem(bodiesId, bodyTemplate->getLabel().c_str(), -1, -1, new ProjectTreeData(bodyData));
			}

			//
			//Sounds
			//
			const Array<SoundTemplatePtr>& soundTemplates = resourcesetTemplate->getSounds();
			for(u32 i=0; i<soundTemplates.size(); ++i)
			{
				SoundTemplatePtr soundTemplate = soundTemplates[i];
				ProjectTreeData soundData(m_entitySoundMenu, "entity", "sound", (void*)entityTemplate.get(), (void*)soundTemplate.get());
				m_tree->AppendItem(soundsId, soundTemplate->getLabel().c_str(), -1, -1, new ProjectTreeData(soundData));
			}
			*/
		}

		//-------------------------------------------------
		void ProjectWindow::populateCameras(wxTreeItemId rootId)
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();

			SmartPtr<CameraManagerTemplate> cameraManagerTemplate = project->getCameraManagerTemplate();
			if (!cameraManagerTemplate)
				return;

			Array<SmartPtr<CameraTemplate>> cameraTemplates = cameraManagerTemplate->getTemplates();
			for (u32 i = 0; i < cameraTemplates.size(); ++i)
			{
				SmartPtr<CameraTemplate> cameraTemplate = cameraTemplates[i];
				String label = cameraTemplate->getLabel();

				ProjectTreeData cameraTemplateData(m_camerasMenu, "cameraManagerTemplate", "cameraTemplate", cameraManagerTemplate, cameraTemplate);
				wxTreeItemId cameraId = m_tree->AppendItem(rootId, label.c_str(), -1, -1, new ProjectTreeData(cameraTemplateData));
			}
		}



		//-------------------------------------------------
		void ProjectWindow::populateApplication(wxTreeItemId rootId)
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			SmartPtr<ApplicationTemplate> appTemplate = appRoot->getProject()->getApplicationTemplate();

			wxTreeItemId componentsId = m_tree->AppendItem(rootId, "Components");
			wxTreeItemId eventsId = m_tree->AppendItem(rootId, "Events");
			//wxTreeItemId scriptsId = m_tree->AppendItem(rootId, "Scripts");
			wxTreeItemId tasksId = m_tree->AppendItem(rootId, "Tasks");

			ProjectTreeData labelFSMData(m_labelFSMMenu, "app", "app", appTemplate, appTemplate);

			wxTreeItemId fsmId = m_tree->AppendItem(rootId, "StateMachines", -1, -1, new ProjectTreeData(labelFSMData));

			//
			//core components
			//
			Array<SmartPtr<ComponentTemplate>> components = appTemplate->getComponents()->getComponentTemplates();
			for (u32 i = 0; i < components.size(); ++i)
			{
				SmartPtr<ComponentTemplate> componentTemplate = components[i];
				String label = componentTemplate->getLabel();

				ProjectTreeData eventData(m_eventMenu, "app", "component", appTemplate, componentTemplate);
				wxTreeItemId componentId = m_tree->AppendItem(componentsId, label.c_str(), -1, -1, new ProjectTreeData(eventData));

				Array<SmartPtr<EventTemplate>> events = componentTemplate->getEventTemplates();
				for (u32 eventIdx = 0; eventIdx < events.size(); ++eventIdx)
				{
					const SmartPtr<EventTemplate>& eventTemplate = events[eventIdx];
					ProjectTreeData eventData("component", "event", componentTemplate, eventTemplate);
					m_tree->AppendItem(componentId, eventTemplate->getLabel().c_str(), -1, -1, new ProjectTreeData(eventData));
				}
			}

			//
			//tasks
			//
			Array<TaskTemplatePtr> taskTemplates = appTemplate->getTasks();
			for (u32 i = 0; i < taskTemplates.size(); ++i)
			{
				TaskTemplatePtr taskTemplate = taskTemplates[i];
				String label = taskTemplate->getLabel();

				ProjectTreeData eventData(m_eventMenu, "app", "task", appTemplate, taskTemplate);
				m_tree->AppendItem(tasksId, label.c_str(), -1, -1, new ProjectTreeData(eventData));
			}

			// 
			// Events
			//
			Array<SmartPtr<EventTemplate>> events = appTemplate->getEventTemplateContainer()->getEventTemplates();
			for (u32 i = 0; i < events.size(); ++i)
			{
				SmartPtr<EventTemplate>& event = events[i];
				String label = event->getLabel();

				ProjectTreeData eventData(m_eventMenu, "app", "event", appTemplate, event);
				m_tree->AppendItem(eventsId, label.c_str(), -1, -1, new ProjectTreeData(eventData));
			}

			//
			//FSMs
			//
			//Array<SmartPtr<FSMTemplate>> fsms = appTemplate->getFSMTemplateContainer()->getFSMTemplates();
			//for(u32 i=0; i<fsms.size(); ++i)
			//{
			//	SmartPtr<FSMTemplate> fsmTemplate = fsms[i];
			//	String fsmName = fsmTemplate->getName();

			//	if(fsmName.length() != 0)
			//	{
			//		ProjectTreeData fsmData(m_fsmMenu, "entity", "fsm", (void*)appTemplate.get(), (void*)fsmTemplate.get());
			//		ProjectTreeData labelEventData(m_fsmLabelEventMenu, "entity", "fsm", (void*)appTemplate.get(), (void*)fsmTemplate.get());
			//		ProjectTreeData labelStatesData(m_fsmLabelStateMenu, "entity", "fsm", (void*)appTemplate.get(), (void*)fsmTemplate.get());

			//		wxTreeItemId currentFSMId = m_tree->AppendItem(fsmId, fsmName.c_str(), -1, -1, new ProjectTreeData(fsmData));
			//		wxTreeItemId eventsId = m_tree->AppendItem(currentFSMId, "Events", -1, -1, new ProjectTreeData(labelEventData));
			//		wxTreeItemId statesId = m_tree->AppendItem(currentFSMId, "States", -1, -1, new ProjectTreeData(labelStatesData));

			//		Array<SmartPtr<EventTemplate>> eventTemplates = fsmTemplate->getEventTemplates();
			//		for(u32 eventIdx=0; eventIdx<eventTemplates.size(); ++eventIdx)
			//		{
			//			SmartPtr<EventTemplate> eventTemplate = eventTemplates[eventIdx];

			//			ProjectTreeData eventData(m_fsmEventMenu, "fsm", "event", (void*)fsmTemplate.get(), (void*)eventTemplate.get());

			//			String eventLabel = eventTemplate->getLabel();
			//			m_tree->AppendItem(eventsId, eventLabel.c_str(), -1, -1, new ProjectTreeData(eventData));
			//		}

			//		Array<String> states = fsmTemplate->getStates();
			//		for(u32 stateIdx=0; stateIdx<states.size(); ++stateIdx)
			//		{
			//			ProjectTreeData stateData(m_fsmStateMenu, "fsm", "fsm", (void*)fsmTemplate.get(), (void*)fsmTemplate.get());
			//			String stateName = states[stateIdx];
			//			m_tree->AppendItem(statesId, stateName.c_str(), -1, -1, new ProjectTreeData(stateData));
			//		}
			//	}
			//}
		}



		//-------------------------------------------------
		void ProjectWindow::handleTreeSelectionChanged(wxTreeEvent& event)
		{
			try
			{
				//if(m_tree->HasFocus())
				{
					wxTreeItemId selectedId = event.GetItem();
					if (!selectedId)
					{
						return;
					}

					if (selectedId.IsOk())
					{
						m_lastSelectedItem = selectedId;
					}

					auto editorManager = EditorManager::getSingletonPtr();
					auto ui = editorManager->getUI();

					auto project = editorManager->getProject();
					auto data = (ProjectTreeData*)m_tree->GetItemData(selectedId);
					if (data)
					{
						if (data->getOwnerType() == ("entity"))
						{
							SmartPtr<EntityTemplate> entityTemplate = data->getOwnerData();
							editorManager->getProject()->setSelectedEntityTemplate(entityTemplate);
						}
						else if (data->getOwnerType() == "folder")
						{
							auto path = data->getObjectType();
							project->setSelectedProjectPath(path);

							auto fileWindow = ui->getFileWindow();
							if (fileWindow)
							{
								fileWindow->buildTree();
							}
						}
						else if (data->getObjectType() == ("mesh"))
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

						const String& editableType = m_selectedObject->getEditableType();
						if (editableType == ("EntityTemplate"))
						{
							m_selectedEntity = m_selectedObject;
						}

						msg->setSelectedObject(m_selectedObject);
					}
					else
					{
						m_selectedObject = nullptr;
					}

					editorManager->getMessageManager()->postMessage(msg);
				}
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
		void ProjectWindow::OnContextMenu(wxTreeEvent& event)
		{
			wxTreeItemId itemId = event.GetItem();

			// select the item so the context menu functions works fine
			if (itemId.IsOk())
				m_tree->SelectItem(itemId);

			ProjectTreeData* data = (ProjectTreeData*)m_tree->GetItemData(itemId);
			if (data != NULL)
			{
				wxMenu* menu = data->getContextMenu();
				if (menu)
					m_tree->PopupMenu(menu);
				else
					m_tree->PopupMenu(m_defaultMenu);
			}
			else
			{
				m_tree->PopupMenu(m_defaultMenu);
			}

			event.Skip();
		}




		//-------------------------------------------------
		void ProjectWindow::OnActivateItem(wxTreeEvent& event)
		{
			if (m_tree->HasFocus())
			{
				wxTreeItemId selectedId = m_tree->GetSelection();
				if (!selectedId)
					return;

				wxTreeItemId parentId = m_tree->GetItemParent(selectedId);
				if (!parentId)
					return;

				wxString parentType = m_tree->GetItemText(parentId);
				wxString componentName = m_tree->GetItemText(selectedId);

				if (parentType.length() == 0 || componentName.length() == 0)
					return;

				ProjectTreeData* projectTreeData = (ProjectTreeData*)m_tree->GetItemData(selectedId);
				if (projectTreeData)
				{
					auto appRoot = IApplicationManager::instance();
					EntityTreeItemActivatedPtr msg(new EntityTreeItemActivated);

					String componentType;

					String ext = Path::getFileExtension(componentName.mb_str().data());
					if (ext == (".lua"))
					{
						componentType = "Script";
					}
					else if (ext == (".gui"))
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
		void ProjectWindow::saveTreeState()
		{


			// clear map
			treeState.clear();

			// reset selected item
			m_newSelectedItem = NULL;

			wxTreeItemId itemId = m_tree->GetRootItem();
			String parent = "";
			if (itemId)
				saveItemState(parent, itemId);
		}



		//-------------------------------------------------
		void ProjectWindow::saveItemState(String parent, wxTreeItemId itemId)
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
		void ProjectWindow::restoreTreeState()
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
		void ProjectWindow::restoreItemState(String parent, wxTreeItemId itemId, bool parentWasNew)
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
				m_tree->Expand(itemId);

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
		int ProjectWindow::getItemState(String itemName)
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



		ProjectWindow::ProjectWindowMessageListener::ProjectWindowMessageListener(ProjectWindow* projectWindow)
			: m_projectWindow(projectWindow)
		{

		}

		void ProjectWindow::ProjectWindowMessageListener::handleMessage(SmartPtr<IMessage> message)
		{
			m_projectWindow->handleMessage(message);
		}

	} // end namespace editor
} // end namespace fb


