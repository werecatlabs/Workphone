#ifndef EntityWindow_h__
#define EntityWindow_h__

#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <core/IMessageListener.hpp>
#include <FBCore/Interface/System/IStateListener.hpp>
#include <wx/treebase.hpp>

namespace fb
{
	namespace editor
	{

		//-------------------------------------------------
		class ProjectWindow : public ui::wxApplicationWindow
		{
		public:
			enum
			{
				MENU_ADD_SCRIPT_ID = wxID_HIGHEST,
				MENU_ADD_BUILDER_ID,
				MENU_ADD_COMPONENT_ID,
				MENU_ADD_FSM_ID,
				ENTITY_TREE_ID,

				MENU_ADD_CAMERA,
				MENU_REMOVE_CAMERA,

				APPLICATION_MENU_ADD_EVENT,

				PW_ADD_NEW_ENTITY,
				PW_ADD_EXISTING_ENTITY,

				PW_ADD_ENT_SCENE_NODE,
				PW_ADD_ENT_GFX_OBJ,
				PW_REMOVE_ENT_GFX_OBJ,
				PW_REMOVE_ENT_SCENE_NODE,

				PW_ADD_ENT_SHAPE,
				PW_ADD_ENT_BODY,
				PW_REMOVE_ENT_SHAPE,
				PW_REMOVE_ENT_BODY,

				PW_ADD_ENT_SOUND,
				PW_REMOVE_ENT_SOUND,

				PW_ENTITY_REMOVE,
				PW_GENERATE_SCRIPT,

				PW_ADD_NEW_GUI,
				PW_ADD_EXISTING_GUI,

				PW_COMPONENT_REMOVE,

				PW_FSM_REMOVE,

				PW_EVENT_ADD,
				PW_EVENT_REMOVE,
				PW_EVENT_MODIFY,

				PW_FSM_EVENT_ADD,
				PW_FSM_EVENT_REMOVE,
				PW_FSM_EVENT_MODIFY,

				PW_FSM_STATE_ADD,
				PW_FSM_STATE_REMOVE,

				PW_ENT_SCRIPT_REMOVE,

				PW_ADD_NEW_SCRIPT,
				PW_ADD_EXISTING_SCRIPT,
				PW_ADD_SCRIPT_FOLDER,
				PW_REMOVE_SCRIPT,
				PW_RENAME_SCRIPT,

				PW_ADD_TERRAIN,
				PW_ADD_MESH,
				PW_ADD_DESTRUCTIBLE_MESH,
				PW_ADD_PARTICLE_SYSTEM,

				PW_ADD_SCENE,

				PW_ADD_VEHICLE_4_WHEEL,
				PW_ADD_VEHICLE_AIRPLANE,
				PW_ADD_VEHICLE_HELICOPTER,
				PW_ADD_VEHICLE_TANK,

				PW_ADD_GOAL,
				PW_ADD_EVALUATOR,

				PW_ADD_MAP,
				PW_MAP_REFRESH,

				PW_ADD_MATERIAL,
				PW_ADD_TECHNIQUE,
			};

			ProjectWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
					const wxSize& size = wxDefaultSize, long style = wxVSCROLL | wxHSCROLL,
					const wxValidator& validator = wxDefaultValidator, const wxString& name = "ProjectWindow");

			~ProjectWindow();

			void update(time_interval t, time_interval dt);

			void handleMessage(SmartPtr<IMessage> message);

			void addFolderToTree(wxTreeItemId parent, SmartPtr<IFolderListing> listing);

			void buildTree();

			void saveTreeState();

			void restoreTreeState();

		protected:
			class ProjectWindowMessageListener : public CSharedObject<IMessageListener>
			{
			public:
				ProjectWindowMessageListener(ProjectWindow* projectWindow);

				void handleMessage(SmartPtr<IMessage> message);

				ProjectWindow* m_projectWindow;
			};

			void handleTreeSelectionChanged(wxTreeEvent& event);

			void OnContextMenu(wxTreeEvent& event);

