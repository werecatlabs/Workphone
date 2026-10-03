#ifndef MeshImportWindow_h__
#define MeshImportWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <FBCore/Base/Properties.hpp>
#include <wx/propgrid/propgrid.hpp>
#include <wx/treebase.hpp>



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		class MeshImportWindow : public ui::wxApplicationWindow
		{
		public:
			enum
			{
				ID_RoadFrameCreateRoad = wxID_HIGHEST,
				ID_RoadFrameAddRoadNode,
				ID_RoadFrameRoadList,
				ID_RoadFrameProperties,
			};

			MeshImportWindow( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
				const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS, 
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "listCtrl" );
			~MeshImportWindow();

			void setCreateRoadBtnToggled( bool status );
			bool isCreateRoadBtnToggled();

			void setLastRoadNodeCreated(SmartPtr<IActor> riverNode);
			SmartPtr<IActor> getLastRoadNodeCreated() const;

		private:
			void OnCreateRoad( wxCommandEvent& event );
			void OnAddRoadNodeToggled( wxCommandEvent& event );
			void OnPropertyGridChange(wxPropertyGridEvent& event); 

			void OnCreateRoad();
			void OnDestroyRoad();
			void OnSelectRoad();

			void populateFoliageLayers();
			void populateLayerProperties();

			//SmartPtr<RoadManagerListener> m_roadManagerListener;

			SmartPtr<ui::wxLabelCheckboxPair> m_meshScale;

			wxListCtrl*			m_riversList;
			wxPropertyGrid* m_roadPropertyGrid;

			wxButton*			m_createRoadBtn;
			wxToggleButton*		m_addRoadNodeBtn;

			SmartPtr<IActor>			m_lastCreatedEntity;

			bool m_isCreateRoadBtnToggled;		
		};



	} // end namespace editor	
} // end namespace fb



#endif // MeshImportWindow_h__


