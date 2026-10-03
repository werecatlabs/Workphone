#include "MeshViewerPCH.hpp"
#include "MeshViewer.hpp"
#include <FBCore/System/ApplicationManagerMT.hpp>
#include <FBCore/System/TimerMT.hpp>
#include <FBApplication/Core/LogManager.hpp>
#include <FBFileSystem/FBFileSystem.hpp>

#include <FBApplication/Core/LogManager.hpp>
#include <FBApplication/System/FactoryManager.hpp>
#include <FBApplication/System/CFrameStatistics.hpp>
#include <FBApplication/ApplicationUtil.hpp>
#include <FBApplication/Resource/CMeshManager.hpp>
#include <FBApplication/Resource/CPrefabManager.hpp>
#include <FBCore/Interface/UI/IUIFileBrowser.hpp>

#if FB_GRAPHICS_SYSTEM_OGRENEXT
#include <FBGraphicsOgreNext/FBGraphicsOgre.hpp>
#elif FB_GRAPHICS_SYSTEM_OGRE
#include <FBGraphicsOgre/FBGraphicsOgre.hpp>
#endif

#if FB_BUILD_IMGUI
#include <FBImGui/FBImGui.hpp>
#endif

#include <FBAssimp/FBAssimp.hpp>



namespace fb
{
	namespace viewer
	{



		//--------------------------------------------
		MeshViewer::MeshViewer()
		{

		}



		//--------------------------------------------
		MeshViewer::~MeshViewer()
		{
			unload(nullptr);
		}



		//--------------------------------------------
		void MeshViewer::load(SmartPtr<ISharedObject> data)
		{
			setLoadingState(LoadingState::Loading);

			auto currentThread = Thread::ThreadId::Primary;
			Thread::setCurrentThreadId(currentThread);

			auto currentTask = Thread::Task::Primary;
			Thread::setCurrentTask(currentTask);

			auto applicationManager = fb::make_ptr<core::ApplicationManagerMT>();
			core::IApplicationManager::setInstance(applicationManager);

			auto prefabManager = fb::make_ptr<CPrefabManager>();
			applicationManager->setPrefabManager(prefabManager);

			auto meshLoader = FBAssimp::createAssimpLoader();
			applicationManager->setMeshLoader(meshLoader);

			auto meshManager = fb::make_ptr<CMeshManager>();
			applicationManager->setMeshManager(meshManager);

			CApplication::load(data);

			auto uiManager = applicationManager->getUI();
			if (uiManager)
			{
				m_frameStatistics = fb::make_ptr<CFrameStatistics>();
				m_frameStatistics->load(nullptr);

				auto menuBar = uiManager->addElementByType<ui::IUIMenubar>();

				auto fileMenu = uiManager->addElementByType<ui::IUIMenu>();
				fileMenu->setLabel("File");
				menuBar->addMenu(fileMenu);

				ApplicationUtil::addMenuItem(fileMenu, (s32)MeshViewer::ElementId::Open, "Open", "Open", ui::IUIMenuItem::Type::Normal);
				ApplicationUtil::addMenuItem(fileMenu, (s32)MeshViewer::ElementId::Exit, "Exit", "Exit", ui::IUIMenuItem::Type::Normal);

				m_application->setMenubar(menuBar);

				auto menubarListener = fb::make_ptr<CUIMenuBarListener>();
				menubarListener->setOwner(this);
				m_menubarListener = menubarListener;

				menuBar->setListener(m_menubarListener);

				auto fileBrowserListener = fb::make_ptr<CUIFileBrowserListener>();
				fileBrowserListener->setOwner(this);
				m_fileBrowserListener = fileBrowserListener;
			}

			auto graphicsSystem = applicationManager->getGraphicsSystem();
			FB_ASSERT(graphicsSystem);

			auto textureManager = graphicsSystem->getTextureManager();
			FB_ASSERT(textureManager);

			m_renderTarget = textureManager->createRenderTexture();
			FB_ASSERT(m_renderTarget);

			FB_ASSERT(m_camera);
			m_camera->setTargetTexture(m_renderTarget);

			if (m_renderWindow)
			{
				m_renderWindow->setRenderTexture(m_renderTarget);
			}

			if (graphicsSystem)
			{
				auto window = graphicsSystem->getDefaultWindow();
				if (!window)
				{
					window = m_renderWindow->getWindow();
				}

				auto renderSceneManager = graphicsSystem->getSceneManager();
				auto renderCamera = renderSceneManager->getCamera();
				graphicsSystem->setupRenderer(renderSceneManager, window, renderCamera, "", true);
			}

			setLoadingState(LoadingState::Loaded);
		}



