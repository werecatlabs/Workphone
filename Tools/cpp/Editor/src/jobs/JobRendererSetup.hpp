#ifndef JobRendererSetup_h__
#define JobRendererSetup_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class JobRendererSetup : public Job
        {
        public:
            JobRendererSetup();
            ~JobRendererSetup() override;

            void execute() override;

        protected:
            void chooseSceneManager();
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // JobRendererSetup_h__
