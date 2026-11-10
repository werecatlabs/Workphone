#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/JobCreatePackage.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/IO/PackageArchive.hpp>
#include <Workphone/IO/PackagePaths.hpp>
#include <boost/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#    include <Windows.h>
#endif

namespace workphone
{
#ifdef _WIN32
    namespace
    {
        namespace fs = std::filesystem;

        // Keep a wide margin below the size and entry limits of the ZIP reader.
        constexpr std::uintmax_t archiveTargetBytes = 1024ull * 1024 * 1024;
        constexpr size_t archiveTargetEntries = 10000;

        struct BuildCancelled
        {
        };

        fs::path nativeWindowsPath( const fs::path &path )
        {
            const auto absolute = fs::absolute( path ).native();
            if( absolute.compare( 0, 4, L"\\\\?\\" ) == 0 )
            {
                return path;
            }
            if( absolute.compare( 0, 2, L"\\\\" ) == 0 )
            {
                return fs::path( L"\\\\?\\UNC\\" + absolute.substr( 2 ) );
            }
            return fs::path( L"\\\\?\\" + absolute );
        }

        struct BuildStagingDirectory
        {
            fs::path path;

            ~BuildStagingDirectory()
            {
                if( !path.empty() )
                {
                    std::error_code error;
                    fs::remove_all( nativeWindowsPath( path ), error );
                    if( error )
                    {
                        WP_LOG_WARNING( String( "Cannot clean temporary game build: " ) +
                                        path.generic_u8string().c_str() );
                    }
                }
            }
        };

        String toEnginePath( const fs::path &path )
        {
            return path.generic_u8string().c_str();
        }

        bool isWithin( const fs::path &root, const fs::path &path )
        {
            auto part = path.begin();
            for( const auto &parent : root )
            {
                if( parent.empty() )
                {
                    continue;
                }
                if( part == path.end() ||
                    CompareStringOrdinal( parent.c_str(), -1, part->c_str(), -1, TRUE ) != CSTR_EQUAL )
                {
                    return false;
                }
                ++part;
            }
            return true;
        }

        fs::path requireDirectory( const String &value, const char *label )
        {
            if( value.empty() || !fs::is_directory( fs::u8path( value.c_str() ) ) )
            {
                throw std::runtime_error( std::string( label ) + " directory is unavailable" );
            }
            return fs::canonical( fs::u8path( value.c_str() ) );
        }

        fs::path executableDirectory()
        {
            wchar_t executable[32768];
            const auto length = GetModuleFileNameW( nullptr, executable, 32768 );
            if( length == 0 || length == 32768 )
            {
                throw std::runtime_error( "Cannot locate the Editor installation" );
            }
            return fs::canonical( fs::path( std::wstring( executable, length ) ) ).parent_path();
        }

        void copyFile( const fs::path &source, const fs::path &destination )
        {
            const auto nativeSource = nativeWindowsPath( source );
            const auto nativeDestination = nativeWindowsPath( destination );
            if( !fs::is_regular_file( nativeSource ) )
            {
                throw std::runtime_error( "Required game file is missing: " +
                                          source.generic_u8string() );
            }
            fs::create_directories( nativeDestination.parent_path() );
            fs::copy_file( nativeSource, nativeDestination );
        }

        void copyRuntime( const fs::path &source, const fs::path &destination )
        {
            static constexpr const char *requiredFiles[] = {
                "WPRuntime.exe", "wp_plugins_runtime.cfg", "Workphone.dll",
                "WPNetwork.dll", "WPGraphics.dll",         "WPAssimp.dll",
                "WPSQLite.dll",  "WPPhysics.dll",          "WPLua.dll",
                "WPAudio.dll",   "WPVehiclePhysics.dll",   "WPProcedural.dll"
            };
            for( auto file : requiredFiles )
            {
                copyFile( source / file, destination / file );
            }

            // Current Windows builds otherwise depend on system DLLs and the MSVC CRT.
            // Copy the app-local redistributable when it is already deployed with the Editor.
            static constexpr const char *optionalFiles[] = { "d3dcompiler_47.dll",
                                                             "msvcp140.dll",
                                                             "msvcp140_1.dll",
                                                             "msvcp140_2.dll",
                                                             "msvcp140_atomic_wait.dll",
                                                             "msvcp140_codecvt_ids.dll",
                                                             "vcruntime140.dll",
                                                             "vcruntime140_1.dll",
                                                             "vcruntime140_threads.dll",
                                                             "concrt140.dll",
                                                             "tbb.dll",
                                                             "tbb12.dll",
                                                             "tbbmalloc.dll" };
            for( auto file : optionalFiles )
            {
                if( fs::is_regular_file( source / file ) )
                {
                    copyFile( source / file, destination / file );
                }
            }
        }