		//--------------------------------------------
		void MeshViewer::unload(SmartPtr<ISharedObject> data)
		{
			setLoadingState(LoadingState::Unloading);

			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			if (m_frameStatistics)
			{
				m_frameStatistics->unload(nullptr);
				m_frameStatistics = nullptr;
			}

			applicationManager->unload(nullptr);

			setLoadingState(LoadingState::Unloaded);
		}



		//--------------------------------------------
		void MeshViewer::run()
		{
			auto currentThreadId = Thread::ThreadId::Primary;
			Thread::setCurrentThreadId(currentThreadId);

			auto task = Thread::Task::Primary;
			Thread::setCurrentTask(task);

			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto timer = applicationManager->getTimer();
			FB_ASSERT(timer);

			auto taskManager = applicationManager->getTaskManager();
			FB_ASSERT(taskManager);

			auto stateManager = applicationManager->getStateManager();
			FB_ASSERT(stateManager);

			auto graphicsSystem = applicationManager->getGraphicsSystem();
			FB_ASSERT(graphicsSystem);

			while (applicationManager->isRunning())
			{
				try
				{
					timer->update();
					auto t = timer->getTime();
					auto dt = timer->getDeltaTime();

					taskManager->update();

					stateManager->update();

					graphicsSystem->update();

					graphicsSystem->messagePump();

					if (applicationManager->getQuit())
					{
						applicationManager->setRunning(false);
					}

					auto& gc = GarbageCollector::instance();
					gc.update();

					//m_renderTarget->save();
				}
				catch (std::exception& e)
				{
					FB_LOG_EXCEPTION(e);
				}
			}
		}



		//--------------------------------------------
		SmartPtr<IActor> MeshViewer::getMeshActor() const
		{
			return m_meshActor;
		}



		//--------------------------------------------
		void MeshViewer::setMeshActor(SmartPtr<IActor> val)
		{
			m_meshActor = val;
		}



		//--------------------------------------------
		void MeshViewer::createLogManager()
		{
			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto logManager = fb::make_ptr<LogManager>();
			applicationManager->setLogManager(logManager);
			logManager->open("MeshViewer.log");
		}



		//--------------------------------------------
		void MeshViewer::createFactoryManager()
		{
			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto factoryManager = fb::make_ptr<FactoryManager>();
			applicationManager->setFactoryManager(factoryManager);
			FB_ASSERT(applicationManager->getFactoryManager());
		}



		//--------------------------------------------
		void MeshViewer::createTimer()
		{
			auto applicationManager = core::IApplicationManager::instance();

			auto timer = fb::make_ptr<TimerMT>();
			applicationManager->setTimer(timer);
		}



		//--------------------------------------------
		void MeshViewer::createTaskManager()
		{
			auto applicationManager = core::IApplicationManager::instance();

			auto taskManager = fb::make_ptr<TaskManager>();
			applicationManager->setTaskManager(taskManager);
		}



		//--------------------------------------------
		void MeshViewer::createThreadPool()
		{
			auto applicationManager = core::IApplicationManager::instance();

			auto threadPool = fb::make_ptr<ThreadPool>();
			applicationManager->setThreadPool(threadPool);
		}



		//--------------------------------------------
		void MeshViewer::createStateManager()
		{
			auto applicationManager = core::IApplicationManager::instance();

			auto stateManager = fb::make_ptr<StateManagerStandard>();
			applicationManager->setStateManager(stateManager);
		}



		//--------------------------------------------
		void MeshViewer::createUI()
		{
			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto uiManager = ui::FBImGui::createUI();
			FB_ASSERT(uiManager);
			applicationManager->setUI(uiManager);

			auto application = uiManager->addElementByType<ui::IUIApplication>();
			uiManager->setApplication(application);
			m_application = application;
		}
		//--------------------------------------------
		bool MeshViewer::createGraphicsSystem()
		{
			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

#if FB_GRAPHICS_SYSTEM_OGRENEXT
			auto graphicsSystem = render::FBGraphicsOgre::createGraphicsOgre();
#elif FB_GRAPHICS_SYSTEM_OGRE
			auto graphicsSystem = render::FBGraphicsOgre::createGraphicsOgre();
#else
			SmartPtr<render::IGraphicsSystem> graphicsSystem;
#endif

			FB_ASSERT(graphicsSystem);

			applicationManager->setGraphicsSystem(graphicsSystem);
			graphicsSystem->load(nullptr);

			auto configuration = graphicsSystem->createConfiguration();
			FB_ASSERT(configuration);

			configuration->setCreateWindow(true);

			if (!graphicsSystem->configure(configuration))
			{
				return false;
			}

			m_window = graphicsSystem->getDefaultWindow();
			FB_ASSERT(m_window);

			return true;
		}
		//--------------------------------------------
		void MeshViewer::createRenderWindow()
		{
			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto ui = applicationManager->getUI();
			FB_ASSERT(ui);

			m_renderWindow = ui->addElementByType<ui::IUIRenderWindow>();
		}

