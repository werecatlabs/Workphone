#ifndef __WP_ProjectManager_h__
#define __WP_ProjectManager_h__

#include <Workphone/Interface/System/IProjectManager.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    /**
     * @brief The ProjectManager class is responsible for generating project files for the engine.
     *
     * This class provides functionality to create and manage project configurations,
     * including handling include folders, library folders, libraries, and various
     * project settings. It supports both shared library and static library configurations
     * and provides thread-safe operations through internal locking mechanisms.
     *
     * @author WorkPhone Engine Team
     * @version 1.0
     * @since Engine v1.0
     */
    class WPCore_API ProjectManager : public IProjectManager
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes a new ProjectManager instance with default settings.
         * The project is configured as a shared library by default.
         */
        ProjectManager();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of resources when the ProjectManager is destroyed.
         */
        ~ProjectManager() override;

        /**
         * @brief Generates the project file based on current configuration.
         *
         * This method creates the actual project file using all the configured
         * settings including project name, engine path, include folders,
         * library folders, and linked libraries.
         *
         * @throws std::runtime_error if project generation fails
         */
        void generateProject() override;

        /**
         * @brief Checks if the project is configured as a shared library.
         *
         * @return true if the project is configured as a shared library, false otherwise
         */
        bool isSharedLibrary() const;

        /**
         * @brief Sets whether the project should be built as a shared library.
         *
         * @param sharedLibrary true to configure as shared library, false for static library
         */
        void setSharedLibrary( bool sharedLibrary );

        /**
         * @brief Gets the current project name.
         *
         * @return The project name as a String
         */
        String getProjectName() const;

        /**
         * @brief Sets the project name.
         *
         * @param projectName The name to assign to the project
         */
        void setProjectName( const String &projectName );

        /**
         * @brief Gets the engine path.
         *
         * @return The path to the engine directory as a String
         */
        String getEnginePath() const;

        /**
         * @brief Sets the engine path.
         *
         * @param enginePath The path to the engine directory
         */
        void setEnginePath( const String &enginePath );

        /**
         * @brief Adds an include folder to the project configuration.
         *
         * Include folders are used by the compiler to locate header files
         * during the build process.
         *
         * @param includeFolder The path to the include folder to add
         */
        void addIncludeFolder( const String &includeFolder ) override;

        /**
         * @brief Removes an include folder from the project configuration.
         *
         * @param includeFolder The path to the include folder to remove
         */
        void removeIncludeFolder( const String &includeFolder ) override;

        /**
         * @brief Adds a library folder to the project configuration.
         *
         * Library folders are used by the linker to locate library files
         * during the linking process.
         *
         * @param libraryFolder The path to the library folder to add
         */
        void addLibraryFolder( const String &libraryFolder ) override;

        /**
         * @brief Removes a library folder from the project configuration.
         *
         * @param libraryFolder The path to the library folder to remove
         */
        void removeLibraryFolder( const String &libraryFolder ) override;

        /**
         * @brief Gets all configured include folders.
         *
         * @return An Array containing all include folder paths
         */
        Array<String> getIncludeFolders() const;

        /**
         * @brief Sets the complete list of include folders.
         *
         * This replaces any existing include folders with the provided list.
         *
         * @param includeFolders Array of include folder paths
         */
        void setIncludeFolders( const Array<String> &includeFolders );

        /**
         * @brief Gets all configured library folders.
         *
         * @return An Array containing all library folder paths
         */
        Array<String> getLibraryFolders() const;

        /**
         * @brief Sets the complete list of library folders.
         *
         * This replaces any existing library folders with the provided list.
         *
         * @param libraryFolders Array of library folder paths
         */
        void setLibraryFolders( const Array<String> &libraryFolders );

        /**
         * @brief Adds a library to be linked with the project.
         *
         * @param library The name of the library to add (without file extension)
         */
        void addLibrary( const String &library );

        /**
         * @brief Removes a library from the project's link dependencies.
         *
         * @param library The name of the library to remove
         */
        void removeLibrary( const String &library );

        /**
         * @brief Gets all libraries configured for linking.
         *
         * @return An Array containing all library names
         */
        Array<String> getLibraries() const;

        /**
         * @brief Sets the complete list of libraries to link.
         *
         * This replaces any existing libraries with the provided list.
         *
         * @param libraries Array of library names
         */
        void setLibraries( const Array<String> &libraries );

        /**
         * @brief Clears all configured libraries.
         *
         * Removes all libraries from the project's link dependencies.
         */
        void clearLibraries();

        /**
         * @brief Locks the ProjectManager for exclusive access.
         *
         * This method blocks until the lock can be acquired. Use this when
         * you need to perform multiple operations atomically.
         */
        void lock() override;

        /**
         * @brief Attempts to lock the ProjectManager without blocking.
         *
         * @return true if the lock was successfully acquired, false otherwise
         */
        bool try_lock() override;

        /**
         * @brief Unlocks the ProjectManager.
         *
         * Must be called after a successful lock() or try_lock() operation.
         */
        void unlock() override;

    protected:
        /**
         * @brief Gets platform-specific compiler and linker options.
         *
         * @return String containing platform-specific build options
         */
        String getPlatformOptions() const;

        /**
         * @brief Gets toolset-specific compiler and linker options.
         *
         * @return String containing toolset-specific build options
         */
        String getToolsetOptions() const;

        String m_projectName;  ///< The name of the project
        String m_enginePath;   ///< Path to the engine directory

        bool m_isSharedLibrary = true;  ///< Whether to build as shared library (true) or static (false)

        Array<String> m_includeFolders;  ///< List of include directories for compilation
        Array<String> m_libraryFolders;  ///< List of library directories for linking
        Array<String> m_libraries;       ///< List of libraries to link against

        mutable RecursiveMutex m_mutex;  ///< Mutex for thread-safe operations
    };
}  // namespace workphone

#endif  // ProjectManager_h__
