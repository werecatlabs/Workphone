#include <GameEditorPCH.hpp>
#include <ui/ApplicationFrame.hpp>
#include <ui/wxGUIUtil.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/AnimationControlsPanel.hpp>
#include <ui/ComponentsWindow.hpp>
#include <ui/FoliageWindow.hpp>
#include <ui/FileWindow.hpp>
#include <ui/ProjectWindow.hpp>
#include <ui/SceneWindow.hpp>
#include <ui/PropertiesWindow.hpp>
#include <ui/RenderWindow.hpp>
#include <ui/GotoDialog.hpp>
#include <ui/OutputWindow.hpp>
#include <ui/UIManager.hpp>
#include <ui/TerrainWindow.hpp>
#include <ui/RoadFrame.hpp>
#include <ui/MeshImportWindow.hpp>
#include <ui/ProjectSettingsDialog.hpp>
#include <ui/TextureWindow.hpp>
#include <ui/MaterialWindow.hpp>
#include <ui/ObjectWindow.hpp>
#include <editor/EditorManager.hpp>
#include <editor/EditorMessages.hpp>
#include <messages/MessageScriptError.hpp>
#include <GameEditorTypes.hpp>
#include <FBCore/Base/Path.hpp>
#include <FBApplication/FBApplicationHeaders.hpp>
#include <FBCore/FBCoreHeaders.hpp>
#include <FBObjectTemplates/ApplicationTemplate.hpp>
#include <FBObjectTemplates/ScriptTemplate.hpp>
#include <FBObjectTemplates/TemplateXMLWriter.hpp>
#include <core/MessageManager.hpp>
#include <editor/Project.hpp>
#include <wx/aui/framemanager.hpp>
#include <wx/aui/dockart.hpp>
#include <wx/aui/auibook.hpp>
#include <wx/aui/aui.hpp>

#include "commands/AddActorCmd.hpp"
#include <FBApplication/ApplicationUtil.hpp>


BEGIN_EVENT_TABLE(fb::editor::ApplicationFrame, wxFrame)
EVT_MENU(NewProjectId, fb::editor::ApplicationFrame::createNewProject)
EVT_MENU(OpenProjectId, fb::editor::ApplicationFrame::OnOpenProject)
EVT_MENU(SaveProjectId, fb::editor::ApplicationFrame::OnSaveProject)
EVT_MENU(NewSceneId, fb::editor::ApplicationFrame::createNewScene)
EVT_MENU(OpenSceneId, fb::editor::ApplicationFrame::OnOpen)
EVT_MENU(SaveId, fb::editor::ApplicationFrame::OnSave)
EVT_MENU(SaveAllId, fb::editor::ApplicationFrame::OnSaveAll)
EVT_MENU(UndoId, fb::editor::ApplicationFrame::OnUndo)
EVT_MENU(RedoId, fb::editor::ApplicationFrame::OnRedo)
EVT_MENU(GotoId, fb::editor::ApplicationFrame::OnGoto)
EVT_MENU(Minimal_Quit, fb::editor::ApplicationFrame::OnQuit)
EVT_MENU(Minimal_About, fb::editor::ApplicationFrame::OnAbout)
EVT_MENU(RunId, fb::editor::ApplicationFrame::OnRun)

EVT_MENU(LoadProceduralSceneId, fb::editor::ApplicationFrame::loadProceduralScene)
EVT_MENU(SaveProceduralSceneId, fb::editor::ApplicationFrame::saveProceduralScene)

EVT_MENU(GenerateCMakeProjectId, fb::editor::ApplicationFrame::generateCMakeProject)
EVT_MENU(CreatePackageId, fb::editor::ApplicationFrame::packageProject)
EVT_MENU(ProjectSettingsId, fb::editor::ApplicationFrame::projectSettings)
EVT_MENU(AssetImportId, fb::editor::ApplicationFrame::importAssets)
EVT_MENU(AssetReimportId, fb::editor::ApplicationFrame::reimportAssets)

EVT_MENU(LuaEditConfigDialogId, fb::editor::ApplicationFrame::OnLuaEditConfigDialog)
EVT_MENU(PhysicsEnableId, fb::editor::ApplicationFrame::OnPhysicsEnable)
//EVT_PG_CHANGED(AppPropertiesId,  AppFrame::OnPropertyChange)	
EVT_CLOSE(fb::editor::ApplicationFrame::OnClose)
END_EVENT_TABLE()