		//--------------------------------------------
		void MeshViewer::loadGraphicsResources()
		{
			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto graphicsSystem = applicationManager->getGraphicsSystem();
			FB_ASSERT(graphicsSystem);

			auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
			FB_ASSERT(resourceGroupManager);
			//resourceGroupManager->load(nullptr);
			graphicsSystem->loadObject(resourceGroupManager, true);
		}

		//--------------------------------------------
		void MeshViewer::setupRenderpipeline()
		{
			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto graphicsSystem = applicationManager->getGraphicsSystem();
			FB_ASSERT(graphicsSystem);

			//if (graphicsSystem)
			//{
			//	auto window = graphicsSystem->getDefaultWindow();
			//	if (!window)
			//	{
			//		window = m_renderWindow->getWindow();
			//	}

			//	auto renderSceneManager = graphicsSystem->getSceneManager();
			//	auto renderCamera = renderSceneManager->getCamera();
			//	graphicsSystem->setupRenderer(renderSceneManager, window, renderCamera, "", true);
			//}
		}

		//--------------------------------------------
		void MeshViewer::createGraphicsSceneManager()
		{
			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto graphicsSystem = applicationManager->getGraphicsSystem();
			FB_ASSERT(graphicsSystem);

			auto name = String("DefaultSceneManager");
			auto type = String("GameSceneManager");

			m_sceneManager = graphicsSystem->addSceneManager(name, type);
			FB_ASSERT(m_sceneManager);
		}

		//--------------------------------------------
		void MeshViewer::createCamera()
		{
			FB_ASSERT(m_sceneManager);
			FB_ASSERT(m_sceneManager->isValid());

			auto rootNode = m_sceneManager->getRootSceneNode();
			FB_ASSERT(rootNode);

			const auto mainCameraName = String("MainCamera");
			m_mainCamera = m_sceneManager->addCamera(mainCameraName);
			FB_ASSERT(m_mainCamera);

            m_mainCamera->setVisible(true);

			m_mainCameraSceneNode = rootNode->addChildSceneNode(mainCameraName);
			FB_ASSERT(m_mainCameraSceneNode);

			m_mainCameraSceneNode->attachObject(m_mainCamera);

			const auto cameraName = String("DefaultCamera");
			m_camera = m_sceneManager->addCamera(cameraName);
			FB_ASSERT(m_camera);

			m_cameraSceneNode = rootNode->addChildSceneNode(cameraName);
			FB_ASSERT(m_cameraSceneNode);

			m_cameraSceneNode->attachObject(m_camera);

			auto cameraPosition = Vector3<real_Num>::zero();
			cameraPosition += Vector3F::unitY() * 5.0;
			cameraPosition += Vector3F::unitZ() * 20.0;
			m_cameraSceneNode->setPosition(cameraPosition);

			m_camera->setNearClipDistance(0.01f);
			m_camera->setFarClipDistance(1000.0f);

            m_camera->setVisible(true);

			FB_ASSERT(m_sceneManager->isValid());
			FB_ASSERT(m_cameraSceneNode->isValid());
			FB_ASSERT(m_camera->isValid());
		}

		//--------------------------------------------
		void MeshViewer::createViewports()
		{
			auto applicationManager = core::IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto graphicsSystem = applicationManager->getGraphicsSystem();
			FB_ASSERT(graphicsSystem);

			auto window = graphicsSystem->getDefaultWindow();
			FB_ASSERT(window);

			auto mainVP = window->addViewport(0, m_mainCamera);
			FB_ASSERT(mainVP);

			mainVP->setClearEveryFrame(true);
			mainVP->setOverlaysEnabled(true);
			mainVP->setAutoUpdated(true);

			m_mainViewport = mainVP;

			FB_ASSERT(m_camera);
			m_camera->setAutoAspectRatio(true);

			auto vp = window->addViewport(0, m_camera);
			FB_ASSERT(vp);

			auto viewportColour = ColourF(0.75f, 0.0f, 0.0f, 1.0f);
			vp->setBackgroundColour(viewportColour);

			auto actualWidth = f32(vp->getActualWidth());
			auto actualHeight = f32(vp->getActualHeight());

			auto aspectRatio = 1.0f;
			if (actualHeight > 0.0f)
			{
				aspectRatio = actualWidth / actualHeight;
			}

			FB_ASSERT(MathF::isFinite(aspectRatio));
			FB_ASSERT(aspectRatio > 0.0f);

			m_camera->setAspectRatio(aspectRatio);
			FB_ASSERT(m_camera->isValid());

			vp->setClearEveryFrame(true);
			vp->setOverlaysEnabled(false);
			vp->setAutoUpdated(true);
			m_viewport = vp;

			FB_ASSERT(m_viewport);
			FB_ASSERT(m_viewport->isValid());
		}



