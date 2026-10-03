#include <GameEditorPCH.hpp>
#include "editor/SceneViewManager.hpp"
#include "editor/EditorManager.hpp"
#include "GameEditorTypes.hpp"
#include <FBState/FBState.hpp>
#include <FBCore/FBCoreHeaders.hpp>

#include <tinyxml.hpp>



namespace fb
{	
	namespace editor
	{

		

		//-------------------------------------------------
		SceneViewManager::SceneViewManager()
		{
			auto applicationManager = IApplicationManager::instance();
			//auto factory = applicationManager->getFactory();
			

			//m_stateObject = platformMgr->createStateObject();

			//m_stateListener = SmartPtr<IStateListener>(new SceneViewManagerStateListener(this));
			//m_stateObject->addStateListener(m_stateListener.get());
		}



		//-------------------------------------------------
		SceneViewManager::~SceneViewManager()
		{
		}



		//-------------------------------------------------
		void SceneViewManager::update(time_interval t, time_interval dt)
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();

			auto task = Thread::getCurrentTask();
			switch(task)
			{
			case Thread::Task::Render:
				{
					if(m_rootNode)
						m_rootNode->_updateBounds();

					//if(m_map)
					//{
					//	SmartPtr<MapTemplate> mapTemplate = appRoot->getSelectedMap();
					//	SmartPtr<TerrainTemplate> terrainTemplate = mapTemplate->getTerrainTemplate();
					//	SmartPtr<ITerrain> terrain = m_map->getTerrain();

					//	Vector3F terrainPos = terrainTemplate->getPosition();
					//	terrain->setPosition(terrainPos);
					//}
					//} 
					//} 



				}
				break;
			case Thread::Task::Application:
				{
				/*					if(m_aabbQuery && m_aabbQuery->isResultReady())
									{
										AABB3F aabb = m_aabbQuery->getAABB();
										initCamera(aabb);


										m_aabbQuery.setNull();
									}	*/
				}
				break;
			default:
				{
				}
			};

			//for(u32 i=0; i<m_entities.size(); ++i)
			//{
			//	m_entities[i]->update(CURRENT_TASK_ID, t, dt);
			//}
			
			//SmartPtr<IParticleSystem> ps = appRoot->getParticleSystem();
			//if(ps)
			//{
			//	ps->update(CURRENT_TASK_ID, t, dt);				
			//}
		}



		//-------------------------------------------------
		SmartPtr<MeshTemplate> SceneViewManager::getMeshTemplate() const
		{
			return m_meshTemplate;
		}



		//-------------------------------------------------
		void SceneViewManager::setMeshTemplate(SmartPtr<MeshTemplate> meshTemplate)
		{
//			if(m_meshTemplate != meshTemplate)
//			{
//				m_meshTemplate = meshTemplate;
//
//				auto appRoot = EditorManager::getSingletonPtr();
//				auto sceneView = appRoot->getMeshSceneView();
//				auto sceneMgr = sceneView->getSceneManager();
//				//sceneMgr->clearScene();
//
//				auto meshName = meshTemplate->getMeshName();
//
//				auto mesh = sceneMgr->addMesh(meshName);
//				auto meshNode = sceneMgr->getRootSceneNode()->addChildSceneNode();
//				meshNode->attachObject(mesh);
//
//				appRoot->setCurrentSceneView( GameEditorTypes::SCENE_VIEW_ID_MESH );
//
//				auto applicationManager = IApplicationManager::instance();
//				auto cameraMgr = applicationManager->getCameraManager();
//				cameraMgr->setCurrentCamera(String("SphericalCamera"));
//				
//				//test mesh splitting
//				/*
//				SmartPtr<Properties> properties(new Properties);
//				properties->setPropertyValue("maxSize", 10.0f);
//				properties->setPropertyValue("recoveryAttempts", 1);
//				Array<GraphicsMeshPtr> meshes = sceneMgr->splitMesh(mesh, properties);
//				for(u32 i=0; i<meshes.size(); ++i)
//				{
//					GraphicsMeshPtr splitMesh = meshes[i];
//
//					splitMesh->detachFromParent();
//					splitMesh->setMaterialName("lambert1");
//
//					SmartPtr<ISceneNode> splitMeshNode = meshNode->addChildSceneNode();
//					splitMeshNode->attachObject(splitMesh);
//				}
//*/
//				//m_aabbQuery = SmartPtr<StateQueryAABB3F>(new StateQueryAABB3F);
//				//m_aabbQuery->setType(ISceneNode::STATE_QUERY_TYPE_WORLD_AABB);
//				//meshNode->getStateObject()->addQuery(m_aabbQuery);
//
//
//				auto cameraCtrl = cameraMgr->getCurrentCamera();
//				if(cameraCtrl)
//				{
//					//cameraCtrl->setPosition(Vector3F::UNIT_Z * 30);
//					//cameraCtrl->setTargetPosition(Vector3F::ZERO);
//				}
//				
//			}
		}



