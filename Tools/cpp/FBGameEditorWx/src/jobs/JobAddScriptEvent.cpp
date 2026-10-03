#include <GameEditorPCH.hpp>
#include "jobs/JobAddScriptEvent.hpp"

#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "jobs/JobGenerateScript.hpp"
#include "jobs/JobOpenScript.hpp"
#include "jobs/JobRestoreTree.hpp"
#include "jobs/JobSaveTree.hpp"
#include "commands/AddNewScriptCmd.hpp"
#include "ui/ProjectWindow.hpp"
#include "ui/ApplicationFrame.hpp"
#include "ui/UIManager.hpp"
#include <FBApplication/Script/ScriptGenerator.hpp>
#include <fstream>

#include <FBState/FBState.hpp>
#include <FBCore/FBCoreHeaders.hpp>



using std::ofstream;
using std::endl;


namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		JobAddScriptEvent::JobAddScriptEvent()
		{
		}



		//--------------------------------------------
		JobAddScriptEvent::~JobAddScriptEvent()
		{
		}



		//--------------------------------------------
		void JobAddScriptEvent::execute()
		{
			auto applicationManager = IApplicationManager::instance();
			auto fileSystem = applicationManager->getFileSystem();

			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
			SmartPtr<UIManager> guiMgr = appRoot->getUI();

			ApplicationFrame* appFrame = guiMgr->getApplicationFrame();
			if(!appFrame)
			{
				FB_EXCEPTION("Error: App frame null! ");
				return;
			}


			ProjectWindow* projectWindow = guiMgr->getProjectWindow();
			if(!projectWindow)
			{
				FB_EXCEPTION("Error: projectWindow null! ");
				return;
			}

			SmartPtr<EventTemplate> event;// = projectWindow->getSelectedObject();

			SmartPtr<JobSaveTree> jobSaveTree(new JobSaveTree);
			//jobSaveTree->setPrimary(true);
			//applicationManager->getJobQueue()->queueJob(jobSaveTree);
			
			SmartPtr<JobGenerateScript> jobGenerateScript(new JobGenerateScript);
			jobGenerateScript->setPrimary(true);
			jobGenerateScript->setEvent(event);
			applicationManager->getJobQueue()->queueJob(jobGenerateScript);
			m_generateScriptJob = jobGenerateScript;


			// update project tree
			if(!jobGenerateScript->isExistingScript())
			{
				SmartPtr<JobRestoreTree> jobRestoreTree(new JobRestoreTree);
				//jobRestoreTree->setPrimary(true);
				//applicationManager->getJobQueue()->queueJob(jobRestoreTree);
			}

			//setState(IJob::STATE_FINISHED);
		}

		//--------------------------------------------
		void JobAddScriptEvent::createOpenScriptJob()
		{
			auto applicationManager = IApplicationManager::instance();

			SmartPtr<JobGenerateScript> jobGenerateScript;// = m_generateScriptJob;

			/// open the script in the editor
			SmartPtr<JobOpenScript> jobOpenScript(new JobOpenScript);
			jobOpenScript->setPrimary(true);
			jobOpenScript->setScriptTemplate(jobGenerateScript->getScriptTemplate());
			applicationManager->getJobQueue()->queueJob(jobOpenScript);
		}

		//--------------------------------------------
		void JobAddScriptEvent::createRestoreTreeJob()
		{
			//auto applicationManager = IApplicationManager::instance();

			//SmartPtr<JobGenerateScript> jobGenerateScript = m_generateScriptJob;

			//// update project tree
			//if(!jobGenerateScript->isExistingScript())
			//{
			//	SmartPtr<JobRestoreTree> jobRestoreTree(new JobRestoreTree);
			//	//jobRestoreTree->setPrimary(true);
			//	//engine->getJobQueue()->queue(jobRestoreTree);
			//}
		}

		SmartPtr<IJob> JobAddScriptEvent::getGenerateScriptJob() const
		{
			return m_generateScriptJob;
		}

		void JobAddScriptEvent::setGenerateScriptJob( SmartPtr<IJob> val )
		{
			m_generateScriptJob = val;
		}

		JobAddScriptEvent::JobListener::JobListener(JobAddScriptEvent* job) : m_job(job)
		{

		}

		JobAddScriptEvent::JobListener::~JobListener()
		{

		}

		//--------------------------------------------
		void JobAddScriptEvent::JobListener::handleStateChanged( const SmartPtr<IStateMessage>& message )
		{
			/*if(message->isExactly(StateMessageJobStatus::TYPE_INFO))
			{
				StateMessageJobStatusPtr jobMessage = message;

				switch(jobMessage->getJobStatus())
				{
				case StateMessageJobStatus::JOB_STATUS_END:
					{
						if(jobMessage->getJob() == m_job->getGenerateScriptJob())
						{

						}
					}
					break;
				default:
					{
					}
				};
			}*/
			
		}



		//--------------------------------------------
		void JobAddScriptEvent::JobListener::handleStateChanged( const SmartPtr<IState>& state )
		{
		}



		//--------------------------------------------
		void JobAddScriptEvent::JobListener::handleQuery( SmartPtr<IStateQuery>& query )
		{
		}



	} // end namespace editor	
} // end namespace fb


