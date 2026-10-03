#ifndef FileSelectedJob_h__
#define FileSelectedJob_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/System/Job.hpp>

namespace workphone
{
    namespace editor
    {
        /** Job that is used to notify the editor that a file has been selected.
         */
        class FileSelectedJob : public Job
        {
        public:
            /** Constructor.
             */
            FileSelectedJob();

            /** Destructor.
             */
            ~FileSelectedJob() override;

            /** @copydoc Job::execute
             */
            void execute() override;

            /** Get the file path.
             * @return The file path.
             */
            String getFilePath() const;

            /** Set the file path.
             * @param filePath The file path.
             */
            void setFilePath( const String &filePath );

        protected:
            AtomicObject<String> m_filePath;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // FileSelectedJob_h__
