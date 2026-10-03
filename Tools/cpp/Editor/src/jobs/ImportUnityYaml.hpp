#ifndef ImportUnityYaml_h__
#define ImportUnityYaml_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class ImportUnityYaml : public Job
        {
        public:
            ImportUnityYaml();
            ~ImportUnityYaml() override;

            void execute() override;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // ImportUnityYaml_h__
