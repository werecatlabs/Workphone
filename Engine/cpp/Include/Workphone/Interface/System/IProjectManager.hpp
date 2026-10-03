#ifndef IProjectManager_h__
#define IProjectManager_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @brief Interface for a project manager responsible for handling project-related operations.
     *
     * This interface provides methods to generate projects, manage include and library folders,
     * and serves as a base for concrete project manager implementations.
     */
    class WPCore_API IProjectManager : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for safe polymorphic destruction.
         */
        ~IProjectManager() override;

        /**
         * @brief Generates a new project or updates the current project structure.
         *
         * This method should be implemented to create or configure the necessary files and folders
         * for a project according to the application's requirements.
         */
        virtual void generateProject() = 0;

        /**
         * @brief Adds a folder to the list of include directories for the project.
         *
         * @param includeFolder The path to the include folder to add.
         */
        virtual void addIncludeFolder( const String &includeFolder ) = 0;

        /**
         * @brief Removes a folder from the list of include directories for the project.
         *
         * @param includeFolder The path to the include folder to remove.
         */
        virtual void removeIncludeFolder( const String &includeFolder ) = 0;

        /**
         * @brief Adds a folder to the list of library directories for the project.
         *
         * @param libraryFolder The path to the library folder to add.
         */
        virtual void addLibraryFolder( const String &libraryFolder ) = 0;

        /**
         * @brief Removes a folder from the list of library directories for the project.
         *
         * @param libraryFolder The path to the library folder to remove.
         */
        virtual void removeLibraryFolder( const String &libraryFolder ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IProjectManager_h__
