#ifndef __SceneWindow_h__
#define __SceneWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include "core/IMessageListener.hpp"
#include <FBCore/Interface/System/IStateListener.hpp>
#include <wx/treectrl.hpp>
#include <wx/dnd.hpp>



namespace fb
{
	namespace editor
	{



		//-------------------------------------------------
		class SceneWindow : public ui::wxApplicationWindow
		{
		public:
			enum
			{
				MENU_ADD_SCRIPT_ID = wxID_HIGHEST,
				PW_ADD_NEW_ENTITY,
				PW_ADD_NEW_TERRAIN,
				PW_ADD_CAMERA,
				PW_ADD_CAR,
				PW_ADD_PLANE,
				PW_ADD_CUBE,
				PW_ADD_PHYSICS_CUBE,
				PW_ADD_DIRECTIONAL_LIGHT,
				PW_ADD_POINT_LIGHT,

				PW_ADD_BUTTON,
				PW_ADD_CANVAS,
				PW_ADD_PANEL,
				PW_ADD_TEXT,

				PW_ADD_EXISTING_ENTITY,
				APPLICATION_MENU_REMOVE_EVENT,
				SCENE_REMOVE_ACTOR,
				ENTITY_TREE_ID,
				WINDOW_CLICKED,
			};

			SceneWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize, long style = wxVSCROLL | wxHSCROLL,
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "SceneWindow");
			~SceneWindow();

			void handleMessage(SmartPtr<IMessage> message);

			void buildTree();

			void addActorToTree(SmartPtr<IActor> actor, wxTreeItemId rootId);

			void saveTreeState();
			void restoreTreeState();

			SmartPtr<IEditableObject> getSelectedObject() const;
			void setSelectedObject(SmartPtr<IEditableObject> val);

		protected:
			class DnDText : public wxTextDropTarget
			{
			public:
				DnDText(SceneWindow* pOwner);

				bool OnDropText(wxCoord x, wxCoord y, const wxString& text) wxOVERRIDE;

			private:
				SceneWindow* m_pOwner = nullptr;
			};

			void handleWindowClicked(wxTreeEvent& event);
			void handleTreeSelectionChanged(wxTreeEvent& event);
			void handleTreeDragStart(wxTreeEvent& event);

			void handleContextMenu(wxMouseEvent& event);

			void OnActivateItem(wxTreeEvent& event);

			void addNewActor(wxCommandEvent& event);
			void removeActor(wxCommandEvent& event);

			void addNewTerrain(wxCommandEvent& event);
			void addDirectionalLight(wxCommandEvent& event);
			void addPointLight(wxCommandEvent& event);
			void addCube(wxCommandEvent& event);
			void addPhysicsCube(wxCommandEvent& event);
			void addPlane(wxCommandEvent& event);
			void addCamera(wxCommandEvent& event);
			void addCar(wxCommandEvent& event);

			void addButton(wxCommandEvent& event);
			void addCanvas(wxCommandEvent& event);
			void addPanel(wxCommandEvent& event);
			void addText(wxCommandEvent& event);

			void beginDrag(wxTreeEvent& event);
			void endDrag(wxTreeEvent& event);

			int getItemState(String itemName);
			void saveItemState(String parent, wxTreeItemId itemId);
			void restoreItemState(String parent, wxTreeItemId itemId, bool parentWasNew);

			wxTreeCtrl* m_tree = nullptr;

			wxMenu* m_applicationMenu = nullptr;
			wxMenu* m_applicationAddMenu = nullptr;

			SmartPtr<IEditableObject> m_selectedObject;

			SmartPtr<IEditableObject> m_selectedEntity;

			SmartPtr<TemplateFilter> m_parentFilter;

			wxTreeItemId m_scriptsId;
			wxTreeItemId m_graphicsMeshId;
			wxTreeItemId m_particleId;

			wxTreeItemId m_draggedItem;
			wxTreeItemId m_lastSelectedItem;
			wxTreeItemId m_newSelectedItem; // use when restore tree state

			wxTreeItemId m_vehicleId;

			std::map<String, bool> treeState;

		};



	} // end namespace editor	
} // end namespace fb



#endif // EntityWindow_h__


