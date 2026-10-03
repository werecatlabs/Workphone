#include <GameEditorPCH.hpp>
#include "EditorManager.hpp"
//#include "ui/LuaEditConfig.hpp"
#include "ui/RenderWindow.hpp"
#include "ui/ProjectWindow.hpp"
#include "script/ScriptManager.hpp"
#include "editor/ComponentTemplateMgr.hpp"
#include "editor/Project.hpp"
#include "editor/SceneViewManager.hpp"
//#include "terrain/FoliageManager.hpp"
//#include "terrain/TerrainManager.hpp"
//#include "terrain/RoadManager.hpp"
//#include "terrain/RiverManager.hpp"
#include <ui/UIManager.hpp>
#include <core/MessageManager.hpp>
#include "editor/ProjectManager.hpp"
#include "GameEditorTypes.hpp"

#include <FBApplication/Manipulators/TranslateManipulator.hpp>
#include <FBApplication/Manipulators/RotateManipulator.hpp>
#include <FBApplication/Manipulators/ScaleManipulator.hpp>
#include <FBCore/FBCoreHeaders.hpp>



namespace fb
{	
	namespace editor
	{


	
		EditorManager* EditorManager::m_singleton = nullptr;
	


		//--------------------------------------------
		EditorManager::EditorManager()
			: m_editTerrain(false),
			m_editFoliage(false),
			m_enablePhysics(FALSE),
			m_fileSaved(true)
		{	
			m_singleton = this;
		}
	
	
	
		//--------------------------------------------
		EditorManager::~EditorManager()
		{
			m_singleton = nullptr;
		}



		//--------------------------------------------
		void EditorManager::unload(SmartPtr<ISharedObject> data)
		{
			if (m_translateManipulator)
			{
				m_translateManipulator->unload(data);
				m_translateManipulator = nullptr;
			}

			if (m_rotateManipulator)
			{
				m_rotateManipulator->unload(data);
				m_rotateManipulator = nullptr;
			}

			if (m_scaleManipulator)
			{
				m_scaleManipulator->unload(data);
				m_scaleManipulator = nullptr;
			}
		}



		//--------------------------------------------
		void EditorManager::update(time_interval t, time_interval dt)
		{
			if (m_guiMananger)
			{
				m_guiMananger->update(t, dt);
			}

			if(m_sceneViewManager)
				m_sceneViewManager->update(t, dt);
		}
	

	
		//--------------------------------------------
		void EditorManager::importAssets()
		{

		}



		//--------------------------------------------
		String EditorManager::getProjectPath() const
		{
			return m_projectPath;
		}

		

		//--------------------------------------------
		void EditorManager::setProjectPath(const String& path)
		{
			m_projectPath = path;
		}



		//--------------------------------------------
		String EditorManager::getCachePath() const
		{
			return m_cachePath;
		}



		//--------------------------------------------
		void EditorManager::setCachePath(const String& path)
		{
			m_cachePath = path;
		}

	
	
	
		//--------------------------------------------
		EditorScriptManagerPtr EditorManager::getScriptManager() const
		{
			return m_scriptManager;
		}
	
	
	
		//--------------------------------------------
		void EditorManager::setScriptManager( EditorScriptManagerPtr val )
		{
			m_scriptManager = val;
		}



		//--------------------------------------------
		MessageManagerPtr EditorManager::getMessageManager() const
		{
			return m_messageManager;
		}



		//--------------------------------------------
		void EditorManager::setMessageManager( MessageManagerPtr val )
		{
			m_messageManager = val;
		}


		//--------------------------------------------
		ProjectManagerPtr EditorManager::getProjectManager() const
		{
			return m_projectManager;
		}



		//--------------------------------------------
		void EditorManager::setProjectManager( ProjectManagerPtr val )
		{
			m_projectManager = val;
		}


		//--------------------------------------------
		ComponentTemplateMgrPtr EditorManager::getComponentTemplateMgr() const
		{
			return m_componentTemplateMgr;
		}



		//--------------------------------------------
		void EditorManager::setComponentTemplateMgr( ComponentTemplateMgrPtr val )
		{
			m_componentTemplateMgr = val;
		}



		//--------------------------------------------
		ProjectPtr EditorManager::getProject() const
		{
			return m_project;
		}



		//--------------------------------------------
		void EditorManager::setProject( ProjectPtr val )
		{
			m_project = val;
		}



		//--------------------------------------------
		//LuaEditConfigPtr ApplicationManager::getLuaEditConfig() const
		//{
		//	return m_luaEditConfig;
		//}



		//--------------------------------------------
		//void ApplicationManager::setLuaEditConfig( LuaEditConfigPtr val )
		//{
		//	m_luaEditConfig = val;
		//}



		//--------------------------------------------
		SmartPtr<UIManager> EditorManager::getUI() const
		{
			return m_guiMananger;
		}



		//--------------------------------------------
		void EditorManager::setGUIManager( SmartPtr<UIManager> val )
		{
			m_guiMananger = val;
		}



		//--------------------------------------------
		SmartPtr<render::IDecalCursor> EditorManager::getDecalCursor() const
		{
			return m_decalCursor;
		}



