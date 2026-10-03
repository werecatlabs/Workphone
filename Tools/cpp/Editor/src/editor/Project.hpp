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
        /** Editor project metadata and lifecycle. Mutations must run on the editor thread.
         * File operations and compilation throw on failure; successful loads/saves clear dirty.
         * Relative asset paths are stored relative to the project directory.
         */
        class Project : public Resource<IProject>
        {
        public:
            static const String DEFAULT_PROJECT_NAME;
            static const String DEFAULT_MEDIA_PATH;
            static const String DEFAULT_SCRIPTS_PATH;
            static const String DEFAULT_ENTITIES_PATH;
            static const String DEFAULT_VERSION;

            Project();
            ~Project() override;
            SmartPtr<scene::GraphicsSettingsDirector> getGraphicsSettingsDirector() const;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            void create( const String &path );
            void loadFromFile( const String &filePath ) override;
            void saveToFile( const String &filePath ) override;
            void save() override;

            String getLabel() const;
            void setLabel( const String &label );

            String getPath() const override;
            void setPath( const String &projectDirectory ) override;

            String getWorkingDirectory() const;
            void setWorkingDirectory( const String &workingDirectory );

            String getApplicationFilePath() const override;
            void setApplicationFilePath( const String &applicationFilePath ) override;

            Array<String> getPaths() const;
            void setPaths( const Array<String> &paths );
            void addPath( const String &path );

            void setProperties( SmartPtr<Properties> properties ) override;
            SmartPtr<Properties> getProperties() const override;

            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            Array<String> getMediaPaths() const;
            void setMediaPaths( const Array<String> &mediaPaths );

            String getSelectedProjectPath() const;
            void setSelectedProjectPath( const String &selectedProjectPath );

            void applyDefaults();

            SmartPtr<ISharedObject> getOwner() const;
            void setOwner( SmartPtr<ISharedObject> owner );
            /** Compatibility alias for the historical misspelled setter. */
            void getOwner( SmartPtr<ISharedObject> owner );

            String getCurrentScenePath() const;
            void setCurrentScenePath( const String &currentScenePath );

            SmartPtr<ISharedObject> getDefaultData() const;

            /** Get object data as a structure. */
            SmartPtr<ISharedObject> toData() const override;

            /** Set object data from a structure. */
            void fromData( SmartPtr<ISharedObject> data ) override;

            /** Build and reload the Plugin target using CMake on PATH.
             * Uses the existing CMakeLists.txt or the editor's configured ProjectManager.
             * Writes configure.log/build.log under Cache/project; throws on failure.
             */
            void compile();

            String getPluginHeader() const;

            String getPluginSource() const;

            SmartPtr<IPlugin> getPlugin() const override;

            void setPlugin( SmartPtr<IPlugin> plugin ) override;

            Array<String> getScriptFilePaths() const override;

            void setScriptFilePaths( const Array<String> &scriptFilePaths ) override;

            Array<String> getResourceFolders() const override;

            void setResourceFolders( const Array<String> &resourceFolders ) override;

            String getApplicationType() const override;

            void setApplicationType( const String &applicationType ) override;

            bool isArchive() const override;

            void setArchive( bool archive ) override;

            bool isDirty() const override;
            void setDirty( bool dirty ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            WeakPtr<ISharedObject> m_owner;
            Array<String> m_scriptFilePaths;
            Array<String> m_resourceFolders;
            String m_applicationType;
            SmartPtr<IPlugin> m_plugin;

            SmartPtr<scene::GraphicsSettingsDirector> m_graphicsSettingsDirector;

            // Used to know if the project is in an archive e.g. a zip file.
            bool m_archive = false;

            // To know if the user has changed the project an that is should be saved.
            bool m_dirty = false;

            String m_currentScenePath;

            String m_uuid;

            String m_productName;
            String m_companyName;

            ///
            String m_label;

            /// The working directory
            String m_projectDirectory;

            /// The working directory
            String m_workingDirectory;

            ///
            String m_projectPath;

            String m_selectedProjectPath;

            ///
            String m_applicationFilePath;

            /// The scripts path.
            String m_scriptsPath;

            ///
            String m_entitiesPath;

            ///
            String m_version;

            /// The media path.
            Array<String> m_mediaPaths;

            ///
            Array<String> m_paths;

            SmartPtr<Properties> m_properties;
        };
    }  // end namespace editor
}  // namespace workphone

#endif  // Project_h__
