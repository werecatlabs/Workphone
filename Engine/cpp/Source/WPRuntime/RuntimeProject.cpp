#include <WPRuntime/WPRuntimePCH.hpp>
#include <WPRuntime/RuntimeProject.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Core/DataUtil.hpp>

namespace workphone
{
    RuntimeProject::RuntimeProject() : m_applicationType( "lua" ), m_isArchive( false )
    {
    }

    RuntimeProject::~RuntimeProject() = default;

    void RuntimeProject::load( const String &filePath )
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            WP_EXCEPTION( "Project file path is empty." );
        }

        const auto extension = StringUtil::make_lower( Path::getFileExtension( filePath ) );
        if( extension == ".fba" )
        {
            auto applicationManager = core::ApplicationManager::instance();
            WP_ASSERT( applicationManager );
            if( !applicationManager )
            {
                WP_EXCEPTION( "Application manager is not available." );
            }

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );
            if( !fileSystem )
            {
                WP_EXCEPTION( "File system is not available." );
            }

            fileSystem->addArchive( filePath );

            Array<String> fileNames;
            fileSystem->getFileNamesWithExtension( ".fbproject", fileNames );
            if( fileNames.empty() )
            {
                fileSystem->getFileNamesWithExtension( ".fbp", fileNames );
            }

            if( !fileNames.empty() )
            {
                loadProjectFile( fileNames[0] );
            }
            else
            {
                WP_EXCEPTION( "No project file found." );
            }

            setArchive( true );
        }
        else
        {
            loadProjectFile( filePath );
            setArchive( false );
        }
    }

    void RuntimeProject::loadProjectFile( const String &filePath )
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            WP_EXCEPTION( "Project file path is empty." );
        }

        m_projectFilePath = filePath;
        m_path = Path::getFilePath( filePath );

        auto dataStr = String();
        auto applicationManager = core::ApplicationManager::instancePtr();
        if( applicationManager )
        {
            if( auto fileSystem = applicationManager->getFileSystemPtr() )
            {
                dataStr = fileSystem->readAllText( filePath );
            }
        }

        if( StringUtil::isNullOrEmpty( dataStr ) && Path::isExistingFile( filePath ) )
        {
            dataStr = Path::readAllText( filePath );
        }

        WP_ASSERT( !StringUtil::isNullOrEmpty( dataStr ) );
        if( StringUtil::isNullOrEmpty( dataStr ) )
        {
            WP_EXCEPTION( "Project file is empty or could not be read." );
        }

        auto properties = workphone::make_ptr<Properties>();
        DataUtil::parse( dataStr, properties.get(), DataFormat::JSON );
        WP_ASSERT( properties );

        String path;
        if( properties->getPropertyValue( "projectPath", path ) ||
            properties->getPropertyValue( "ProjectPath", path ) ||
            properties->getPropertyValue( "Path", path ) )
        {
            m_path = path;
        }

        if( StringUtil::isNullOrEmpty( m_path ) )
        {
            m_path = Path::getFilePath( filePath );
        }

        properties->getPropertyValue( "applicationFilePath", m_applicationFilePath );
        if( StringUtil::isNullOrEmpty( m_applicationFilePath ) )
        {
            properties->getPropertyValue( "ApplicationFilePath", m_applicationFilePath );
        }

        properties->getPropertyValue( "currentScenePath", m_sceneFilePath );
        if( StringUtil::isNullOrEmpty( m_sceneFilePath ) )
        {
            properties->getPropertyValue( "sceneFilePath", m_sceneFilePath );
        }
        if( StringUtil::isNullOrEmpty( m_sceneFilePath ) )
        {
            properties->getPropertyValue( "SceneFilePath", m_sceneFilePath );
        }

        properties->getPropertyValue( "scriptFilePaths", m_scriptFilePaths );
        if( m_scriptFilePaths.empty() )
        {
            properties->getPropertyValue( "ScriptFilePaths", m_scriptFilePaths );
        }

        properties->getPropertyValue( "resourceFolders", m_resourceFolders );
        if( m_resourceFolders.empty() )
        {
            properties->getPropertyValue( "ResourceFolders", m_resourceFolders );
        }

        auto applicationType = String();
        if( properties->getPropertyValue( "applicationType", applicationType ) ||
            properties->getPropertyValue( "ApplicationType", applicationType ) )
        {
            setApplicationType( applicationType );
        }

        auto archive = m_isArchive;
        if( properties->getPropertyValue( "archive", archive ) ||
            properties->getPropertyValue( "Archive", archive ) )
        {
            setArchive( archive );
        }

        setFilePath( filePath );
    }

    String RuntimeProject::getApplicationFilePath() const
    {
        return m_applicationFilePath;
    }

    void RuntimeProject::setApplicationFilePath( const String &applicationFilePath )
    {
        m_applicationFilePath = applicationFilePath;
    }

    String RuntimeProject::getPath() const
    {
        return m_path;
    }

    void RuntimeProject::setPath( const String &projectPath )
    {
        m_path = projectPath;
    }

    Array<String> RuntimeProject::getScriptFilePaths() const
    {
        return m_scriptFilePaths;
    }

    void RuntimeProject::setScriptFilePaths( const Array<String> &scriptFilePaths )
    {
        m_scriptFilePaths = scriptFilePaths;
    }

    Array<String> RuntimeProject::getResourceFolders() const
    {
        return m_resourceFolders;
    }

    void RuntimeProject::setResourceFolders( const Array<String> &resourceFolders )
    {
        m_resourceFolders = resourceFolders;
    }

    String RuntimeProject::getApplicationType() const
    {
        return m_applicationType;
    }

    void RuntimeProject::setApplicationType( const String &applicationType )
    {
        m_applicationType = applicationType;
    }

    String RuntimeProject::getSceneFilePath() const
    {
        return m_sceneFilePath;
    }

    void RuntimeProject::setSceneFilePath( const String &sceneFilePath )
    {
        m_sceneFilePath = sceneFilePath;
    }

    bool RuntimeProject::isArchive() const
    {
        return m_isArchive;
    }

    void RuntimeProject::setArchive( bool archive )
    {
        m_isArchive = archive;
    }

    bool RuntimeProject::isDirty() const
    {
        return m_isDirty;
    }

    void RuntimeProject::setDirty( bool dirty )
    {
        m_isDirty = dirty;
    }

    SmartPtr<IPlugin> RuntimeProject::getPlugin() const
    {
        return m_plugin;
    }

    void RuntimeProject::setPlugin( SmartPtr<IPlugin> plugin )
    {
        m_plugin = plugin;
    }

    SmartPtr<core::IPrototype> RuntimeProject::getParentPrototype() const
    {
        return m_parentPrototype;
    }

    void RuntimeProject::setParentPrototype( SmartPtr<core::IPrototype> prototype )
    {
        m_parentPrototype = prototype;
    }

    SmartPtr<Properties> RuntimeProject::getProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();
        WP_ASSERT( properties );
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( "projectPath", m_path );
        properties->setProperty( "applicationFilePath", m_applicationFilePath );
        properties->setProperty( "currentScenePath", m_sceneFilePath );
        properties->setProperty( "scriptFilePaths", m_scriptFilePaths );
        properties->setProperty( "resourceFolders", m_resourceFolders );
        properties->setProperty( "applicationType", m_applicationType );
        properties->setProperty( "archive", m_isArchive );
        properties->setProperty( "dirty", m_isDirty );

        return properties;
    }

    void RuntimeProject::setProperties( SmartPtr<Properties> properties )
    {
        WP_ASSERT( properties );
        if( !properties )
        {
            return;
        }

        properties->getPropertyValue( "projectPath", m_path );
        if( StringUtil::isNullOrEmpty( m_path ) )
        {
            properties->getPropertyValue( "ProjectPath", m_path );
        }

        properties->getPropertyValue( "applicationFilePath", m_applicationFilePath );
        if( StringUtil::isNullOrEmpty( m_applicationFilePath ) )
        {
            properties->getPropertyValue( "ApplicationFilePath", m_applicationFilePath );
        }

        properties->getPropertyValue( "currentScenePath", m_sceneFilePath );
        if( StringUtil::isNullOrEmpty( m_sceneFilePath ) )
        {
            properties->getPropertyValue( "sceneFilePath", m_sceneFilePath );
        }
        if( StringUtil::isNullOrEmpty( m_sceneFilePath ) )
        {
            properties->getPropertyValue( "SceneFilePath", m_sceneFilePath );
        }

        properties->getPropertyValue( "scriptFilePaths", m_scriptFilePaths );
        if( m_scriptFilePaths.empty() )
        {
            properties->getPropertyValue( "ScriptFilePaths", m_scriptFilePaths );
        }

        properties->getPropertyValue( "resourceFolders", m_resourceFolders );
        if( m_resourceFolders.empty() )
        {
            properties->getPropertyValue( "ResourceFolders", m_resourceFolders );
        }

        properties->getPropertyValue( "applicationType", m_applicationType );
        if( StringUtil::isNullOrEmpty( m_applicationType ) )
        {
            properties->getPropertyValue( "ApplicationType", m_applicationType );
        }

        properties->getPropertyValue( "archive", m_isArchive );
        properties->getPropertyValue( "dirty", m_isDirty );
    }

    void RuntimeProject::saveToFile( const String &filePath )
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            WP_EXCEPTION( "Project file path is empty." );
        }

        auto properties = getProperties();
        const auto dataStr = DataUtil::toString( properties.get(), true, DataFormat::JSON );
        Path::writeAllText( filePath, dataStr );
        setFilePath( filePath );
        setDirty( false );
    }

    void RuntimeProject::loadFromFile( const String &filePath )
    {
        load( filePath );
    }

    void RuntimeProject::save()
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( m_projectFilePath ) );
        if( StringUtil::isNullOrEmpty( m_projectFilePath ) )
        {
            WP_EXCEPTION( "Project file path is empty." );
        }

        saveToFile( m_projectFilePath );
    }

    void RuntimeProject::import()
    {
    }

    void RuntimeProject::reimport()
    {
        if( !StringUtil::isNullOrEmpty( m_projectFilePath ) )
        {
            load( m_projectFilePath );
        }
    }

    UUID RuntimeProject::getFileSystemId() const
    {
        return m_fileSystemId;
    }

    void RuntimeProject::setFileSystemId( UUID id )
    {
        m_fileSystemId = id;
    }

    String RuntimeProject::getFilePath() const
    {
        return m_projectFilePath;
    }

    void RuntimeProject::setFilePath( const String &filePath )
    {
        m_projectFilePath = filePath;
    }

    UUID RuntimeProject::getSettingsFileSystemId() const
    {
        return m_settingsFileSystemId;
    }

    void RuntimeProject::setSettingsFileSystemId( UUID id )
    {
        m_settingsFileSystemId = id;
    }

    void RuntimeProject::_getObject( void **ppObject ) const
    {
        WP_ASSERT( ppObject );
        if( ppObject )
        {
            *ppObject = const_cast<RuntimeProject *>( this );
        }
    }

    Array<SmartPtr<IResource>> RuntimeProject::getDependencies() const
    {
        return {};
    }

    IResourceManager *RuntimeProject::getResourceManagerPtr() const
    {
        return m_resourceManager.get();
    }

    SmartPtr<IResourceManager> RuntimeProject::getResourceManager() const
    {
        return m_resourceManager;
    }

    void RuntimeProject::setResourceManager( SmartPtr<IResourceManager> resourceManager )
    {
        m_resourceManager = resourceManager;
    }

    IStateContext *RuntimeProject::getStateContextPtr() const
    {
        return nullptr;
    }

    SmartPtr<IStateContext> RuntimeProject::getStateContext() const
    {
        return nullptr;
    }

    bool RuntimeProject::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool RuntimeProject::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }
}  // namespace workphone
