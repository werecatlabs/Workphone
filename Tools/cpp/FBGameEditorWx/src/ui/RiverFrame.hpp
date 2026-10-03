#ifndef _RiverFrame_H
#define _RiverFrame_H




#include <GameEditorPrerequisites.hpp>
#include <FBCore/Base/Properties.hpp>
#include <wx/wx.hpp>
#include <wx/propgrid/propgrid.hpp>
#include <wx//treectrl.hpp>


//forward decs




namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		class RiverFrame : public wxScrolledWindow
		{
		public:
			enum
			{
				ID_RiverFrameCreateRiver = wxID_HIGHEST,
				ID_RiverFrameAddRiverNode,
				ID_RiverFrameRiverList,
				ID_RiverFrameRiverProperties,
			};

			RiverFrame(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
					   const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS,
					   const wxValidator& validator = wxDefaultValidator, const wxString& name = "listCtrl");
			~RiverFrame();

			void setCreateRiverBtnToggled(bool status);
			bool isCreateRiverBtnToggled();

			void setCurrentRiver(SmartPtr<IActor> riverNode);
			SmartPtr<IActor> getCurrentRiver() const;

		private:
			//events 
			void OnPlayAnimBtnClick(wxCommandEvent& event);

			void OnCreateRiver(wxCommandEvent& event);
			void OnAddRiverNodeToggled(wxCommandEvent& event);

			void OnButtonToggled(wxCommandEvent& event);
			void OnSwitchCamera(wxCommandEvent& event);
			void OnNextCameraNode(wxCommandEvent& event);
			void OnPrevCameraNode(wxCommandEvent& event);
			void OnRiverPropertyGridChange(wxPropertyGridEvent& event);

			void OnCreateRiver();
			void OnDestroyRiver();
			void OnSelectRiver();

			void populateFoliageLayers();
			void populateLayerProperties();

			wxButton*			m_createRiverBtn;
			wxToggleButton*		m_addRiverNodeBtn;
			wxToggleButton*		m_insertRiverNodeBtn;

			wxSizer*			m_pGridBagSizer;

			class wxListCtrl*	m_riversList;
			wxPropertyGrid*		m_riverPropertyGrid;

			SmartPtr<IActor>			m_lastCreatedEntity;

			bool m_isCreateRiverBtnToggled;

			static RiverFrame* m_pFoliageFrame;

			//DECLARE_EVENT_TABLE();
		};



	} // end namespace editor	
} // end namespace fb



#endif


