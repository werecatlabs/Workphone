#ifndef JobRestoreTree_h__
#define JobRestoreTree_h__


#include <GameEditorPrerequisites.hpp>
#include <FBCore/System/CJob.hpp>



namespace fb
{
	namespace editor
	{



		class JobRestoreTree : public CJob
		{
		public:
			JobRestoreTree();
			~JobRestoreTree();

			void execute();

		protected:
		};



	} // end namespace editor	
} // end namespace fb



#endif // JobRestoreTree_h__
