#ifndef OpenSceneJob_h__
#define OpenSceneJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class OpenSceneJob : public Job
        {
        public:
            OpenSceneJob();
            ~OpenSceneJob() override;

            void execute() override;

            String getFilePath() const;

            void setFilePath( const String &filePath );

        protected:
            String m_filePath;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // OpenSceneJob_h__