		//--------------------------------------------
		void EditorManager::setDecalCursor( SmartPtr<render::IDecalCursor> val )
		{
			m_decalCursor = val;
		}



		//--------------------------------------------
		SmartPtr<render::ITerrain> EditorManager::getTerrain() const
		{
			return m_terrain;
		}



		//--------------------------------------------
		void EditorManager::setTerrain( SmartPtr<render::ITerrain> val )
		{
			m_terrain = val;
		}



		//--------------------------------------------
		SmartPtr<render::IWater> EditorManager::getWater() const
		{
			return m_water;
		}



		//--------------------------------------------
		void EditorManager::setWater( SmartPtr<render::IWater> val )
		{
			m_water = val;
		}

		//FoliageManagerPtr ApplicationManager::getFoliageManager() const
		//{
		//	return m_foliageManager;
		//}

		//void ApplicationManager::setFoliageManager( FoliageManagerPtr val )
		//{
		//	m_foliageManager = val;
		//}



		//--------------------------------------------
		bool EditorManager::getEditTerrain() const
		{
			return m_editTerrain;
		}



		//--------------------------------------------
		void EditorManager::setEditTerrain( bool val )
		{
			m_editTerrain = val;
		}



		//--------------------------------------------
		bool EditorManager::getEditFoliage() const
		{
			return m_editFoliage;
		}



		//--------------------------------------------
		void EditorManager::setEditFoliage( bool val )
		{
			m_editFoliage = val;
		}	

		//TerrainManagerPtr ApplicationManager::getTerrainManager() const
		//{
		//	//return m_terrainManager;
		//	return nullptr;
		//}

		//void ApplicationManager::setTerrainManager( TerrainManagerPtr val )
		//{
		//	//m_terrainManager = val;
		//}

		//RiverManagerPtr ApplicationManager::getRiverManager() const
		//{
		//	return m_riverManager;
		//}

		//void ApplicationManager::setRiverManager( RiverManagerPtr val )
		//{
		//	m_riverManager = val;
		//}








		//--------------------------------------------
		void EditorManager::setCurrentSceneView(u32 sceneViewId)
		{
			//if(sceneViewId == GameEditorTypes::SCENE_VIEW_ID_TERRAIN)
			//{
			//	auto terrainSceneView = getTerrainSceneView();
			//	setCurrentSceneView(terrainSceneView);

			//}
			//else if(sceneViewId == GameEditorTypes::SCENE_VIEW_ID_MESH)
			//{
			//	auto meshSceneView = getMeshSceneView();
			//	setCurrentSceneView(meshSceneView);
			//}
			//else if(sceneViewId == GameEditorTypes::SCENE_VIEW_ID_PARTICLE)
			//{
			//	setCurrentSceneView(getParticleSceneView());
			//}

		}



		//--------------------------------------------
		SceneViewManagerPtr EditorManager::getSceneViewManager() const
		{
			return m_sceneViewManager;
		}



		//--------------------------------------------
		void EditorManager::setSceneViewManager( SceneViewManagerPtr val )
		{
			m_sceneViewManager = val;
		}



		//--------------------------------------------
		bool EditorManager::getFileSaved() const
		{
			return m_fileSaved;
		}



		//--------------------------------------------
		void EditorManager::setFileSaved( bool val )
		{
			m_fileSaved = val;
		}



		//--------------------------------------------
		SmartPtr<render::IParticleSystem> EditorManager::getParticleSystem() const
		{
			return m_particleSystem;
		}



		//--------------------------------------------
		void EditorManager::setParticleSystem( SmartPtr<render::IParticleSystem> val )
		{
			m_particleSystem = val;
		}



		//--------------------------------------------
		SmartPtr<MapTemplate> EditorManager::getSelectedMap() const
		{
			return m_selectedMap;
		}



		//--------------------------------------------
		void EditorManager::setSelectedMap( SmartPtr<MapTemplate> val )
		{
			m_selectedMap = val;
		}



		//--------------------------------------------
		void EditorManager::setCityGenerator(SmartPtr<procedural::ICityGenerator> val)
		{
			m_cityGenerator = val;
		}



		//--------------------------------------------
		void EditorManager::previewAsset(const String& path)
		{
			try
			{
				auto ext = Path::getFileExtension(path);
				if (ext == ".fbx" || ext == ".FBX" ||
					ext == ".hda" || ext == ".HDA")
				{
					auto applicationManager = IApplicationManager::instance();
					auto sceneManager = applicationManager->getSceneManager();
					auto scene = sceneManager->getCurrentScene();
					auto prefabManager = applicationManager->getPrefabManager();
	
					auto resource = prefabManager->loadPrefab(path);
					if (resource)
					{
						auto actor = resource->createActor();
						if (actor)
						{
							scene->addActor(actor);
							scene->registerAllUpdates(actor);
						}
					}
	
					auto uiManager = getUI();
					if (uiManager)
					{
						uiManager->rebuildSceneTree();
					}
				}
			}
			catch (std::exception& e)
			{
				FB_LOG_EXCEPTION(e);
			}
		}



