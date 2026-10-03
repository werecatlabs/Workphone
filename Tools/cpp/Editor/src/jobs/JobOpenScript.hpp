#ifndef JobOpenScript_h__
#define JobOpenScript_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class JobOpenScript : public Job
        {
        public:
            JobOpenScript();
            ~JobOpenScript() override;

            void execute() override;

        protected:
            //SmartPtr<ScriptTemplate> m_scriptTemplate;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // JobOpenScript_h__
