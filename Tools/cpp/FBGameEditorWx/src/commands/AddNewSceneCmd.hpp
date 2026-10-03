#ifndef AddNewSceneCmd_h__
#define AddNewSceneCmd_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/Interface/System/ICommand.hpp>
#include <FBCore/Memory/CSharedObject.hpp>



namespace fb
{
	namespace editor	
	{



		//--------------------------------------------
		class AddNewSceneCmd : public CSharedObject<ICommand> 
		{
		public:
			AddNewSceneCmd(Properties properties);
			~AddNewSceneCmd();

			void undo();
			void redo();
			void execute();

		protected:
			Properties m_properties;
		};


	
		typedef SmartPtr<AddNewSceneCmd> AddNewSceneCmdPtr;



	} // end namespace editor
} // end namespace fb



#endif // AddNewSceneCmd_h__