		//--------------------------------------------
		EditorManager* EditorManager::getSingletonPtr()
		{
			return m_singleton;
		}



		//--------------------------------------------
		void EditorManager::startPlaying()
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto fileSystem = applicationManager->getFileSystem();
			FB_ASSERT(fileSystem);

			auto graphicsSystem = applicationManager->getGraphicsSystem();
            if( graphicsSystem )
            {
               auto debug = graphicsSystem->getDebug();
                if( debug )
                {
                    debug->clear();
                }
            }

			if (!applicationManager->isPlaying())
			{
				auto sceneManager = applicationManager->getSceneManager();
				FB_ASSERT(sceneManager);

				auto scene = sceneManager->getCurrentScene();
				FB_ASSERT(scene);

				auto cachePath = applicationManager->getCachePath();
				FB_ASSERT(!StringUtil::isNullOrEmpty(cachePath));

				static const auto filePath = "/tmp.fbscene";
				auto tempScenePath = cachePath + filePath;

				auto projectPath = applicationManager->getProjectPath();
				if (StringUtil::isNullOrEmpty(projectPath))
				{
					projectPath = Path::getWorkingDirectory();
				}

				tempScenePath = Path::getRelativePath(projectPath, tempScenePath);

				FB_ASSERT(!StringUtil::isNullOrEmpty(tempScenePath));
				FB_ASSERT(!Path::isPathAbsolute(tempScenePath));

				scene->saveScene(tempScenePath);

				// safety code
				// clear and load to make sure values are 
				// loaded correctly in actors and components
				// can refactor to add playing events
				scene->clear();
				scene->loadScene(tempScenePath);

				applicationManager->setPlaying(true);

				auto editorManager = EditorManager::getSingletonPtr();
				FB_ASSERT(editorManager);

				auto uiManager = editorManager->getUI();
				FB_ASSERT(uiManager);

				uiManager->rebuildSceneTree();
			}

			auto sceneManager = applicationManager->getSceneManager();
			if (sceneManager)
			{
				sceneManager->play();
			}
		}



		//--------------------------------------------
		void EditorManager::pausePlaying()
		{

		}



		//--------------------------------------------
		void EditorManager::stopPlaying()
		{
			auto applicationManager = IApplicationManager::instance();
			FB_ASSERT(applicationManager);

			auto fileSystem = applicationManager->getFileSystem();
			FB_ASSERT(fileSystem);

			if (applicationManager->isPlaying())
			{
				applicationManager->setPlaying(false);

				auto sceneManager = applicationManager->getSceneManager();
				FB_ASSERT(sceneManager);

				auto scene = sceneManager->getCurrentScene();
				FB_ASSERT(scene);

				auto cachePath = applicationManager->getCachePath();
				FB_ASSERT(!StringUtil::isNullOrEmpty(cachePath));

				static const auto filePath = "/tmp.fbscene";
				auto tempScenePath = cachePath + filePath;

				auto projectPath = applicationManager->getProjectPath();
				if (StringUtil::isNullOrEmpty(projectPath))
				{
					projectPath = Path::getWorkingDirectory();
				}

				tempScenePath = Path::getRelativePath(projectPath, tempScenePath);

				FB_ASSERT(!StringUtil::isNullOrEmpty(tempScenePath));
				FB_ASSERT(!Path::isPathAbsolute(tempScenePath));

				scene->clear();
				scene->loadScene(tempScenePath);

				auto editorManager = EditorManager::getSingletonPtr();
				FB_ASSERT(editorManager);

				auto uiManager = editorManager->getUI();
				FB_ASSERT(uiManager);

				uiManager->rebuildSceneTree();
			}

			auto sceneManager = applicationManager->getSceneManager();
			if (sceneManager)
			{
				sceneManager->edit();
			}
		}



		//--------------------------------------------
		SmartPtr<TranslateManipulator> EditorManager::getTranslateManipulator() const
		{
			return m_translateManipulator;
		}



		//--------------------------------------------
		void EditorManager::setTranslateManipulator(SmartPtr<TranslateManipulator> val)
		{
			m_translateManipulator = val;
		}



		//--------------------------------------------
		SmartPtr<RotateManipulator> EditorManager::getRotateManipulator() const
		{
			return m_rotateManipulator;
		}



		//--------------------------------------------
		void EditorManager::setRotateManipulator(SmartPtr<RotateManipulator> val)
		{
			m_rotateManipulator = val;
		}



		//--------------------------------------------
		SmartPtr<ScaleManipulator> EditorManager::getScaleManipulator() const
		{
			return m_scaleManipulator;
		}



		//--------------------------------------------
		void EditorManager::setScaleManipulator(SmartPtr<ScaleManipulator> val)
		{
			m_scaleManipulator = val;
		}



		//--------------------------------------------
		const SmartPtr<procedural::ICityGenerator>& EditorManager::getCityGenerator() const
		{
			return m_cityGenerator;
		}



		//--------------------------------------------
		SmartPtr<procedural::ICityGenerator>& EditorManager::getCityGenerator()
		{
			return m_cityGenerator;
		}



	} // end namespace editor
} // end namespace fb



