#ifndef JobAddScriptEvent_h__
#define JobAddScriptEvent_h__



#include <GameEditorPrerequisites.hpp>
#include <FBCore/System/JobGroupBase.hpp>
#include <FBCore/Interface/System/IStateListener.hpp>



namespace fb
{
	namespace editor
	{
	


		//--------------------------------------------
		class JobAddScriptEvent : public JobGroupBase
		{
		public:
			JobAddScriptEvent();
			~JobAddScriptEvent();

			void execute();

			void createOpenScriptJob();
			void createRestoreTreeJob();

			SmartPtr<IJob> getGenerateScriptJob() const;
			void setGenerateScriptJob(SmartPtr<IJob> val);

		protected:
			class JobListener : public IStateListener
			{
			public:
				JobListener(JobAddScriptEvent* job);
				~JobListener();

				void handleStateChanged( const SmartPtr<IStateMessage>& message );
				void handleStateChanged( const SmartPtr<IState>& state );
				void handleQuery( SmartPtr<IStateQuery>& query );

			protected:
				JobAddScriptEvent* m_job = nullptr;
			};

			SmartPtr<IJob> m_generateScriptJob;

		};



	} // end namespace editor	
} // end namespace fb



#endif // JobAddScriptEvent_h__