        bool isLinkedEntry( const fs::path &path )
        {
            const auto attributes = GetFileAttributesW( path.c_str() );
            return attributes != INVALID_FILE_ATTRIBUTES &&
                   ( attributes & FILE_ATTRIBUTE_REPARSE_POINT );
        }

        template <class Visitor>
        void visitFiles( const fs::path &root, Visitor visit, const std::atomic<bool> &cancelled )
        {
            for( fs::recursive_directory_iterator it( root ), end; it != end; ++it )
            {
                if( cancelled )
                {
                    throw BuildCancelled{};
                }
                const auto &entry = *it;
                if( isLinkedEntry( entry.path() ) )
                {
                    throw std::runtime_error(
                        "Move linked content inside the project before building: " +
                        entry.path().generic_u8string() );
                }
                if( entry.is_directory() )
                {
                    const auto name = packagePathAsciiLower( toEnginePath( entry.path().filename() ) );
                    if( name == ".git" || name == ".workphone" || name == "autosave" ||
                        name == "diagnostics" )
                    {
                        it.disable_recursion_pending();
                    }
                }
                else if( entry.is_regular_file() )
                {
                    visit( entry.path() );
                }
            }
        }

        bool needsLooseAsset( const fs::path &file )
        {
            const auto extension = packagePathAsciiLower( toEnginePath( file.extension() ) );
            // Streaming worlds and some importers open these directly on disk.
            return extension == ".wpworld" || extension == ".wpcell" || extension == ".bin" ||
                   extension == ".mtl" || extension == ".ttf" || extension == ".otf" ||
                   extension == ".font" || extension == ".db" ||
                   ( extension != ".fbmeshbin" &&
                     ApplicationUtil::isSupportedMesh( toEnginePath( file ) ) );
        }

        void appendConfiguredPaths( const Properties &settings, const String &key, Array<String> &paths )
        {
            if( auto entries = settings.getChild( key + "Entries" ) )
            {
                for( const auto &entry : entries->getChildren() )
                {
                    String value;
                    if( entry && entry->getPropertyValue( "pathValue", value ) && !value.empty() )
                    {
                        paths.push_back( value );
                    }
                }
            }
            else
            {
                Array<String> values;
                settings.getPropertyValue( key, values );
                paths.insert( paths.end(), values.begin(), values.end() );
            }
        }

        fs::path sourcePath( const fs::path &root, const String &value )
        {
            const auto path = fs::weakly_canonical( root / fs::u8path( value.c_str() ) );
            if( !isWithin( root, path ) )
            {
                throw std::runtime_error(
                    "Move external project content inside the project before building: " +
                    path.generic_u8string() );
            }
            return path;
        }

        fs::path checkedScene( const fs::path &root, const String &value )
        {
            const auto path = sourcePath( root, value );
            const auto extension = packagePathAsciiLower( toEnginePath( path.extension() ) );
            if( ( extension != ".fbscene" && extension != ".fbscenebin" &&
                  extension != ".fbscenexml" ) ||
                !fs::is_regular_file( path ) )
            {
                throw std::runtime_error( "A scene listed in Project Settings must be saved: " +
                                          path.generic_u8string() );
            }
            return path;
        }

        fs::path startupScene( const fs::path &root, const Properties &settings )
        {
            String scene;
            settings.getPropertyValue( "project.startupScene", scene );
            if( scene.empty() )
            {
                settings.getPropertyValue( "currentScenePath", scene );
            }
            if( scene.empty() )
            {
                settings.getPropertyValue( "sceneFilePath", scene );
            }
            if( scene.empty() )
            {
                settings.getPropertyValue( "SceneFilePath", scene );
            }
            if( scene.empty() )
            {
                scene = "Assets/Application.fbscene";
            }
            return checkedScene( root, scene );
        }

        void snapshotCatalog( SmartPtr<IDatabase> &database, const fs::path &source,
                              const fs::path &destination )
        {
            auto reader = database ? dynamic_cast<IReadOnlyDatabase *>( database.get() ) : nullptr;
            auto backup = database ? dynamic_cast<IBackupDatabase *>( database.get() ) : nullptr;
            String error;
            if( !reader || !backup )
            {
                throw std::runtime_error( "The SQLite project catalog backend is unavailable" );
            }
            const auto opened = reader->openReadOnly( toEnginePath( source ), error );
            const auto copied = opened && backup->backupTo( toEnginePath( destination ), error );
            database->close();
            if( !copied )
            {
                throw std::runtime_error( std::string( "Cannot snapshot the project catalog: " ) +
                                          error.c_str() );
            }
        }

