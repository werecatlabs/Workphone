#include <GameEditorPCH.hpp>
#include <script/ScriptManager.hpp>
#include <jobs/JobAddScriptEvent.hpp>
#include <FBCore/Interface/IApplicationManager.hpp>
#include <FBCore/Interface/System/IJobQueue.hpp>




namespace fb 
{	
	namespace editor 
	{
	
	
	
		//--------------------------------------------
		EditorScriptManager::EditorScriptManager()
		{
	
		}



		//--------------------------------------------
		EditorScriptManager::~EditorScriptManager()
		{
	
		}
		

		//--------------------------------------------
		void EditorScriptManager::update()
		{

		}

		//--------------------------------------------
		void EditorScriptManager::createEvent( const String& className_, const String& functionName )
		{	
			auto applicationManager = IApplicationManager::instance();
			SmartPtr<IJobQueue> jobQueue = applicationManager->getJobQueue();

			SmartPtr<JobAddScriptEvent> job(new JobAddScriptEvent);
			jobQueue->queueJob(job);
		}


			
	} // end namespace editor
} // end namespace fb


