#ifndef JobSaveTree_h__
#define JobSaveTree_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class JobSaveTree : public Job
        {
        public:
            JobSaveTree();
            ~JobSaveTree() override;

            void execute() override;

        protected:
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // JobSaveTree_h__
