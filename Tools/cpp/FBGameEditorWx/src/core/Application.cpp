#include <GameEditorPCH.hpp>
#include "Application.hpp"
#include <core/MessageManager.hpp>
#include <editor/EditorManager.hpp>
#include <editor/EditorMessages.hpp>
#include <jobs/JobRendererSetup.hpp>
#include <ui/ApplicationFrame.hpp>
#include <ui/UIManager.hpp>
#include <ui/RenderWindow.hpp>
#include <script/ScriptManager.hpp>
#include <FBApplication/Application/SimulatorApplication.hpp>
#include <FBApplication/Application/TruckSimulatorApplication.hpp>
#include <FBApplication/Actor/CScene.hpp>
#include <FBApplication/Actor/CSceneManager.hpp>
#include <FBApplication/Camera/CameraManager.hpp>
#include <FBApplication/Camera/EditorFpsCameraController.hpp>
#include <FBApplication/Camera/SphericalCameraController.hpp>
#include <FBApplication/Command/CommandManagerStandard.hpp>
#include <FBApplication/Selection/CSelectionManager.hpp>
#include <FBApplication/System/FactoryTemplate.hpp>
#include <FBFileSystem/FBFileSystem.hpp>
#include <FBCore/FBCoreHeaders.hpp>
#include <FBAnimation/FBAnimation.hpp>
#include <FBGraphics/FBGraphics.hpp>
#include <FBAssimp/FBAssimp.hpp>

#if FB_GRAPHICS_SYSTEM_OGRENEXT
#include <FBGraphicsOgreNext/FBGraphicsOgre.hpp>
#elif FB_GRAPHICS_SYSTEM_OGRE
#include <FBGraphicsOgre/FBGraphicsOgre.hpp>
#else
#include <FBGraphicsD3D11/FBGraphicsD3D11.hpp>
#endif

#include <FBRenderUI/FBRenderUI.hpp>
#include <FBCore/System/TaskListenerAdapter.hpp>
#include <FBCore/System/TimerMT.hpp>
#include <FBCore/System/Task.hpp>
#include <FBCore/System/StateObjectStandard.hpp>
#include <FBCore/System/CJobQueue.hpp>

#if FB_BUILD_PHYSX
#include <FBPhysx/FBPhysx.hpp>
#elif FB_BUILD_ODE
#include <FBODE3/CPhysicsManagerODE.hpp>
#endif

#include <FBParticleSystem/FBParticleSystem.hpp>
#include <FBCore/System/StateManagerTBB.hpp>
#include <FBApplication/FBApplication.hpp>
#include <FBProcedural/FBProcedural.hpp>
#include <editor/Project.hpp>
#include <editor/ProjectManager.hpp>
#include <editor/SceneViewManager.hpp>
#include <FBCore/System/ApplicationManagerMT.hpp>
#include <FBState/Messages/StateMessage.hpp>

#include <FBCore/Base/LogManager.hpp>
#include <FBApplication/Actor/CActor.hpp>
#include <FBApplication/Manipulators/TranslateManipulator.hpp>
#include <FBApplication/Manipulators/RotateManipulator.hpp>
#include <FBApplication/Manipulators/ScaleManipulator.hpp>
#include <editor/ComponentTemplateMgr.hpp>
#include <FBCore/FBCoreHeaders.hpp>
#include <GameEditorTypes.hpp>
#include <wx/memory.hpp>
#include <wx/evtloop.hpp>
#include <wx/msgdlg.hpp>
#include <FBRenderUI/FBRenderUI.hpp>
#include <FBVehicle/CVehicleManager.hpp>
#include <FBCPlusPlus/FBCPlusPlus.hpp>

#include <FBWxWidgets/EventLoop.hpp>

#include <FBApplication/Components/CarController.hpp>
#include <FBApplication/Components/Constraint.hpp>
#include <FBApplication/Components/CollisionBox.hpp>
#include <FBApplication/Components/MaterialComponent.hpp>
#include <FBApplication/Components/MeshComponent.hpp>
#include <FBApplication/Components/MeshRenderer.hpp>
#include <FBApplication/Components/Rigidbody.hpp>
#include <FBApplication/Components/WheelController.hpp>

#include <FBApplication/UI/Components/CanvasTransformManager.hpp>

#include <FBState/Messages/StateMessageVector3.hpp>
#include <FBState/Messages/StateMessageVector4.hpp>
#include <FBState/Messages/StateMessageUIntValue.hpp>
#include <FBState/Messages/StateMessageIntValue.hpp>
#include <FBState/Messages/StateMessageVisible.hpp>



wxIMPLEMENT_APP(fb::editor::Application);



namespace fb
{
	namespace editor
	{



        //--------------------------------------------
		Application::Application()
			: m_editorManager(nullptr),
			m_isRunning(true)
		{
		}



		//--------------------------------------------
		Application::~Application()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				if (m_frameStatistics)
				{
					m_frameStatistics->unload(0);
					m_frameStatistics = nullptr;
				}
	
				auto sceneManager = applicationManager->getSceneManager();
				if (sceneManager)
				{
					auto currentScene = sceneManager->getCurrentScene();
					if (currentScene)
					{
						currentScene->clear();
					}
				}
	
				auto editorManager = EditorManager::getSingletonPtr();
				if (editorManager)
				{
					editorManager->unload(0);
				}
	
				auto graphicsSystem = applicationManager->getGraphicsSystem();
				if (graphicsSystem)
				{
					graphicsSystem->unload(0);
				}
	
