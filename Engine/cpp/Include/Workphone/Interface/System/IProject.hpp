#ifndef IProject_h__
#define IProject_h__

#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @brief Interface for a project within the system.
     *
     * This interface defines the contract for managing project-related data, such as file paths,
     * resource folders, scripts, and plugin associations. It also provides methods for tracking
     * the state of the project, such as whether it is an archive or has unsaved changes.
     *
     * @ingroup System
     */
    class WPCore_API IProject : public IResource
    {
    public:
        IProject();

        IProject( u32 poolTypeId );

        /**
         * @brief Virtual destructor for safe cleanup of derived classes.
         */
        ~IProject() override;

        /**
         * @brief Gets the file path to the application associated with this project.
         * @return The application file path as a String.
         */
        virtual String getApplicationFilePath() const = 0;

        /**
         * @brief Sets the file path to the application associated with this project.
         * @param applicationFilePath The new application file path.
         */
        virtual void setApplicationFilePath( const String &applicationFilePath ) = 0;

        /**
         * @brief Gets the root path of the project.
         * @return The project path as a String.
         */
        virtual String getPath() const = 0;

        /**
         * @brief Sets the root path of the project.
         * @param path The new project path.
         */
        virtual void setPath( const String &path ) = 0;

        /**
         * @brief Gets the list of script file paths associated with this project.
         * @return An array of script file paths.
         */
        virtual Array<String> getScriptFilePaths() const = 0;

        /**
         * @brief Sets the list of script file paths for this project.
         * @param scriptFilePaths The array of script file paths to set.
         */
        virtual void setScriptFilePaths( const Array<String> &scriptFilePaths ) = 0;

        /**
         * @brief Gets the list of resource folder paths used by this project.
         * @return An array of resource folder paths.
         */
        virtual Array<String> getResourceFolders() const = 0;

        /**
         * @brief Sets the list of resource folder paths for this project.
         * @param resourceFolders The array of resource folder paths to set.
         */
        virtual void setResourceFolders( const Array<String> &resourceFolders ) = 0;

        /**
         * @brief Gets the application type for this project (e.g., game, tool).
         * @return The application type as a String.
         */
        virtual String getApplicationType() const = 0;

        /**
         * @brief Sets the application type for this project.
         * @param applicationType The new application type.
         */
        virtual void setApplicationType( const String &applicationType ) = 0;

        /**
         * @brief Checks if the project is stored as an archive (e.g., zip file).
         * @return True if the project is an archive, false otherwise.
         */
        virtual bool isArchive() const = 0;

        /**
         * @brief Sets whether the project is stored as an archive.
         * @param archive True to mark as archive, false otherwise.
         */
        virtual void setArchive( bool archive ) = 0;

        /**
         * @brief Checks if the project has unsaved changes (dirty state).
         * @return True if the project is dirty, false otherwise.
         */
        virtual bool isDirty() const = 0;

        /**
         * @brief Sets the dirty state of the project.
         * @param dirty True if the project has unsaved changes, false otherwise.
         */
        virtual void setDirty( bool dirty ) = 0;

        /**
         * @brief Gets the plugin associated with this project, if any.
         * @return A smart pointer to the associated IPlugin instance.
         */
        virtual SmartPtr<IPlugin> getPlugin() const = 0;

        /**
         * @brief Sets the plugin associated with this project.
         * @param plugin A smart pointer to the IPlugin instance to associate.
         */
        virtual void setPlugin( SmartPtr<IPlugin> plugin ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IProject_h__
