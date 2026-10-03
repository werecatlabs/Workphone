#ifndef ImportResourceJob_h__
#define ImportResourceJob_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>

namespace workphone
{

    /**
     * @brief Job that imports a resource from disk into the engine.
     *
     * This job encapsulates the work required to import a single resource
     * (file) into the engine systems. It stores the file path to import
     * and a boolean flag indicating whether this import is a re-import
     * (overwrite/update an existing resource) or a fresh import.
     *
     * Thread-safety:
     * - The `m_filePath` member is protected by `m_mutex` for concurrent
     *   reads/writes.
     * - The `m_reimport` flag is atomic and safe to read/write from
     *   multiple threads.
     *
     * Typical usage:
     * - Construct an instance, set the file path and reimport flag,
     *   then submit the job to the task/job system for execution.
     */
    class WPCore_API ImportResourceJob : public Job
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes an import job with no file path and `m_reimport` set
         * to false.
         */
        ImportResourceJob();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup when deleting via base-class pointer.
         */
        ~ImportResourceJob() override;

        /**
         * @brief Execute the import operation.
         *
         * This method is invoked by the job system when the job is scheduled.
         * Implementations should:
         * - Lock necessary resources (use `m_mutex` for `m_filePath` access).
         * - Perform the import logic (load/convert/register resource).
         * - Respect the `m_reimport` flag (update existing resources when true).
         *
         * The function overrides `Job::execute()` and should be safe to call
         * from a worker thread.
         */
        void execute() override;

        /**
         * @brief Get the path of the file to import.
         *
         * Thread-safe: acquires a shared/read lock on `m_mutex` before
         * returning the path.
         *
         * @return The file path string for the resource to import.
         */
        String getFilePath() const;

        /**
         * @brief Set the path of the file to import.
         *
         * Thread-safe: acquires an exclusive/write lock on `m_mutex` while
         * updating `m_filePath`.
         *
         * @param filePath Absolute or relative file path to import.
         */
        void setFilePath( const String &filePath );

        /**
         * @brief Query whether this import is a re-import.
         *
         * The re-import flag indicates that an existing resource should be
         * updated/replaced rather than created from scratch.
         *
         * @return True if this job should re-import (update) an existing resource.
         */
        bool getReimport() const;

        /**
         * @brief Set the re-import behaviour for this job.
         *
         * When set to true, the job should prefer updating existing resources
         * (where applicable) instead of creating new ones.
         *
         * @param reimport True to mark this job as a re-import.
         */
        void setReimport( bool reimport );

        WP_CLASS_REGISTER_DECL;

    protected:
        /// Indicates whether this job should re-import/update existing resource.
        atomic_bool m_reimport = false;

        /// File path of the resource to import. Access guarded by `m_mutex`.
        AtomicObject<String> m_filePath;
    };

}  // namespace workphone

#endif  // ImportResourceJob_h__