				applicationManager->unload(0);
				IApplicationManager::setInstance(nullptr);
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//--------------------------------------------
		bool Application::OnInit()
		{
			try
			{
				wxInitAllImageHandlers();

				auto applicationManager = fb::make_ptr<ApplicationManagerMT>();
				IApplicationManager::setInstance(applicationManager);

				applicationManager->setEditor(true);

				auto stateRoot = fb::make_ptr<CStateRoot>();
				IStateRoot::setInstance(stateRoot);

				auto resourceDatabase = fb::make_ptr<CResourceDatabase>();
				applicationManager->setResourceDatabase(resourceDatabase);

				auto meshManager = fb::make_ptr<CMeshManager>();
				applicationManager->setMeshManager(meshManager);

				auto timer = fb::make_ptr<TimerMT>();
				applicationManager->setTimer(timer);

				SmartPtr<ICommandManager> commandMgr(new CommandManagerStandard);
				applicationManager->setCommandManager(commandMgr);

				auto prefabManager = fb::make_ptr<CPrefabManager>();
				applicationManager->setPrefabManager(prefabManager);

				auto sceneManager = fb::make_ptr<CSceneManager>();
				applicationManager->setSceneManager(sceneManager);

				auto selectionManager = fb::make_ptr<CSelectionManager>();
				applicationManager->setSelectionManager(selectionManager);

				auto meshLoader = FBAssimp::createAssimpLoader();
				applicationManager->setMeshLoader(meshLoader);

				auto scene = fb::make_ptr<CScene>();
				scene->load(nullptr);
				scene->setName("Untitled");
				sceneManager->setCurrentScene(scene);

				if (!WxApplication::OnInit())
				{
					return false;
				}

				auto factoryManager = applicationManager->getFactoryManager();
				FB_ASSERT(factoryManager);

				auto stateManager = applicationManager->getStateManager();
				FB_ASSERT(stateManager);
				
				m_stateObject = stateManager->addStateObject();

				auto applicationStateListener = factoryManager->make_ptr<ApplicationStateListener>();
				applicationStateListener->setOwner(this);
				m_stateListener = applicationStateListener;
				m_stateObject->addStateListener(m_stateListener);

				//ilInit();
				//iluInit();

				//LibraryPtr applicationLib(new AppBaseLibrary);
				//applicationLib->initialise();

				//LibraryPtr hydraxLib(new HydraxLibrary);
				//hydraxLib->initialise();

				//LibraryPtr particleSystemLib(new ParticleSystemLibrary);
				//particleSystemLib->initialise();

				SmartPtr<IInputManager> inputMgr(new ui::WxInputManager);
				applicationManager->setInputManager(inputMgr);

				auto jobQueue = fb::make_ptr<CJobQueue>();
				applicationManager->setJobQueue(jobQueue);

				//FactoryPtr factory(new FactoryStandard);
				//m_engine->setFactory(factory);

				SmartPtr<ICameraManager> cameraMgr(new CameraManager);
				applicationManager->setCameraManager(cameraMgr);

				m_editorManager = new EditorManager;

				auto componentTemplateManager = ComponentTemplateMgrPtr(new ComponentTemplateMgr);
				m_editorManager->setComponentTemplateMgr(componentTemplateManager);
				//componentTemplateManager->loadComponents();

				m_editorManager->setMessageManager(MessageManagerPtr(new MessageManager));

				auto projectManager = ProjectManagerPtr(new ProjectManager);
				m_editorManager->setProjectManager(projectManager);

				m_editorManager->setProject(ProjectPtr(new Project));
				m_editorManager->setScriptManager(EditorScriptManagerPtr(new EditorScriptManager));
				m_editorManager->setSceneViewManager(SceneViewManagerPtr(new SceneViewManager));

				//RoadManagerPtr roadManager(new RoadManager);
				//m_appRoot->setRoadManager(roadManager);

				//TerrainManagerPtr terrainManager(new TerrainManager);
				//m_appRoot->setTerrainManager(terrainManager);

				//FoliageManagerPtr foliageManager(new FoliageManager);
				//m_appRoot->setFoliageManager(foliageManager);

				//LuaEditConfigPtr luaEditConfig(new LuaEditConfig);
				//luaEditConfig->load("./LuaConfig.xml");
				//m_appRoot->setLuaEditConfig(luaEditConfig);

				auto renderUI = ui::FBRenderUI::createUIManager();
				applicationManager->setRenderUI(renderUI);

				SmartPtr<UIManager> uiManager(new UIManager);
				m_editorManager->setGUIManager(uiManager);

				////create the gui manager
				//fb::SmartPtr<UIManager> overlayGuiManager(new CGUIManager);
				//m_engine->setGUIManager(overlayGuiManager);

				// create the main application window
				m_frame = new ApplicationFrame("FireBlade Editor");
				m_frame->Show(true);
				m_frame->Maximize(true);

				uiManager->setApplicationFrame(m_frame);

				auto messageManager = m_editorManager->getMessageManager();

				auto selectEntityTemplateMsg = SelectEntityTemplatePtr(new SelectEntityTemplate);
				messageManager->postMessage(selectEntityTemplateMsg);

				auto sceneManagerName = String("ViewSM");
				auto graphicsSystem = applicationManager->getGraphicsSystem();
				if (graphicsSystem)
				{
					auto smgr = graphicsSystem->getSceneManager(sceneManagerName);
				}

				auto inputManager = applicationManager->getInputManager();
				inputManager->addListener(SmartPtr<IInputListener>(new ApplicationInputListener(this)));

				//Array<String> scriptFileNames;
				//fileSystem->getFileNamesWithExtension(".material", scriptFileNames);
				//Set<String> uniqueScriptFileNames = CoreUtil::createSet(scriptFileNames);
				//graphicsSystem->parseScripts(uniqueScriptFileNames);

				//auto fpsCamera = factoryManager->createObject<AppFPSCameraCtrl>();
				//fpsCamera->setName("FPSCameraCtrl");
				//fpsCamera->setRotationSpeed(10.0f);
				//fpsCamera->setPosition((Vector3F::UNIT_Z * 100.f) + (Vector3F::UNIT_Y * 100.0f));
				//fpsCamera->setTargetPosition(Vector3F::ZERO);
				//cameraMgr->addCamera(fpsCamera);
				//cameraMgr->setCurrentCamera(fpsCamera);
				//m_fpsCamera = fpsCamera;

				m_sphericalCameraActor = fb::make_ptr<CActor>();

				auto sphericalCamera = factoryManager->createObject<component::SphericalCameraController>();
				//sphericalCamera->setName("SphericalCamera");
				cameraMgr->addCamera(m_sphericalCameraActor);
				cameraMgr->setCurrentCamera(m_sphericalCameraActor);
				sphericalCamera->setSphericalCoords(Vector3<real_Num>(5.0f, 0.0f, 2.0f));
				m_sphericalCamera = sphericalCamera;

				m_sphericalCameraActor->addComponentInstance(sphericalCamera);

				createSceneViews();

				//auto terrainSceneView = m_editorManager->getTerrainSceneView();
				//fpsCamera->addCamera(terrainSceneView->getCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_0));

				//auto meshSceneView = m_editorManager->getMeshSceneView();
				//sphericalCamera->addCamera(meshSceneView->getCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_0));

				//auto particleSceneView = m_editorManager->getParticleSceneView();
				//sphericalCamera->addCamera(particleSceneView->getCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_0));

				//SmartPtr<IInputListener> inputListener(new AppInputListener(this));
				//inputMgr->addInputListener(inputListener);

				// Need support for images
				wxImage::AddHandler(new wxPNGHandler());
				wxImage::AddHandler(new wxGIFHandler());
				wxImage::AddHandler(new wxJPEGHandler());

				//MapManagerPtr mapMgr(new MapManagerStandard);
				//m_engine->setMapManager(mapMgr);

				auto translateManipulator = factoryManager->make_ptr<TranslateManipulator>();
				m_editorManager->setTranslateManipulator(translateManipulator);

				auto rotateManipulator = factoryManager->make_ptr<RotateManipulator>();
				m_editorManager->setRotateManipulator(rotateManipulator);

				auto scaleManipulator = factoryManager->make_ptr<ScaleManipulator>();
				m_editorManager->setScaleManipulator(scaleManipulator);

				//
				// map test
				//
				SmartPtr<TerrainTemplate> terrainTemplate(new TerrainTemplate);
				//terrainTemplate

				//MapStandardPtr map(new EditorMap);
				//map->setSceneManagerName(sceneManagerName);
				//map->load("");
				//m_map = map;

				//auto renderWindow = uiManager->getRenderWindow();
				//if (renderWindow)
				//{
				//	auto camera = renderWindow->getCamera();
				//	if (camera)
				//	{
				//		sphericalCamera->addCamera(camera);
				//	}
				//}

				//resourceDatabase->importAssets();

				ApplicationUtil::createDefaultSky();
				ApplicationUtil::createDirectionalLight();
				ApplicationUtil::createDefaultCube();
				//ApplicationUtil::createDefaultTerrain();
				//ApplicationUtil::createDefaultPlane();
				//ApplicationUtil::createDefaultVehicle();
				ApplicationUtil::createProceduralTest();
				//ApplicationUtil::createOverlayPanelTest();

				//auto currentScene = sceneManager->getCurrentScene();
				//auto prefab = prefabManager->loadPrefab("car.prefab");
				//auto carActor = prefab->getActor();
				//if (carActor)
				//{
				//	auto groundPosition = Vector3F::unitY() * 5.0f;
				//	auto groundScale = Vector3F::unit() * 1.0f;

				//	auto localTransform = carActor->getLocalTransform();
				//	if (localTransform)
				//	{
				//		localTransform->setPosition(groundPosition);
				//		localTransform->setScale(groundScale);
				//	}

				//	carActor->updateTransform();
				//	currentScene->addActor(carActor);
				//	currentScene->registerAllUpdates(carActor);
				//}

				uiManager->rebuildSceneTree();

				if (graphicsSystem)
				{
					auto renderWindow = uiManager->getRenderWindow();
					auto window = renderWindow->getWindow();
					if (!window)
					{
						window = graphicsSystem->getDefaultWindow();
					}

					auto renderSceneManager = graphicsSystem->getSceneManager();
					auto renderCamera = renderSceneManager->getCamera();
					graphicsSystem->setupRenderer(renderSceneManager, window, renderCamera, "", true);
				}

				m_frameStatistics = factoryManager->make_ptr<CFrameStatistics>();
				m_frameStatistics->load(nullptr);

				if (sceneManager)
				{
					sceneManager->edit();
				}

				auto taskManager = applicationManager->getTaskManager();
				taskManager->setState(ITaskManager::State::FreeStep);

				return true;
			}
			catch (Exception& e)
			{
				wxMessageBox(e.what());
			}
			catch (std::exception& e)
			{
				wxMessageBox(e.what());
			}
			catch (...)
			{
				wxMessageBox("Unknown");
			}

			//m_engine->setRunning(false);
			return false;
		}