		//-------------------------------------------------
		SmartPtr<render::ISceneNode> SceneViewManager::createMeshSceneNode(const String& fileName)
		{
			//ApplicationManager* appRoot = IApplicationManager::instance();
			//SmartPtr<ISceneView> sceneView = appRoot->getMeshSceneView();
			//SmartPtr<render::ISceneManager> sceneMgr = sceneView->getSceneManager();			

			//GraphicsMeshPtr mesh = sceneMgr->addMesh(fileName);
			//SmartPtr<ISceneNode> meshNode = m_rootNode->addChildSceneNode();
			//meshNode->attachObject(mesh);

			//return meshNode;

			return nullptr;
		}



		//-------------------------------------------------
		void SceneViewManager::createDestructible(const String& fileName)
		{
			//EditorManager* appRoot = EditorManager::getSingletonPtr();
			//appRoot->setCurrentSceneView( GameEditorTypes::SCENE_VIEW_ID_MESH );

			//auto applicationManager = IApplicationManager::instance();
			//auto cameraMgr = applicationManager->getCameraManager();
			//cameraMgr->setCurrentCamera(String("SphericalCamera"));
			//		
			//auto sceneView = appRoot->getMeshSceneView();
			//auto sceneMgr = sceneView->getSceneManager();

			//m_rootNode = sceneMgr->getRootSceneNode()->addChildSceneNode();


			//TiXmlDocument document;
			//document.LoadFile(fileName.c_str());

			//if (document.Error())
			//{
			//	return;
			//}

			//TiXmlElement* rootElement = document.FirstChildElement();
			//TiXmlElement* childElement = rootElement->FirstChildElement();
			//while(childElement)
			//{
			//	String name = childElement->Attribute("name");
			//	auto node = createMeshSceneNode(name);
			//	m_nodes.push_back(node);

			//	//RigidBodyMeshPtr rigidBodyMesh(new RigidBodyMesh);
			//	//rigidBodyMesh->initialise(node);
			//	//m_entities.push_back(rigidBodyMesh);

			//	/*
			//	SplitPart *part1 = getOrCreatePart(childElement->Attribute("name"));
			//	TiXmlElement* connectionElement = childElement->FirstChildElement();
			//	while(connectionElement)
			//	{
			//		SplitPart *part2 = getOrCreatePart(connectionElement->Attribute("target"));

			//		TiXmlElement* attributeElement = connectionElement->FirstChildElement();
			//		float breakForce = mBreakForce;
			//		float breakTorque = mBreakTorque;
			//		while(attributeElement)
			//		{
			//			double d;
			//			if (attributeElement->ValueStr() == "rel_force")
			//			{
			//				attributeElement->Attribute("value", &d);
			//				breakForce *= d;
			//			}
			//			if (attributeElement->ValueStr() == "rel_torque")
			//			{
			//				attributeElement->Attribute("value", &d);
			//				breakTorque *= d;
			//			}

			//			attributeElement = attributeElement->NextSiblingElement();
			//		}

			//		//connect the parts
			//		createJoint(part1, part2, breakForce, breakTorque);

			//		connectionElement = connectionElement->NextSiblingElement();
			//	}
			//	*/

			//	childElement = childElement->NextSiblingElement();
			//}


			////m_aabbQuery = SmartPtr<StateQueryAABB3F>(new StateQueryAABB3F);
			////m_aabbQuery->setType(ISceneNode::STATE_QUERY_TYPE_WORLD_AABB);
			////m_rootNode->getStateObject()->addQuery(m_aabbQuery);
		}



		//-------------------------------------------------
		void SceneViewManager::setMesh(const String& filePath)
		{

		}



