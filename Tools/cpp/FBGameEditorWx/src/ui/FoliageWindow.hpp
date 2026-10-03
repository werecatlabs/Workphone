#ifndef _FoliageWindow_H
#define _FoliageWindow_H



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <FBCore/Base/Properties.hpp>
#include <wx/wx.hpp>
#include <wx/treectrl.hpp>
#include <wx/propgrid/propgrid.hpp>
//#include "terrain\IFoliageManagerListener.hpp"




namespace fb
{
	namespace editor
	{
	

	
		//--------------------------------------------
		class FoliageWindow : public ui::wxApplicationWindow
		{
		public:
			enum 
			{
				ID_FoliageFrameFoliageList = wxID_HIGHEST+1,
				ID_FoliageFrameAdd,
				ID_FoliageFramePaint,
				ID_FoliageFrameErase,
				ID_FoliageFrameCreateLayer,
				ID_FoliageFrameDestroyLayer,
				ID_FoliageFrameGenerate,
				ID_FoliageFrameAutoGenerateChkBox,
				ID_FoliageFrameToolProperties,
				ID_FoliageFrameLayerProperties,
				ID_FoliageFrameAddFoliage,
			};

			FoliageWindow( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
				const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS, 
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "listCtrl" );
			~FoliageWindow();
	
			static FoliageWindow* GetInstance( wxWindow *parent_window );
	
			void setAddFoliageToggled( bool status );
			bool GetAddFoliageToggled();
	
			int GetFoliageIdx();
		
		private:
			/*class FoliageWindowListener : public IFoliageManagerListener
			{
			public:
				FoliageWindowListener(FoliageWindow* foliageWindow);
				~FoliageWindowListener();

				void OnCreateLayer();
				void OnDestroyLayer();
				void OnSelectLayer();

				void OnSelectTool(FoliageTool* tool);
				void OnDeselectTool(FoliageTool* tool);

			protected:
				FoliageWindow* m_foliageWindow;
			};*/

			//events 
			void OnAddBtnToggled( wxCommandEvent& event );
			void OnPaintBtnToggled( wxCommandEvent& event );
			void OnEraseBtnToggled( wxCommandEvent& event );
			void OnCreateLayerBtn( wxCommandEvent& event );
			void OnDestroyLayerBtn( wxCommandEvent& event );
			void OnGenerateBtn( wxCommandEvent& event );
			void OnCheckBox( wxCommandEvent& event );
			void OnPropertyGridChangeTool(wxPropertyGridEvent& event); 
			void OnPropertyGridChangeVegProps(wxPropertyGridEvent& event);
			void OnAddFoliageBtn( wxCommandEvent& event );
	
			void OnCreateLayer();
			void OnDestroyLayer();
			void OnSelectLayer();	
			//void OnSelectTool(FoliageToolPtr tool);

			void populateToolProperties();
			void populateFoliageLayers();
			void populateLayerProperties();
	
			void clearToolProperties();
			void clearFoliageLayers();
			void clearLayerProperties();
	
			class wxFrameManager* m_pMgr;

			//FoliageWindowListener* m_listener;
		
			wxToggleButton* m_addBtn;
			wxToggleButton* m_paintBtn;
			wxToggleButton* m_eraseBtn;

			wxNotebook* m_notebook;
		
			wxButton* m_newLayerBtn;
			wxButton* m_removeLayerBtn;
			wxButton* m_generateBtn;
			wxButton* m_addFoliage;
	
			wxCheckBox* m_autoGenChkBox;
	
			wxPropertyGrid* m_toolPropertyGrid; 
			wxPropertyGrid* m_layerPropertyGrid; 
	
			wxListCtrl* m_foliageMeshes;
			wxListCtrl* m_vegetationLayers;
	
			bool m_bIsPortalAddToggled;			//value to know if a button is toggled	
		
			static FoliageWindow* m_pFoliageFrame;	
		};
	


	} // end namespace editor
} // end namespace fb



#endif