		//--------------------------------------------
		int Application::MainLoop()
		{
#if 0
			return wxApp::MainLoop();
#else
			auto currentThreadId = Thread::ThreadId::Primary;
			Thread::setCurrentThreadId(currentThreadId);

			auto task = Thread::Task::Primary;
			Thread::setCurrentTask(task);

			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto timer = applicationManager->getTimer();
			FB_ASSERT(timer);

			auto loop = new ui::EventLoop;
			//auto loop = new wxGUIEventLoop;
			wxEventLoopBase::SetActive(loop);

			while (applicationManager->isRunning())
			{
				try
				{
					Thread::setCurrentTask(task);

					//if (Pending()) // Unprocessed events in queue
					//{
					//	Dispatch(); // Dispatch next event in queue 
					//}

					timer->update();
					auto dt = timer->getTimeInterval();
					auto time = timer->getTime();

					auto sceneManager = applicationManager->getSceneManager();
					if (sceneManager)
					{
						sceneManager->update();
					}

					m_editorManager->update(time, dt);

					m_frame->update(CURRENT_TASK_ID, time, dt);

					auto stateManager = applicationManager->getStateManager();
					if (stateManager)
					{
						stateManager->update();
					}

					auto jobQueue = applicationManager->getJobQueue();
					if (jobQueue)
					{
						jobQueue->update();
					}

					auto taskManager = applicationManager->getTaskManager();
					if (taskManager)
					{
						taskManager->update();
					}
				}
				catch (std::exception& e)
				{
					wxMessageBox(e.what());
				}
			}
#endif

			return 0;
		}



