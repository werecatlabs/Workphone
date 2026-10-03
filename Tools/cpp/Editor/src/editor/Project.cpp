#include <EditorPCH.hpp>
#include <editor/Project.hpp>
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cctype>
#if defined(_WIN32)
#    include <windows.h>
#else
#    include <spawn.h>
#    include <sys/wait.h>
#    include <signal.h>
#    include <unistd.h>
#    include <cerrno>
#    include <fcntl.h>
extern char **environ;
#endif

namespace workphone::editor
{
    namespace
    {
        namespace fs = std::filesystem;

        SmartPtr<Properties> copyProperties( const Properties &source )
        {
            auto result = workphone::make_ptr<Properties>();
            result->setName( source.getName() );
            for( const auto &property : source.getPropertiesAsArray() ) result->addProperty( property );
            for( auto child : source.getChildren() )
                if( child ) result->addChild( copyProperties( *child ) );
            return result;
        }

        fs::path checkedPath( const String &value )
        {
            if( value.empty() || value.find( '\0' ) != String::npos )
                throw std::invalid_argument( "Project path is empty or contains a null character" );
            return fs::absolute( fs::u8path( value.c_str() ) ).lexically_normal();
        }

        Array<String> cleanPaths( const Array<String> &paths )
        {
            Array<String> result;
            for( const auto &path : paths )
            {
                if( path.empty() )
                    continue;
                auto clean = StringUtil::cleanupPath( path );
                if( std::find( result.begin(), result.end(), clean ) == result.end() )
                    result.push_back( clean );
            }
            return result;
        }

        // Keep legacy flat arrays, but use child entries for lossless filenames (including ';').
        void writePaths( Properties &properties, const String &key, const Array<String> &paths )
        {
            properties.setProperty( key, paths );
            properties.removeChild( key + "Entries" );
            auto group = workphone::make_ptr<Properties>();
            group->setName( key + "Entries" );
            for( const auto &path : paths )
            {
                auto entry = workphone::make_ptr<Properties>();
                entry->setName( "path" );
                entry->setProperty( "pathValue", path );
                group->addChild( entry );
            }
            properties.addChild( group );
        }

        void readPaths( const Properties &properties, const String &key, Array<String> &paths )
        {
            if( auto group = properties.getChild( key + "Entries" ) )
            {
                paths.clear();
                for( auto entry : group->getChildren() )
                {
                    String path;
                    if( entry && entry->getPropertyValue( "pathValue", path ) ) paths.push_back( path );
                }
            }
            else if( properties.hasProperty( key ) )
            {
                paths.clear();
                properties.getPropertyValue( key, paths );
            }
        }

