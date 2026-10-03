#ifndef JobAddScriptEvent_h__
#define JobAddScriptEvent_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/JobGroupBase.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace editor
    {
        //--------------------------------------------
        class JobAddScriptEvent : public JobGroupBase
        {
        public:
            JobAddScriptEvent();
            ~JobAddScriptEvent() override;

            void execute() override;

            void createOpenScriptJob();
            void createRestoreTreeJob();

            SmartPtr<IJob> getGenerateScriptJob() const;
            void setGenerateScriptJob( SmartPtr<IJob> val );

        protected:
            class JobListener : public IStateListener
            {
            public:
                JobListener( JobAddScriptEvent *job );
                ~JobListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                void handleStateChanged( const SmartPtr<IState> &state );

            protected:
                JobAddScriptEvent *m_job = nullptr;
            };

            SmartPtr<IJob> m_generateScriptJob;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // JobAddScriptEvent_h__
