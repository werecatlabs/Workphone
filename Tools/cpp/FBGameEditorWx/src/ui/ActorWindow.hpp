#ifndef __ActorWindow_h__
#define __ActorWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <FBCore/Math/Transform3.hpp>
#include <FBCore/Interface/System/IStateListener.hpp>
#include "core/IMessageListener.hpp"
#include <wx/event.hpp>
#include <wx/scrolwin.hpp>
#include <wx/treebase.hpp>
#include <wx/textctrl.hpp>



namespace fb
{
	namespace editor
	{



		//-------------------------------------------------
		class ActorWindow : public ui::wxApplicationWindow
		{
		public:
			enum class ObjectType
			{
				ACTOR,
				RESOURCE,
				MESH,
				MATERIAL,
				MATERIAL_NODE,

				COUNT
			};

			enum
			{
				MENU_ADD_SCRIPT_ID = wxID_HIGHEST,
				ENTITY_TREE_ID,

				ADD_COMPONENT,
				REMOVE_COMPONENT,
			};

			ActorWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize, long style = wxNO_BORDER,
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "ActorWindow");
			~ActorWindow();

			void unload(SmartPtr<ISharedObject> data);

			void update(time_interval t, time_interval dt);

			void handleMessage(SmartPtr<IMessage> message);

			void buildTree();

			void addTransformToTree(SmartPtr<ITransform> component, String label, wxTreeItemId rootId);
			void addComponentToTree(SmartPtr<component::IComponent> actor, wxTreeItemId rootId);
			void addObjectToTree(SmartPtr<ISharedObject> object, wxTreeItemId rootId);

			void addMaterialToTree(SmartPtr<render::IMaterial> material, wxTreeItemId rootId);
			void addActorToTree(SmartPtr<IActor> actor, wxTreeItemId rootId);
			void addResourceToTree(SmartPtr<IResource> resource, wxTreeItemId rootId);

			void saveTreeState();
			void restoreTreeState();

			void updateSelection();

		protected:
			struct ScriptTreeItemData
			{
				wxTreeItemId m_parentId;
				wxTreeItemId m_itemId;
			};

			class TerrainStateListener : public CSharedObject<IStateListener>
			{
			public:
				TerrainStateListener(ActorWindow* projectWindow);
				~TerrainStateListener();

				void handleStateChanged(const SmartPtr<IStateMessage>& message);
				void handleStateChanged(SmartPtr<IState>& state);

				void handleQuery(SmartPtr<IStateQuery>& query);

			protected:
				ActorWindow* m_projectWindow = nullptr;
			};

			class ProjectWindowMessageListener : public CSharedObject<IMessageListener>
			{
			public:
				ProjectWindowMessageListener(ActorWindow* projectWindow);

				void handleMessage(SmartPtr<IMessage> message);

				ActorWindow* m_projectWindow = nullptr;
			};

			void handleTreeSelectionChanged(wxTreeEvent& event);

			void handleContextMenu(wxMouseEvent& event);

			int getItemState(String itemName);
			void saveItemState(String parent, wxTreeItemId itemId);
			void restoreItemState(String parent, wxTreeItemId itemId, bool parentWasNew);

			void staticCheckboxState(wxCommandEvent& event);

			void visibleCheckboxState(wxCommandEvent& event);

			void addComponent(wxCommandEvent& event);

			void removeComponent(wxCommandEvent& event);


			void OnSize(wxSizeEvent& event);

			void OnText(wxCommandEvent& event);

			void setActorName(const String& textStr);

			void OnTextEnter(wxCommandEvent& event);
			void OnTextURL(wxTextUrlEvent& event);
			void OnTextMaxLen(wxCommandEvent& event);

			void OnTextCut(wxClipboardTextEvent& event);
			void OnTextCopy(wxClipboardTextEvent& event);
			void OnTextPaste(wxClipboardTextEvent& event);

			wxCheckBox* m_enabledCheckbox = nullptr;
			wxCheckBox* m_staticCheckbox = nullptr;
			wxCheckBox* m_visibleCheckbox = nullptr;

			ProjectWindowMessageListener* m_messageListener = nullptr;

			wxBoxSizer* m_baseSizer = nullptr;
			wxTreeCtrl* m_tree = nullptr;

			wxMenu* m_defaultMenu = nullptr;

			SmartPtr<ui::wxLabelTextInputPair> m_actorNamePair;

			SmartPtr<ISharedObject> m_selectedObject;

			SmartPtr<IEditableObject> m_selectedEntity;

			SmartPtr<TemplateFilter> m_parentFilter;

			wxTreeItemId m_scriptsId;
			wxTreeItemId m_graphicsMeshId;
			wxTreeItemId m_particleId;

			wxTreeItemId m_lastSelectedItem;
			wxTreeItemId m_newSelectedItem; // use when restore tree state

			wxTreeItemId m_vehicleId;

			ActorWindow::ObjectType m_objectType;

			std::map<String, bool> treeState;

			DECLARE_EVENT_TABLE()
		};



	} // end namespace editor
} // end namespace fb



#endif // EntityWindow_h__