        // Write next to the destination so replacement stays on the same filesystem.
        void writeProjectFile( const fs::path &destination, const String &text )
        {
            fs::create_directories( destination.parent_path() );
            auto temporary = destination;
            temporary += std::string( String( "." + StringUtil::getUUID() + ".tmp" ).c_str() );
            try
            {
                std::ofstream stream( temporary, std::ios::binary | std::ios::trunc );
                stream.exceptions( std::ios::badbit | std::ios::failbit );
                stream.write( text.data(), static_cast<std::streamsize>( text.size() ) );
                stream.flush();
                stream.close();
#if defined(_WIN32)
                if( !MoveFileExW( temporary.c_str(), destination.c_str(),
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                    throw std::runtime_error( "Unable to replace project file: " + destination.u8string() );
#else
                fs::rename( temporary, destination );
#endif
            }
            catch( ... )
            {
                std::error_code ignored;
                fs::remove( temporary, ignored );
                throw;
            }
        }

        // Invoke CMake directly, wait for completion and check its exit code.
        // Never change the editor process's working directory or invoke a shell.
        void runCMake( const Array<String> &args, const fs::path &logPath )
        {
            fs::create_directories( logPath.parent_path() );
#if defined(_WIN32)
            struct Handle
            {
                HANDLE value;
                ~Handle() { if( value && value != INVALID_HANDLE_VALUE ) CloseHandle( value ); }
            };
            SECURITY_ATTRIBUTES security = { sizeof( SECURITY_ATTRIBUTES ), nullptr, TRUE };
            Handle output{ CreateFileW( logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security,
                                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr ) };
            Handle input{ CreateFileW( L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr ) };
            if( output.value == INVALID_HANDLE_VALUE || input.value == INVALID_HANDLE_VALUE )
                throw std::runtime_error( "Unable to open CMake build log" );
            Handle job{ CreateJobObjectW( nullptr, nullptr ) };
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
            limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            if( !job.value || !SetInformationJobObject( job.value, JobObjectExtendedLimitInformation,
                                                        &limits, sizeof( limits ) ) )
                throw std::runtime_error( "Unable to create CMake process job" );
            StringW command = L"cmake.exe";
            for( const auto &arg : args )
            {
                auto wide = StringUtil::toUTF8to16( arg );
                command += L" \"";
                size_t slashes = 0;
                for( auto c : wide )
                {
                    if( c == L'\\' ) { ++slashes; continue; }
                    command.append( slashes * ( c == L'\"' ? 2 : 1 ), L'\\' );
                    slashes = 0;
                    if( c == L'\"' ) command += L'\\';
                    command += c;
                }
                command.append( slashes * 2, L'\\' );
                command += L'\"';
            }
            STARTUPINFOW startup = {};
            startup.cb = sizeof( startup );
            startup.dwFlags = STARTF_USESTDHANDLES;
            startup.hStdOutput = output.value;
            startup.hStdError = output.value;
            startup.hStdInput = input.value;
            PROCESS_INFORMATION process = {};
            if( !CreateProcessW( nullptr, command.data(), nullptr, nullptr, TRUE,
                                 CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &startup, &process ) )
                throw std::runtime_error( "Unable to start CMake. Ensure it is installed and on PATH." );
            if( !AssignProcessToJobObject( job.value, process.hProcess ) )
            {
                TerminateProcess( process.hProcess, 1 );
                CloseHandle( process.hThread );
                CloseHandle( process.hProcess );
                throw std::runtime_error( "Unable to manage CMake process lifetime" );
            }
            ResumeThread( process.hThread );
            CloseHandle( process.hThread );
            const auto wait = WaitForSingleObject( process.hProcess, 10 * 60 * 1000 );
            DWORD code = 1;
            const bool completed = wait == WAIT_OBJECT_0 && GetExitCodeProcess( process.hProcess, &code );
            if( !completed )
            {
                TerminateJobObject( job.value, 1 );
                WaitForSingleObject( process.hProcess, 5000 );
            }
            CloseHandle( process.hProcess );
            if( !completed || code != 0 )
                throw std::runtime_error( "CMake failed or timed out (exit " + std::to_string( code ) +
                                          "). See " + logPath.u8string() );
#else
            std::vector<String> strings = { "cmake" };
            for( const auto &arg : args ) strings.push_back( arg );
            std::vector<char *> argv;
            for( auto &arg : strings ) argv.push_back( arg.data() );
            argv.push_back( nullptr );
            posix_spawn_file_actions_t actions;
            posix_spawnattr_t attributes;
            if( posix_spawn_file_actions_init( &actions ) != 0 )
                throw std::runtime_error( "Unable to initialize CMake output redirection" );
            if( posix_spawnattr_init( &attributes ) != 0 )
            {
                posix_spawn_file_actions_destroy( &actions );
                throw std::runtime_error( "Unable to initialize CMake process attributes" );
            }
            int error = posix_spawn_file_actions_addopen( &actions, STDOUT_FILENO, logPath.c_str(),
                                                          O_WRONLY | O_CREAT | O_TRUNC, 0600 );
            if( !error ) error = posix_spawn_file_actions_adddup2( &actions, STDOUT_FILENO, STDERR_FILENO );
            if( !error ) error = posix_spawnattr_setflags( &attributes, POSIX_SPAWN_SETPGROUP );
            if( !error ) error = posix_spawnattr_setpgroup( &attributes, 0 );
            pid_t pid;
            if( !error ) error = posix_spawnp( &pid, "cmake", &actions, &attributes, argv.data(), environ );
            posix_spawn_file_actions_destroy( &actions );
            posix_spawnattr_destroy( &attributes );
            if( error )
                throw std::runtime_error( "Unable to start CMake" );
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::minutes( 10 );
            int status = 0;
            for( ;; )
            {
                const auto result = waitpid( pid, &status, WNOHANG );
                if( result == pid ) break;
                if( result < 0 && errno != EINTR )
                    throw std::runtime_error( "Unable to wait for CMake" );
                if( std::chrono::steady_clock::now() >= deadline )
                {
                    kill( -pid, SIGKILL );
                    while( waitpid( pid, &status, 0 ) < 0 && errno == EINTR ) {}
                    throw std::runtime_error( "CMake timed out. See " + logPath.u8string() );
                }
                std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            }
            if( !WIFEXITED( status ) || WEXITSTATUS( status ) != 0 )
                throw std::runtime_error( "CMake failed. See " + logPath.u8string() );
#endif
        }
    }

    WP_CLASS_REGISTER_DERIVED( workphone::editor, Project, Resource<IProject> );

    const String Project::DEFAULT_PROJECT_NAME = "Project";
    const String Project::DEFAULT_MEDIA_PATH = "../Media/";
    const String Project::DEFAULT_SCRIPTS_PATH = "../Media/Scripts/";
    const String Project::DEFAULT_ENTITIES_PATH = "../Media/Scripts/Entities/";
    const String Project::DEFAULT_VERSION = "1.0.0";

    Project::Project() :
        m_label( StringUtil::EmptyString ),
        m_projectDirectory( StringUtil::EmptyString ),
        m_projectPath( StringUtil::EmptyString ),
        m_applicationFilePath( StringUtil::EmptyString ),
        m_scriptsPath( StringUtil::EmptyString ),
        m_entitiesPath( StringUtil::EmptyString ),
        m_version( StringUtil::EmptyString )
    {
        m_graphicsSettingsDirector = workphone::make_ptr<scene::GraphicsSettingsDirector>();
        applyDefaults();
        m_dirty = false;
    }

    Project::~Project()
    {
        unload( nullptr );
    }

    SmartPtr<scene::GraphicsSettingsDirector> Project::getGraphicsSettingsDirector() const
    {
        return m_graphicsSettingsDirector;
    }

    void Project::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() ) return;
        setLoadingState( LoadingState::Loading );
        try
        {
            if( !m_properties ) applyDefaults();
            Resource<IProject>::load( data );
            setLoadingState( LoadingState::Loaded );
        }
        catch( ... )
        {
            setLoadingState( LoadingState::Unloaded );
            throw;
        }
    }

