#ifndef _TerrainFrame_H
#define _TerrainFrame_H



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <FBCore/Base/StringTypes.hpp>
#include <wx/defs.hpp>
#include <wx/treebase.hpp>
#include <wx/propgrid/propgrid.hpp>



namespace fb
{	
	namespace editor
	{



		//--------------------------------------------
		class TerrainWindow : public ui::wxApplicationWindow
		{
		public:
			enum 
			{
				ID_TerrainCreateTerrainBtn = wxID_HIGHEST+1,
				ID_TerrainRaiseBtn,
				ID_TerrainLowerBtn,
				ID_TerrainMinimumBtn,
				ID_TerrainMaximumBtn,
				ID_TerrainsetHeightBtn, 
				ID_TerrainPaintBtn,
				ID_TerrainEraseBtn,
				ID_TerrainBlendBtn,
				ID_TerrainFrameToolProperties,
				ID_TerrainBrushImage,
				ID_TerrainSplatImage,
				ID_AddTerrainSplatImage,
			};

			TerrainWindow( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
				const wxSize& size = wxDefaultSize, long style = wxTR_HAS_BUTTONS, 
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "listCtrl" );
			~TerrainWindow();

		private:
			//events 
			void OnButtonToggled( wxCommandEvent& event );

			void OnCreateTerrainBtn( wxCommandEvent& event );

			void OnRaiseBtnToggled( wxCommandEvent& event );
			void OnLowerBtnToggled( wxCommandEvent& event );
			void OnMinimumBtnToggled( wxCommandEvent& event );
			void OnMaximumBtnToggled( wxCommandEvent& event );
			void OnsetHeightBtnToggled( wxCommandEvent& event );

			void OnPaintBtnToggled( wxCommandEvent& event );
			void OnEraseBtnToggled( wxCommandEvent& event );
			void OnBlendBtnToggled( wxCommandEvent& event );

			void OnAddTextureBtn( wxCommandEvent& event );

			void OnComboBox(wxCommandEvent& event);

			void OnPropertyGridChangeTool(wxPropertyGridEvent& event); 

			//void OnSelectTool(TerrainToolPtr tool);

			void populateToolProperties();

			void loadImages(const String& folderName, Array<String>& images, wxBitmapComboBox* comboBox);

			void addComboBoxBitmap(const String& folderName, wxBitmapComboBox* comboBox);

			wxBitmap* createTexture(const String& texturePath);

			wxWindow *m_pWindow = nullptr;
			wxFrameManager* m_pMgr = nullptr;

			ITerrainManagerListener* m_terrainManagerListener = nullptr;

			wxPropertyGrid* m_toolPropertyGrid = nullptr;

			wxButton*		m_createTerrainBtn = nullptr;
			wxToggleButton* m_raiseBtn = nullptr;
			wxToggleButton* m_lowerBtn = nullptr;
			wxToggleButton* m_minimumBtn = nullptr;
			wxToggleButton* m_maximumBtn = nullptr;
			wxToggleButton* m_setHeightBtn = nullptr;
			wxToggleButton* m_paintBtn = nullptr; 
			wxToggleButton* m_eraseBtn = nullptr; 
			wxToggleButton* m_blendBtn = nullptr; 
			wxButton*		m_addTextureBtn = nullptr;
			wxToggleButton* m_pButton = nullptr;

			wxBitmapComboBox *m_brushesCombobox = nullptr;
			wxBitmapComboBox *m_splatMapsCombobox = nullptr;

			bool m_bIsPortalAddToggled = false;			//value to know if a button is toggled	

			Array<String> m_brushes;
			Array<String> m_splatMaps;

			DECLARE_EVENT_TABLE();			
		};



	} // end namespace editor	
} // end namespace fb



#endif


