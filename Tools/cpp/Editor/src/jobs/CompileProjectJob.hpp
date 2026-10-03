#ifndef CompileProjectJob_h__
#define CompileProjectJob_h__

#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class CompileProjectJob : public Job
        {
        public:
            CompileProjectJob();
            ~CompileProjectJob() override;

            void execute() override;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // CompileProjectJob_h__