        void writeDescriptor( const fs::path &file, const fs::path &scene,
                              const boost::json::array &archives )
        {
            const boost::json::object descriptor{ { "projectPath", "." },
                                                  { "currentScenePath", scene.generic_u8string() },
                                                  { "applicationType", "Desktop" },
                                                  { "archive", true },
                                                  { "packageArchives", archives } };
            std::ofstream output( file, std::ios::binary );
            output << boost::json::serialize( descriptor ) << '\n';
            output.close();
            if( !output )
            {
                throw std::runtime_error( "Cannot write Runtime.fbproject" );
            }
        }
    }  // namespace
#endif

    WP_CLASS_REGISTER_DERIVED( workphone, JobCreatePackage, Job );

    JobCreatePackage::JobCreatePackage( const String &projectFile, const String &destination,
                                        const String &mediaPath, const String &cachePath,
                                        const String &settingsPath, const String &mediaCatalogPath ) :
        m_projectFile( projectFile ),
        m_destination( destination ),
        m_mediaPath( mediaPath ),
        m_cachePath( cachePath ),
        m_settingsPath( settingsPath ),
        m_mediaCatalogPath( mediaCatalogPath )
    {
#ifdef _WIN32
        if( auto app = core::IApplicationManager::instancePtr() )
        {
            if( auto factory = app->getFactoryManager() )
            {
                m_catalog = factory->make_object<IDatabase>();
            }
        }
#endif
    }