    void Project::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );
            if( m_plugin && core::IApplicationManager::instancePtr() )
            {
                auto job = workphone::make_ptr<UnloadPluginJob>();
                job->setPlugin( m_plugin );
                job->execute();
            }
            m_plugin = nullptr;
            Resource<IProject>::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( const std::exception &e )
        {
            setLoadingState( LoadingState::Unloaded );
            WP_LOG_EXCEPTION( e );
        }
    }

    void Project::create( const String &path )
    {
        const auto directory = checkedPath( path );
        const auto destination = directory / "project.fbproject";
        if( fs::exists( destination ) )
            throw std::runtime_error( "A project already exists in this directory" );
        for( const auto &folder : { "Assets", "Cache", "Engine", "Plugin" } )
            fs::create_directories( directory / folder );
        // Preserve user-owned plugin sources in an existing directory.
        if( !fs::exists( directory / "Plugin/Plugin.hpp" ) )
            writeProjectFile( directory / "Plugin/Plugin.hpp", getPluginHeader() );
        if( !fs::exists( directory / "Plugin/Plugin.cpp" ) )
            writeProjectFile( directory / "Plugin/Plugin.cpp", getPluginSource() );
        applyDefaults();
        saveToFile( destination.u8string() );
        if( auto editor = EditorManager::getSingletonPtr() )
            if( auto ui = editor->getUI() ) ui->rebuildResourceTree();
    }

    void Project::loadFromFile( const String &filePath )
    {
        const auto destination = checkedPath( filePath );
        std::ifstream stream( destination, std::ios::binary );
        if( !stream ) throw std::runtime_error( "Unable to open project: " + filePath );
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        if( stream.bad() ) throw std::runtime_error( "Unable to read project: " + filePath );
        auto properties = workphone::make_ptr<Properties>();
        DataUtil::parse( buffer.str(), properties.get() );
        // DataUtil can log malformed JSON without throwing. Reject incomplete documents.
        if( !properties->hasProperty( "projectVersion" ) && !properties->hasProperty( "version" ) )
            throw std::runtime_error( "Invalid project document: missing projectVersion" );
        if( m_plugin )
        {
            if( core::IApplicationManager::instancePtr() )
            {
                auto job = workphone::make_ptr<UnloadPluginJob>();
                job->setPlugin( m_plugin );
                job->execute();
            }
            m_plugin = nullptr;
        }
        fromData( properties );
        m_projectDirectory = destination.parent_path().u8string();
        m_projectPath = destination.u8string();
        m_selectedProjectPath = m_projectPath;
        setFilePath( m_projectPath );
        m_dirty = false;

        if( auto app = core::IApplicationManager::instancePtr() )
        {
            app->setProjectPath( m_projectDirectory );
            app->setCachePath( m_projectDirectory + "/Cache" );
            app->setSettingsPath( m_projectDirectory + "/SettingsCache" );
            auto fileSystem = app->getFileSystem();
            if( fileSystem ) fileSystem->addFolder( m_projectDirectory, true );
            if( fileSystem )
            {
                auto folders = m_resourceFolders;
                folders.insert( folders.end(), m_mediaPaths.begin(), m_mediaPaths.end() );
                folders.insert( folders.end(), m_paths.begin(), m_paths.end() );
                for( const auto &folder : cleanPaths( folders ) )
                {
                    const auto resolved = destination.parent_path() / fs::u8path( folder.c_str() );
                    if( fs::is_directory( resolved ) ) fileSystem->addFolder( resolved.u8string(), true );
                }
            }
            if( auto scripts = app->getScriptManager() )
            {
                auto files = m_scriptFilePaths;
                for( auto &file : files )
                    file = ( destination.parent_path() / fs::u8path( file.c_str() ) ).lexically_normal().u8string();
                if( files.empty() && fileSystem )
                {
#if WP_ENABLE_LUA
                    files = fileSystem->getFileNamesWithExtension( ".lua" );
#elif WP_ENABLE_PYTHON
                    files = fileSystem->getFileNamesWithExtension( ".py" );
#endif
                }
                if( !files.empty() ) scripts->loadScripts( files );
            }
            app->setProjectLibraryName( "Plugin" );
            const auto library = app->getProjectLibraryPath();
            if( !library.empty() && fs::is_regular_file( fs::u8path( library.c_str() ) ) )
            {
                auto job = workphone::make_ptr<LoadPluginJob>();
                job->setPluginPath( library );
                job->execute();
                setPlugin( job->getPlugin() );
            }
            if( auto scenes = app->getGameManager() )
            {
                if( !m_currentScenePath.empty() ) scenes->loadScene( m_currentScenePath );
                scenes->edit();
            }
        }
    }

    void Project::saveToFile( const String &filePath )
    {
        if( m_archive ) throw std::runtime_error( "Extract the archived project before saving" );
        const auto destination = checkedPath( filePath );
        auto properties = workphone::static_pointer_cast<Properties>( toData() );
        writeProjectFile( destination, DataUtil::toString( properties.get(), true ) );
        writeProjectFile( destination.parent_path() / "GameGraphics.settings",
                          DataUtil::toString( m_graphicsSettingsDirector->getProperties().get(), true ) );
        m_projectDirectory = destination.parent_path().u8string();
        m_projectPath = destination.u8string();
        m_selectedProjectPath = m_projectPath;
        setFilePath( m_projectPath );
        m_dirty = false;
    }

    void Project::save()
    {
        auto filePath = getFilePath();
        if( filePath.empty() )
        {
            if( m_projectDirectory.empty() )
                throw std::runtime_error( "Set a project directory before saving" );
            filePath = ( fs::u8path( m_projectDirectory.c_str() ) / "project.fbproject" ).u8string();
        }
        saveToFile( filePath );
    }

    String Project::getLabel() const
    {
        return m_label;
    }

    void Project::setLabel( const String &label )
    {
        if( m_label != label )
        {
            m_label = label;
            m_dirty = true;
        }
    }

    String Project::getPath() const
    {
        return m_projectDirectory;
    }

    void Project::setPath( const String &projectDirectory )
    {
        if( m_projectDirectory != projectDirectory )
        {
            m_projectDirectory = projectDirectory;
            m_dirty = true;
        }
    }

    String Project::getWorkingDirectory() const
    {
        return m_workingDirectory;
    }

    void Project::setWorkingDirectory( const String &workingDirectory )
    {
        if( m_workingDirectory != workingDirectory )
        {
            m_workingDirectory = workingDirectory;
            m_dirty = true;
        }
    }

    String Project::getApplicationFilePath() const
    {
        return m_applicationFilePath;
    }

    void Project::setApplicationFilePath( const String &applicationFilePath )
    {
        if( m_applicationFilePath != applicationFilePath )
        {
            m_applicationFilePath = applicationFilePath;
            m_dirty = true;
        }
    }

    Array<String> Project::getPaths() const
    {
        return m_paths;
    }

    void Project::setPaths( const Array<String> &paths )
    {
        auto normalized = cleanPaths( paths );
        if( m_paths != normalized )
        {
            m_paths = normalized;
            m_dirty = true;
        }
    }

    void Project::addPath( const String &path )
    {
        auto paths = m_paths;
        paths.push_back( path );
        setPaths( paths );
    }

    void Project::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties ) throw std::invalid_argument( "Project properties cannot be null" );
        auto copy = copyProperties( *properties );
        // Keep resource metadata handling, but commit our copy after lifecycle callbacks.
        Resource<IProject>::setProperties( copy );
        m_properties = copy;
        if( auto graphics = copy->getChild( "Game Graphics" ) )
        {
            // Restore the preset first, then the saved per-effect overrides.
            m_graphicsSettingsDirector->setProperties( graphics );
            m_graphicsSettingsDirector->setProperties( graphics );
        }
        else
        {
            m_graphicsSettingsDirector = workphone::make_ptr<scene::GraphicsSettingsDirector>();
        }
        if( !copy->getPropertyValue( "projectVersion", m_version ) )
            copy->getPropertyValue( "version", m_version );
        copy->getPropertyValue( "uuid", m_uuid );
        copy->getPropertyValue( "label", m_label );
        copy->getPropertyValue( "productName", m_productName );
        copy->getPropertyValue( "companyName", m_companyName );
        copy->getPropertyValue( "currentScenePath", m_currentScenePath );
        copy->getPropertyValue( "workingDirectory", m_workingDirectory );
        copy->getPropertyValue( "applicationFilePath", m_applicationFilePath );
        copy->getPropertyValue( "applicationType", m_applicationType );
        copy->getPropertyValue( "scriptsPath", m_scriptsPath );
        copy->getPropertyValue( "entitiesPath", m_entitiesPath );
        copy->getPropertyValue( "archive", m_archive );
        readPaths( *copy, "paths", m_paths );
        readPaths( *copy, "mediaPaths", m_mediaPaths );
        readPaths( *copy, "scriptFilePaths", m_scriptFilePaths );
        readPaths( *copy, "resourceFolders", m_resourceFolders );
        if( m_uuid.empty() ) m_uuid = StringUtil::getUUID();
        if( m_version.empty() ) m_version = DEFAULT_VERSION;
        m_paths = cleanPaths( m_paths );
        m_mediaPaths = cleanPaths( m_mediaPaths );
        m_scriptFilePaths = cleanPaths( m_scriptFilePaths );
        m_resourceFolders = cleanPaths( m_resourceFolders );
        m_currentScenePath = StringUtil::cleanupPath( m_currentScenePath );
        m_dirty = true;
    }

    SmartPtr<Properties> Project::getProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();
        if( m_properties )
        {
            // Preserve custom properties and children.
            properties = copyProperties( *m_properties );
        }
        properties->setProperty( IResource::nameStr, getName() );
        properties->setProperty( "projectVersion", m_version );
        properties->setProperty( "version", m_version );
        properties->setProperty( "uuid", m_uuid );
        properties->setProperty( "label", m_label );
        properties->setProperty( "productName", m_productName );
        properties->setProperty( "companyName", m_companyName );
        properties->setProperty( "currentScenePath", m_currentScenePath );
        properties->setProperty( "workingDirectory", m_workingDirectory );
        properties->setProperty( "applicationFilePath", m_applicationFilePath );
        properties->setProperty( "applicationType", m_applicationType );
        properties->setProperty( "scriptsPath", m_scriptsPath );
        properties->setProperty( "entitiesPath", m_entitiesPath );
        properties->setProperty( "archive", m_archive );
        writePaths( *properties, "paths", m_paths );
        writePaths( *properties, "mediaPaths", m_mediaPaths );
        writePaths( *properties, "scriptFilePaths", m_scriptFilePaths );
        writePaths( *properties, "resourceFolders", m_resourceFolders );
        auto graphics = m_graphicsSettingsDirector->getProperties();
        graphics->setName( "Game Graphics" );
        properties->removeChild( "Game Graphics" );
        properties->addChild( graphics );
        return properties;
    }

    Array<SmartPtr<ISharedObject>> Project::getChildObjects() const
    {
        Array<SmartPtr<ISharedObject>> childObjects;
        childObjects.reserve( 12 );

        childObjects.emplace_back( m_graphicsSettingsDirector );

        return childObjects;
    }

    String Project::getSelectedProjectPath() const
    {
        return m_selectedProjectPath;
    }

    void Project::setSelectedProjectPath( const String &selectedProjectPath )
    {
        m_selectedProjectPath = selectedProjectPath;
    }

    void Project::applyDefaults()
    {
        m_graphicsSettingsDirector = workphone::make_ptr<scene::GraphicsSettingsDirector>();
        m_properties = workphone::static_pointer_cast<Properties>( getDefaultData() );
        m_properties->getPropertyValue( "uuid", m_uuid );
        m_label = DEFAULT_PROJECT_NAME;
        m_version = DEFAULT_VERSION;
        m_productName = "Untitled";
        m_companyName = "Untitled";
        m_scriptsPath = DEFAULT_SCRIPTS_PATH;
        m_entitiesPath = DEFAULT_ENTITIES_PATH;
        m_mediaPaths = { DEFAULT_MEDIA_PATH };
        m_resourceFolders = { "Assets" };
        m_paths.clear();
        m_scriptFilePaths.clear();
        m_applicationType = "Desktop";
        m_applicationFilePath.clear();
        m_currentScenePath.clear();
        m_workingDirectory.clear();
        m_archive = false;
        m_dirty = true;
    }

    SmartPtr<ISharedObject> Project::getOwner() const
    {
        return m_owner.lock();
    }

    void Project::getOwner( SmartPtr<ISharedObject> owner )
    {
        setOwner( owner );
    }

    void Project::setOwner( SmartPtr<ISharedObject> owner )
    {
        m_owner = owner;
    }

    String Project::getCurrentScenePath() const
    {
        return m_currentScenePath;
    }

    void Project::setCurrentScenePath( const String &currentScenePath )
    {
        if( m_currentScenePath != currentScenePath )
        {
            m_currentScenePath = currentScenePath;
            m_dirty = true;
        }
    }

    SmartPtr<ISharedObject> Project::getDefaultData() const
    {
        auto properties = workphone::make_ptr<Properties>();

        auto uuid = StringUtil::getUUID();
        auto productName = String( "Untitled" );
        auto companyName = String( "Untitled" );
        auto currentScenePath = String();

        properties->setProperty( "projectVersion", "1.0.0" );
        properties->setProperty( "uuid", uuid );
        properties->setProperty( "productName", productName );
        properties->setProperty( "companyName", companyName );
        properties->setProperty( "currentScenePath", currentScenePath );

        return properties;
    }

    SmartPtr<ISharedObject> Project::toData() const
    {
        return getProperties();
    }

    void Project::fromData( SmartPtr<ISharedObject> data )
    {
        auto properties = workphone::dynamic_pointer_cast<Properties>( data );
        if( !properties ) throw std::invalid_argument( "Project data must contain Properties" );
        // Reset absent legacy fields to defaults, never to values from a previous project.
        applyDefaults();
        setProperties( properties );
        m_dirty = false;
    }

    void Project::compile()
    {
        if( m_archive ) throw std::runtime_error( "Extract the archived project before compiling" );
        const auto directory = checkedPath( m_projectDirectory );
        const auto buildDirectory = directory / "Cache/project";
        auto app = core::IApplicationManager::instancePtr();
        if( !app ) throw std::runtime_error( "Compilation requires the application manager" );
        if( !fs::exists( directory / "CMakeLists.txt" ) )
        {
            if( !app->getFileSystem() )
                throw std::runtime_error( "Generating a project requires the application file system" );
            if( checkedPath( app->getProjectPath() ) != directory )
                throw std::runtime_error( "The application project path does not match this project" );
            SmartPtr<ProjectManager> manager;
            if( auto editor = EditorManager::getSingletonPtr() ) manager = editor->getProjectManager();
            if( !manager ) manager = workphone::make_ptr<ProjectManager>();
            manager->generateProject();
        }
        const auto configuration = app->getBuildConfig();
        app->setCachePath( ( directory / "Cache" ).u8string() );
        app->setProjectLibraryName( "Plugin" );
        const auto library = app->getProjectLibraryPath();
        const auto outputDirectory = checkedPath( library ).parent_path().u8string();
        auto upperConfig = configuration;
        std::transform( upperConfig.begin(), upperConfig.end(), upperConfig.begin(),
                        []( unsigned char c ) { return static_cast<char>( std::toupper( c ) ); } );
        runCMake( { "-S", directory.u8string(), "-B", buildDirectory.u8string(),
                    "-DCMAKE_BUILD_TYPE=" + configuration,
                    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_" + upperConfig + "=" + outputDirectory,
                    "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_" + upperConfig + "=" + outputDirectory },
                  buildDirectory / "configure.log" );
        auto oldPlugin = m_plugin;
        if( oldPlugin )
        {
            auto job = workphone::make_ptr<UnloadPluginJob>();
            job->setPlugin( oldPlugin );
            job->execute();
            m_plugin = nullptr;
        }
        try
        {
            runCMake( { "--build", buildDirectory.u8string(), "--config", configuration,
                        "--target", "Plugin" }, buildDirectory / "build.log" );
            if( library.empty() || !fs::is_regular_file( fs::u8path( library.c_str() ) ) )
                throw std::runtime_error( "Build completed but the project plugin was not found" );
            auto job = workphone::make_ptr<LoadPluginJob>();
            job->setPluginPath( library );
            job->execute();
            m_plugin = job->getPlugin();
            if( !m_plugin ) throw std::runtime_error( "Built plugin could not be loaded" );
            if( auto scripts = app->getScriptManager() ) scripts->reload( nullptr );
        }
        catch( ... )
        {
            if( !m_plugin && oldPlugin && !library.empty() && fs::is_regular_file( fs::u8path( library.c_str() ) ) )
            {
                auto job = workphone::make_ptr<LoadPluginJob>();
                job->setPluginPath( library );
                job->execute();
                m_plugin = job->getPlugin();
            }
            throw;
        }
    }

    String Project::getPluginHeader() const
    {
        return R"plugin(#pragma once
#include <Workphone/WorkphoneConfig.hpp>
namespace workphone::core { class IApplicationManager; }
#if defined(_WIN32)
#    define PROJECT_EXPORT __declspec(dllexport)
#    define PROJECT_CALL
#else
#    define PROJECT_EXPORT __attribute__((visibility("default")))
#    define PROJECT_CALL
#endif
extern "C" {
PROJECT_EXPORT void PROJECT_CALL loadPlugin(workphone::core::IApplicationManager *manager);
PROJECT_EXPORT void PROJECT_CALL unloadPlugin(workphone::core::IApplicationManager *manager);
PROJECT_EXPORT void PROJECT_CALL workphone_get_version(int *major, int *minor, int *patch);
}
)plugin";
    }

    String Project::getPluginSource() const
    {
        return R"plugin(#include "Plugin.hpp"

extern "C" {
void PROJECT_CALL workphone_get_version(int *major, int *minor, int *patch)
{
    if (major) *major = WP_VERSION_MAJOR;
    if (minor) *minor = WP_VERSION_MINOR;
    if (patch) *patch = WP_VERSION_PATCH;
}

void PROJECT_CALL loadPlugin(workphone::core::IApplicationManager *manager)
{
    // Register project component factories here.
    (void)manager;
}

void PROJECT_CALL unloadPlugin(workphone::core::IApplicationManager *manager)
{
    // Release project resources and unregister factories here.
    (void)manager;
}
}
)plugin";
    }

    SmartPtr<IPlugin> Project::getPlugin() const
    {
        return m_plugin;
    }

    void Project::setPlugin( SmartPtr<IPlugin> plugin )
    {
        m_plugin = plugin;
    }

    Array<String> Project::getScriptFilePaths() const
    {
        return m_scriptFilePaths;
    }

    void Project::setScriptFilePaths( const Array<String> &val )
    {
        auto paths = cleanPaths( val );
        if( m_scriptFilePaths != paths )
        {
            m_scriptFilePaths = paths;
            m_dirty = true;
        }
    }

    Array<String> Project::getResourceFolders() const
    {
        return m_resourceFolders;
    }

    void Project::setResourceFolders( const Array<String> &val )
    {
        auto paths = cleanPaths( val );
        if( m_resourceFolders != paths )
        {
            m_resourceFolders = paths;
            m_dirty = true;
        }
    }

    String Project::getApplicationType() const
    {
        return m_applicationType;
    }

    void Project::setApplicationType( const String &val )
    {
        if( m_applicationType != val )
        {
            m_applicationType = val;
            m_dirty = true;
        }
    }

    bool Project::isArchive() const
    {
        return m_archive;
    }

    void Project::setArchive( bool archive )
    {
        if( m_archive != archive )
        {
            m_archive = archive;
            m_dirty = true;
        }
    }

    bool Project::isDirty() const
    {
        return m_dirty;
    }

    void Project::setDirty( bool dirty )
    {
        m_dirty = dirty;
    }

    void Project::setMediaPaths( const Array<String> &mediaPaths )
    {
        auto paths = cleanPaths( mediaPaths );
        if( m_mediaPaths != paths )
        {
            m_mediaPaths = paths;
            m_dirty = true;
        }
    }

    Array<String> Project::getMediaPaths() const
    {
        return m_mediaPaths;
    }
}  // namespace workphone::editor