		//-------------------------------------------------
		void SceneViewManager::setScene(const String& filePath)
		{
			//ApplicationManager* appRoot = IApplicationManager::instance();
			////appRoot->setCurrentSceneView( GameEditorTypes::SCENE_VIEW_ID_MESH );
			//appRoot->setCurrentSceneView( GameEditorTypes::SCENE_VIEW_ID_TERRAIN ); // temp

			//auto applicationManager = IApplicationManager::instance();
			//SmartPtr<ICameraManager> cameraMgr = applicationManager->getCameraManager();
			////cameraMgr->setCurrentCamera(String("SphericalCamera"));

			////SmartPtr<ISceneView> sceneView = appRoot->getMeshSceneView();
			//SmartPtr<ISceneView> sceneView = appRoot->getTerrainSceneView(); // temp
			//SmartPtr<render::ISceneManager> sceneMgr = sceneView->getSceneManager();

			//String path = Path::getFilePath(filePath);

			//SmartPtr<IFileSystem> fileSystem = engine->getFileSystem();
			//fileSystem->addFileArchive(path, true, true, FB_ARCH_TYPE_FOLDER);
			//
			//m_rootNode = sceneMgr->getRootSceneNode()->addChildSceneNode();

			//Array<SmartPtr<ISceneNode>> sceneNodes;
			//Array<GraphicsObjectPtr> graphicsObjects;

			//String ext = StringUtil::getFileNameExtension(filePath);
			//if(ext==(".vuescene"))
			//{
			//	TiXmlDocument doc;
			//	doc.LoadFile(filePath.c_str());

			//	if(!doc.Error())
			//	{
			//		TiXmlElement* root = doc.RootElement();
			//		if(root)
			//		{
			//			TiXmlElement* objectsElem = root->FirstChildElement("objects");
			//			if(objectsElem)
			//			{
			//				TiXmlElement* objectElem = objectsElem->FirstChildElement("object");
			//				while(objectElem)
			//				{
			//					TiXmlElement* positionElem = objectElem->FirstChildElement("position");
			//					TiXmlElement* rotationElem = objectElem->FirstChildElement("rotation");
			//					TiXmlElement* scaleElem = objectElem->FirstChildElement("scale");
			//					TiXmlElement* fileNameElem = objectElem->FirstChildElement("fileName");

			//					Vector3F position;
			//					Vector3F rotation;
			//					Vector3F scale;

			//					position.X() = StringUtil::parseFloat(positionElem->Attribute("x"));
			//					position.Y() = StringUtil::parseFloat(positionElem->Attribute("z"));
			//					position.Z() = StringUtil::parseFloat(positionElem->Attribute("y"));

			//					rotation.X() = StringUtil::parseFloat(rotationElem->Attribute("x"));
			//					rotation.Y() = StringUtil::parseFloat(rotationElem->Attribute("z"));
			//					rotation.Z() = StringUtil::parseFloat(rotationElem->Attribute("y"));

			//					scale.X() = StringUtil::parseFloat(scaleElem->Attribute("x"));
			//					scale.Y() = StringUtil::parseFloat(scaleElem->Attribute("z"));
			//					scale.Z() = StringUtil::parseFloat(scaleElem->Attribute("y"));

			//					SmartPtr<ISceneNode> sceneNode = m_rootNode->addChildSceneNode();
			//					sceneNode->setPosition(position);
			//					sceneNode->setScale(Vector3F::UNIT*1.0);

			//					GraphicsMeshPtr mesh = sceneMgr->addMesh(path + String("/") + String(fileNameElem->Attribute("name")));
			//					if(mesh)
			//					{
			//						sceneNode->attachObject(mesh);

			//						TiXmlElement* materialElem = objectElem->FirstChildElement("material");
			//						if(materialElem)
			//						{
			//							//Ogre::Root* root = Ogre::Root::getSingletonPtr();
			//							//Ogre::MaterialManager* materialMgr = Ogre::MaterialManager::getSingletonPtr();
			//							//Ogre::SmartPtr<IMaterial> material = materialMgr->create((String(materialElem->Attribute("name")) + String("_Generated")).c_str(), 
			//							//	Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

			//							//TiXmlElement* textureElem = materialElem->FirstChildElement("texture");
			//							//if(textureElem)
			//							//{
			//							//	Ogre::Technique* tech = material->getTechnique(0);
			//							//	Ogre::Pass* pass = tech->createPass();
			//							//	if(pass)
			//							//	{
			//							//		Ogre::TextureUnitState* textureState = pass->createTextureUnitState();
			//							//		textureState->setTextureName(textureElem->Attribute("name"));
			//							//	}
			//							//}

			//							//SmartPtr<IMesh> entity = nullptr;
			//							//mesh->_getObject((void**)&entity);
			//							//entity->setMaterial(material);
			//						}



			//						//SmartPtr<IGraphicsSystem> graphicsSystem;
			//						//SmartPtr<IMaterialManager> materialMgr = graphicsSystem->getMaterialManager();
			//						//materialMgr->
			//					}

			//					objectElem = objectElem->NextSiblingElement();
			//				}
			//			}
			//		}
			//	}
			//}
			//else if(ext==(".map"))
			//{
			//	MapManagerPtr mapMgr = engine->getMapManager();
			//	SmartPtr<IMap> map = mapMgr->addMap();

			//	//SmartPtr<TerrainTemplate> terrainTemplate(new TerrainTemplate);

			//	SmartPtr<MapTemplate> mapTemplate = appRoot->getSelectedMap();
			//	SmartPtr<TerrainTemplate> terrainTemplate = mapTemplate->getTerrainTemplate();

			//	terrainTemplate->setSceneManagerName(sceneMgr->getName());
			//	map->initialise(mapTemplate);

			//	m_map = map;
			//}
			//else
			//{
			//	sceneMgr->loadSceneFile(filePath, m_rootNode, sceneNodes, graphicsObjects);
			//}

			//AABB3F box = m_rootNode->getWorldAABB();

			//SmartPtr<ICameraController> cameraCtrl = cameraMgr->getCurrentCamera();
			////cameraCtrl->setPosition(Vector3F::UNIT_Z * -box.Max.Z());
			//cameraCtrl->setPosition((Vector3F::UNIT_Y * 200.0f) + (Vector3F::UNIT_Z * 500.0f));
			//cameraCtrl->setTargetPosition(Vector3F::ZERO);


			//m_aabbQuery = StateQueryAABB3FPtr(new StateQueryAABB3F);
			//m_aabbQuery->setType(ISceneNode::STATE_QUERY_TYPE_WORLD_AABB);
			//m_rootNode->getStateObject()->addQuery(m_aabbQuery);
			// 

			//auto applicationManager = IApplicationManager::instance();
			//SmartPtr<ICameraManager> cameraMgr = engine->getCameraManager();
			//cameraMgr->setCurrentCamera(String("SphericalCamera"));
			
			//initCamera( m_rootNode->getWorldAABB() );

			//sceneMgr->setSkyBox(true, "Sky/EarlyMorning", 5000, false);
		}



