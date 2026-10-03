#ifndef PackageManager_h__
#define PackageManager_h__

#include <Workphone/Interface/System/IPackageManager.hpp>

namespace workphone
{

    /**
     * @brief Manages packaging options and operations for the application.
     *
     * The PackageManager is responsible for creating packages and storing
     * packaging-related settings such as whether textures should be included
     * and which target platforms should be built.
     *
     * This class implements the IPackageManager interface and provides
     * concrete behavior for loading/unloading package-related data.
     */
    class WPCore_API PackageManager : public IPackageManager
    {
    public:
        /**
         * @brief Construct a new PackageManager instance.
         */
        PackageManager();

        /**
         * @brief Destroy the PackageManager instance.
         */
        ~PackageManager() override;

        /**
         * @brief Load package settings or state from a shared object.
         *
         * @param data Shared object that contains package settings/state to load.
         *
         * @copydetails IPackageManager::load
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload package settings or state and perform any necessary cleanup.
         *
         * @param data Shared object that contains package settings/state to unload.
         *
         * @copydetails IPackageManager::unload
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Create the package using the current settings.
         *
         * Concrete implementation of IPackageManager::createPackage. This will
         * assemble the package according to the configured options.
         */
        void createPackage() override;

        /**
         * @brief Query whether textures are included in the package.
         *
         * @return true if textures will be packaged; false otherwise.
         */
        bool getPackageTextures() const override;

        /**
         * @brief Enable or disable packaging of textures.
         *
         * @param packageTextures Set to true to include textures in the package.
         */
        void setPackageTextures( bool packageTextures ) override;

        /**
         * @brief Get the build target platform flags.
         *
         * The returned array contains a boolean flag for each TargetPlatform
         * indicating whether that platform is enabled for packaging.
         *
         * @return Array<bool> Copy of internal build target platform flags.
         */
        Array<bool> getBuildTargetPlatform() const override;

        /**
         * @brief Set the build target platform flags.
         *
         * Replaces the internal list of enabled target platforms with the
         * provided array.
         *
         * @param buildTargetPlatform Array of booleans indexed by TargetPlatform.
         */
        void setBuildTargetPlatform( const Array<bool> &buildTargetPlatform ) override;

        /**
         * @brief Check if a specific target platform is enabled for build.
         *
         * @param targetPlatform The platform to query.
         * @return true if the platform is enabled; false otherwise.
         */
        bool getBuildTargetPlatformEnabled( TargetPlatform targetPlatform ) const override;

        /**
         * @brief Enable or disable a specific build target platform.
         *
         * @param targetPlatform The platform to modify.
         * @param enabled True to enable the platform; false to disable it.
         */
        void setBuildTargetPlatformEnabled( TargetPlatform targetPlatform, bool enabled ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Flags indicating which target platforms are enabled for packaging.
         *
         * The array is indexed by the TargetPlatform enum. A value of true
         * means the corresponding platform will be included when creating a
         * package.
         */
        Array<bool> m_buildTargetPlatform;

        /**
         * @brief Whether textures should be included when creating a package.
         *
         * Defaults to true.
         */
        bool m_packageTextures = true;
    };
}  // namespace workphone

#endif  // PackageManager_h__
