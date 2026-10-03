#ifndef GameEditorTypes_h__
#define GameEditorTypes_h__



#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{	
	namespace editor
	{
		
		

		class GameEditorTypes : public CSharedObject<ISharedObject> 
		{
		public:	
			static const u32 SCENE_VIEW_ID_TERRAIN;
			static const u32 SCENE_VIEW_ID_MESH;
			static const u32 SCENE_VIEW_ID_PARTICLE;

			static const u32 CAMERA_TYPE_VIEWPORT_0;
			static const u32 CAMERA_TYPE_VIEWPORT_1;
			static const u32 CAMERA_TYPE_VIEWPORT_2;
			static const u32 CAMERA_TYPE_VIEWPORT_3;
		};
	

	
	} // end namespace editor	
} // end namespace fb




#endif // GameEditorTypes_h__
