#ifndef IPackageManager_h__
#define IPackageManager_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for a package manager responsible for handling package creation and build target
     * configuration.
     *
     * This interface provides methods to create packages, manage texture packaging, and configure build
     * target platforms. Implementations should provide concrete logic for these operations, typically
     * used in build systems or asset pipelines.
     */
    class WPCore_API IPackageManager : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for safe polymorphic destruction.
         */
        ~IPackageManager() override;

        /**
         * @brief Creates a new package.
         *
         * Implementations should define the logic for initializing or generating a package.
         */
        virtual void createPackage() = 0;

        /**
         * @brief Checks if package textures are enabled.
         * @return True if package textures are enabled, false otherwise.
         */
        virtual bool getPackageTextures() const = 0;

        /**
         * @brief Sets whether package textures should be enabled.
         * @param packageTextures True to enable package textures, false to disable.
         */
        virtual void setPackageTextures( bool packageTextures ) = 0;

        /**
         * @brief Gets the build target platforms as an array of booleans.
         * @return Array of booleans indicating enabled build target platforms.
         */
        virtual Array<bool> getBuildTargetPlatform() const = 0;

        /**
         * @brief Sets the build target platforms.
         * @param buildTargetPlatform Array of booleans indicating which platforms are enabled.
         */
        virtual void setBuildTargetPlatform( const Array<bool> &buildTargetPlatform ) = 0;

        /**
         * @brief Checks if a specific build target platform is enabled.
         * @param targetPlatform The target platform to check.
         * @return True if the specified platform is enabled, false otherwise.
         */
        virtual bool getBuildTargetPlatformEnabled( TargetPlatform targetPlatform ) const = 0;

        /**
         * @brief Enables or disables a specific build target platform.
         * @param targetPlatform The target platform to modify.
         * @param enabled True to enable, false to disable.
         */
        virtual void setBuildTargetPlatformEnabled( TargetPlatform targetPlatform, bool enabled ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IPackageManager_h__