			void OnActivateItem(wxTreeEvent& event);

			void populateCameras(wxTreeItemId rootId);

			void populateApplication(wxTreeItemId rootId);

			void populateProjectResources(wxTreeItemId resourcesId, SmartPtr<ResourceSetTemplate> resourcesetTemplate,
					ProjectPtr project);

			int getItemState(String itemName);

			void saveItemState(String parent, wxTreeItemId itemId);

			void restoreItemState(String parent, wxTreeItemId itemId, bool parentWasNew);

			ProjectWindowMessageListener* m_messageListener;

			wxTreeCtrl* m_tree;

			wxMenu* m_defaultMenu;

			wxMenu* m_applicationMenu;
			wxMenu* m_applicationAddMenu;

			wxMenu* m_entitySubMenu;
			wxMenu* m_entitiesMenu;
			wxMenu* m_entitiesAddMenu;
			wxMenu* m_entityMenu;

			wxMenu* m_entityResLabelMenu;

			wxMenu* m_entityGfxLabelMenu;
			wxMenu* m_entityGfxObjLabelMenu;
			wxMenu* m_entityGfxObjMenu;
			wxMenu* m_entityNodeLabelMenu;
			wxMenu* m_entityNodeMenu;

			wxMenu* m_entityPhysicsLabelMenu;
			wxMenu* m_entityShapeLabelMenu;
			wxMenu* m_entityShapeMenu;
			wxMenu* m_entityBodyLabelMenu;
			wxMenu* m_entityBodyMenu;

			wxMenu* m_entitySoundLabelMenu;
			wxMenu* m_entitySoundMenu;

			wxMenu* m_entityScriptLabelMenu;
			wxMenu* m_entityScriptMenu;

			wxMenu* m_guiMenu;
			wxMenu* m_guiAddMenu;

			wxMenu* m_scenesMenu;
			wxMenu* m_scenesAddMenu;

			wxMenu* m_sceneMenu;
			wxMenu* m_sceneAddMenu;

			wxMenu* m_componentMenu;
			wxMenu* m_labelComponentMenu;

			wxMenu* m_fsmMenu;
			wxMenu* m_labelFSMMenu;

			wxMenu* m_eventMenu;
			wxMenu* m_labelEventMenu;

			wxMenu* m_fsmEventMenu;
			wxMenu* m_fsmLabelEventMenu;

			wxMenu* m_fsmStateMenu;
			wxMenu* m_fsmLabelStateMenu;

			wxMenu* m_scriptsMenu;
			wxMenu* m_scriptsAddMenu;
			wxMenu* m_scriptMenu;

			wxMenu* m_resourceMenu;
			wxMenu* m_resourceGfxMenu;
			wxMenu* m_resourcePhysicsMenu;
			wxMenu* m_resourceSoundMenu;

			wxMenu* m_vehiclesMenu;
			wxMenu* m_vehicleMenu;
			wxMenu* m_vehicleSubMenu;

			wxMenu* m_dotSceneMenu;

			wxMenu* m_aiMenu;

			wxMenu* m_mapsMenu;

			wxMenu* m_camerasMenu;

			wxMenu* m_materialsMenu;
			wxMenu* m_materialMenu;
			wxMenu* m_techniqueMenu;
			wxMenu* m_passMenu;

			SmartPtr<IEditableObject> m_selectedObject;

			SmartPtr<IEditableObject> m_selectedEntity;

			SmartPtr<TemplateFilter> m_parentFilter;

			wxTreeItemId m_scriptsId;
			wxTreeItemId m_graphicsMeshId;
			wxTreeItemId m_particleId;

			wxTreeItemId m_lastSelectedItem;
			wxTreeItemId m_newSelectedItem; // use when restore tree state

			wxTreeItemId m_vehicleId;

			std::map<String, bool> treeState;

			DECLARE_EVENT_TABLE()
		};


	} // end namespace editor
} // end namespace fb

#endif // EntityWindow_h__


