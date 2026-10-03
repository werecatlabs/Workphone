#ifndef EditorGrid_H
#define EditorGrid_H



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		class EditorGrid : public CSharedObject<ISharedObject>
		{
		public:
			EditorGrid();
			~EditorGrid();

			void OnRegisterSceneNode();
			void render();

			//const AABB3<f32>& getBoundingBox() const;

			u32 getMaterialCount();

		private:
			//AABB3<f32> Box;				//

			f32 m_cellSize = 0.0f;
		};



	} // end namespace editor
} // end namespace fb



#endif