		//-------------------------------------------------
		void SceneViewManager::addOverlay(const String& filePath)
		{
			//StateMessageStringValuePtr message(new StateMessageStringValue);
			//message->setValue(filePath);
			//message->setType(StringUtil::getHash("loadGui"));
			//m_stateObject->addMessage(Thread::Task::TASK_ID_RENDER, message);
		}


		//-------------------------------------------------
		SmartPtr<ParticleSystemTemplate> SceneViewManager::getParticleTemplate() const
		{
			return m_particleTemplate;
		}



		//-------------------------------------------------
		void SceneViewManager::setParticleTemplate( SmartPtr<ParticleSystemTemplate> val )
		{
			//m_particleTemplate = val;

			//ApplicationManager* appRoot = IApplicationManager::instance();
			//appRoot->setCurrentSceneView( GameEditorTypes::SCENE_VIEW_ID_PARTICLE );

			//auto applicationManager = IApplicationManager::instance();
			//SmartPtr<IGraphicsSystem>& graphicsSystem = engine->getGraphicsSystem();
			//FactoryPtr& factory = engine->getFactory();

			//SmartPtr<ICameraManager> cameraMgr = engine->getCameraManager();
			//cameraMgr->setCurrentCamera(String("SphericalCamera"));

			//SmartPtr<ISceneView> particleSceneView = appRoot->getParticleSceneView();
			//SmartPtr<render::ISceneManager> sceneManager = particleSceneView->getSceneManager();

			//SmartPtr<Properties> properties(new Properties);
			//properties->setProperty("sceneManagerName", sceneManager->getName());

			////SmartPtr<IParticleSystem> particleSystem = factory->createById(StringUtil::getHash("ParticleSystem"));
			////particleSystem->initialise(m_particleTemplate, properties);
			// 
			//SmartPtr<IParticleSystem> particleSystem = sceneManager->addParticleSystem("ps", m_particleTemplate->getTemplateName());

			//m_rootNode = sceneManager->getRootSceneNode()->addChildSceneNode();
			//m_rootNode->attachObject(particleSystem);

			//appRoot->setParticleSystem(particleSystem);

			//particleSystem->start();

			////m_aabbQuery = StateQueryAABB3FPtr(new StateQueryAABB3F);
			////m_aabbQuery->setType(ISceneNode::STATE_QUERY_TYPE_WORLD_AABB);
			////m_rootNode->getStateObject()->addQuery(m_aabbQuery);
			//
			//SmartPtr<ISceneNode> boxNode = sceneManager->getRootSceneNode()->addChildSceneNode();
			//GraphicsMeshPtr boxMesh = sceneManager->addMesh("crabMeshShape.mesh");
			//boxMesh->setMaterialName("Boss_1");
			//boxNode->attachObject(boxMesh);
			//boxNode->setPosition(Vector3F(0,-0,-10));

			//SphericalCameraPtr sphericalCamera = cameraMgr->getCurrentCamera();
			//sphericalCamera->setTargetPosition(Vector3F::ZERO);
			//sphericalCamera->setSphericalCoords(Vector3F(5.0, 0.0f, 1.5f));
			//sphericalCamera->setMaxDistance(250.0f);
		}