		//--------------------------------------------------------
		void Application::update()
		{
			auto applicationManager = IApplicationManager::instance();
			auto factoryManager = applicationManager->getFactoryManager();
			auto timer = applicationManager->getTimer();

			auto task = Thread::getCurrentTask();
			auto t = timer->getTime();
			auto dt = timer->getDeltaTime();

			auto editorManager = EditorManager::getSingletonPtr();

			auto sceneManager = applicationManager->getSceneManager();
			if (sceneManager)
			{
				sceneManager->preUpdate();
				sceneManager->update();
				sceneManager->postUpdate();
			}

			if (m_frameStatistics)
			{
				m_frameStatistics->update();
			}

			switch (task)
			{
			case Thread::Task::Primary:
			{
			}
			break;
			case Thread::Task::Render:
			{
				try
				{
					auto graphicsSystem = applicationManager->getGraphicsSystem();
	
					//Pump messages in all registered RenderWindow windows
					//graphicsSystem->messagePump();
	
					auto taskManager = applicationManager->getTaskManager();
					taskManager->update();
	
					static const auto updateHash = StringUtil::getHash("update");
	
					auto event = factoryManager->make_ptr<FrameEventStandard>();
					event->setTaskId((u32)task);
					event->setTime(t);
					event->setTimeInterval(dt);
					event->setType(updateHash);
					applicationManager->triggerFrameEvent(event);
	
					auto stateManager = applicationManager->getStateManager();
					if (stateManager)
					{
						stateManager->update();
					}
	
					auto cameraManager = applicationManager->getCameraManager();
					if (cameraManager)
					{
						cameraManager->update();
					}
	
					auto water = editorManager->getWater();
					if (water)
					{
						//water->update(CURRENT_TASK_ID, t, dt);
					}
	
					if (graphicsSystem)
					{
						graphicsSystem->update();
					}
	
					//auto renderer = graphicsSystem->getRenderer();
					//if (renderer)
					//{
					//	renderer->swapAllRenderTargetBuffers();
					//}
	
					auto window = graphicsSystem->getDefaultWindow();
					if (window)
					{
						window->update();
					}
	
					m_editorManager->update(t, dt);
				}
				catch (std::exception& e)
				{
					wxMessageBox(e.what());
				}
			}
			break;
			case Thread::Task::Physics:
			{
				auto stateManager = applicationManager->getStateManager();
				if (stateManager)
				{
					stateManager->update();
				}

				auto vehicleManager = applicationManager->getVehicleManager();
				if (vehicleManager)
				{
					vehicleManager->preUpdate();
					vehicleManager->update();
					vehicleManager->postUpdate();
				}

				if (applicationManager->isPlaying())
				{
					auto physicsManager = applicationManager->getPhysicsManager3();
					if (physicsManager)
					{
						physicsManager->preUpdate();
						physicsManager->update();
						physicsManager->postUpdate();
					}

					auto physicsScene = applicationManager->getPhysicsScene();
					if (physicsScene)
					{
						physicsScene->update();
					}
				}

				editorManager->update(t, dt);
			}
			break;
			case Thread::Task::Application:
			{
				auto stateManager = applicationManager->getStateManager();
				if (stateManager)
				{
					stateManager->update();
				}

				auto inputManager = applicationManager->getInputManager();
				if (inputManager)
				{
					inputManager->update();
				}

				auto cameraManager = applicationManager->getCameraManager();
				if (cameraManager)
				{
					cameraManager->update();
				}

				auto sceneManager = applicationManager->getSceneManager();
				if (sceneManager)
				{
					sceneManager->preUpdate();
					sceneManager->update();
					sceneManager->postUpdate();
				}

				auto translateManipulator = editorManager->getTranslateManipulator();
				if (translateManipulator)
				{
					translateManipulator->preUpdate();
					translateManipulator->update();
					translateManipulator->postUpdate();
				}

				auto rotateManipulator = editorManager->getRotateManipulator();
				if (rotateManipulator)
				{
					rotateManipulator->preUpdate();
					rotateManipulator->update();
					rotateManipulator->postUpdate();
				}

				m_editorManager->update(t, dt);
			}
			break;
			default:
			{
				auto stateManager = applicationManager->getStateManager();
				if (stateManager)
				{
					stateManager->update();
				}

				m_editorManager->update(t, dt);
			}
			};
		}



		//--------------------------------------------------------
		bool Application::inputEvent(SmartPtr<IInputEvent> event)
		{
			auto editorManager = EditorManager::getSingletonPtr();

			auto translateManipulator = editorManager->getTranslateManipulator();
			if (translateManipulator)
			{
				if (translateManipulator->OnEvent(event))
				{
					return true;
				}
			}

			auto rotateManipulator = editorManager->getRotateManipulator();
			if (rotateManipulator)
			{
				if (rotateManipulator->OnEvent(event))
				{
					return true;
				}
			}

			if (m_sphericalCamera)
			{
				if (m_sphericalCamera->handleEvent(event))
				{
					return true;
				}
			}

			return false;
		}


		//--------------------------------------------
		SmartPtr<ui::IUIMenubar> Application::getMenubar() const
		{
			throw std::logic_error("The method or operation is not implemented.");
		}
		//--------------------------------------------
		void Application::setMenubar(SmartPtr<ui::IUIMenubar> val)
		{
			throw std::logic_error("The method or operation is not implemented.");
		}
		//--------------------------------------------
		Vector2I Application::getWindowSize() const
		{
			throw std::logic_error("The method or operation is not implemented.");
		}
		//--------------------------------------------
		void Application::setWindowSize(const Vector2I& size)
		{
			throw std::logic_error("The method or operation is not implemented.");
		}
		//--------------------------------------------
		int Application::FilterEvent(wxEvent& event)
		{
			throw std::logic_error("The method or operation is not implemented.");
		}



		//--------------------------------------------
		void Application::ExitMainLoop()
		{
			m_isRunning = false;
		}



		//--------------------------------------------
		void Application::createLogManager()
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto logManager = fb::make_ptr<LogManager>();
			applicationManager->setLogManager(logManager);
			logManager->open("Editor.log");
		}



		//--------------------------------------------
		void Application::createFactoryManager()
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto factoryManager = fb::make_ptr<FactoryManager>();
			applicationManager->setFactoryManager(factoryManager);
			FB_ASSERT(applicationManager->getFactoryManager());

