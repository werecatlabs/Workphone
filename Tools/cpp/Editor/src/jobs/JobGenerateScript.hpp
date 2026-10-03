#ifndef JobGenerateScript_h__
#define JobGenerateScript_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class JobGenerateScript : public Job
        {
        public:
            JobGenerateScript();
            ~JobGenerateScript() override;

            void execute() override;

            bool isExistingScript() const;
            void setExistingScript( bool val );

        protected:
            //SmartPtr<ScriptTemplate> m_scriptTemplate;
            //SmartPtr<EventTemplate> m_event;

            bool m_isExistingScript = false;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // JobGenerateScript_h__