		//-------------------------------------------------
		void SceneViewManager::initCamera( const AABB3F &aabb )
		{
			//Vector3F center = aabb.getCenter();
			//Vector3F extent = aabb.getExtent() * 0.5f;

			//auto applicationManager = IApplicationManager::instance();
			//SmartPtr<ICameraManager> cameraMgr = engine->getCameraManager();

			//SphericalCameraPtr sphericalCamera = cameraMgr->getCurrentCamera();
			//sphericalCamera->setTargetPosition(center);
			//sphericalCamera->setSphericalCoords(Vector3F(extent.length(), 0.0f, 1.5f));
			//sphericalCamera->setMaxDistance(extent.length() * 5.0f);
		}



		//-------------------------------------------------
		void SceneViewManager::refresh()
		{
			//if(m_map)
			//{
			//	m_map->refresh();
			//}
			//else
			//{	
			//	//ApplicationManager* appRoot = IApplicationManager::instance();
			//	//ProjectPtr project = appRoot->getProject();

			//	//appRoot->setCurrentSceneView( GameEditorTypes::SCENE_VIEW_ID_TERRAIN );

			//	//SmartPtr<MapTemplate> mapTemplate = appRoot->getSelectedMap();
			//	//SmartPtr<TerrainTemplate> terrainTemplate = mapTemplate->getTerrainTemplate();

			//	//auto applicationManager = IApplicationManager::instance();
			//	//SmartPtr<IGraphicsSystem>& graphicsSystem = engine->getGraphicsSystem();
			//	//SmartPtr<render::ISceneManager>& terrainSmgr = graphicsSystem->getSceneManager("TerrainView");

			//	//SmartPtr<ITerrain> terrain = terrainSmgr->createTerrain();
			//	//terrain->initialise(terrainTemplate);

			//	//SmartPtr<IStateObject>& stateObject = terrain->getStateObject();

			//	//appRoot->setTerrain(terrain);
			//}
		}



		//-------------------------------------------------
		SceneViewManager::SceneViewManagerStateListener::SceneViewManagerStateListener( SceneViewManager* sceneViewManager ) 
			: m_sceneViewManager(sceneViewManager)
		{
		}



		//-------------------------------------------------
		void SceneViewManager::SceneViewManagerStateListener::handleStateChanged( const SmartPtr<IStateMessage>& message )
		{
			//hash32 messageType = message->getType();
			//if(messageType == StringUtil::getHash("loadGui"))
			//{
			//	StateMessageStringValuePtr stringValueMessage = message;
			//	
			//	auto applicationManager = IApplicationManager::instance();
			//	fb::SmartPtr<UIManager> guiManager = engine->getGUIManager();

			//	guiManager->load("GuiTemplates.gui");

			//	guiManager->load(stringValueMessage->getValue());

			//	GUIUtil::setItemVisible("GameUILayout");

			//	ApplicationManager* appRoot = IApplicationManager::instance();
			//	appRoot->setCurrentSceneView( GameEditorTypes::SCENE_VIEW_ID_MESH );
			//}
		}



		//-------------------------------------------------
		void SceneViewManager::SceneViewManagerStateListener::handleStateChanged( const SmartPtr<IState>& state )
		{
		}



		//-------------------------------------------------
		void SceneViewManager::SceneViewManagerStateListener::handleQuery( SmartPtr<IStateQuery>& query )
		{
		}



	} // end namespace editor	
} // end namespace fb	