		//--------------------------------------------
		void MeshViewer::createFileSystem()
		{
			try
			{
				auto applicationManager = core::IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto fileSystem = FBFileSystem::createFileSystem();
				applicationManager->setFileSystem(fileSystem);

				auto workingDirectory = Path::getWorkingDirectory();
				fileSystem->addFolder(workingDirectory);

				auto mediaFolderPath = String("");

#if defined FB_PLATFORM_WIN32
				mediaFolderPath = String("../../../../Media/");
#elif defined FB_PLATFORM_APPLE
				mediaFolderPath = String("../../Media/");
#else
				mediaFolderPath = String("../../Media/");
#endif

				auto packs = fileSystem->getFiles(mediaFolderPath + "/packs");
				for (auto& pack : packs)
				{
					fileSystem->addFileArchive(pack, true, true, IFileSystem::ArchiveType::Zip);
				}

				fileSystem->addFolder(mediaFolderPath, true);
				fileSystem->addFolder("./", true);

				applicationManager->setMediaPath(mediaFolderPath);
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//--------------------------------------------
		MeshViewer::CUIMenuBarListener::CUIMenuBarListener()
		{

		}



		//--------------------------------------------
		MeshViewer::CUIMenuBarListener::~CUIMenuBarListener()
		{

		}



		//--------------------------------------------
		void MeshViewer::CUIMenuBarListener::handleEvent(s32 id, SmartPtr<ISharedObject> data)
		{
			if (id == (s32)MeshViewer::ElementId::Open)
			{
				auto applicationManager = core::IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto ui = applicationManager->getUI();
				FB_ASSERT(ui);

				auto fileDialog = ui->addElementByType<ui::IUIFileBrowser>();
				fileDialog->setFileExtension(".fbx");
				
				auto listener = m_owner->m_fileBrowserListener;
				//listener->setFileBrowser(fileDialog);
				fileDialog->setListener(listener);
				fileDialog->show();
			}
			else if (id == (s32)MeshViewer::ElementId::Exit)
			{
				auto applicationManager = core::IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				applicationManager->setQuit(true);
			}
		}



		//--------------------------------------------
		void MeshViewer::CUIMenuBarListener::handleNoMenuItemsEnabled(SmartPtr<ui::IUIElement> item)
		{

		}



		//--------------------------------------------
		void MeshViewer::CUIMenuBarListener::handleNoMenuItemsVisible(SmartPtr<ui::IUIElement> item)
		{

		}



		//--------------------------------------------
		MeshViewer* MeshViewer::CUIMenuBarListener::getOwner() const
		{
			return m_owner;
		}



		//--------------------------------------------
		void MeshViewer::CUIMenuBarListener::setOwner(MeshViewer* val)
		{
			m_owner = val;
		}



		//--------------------------------------------
		void MeshViewer::CUIFileBrowserListener::handleSelection()
		{
			auto fileBrowser = getFileBrowser();
			if (fileBrowser)
			{
				auto filePath = fileBrowser->getFilePath();
				if (!StringUtil::isNullOrEmpty(filePath))
				{
					auto meshActor = ApplicationUtil::loadMesh(filePath);
					m_owner->setMeshActor(meshActor);
				}
			}
		}



		//--------------------------------------------
		void MeshViewer::CUIFileBrowserListener::handleCancel()
		{

		}



		//--------------------------------------------
		void MeshViewer::CUIFileBrowserListener::handleClose()
		{

		}


		//--------------------------------------------
		SmartPtr<ui::IUIFileBrowser> MeshViewer::CUIFileBrowserListener::getFileBrowser() const
		{
			return m_fileBrowser;
		}



		//--------------------------------------------
		void MeshViewer::CUIFileBrowserListener::setFileBrowser(SmartPtr<ui::IUIFileBrowser> fileBrowser)
		{
			m_fileBrowser = fileBrowser;
		}



		//--------------------------------------------
		MeshViewer* MeshViewer::CUIFileBrowserListener::getOwner() const
		{
			return m_owner;
		}



		//--------------------------------------------
		void MeshViewer::CUIFileBrowserListener::setOwner(MeshViewer* val)
		{
			m_owner = val;
		}



	} // end namespace viewer
} // end namespace fb



#ifdef FB_PLATFORM_WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	fb::viewer::MeshViewer app;
	app.load(nullptr);
	app.run();
	return 0;
}
#else
int main()
{
	fb::viewer::MeshViewer app;
	app.load(nullptr);
	app.run();
	return 0;
}
#endif

//
//int main(int argc, char* argv[])
//{
//	return 0;
//}
