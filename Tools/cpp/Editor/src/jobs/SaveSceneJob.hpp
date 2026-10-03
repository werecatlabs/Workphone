#ifndef SaveSceneJob_h__
#define SaveSceneJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        class SaveSceneJob : public Job
        {
        public:
            SaveSceneJob();
            ~SaveSceneJob() override;

            void execute() override;

            void saveScene( const String &filePath );

            String getFilePath() const;

            void setFilePath( const String &filePath );

            bool getSaveAs() const;

            void setSaveAs( bool saveAs );

            String getSceneFileDialogExtensions();

            bool isSupportedSceneFileExtension( const String &ext );

            WP_CLASS_REGISTER_DECL;

        protected:
            String m_filePath;
            bool m_saveAs = false;
            mutable SpinRWMutex m_mutex;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // SaveSceneJob_h__
