#ifndef __RuntimeProject_h__
#define __RuntimeProject_h__

#include <Workphone/Interface/System/IProject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /**
     * @class RuntimeProject
     * @brief Runtime representation of a Workphone project.
     *
     * This class implements the IProject interface and holds runtime-only
     * project metadata such as paths to the application binary, scripts and
     * resource folders. It provides loading of a project file and simple
     * accessors/mutators for the stored values.
     *
     * The class is intended for use by the runtime subsystem and does not
     * perform heavy validation — callers are expected to provide valid paths.
     */
    class RuntimeProject : public IProject
    {
    public:
        /**
         * @brief Construct a new RuntimeProject.
         *
         * Initializes members to sensible defaults. By default the project is
         * considered an archive (m_isArchive = true) until explicitly changed.
         */
        RuntimeProject();

        /**
         * @brief Destroy the RuntimeProject.
         */
        ~RuntimeProject() override;

        /**
         * @brief Load project data from a project file.
         * @param filePath Path to the project file to load.
         *
         * This function is the public entry point for loading a project. It
         * will typically parse the file and populate internal members via
         * loadProjectFile().
         */
        void load( const String &filePath );

        /**
         * @brief Get the application binary/file path associated with this project.
         * @return String Absolute or relative path to the application file.
         */
        String getApplicationFilePath() const override;

        /**
         * @brief Set the application binary/file path associated with this project.
         * @param applicationFilePath Path to the application file.
         */
        void setApplicationFilePath( const String &applicationFilePath ) override;

        /**
         * @brief Get the project base path.
         * @return String Path that represents the project's directory or base path.
         */
        String getPath() const override;

        /**
         * @brief Set the project base path.
         * @param projectPath Project directory or base path.
         */
        void setPath( const String &projectPath ) override;

        /**
         * @brief Get the list of script file paths referenced by the project.
         * @return Array<String> Array of script file paths.
         */
        Array<String> getScriptFilePaths() const override;

        /**
         * @brief Set the list of script file paths referenced by the project.
         * @param scriptFilePaths Array of script file paths to store.
         */
        void setScriptFilePaths( const Array<String> &scriptFilePaths ) override;

        /**
         * @brief Get the list of resource folders used by the project.
         * @return Array<String> Array of resource folder paths.
         */
        Array<String> getResourceFolders() const override;

        /**
         * @brief Set the list of resource folders used by the project.
         * @param resourceFolders Array of resource folder paths.
         */
        void setResourceFolders( const Array<String> &resourceFolders ) override;

        /**
         * @brief Get the application type (e.g. "native", "managed", "web").
         * @return String Application type string.
         */
        String getApplicationType() const override;

        /**
         * @brief Set the application type.
         * @param applicationType Application type string.
         */
        void setApplicationType( const String &applicationType ) override;

        String getSceneFilePath() const;

        void setSceneFilePath( const String &sceneFilePath );

        /**
         * @brief Query whether the project is an archive package.
         * @return true if the project represents an archive; false otherwise.
         */
        bool isArchive() const override;

        /**
         * @brief Mark the project as an archive or not.
         * @param archive true to mark as archive, false otherwise.
         */
        void setArchive( bool archive ) override;

        bool isDirty() const override;

        void setDirty( bool dirty ) override;

        SmartPtr<IPlugin> getPlugin() const override;

        void setPlugin( SmartPtr<IPlugin> plugin ) override;

        SmartPtr<core::IPrototype> getParentPrototype() const override;

        void setParentPrototype( SmartPtr<core::IPrototype> prototype ) override;

        SmartPtr<Properties> getProperties() const override;

        void setProperties( SmartPtr<Properties> properties ) override;

        void saveToFile( const String &filePath ) override;

        void loadFromFile( const String &filePath ) override;

        void save() override;

        void import() override;

        void reimport() override;

        UUID getFileSystemId() const override;

        void setFileSystemId( UUID id ) override;

        String getFilePath() const override;

        void setFilePath( const String &filePath ) override;

        UUID getSettingsFileSystemId() const override;

        void setSettingsFileSystemId( UUID id ) override;

        void _getObject( void **ppObject ) const override;

        Array<SmartPtr<IResource>> getDependencies() const override;

        IResourceManager *getResourceManagerPtr() const override;

        SmartPtr<IResourceManager> getResourceManager() const override;

        void setResourceManager( SmartPtr<IResourceManager> resourceManager ) override;

        IStateContext *getStateContextPtr() const override;

        SmartPtr<IStateContext> getStateContext() const override;

        bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

        bool handleStateChanged( SmartPtr<IState> &state ) override;

    protected:
        /**
         * @brief Load and parse the project file contents.
         * @param filePath Path to the project file to parse.
         *
         * This protected helper performs the actual parsing and population of
         * internal members. It is separated from the public load() to allow
         * subclasses or test harnesses to call it directly if required.
         */
        void loadProjectFile( const String &filePath );

        /// Path to the application binary/file referenced by the project.
        String m_applicationFilePath;

        /// Path to the project file that was loaded (if any).
        String m_projectFilePath;

        /// Parent prototype used by resource tooling.
        SmartPtr<core::IPrototype> m_parentPrototype;

        /// Resource file-system id.
        UUID m_fileSystemId;

        /// Resource settings file-system id.
        UUID m_settingsFileSystemId;

        /// Owning resource manager.
        SmartPtr<IResourceManager> m_resourceManager;

        /// Base path for the project (typically the project directory).
        String m_path;

        /// String describing the application type (consumer-facing classification).
        String m_applicationType;

        /// Flag indicating whether the project is stored as an archive package.
        bool m_isArchive = true;

        /// Flag indicating whether this project has unsaved runtime changes.
        bool m_isDirty = false;

        /// Plugin associated with this project, if any.
        SmartPtr<IPlugin> m_plugin;

        /// Collection of script file paths included in the project.
        Array<String> m_scriptFilePaths;

        /// Collection of resource folder paths referenced by the project.
        Array<String> m_resourceFolders;

        /// Scene path selected by the project file.
        String m_sceneFilePath;
    };
}  // namespace workphone

#endif  // __RuntimeProject_h__