			try
			{
				auto typeManager = TypeManager::instance();

				factoryManager->addFactory(
					new FactoryTemplate<CActor>(
						typeManager->getName(CActor::typeInfo()),
						typeManager->getHash(CActor::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::SimulatorApplication>(
						typeManager->getName(component::SimulatorApplication::typeInfo()),
						typeManager->getHash(component::SimulatorApplication::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::TruckSimulatorApplication>(
						typeManager->getName(component::TruckSimulatorApplication::typeInfo()),
						typeManager->getHash(component::TruckSimulatorApplication::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::CanvasComponent>(
						typeManager->getName(component::CanvasComponent::typeInfo()),
						typeManager->getHash(component::CanvasComponent::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::CanvasTransform>(
						typeManager->getName(component::CanvasTransform::typeInfo()),
						typeManager->getHash(component::CanvasTransform::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::ButtonComponent>(
						typeManager->getName(component::ButtonComponent::typeInfo()),
						typeManager->getHash(component::ButtonComponent::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::ImageComponent>(
						typeManager->getName(component::ImageComponent::typeInfo()),
						typeManager->getHash(component::ImageComponent::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::TextComponent>(
						typeManager->getName(component::TextComponent::typeInfo()),
						typeManager->getHash(component::TextComponent::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::CarController>(
						typeManager->getName(component::CarController::typeInfo()),
						typeManager->getHash(component::CarController::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::CollisionBox>(
						typeManager->getName(component::CollisionBox::typeInfo()),
						typeManager->getHash(component::CollisionBox::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::CollisionMesh>(
						typeManager->getName(component::CollisionMesh::typeInfo()),
						typeManager->getHash(component::CollisionMesh::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::Rigidbody>(
						typeManager->getName(component::Rigidbody::typeInfo()),
						typeManager->getHash(component::Rigidbody::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::CarController>(
						typeManager->getName(component::CarController::typeInfo()),
						typeManager->getHash(component::CarController::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::Constraint>(
						typeManager->getName(component::Constraint::typeInfo()),
						typeManager->getHash(component::Constraint::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::MaterialComponent>(
						typeManager->getName(component::MaterialComponent::typeInfo()),
						typeManager->getHash(component::MaterialComponent::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::MeshComponent>(
						typeManager->getName(component::MeshComponent::typeInfo()),
						typeManager->getHash(component::MeshComponent::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::MeshRenderer>(
						typeManager->getName(component::MeshRenderer::typeInfo()),
						typeManager->getHash(component::MeshRenderer::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<component::WheelController>(
						typeManager->getName(component::WheelController::typeInfo()),
						typeManager->getHash(component::WheelController::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<procedural::CRoad>(
						typeManager->getName(procedural::CRoad::typeInfo()),
						typeManager->getHash(procedural::CRoad::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<procedural::CRoadNode>(
						typeManager->getName(procedural::CRoadNode::typeInfo()),
						typeManager->getHash(procedural::CRoadNode::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<StateMessageVector3>(
						typeManager->getName(StateMessageVector3::typeInfo()),
						typeManager->getHash(StateMessageVector3::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<StateMessageVector4>(
						typeManager->getName(StateMessageVector4::typeInfo()),
						typeManager->getHash(StateMessageVector4::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<StateMessageUIntValue>(
						typeManager->getName(StateMessageUIntValue::typeInfo()),
						typeManager->getHash(StateMessageUIntValue::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<StateMessageIntValue>(
						typeManager->getName(StateMessageIntValue::typeInfo()),
						typeManager->getHash(StateMessageIntValue::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<StateMessageVisible>(
						typeManager->getName(StateMessageVisible::typeInfo()),
						typeManager->getHash(StateMessageVisible::typeInfo())));

				factoryManager->addFactory(
					new FactoryTemplate<FrameEventStandard>(
						typeManager->getName(FrameEventStandard::typeInfo()),
						typeManager->getHash(FrameEventStandard::typeInfo())));

				factoryManager->setPoolSizeByType<CActor>(128);
				factoryManager->setPoolSizeByType<procedural::CRoad>(1024);
				factoryManager->setPoolSizeByType<procedural::CRoadElement>(1024);
				factoryManager->setPoolSizeByType<procedural::CRoadSection>(1024);
				factoryManager->setPoolSizeByType<procedural::CRoadNode>(1024);
				factoryManager->setPoolSizeByType<FrameEventStandard>(8);
				factoryManager->setPoolSizeByType<StateMessageVector3>(128);
				factoryManager->setPoolSizeByType<StateMessageVector4>(128);
				factoryManager->setPoolSizeByType<StateMessageUIntValue>(128);
				factoryManager->setPoolSizeByType<StateMessageIntValue>(128);
				factoryManager->setPoolSizeByType<StateMessageVisible>(128);
			}
			catch (Exception& e)
			{
				wxMessageBox(e.what());
			}
		}



		//--------------------------------------------
		void Application::createTimer()
		{
			auto applicationManager = IApplicationManager::instance();

			auto timer = fb::make_ptr<TimerMT>();
			applicationManager->setTimer(timer);
		}



		//--------------------------------------------
		void Application::createTaskManager()
		{
			auto applicationManager = IApplicationManager::instance();

			auto taskManager = fb::make_ptr<TaskManager>();
			applicationManager->setTaskManager(taskManager);
		}



		//--------------------------------------------
		void Application::createThreadPool()
		{
			auto applicationManager = IApplicationManager::instance();

			auto threadPool = fb::make_ptr<ThreadPool>();
			applicationManager->setThreadPool(threadPool);
		}



		//--------------------------------------------
		void Application::createStateManager()
		{
			auto applicationManager = IApplicationManager::instance();

			auto stateManager = fb::make_ptr<StateManagerStandard>();
			applicationManager->setStateManager(stateManager);
		}



		//--------------------------------------------
		void Application::createFileSystem()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();

				auto fileSystem = FBFileSystem::createFileSystem();
				applicationManager->setFileSystem(fileSystem);

				auto workingDirectory = Path::getWorkingDirectory();
				fileSystem->addFolder(workingDirectory);

				auto cachePath = applicationManager->getCachePath();
				auto cacheAbsolutePath = Path::getAbsolutePath(workingDirectory, cachePath);
				fileSystem->addFolder(cacheAbsolutePath);

				auto settingsCachePath = applicationManager->getSettingsCachePath();
				auto settingsCacheAbsolutePath = Path::getAbsolutePath(workingDirectory, settingsCachePath);
				fileSystem->addFolder(settingsCacheAbsolutePath);

				//add archives
				Array<String> fileNames;
				fileSystem->getFileNamesInFolder(String("./"), fileNames);
				for (u32 i = 0; i < fileNames.size(); ++i)
				{
					const auto& fileName = fileNames[i];
					auto ext = Path::getFileExtension(fileName);
					if (ext == ".fba")
					{
						fileSystem->addFileArchive(fileName, true, true, IFileSystem::ArchiveType::ObfuscatedZip, "");
					}
				}

                auto mediaPath = String("");

#if defined FB_PLATFORM_WIN32
                mediaPath = String("../../../../Media/");
#elif defined FB_PLATFORM_APPLE
                mediaPath = String("../../Media/");
#else
                mediaPath = String("../../Media/");
#endif

				applicationManager->setMediaPath(mediaPath);

				fileSystem->getFileNamesInFolder(mediaPath, fileNames);
				for (u32 i = 0; i < fileNames.size(); ++i)
				{
					const auto& fileName = fileNames[i];
					auto ext = Path::getFileExtension(fileName);
					if (ext == ".fba")
					{
						fileSystem->addFileArchive(String("../Media/") + fileName, true, true, IFileSystem::ArchiveType::ObfuscatedZip, "");
					}
				}

				auto packs = fileSystem->getFiles(mediaPath + "packs");
				for (auto& pack : packs)
				{
					fileSystem->addFileArchive(pack, true, true, IFileSystem::ArchiveType::Zip);
				}


				fileSystem->addFolder(mediaPath, true);

				/*
				fileSystem->addFolder(mediaPath + "/2.0", true);
				fileSystem->addFolder(mediaPath + "/gui", true);
				fileSystem->addFolder(mediaPath + "/materials", true);
				fileSystem->addFolder(mediaPath + "/Utility", true);
				 */

				// test
				//fileSystem->open("Quad_vs.metal");

				fileSystem->addFileArchive(".FBCache", true, true, IFileSystem::ArchiveType::Folder);
				fileSystem->addFileArchive("./", true, true, IFileSystem::ArchiveType::Folder);
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//--------------------------------------------
		bool Application::createGraphicsSystem()
		{
#if FB_GRAPHICS_SYSTEM_OGRENEXT
			auto graphicsSystem = render::FBGraphicsOgre::createGraphicsOgre();
#elif FB_GRAPHICS_SYSTEM_OGRE
			auto graphicsSystem = render::FBGraphicsOgre::createGraphicsOgre();
#else
			auto graphicsSystem = createGraphicsSystemD3D11();
#endif

#if FB_GRAPHICS_SYSTEM_OGRENEXT
			auto applicationManager = IApplicationManager::instance();
			applicationManager->setGraphicsSystem(graphicsSystem);

			graphicsSystem->load(nullptr);

			auto configuration = graphicsSystem->createConfiguration();
			FB_ASSERT(configuration);

			configuration->setCreateWindow(false);
			graphicsSystem->configure(configuration);

			auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
			graphicsSystem->loadObject(resourceGroupManager);
#elif FB_GRAPHICS_SYSTEM_OGRE
			auto applicationManager = IApplicationManager::instance();
			applicationManager->setGraphicsSystem(graphicsSystem);

			auto task = Thread::getCurrentTask();
			auto renderTask = graphicsSystem->getRenderTask();
			if (task == renderTask)
			{
				graphicsSystem->load(nullptr);

				auto configuration = graphicsSystem->createConfiguration();
				configuration->setCreateWindow(false);
				graphicsSystem->configure(configuration);
			}
			else
			{
				createRenderInitJob();
			}
#endif

			return true;
		}



		//--------------------------------------------
		void Application::createRenderInitJob()
		{
			auto applicationManager = IApplicationManager::instance();
			auto taskManager = applicationManager->getTaskManager();
			auto jobQueue = applicationManager->getJobQueue();

			auto jobRendererSetup = fb::make_ptr<JobRendererSetup>();

			auto renderTask = taskManager->getTask(Thread::Task::Render);
			if (renderTask)
			{
				renderTask->queueJob(jobRendererSetup);
			}
			else
			{
				jobQueue->queueJob(jobRendererSetup);
			}
		}



		//--------------------------------------------
		void Application::createCoreComponents()
		{
			//WxApplication::createCoreComponents();

			//fb::MessageManagerPtr messageManager = m_engine->getPlatformManager()->createMessageManager();
			//m_engine->setMessageManager(messageManager);
		}



		//--------------------------------------------
		void Application::createPhysics()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();

				auto properties = fb::make_ptr<Properties>();

#if FB_BUILD_PHYSX
				auto physicsManager = physics::createPhysxManager(properties);
				applicationManager->setPhysicsManager3(physicsManager);
#elif FB_BUILD_ODE
                auto physicsManager = fb::make_ptr<physics::CPhysicsManagerODE>();
                applicationManager->setPhysicsManager3(physicsManager);
#endif

				auto physicsScene = physicsManager->createScene();
				applicationManager->setPhysicsScene(physicsScene);

				auto vehicleManager = fb::make_ptr<CVehicleManager>();
				applicationManager->setVehicleManager(vehicleManager);
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
				wxMessageBox(e.what());
			}
		}



		//--------------------------------------------
		bool Application::createScriptManager()
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto scriptManager = SmartPtr<IScriptManager>();

			scriptManager = createScriptManagerCPP();
			FB_ASSERT(scriptManager);

			applicationManager->setScriptManager(scriptManager);

			return scriptManager != nullptr;
		}



		//--------------------------------------------
		void Application::chooseSceneManager()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				auto graphicsSystem = applicationManager->getGraphicsSystem();

				auto smgr = graphicsSystem->addSceneManager("DefaultSceneManager", "ViewSM");
				smgr->setAmbientLight(ColourF::White);

				//SmartPtr<IWindow> window = graphicsSystem->getDefaultWindow();
				//if ( window )
				//{
				//	SmartPtr<ICamera> camera = smgr->addCamera("DefaultWindowCamera");
				//	SmartPtr<ISceneNode> cameraNode = smgr->getRootSceneNode()->addChildSceneNode();
				//	cameraNode->attachObject(camera);
				//	cameraNode->setPosition(Vector3F::UNIT_Z * 250.0f);
				//	SmartPtr<IViewport> viewport = window->addViewport(0, camera);
				//	viewport->setBackgroundColour(ColourF(0.02, 0.85, 0.75, 1.0));
				//}

				//SmartPtr<render::ISceneManager> particleSceneManager = graphicsSystem->addSceneManager("DefaultSceneManager", "ParticleSceneManager");
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
				wxMessageBox(e.what());
			}
		}



		//--------------------------------------------
		void Application::createTasks()
		{
			try
			{
				auto applicationManager = IApplicationManager::instance();
				FB_ASSERT(applicationManager);

				auto factoryManager = applicationManager->getFactoryManager();
				FB_ASSERT(factoryManager);

				auto taskManager = applicationManager->getTaskManager();
				auto threadPool = applicationManager->getThreadPool();

				FB_ASSERT(taskManager);
				FB_ASSERT(threadPool);

				auto updateFrequency = 1.0 / 800.0;
				auto numThreads = Thread::hardware_concurrency();
				numThreads = 0; // todo check thread safety
				
				if (threadPool)
				{
					m_workerThreads.reserve(numThreads);
					//for (s32 threadIdx = 0; threadIdx < numThreads; ++threadIdx)
					//{
					//	auto workerThread = factoryManager->make_ptr<CWorkerThread>();
					//	workerThread->setTargetFPS(800.0);
					//	threadPool->addWorkerThread(workerThread);
					//	m_workerThreads.push_back(workerThread);
					//}
				}

				//m_appLogicTask = SmartPtr<ITask>(
				//		new TaskStandard(Thread::Task::TASK_ID_APPLICATION, updateFrequency, taskManager));
				//taskManager->addTask(m_appLogicTask);

				//m_gfxTask = SmartPtr<ITask>(
				//		new TaskStandard(Thread::Task::TASK_ID_RENDER, updateFrequency, taskManager));
				//taskManager->addTask(m_gfxTask);
				//m_gfxTask->setPrimary(true);

				//m_physicsTask = SmartPtr<ITask>(
				//		new TaskStandard(Thread::Task::TASK_ID_PHYSICS, updateFrequency, taskManager));
				//taskManager->addTask(m_physicsTask);

				//m_taskGraphics = new TaskListenerAdapter<Application>(this, (u32)Thread::Task::TASK_ID_RENDER);
				//m_gfxTask->addListener(m_taskGraphics);

				//m_taskApplication = new TaskListenerAdapter<Application>(this, (u32)Thread::Task::TASK_ID_APPLICATION);
				//m_appLogicTask->addListener(m_taskApplication);

				//m_taskPhysics = new TaskListenerAdapter<Application>(this, (u32)Thread::Task::TASK_ID_PHYSICS);
				//m_physicsTask->addListener(m_taskPhysics);

				//if (m_workerThreads.size() >= 1)
				//{
				//	m_workerThreads[0]->setReserveFlag((u32)Thread::Task::TASK_ID_RENDER, true);
				//}

				//if (m_workerThreads.size() >= 2)
				//{
				//	m_workerThreads[1]->setReserveFlag((u32)Thread::Task::TASK_ID_APPLICATION, true);
				//}

				//if (m_workerThreads.size() >= 3)
				//{
				//	m_workerThreads[1]->setReserveFlag((u32)Thread::Task::TASK_ID_PHYSICS, true);
				//}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//--------------------------------------------
		void Application::createSceneViews()
		{
			// setup some scene views
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			auto applicationManager = IApplicationManager::instance();
			//SmartPtr<IGraphicsSystem> graphicsSystem = engine->getGraphicsSystem();

			//SmartPtr<render::ISceneManager> terrainSmgr = graphicsSystem->addSceneManager("DefaultSceneManager", "TerrainView");
			//SmartPtr<ISceneView> terrainSceneView(new SceneView);
			//terrainSceneView->setSceneManager(terrainSmgr);
			//terrainSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_0);
			//terrainSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_1);
			//terrainSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_2);
			//terrainSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_3);
			//editorManager->setTerrainSceneView(terrainSceneView);

			//SmartPtr<render::ISceneManager> meshSmgr = graphicsSystem->addSceneManager("DefaultSceneManager", "MeshView");
			//SmartPtr<ISceneView> meshSceneView(new SceneView);
			//meshSceneView->setSceneManager(meshSmgr);
			//meshSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_0);
			//meshSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_1);
			//meshSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_2);
			//meshSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_3);
			//editorManager->setMeshSceneView(meshSceneView);

			//SmartPtr<render::ISceneManager> particleSmgr = graphicsSystem->getSceneManager("ParticleSceneManager");
			//SmartPtr<ISceneView> particleSceneView(new SceneView);
			//particleSceneView->setSceneManager(particleSmgr);
			//particleSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_0);
			//particleSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_1);
			//particleSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_2);
			//particleSceneView->createViewCamera(GameEditorTypes::CAMERA_TYPE_VIEWPORT_3);
			//editorManager->setParticleSceneView(particleSceneView);
		}



		//--------------------------------------------
		Application::ApplicationInputListener::ApplicationInputListener(Application* app)
			: m_application(app)
		{
		}



		//--------------------------------------------
		Application::ApplicationInputListener::~ApplicationInputListener()
		{
		}



		//--------------------------------------------
		bool Application::ApplicationInputListener::inputEvent(SmartPtr<IInputEvent> event)
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto graphicsSystem = applicationManager->getGraphicsSystem();
			FB_ASSERT(graphicsSystem);

			auto smgr = graphicsSystem->getSceneManager("ViewSM");
			FB_ASSERT(smgr);

			if (m_application->inputEvent(event))
			{
				return true;
			}

			switch (event->getEventType())
			{
			case IInputEvent::EventType::Mouse:
			{
				const SmartPtr<IMouseState>& mouseState = event->getMouseState();
				if (mouseState)
				{
					auto window = static_cast<render::IWindow*>(event->getWindow());
					if (!window)
					{
						return false;
					}

					Vector2F windowSize = window->getSize();
					Vector2F mousePosition = mouseState->getAbsolutePosition();
					Vector2F mouseCoords = mousePosition / windowSize;


					if (mouseState->getEventType() == IMouseState::Event::LeftPressed)
					{
						//RoadManagerPtr roadManager = applicationManager->getRoadManager();

						//if ( roadManager )
						//{
						//	if ( roadManager->getCreateRoadNode() )
						//	{
						//		//SmartPtr<ISceneView> meshSceneView = applicationManager->getMeshSceneView();
						//		//SmartPtr<render::ISceneManager> smgr = meshSceneView->getSceneManager();

						//		// create road node
						//		SmartPtr<ISceneNode> boxNode;
						//		GraphicsMeshPtr boxMesh;

						//		boxNode = smgr->getRootSceneNode()->addChildSceneNode();
						//		boxMesh = smgr->addMesh("4x4chassis.mesh");
						//		boxMesh->setMaterialName("Box/SphereMappedRustySteel");
						//		boxNode->attachObject(boxMesh);
						//		boxNode->setPosition(Vector3F(0, 0, 0));
						//		boxNode->setScale(Vector3F::UNIT * 2.0f);

						//		m_app->boxNode = boxNode;
						//		m_app->boxMesh = boxMesh;

						//		SphericalCameraPtr sphericalCamera = m_app->m_sphericalCamera;
						//		Array<SmartPtr<ICamera>> cameras = sphericalCamera->getCameras();
						//		for ( u32 i = 0; i < cameras.size(); ++i )
						//		{
						//			SmartPtr<ICamera> camera = cameras[i];

						//			Ray3F ray = camera->getRay(mouseCoords.X(), mouseCoords.Y());

						//			Vector3F intersectionPoint = Vector3F::ZERO;

						//			Plane3F groundPlane(Vector3F::ZERO, Vector3F::UNIT_Y);
						//			if ( groundPlane.getIntersectionWithLine(ray.getOrigin(), ray.getDirection(), intersectionPoint) )
						//			{
						//				boxNode->setPosition(intersectionPoint);
						//			}
						//		}
						//	}
						//}
					}



					auto viewWindow = static_cast<ui::wxViewWindow*>(event->getUserData());
					auto camera = viewWindow->getCamera();
					if (!camera)
					{
						return false;
					}

					Ray3F ray = camera->getRay(mouseCoords.X(), mouseCoords.Y());

					if (mouseState->getEventType() == IMouseState::Event::LeftPressed)
					{
						//SmartPtr<ISceneView> meshSceneView = applicationManager->getMeshSceneView();

						//Vector3F meshHitPosition;

						//SmartPtr<render::ISceneManager> sceneManager = meshSceneView->getSceneManager();
						//if (sceneManager->castRay(ray, meshHitPosition))
						//{
						//	m_application->m_sphericalCamera->setTargetPosition(meshHitPosition);
						//}
					}

					//						SmartPtr<ITerrain> terrain = applicationManager->getTerrain();
					//						if ( terrain && terrain->isLoaded() )
					//						{
					//							TerrainRayResultPtr result = terrain->intersects(ray);
					//							if ( result && result->hasIntersected() )
					//							{
					//								Vector3F hitPosition = result->getPosition();
					//
					//								SmartPtr<IDecalCursor> decalCursor = applicationManager->getDecalCursor();
					//								if ( decalCursor )
					//								{
					//									decalCursor->setPosition(hitPosition);
					//								}
					//
					//								if(mouseState->getEventType() == IMouseState::MOUSE_EVENT_LEFT_BUTTON_PRESSED)
					//								{
					//									if(applicationManager->getEditFoliage())
					//									{
					//#if 0
					//										auto applicationManager = IApplicationManager::instance();
					//										SmartPtr<IGraphicsSystem> graphicsSystem = engine->getGraphicsSystem();
					//										SmartPtr<render::ISceneManager> smgr = graphicsSystem->getSceneManager("ViewSM");
					//
					//										GraphicsMeshPtr treeMesh = smgr->addMesh("Palm.mesh");
					//										SmartPtr<ISceneNode> sceneNode = smgr->getRootSceneNode()->addChildSceneNode();
					//										sceneNode->attachObject(treeMesh);
					//										sceneNode->setPosition(hitPosition);
					//
					//										QuaternionF orientation;
					//										orientation.fromAngleAxis(MathF::DegToRad(-90.0f), Vector3F::UNIT_X);
					//										sceneNode->setOrientation(orientation);
					//#else 
					//
					//										ApplicationManager* editorManager = IApplicationManager::instance();
					//										FoliageManagerPtr foliageMgr = editorManager->getFoliageManager();
					//
					//										Vector3F terrrainSpacePosition = terrain->getTerrainSpacePosition(result->getPosition());
					//										foliageMgr->add(terrrainSpacePosition);
					//
					//#endif
					//								}
					//									else
					//									{
					//										Vector3F terrrainSpacePosition = terrain->getTerrainSpacePosition(result->getPosition());
					//										TerrainBlendMapPtr blendMap = terrain->getBlendMap(1);
					//										if ( blendMap )
					//										{
					//											f32 blendMapSize = terrain->getLayerBlendMapSize();
					//											u32 brushSize = 64;
					//											Vector3F blendMapCoords(terrrainSpacePosition.X() * blendMapSize, terrrainSpacePosition.Y() * blendMapSize, 0.0f);
					//
					//											Vector2I minCoord(blendMapCoords.X() - brushSize, blendMapCoords.Y() - brushSize);
					//											Vector2I maxCoord(blendMapCoords.X() + brushSize, blendMapCoords.Y() + brushSize);
					//
					//											for ( u32 y = minCoord.Y(); y < maxCoord.Y(); ++y )
					//											{
					//												for ( u32 x = minCoord.X(); x < maxCoord.X(); ++x )
					//												{
					//													blendMap->setBlendValue(x, y, MathF::RangedRandom(0.0, 1.0));
					//												}
					//											}
					//
					//											blendMap->updateModifications();
					//										}
					//									}
					//							}
					//								else if ( mouseState->getEventType() == IMouseState::MOUSE_EVENT_RIGHT_BUTTON_PRESSED )
					//								{
					//
					//								}
					//						}
					//					}
				}
			}
			break;
			default:
			{
			}
			};

			

			return false;
		}



		//--------------------------------------------
		bool Application::ApplicationInputListener::updateEvent(const SmartPtr<IInputEvent>& event)
		{
			return false;
		}



		//--------------------------------------------
		void Application::ApplicationInputListener::setPriority(s32 priority)
		{

		}



		//--------------------------------------------
		s32 Application::ApplicationInputListener::getPriority() const
		{
			return 0;
		}



		Application::ApplicationStateListener::~ApplicationStateListener()
		{
			setOwner(nullptr);
		}

		void Application::ApplicationStateListener::handleStateChanged(SmartPtr<IState>& state)
		{

		}

		void Application::ApplicationStateListener::handleStateChanged(const SmartPtr<IStateMessage>& message)
		{
			//if (message->isExactly<StateMessage>())
			//{
			//	static const auto RENDERER_READY_HASH = StringUtil::getHash("RendererReady");

			//	auto type = message->getType();
			//	if (type == RENDERER_READY_HASH)
			//	{
			//		auto editorManager = EditorManager::getSingletonPtr();
			//		auto uiManager = editorManager->getUI();
			//		auto renderWindow = uiManager->getRenderWindow();
			//		renderWindow->setRenderer();

			//		auto applicationManager = IApplicationManager::instance();
			//		auto graphicsSystem = applicationManager->getGraphicsSystem();
			//		graphicsSystem->loadResources();
			//	}
			//}
		}

		void Application::ApplicationStateListener::handleQuery(SmartPtr<IStateQuery>& query)
		{

		}

		Application* Application::ApplicationStateListener::getOwner() const
		{
			return m_owner;
		}

		void Application::ApplicationStateListener::setOwner(Application* val)
		{
			m_owner = val;
		}

	} // end namespace editor
} // end namespace fb