template<> fb::editor::ApplicationFrame* fb::Singleton<fb::editor::ApplicationFrame>::m_singleton = nullptr;



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		ApplicationFrame::ApplicationFrame(const wxString& title)
			: wxFrame(nullptr, wxID_ANY, title),
			m_bookProject(nullptr),
			m_bookProps(nullptr),
			m_bookComponents(nullptr),
			m_bookMain(nullptr),
			m_bookAnimation(nullptr)
		{
			// set the frame icon
			//setIcon(wxICON(sample));

			m_aui = new wxAuiManager(this, wxAUI_MGR_ALLOW_ACTIVE_PANE | wxAUI_MGR_DEFAULT);

			createFileMenu();

			// create a status bar just for fun (by default with 1 pane only)
			CreateStatusBar(2);
			SetStatusText("Editor");

			createWindows();
			createToolbars();

			m_aui->Update();

			m_applicationData = new ApplicationData;

			//try
			//{
			//	appData->loadData("./Config.xml");
			//}
			//catch (Exception& e)
			//{
			//	wxMessageBox(e.what().c_str());
			//}

			Properties propertyGroup;
			m_applicationData->getPropertyGroup(propertyGroup);
			//wxGUIUtil::populateProperties(propertyGroup, m_appProperties);

			//ApplicationManager* appRoot = IApplicationManager::instance();
			//appRoot->getMessageManager()->addListener(this);
		}



		//--------------------------------------------
		ApplicationFrame::~ApplicationFrame()
		{
			for (auto window : m_windows)
			{
				window->unload(nullptr);
			}

			m_windows.clear();

			FB_SAFE_DELETE(m_bookProject);
			FB_SAFE_DELETE(m_bookProps);
			FB_SAFE_DELETE(m_bookComponents);
			FB_SAFE_DELETE(m_bookMain);
			FB_SAFE_DELETE(m_bookAnimation);

			m_aui->UnInit();
			FB_SAFE_DELETE(m_aui);

			FB_SAFE_DELETE(m_applicationData);

			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//appRoot->getMessageManager()->removeListener(this);
		}



		//--------------------------------------------
		void ApplicationFrame::handleMessage(SmartPtr<IMessage> message)
		{
			String type = message->getType();
			if (type == ("ScriptError"))
			{
				//MessageScriptErrorPtr msg = message;
				//String sourcePath = msg->getSourcePath();
				//int lineNumber = msg->getLineNumber();

				//String fileName = Path::getFileName(sourcePath);

				//ApplicationManager* appRoot = IApplicationManager::instance();
				//ProjectPtr project = appRoot->getProject();
				//SmartPtr<ScriptTemplate> scriptTemplate = project->getScriptTemplate(fileName);
				//if(scriptTemplate)
				//{
				//	LuaEdit* edit = openScript(scriptTemplate);
				//	if(edit)
				//		edit->GotoLine(lineNumber-1);
				//}
			}
			else if (type == ("EntityTreeItemActivated"))
			{
				//EntityTreeItemActivatedPtr msg = message;
				//String componentType = msg->getComponentType();
				//if( componentType==("Script") || 
				//	componentType==("Scripts") )
				//{
				//	SmartPtr<ScriptTemplate> scriptTemplate = msg->getObject();
				//	if(scriptTemplate)
				//	{
				//		LuaEdit* edit = openScript(scriptTemplate);
				//	}
				//}
			}
		}



		//--------------------------------------------
		void ApplicationFrame::removeLuaTextWindow(LuaEdit* luaTextWindow)
		{
			//String name = luaTextWindow->getFileName();
			//m_luaEditMap.erase(name);
		}



		//--------------------------------------------
		void ApplicationFrame::OnRun(wxCommandEvent& event)
		{
			auto appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();

			String projectFilePath = project->getProjectFilePath();


			auto applicationManager = IApplicationManager::instance();
			auto fileSystem = applicationManager->getFileSystem();

			char* path = "";//  getenv("FIREBIRD_HOME");
			String exePath = Path::getAbsolutePath(path, "../bin/Release/");
			String fullExeFilePath = exePath + String("/") + String("FBGameApp.exe");

			if (!fileSystem->isExistingFile(fullExeFilePath))
			{
				wxFileDialog dialog(this,
					_T("Open"),
					wxEmptyString,
					wxEmptyString,
					_T("FireBlade Exe (*.exe)|*.exe")
				);

				if (dialog.ShowModal() == wxID_OK)
				{
					//String filePath = dialog.GetPath().c_str();
					//debugger->run(filePath);
				}
			}
			else
			{
				//debugger->run(fullExeFilePath);
			}
		}



		//--------------------------------------------
		void ApplicationFrame::OnQuit(wxCommandEvent& event)
		{
			// true is to force the frame to close
			Close(true);
		}



		//--------------------------------------------
		void ApplicationFrame::OnAbout(wxCommandEvent& event)
		{
			wxMessageBox(wxString::Format
			(
				"Welcome to %s!\n"
				"\n"
				"This is the minimal wxWidgets sample\n"
				"running under %s.",
				wxVERSION_STRING,
				wxGetOsDescription()
			),
				"About wxWidgets minimal sample",
				wxOK | wxICON_INFORMATION,
				this);
		}



		//--------------------------------------------
		void ApplicationFrame::OnOpenScene(wxCommandEvent& event)
		{
			wxFileDialog dialog(this,
				_T("Open"),
				wxEmptyString,
				wxEmptyString,
				_T("FireBlade Project (*.fbproject)|*.fbproject")
			);

			dialog.SetFilterIndex(0);

			if (dialog.ShowModal() == wxID_OK)
			{
				auto applicationManager = IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto editorManager = EditorManager::getSingletonPtr();
				FB_ASSERT(editorManager);

				auto project = editorManager->getProject();
				FB_ASSERT(project);

				auto uiManager = editorManager->getUI();
				FB_ASSERT(uiManager);

				auto filePath = String(dialog.GetPath().c_str());

				auto fileSystem = applicationManager->getFileSystem();
				FB_ASSERT(fileSystem);

				auto path = Path::getFilePath(filePath);
				path = StringUtil::cleanupPath(path);

				applicationManager->setProjectPath(path);

				auto projectPath = applicationManager->getProjectPath();
				if (StringUtil::isNullOrEmpty(projectPath))
				{
					projectPath = Path::getWorkingDirectory();
				}

				fileSystem->addFolder(path, true);

				auto cachePath = path + "/Cache/";
				fileSystem->addFolder(cachePath, true);
				applicationManager->setCachePath(cachePath);

				auto settingsCachePath = path + "/SettingsCache/";
				fileSystem->addFolder(settingsCachePath, true);
				applicationManager->setSettingsCachePath(settingsCachePath);

				auto relativeProjectPath = Path::getRelativePath(projectPath, filePath);
				project->load(relativeProjectPath);

				LoadProjectMsgPtr msg(new LoadProjectMsg);
				auto messageManager = editorManager->getMessageManager();
				messageManager->postMessage(msg);

				uiManager->rebuildSceneTree();
			}
		}



		//--------------------------------------------
		void ApplicationFrame::createNewProject(wxCommandEvent& event)
		{
			wxDirDialog dialog(this,
				_("Select a folder"),
				wxEmptyString,
				0);

			if (dialog.ShowModal() == wxID_OK)
			{
				auto applicationManager = IApplicationManager::instance();
				auto editorManager = EditorManager::getSingletonPtr();
				auto project = editorManager->getProject();

				auto path = String(dialog.GetPath().c_str());
				if (!StringUtil::isNullOrEmpty(path))
				{
					auto fileSystem = applicationManager->getFileSystem();

					fileSystem->addFolder(path);

					auto assetsPath = path + "/Assets/";
					if (!fileSystem->isExistingFolder(assetsPath))
					{
						fileSystem->createDirectories(assetsPath);
					}

					auto cachePath = path + "/Cache/";
					if (!fileSystem->isExistingFolder(cachePath))
					{
						fileSystem->createDirectories(cachePath);
					}

					fileSystem->addFolder(cachePath);

					auto projectName = Path::getLeaf(path);
					auto projectFilePath = path + "/" + projectName + ".fbproject";
					project->save(projectFilePath);
				}
			}
		}



		//--------------------------------------------
		void ApplicationFrame::OnOpenProject(wxCommandEvent& event)
		{
			wxFileDialog dialog(this,
				_T("Open"),
				wxEmptyString,
				wxEmptyString,
				_T("FireBlade Project (*.fbproject)|*.fbproject")
			);

			dialog.SetFilterIndex(0);

			if (dialog.ShowModal() == wxID_OK)
			{
				auto applicationManager = IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto editorManager = EditorManager::getSingletonPtr();
				FB_ASSERT(editorManager);

				auto project = editorManager->getProject();
				FB_ASSERT(project);

				auto uiManager = editorManager->getUI();
				FB_ASSERT(uiManager);

				auto filePath = String(dialog.GetPath().c_str());

				auto fileSystem = applicationManager->getFileSystem();
				FB_ASSERT(fileSystem);

				auto path = Path::getFilePath(filePath);
				path = StringUtil::cleanupPath(path);

				applicationManager->setProjectPath(path);

				auto projectPath = applicationManager->getProjectPath();
				if (StringUtil::isNullOrEmpty(projectPath))
				{
					projectPath = Path::getWorkingDirectory();
				}

				fileSystem->addFolder(path, true);

				auto cachePath = path + "/Cache/";
				fileSystem->addFolder(cachePath, true);
				applicationManager->setCachePath(cachePath);

				auto settingsCachePath = path + "/SettingsCache/";
				fileSystem->addFolder(settingsCachePath, true);
				applicationManager->setSettingsCachePath(settingsCachePath);

				auto relativeProjectPath = Path::getRelativePath(projectPath, filePath);
				project->load(relativeProjectPath);

				LoadProjectMsgPtr msg(new LoadProjectMsg);
				auto messageManager = editorManager->getMessageManager();
				messageManager->postMessage(msg);

				uiManager->rebuildSceneTree();
			}
		}



		//--------------------------------------------
		void ApplicationFrame::OnSaveProject(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto uiManager = editorManager->getUI();
			auto project = editorManager->getProject();

			auto projectFilePath = project->getProjectFilePath();
			auto projectPath = Path::getFilePath(projectFilePath);

			wxFileDialog dialog(this,
				_T("Save"),
				projectPath,
				projectFilePath,
				_T("FireBlade Project (*.fbproject)|*.fbproject")
			);

			dialog.SetFilterIndex(0);

			if (dialog.ShowModal() == wxID_OK)
			{
				auto filePath = String(dialog.GetPath().c_str());
				filePath = StringUtil::cleanupPath(filePath);

				project->save(filePath);
			}
		}



		//--------------------------------------------
		void ApplicationFrame::createNewScene(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto uiManager = editorManager->getUI();

			auto sceneManager = applicationManager->getSceneManager();
			auto scene = sceneManager->getCurrentScene();
			if (scene)
			{
				scene->clear();
			}

			uiManager->rebuildSceneTree();
		}



		//--------------------------------------------
		void ApplicationFrame::OnOpen(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto editorManager = EditorManager::getSingletonPtr();
			auto project = editorManager->getProject();
			FB_ASSERT(project);

			auto uiManager = editorManager->getUI();
			FB_ASSERT(uiManager);

			wxFileDialog dialog(this,
				_T("Open"),
				wxEmptyString,
				wxEmptyString,
				_T("FireBlade Scene (*.fbscene)|*.fbscene")
			);

			dialog.SetFilterIndex(0);

			if (dialog.ShowModal() == wxID_OK)
			{
				auto filePath = String(dialog.GetPath().c_str());

				auto projectPath = applicationManager->getProjectPath();
				if (StringUtil::isNullOrEmpty(projectPath))
				{
					projectPath = Path::getWorkingDirectory();
				}

				auto scenePath = Path::getRelativePath(projectPath, filePath);
				scenePath = StringUtil::cleanupPath(scenePath);
				project->setCurrentScenePath(scenePath);

				auto sceneManager = applicationManager->getSceneManager();
				auto scene = sceneManager->getCurrentScene();
				if (scene)
				{
					scene->clear();
					scene->loadScene(scenePath);
				}

				uiManager->rebuildSceneTree();
			}
		}



		//--------------------------------------------
		void ApplicationFrame::OnSave(wxCommandEvent& event)
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto editorManager = EditorManager::getSingletonPtr();
				FB_ASSERT(editorManager);

				auto project = editorManager->getProject();
				FB_ASSERT(project);

				auto sceneManager = applicationManager->getSceneManager();
				FB_ASSERT(sceneManager);

				auto scene = sceneManager->getCurrentScene();
				if (scene)
				{
					wxFileDialog dialog(this,
						_T("Save scene"),
						wxEmptyString,
						_T("Game.fbscene"),
						_T("FireBlade Scene (*.fbscene)|*.fbscene"),
						wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

					dialog.SetFilterIndex(0);

					if (dialog.ShowModal() == wxID_OK)
					{
						auto filePath = String(dialog.GetPath().c_str());
						filePath = StringUtil::cleanupPath(filePath);

						auto projectPath = applicationManager->getProjectPath();
						if (StringUtil::isNullOrEmpty(projectPath))
						{
							projectPath = Path::getWorkingDirectory();
						}

						auto scenePath = Path::getRelativePath(projectPath, filePath);

						scene->saveScene(scenePath);
					}
				}
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//--------------------------------------------
		void ApplicationFrame::OnSaveAll(wxCommandEvent& event)
		{
			//LuaEditMap::iterator it = m_luaEditMap.begin();
			//for(; it!=m_luaEditMap.end(); it++)
			//{
			//	it->second->save();
			//}
		}



		//--------------------------------------------
		void ApplicationFrame::OnUndo(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto commandManager = applicationManager->getCommandManager();
			auto cmd = commandManager->getPreviousCommand();
			if (cmd != nullptr)
			{
				cmd->undo();
			}
		}



		//--------------------------------------------
		void ApplicationFrame::OnRedo(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto commandManager = applicationManager->getCommandManager();
			auto cmd = commandManager->getNextCommand();
			if (cmd != nullptr)
			{
				cmd->redo();
			}
		}



		//--------------------------------------------
		void ApplicationFrame::OnEdit(wxCommandEvent& event)
		{
			//LuaEdit* edit = dynamic_cast<LuaEdit*>(m_bookMain->GetPage(m_bookMain->GetSelection()));
			//if (edit) 
			//	edit->GetEventHandler()->ProcessEvent (event);
		}



		//--------------------------------------------
		void ApplicationFrame::OnGoto(wxCommandEvent& event)
		{
			GotoDialog dlg(this);
			//if(dlg.ShowModal() == wxID_OK)
			//{
			//	int lineNumber = dlg.getLineNumber();

			//	LuaEdit* edit = dynamic_cast<LuaEdit*>(m_bookMain->GetPage(m_bookMain->GetSelection()));
			//	if (edit) 
			//	{
			//		edit->GotoLine(lineNumber);
			//	}
			//}
		}



		//--------------------------------------------
		void ApplicationFrame::createFileMenu()
		{
			wxMenu* recentFilesMenu = new wxMenu;
			wxMenu* recentProjectsMenu = new wxMenu;

			// create a menu bar
			wxMenu* fileMenu = new wxMenu;
			fileMenu->Append(NewProjectId, "New Project", "Create a project");
			fileMenu->Append(OpenProjectId, "Open Project", "Open a project");
			fileMenu->Append(SaveProjectId, "Save Project", "Save a project");
			fileMenu->AppendSeparator();
			fileMenu->Append(NewSceneId, "&New Scene", "New Scene");
			fileMenu->Append(OpenSceneId, "&Open Scene", "Open Scene");
			fileMenu->Append(SaveId, "&Save Scene\tCtrl+S", "Save Scene");
			fileMenu->Append(-1, "Save Scene As", "Save Scene As");
			fileMenu->Append(SaveAllId, "Save All", "Save All");
			fileMenu->AppendSeparator();
			fileMenu->Append(GenerateCMakeProjectId, "Generate CMake Project", "Generate CMake Project");
			fileMenu->AppendSeparator();
			fileMenu->Append(CreatePackageId, "Create Package", "Create Package");
			fileMenu->AppendSeparator();
			fileMenu->Append(ProjectSettingsId, "Project Settings", "Project Settings");
			fileMenu->AppendSeparator();
			fileMenu->AppendSubMenu(recentFilesMenu, "Recent Files");
			fileMenu->AppendSubMenu(recentProjectsMenu, "Recent Projects");
			fileMenu->AppendSeparator();
			fileMenu->Append(Minimal_Quit, "E&xit\tAlt-X", "Quit this program");

			auto assetsMenu = new wxMenu;
			assetsMenu->Append(AssetImportId, "Import");
			assetsMenu->Append(AssetReimportId, "Reimport");

			// create a menu bar
			wxMenu* editMenu = new wxMenu;
			editMenu->Append(UndoId, "Undo", "Undo last operation");
			editMenu->Append(RedoId, "Redo", "Redo last operation");
			editMenu->AppendSeparator();
			editMenu->Append(-1, "Find", "Find");
			editMenu->Append(GotoId, _("&Goto\tCtrl+G"));

			// create a menu bar
			wxMenu* utilMenu = new wxMenu;

			wxMenu* utilImportMenu = new wxMenu;
			utilImportMenu->Append(ImportJsonSceneId, "Import Json Scene", "Import Json Scene");

			utilImportMenu->Bind(wxEVT_MENU, &ApplicationFrame::importJsonScene, this, ApplicationFrame::ImportJsonSceneId);

			wxMenu* utilDebugMenu = new wxMenu;
			utilDebugMenu->Append(ShowAllOverlaysId, "showAllOverlays", "Undo last operation");
			utilDebugMenu->Append(HideAllOverlaysId, "hideAllOverlays", "Redo last operation");
			utilDebugMenu->Append(CreateOverlayTestId, "createOverlayPanelTest", "Redo last operation");
			utilDebugMenu->Append(CreateOverlayTextTestId, "createOverlayTextTest", "Redo last operation");
			utilDebugMenu->Append(CreateOverlayButtonTestId, "createOverlayButtonTest", "Redo last operation");

			utilDebugMenu->Bind(wxEVT_MENU, &ApplicationFrame::showAllOverlays, this, ApplicationFrame::ShowAllOverlaysId);
			utilDebugMenu->Bind(wxEVT_MENU, &ApplicationFrame::hideAllOverlays, this, ApplicationFrame::HideAllOverlaysId);
			utilDebugMenu->Bind(wxEVT_MENU, &ApplicationFrame::createOverlayPanelTest, this, ApplicationFrame::CreateOverlayTestId);
			utilDebugMenu->Bind(wxEVT_MENU, &ApplicationFrame::createOverlayTextTest, this, ApplicationFrame::CreateOverlayTextTestId);
			utilDebugMenu->Bind(wxEVT_MENU, &ApplicationFrame::createOverlayButtonTest, this, ApplicationFrame::CreateOverlayButtonTestId);

			wxMenu* utilPhysicsMenu = new wxMenu;
			utilPhysicsMenu->Append(CreateRigidStaticMeshId, "Create Rigid Static Mesh", "Undo last operation");
			utilPhysicsMenu->Append(CreateRigidDynamicMeshId, "Create Rigid Dynamic Mesh", "Redo last operation");
			utilPhysicsMenu->Append(CreateConstraintId, "Create Constraint", "Redo last operation");

			utilPhysicsMenu->Bind(wxEVT_MENU, &ApplicationFrame::createRigidStaticMesh, this, ApplicationFrame::CreateRigidStaticMeshId);
			utilPhysicsMenu->Bind(wxEVT_MENU, &ApplicationFrame::createRigidDynamicMesh, this, ApplicationFrame::CreateRigidDynamicMeshId);
			utilPhysicsMenu->Bind(wxEVT_MENU, &ApplicationFrame::createConstraint, this, ApplicationFrame::CreateConstraintId);

			wxMenu* utilProceduralMenu = new wxMenu;
			utilProceduralMenu->Append(CreateProceduralTestId, "Create Procedural Test", "Create Procedural Test");

			utilProceduralMenu->Bind(wxEVT_MENU, &ApplicationFrame::createProceduralTest, this, ApplicationFrame::CreateProceduralTestId);

			wxMenu* utilVehicleMenu = new wxMenu;
			utilVehicleMenu->Append(CreateDefaultCarId, "Create Car", "Create Car");
			utilVehicleMenu->Append(CreateDefaultTruckId, "Create Truck", "Create Truck");
			utilVehicleMenu->Bind(wxEVT_MENU, &ApplicationFrame::createDefaultCar, this, ApplicationFrame::CreateDefaultCarId);
			utilVehicleMenu->Bind(wxEVT_MENU, &ApplicationFrame::createDefaultTruck, this, ApplicationFrame::CreateDefaultTruckId);

			wxMenu* utilCodeMenu = new wxMenu;
			utilCodeMenu->Append(ConvertCSharpId, "Convert CSharp", "Convert CSharp");
			utilCodeMenu->Bind(wxEVT_MENU, &ApplicationFrame::createConvertCSharp, this, ApplicationFrame::ConvertCSharpId);

			utilMenu->AppendSubMenu(utilImportMenu, "&Import");
			utilMenu->AppendSubMenu(utilCodeMenu, "&Code");
			utilMenu->AppendSubMenu(utilDebugMenu, "&Debug");
			utilMenu->AppendSubMenu(utilPhysicsMenu, "&Physics");
			utilMenu->AppendSubMenu(utilProceduralMenu, "&Procedural");
			utilMenu->AppendSubMenu(utilVehicleMenu, "&Vehicle");



			// the "About" item should be in the help menu
			wxMenu* helpMenu = new wxMenu;
			helpMenu->Append(Minimal_About, "&About...\tF1", "Show about dialog");

			wxMenu* scriptMenu = new wxMenu;
			scriptMenu->Append(RunId, "Run");

			wxMenu* windowMenu = new wxMenu;
			windowMenu->Append(-1, "Application Components");
			windowMenu->Append(LuaEditConfigDialogId, "Lua Config");

			wxMenu* proceduralMenu = new wxMenu;
			proceduralMenu->Append(LoadProceduralSceneId, "Load Scene");
			proceduralMenu->Append(SaveProceduralSceneId, "Save Scene");

			wxMenu* physicsMenu = new wxMenu;
			wxMenuItem* physicsEnabled = physicsMenu->AppendCheckItem(PhysicsEnableId, "Enable");

			// now append the freshly created menu to the menu bar...
			wxMenuBar* menuBar = new wxMenuBar();
			menuBar->Append(fileMenu, "&File");
			menuBar->Append(editMenu, "&Edit");
			menuBar->Append(utilMenu, "&Util");
			menuBar->Append(assetsMenu, "&Assets");
			menuBar->Append(scriptMenu, "&Run");
			menuBar->Append(physicsMenu, "&Physics");
			menuBar->Append(proceduralMenu, "&Procedural");
			menuBar->Append(windowMenu, "&Windows");
			menuBar->Append(helpMenu, "&Help");

			// ... and attach this menu bar to the frame
			SetMenuBar(menuBar);
		}



		//--------------------------------------------
		void ApplicationFrame::createWindows()
		{
			auto applicationManager = IApplicationManager::instance();
			auto editorManager = EditorManager::getSingletonPtr();
			auto uiManager = editorManager->getUI();

			m_outputWindow = new OutputWindow(this);

			m_bookProject = new wxAuiNotebook(this, -1, wxDefaultPosition, wxDefaultSize);
			m_bookProps = new wxAuiNotebook(this, -1, wxDefaultPosition, wxDefaultSize);
			//m_bookComponents = new wxAuiNotebook(this, -1, wxDefaultPosition, wxDefaultSize);
			m_bookMain = new wxAuiNotebook(this, -1, wxDefaultPosition, wxDefaultSize);
			m_bookAnimation = new wxAuiNotebook(this, -1, wxDefaultPosition, wxDefaultSize);
			m_bookFiles = new wxAuiNotebook(this, -1, wxDefaultPosition, wxDefaultSize);

			auto sceneWindow = new SceneWindow(this, -1);
			m_bookProject->AddPage(sceneWindow->getWindow(), "Scene", false);
			uiManager->setSceneWindow(sceneWindow);

			auto projectWindow = new ProjectWindow(this, -1);
			m_bookAnimation->AddPage(projectWindow->getWindow(), "Project", false);
			uiManager->setProjectWindow(projectWindow);

			auto fileWindow = new FileWindow(this, -1);
			m_bookFiles->AddPage(fileWindow->getWindow(), "file browser", false);
			uiManager->setFileWindow(fileWindow);

			//auto componentsWindow = new ComponentsWindow(this, -1);
			//m_bookProps->AddPage(componentsWindow, "Components", false);

			auto objectWindow = fb::make_ptr<ObjectWindow>();
			objectWindow->setParent(this);
			objectWindow->load(nullptr);
			uiManager->setObjectWindow(objectWindow);

			m_bookProps->AddPage(objectWindow->getWindow(), "Object", false);

			/*
			m_actorWindow = new ActorWindow(this, -1);
			m_bookProps->AddPage(m_actorWindow->getWindow(), "Actor", false);
			m_actorWindow->setName("Actor");
			uiManager->setActorWindow(m_actorWindow);

			auto meshImportWindow = new MeshImportWindow(this, -1);
			m_bookComponents->AddPage(meshImportWindow->getWindow(), "Mesh Import", false);
			meshImportWindow->setName("Mesh Import");
			meshImportWindow->setNotebook(m_bookComponents);
			uiManager->setMeshImportWindow(meshImportWindow);

			auto terrainWindow = new TerrainWindow(this, -1);
			terrainWindow->setNotebook(m_bookComponents);
			m_bookComponents->AddPage(terrainWindow->getWindow(), "Terrain", false);
			terrainWindow->setName("Terrain");
			uiManager->setTerrainWindow(terrainWindow);

			auto foliageWindow = new FoliageWindow(this, -1);
			m_bookComponents->AddPage(foliageWindow->getWindow(), "Foliage", false);
			foliageWindow->setNotebook(m_bookComponents);
			foliageWindow->setName("Foliage");
			uiManager->setFoliageWindow(foliageWindow);

			auto roadFrame = new RoadFrame(this, -1);
			m_bookComponents->AddPage(roadFrame->getWindow(), "Road", false);
			roadFrame->setNotebook(m_bookComponents);
			roadFrame->setName("Road");
			uiManager->setRoadWindow(roadFrame);

			auto propertiesWindow = new PropertiesWindow(this, -1);
			m_bookComponents->AddPage(propertiesWindow->getWindow(), "Properties", false);
			propertiesWindow->setName("Properties");
			uiManager->setPropertiesWindow(propertiesWindow);

			auto textureWindow = new TextureWindow(this, -1);
			m_bookComponents->AddPage(textureWindow->getWindow(), "Texture", false);
			textureWindow->setNotebook(m_bookComponents);
			//textureWindow->setName("Texture");
			uiManager->setTextureWindow(textureWindow);

			auto materialWindow = new MaterialWindow(this, -1);
			m_bookComponents->AddPage(materialWindow->getWindow(), "Material", false);
			materialWindow->setNotebook(m_bookComponents);
			//textureWindow->setName("Texture");
			uiManager->setMaterialWindow(materialWindow);
			*/

			//auto animCtrl = new AnimationControlsPanel(this, -1);
			//m_bookAnimation->AddPage(animCtrl, "Animation", false);

			auto editorRenderWindow = new RenderWindow(this, -1);
			m_bookMain->AddPage(editorRenderWindow, "Editor", false);
			uiManager->setRenderWindow(editorRenderWindow);

			//auto sceneRenderWindow = new RenderWindow(this, -1);
			//m_bookMain->AddPage(sceneRenderWindow, "Scene", false);
			//uiManager->setSceneWindow(sceneRenderWindow);

			//m_windows.push_back(sceneWindow);
			m_windows.push_back(projectWindow);
			//m_windows.push_back(editorRenderWindow);
			//m_windows.push_back(sceneRenderWindow);
			//m_windows.push_back(propertiesWindow);

			wxAuiPaneInfo info;
			info.DestroyOnClose(false).MaximizeButton(true).MinimizeButton(true);

			m_aui->AddPane(m_bookProject,
				wxAuiPaneInfo(info).Caption("Project").Left().Layer(0).Name("Project").MinSize(200, 100));

			m_aui->AddPane(m_bookProps,
				wxAuiPaneInfo(info).Caption("Properties").Right().Layer(2).Name("Properties").MinSize(200, 100));

			//m_aui->AddPane(m_bookComponents,
			//	wxAuiPaneInfo(info).Caption("Components").Right().Layer(2).Name("Components").MinSize(200,100));

			m_aui->AddPane(m_bookAnimation,
				wxAuiPaneInfo(info).Caption("Animation").Center().Bottom().Name("Animation").MinSize(200, 100));

			m_aui->AddPane(m_bookFiles,
				wxAuiPaneInfo(info).Caption("FileBrowser").Center().Bottom().Name("FileBrowser").MinSize(200, 100));

			m_aui->AddPane(m_bookMain, wxAuiPaneInfo().Name("bookMain").Layer(0).
				CenterPane().PaneBorder(false));

			m_bookMain->AddPage(m_outputWindow, "Output");
		}



		//--------------------------------------------
		void ApplicationFrame::createToolbars()
		{
			auto mediaIconsPath = String("");

#ifdef FB_PLATFORM_WIN32
			mediaIconsPath = String("../../../../Media/icons/");
#elif defined (FB_PLATFORM_APPLE)
			mediaIconsPath = String("../../Media/icons/");
#else
			mediaIconsPath = String("../../../../Media/icons/");
#endif

			wxAuiToolBarItemArray prepend_items;
			wxAuiToolBarItemArray append_items;
			wxAuiToolBarItem item;
			item.SetKind(wxITEM_SEPARATOR);
			append_items.Add(item);
			item.SetKind(wxITEM_NORMAL);
			item.SetId(ID_CustomizeToolbar);
			item.SetLabel(_("Customize..."));
			append_items.Add(item);


			// create some toolbars
			wxAuiToolBar* tb1 = new wxAuiToolBar(this, ID_SampleItem, wxDefaultPosition, wxDefaultSize,
				wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_OVERFLOW | wxAUI_TB_HORIZONTAL);
			tb1->SetToolBitmapSize(FromDIP(wxSize(16, 16)));

			tb1->Connect(wxEVT_COMMAND_TOOL_CLICKED, wxAuiToolBarEventHandler(ApplicationFrame::toolbarButtonClick), nullptr, this);

			wxImage orbitPNG(mediaIconsPath + "/32/2728679_tool_cad_sphere_wire_modeling_icon.png");
			wxBitmap orbitBMP(orbitPNG);

			auto tool1 = tb1->AddTool(ID_SampleItem + 1, "Test", orbitBMP);

			tb1->AddSeparator();

			wxImage translatePNG(mediaIconsPath + "/32/2728683_orgin_tool_move_cad_modeling_icon.png");
			wxBitmap translateBMP(translatePNG);

			wxImage rotatePNG(mediaIconsPath + "/32/2728714_modeling_tool_orbit_cad_drawing_icon.png");
			wxBitmap rotateBMP(rotatePNG);

			wxImage scalePNG(mediaIconsPath + "/32/2728685_orgin_tool_cad_scale_modeling_icon.png");
			wxBitmap scaleBMP(scalePNG);

			wxImage transformSpacePNG(mediaIconsPath + "/32/2728687_tool_cad_modeling_drawing_tessellate_icon.png");
			wxBitmap transformSpaceBMP(transformSpacePNG);

			tb1->AddTool(ID_SampleItem + 2, "Test", translateBMP);
			tb1->AddTool(ID_SampleItem + 3, "Test", rotateBMP);
			tb1->AddTool(ID_SampleItem + 4, "Test", scaleBMP);

			tb1->AddSeparator();

			tb1->AddTool(ID_SampleItem + 5, "Transform Space", transformSpaceBMP);
			tb1->SetCustomOverflowItems(prepend_items, append_items);
			tb1->Realize();

			m_aui->AddPane(tb1, wxAuiPaneInfo().
				Name("tb1").Caption("Big Toolbar").
				ToolbarPane().Top());

			wxAuiToolBar* tb2 = new wxAuiToolBar(this, ID_SampleItem, wxDefaultPosition, wxDefaultSize,
				wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_OVERFLOW | wxAUI_TB_HORIZONTAL);
			tb2->SetToolBitmapSize(FromDIP(wxSize(16, 16)));

			wxImage playPNG(mediaIconsPath + "/32/344692_audio_play_sound_video_arrow_icon.png");
			wxBitmap playBMP(playPNG);

			wxImage pausePNG(mediaIconsPath + "/32/6644786_pause_pause button_pause song_player pause_track pause_icon.png");
			wxBitmap pauseBMP(pausePNG);

			wxImage stopPNG(mediaIconsPath + "/32/5486265_circle_media_player_stop_icon.png");
			wxBitmap stopBMP(stopPNG);

			tb2->AddTool(ID_SampleItem + 6, "Test", playBMP);
			tb2->AddTool(ID_SampleItem + 7, "Test", pauseBMP);
			tb2->AddTool(ID_SampleItem + 8, "Test", stopBMP);

			tb2->Connect(wxEVT_COMMAND_TOOL_CLICKED, wxAuiToolBarEventHandler(ApplicationFrame::toolbarButtonClick), nullptr, this);

			m_aui->AddPane(tb2, wxAuiPaneInfo().
				Name("tb2").Caption("Big Toolbar").
				ToolbarPane().Top());
		}



		//--------------------------------------------
		void ApplicationFrame::importJsonScene(wxCommandEvent& event)
		{
			wxFileDialog dialog(this,
				_T("Open"),
				wxEmptyString,
				wxEmptyString,
				_T("Json Scene (*.json)|*.json")
			);

			dialog.SetFilterIndex(0);

			if (dialog.ShowModal() == wxID_OK)
			{
				auto applicationManager = IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto editorManager = EditorManager::getSingletonPtr();
				FB_ASSERT(editorManager);

				auto project = editorManager->getProject();
				FB_ASSERT(project);

				auto uiManager = editorManager->getUI();
				FB_ASSERT(uiManager);

				auto filePath = String(dialog.GetPath().c_str());
				filePath = StringUtil::cleanupPath(filePath);

				auto fileSystem = applicationManager->getFileSystem();
				FB_ASSERT(fileSystem);
				
				auto sceneData = ApplicationUtil::importScene(filePath);

				auto sceneManager = applicationManager->getSceneManager();
				auto scene = sceneManager->getCurrentScene();
				if (scene)
				{
					scene->fromData(sceneData);
				}

				uiManager->rebuildSceneTree();
			}
		}



		//--------------------------------------------
		void ApplicationFrame::showAllOverlays(wxCommandEvent& event)
		{
			ApplicationUtil::showAllOverlays();
		}



		//--------------------------------------------
		void ApplicationFrame::hideAllOverlays(wxCommandEvent& event)
		{
			ApplicationUtil::hideAllOverlays();
		}



		//--------------------------------------------
		void ApplicationFrame::createOverlayPanelTest(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto actor = ApplicationUtil::createOverlayPanelTest();
			FB_ASSERT(actor);

			//if (applicationManager->isPlaying())
			//{
			//	actor->start();
			//}
			//else
			//{
			//	actor->edit();
			//}

			auto editorManager = EditorManager::getSingletonPtr();
			auto ui = editorManager->getUI();
			ui->rebuildSceneTree();
		}



		//--------------------------------------------
		void ApplicationFrame::createOverlayTextTest(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto actor = ApplicationUtil::createOverlayTextTest();
			FB_ASSERT(actor);

			//if (applicationManager->isPlaying())
			//{
			//	actor->start();
			//}
			//else
			//{
			//	actor->edit();
			//}

			auto editorManager = EditorManager::getSingletonPtr();
			auto ui = editorManager->getUI();
			ui->rebuildSceneTree();
		}



		//--------------------------------------------
		void ApplicationFrame::createOverlayButtonTest(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto actor = ApplicationUtil::createOverlayButtonTest();
			FB_ASSERT(actor);

			//if (applicationManager->isPlaying())
			//{
			//	actor->start();
			//}
			//else
			//{
			//	actor->edit();
			//}

			auto editorManager = EditorManager::getSingletonPtr();
			auto ui = editorManager->getUI();
			ui->rebuildSceneTree();
		}



		//--------------------------------------------
		void ApplicationFrame::createRigidStaticMesh(wxCommandEvent& event)
		{
			ApplicationUtil::createRigidStaticMesh();
		}



		//--------------------------------------------
		void ApplicationFrame::createRigidDynamicMesh(wxCommandEvent& event)
		{
			ApplicationUtil::createRigidDynamicMesh();
		}



		//--------------------------------------------
		void ApplicationFrame::createConstraint(wxCommandEvent& event)
		{
			ApplicationUtil::createDefaultConstraint();

			auto editorManager = EditorManager::getSingletonPtr();
			auto ui = editorManager->getUI();
			ui->rebuildSceneTree();
		}




		//--------------------------------------------
		void ApplicationFrame::createProceduralTest(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto actor = ApplicationUtil::createProceduralTest();
			FB_ASSERT(actor);

			//if (applicationManager->isPlaying())
			//{
			//	actor->start();
			//}
			//else
			//{
			//	actor->edit();
			//}

			auto editorManager = EditorManager::getSingletonPtr();
			auto ui = editorManager->getUI();
			ui->rebuildSceneTree();
		}



		//--------------------------------------------
		void ApplicationFrame::createDefaultCar(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto actor = ApplicationUtil::createDefaultCar();
			FB_ASSERT(actor);

			//if (applicationManager->isPlaying())
			//{
			//	actor->start();
			//}
			//else
			//{
			//	actor->edit();
			//}

			auto editorManager = EditorManager::getSingletonPtr();
			auto ui = editorManager->getUI();
			ui->rebuildSceneTree();
		}



		//--------------------------------------------
		void ApplicationFrame::createDefaultTruck(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto actor = ApplicationUtil::createDefaultTruck();
			FB_ASSERT(actor);

			//if (applicationManager->isPlaying())
			//{
			//	actor->start();
			//}
			//else
			//{
			//	actor->edit();
			//}

			auto editorManager = EditorManager::getSingletonPtr();
			auto ui = editorManager->getUI();
			ui->rebuildSceneTree();
		}



		//--------------------------------------------
		void ApplicationFrame::createConvertCSharp(wxCommandEvent& event)
		{
			ApplicationUtil::convertCSharp("", "");
		}



		//--------------------------------------------
		void ApplicationFrame::OnLuaEditConfigDialog(wxCommandEvent& event)
		{
			//LuaEditConfigDialog dlg(this);
			//dlg.ShowModal();
		}



		//--------------------------------------------
		void ApplicationFrame::OnPhysicsEnable(wxCommandEvent& event)
		{
			bool checked = event.IsChecked();

			auto applicationManager = IApplicationManager::instance();
			applicationManager->setPlaying(checked);
		}



		//--------------------------------------------
		void ApplicationFrame::generateCMakeProject(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto fileSystem = applicationManager->getFileSystem();

			Array<String> defines;
			Array<String> includes;
			Array<String> libraryPaths;
			Array<String> libs;

			defines.push_back("_FB_STATIC_LIB_");

#if FB_USE_ONETBB
			defines.push_back("FB_USE_ONETBB=1");
#elif FB_USE_TBB
			defines.push_back("FB_USE_TBB=1");
#endif

			auto enginePath = String("E:/dev/fireblade");

			includes.push_back(enginePath + "/Dependencies/boost");

#if FB_USE_ONETBB
			includes.push_back(enginePath + "/Dependencies/onetbb/include");
#elif FB_USE_TBB
			includes.push_back(enginePath + "/Dependencies/tbb/include");
#endif

			includes.push_back(enginePath + "/FBEngine/Include/FBCPlusPlus");
			includes.push_back(enginePath + "/AdditionalLibraries/cppsqlite/include");
			includes.push_back(enginePath + "/AdditionalLibraries/sqlite/include");
			includes.push_back(enginePath + "/AdditionalLibraries/aurora");
			includes.push_back(enginePath + "/FBEngine/Include");
			includes.push_back(enginePath + "/FBEngine/Include/FBCPlusPlus");

			libs.push_back("FBCore.lib");
			libs.push_back("FBFileSystem.lib");
			libs.push_back("FBMath.lib");
			libs.push_back("FBMesh.lib");
			libs.push_back("FBInterface.lib");
			libs.push_back("FBProcedural.lib");
			libs.push_back("FBSystem.lib");
			libs.push_back("FBPhysx.lib");
			libs.push_back("FBData.lib");
			libs.push_back("FBSQLite.lib");
			libs.push_back("FBObjectTemplates.lib");
			libs.push_back("FBVehicle.lib");
			libs.push_back("FBWxWidgets.lib");
			libs.push_back("FBObjectTemplates.lib");
			libs.push_back("FBApplication.lib");
			libs.push_back("FBStateMessages.lib");
			libs.push_back("FBOISInput.lib");
			libs.push_back("FBGraphics.lib");
			libs.push_back("glfw.lib");
			libs.push_back("RocketControls_d.lib");
			libs.push_back("RocketCore_d.lib");
			libs.push_back("RocketDebugger_d.lib");
			libs.push_back("RuntimeCompiler_VS2010.lib");
			libs.push_back("RuntimeObjectSystem.lib");
			libs.push_back("Systems.lib");

			auto toolset = String("v142");
			auto arch = String("x64");

			libraryPaths.push_back(enginePath + "/libs/windows/" + toolset + "/" + arch + "/${CMAKE_BUILD_TYPE}");
			libraryPaths.push_back(enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch + "/physx/");
			libraryPaths.push_back(enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch + "/ogre/");
			libraryPaths.push_back(enginePath + "/Dependencies/lib/windows/" + toolset + "/" + arch + "/tbb/");
			libraryPaths.push_back(enginePath + "/Dependencies/lib/windows/" + toolset + "/" + arch + "/boost/");
			libraryPaths.push_back(enginePath + "/Dependencies/lib/windows/" + toolset + "/" + arch + "/tinybind/");
			libraryPaths.push_back(enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch + "/wildmagic/");
			libraryPaths.push_back(enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch + "/assimp/${CMAKE_BUILD_TYPE}");
			libraryPaths.push_back(enginePath + "/AdditionalLibraries/lib/windows/" + toolset + "/" + arch + "/ois/");
			libraryPaths.push_back(enginePath + "/AdditionalLibraries/aurora/x64/${CMAKE_BUILD_TYPE}");

			auto newLineStr = "\n";
			auto cmakeRequired = String("cmake_minimum_required(VERSION 3.13)\n");
			auto projectName = String("Vehicle");
			auto projectDeclare = "project(" + projectName + ")\n";

			auto compileOptions =
				String("if (WIN32)") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} /fp:fast\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} /Qvec-report:2\")") + newLineStr +
				String("") + newLineStr +
				String("	if (NOT FB_PLATFORM_X64)") + newLineStr +
				String("		set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} /arch:SSE2\")") + newLineStr +
				String("	endif()") + newLineStr +
				String("") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_DEBUG \"/D_DEBUG /MP /MTd /Zi /Ob0 /Od\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_MINSIZEREL \"/MT /MP /Zi /O2 /Ob2 /D NDEBUG\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_RELEASE \"/MT /MP /O2 /Ob2 /Zi /D NDEBUG\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO \"/MT /MP /Zi /Od /Ob0 /D NDEBUG\")") + newLineStr +
				String("elseif(APPLE)") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -msse -msse2 -msse3\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_DEBUG \"-g\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_MINSIZEREL \"-O3 -finline-functions -DNDEBUG\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_RELEASE \"-O3 -finline-functions -DNDEBUG\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO \"-O3 -g -finline-functions -DNDEBUG\")") + newLineStr +
				String("else()") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -msse -msse2 -msse3\")") + newLineStr +
				String("	#set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS} -msse -msse2 -msse3 -Wno-everything\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_DEBUG \"-g\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_MINSIZEREL \"-O3 -finline-functions -DNDEBUG\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_RELEASE \"-O3 -finline-functions -DNDEBUG\")") + newLineStr +
				String("	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO \"-O3 -g -finline-functions -DNDEBUG\")") + newLineStr +
				String("") + newLineStr +
				String("	add_compile_definitions(NDEBUG)") + newLineStr +
				String("endif()");

			auto headerFilesStr = String("file(GLOB_RECURSE HEADER_FILES \"Assets/*.h\")\n");
			auto sourceFilesStr = String("file(GLOB_RECURSE SOURCE_FILES \"Assets/*.cpp\")\n");

			auto defineOpen = String("add_compile_definitions(");
			auto defineClose = String(")");

			auto includeDirOpen = String("include_directories(");
			auto includeDirClose = String(")");

			auto libraryDirOpen = String("link_directories(");
			auto libraryDirClose = String(")");

			auto libraryOpen = String("target_link_libraries(");
			auto libraryClose = String(")");

			auto cmakeStr = cmakeRequired + projectDeclare;
			cmakeStr += newLineStr;

			cmakeStr += compileOptions + newLineStr;

			cmakeStr += newLineStr;

			for (auto define : defines)
			{
				cmakeStr += defineOpen + define + defineClose + newLineStr;
			}

			cmakeStr += newLineStr +
				headerFilesStr +
				sourceFilesStr + newLineStr;

			for (auto includeDir : includes)
			{
				cmakeStr += includeDirOpen + includeDir + includeDirClose + newLineStr;
			}

			cmakeStr += newLineStr;

			for (auto libraryPath : libraryPaths)
			{
				cmakeStr += libraryDirOpen + projectName + " " + libraryPath + libraryDirClose + newLineStr;
			}

			cmakeStr += newLineStr;
			auto libraryDeclare = String("add_library(" + projectName + " SHARED ${HEADER_FILES} ${SOURCE_FILES})\n");
			cmakeStr += libraryDeclare + newLineStr;

			for (auto lib : libs)
			{
				cmakeStr += libraryOpen + projectName + " " + lib + libraryClose + newLineStr;
			}

			cmakeStr += newLineStr;

			auto projectPath = applicationManager->getProjectPath();

			fileSystem->writeAllText(projectPath + "/CMakeLists.txt", cmakeStr);
		}



		//--------------------------------------------
		void ApplicationFrame::packageProject(wxCommandEvent& event)
		{
			// copy exe

			//create pak file

			// copy over pak file
		}



		//--------------------------------------------
		void ApplicationFrame::projectSettings(wxCommandEvent& event)
		{
			ProjectSettingsDialog dialog(this);
			if (dialog.ShowModal() == wxID_OK)
			{

			}
		}



		//--------------------------------------------
		LuaEdit* ApplicationFrame::openScript(SmartPtr<ScriptTemplate> scriptTemplate)
		{


			String fileName = scriptTemplate->getFileName();

			//LuaEditMap::iterator it = m_luaEditMap.find(fileName);
			//if(it == m_luaEditMap.end())
			//{				
			//	String data;

			//	ApplicationManager* appRoot = IApplicationManager::instance();

			//	auto applicationManager = IApplicationManager::instance();
			//	SmartPtr<IFileSystem>& fileSystem = applicationManager->getFileSystem();

			//	ProjectPtr project = appRoot->getProject();
			//	String projectPath = project->getProjectDirectory();
			//	String scriptPath = Path::getAbsolutePath(projectPath, scriptTemplate->getFilePath());
			//	scriptPath += "/";

			//	SmartPtr<IStream> stream = fileSystem->open(scriptTemplate->getFileName(), scriptPath);
			//	if(!stream)
			//	{
			//		wxMessageBox("File not found. ");
			//		return nullptr;
			//	}
			//	else 
			//	{
			//		data = stream->getAsString();
			//		scriptTemplate->setData(data);

			//		//LuaEdit* luaEdit = new LuaEdit(this, this);
			//		//luaEdit->setScriptTemplate(scriptTemplate);
			//		////luaEdit->setData(data);
			//		//luaEdit->openFile((scriptPath + scriptTemplate->getFileName()).c_str());
			//		//luaEdit->setFileName(scriptTemplate->getFileName().c_str());
			//		//m_bookMain->AddPage(luaEdit, fileName.c_str());
			//		//luaEdit->SetFocus();
			//		//luaEdit->setPage(m_bookMain->GetSelection());

			//		//m_luaEditMap[fileName] = luaEdit;

			//		//return luaEdit;
			//	}
			//}
			//else
			//{
			//	LuaEdit* luaEdit = it->second;
			//	m_bookMain->SetSelection(luaEdit->getPage());
			//	luaEdit->reload();
			//	return luaEdit;
			//}

			return nullptr;
		}



		//--------------------------------------------
		void ApplicationFrame::importAssets(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto resourceDatabase = applicationManager->getResourceDatabase();

			resourceDatabase->importAssets();
		}



		//--------------------------------------------
		void ApplicationFrame::reimportAssets(wxCommandEvent& event)
		{
			auto applicationManager = IApplicationManager::instance();
			auto resourceDatabase = applicationManager->getResourceDatabase();

			resourceDatabase->reimportAssets();
		}



		//--------------------------------------------
		void ApplicationFrame::loadProceduralScene(wxCommandEvent& event)
		{
			wxFileDialog dialog(this,
				_T("Open"),
				wxEmptyString,
				wxEmptyString,
				_T("OSM (*.osm)|*.osm")
			);

			dialog.SetFilterIndex(0);

			if (dialog.ShowModal() == wxID_OK)
			{
				auto applicationManager = IApplicationManager::instance();
				auto editorManager = EditorManager::getSingletonPtr();
				auto uiManager = editorManager->getUI();

				auto commandManager = applicationManager->getCommandManager();
				auto cmd = fb::make_ptr<AddActorCmd>();
				cmd->setActorType(AddActorCmd::ActorType::ProceduralScene);
				commandManager->addCommand(cmd);

				auto actor = cmd->getActor();
				if (actor)
				{
					auto proceduralScene = actor->getComponent<component::ProceduralScene>();
					if (proceduralScene)
					{
						auto filePath = dialog.GetPath();
						auto filePathStr = String(filePath.c_str());
						filePathStr = StringUtil::cleanupPath(filePathStr);

						proceduralScene->loadMapData(filePathStr);
					}
				}

				uiManager->rebuildSceneTree();
			}
		}



		//--------------------------------------------
		void ApplicationFrame::saveProceduralScene(wxCommandEvent& event)
		{
		}



		//--------------------------------------------
		void ApplicationFrame::OnClose(wxCloseEvent& event)
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			bool isFileSaved = appRoot->getFileSaved();

			if (event.CanVeto() && !isFileSaved)
			{
				if (wxMessageBox("The file has not been saved... continue closing?",
					"Please confirm",
					wxICON_QUESTION | wxYES_NO) != wxYES)
				{
					event.Veto();
					return;
				}
			}

			Destroy();  // you may also do:  event.Skip();
			// since the default event handler does call Destroy(), too

			auto applicationManager = IApplicationManager::instance();
			applicationManager->setRunning(false);
		}



		void ApplicationFrame::toolbarButtonClick(wxAuiToolBarEvent& event)
		{
			auto editorManager = EditorManager::getSingletonPtr();
			FB_ASSERT(editorManager);

			auto id = event.GetId();

			if (id == ID_SampleItem + 1)
			{
				auto translateTool = editorManager->getTranslateManipulator();
				if (translateTool)
				{
					translateTool->setEnabled(false);
				}

				auto rotateTool = editorManager->getRotateManipulator();
				if (rotateTool)
				{
					rotateTool->setEnabled(false);
				}

				auto scaleTool = editorManager->getScaleManipulator();
				if (scaleTool)
				{
					//scaleTool->setEnabled(false);
				}
			}
			else if (id == ID_SampleItem + 2)
			{
				auto translateTool = editorManager->getTranslateManipulator();
				if (translateTool)
				{
					translateTool->load(nullptr);
					translateTool->setEnabled(true);
				}

				auto rotateTool = editorManager->getRotateManipulator();
				if (rotateTool)
				{
					rotateTool->setEnabled(false);
				}
			}
			else if (id == ID_SampleItem + 3)
			{
				auto translateTool = editorManager->getTranslateManipulator();
				if (translateTool)
				{
					translateTool->setEnabled(false);
				}

				auto rotateTool = editorManager->getRotateManipulator();
				if (rotateTool)
				{
					rotateTool->load(nullptr);
					rotateTool->setEnabled(true);
				}
			}
			else if (id == ID_SampleItem + 4)
			{
				auto translateTool = editorManager->getTranslateManipulator();
				if (translateTool)
				{
					translateTool->setEnabled(false);
				}

				auto rotateTool = editorManager->getRotateManipulator();
				if (rotateTool)
				{
					rotateTool->setEnabled(false);
				}

				auto scaleTool = editorManager->getScaleManipulator();
				if (scaleTool)
				{
					//scaleTool->setEnabled(false);
				}
			}
			else if (id == ID_SampleItem + 5)
			{
				int stop = 0;
				stop = 0;
			}
			else if (id == ID_SampleItem + 6)
			{
				editorManager->startPlaying();
			}
			else if (id == ID_SampleItem + 7)
			{
				editorManager->pausePlaying();
			}
			else if (id == ID_SampleItem + 8)
			{
				editorManager->stopPlaying();
			}
		}

		wxAuiManager* ApplicationFrame::getAui() const
		{
			return m_aui;
		}

		void ApplicationFrame::setAui(wxAuiManager* val)
		{
			m_aui = val;
		}

		bool ApplicationFrame::saveProject()
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();

			String projectFilePath = project->getProjectFilePath();
			if (projectFilePath.length() == 0 || projectFilePath == StringUtil::EmptyString)
			{
				wxFileDialog dialog(this,
					_T("Save scene"),
					wxEmptyString,
					_T("Game.fbp"),
					_T("FireBlade Project (*.fbp)|*.fbp"),
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

				dialog.SetFilterIndex(0);

				if (dialog.ShowModal() == wxID_OK)
				{
					String filePath = String(dialog.GetPath().c_str());
					project->setProjectPath(filePath);
					project->save(filePath);
					return true;
				}
			}

			return true;
		}

		void ApplicationFrame::saveApplication()
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();

			SmartPtr<ApplicationTemplate> applicationTemplate = project->getApplicationTemplate();
			if (applicationTemplate->getFilePath() == StringUtil::EmptyString)
			{
				wxFileDialog dialog(this,
					_T("Save Application"),
					wxEmptyString,
					_T("Application.application"),
					_T("FireBlade Application (*.application)|*.application"),
					wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

				dialog.SetFilterIndex(0);

				if (dialog.ShowModal() == wxID_OK)
				{
					String filePath = String(dialog.GetPath().c_str());

					String projectPath = project->getProjectDirectory();
					String relativePath = Path::getRelativePath(projectPath, filePath);

					applicationTemplate->setFilePath(relativePath);

					TemplateXMLWriter::save(filePath, applicationTemplate);
				}
			}
			else
			{
				String fullScriptFilePath = Path::getAbsolutePath(project->getProjectDirectory(), applicationTemplate->getFilePath());
				TemplateXMLWriter::save(fullScriptFilePath, applicationTemplate);
			}
		}

		void ApplicationFrame::DosetSize(int x, int y, int width, int height, int WXUNUSED(sizeFlags /*= wxSIZE_AUTO*/))
		{

		}

		void ApplicationFrame::update(s32 task, time_interval t, time_interval dt)
		{
			//if (m_aui)
			//{
			//	m_aui->Update();
			//}

			for (auto window : m_windows)
			{
				window->update();
			}
		}



	} // end namespace editor
} // end namespace fb



