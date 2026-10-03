#ifndef HoudiniWindow_h__
#define HoudiniWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <wx/defs.hpp>
#include <wx/treebase.hpp>
#include <wx/propgrid/propgrid.hpp>



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		class HoudiniWindow : public ui::wxApplicationWindow
		{
		public:
			enum
			{
				ID_RoadFrameCreateRoad = wxID_HIGHEST,
				ID_RoadFrameAddRoadNode,
				ID_RoadFrameRoadList,
				ID_RoadFrameProperties,
			};

			HoudiniWindow( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
				const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS, 
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "listCtrl" );
			~HoudiniWindow();

			void setCreateRoadBtnToggled( bool status );
			bool isCreateRoadBtnToggled();

			void setLastRoadNodeCreated(SmartPtr<IActor> riverNode);
			SmartPtr<IActor> getLastRoadNodeCreated() const;

		private:
			//events 
			void OnButtonToggled( wxCommandEvent& event );

			void OnCreateRoad( wxCommandEvent& event );
			void OnAddRoadNodeToggled( wxCommandEvent& event );
			void OnPropertyGridChange(wxPropertyGridEvent& event); 

			void OnCreateRoad();
			void OnDestroyRoad();
			void OnSelectRoad();

			void populateFoliageLayers();
			void populateLayerProperties();

			//SmartPtr<RoadManagerListener> m_roadManagerListener;

			class wxListCtrl*			m_riversList;
			wxPropertyGrid* m_roadPropertyGrid;

			wxButton*			m_createRoadBtn;
			wxToggleButton*		m_addRoadNodeBtn;

			SmartPtr<IActor>			m_lastCreatedEntity;

			bool m_isCreateRoadBtnToggled;	
		};



	} // end namespace editor	
} // end namespace fb



#endif // HoudiniWindow_h__


