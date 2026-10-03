#include <GameEditorPCH.hpp>
#include "EditorCursor.hpp"
//#include "EditorEventReceiver.hpp"
//#include "EditorSceneManager.hpp"
//#include "EditorCollisionManager.hpp"
//#include "DecalCursor.hpp"
//#include "Cameras\CameraManager.hpp"
//#include "Terrain\TerrainManager.h



namespace fb
{
	namespace editor
	{


		//template<> EditorCursor* Singleton<EditorCursor>::m_singleton = 0;
		//SharedMutex EditorCursor::CollisionUpdateMutex;

		//Ogre::SceneNode* dummyCursor;



		//--------------------------------------------
		EditorCursor::EditorCursor()
			//:
			//m_size(1.0f),
			//m_updateCursorPos(false),
			//m_state(ECS_EDIT_NORMAL),
			//m_stateListener(NULL)
		{
			//Ogre::Root* root = Ogre::Root::getSingletonPtr();
			//Ogre::SceneManager* smgr = root->getSceneManager("ViewSM");

			////add a simple box for testing
			//Ogre::Entity* boxEntity = smgr->createEntity("Box000", "cube_cube1.mesh");
			//boxEntity->setMaterialName("Terrain_Shader_");
			//Ogre::SceneNode* boxSceneNode = smgr->getRootSceneNode()->createChildSceneNode();
			//boxSceneNode->attachObject(boxEntity);
			//boxSceneNode->setScale(Ogre::Vector3(10, 10, 10));
			//dummyCursor = boxSceneNode;

			//EditorEventReceiver::getSingletonPtr()->AddEventListener(this);
			//setEventPriority(50000);

			//Ogre::MaterialPtr material(Ogre::MaterialManager::getSingleton().getByName("Terrain_Shader"));
			//m_decalCursor = new DecalCursor(smgr, material, Ogre::Vector2(10, 10), "BananaLeaf.tga");

			//dummyCursor->setVisible(false);
			//m_decalCursor->show();

			//m_state = 1;

			//m_stateListener = new StateListenerAdapter<EditorCursor>(this);
			//FBSystem::getSingletonPtr()->getStateManager()->registerObserver(this, m_stateListener);

			//m_thread = new boost::thread(boost::function0<void>(&EditorCursor::threadFunc));
		}



		//--------------------------------------------
		EditorCursor::~EditorCursor()
		{
		}




		//--------------------------------------------
		void EditorCursor::update(f64 t, f64 dt)
		{

		}



		////--------------------------------------------
		//bool EditorCursor::OnEvent(const SEvent& event)
		//{
		//	switch (event.EventType)
		//	{
		//	case EET_MOUSE_INPUT_EVENT:
		//	{
		//		//Mouse event 
		//		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
		//		{
		//		}
		//		else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
		//		{
		//		}
		//		else if (event.MouseInput.Event == EMIE_MMOUSE_PRESSED_DOWN)
		//		{
		//			printf("middle button down");
		//		}
		//		else if (event.MouseInput.Event == EMIE_MOUSE_MOVED)
		//		{
		//			mousePos = Vector2I(event.MouseInput.X, event.MouseInput.Y);
		//			m_updateCursorPos = true;

		//			m_collisionTread.MousePos = mousePos;
		//			m_collisionTread.UpdateCursorPos = true;
		//		}
		//	}
		//	break;
		//	default:
		//	{
		//	}
		//	};

		//	return false;
		//}



		////--------------------------------------------
		//Vector3F EditorCursor::getPosition() const
		//{
		//	return Position;
		//}



		////--------------------------------------------
		//const AABB3<f32>& EditorCursor::getBoundingBox() const
		//{
		//	return Box;
		//}



		//--------------------------------------------
		void EditorCursor::setState(u32 state)
		{
			m_state = state;
		}



		//--------------------------------------------
		u32 EditorCursor::getState() const
		{
			return m_state;
		}