    void JobCreatePackage::execute()
    {
#ifdef _WIN32
        try
        {
            report( "running", "Checking the project and build folder" );
            checkCancelled();
            const auto projectFile = fs::canonical( fs::u8path( m_projectFile.c_str() ) );
            const auto root = projectFile.parent_path();
            const auto media = requireDirectory( m_mediaPath, "Bundled Media" );
            const auto cache = requireDirectory( m_cachePath, "Project Cache" );
            const auto settings = requireDirectory( m_settingsPath, "Project SettingsCache" );
            if( m_destination.empty() )
            {
                throw std::runtime_error( "Choose a new game build folder" );
            }
            const auto destination =
                fs::weakly_canonical( fs::absolute( fs::u8path( m_destination.c_str() ) ) );
            if( fs::exists( destination ) || fs::is_symlink( fs::symlink_status( destination ) ) )
            {
                throw std::runtime_error(
                    "Choose a new game build folder. The selected folder already exists." );
            }
            const auto binaries = executableDirectory();
            if( isWithin( destination, root ) || isWithin( root / "Assets", destination ) ||
                isWithin( cache, destination ) || isWithin( settings, destination ) ||
                isWithin( media, destination ) || isWithin( binaries, destination ) ||
                isWithin( root / "Localization", destination ) ||
                isWithin( root / ".workphone", destination ) )
            {
                throw std::runtime_error(
                    "Choose a build folder outside the project content and engine installation" );
            }

            auto project = make_ptr<Properties>();
            const auto projectText = Path::readAllText( toEnginePath( projectFile ) );
            if( !DataUtil::isValidData( projectText ) )
            {
                throw std::runtime_error( "Cannot read the saved project document" );
            }
            DataUtil::parse( projectText, project.get(), DataFormat::JSON );
            if( !project->getProperty( "applicationFilePath" ).empty() ||
                !project->getProperty( "ApplicationFilePath" ).empty() )
            {
                throw std::runtime_error(
                    "Build Game supports scene-based Lua projects. Projects with applicationFilePath "
                    "require the separate project build workflow." );
            }
            const auto scene = startupScene( root, *project );
            std::set<fs::path> files{ scene };
            std::istringstream sceneList( project->getProperty( "project.sceneList" ).c_str() );
            for( std::string item; std::getline( sceneList, item, ';' ); )
            {
                const auto path = StringUtil::trim( String( item.c_str() ) );
                if( !path.empty() )
                {
                    files.insert( checkedScene( root, path ) );
                }
            }
            Array<String> resourceFolders{ "Assets" };
            appendConfiguredPaths( *project, "resourceFolders", resourceFolders );
            appendConfiguredPaths( *project, "paths", resourceFolders );
            Array<String> mediaFolders;
            appendConfiguredPaths( *project, "mediaPaths", mediaFolders );
            String scriptsPath;
            project->getPropertyValue( "scriptsPath", scriptsPath );
            mediaFolders.push_back( scriptsPath );
            for( const auto &folder : mediaFolders )
            {
                if( folder.empty() )
                {
                    continue;
                }
                const auto path = fs::u8path( folder.c_str() ).lexically_normal();
                const auto resolved = fs::weakly_canonical( root / path );
                if( path != "../Media" && path != "../Media/" && path != "../Media/Scripts" &&
                    path != "../Media/Scripts/" && !isWithin( media, resolved ) )
                {
                    resourceFolders.push_back( folder );
                }
            }
            std::vector<fs::path> scanned;
            const auto addAssetFolder = [&]( const fs::path &folder ) {
                if( isWithin( folder, root ) )
                {
                    throw std::runtime_error(
                        "Use a content subfolder in the project's resource paths" );
                }
                if( isWithin( folder, destination ) )
                {
                    throw std::runtime_error(
                        "Choose a build folder outside the configured project content" );
                }
                const auto wasScanned =
                    std::any_of( scanned.begin(), scanned.end(),
                                 [&]( const fs::path &parent ) { return isWithin( parent, folder ); } );
                if( !wasScanned && fs::is_directory( folder ) )
                {
                    visitFiles(
                        folder, [&]( const fs::path &file ) { files.insert( file ); }, m_cancelled );
                    scanned.push_back( folder );
                }
            };
            for( const auto &folder : resourceFolders )
            {
                if( !folder.empty() )
                {
                    addAssetFolder( sourcePath( root, folder ) );
                }
            }
            Array<String> scripts;
            appendConfiguredPaths( *project, "scriptFilePaths", scripts );
            for( const auto &script : scripts )
            {
                if( !script.empty() )
                {
                    const auto file = sourcePath( root, script );
                    if( !fs::is_regular_file( file ) )
                    {
                        throw std::runtime_error( "Configured script is missing: " +
                                                  file.generic_u8string() );
                    }
                    files.insert( file );
                }
            }

            fs::create_directories( destination.parent_path() );
            const auto temporary =
                destination.parent_path() /
                fs::u8path( ".wp-game-build-" + std::string( StringUtil::getUUID().c_str() ) );
            if( !fs::create_directory( temporary ) )
            {
                throw std::runtime_error( "Cannot create the temporary game build folder" );
            }
            BuildStagingDirectory stage{ temporary };
            report( "running", "Copying WPRuntime, bundled Media and project settings" );
            setProgress( 10 );
            copyRuntime( binaries, stage.path );
            const auto mediaOutput = stage.path / "Media";
            visitFiles(
                media,
                [&]( const fs::path &file ) {
                    copyFile( file, mediaOutput / file.lexically_relative( media ) );
                },
                m_cancelled );
            const auto mediaCatalog = m_mediaCatalogPath.empty()
                                          ? media / ".workphone/media-assets.db"
                                          : fs::u8path( m_mediaCatalogPath.c_str() );
            copyFile( mediaCatalog, mediaOutput / ".workphone/media-assets.db" );
            for( const auto file : { "GameGraphics.settings", "ProjectRuntime.settings" } )
            {
                if( fs::is_regular_file( root / file ) )
                {
                    copyFile( root / file, stage.path / file );
                }
            }
            for( const auto folder : { "Localization", ".workphone/lighting", ".workphone/generated" } )
            {
                const auto input = root / folder;
                if( fs::is_directory( input ) )
                {
                    if( isLinkedEntry( input ) || !isWithin( root, fs::canonical( input ) ) )
                    {
                        throw std::runtime_error( "Move linked runtime data inside the project: " +
                                                  input.generic_u8string() );
                    }
                    visitFiles(
                        input,
                        [&]( const fs::path &file ) {
                            copyFile( file, stage.path / file.lexically_relative( root ) );
                            if( file != scene )
                            {
                                files.erase( file );
                            }
                        },
                        m_cancelled );
                }
            }

            report( "running", "Copying cooked content and taking a project catalog snapshot" );
            setProgress( 25 );
            const auto cacheOutput = stage.path / "Cache";
            fs::create_directory( cacheOutput );
            visitFiles(
                cache,
                [&]( const fs::path &file ) {
                    const auto relative = file.lexically_relative( cache );
                    const auto name = packagePathAsciiLower( toEnginePath( file.filename() ) );
                    const auto extension = packagePathAsciiLower( toEnginePath( file.extension() ) );
                    if( name != "asset.db" && name != "asset.db-wal" && name != "asset.db-shm" &&
                        name != "asset.db-journal" && extension != ".log" && extension != ".dll" &&
                        extension != ".pdb" && extension != ".lib" )
                    {
                        copyFile( file, cacheOutput / relative );
                    }
                },
                m_cancelled );
            if( fs::is_regular_file( cache / "asset.db" ) )
            {
                snapshotCatalog( m_catalog, cache / "asset.db", cacheOutput / "asset.db" );
            }
            const auto settingsOutput = stage.path / "SettingsCache";
            fs::create_directory( settingsOutput );
            visitFiles(
                settings,
                [&]( const fs::path &file ) {
                    // This tree also holds local graphics overrides and saved player progress.
                    const auto extension = packagePathAsciiLower( toEnginePath( file.extension() ) );
                    if( extension == ".resourcedata" || extension == ".meshdata" )
                    {
                        copyFile( file, settingsOutput / file.lexically_relative( settings ) );
                    }
                },
                m_cancelled );

            boost::json::array archives;
            Array<StringW> archiveFiles;
            std::uintmax_t bytes = 0;
            size_t completed = 0;
            const auto writeArchive = [&] {
                if( archiveFiles.empty() )
                {
                    return;
                }
                checkCancelled();
                const auto archiveName = "Content_" + std::to_string( archives.size() + 1 ) + ".fbpak";
                report( "running", String( "Packing game assets into " ) + archiveName.c_str() );
                const auto archivePath = stage.path / archiveName;
                if( writeGamePackageArchive( archivePath.wstring().c_str(), archiveFiles,
                                             root.wstring().c_str(),
                                             [&] { return m_cancelled.load(); } ) != 0 )
                {
                    checkCancelled();
                    throw std::runtime_error( "Could not write " + archiveName +
                                              ". See the Editor log." );
                }
                archives.emplace_back( archiveName );
                archiveFiles.clear();
                bytes = 0;
                setProgress( 50 + static_cast<u32>( 49 * completed / files.size() ) );
            };

            setProgress( 50 );
            for( const auto &file : files )
            {
                checkCancelled();
                if( needsLooseAsset( file ) )
                {
                    copyFile( file, stage.path / file.lexically_relative( root ) );
                }
                else
                {
                    const auto size = fs::file_size( file );
                    if( archiveFiles.size() >= archiveTargetEntries ||
                        bytes + size > archiveTargetBytes )
                    {
                        writeArchive();
                    }
                    archiveFiles.emplace_back( file.wstring().c_str() );
                    bytes += size;
                }
                ++completed;
            }
            writeArchive();
            writeDescriptor( stage.path / "Runtime.fbproject", scene.lexically_relative( root ),
                             archives );
            checkCancelled();
            report( "running", "Publishing the completed game build" );
            // Without MOVEFILE_REPLACE_EXISTING even a competing empty directory wins.
            if( !MoveFileExW( stage.path.c_str(), destination.c_str(), 0 ) )
            {
                throw fs::filesystem_error( "Cannot publish the game build", stage.path, destination,
                                            std::error_code( GetLastError(), std::system_category() ) );
            }
            stage.path.clear();
            {
                std::lock_guard<std::mutex> lock( m_resultMutex );
                m_outputPath = toEnginePath( destination );
            }
            setProgress( 100 );
            report( "succeeded", "Game built. Run WPRuntime.exe from the build folder." );
        }
        catch( const BuildCancelled & )
        {
            report( "cancelled", "Game build cancelled" );
        }
        catch( const std::exception &error )
        {
            report( "failed", error.what() );
            WP_LOG_ERROR( String( "Game build failed: " ) + error.what() );
        }
#else
        report( "failed", "Standalone game packaging is available on Windows only" );
#endif
    }

    void JobCreatePackage::stop()
    {
        cancel();
        Job::stop();
    }

    void JobCreatePackage::cancel()
    {
        m_cancelled = true;
    }

    String JobCreatePackage::getStatus() const
    {
        std::lock_guard<std::mutex> lock( m_resultMutex );
        return m_status;
    }

    String JobCreatePackage::getDiagnostic() const
    {
        std::lock_guard<std::mutex> lock( m_resultMutex );
        return m_diagnostic;
    }

    String JobCreatePackage::getOutputPath() const
    {
        std::lock_guard<std::mutex> lock( m_resultMutex );
        return m_outputPath;
    }

    void JobCreatePackage::report( const String &status, const String &diagnostic )
    {
        std::lock_guard<std::mutex> lock( m_resultMutex );
        m_status = status;
        m_diagnostic = diagnostic;
    }

    void JobCreatePackage::checkCancelled() const
    {
#ifdef _WIN32
        if( m_cancelled )
        {
            throw BuildCancelled{};
        }
#endif
    }
}  // namespace workphone
