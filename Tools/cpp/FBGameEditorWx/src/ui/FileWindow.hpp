#ifndef __FileWindow_h__
#define __FileWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <FBCore/Interface/System/IStateListener.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <core/IMessageListener.hpp>
#include <wx/treectrl.hpp>
#include <wx/dnd.hpp>


namespace fb
{	
	namespace editor
	{
	
	
	
		//-------------------------------------------------
		class FileWindow : public ui::wxApplicationWindow
		{
		public:
			enum UiIds
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

				ADD_SCENE,

				PW_ADD_VEHICLE_4_WHEEL,
				PW_ADD_VEHICLE_AIRPLANE,
				PW_ADD_VEHICLE_HELICOPTER,
				PW_ADD_VEHICLE_TANK,

				PW_ADD_GOAL,
				PW_ADD_EVALUATOR,

				PW_ADD_MAP,
				PW_MAP_REFRESH,

				ADD_MATERIAL,
				ADD_SCRIPT,
				REMOVE_COMPONENT,

				REFRESH
			};
	
			FileWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
				const wxSize& size = wxDefaultSize, long style = wxVSCROLL | wxHSCROLL, 
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "FileWindow");
			~FileWindow();

			void update(time_interval t, time_interval dt);
	
			void handleMessage(SmartPtr<IMessage> message);
	
			void addFolderToTree(wxTreeItemId parent, SmartPtr<IFolderListing> listing);

			void buildTree();

			void saveTreeState();
			void restoreTreeState();

			SmartPtr<IEditableObject> getSelectedObject() const;
			void setSelectedObject(SmartPtr<IEditableObject> val);
	
			String getPath() const;
			void setPath(const String& val);

		protected:
			class DnDText : public wxTextDropTarget
			{
			public:
				DnDText(FileWindow* pOwner);

				bool OnDropText(wxCoord x, wxCoord y, const wxString& text) override;

			private:
				FileWindow* m_pOwner = nullptr;
			};

			class DnDFile : public wxFileDropTarget
			{
			public:
				DnDFile(FileWindow* pOwner);

				bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override;

			private:
				FileWindow* m_pOwner = nullptr;
			};

			void handleTreeSelectionChanged(wxTreeEvent& event);
			void handleTreeSelectionActivated(wxTreeEvent& event);
			void handleTreeDragStart(wxTreeEvent& event);
	
			void handleContextMenu(wxMouseEvent& event );
	
			int getItemState(String itemName);
			void saveItemState(String parent, wxTreeItemId itemId);
			void restoreItemState(String parent, wxTreeItemId itemId, bool parentWasNew);

			void addScript(wxCommandEvent& event);
			void addMaterial(wxCommandEvent& event);
			void addScene(wxCommandEvent& event);
			void refresh(wxCommandEvent& event);
				
			wxTreeCtrl* m_tree = nullptr;
	
			wxMenu*	m_defaultMenu = nullptr;
			wxMenu* m_defaultAddMenu = nullptr;
	
			SmartPtr<IEditableObject> m_selectedObject;

			SmartPtr<IEditableObject> m_selectedEntity;
	
			SmartPtr<TemplateFilter> m_parentFilter;
	
			wxTreeItemId m_scriptsId;
			wxTreeItemId m_graphicsMeshId;
			wxTreeItemId m_particleId;
	
			wxTreeItemId m_lastSelectedItem;
			wxTreeItemId m_newSelectedItem; // use when restore tree state

			wxTreeItemId m_vehicleId;
	
			String m_path;
			std::map<String, bool> treeState;
	
			DECLARE_EVENT_TABLE()
			
		};
	
	
	
	} // end namespace editor
} // end namespace fb



#endif // EntityWindow_h__


