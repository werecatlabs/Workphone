#ifndef __ApplicationFrame_H
#define __ApplicationFrame_H



#include <GameEditorPrerequisites.hpp>
#include <core/ApplicationData.hpp>
#include <core/IMessageListener.hpp>
#include <FBCore/Base/Map.hpp>
#include <FBCore/Base/Singleton.hpp>
#include <FBCore/Memory/CSharedObject.hpp>
#include <wx/frame.hpp>
#include <wx/aui/auibar.hpp>
#include <wx/artprov.hpp>



namespace fb
{	
	namespace editor
	{
		
	
	
		//--------------------------------------------
		class ApplicationFrame : public wxFrame, public Singleton<ApplicationFrame>
		{
		public:
			// IDs for the controls and the menu commands
			enum
			{
				Minimal_Quit = wxID_EXIT,
				Minimal_About = wxID_ABOUT,
	
				BatchAllBtnId = wxID_HIGHEST+1,
				AppPropertiesId,
	
				NewProjectId,
				OpenProjectId,
				SaveProjectId,

				NewSceneId,
				OpenSceneId,
				SaveId,
				SaveAllId,
				GenerateCMakeProjectId,
				CreatePackageId,
				ProjectSettingsId,

				LoadProceduralSceneId,
				SaveProceduralSceneId,
	
				LuaEditConfigDialogId, 
	
				UndoId,
				RedoId,
				GotoId,

				ImportJsonSceneId,

				ShowAllOverlaysId,
				HideAllOverlaysId,
				CreateOverlayTestId,
				CreateOverlayTextTestId,
				CreateOverlayButtonTestId,

				CreateRigidStaticMeshId,
				CreateRigidDynamicMeshId,
				CreateConstraintId,

				CreateProceduralTestId,

				CreateDefaultCarId,
				CreateDefaultTruckId,

				ConvertCSharpId,

				PhysicsEnableId,

				ID_CustomizeToolbar,
	
				RunId,

				AssetImportId,
				AssetReimportId,

				ID_SampleItem,
			};
	
			ApplicationFrame(const wxString& title);
			~ApplicationFrame();

			void update(s32 task, time_interval t, time_interval dt);
	
			void handleMessage(SmartPtr<IMessage> message);
	
			LuaEdit* openScript( SmartPtr<ScriptTemplate> scriptTemplate );
	
			void removeLuaTextWindow(LuaEdit* luaTextWindow);
	
			void OnOpenProject(wxCommandEvent& event);
			void OnSaveProject(wxCommandEvent& event);

			void createNewScene(wxCommandEvent& event);
			void saveApplication();

			bool saveProject();

			void OnOpen(wxCommandEvent& event);
			void OnSave(wxCommandEvent& event);
			void OnSaveAll(wxCommandEvent& event);
	
			void OnGoto (wxCommandEvent &event);
			void OnEdit (wxCommandEvent &event);
	
			void OnUndo(wxCommandEvent& event);
			void OnRedo(wxCommandEvent& event);
			void OnOpenScene(wxCommandEvent& event);

			void createNewProject(wxCommandEvent& event);
			void OnQuit(wxCommandEvent& event);
			void OnAbout(wxCommandEvent& event);
			//void OnPropertyChange(wxPropertyGridEvent& event);
			void OnRun(wxCommandEvent& event);
	
			void OnLuaEditConfigDialog(wxCommandEvent& event);

			void OnPhysicsEnable(wxCommandEvent& event);

			void generateCMakeProject(wxCommandEvent& event);
			void packageProject(wxCommandEvent& event);
			void projectSettings(wxCommandEvent& event);

			void importAssets(wxCommandEvent& event);
			void reimportAssets(wxCommandEvent& event);

			void loadProceduralScene(wxCommandEvent& event);
			void saveProceduralScene(wxCommandEvent& event);

			void OnClose(wxCloseEvent& event);

			void toolbarButtonClick(wxAuiToolBarEvent& event);

			wxAuiManager* getAui() const;
			void setAui(wxAuiManager* val);
	
		private:
			class MessageListener : public CSharedObject<IMessageListener>
			{

			};

			void DosetSize(int x, int y,
				int width, int height,
				int WXUNUSED(sizeFlags = wxSIZE_AUTO));

			void createFileMenu();
			void createWindows();
	
			void createToolbars();

			void importJsonScene(wxCommandEvent& event);

			void showAllOverlays(wxCommandEvent& event);
			void hideAllOverlays(wxCommandEvent& event);

			void createOverlayPanelTest(wxCommandEvent& event);
			void createOverlayTextTest(wxCommandEvent& event);
			void createOverlayButtonTest(wxCommandEvent& event);

			void createRigidStaticMesh(wxCommandEvent& event);
			void createRigidDynamicMesh(wxCommandEvent& event);
			void createConstraint(wxCommandEvent& event);

			void createProceduralTest(wxCommandEvent& event);

			void createDefaultCar(wxCommandEvent& event);
			void createDefaultTruck(wxCommandEvent& event);

			void createConvertCSharp(wxCommandEvent& event);

			ApplicationData* m_applicationData = nullptr;
	
			wxAuiManager*		m_aui = nullptr;

			wxAuiNotebook*		m_bookProject = nullptr;
			wxAuiNotebook*		m_bookProps = nullptr;
			wxAuiNotebook*		m_bookComponents = nullptr;
			wxAuiNotebook*		m_bookMain = nullptr;
			wxAuiNotebook*		m_bookAnimation = nullptr;
			wxAuiNotebook*		m_bookFiles = nullptr;
	
			OutputWindow*		m_outputWindow = nullptr;
			ActorWindow* m_actorWindow = nullptr;
	
			typedef Map<String, LuaEdit*> LuaEditMap;
			LuaEditMap m_luaEditMap;

			Array<SmartPtr<ui::wxApplicationWindow>> m_windows;
	
			DECLARE_EVENT_TABLE()	
		};
	
	
	
	} // end namespace editor
} // end namespace fb



#endif


