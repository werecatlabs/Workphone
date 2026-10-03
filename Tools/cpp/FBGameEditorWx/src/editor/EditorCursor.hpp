#ifndef EditorCursor_H
#define EditorCursor_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		class EditorCursor : public CSharedObject<ISharedObject>
		{
		public:
			enum EditorCursorState
			{
				ECS_EDIT_NORMAL,
				ECS_EDIT_TERRAIN,
				ECS_EDIT_FOLIAGE,
				ECS_EDIT_CITY,

				ECS_COUNT
			};

			EditorCursor();
			~EditorCursor();

			void update(f64 t, f64 dt);

			//bool OnEvent(const SEvent& event);

			//Vector3F getPosition() const;

			//const AABB3<f32>& getBoundingBox() const;

			void setState(u32 state);
			u32 getState() const;

			void setSize(f32 size);

			//Properties* getPropertyGroup() const;

			//const triangle3df& getCurTriangle() const
			//{
			//	return m_curTriangle;
			//}

			//event
			//void OnNotifyStateChanged(StateChangedMsgPtr message);

		protected:
			//struct CollisionTreadData
			//{
			//	CollisionTreadData()
			//		:
			//		UpdateCursorPos(false)
			//	{
			//	}

			//	bool UpdateCursorPos;
			//	Vector2I MousePos;
			//	triangle3df Triangle;
			//	Vector3F Position;
			//	Vector3F Normal;
			//};

			static void threadFunc(void);

			//static SharedMutex CollisionUpdateMutex;

			//StateListenerAdapter<EditorCursor>* m_stateListener;

			//DecalSceneNode* m_cursorDecal;
			//Properties* m_propertyGroup;
			//boost::thread* m_thread;

			//aabbox3df Box;				//
			//Vector2I mousePos;

			//Vector3F Position;

			//triangle3df m_curTriangle;

			//CollisionTreadData m_collisionTread;

			DecalCursor* m_decalCursor = nullptr;

			f32 m_size = 0.0f;
			u32 m_state = 0;

			bool m_updateCursorPos = false;
		};



	} // end namespace editor
} // end namespace fb



#endif


