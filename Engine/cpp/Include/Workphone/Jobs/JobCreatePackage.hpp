#ifndef JobCreatePackage_h__
#define JobCreatePackage_h__

#include <Workphone/System/Job.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include "Workphone/Atomics/AtomicObject.hpp"

namespace workphone
{
    /**
     * @class JobCreatePackage
     * @brief Job responsible for creating a package archive of resources.
     *
     * This job is intended to be scheduled/executed by the engine job system.
     * When executed it will create a package at the configured destination path.
     * Optionally the job can include textures in the created package.
     *
     * Thread-safety:
     * - The destination path is stored in an AtomicObject<StringW> so it can be
     *   read/written safely from different threads.
     * - The package textures flag is an atomic_bool.
     *
     * @note Concrete packaging behaviour (format, compression, resource selection)
     *       is implemented in the job's execute() method.
     */
    class WPCore_API JobCreatePackage : public Job
    {
    public:
        /**
         * @brief Construct a new JobCreatePackage.
         *
         * Initializes members to sensible defaults. By default textures will be
         * included in the package.
         */
        JobCreatePackage();

        /**
         * @brief Destructor.
         */
        ~JobCreatePackage() override;

        /**
         * @brief Execute the job.
         *
         * This method performs the work required to create the package. It should
         * be safe to call from a worker thread and must respect the atomic members
         * used to configure the job (destination path and whether to package
         * textures).
         *
         * @see setDst, setPackageTextures
         */
        void execute() override;

        /**
         * @brief Get the destination path for the created package.
         *
         * The returned path is the value stored in the atomic destination member.
         *
         * @return StringW The destination path where the package will be written.
         */
        StringW getDst() const;

        /**
         * @brief Set the destination path for the created package.
         *
         * This method updates the atomic destination member and is safe to call
         * from other threads.
         *
         * @param dst The destination path (absolute or relative) to write the package.
         */
        void setDst( const StringW &dst );

        /**
         * @brief Query whether textures will be included in the package.
         *
         * @return true if textures will be packaged; false otherwise.
         */
        bool getPackageTextures() const;

        /**
         * @brief Set whether textures should be included in the package.
         *
         * This updates an atomic flag that will be read by execute().
         *
         * @param packageTextures true to include textures, false to exclude them.
         */
        void setPackageTextures( bool packageTextures );

        /**
         * @brief Get the enabled target platform flags used when filtering references.
         * @return Array indexed by TargetPlatform.
         */
        Array<bool> getBuildTargetPlatform() const;

        /**
         * @brief Set the enabled target platform flags used when filtering references.
         * @param buildTargetPlatform Array indexed by TargetPlatform.
         */
        void setBuildTargetPlatform( const Array<bool> &buildTargetPlatform );

        /** Macro used for runtime class registration. */
        WP_CLASS_REGISTER_DECL;

    protected:
        /** Atomic destination path where the package will be created. */
        AtomicObject<StringW> m_dst;

        /** Atomic flag indicating whether textures should be packaged. Default true. */
        atomic_bool m_packageTextures = true;

        /** Target platforms enabled for this package run. */
        AtomicObject<Array<bool>> m_buildTargetPlatform;
    };
}  // namespace workphone

#endif  // JobCreatePackage_h__
