#ifndef Project_h__
#define Project_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/System/IProject.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Memory/WeakPtr.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @brief Represents a Workphone editor project and its persistent metadata.
         *
         * A project owns the configuration needed to load, save, compile, and run an
         * application in the editor. It stores the project root, working directory,
         * application executable path, resource search paths, script paths, default media
         * folders, plugin metadata, and the current scene.
         *
         * @note Mutations must be performed on the editor thread to keep the project state
         *       coherent with the rest of the editor UI and resource system.
         * @note File operations and compilation throw on failure; a successful load or save
         *       clears the dirty flag.
         * @note Relative asset paths are stored relative to the project directory.
         */
        class Project : public Resource<IProject>
        {
        public:
            /** @brief Default project name used for newly created projects. */
            static const String DEFAULT_PROJECT_NAME;
            /** @brief Default media folder name relative to the project root. */
            static const String DEFAULT_MEDIA_PATH;
            /** @brief Default scripts folder name relative to the project root. */
            static const String DEFAULT_SCRIPTS_PATH;
            /** @brief Default entities folder name relative to the project root. */
            static const String DEFAULT_ENTITIES_PATH;
            /** @brief Default project file format version. */
            static const String DEFAULT_VERSION;

            /** @brief Creates an empty project with default settings. */
            Project();
            /** @brief Destroys the project and releases owned resources. */
            ~Project() override;

            /** @return Graphics settings director associated with the project, if any. */
            SmartPtr<scene::GraphicsSettingsDirector> getGraphicsSettingsDirector() const;

            /** @brief Loads project state from a serialized object. */
            void load( SmartPtr<ISharedObject> data ) override;
            /** @brief Releases any state owned by the provided serialized object. */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @brief Creates a new project at the provided path and applies defaults. */
            void create( const String &path );
            /** @brief Loads project data from a file on disk. */
            void loadFromFile( const String &filePath ) override;
            /** @brief Saves the project to a file on disk. */
            void saveToFile( const String &filePath ) override;
            /** @brief Saves the current project state using the configured project path. */
            void save() override;

            /** @return Human-readable project label. */
            String getLabel() const;
            /** @brief Sets the project label displayed in the editor UI. */
            void setLabel( const String &label );

            /** @return Absolute project directory path. */
            String getPath() const override;
            /** @brief Sets the project root directory. */
            void setPath( const String &projectDirectory ) override;

            /** @return Current working directory for the project. */
            String getWorkingDirectory() const;
            /** @brief Sets the working directory used during project execution and tooling. */
            void setWorkingDirectory( const String &workingDirectory );

            /** @return Absolute path to the application executable or entry point. */
            String getApplicationFilePath() const override;
            /** @brief Sets the path to the built application or launcher executable. */
            void setApplicationFilePath( const String &applicationFilePath ) override;

            /** @return All configured project search paths. */
            Array<String> getPaths() const;
            /** @brief Replaces the search path list used to resolve resources and assets. */
            void setPaths( const Array<String> &paths );
            /** @brief Adds a new project search path. */
            void addPath( const String &path );

            /** @brief Stores arbitrary project metadata as a property bag. */
            void setProperties( SmartPtr<Properties> properties ) override;
            /** @return Project metadata properties. */
            SmartPtr<Properties> getProperties() const override;

            /** @return Child objects belonging to this project resource. */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @return Media folders configured for the project. */
            Array<String> getMediaPaths() const;
            /** @brief Replaces the list of media folders relative to the project root. */
            void setMediaPaths( const Array<String> &mediaPaths );

            /** @return The selected project path in the UI or project browser. */
            String getSelectedProjectPath() const;
            /** @brief Stores the currently selected project path. */
            void setSelectedProjectPath( const String &selectedProjectPath );

            /** @brief Applies the default values for all project settings and paths. */
            void applyDefaults();

            /** @return Owning object for this project resource, if any. */
            SmartPtr<ISharedObject> getOwner() const;
            /** @brief Sets the owning object for this project resource. */
            void setOwner( SmartPtr<ISharedObject> owner );
            /**
             * @brief Compatibility alias for the historical misspelled setter.
             * @deprecated Use setOwner() instead.
             */
            void getOwner( SmartPtr<ISharedObject> owner );

            /** @return Path to the currently active scene file. */
            String getCurrentScenePath() const;
            /** @brief Sets the current scene path for the editor. */
            void setCurrentScenePath( const String &currentScenePath );

            /** @return Default serialized project data used for new instances. */
            SmartPtr<ISharedObject> getDefaultData() const;

            /** @brief Serializes the project into a data object. */
            SmartPtr<ISharedObject> toData() const override;

            /** @brief Restores the project from serialized data. */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Configures and builds the plugin target with CMake on PATH.
             *
             * Uses the existing CMakeLists.txt or the editor's configured ProjectManager.
             * Writes configure.log and build.log under Cache/project and throws on failure.
             */
            void compile();

            /** @return Header source generated for the project plugin. */
            String getPluginHeader() const;

            /** @return Source file generated for the project plugin. */
            String getPluginSource() const;

            /** @return Plugin instance associated with this project, if any. */
            SmartPtr<IPlugin> getPlugin() const override;

            /** @brief Assigns the plugin instance that should be used for this project. */
            void setPlugin( SmartPtr<IPlugin> plugin ) override;

            /** @return Script files included in the project. */
            Array<String> getScriptFilePaths() const override;

            /** @brief Replaces the script file list for this project. */
            void setScriptFilePaths( const Array<String> &scriptFilePaths ) override;

            /** @return Resource folders tracked by the project. */
            Array<String> getResourceFolders() const override;

            /** @brief Replaces the list of resource folders tracked by the project. */
            void setResourceFolders( const Array<String> &resourceFolders ) override;

            /** @return Application type identifier for the project. */
            String getApplicationType() const override;

            /** @brief Sets the application type identifier for this project. */
            void setApplicationType( const String &applicationType ) override;

            /** @return True if the project is stored in an archive such as a zip file. */
            bool isArchive() const override;

            /** @brief Marks whether the project is being loaded from an archive. */
            void setArchive( bool archive ) override;

            /** @return True if the project contains unsaved changes. */
            bool isDirty() const override;
            /** @brief Sets the dirty flag indicating whether the project needs to be saved. */
            void setDirty( bool dirty ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** @brief Owning editor object, if this project is attached to one. */
            WeakPtr<ISharedObject> m_owner;
            /** @brief Script file paths assembled for project compilation or runtime use. */
            Array<String> m_scriptFilePaths;
            /** @brief Resource folders that should be scanned for asset data. */
            Array<String> m_resourceFolders;
            /** @brief Logical application type for the project. */
            String m_applicationType;
            /** @brief Plugin instance associated with this project. */
            SmartPtr<IPlugin> m_plugin;

            /** @brief Graphics settings director used for rendering configuration. */
            SmartPtr<scene::GraphicsSettingsDirector> m_graphicsSettingsDirector;

            /** @brief True when the project is stored in an archive such as a zip file. */
            bool m_archive = false;

            /** @brief True when the project contains edits that still need to be saved. */
            bool m_dirty = false;

            /** @brief Path to the currently loaded scene file. */
            String m_currentScenePath;

            /** @brief Unique project identifier. */
            String m_uuid;

            /** @brief Product name associated with the packaged application. */
            String m_productName;
            /** @brief Company name associated with the packaged application. */
            String m_companyName;

            /** @brief Display name shown for the project in the editor. */
            String m_label;

            /** @brief Root directory of the project on disk. */
            String m_projectDirectory;

            /** @brief Current working directory used for project execution and tools. */
            String m_workingDirectory;

            /** @brief Full project path, including the project file if applicable. */
            String m_projectPath;

            /** @brief Project path selected in the UI or project browser. */
            String m_selectedProjectPath;

            /** @brief Application executable or entry point path. */
            String m_applicationFilePath;

            /** @brief Scripts directory relative to the project root. */
            String m_scriptsPath;

            /** @brief Entities directory relative to the project root. */
            String m_entitiesPath;

            /** @brief File format version of this project. */
            String m_version;

            /** @brief Media folders configured for the project. */
            Array<String> m_mediaPaths;

            /** @brief Additional project resource search paths. */
            Array<String> m_paths;

            /** @brief Arbitrary project metadata. */
            SmartPtr<Properties> m_properties;
        }; 
    }  // end namespace editor
}  // namespace workphone

#endif  // Project_h__
