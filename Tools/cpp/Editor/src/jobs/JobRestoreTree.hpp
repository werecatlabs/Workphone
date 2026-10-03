#ifndef JobRestoreTree_h__
#define JobRestoreTree_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class JobRestoreTree : public Job
        {
        public:
            JobRestoreTree();
            ~JobRestoreTree() override;

            void execute() override;

        protected:
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // JobRestoreTree_h__