		//--------------------------------------------
		void EditorCursor::setSize(f32 size)
		{
			//m_size = size;
			//m_decalCursor->setSize(Ogre::Vector2(m_size, m_size));
		}


		////--------------------------------------------
		//Properties* EditorCursor::getPropertyGroup() const
		//{
		//	return m_propertyGroup;
		//}



		////--------------------------------------------
		//ray3df getLineFromMousePos(Vector2I mousePos)
		//{
		//	CameraControllerPtr camera = CameraManager::getSingletonPtr()->findCamera("FPSCameraController");
		//	return camera->getCameraToViewportRay(Vector2F(mousePos.X(), mousePos.Y()));
		//}



		////--------------------------------------------
		//void EditorCursor::OnNotifyStateChanged(StateChangedMsgPtr message)
		//{
		//	switch (message->getType())
		//	{
		//	case CSCT_TRANSFORMATION:
		//	{
		//		TransformChangedMsgPtr transformChangedMsg = message;

		//		Position = transformChangedMsg->Position;

		//		//switch to right handed coord
		//		Ogre::Vector3 ogrePosition(Position);
		//		ogrePosition.z = -ogrePosition.z;

		//		dummyCursor->setPosition(ogrePosition);
		//		m_decalCursor->setPosition(ogrePosition);
		//	}
		//	break;
		//	case CSCT_SHUTDOWN:
		//	{
		//		bool result = EditorEventReceiver::getSingletonPtr()->RemoveEventListener(this);
		//		_FB_DEBUG_BREAK_IF(!result);

		//		delete m_decalCursor;

		//		FB_SAFE_DROP(m_stateListener);

		//		if (m_thread)
		//		{
		//			m_thread->join();
		//			delete m_thread;
		//			m_thread = NULL;
		//		}
		//	}
		//	break;
		//	default:
		//	{
		//	}
		//	};
		//}



		//--------------------------------------------
		void EditorCursor::threadFunc(void)
		{
			//array<String> filter;
			//filter.push_back("Terrain");

			//while (FBSystem::getSingletonPtr()->isRunning())
			//{
			//	static bool freezeThread = false;
			//	if (freezeThread)
			//		continue;

			//	Sleep(50);

			//	u32 cursorState = EditorCursor::getSingletonPtr()->getState();
			//	if (cursorState == ECS_EDIT_NORMAL)
			//	{
			//		Sleep(1000);
			//		continue;
			//	}

			//	CollisionTreadData collisionTreadData;
			//	{
			//		READ_LOCK(CollisionUpdateMutex);
			//		collisionTreadData = EditorCursor::getSingletonPtr()->m_collisionTread;
			//	}

			//	bool collisionDataUpdated = false;
			//	if (collisionTreadData.UpdateCursorPos)
			//	{
			//		ray3df ray = getLineFromMousePos(collisionTreadData.MousePos);

			//		triangle3df triangle;
			//		Vector3F intersectionPoint;
			//		//bool foundCollision = EditorCollisionManager::getSingletonPtr()->RayTest(ray.Start, ray.Direction, intersectionPoint, triangle, filter);
			//		bool foundCollision = TerrainManager::getSingletonPtr()->rayIntersects(ray, intersectionPoint);
			//		collisionTreadData.Position = intersectionPoint;
			//		collisionTreadData.Triangle = triangle;
			//		collisionTreadData.Normal = triangle.getPlane().Normal;

			//		collisionTreadData.UpdateCursorPos = false;
			//		collisionDataUpdated = true;
			//	}

			//	TransformChangedMsgPtr transformChangedMsg = new TransformChangedMsg(EditorCursor::getSingletonPtr());
			//	transformChangedMsg->Position = collisionTreadData.Position;
			//	FBSystem::getSingletonPtr()->getStateManager()->notifyStateChanged(transformChangedMsg);
			//	transformChangedMsg->drop();
			//}
		}


	}
}