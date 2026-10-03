#ifndef JobRendererSetup_h__
#define JobRendererSetup_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/System/CJob.hpp>



namespace fb
{
	namespace editor
	{



		class JobRendererSetup : public CJob
		{
		public:
			JobRendererSetup();
			~JobRendererSetup();

			void execute();

		protected:
			void chooseSceneManager();
		};



	} // end namespace editor	
} // end namespace fb



#endif // JobRendererSetup_h__
