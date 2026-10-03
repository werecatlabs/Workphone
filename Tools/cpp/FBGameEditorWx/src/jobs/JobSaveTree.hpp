#ifndef JobSaveTree_h__
#define JobSaveTree_h__


#include <GameEditorPrerequisites.hpp>
#include <FBCore/System/CJob.hpp>



namespace fb
{
	namespace editor
	{



		class JobSaveTree : public CJob
		{
		public:
			JobSaveTree();
			~JobSaveTree();

			void execute();

		protected:
		};



	} // end namespace editor	
} // end namespace fb



#endif // JobSaveTree_h__



