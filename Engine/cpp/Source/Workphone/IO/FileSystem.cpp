#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/IO/FileSystemArchive.hpp>
#include <Workphone/IO/ObfuscatedZipArchive.hpp>
#include <Workphone/IO/ZipArchive.hpp>
#include <Workphone/IO/FolderListing.hpp>
#include <Workphone/IO/FileDataStream.hpp>
#include <Workphone/IO/FileListener.hpp>
#include <Workphone/IO/NativeFileDialog.hpp>
#include <Workphone/IO/FileList.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Interface/IO/IFileListener.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>

#if WP_USE_BOOST
#    include <boost/uuid/uuid.hpp>
#    include <boost/uuid/uuid_generators.hpp>
#    include <boost/uuid/uuid_io.hpp>
#    include <boost/functional/hash.hpp>

#    include <boost/filesystem/operations.hpp>
#    include <boost/crc.hpp>      // for boost::crc_basic, boost::crc_optimal
#    include <boost/cstdint.hpp>  // for boost::uint16_t
#    include <boost/range/iterator_range.hpp>
#    include <boost/range/algorithm.hpp>
#    include <boost/range/iterator.hpp>
#endif

#ifdef WP_PLATFORM_WIN32
#    include <windows.h>
#endif

namespace workphone
{

    namespace
    {
        inline std::error_code &_wpFsEc()
        {
            thread_local std::error_code ec;
            ec.clear();
            return ec;
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, FileSystem, IFileSystem );

    const Array<String> FileSystem::textExtensions = { ".compositor", ".material", ".program",
                                                       ".fontdef",    ".glsl",     ".hlsl",
                                                       ".cfg",        ".h",        ".cpp",
                                                       ".txt",        ".text" };
    const Array<String> FileSystem::binaryExtensions = { ".ttf",  ".dds", ".png", ".jpg",
                                                         ".jpeg", ".bmp", ".tga", ".gif" };

    FileSystem::FileSystem()
    {
        static const String fileSystemName = "FileSystem";
        setName( fileSystemName );

        // Refresh and watcher notifications are consumed by application-level systems such as
        // the editor asset tree. Opt in to the global event bus and keep UI-facing delivery on the
        // application task.
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setEventTaskFlags( Thread::Application_Flag );
    }

    FileSystem::~FileSystem()
    {
        unload( nullptr );
    }

    void FileSystem::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            const auto size = 32;

            constexpr auto fileInfoSize = sizeof( FileInfo );

            m_files.reserve( size );
            m_folderArchives.reserve( size );
            m_fileArchives.reserve( size );

            //m_fileWatcher = new FW::FileWatcher();
            //m_fileListener = new FileListener();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FileSystem::reload( SmartPtr<ISharedObject> data )
    {
        ScopedLock lock( this );
        unload( data );
        load( data );
    }

    void FileSystem::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                auto folderArchives = m_folderArchives.snapshot();
                auto fileArchives = m_fileArchives.snapshot();

                for( auto &archive : folderArchives )
                {
                    if( archive )
                    {
                        archive->unload( nullptr );
                    }
                }

                for( auto &archive : fileArchives )
                {
                    if( archive )
                    {
                        archive->unload( nullptr );
                    }
                }

                m_files.clear();
                m_folderArchives.clear();
                m_fileArchives.clear();

                if( m_fileWatcher )
                {
                    delete m_fileWatcher;
                    m_fileWatcher = nullptr;
                }

                if( m_fileListener )
                {
                    delete m_fileListener;
                    m_fileListener = nullptr;
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto FileSystem::isExistingFile( const String &filePath, bool ignorePath, bool ignoreCase ) const
        -> bool
    {
        try
        {
            if( !Path::hasFileName( filePath ) )
            {
                return false;
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            // Files created at runtime are not part of the indexed archives.
            // Check the physical path in game mode as well as editor mode.
            if( applicationManager )
            {
                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                auto existing = false;
                if( Path::isPathAbsolute( filePath ) )
                {
                    existing = Path::isExistingFile( filePath );
                }
                else
                {
                    auto absolutePath = Path::lexically_normal( projectPath, filePath );
                    existing = Path::isExistingFile( absolutePath );
                }

                if( existing )
                {
                    return true;
                }
            }

            for( auto &file : m_files )
            {
                if( StringUtil::isEqual( file.filePath.c_str(), filePath.c_str() ) ||
                    StringUtil::isEqual( file.filePathLowerCase.c_str(), filePath.c_str() ) ||
                    StringUtil::isEqual( file.fileName.c_str(), filePath.c_str() ) ||
                    StringUtil::isEqual( file.fileNameLowerCase.c_str(), filePath.c_str() ) )
                {
                    return true;
                }
            }

            auto filePathLower = StringUtil::make_lower( filePath );
            auto pFilePathLower = filePathLower.c_str();

            for( auto &file : m_files )
            {
                if( StringUtil::isEqual( file.filePath.c_str(), pFilePathLower ) ||
                    StringUtil::isEqual( file.filePathLowerCase.c_str(), pFilePathLower ) ||
                    StringUtil::isEqual( file.fileName.c_str(), pFilePathLower ) ||
                    StringUtil::isEqual( file.fileNameLowerCase.c_str(), pFilePathLower ) )
                {
                    return true;
                }
            }

            if( Path::isPathAbsolute( filePath ) )
            {
                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                auto relativePath = Path::getRelativePath( projectPath, filePath );

                for( auto &file : m_files )
                {
                    if( StringUtil::isEqual( file.filePath.c_str(), relativePath.c_str() ) ||
                        StringUtil::isEqual( file.filePathLowerCase.c_str(), relativePath.c_str() ) ||
                        StringUtil::isEqual( file.fileName.c_str(), relativePath.c_str() ) ||
                        StringUtil::isEqual( file.fileNameLowerCase.c_str(), relativePath.c_str() ) )
                    {
                        return true;
                    }
                }
            }

            for( auto archive : m_folderArchives )
            {
                if( archive )
                {
                    auto hasFile = archive->exists( filePath, ignorePath, ignoreCase );
                    if( hasFile )
                    {
                        return true;
                    }
                }
            }

            // check archives
            for( auto archive : m_fileArchives )
            {
                if( archive )
                {
                    auto hasFile = archive->exists( filePath, ignorePath, ignoreCase );
                    if( hasFile )
                    {
                        return true;
                    }
                }
            }

            //WP_ASSERT( !Path::isExistingFile( filePath ) && Path::isFile(filePath) );
            return false;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    auto FileSystem::isExistingFile( const String &path, const String &filePath, bool ignorePath,
                                     bool ignoreCase ) const -> bool
    {
        try
        {
            auto relativePath = Path::lexically_normal( path, filePath );
            if( Path::isPathAbsolute( relativePath ) )
            {
                return Path::isExistingFile( relativePath.c_str() );
            }

            auto applicationManager = core::IApplicationManager::instancePtr();

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto absolutePath = Path::lexically_normal( projectPath, relativePath );
            return Path::isExistingFile( absolutePath );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    auto FileSystem::isExistingFolder( const String &path ) const -> bool
    {
        try
        {
            if( Path::isPathAbsolute( path ) )
            {
                return Path::isExistingFolder( path.c_str() );
            }

            auto applicationManager = core::IApplicationManager::instancePtr();

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto absolutePath = Path::lexically_normal( projectPath, path );
            return Path::isExistingFolder( absolutePath );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    void FileSystem::createDirectories( const String &path )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto cleanDst = String();

            if( Path::isPathAbsolute( path ) )
            {
                cleanDst = StringUtil::cleanupPath( path );
            }
            else
            {
                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                auto absolutePath = Path::lexically_normal( projectPath, path );
                cleanDst = StringUtil::cleanupPath( absolutePath );
            }

            bool bSuccess = true;
            std::error_code ec;

            auto retries = 0;
            std::filesystem::path dir( cleanDst.c_str() );
            while( !std::filesystem::is_directory( dir ) && retries++ < 10 )
            {
                bSuccess = false;

                if( std::filesystem::create_directories( dir, ec ) )
                {
                    bSuccess = true;
                    break;
                }

                Thread::sleep( 0.2 );
            }

            if( !bSuccess && ec.value() != 0 )
            {
                auto message =
                    String( "Cannot create directory." ) + cleanDst + " " + ec.message().c_str();
                throw Exception( message );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FileSystem::deleteFilesFromPath( const String &path )
    {
        auto files = Path::getFiles( path );
        for( auto &fileName : files )
        {
            auto filePath = path + String( "/" ) + fileName;

            std::error_code ec;
            std::filesystem::remove( std::filesystem::path( filePath.c_str() ), ec );
            if( ec )
            {
                WP_LOG_ERROR(
                    ( String( "Failed to remove file: " ) + filePath + " " + ec.message().c_str() )
                        .c_str() );
            }
        }

        auto folders = Path::getFolders( path, false );
        for( auto &folder : folders )
        {
            try
            {
                auto folderPath = path + "/" + folder + "/";

                std::filesystem::path dir( folderPath.c_str() );
                if( std::filesystem::is_directory( dir ) )
                {
                    std::error_code ec;
                    std::filesystem::remove_all( dir, ec );
                    if( ec )
                    {
                        WP_LOG_ERROR( ( String( "Failed to remove directory: " ) + folderPath + " " +
                                        ec.message().c_str() )
                                          .c_str() );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "Unknown exception." );
            }
        }
    }

    void FileSystem::getFileNamesInFolder( const String &path, Array<String> &fileNames )
    {
#ifdef WP_PLATFORM_WIN32
        if( isFolder( path ) )
        {
            WIN32_FIND_DATAA fileData;
            auto handle = INVALID_HANDLE_VALUE;

            String searchPath = path + "/*";
            handle = FindFirstFileA( searchPath.c_str(), &fileData );

            if( handle != INVALID_HANDLE_VALUE )
            {
                while( FindNextFileA( handle, &fileData ) )
                {
                    String fileName = fileData.cFileName;
                    if( fileName == ".." )  // IF ITS NOT THE PARENT FILE
                    {
                        fileNames.push_back( fileName );
                    }
                }

                FindClose( handle );
            }
        }
        else
        {
            throw Exception( "FileSystem::getFileNamesInFolder - Error path is not a folder" );
        }
#else
#endif
    }

    void FileSystem::getSubFolders( const String &path, Array<String> &folderNames )
    {
#ifdef WP_PLATFORM_WIN32
        if( isFolder( path ) )
        {
            WIN32_FIND_DATAA fileData;
            auto handle = INVALID_HANDLE_VALUE;

            String searchPath = path + "/*";
            handle = FindFirstFileA( searchPath.c_str(), &fileData );

            if( handle != INVALID_HANDLE_VALUE )
            {
                while( FindNextFileA( handle, &fileData ) )
                {
                    if( fileData.dwFileAttributes &
                        FILE_ATTRIBUTE_DIRECTORY )  // IF ITS NOT THE PARENT FILE
                    {
                        String fileName = StringUtil::toString( fileData.cFileName );
                        if( fileName != String( ".." ) )
                        {
                            folderNames.push_back( fileName );
                        }
                    }
                }

                FindClose( handle );
            }
        }
        else
        {
            WP_EXCEPTION( "FileSystem::getFileNamesInFolder - Error path is not a folder" );
        }
#else
#endif
    }

    auto FileSystem::isFolder( const String &path ) -> bool
    {
#if WP_USE_BOOST
        return std::filesystem::is_directory( path.c_str() );
#elif defined WP_PLATFORM_WIN32
        auto str = path;
        return ( GetFileAttributesA( str.c_str() ) & FILE_ATTRIBUTE_DIRECTORY ) != 0;
#else
        return false;
#endif
    }

    auto FileSystem::setWorkingDirectory( const String &directory ) -> bool
    {
#if WP_USE_BOOST
        std::filesystem::path path( directory.c_str() );
        current_path( path );
        return true;
#else
        return false;
#endif
    }

    auto FileSystem::getWorkingDirectory() -> String
    {
#if WP_USE_BOOST
        return std::filesystem::current_path().string().c_str();
#else
        return "";
#endif
    }

    auto FileSystem::addFileArchive( const String &filename, bool ignoreCase /*=true*/,
                                     bool ignorePaths /*=true*/, ArchiveType archiveType /*=0*/,
                                     const String &password /*= StringUtil::EmptyString*/ ) -> bool
    {
        if( StringUtil::isNullOrEmpty( filename ) )
        {
            WP_EXCEPTION( "Invalid file name." );
        }

        addArchive( filename, archiveType );
        return true;
    }

    auto FileSystem::openFileDialog() -> SmartPtr<INativeFileDialog>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        return factoryManager->make_ptr<NativeFileDialog>();
    }

    auto FileSystem::open( const String &filePath ) -> SmartPtr<IStream>
    {
        auto isBinary = true;

        for( auto &textExtension : textExtensions )
        {
            if( Path::endsWith( filePath, textExtension ) )
            {
                isBinary = false;
                break;
            }
        }

        for( auto &binaryExtension : binaryExtensions )
        {
            if( Path::endsWith( filePath, binaryExtension ) )
            {
                isBinary = true;
                break;
            }
        }

        return open( filePath, true, isBinary, false, false, false );
    }

    auto FileSystem::open( const String &filePath, bool input, bool binary, bool truncate,
                           bool ignorePath, bool ignoreCase ) -> SmartPtr<IStream>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            s32 flags = input ? std::ifstream::in : std::ifstream::out;

            if( binary )
            {
                flags = flags | std::ifstream::binary;
            }

            if( truncate )
            {
                flags = flags | std::ifstream::trunc;
            }

            auto f = flags;

            auto fileId = StringUtil::getUUID( filePath );

            auto files = m_files.snapshot();
            for( auto &file : files )
            {
                if( file.fileId == fileId )
                {
                    auto absolutePath = Path::getAbsolutePath( projectPath, file.filePath.c_str() );
                    auto stream = new std::fstream( absolutePath.c_str(), f );
                    if( stream->is_open() )
                    {
                        auto pStream = factoryManager->make_ptr<FileDataStream>();
                        pStream->setFStream( stream );
                        return pStream;
                    }

                    delete stream;
                }
            }

            for( auto &file : files )
            {
                if( StringUtil::isEqual( file.filePath.c_str(), filePath.c_str() ) ||
                    StringUtil::isEqual( file.filePathLowerCase.c_str(), filePath.c_str() ) ||
                    StringUtil::isEqual( file.fileName.c_str(), filePath.c_str() ) ||
                    StringUtil::isEqual( file.fileNameLowerCase.c_str(), filePath.c_str() ) )
                {
                    auto absolutePath = Path::getAbsolutePath( projectPath, file.filePath.c_str() );
                    auto stream = new std::fstream( absolutePath.c_str(), f );
                    if( stream->is_open() )
                    {
                        auto pStream = factoryManager->make_ptr<FileDataStream>();
                        pStream->setFStream( stream );
                        return pStream;
                    }

                    delete stream;
                }
            }

            auto folderArchives = m_folderArchives.snapshot();
            for( auto &archive : folderArchives )
            {
                if( archive )
                {
                    if( archive->exists( filePath, ignorePath, ignoreCase ) )
                    {
                        if( auto file = archive->open( filePath, input, binary, truncate, ignorePath,
                                                       ignoreCase ) )
                        {
                            return file;
                        }
                    }
                }
            }

            auto fileArchives = m_fileArchives.snapshot();
            for( auto &archive : fileArchives )
            {
                if( archive )
                {
                    if( archive->exists( filePath, ignorePath, ignoreCase ) )
                    {
                        if( auto file = archive->open( filePath, input, binary, truncate, ignorePath,
                                                       ignoreCase ) )
                        {
                            return file;
                        }
                    }
                }
            }

            if( Path::isPathAbsolute( filePath ) )
            {
                if( !input )
                {
                    auto stream = new std::fstream( filePath.c_str(), f );
                    if( stream->is_open() )
                    {
                        auto pStream = factoryManager->make_ptr<FileDataStream>();
                        pStream->setFStream( stream );
                        return pStream;
                    }

                    delete stream;
                }
                else
                {
                    auto stream = new std::ifstream( filePath.c_str(), f );
                    if( stream->is_open() )
                    {
                        auto pStream = factoryManager->make_ptr<FileDataStream>();
                        pStream->setInStream( stream );
                        return pStream;
                    }

                    delete stream;
                }
            }
            else
            {
                auto absolutePath = Path::lexically_normal( projectPath, filePath );

                if( !input )
                {
                    auto stream = new std::fstream( absolutePath.c_str(), f );
                    if( stream->is_open() )
                    {
                        auto pStream = factoryManager->make_ptr<FileDataStream>();
                        pStream->setFStream( stream );
                        return pStream;
                    }

                    delete stream;
                }
                else
                {
                    auto stream = new std::ifstream( absolutePath.c_str(), f );
                    if( stream->is_open() )
                    {
                        auto pStream = factoryManager->make_ptr<FileDataStream>();
                        pStream->setInStream( stream );
                        return pStream;
                    }

                    delete stream;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto FileSystem::getAbsolutePath( const String &filename ) const -> String
    {
        try
        {
            std::filesystem::path p( filename.c_str() );

            // Try to get canonical path if the file/directory exists
            std::filesystem::path result;
            if( std::filesystem::exists( p ) )
            {
                result = std::filesystem::canonical( p );
            }
            else
            {
                // If path doesn't exist, make it absolute without resolving symlinks
                result = std::filesystem::absolute( p );
            }

            // Convert to string and normalize separators to forward slashes
            auto pathStr = result.string();
            std::replace( pathStr.begin(), pathStr.end(), '\\', '/' );

            return pathStr.c_str();
        }
        catch( const std::filesystem::filesystem_error &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return filename;
    }

    auto FileSystem::getFilesWithExtension( const String &extension ) const -> Array<FileInfo>
    {
        Array<FileInfo> files;
        files.reserve( 128 );

        for( auto &file : m_files )
        {
            auto fileName = file.filePath;
            auto fileExtension = Path::getFileExtension( fileName.c_str() );

            if( StringUtil::isEqual( extension.c_str(), fileExtension.c_str() ) )
            {
                files.push_back( file );
            }
        }

        for( auto &archive : m_folderArchives )
        {
            auto archiveFiles = archive->getFiles();

            for( auto &file : archiveFiles )
            {
                const auto &fileName = file.filePath;
                auto fileExtension = Path::getFileExtension( fileName.c_str() );

                if( StringUtil::isEqual( extension.c_str(), fileExtension.c_str() ) )
                {
                    files.push_back( file );
                }
            }
        }

        for( auto &archive : m_fileArchives )
        {
            auto archiveFiles = archive->getFiles();

            for( auto &file : archiveFiles )
            {
                const auto &fileName = file.filePath;
                auto fileExtension = Path::getFileExtension( fileName.c_str() );

                if( StringUtil::isEqual( extension.c_str(), fileExtension.c_str() ) )
                {
                    files.push_back( file );
                }
            }
        }

        return files;
    }

    auto FileSystem::getFilesWithExtension( const String &path, const String &extension ) const
        -> Array<FileInfo>
    {
        Array<FileInfo> files;
        files.reserve( 128 );

        for( auto &file : m_files )
        {
            if( StringUtil::isEqual( file.path.c_str(), path.c_str() ) ||
                file.path.find( path.c_str() ) != String::npos )
            {
                auto fileName = file.filePath;
                auto fileExtension = Path::getFileExtension( fileName.c_str() );
                if( StringUtil::isEqual( extension.c_str(), fileExtension.c_str() ) )
                {
                    files.push_back( file );
                }
            }
        }

        for( auto &archive : m_folderArchives )
        {
            auto archiveFiles = archive->getFiles();
            for( auto &file : archiveFiles )
            {
                if( StringUtil::isEqual( file.path.c_str(), path.c_str() ) ||
                    file.path.find( path.c_str() ) != String::npos )
                {
                    const auto fileName = file.filePath;
                    auto fileExtension = Path::getFileExtension( fileName.c_str() );
                    if( StringUtil::isEqual( extension.c_str(), fileExtension.c_str() ) )
                    {
                        files.push_back( file );
                    }
                }
            }
        }

        for( auto &archive : m_fileArchives )
        {
            auto archiveFiles = archive->getFiles();
            for( auto &file : archiveFiles )
            {
                if( StringUtil::isEqual( file.path.c_str(), path.c_str() ) ||
                    file.path.find( path.c_str() ) != String::npos )
                {
                    const auto &fileName = file.filePath;
                    auto fileExtension = Path::getFileExtension( fileName.c_str() );
                    if( StringUtil::isEqual( extension.c_str(), fileExtension.c_str() ) )
                    {
                        files.push_back( file );
                    }
                }
            }
        }

        return files;
    }

    auto FileSystem::getFileNamesWithExtension( const String &path, const String &extension,
                                                bool recursive ) -> Array<String>
    {
#if 0
        Array<String> filesWithExtension;
        filesWithExtension.reserve(1024);

        try
        {
            std::filesystem::path dir(path);
            
            if (recursive)
            {
                // Use recursive directory iterator for recursive search
                std::filesystem::recursive_directory_iterator it( dir, _wpFsEc() ), end;
                
                for (auto &entry : boost::make_iterator_range(it, end))
                {
                    if (!is_directory(entry))
                    {
                        auto filePath = entry.path().u8string();
                        if (Path::endsWith(filePath, extension))
                        {
                            filesWithExtension.push_back(filePath);
                        }
                    }
                }
            }
            else
            {
                // Use regular directory iterator for non-recursive search
                std::filesystem::directory_iterator it( dir, _wpFsEc() ), end;
                
                for (auto &entry : boost::make_iterator_range(it, end))
                {
                    if (!is_directory(entry))
                    {
                        auto filePath = entry.path().u8string();
                        if (Path::endsWith(filePath, extension))
                        {
                            filesWithExtension.push_back(filePath);
                        }
                    }
                }
            }
        }
        catch (std::exception &e)
        {
            WP_LOG_EXCEPTION(e);
        }

        return filesWithExtension;
#else
        Array<String> filesWithExtension;
        filesWithExtension.reserve( 1024 );

        try
        {
            std::filesystem::path dir( StringUtil::str( path ) );

            if( recursive )
            {
                // Use recursive directory iterator for recursive search
                for( const auto &entry :
                     std::filesystem::recursive_directory_iterator( dir, _wpFsEc() ) )
                {
                    if( !std::filesystem::is_directory( entry ) )
                    {
                        auto filePath = entry.path().u8string();
                        if( Path::endsWith( filePath.c_str(), extension ) )
                        {
                            filesWithExtension.push_back( filePath.c_str() );
                        }
                    }
                }
            }
            else
            {
                // Use regular directory iterator for non-recursive search
                for( const auto &entry : std::filesystem::directory_iterator( dir, _wpFsEc() ) )
                {
                    if( !std::filesystem::is_directory( entry ) )
                    {
                        auto filePath = entry.path().u8string();
                        if( Path::endsWith( filePath.c_str(), extension ) )
                        {
                            filesWithExtension.push_back( filePath.c_str() );
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return filesWithExtension;
#endif
    }

    auto FileSystem::getFileNamesWithExtension( const String &extension ) const -> Array<String>
    {
        Array<String> fileNames;
        fileNames.reserve( 128 );

        getFileNamesWithExtension( extension, fileNames );
        return fileNames;
    }

    void FileSystem::getFileNamesWithExtension( const String &extension, Array<String> &fileNames ) const
    {
        for( auto &file : m_files )
        {
            auto fileName = file.filePath;
            auto fileExtension = Path::getFileExtension( fileName.c_str() );

            if( extension == fileExtension )
            {
                fileNames.push_back( fileName.c_str() );
            }
        }

        for( auto &archive : m_folderArchives )
        {
            auto archiveFiles = archive->getFiles();

            for( auto &file : archiveFiles )
            {
                const auto &fileName = file.filePath;
                auto fileExtension = Path::getFileExtension( fileName.c_str() );

                if( extension == fileExtension )
                {
                    fileNames.push_back( fileName.c_str() );
                }
            }
        }

        for( auto &archive : m_fileArchives )
        {
            auto archiveFiles = archive->getFiles();

            for( auto &file : archiveFiles )
            {
                const auto &fileName = file.filePath;
                auto fileExtension = Path::getFileExtension( fileName.c_str() );

                if( extension == fileExtension )
                {
                    fileNames.push_back( fileName.c_str() );
                }
            }
        }
    }

    auto FileSystem::isInSameDirectory( const String &path, const String &file ) -> s32
    {
        // Early return: if path is empty, they can't be in the same directory
        if( path.empty() )
        {
            return -1;
        }

        // Early return: if path and file are identical, they're the same (0 levels apart)
        if( path == file )
        {
            return 0;
        }

        // Count directory separators in both strings
        size_t pathDepth = std::count( path.begin(), path.end(), '/' );
        size_t fileDepth = std::count( file.begin(), file.end(), '/' );

        return static_cast<s32>( fileDepth - pathDepth );
    }

    auto FileSystem::getFileDir( const String &filename ) const -> String
    {
        FileInfo info;

        if( findFileInfo( filename, info ) )
        {
            return String( info.path.c_str() );
        }

        return {};
    }

    void FileSystem::addFolder( SmartPtr<IFolderExplorer> parent )
    {
        if( parent )
        {
            auto folderName = parent->getFolderName();
            addArchive( folderName, ArchiveType::Folder );

            auto subFolders = parent->getSubFolders();
            for( auto &subFolder : subFolders )
            {
                addFolder( subFolder );
            }

#if WP_BUILD_FILEWATCHER
            if( m_fileWatcher )
            {
                m_fileWatcher->addWatch( folderName.c_str(), m_fileListener );
            }
#endif
        }
    }

    void FileSystem::addFolder( const String &folderPath, bool recursive )
    {
        if( recursive )
        {
            if( auto folderListing = getFolderListing( folderPath ) )
            {
                addFolder( folderListing );
            }
        }
        else
        {
            addArchive( folderPath, ArchiveType::Folder );
        }

#if WP_BUILD_FILEWATCHER
        if( m_fileWatcher )
        {
            m_fileWatcher->addWatch( folderPath.c_str(), m_fileListener );
        }
#endif
    }

    void FileSystem::addArchive( const String &filePath )
    {
        auto ext = Path::getFileExtension( filePath );
        auto type = getTypeFromTypeName( ext );
        addArchive( filePath, type );
    }

    void FileSystem::addArchive( const String &filePath, const String &typeName )
    {
        auto type = getTypeFromTypeName( typeName );
        addArchive( filePath, type );
    }

    void FileSystem::addArchive( const String &filePath, ArchiveType type )
    {
        try
        {
            WP_ASSERT( IArchive::typeInfo() != 0 );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            switch( type )
            {
            case ArchiveType::Folder:
            case ArchiveType::FileSystem:
            {
                auto a = factoryManager->make_ptr<FileSystemArchive>();
                a->setPath( filePath );
                addFolderArchive( a );
                a->load( nullptr );
            }
            break;
            case ArchiveType::Zip:
            {
                auto a = factoryManager->make_ptr<ZipArchive>( filePath, true, true );
                addFileArchive( a );
                a->load( nullptr );
            }
            break;
            case ArchiveType::ObfuscatedZip:
            {
                auto a = factoryManager->make_ptr<ObfuscatedZipArchive>( filePath, true, true );
                addFileArchive( a );
                a->load( nullptr );
            }
            break;
            case ArchiveType::Unknown:
            {
            }
            break;
            default:
            {
            }
            break;
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto FileSystem::getFileArchive( u32 index ) -> SmartPtr<IArchive>
    {
        try
        {
            auto folderArchives = m_folderArchives.snapshot();
            auto fileArchives = m_fileArchives.snapshot();

            auto totalCount = static_cast<u32>( folderArchives.size() + fileArchives.size() );

            if( index >= totalCount )
            {
                return nullptr;
            }

            if( index < static_cast<u32>( folderArchives.size() ) )
            {
                // Return from folder archives
                return folderArchives[index];
            }
            else
            {
                // Return from file archives
                auto fileIndex = index - static_cast<u32>( folderArchives.size() );
                return fileArchives[fileIndex];
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto FileSystem::moveFileArchive( u32 sourceIndex, s32 relative ) -> bool
    {
        try
        {
            auto folderArchives = m_folderArchives.snapshot();
            auto fileArchives = m_fileArchives.snapshot();

            auto folderCount = static_cast<u32>( folderArchives.size() );
            auto fileCount = static_cast<u32>( fileArchives.size() );
            auto totalCount = folderCount + fileCount;

            // Validate source index
            if( sourceIndex >= totalCount )
            {
                return false;
            }

            // Calculate destination index
            auto destIndex = static_cast<s32>( sourceIndex ) + relative;
            if( destIndex < 0 || destIndex >= static_cast<s32>( totalCount ) )
            {
                return false;
            }

            // No movement needed
            if( relative == 0 )
            {
                return true;
            }

            auto destIndexU32 = static_cast<u32>( destIndex );

            // Determine source archive and collection
            SmartPtr<IArchive> sourceArchive;
            bool sourceInFolderArchives = sourceIndex < folderCount;

            if( sourceInFolderArchives )
            {
                sourceArchive = folderArchives[sourceIndex];
            }
            else
            {
                auto fileIndex = sourceIndex - folderCount;
                sourceArchive = fileArchives[fileIndex];
            }

            // Remove from current position
            if( sourceInFolderArchives )
            {
                removeFolderArchive( sourceArchive );
            }
            else
            {
                removeFileArchive( sourceArchive );
            }

            // Re-fetch snapshots after removal
            folderArchives = m_folderArchives.snapshot();
            fileArchives = m_fileArchives.snapshot();
            folderCount = static_cast<u32>( folderArchives.size() );

            // Determine destination collection and insert
            bool destInFolderArchives = destIndexU32 < folderCount + 1;

            if( destInFolderArchives )
            {
                // Insert into folder archives
                auto insertPos = destIndexU32;
                if( !sourceInFolderArchives && destIndexU32 > folderCount )
                {
                    insertPos = folderCount;
                }
                else if( sourceInFolderArchives && destIndexU32 > sourceIndex )
                {
                    // Adjust for the removed element
                    insertPos = destIndexU32;
                }

                m_folderArchives.insert( m_folderArchives.begin() + insertPos, sourceArchive );
            }
            else
            {
                // Insert into file archives
                auto fileInsertPos = destIndexU32 - ( folderCount + 1 );
                if( sourceInFolderArchives )
                {
                    fileInsertPos = destIndexU32 - folderCount;
                }
                else if( destIndexU32 > sourceIndex )
                {
                    fileInsertPos = destIndexU32 - folderCount - 1;
                }

                m_fileArchives.insert( m_fileArchives.begin() + fileInsertPos, sourceArchive );
            }

            return true;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    auto FileSystem::removeFileArchive( const String &filename ) -> bool
    {
        try
        {
            auto folderArchives = m_folderArchives.snapshot();
            auto fileArchives = m_fileArchives.snapshot();

            // Search in folder archives
            for( auto &archive : folderArchives )
            {
                if( archive )
                {
                    auto archivePath = archive->getPath();
                    if( StringUtil::isEqual( archivePath.c_str(), filename.c_str() ) )
                    {
                        removeFolderArchive( archive );
                        return true;
                    }
                }
            }

            // Search in file archives
            for( auto &archive : fileArchives )
            {
                if( archive )
                {
                    auto archivePath = archive->getPath();
                    if( StringUtil::isEqual( archivePath.c_str(), filename.c_str() ) )
                    {
                        removeFileArchive( archive );
                        return true;
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    auto FileSystem::removeFileArchive( u32 index ) -> bool
    {
        try
        {
            auto folderArchives = m_folderArchives.snapshot();
            auto fileArchives = m_fileArchives.snapshot();

            auto totalCount = static_cast<u32>( folderArchives.size() + fileArchives.size() );

            if( index >= totalCount )
            {
                return false;
            }

            if( index < static_cast<u32>( folderArchives.size() ) )
            {
                // Remove from folder archives
                auto archive = folderArchives[index];
                removeFolderArchive( archive );
                return true;
            }
            else
            {
                // Remove from file archives
                auto fileIndex = index - static_cast<u32>( folderArchives.size() );
                auto archive = fileArchives[fileIndex];
                removeFileArchive( archive );
                return true;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return false;
    }

    auto FileSystem::getFileArchiveCount() const -> u32
    {
        auto folderArchives = m_folderArchives.snapshot();
        auto fileArchives = m_fileArchives.snapshot();
        return static_cast<u32>( folderArchives.size() + fileArchives.size() );
    }

    auto FileSystem::readAllBytes( const String &path ) -> Array<u8>
    {
        if( Path::isPathAbsolute( path ) )
        {
            std::ifstream file( path.c_str(), std::ios::binary | std::ios::ate );
            if( file )
            {
                auto size = file.tellg();
                if( size > 0 )
                {
                    Array<u8> bytes( static_cast<size_t>( size ) );
                    file.seekg( 0, std::ios::beg );
                    file.read( reinterpret_cast<char *>( bytes.data() ), size );
                    bytes.resize( static_cast<size_t>( file.gcount() ) );
                    return bytes;
                }
            }
            return {};
        }

        auto stream = open( path, true, true, false, false, false );
        if( !stream )
        {
            stream = open( path, true, true, false, true, true );
        }

        if( stream )
        {
            auto size = stream->size();
            Array<u8> bytes;
            bytes.resize( size );
            bytes.resize( stream->read( bytes.data(), size ) );
            return bytes;
        }

        return {};
    }

    auto FileSystem::readAllText( const String &path ) -> String
    {
        if( Path::isPathAbsolute( path ) )
        {
            auto stream = new std::fstream( path.c_str(), std::fstream::in );

            auto pStream = workphone::make_ptr<FileDataStream>();
            pStream->setFStream( stream );
            return pStream->getAsString();
        }

        auto stream = open( path, true, false, false, false, false );
        if( !stream )
        {
            stream = open( path, true, false, false, true, true );
        }

        if( stream )
        {
            return stream->getAsString();
        }

        return {};
    }

    void FileSystem::writeAllBytes( const String &path, u8 *bytes, u32 size )
    {
        try
        {
            auto filePath = String();

            if( Path::isPathAbsolute( path ) )
            {
                filePath = path;
            }
            else
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                filePath = Path::lexically_normal( projectPath, path );
            }

            std::fstream fs;
            fs.open( StringUtil::str( filePath ),
                     std::fstream::binary | std::fstream::out | std::fstream::trunc );

            fs.write( reinterpret_cast<const char *>( bytes ), size );

            fs.close();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FileSystem::writeAllBytes( const String &path, Array<u8> bytes )
    {
        writeAllBytes( path, bytes.data(), (u32)bytes.size() );
    }

    void FileSystem::writeAllText( const String &path, const String &contents )
    {
        try
        {
            auto p = Path::getFilePath( path );
            if( !StringUtil::isNullOrEmpty( p ) )
            {
                if( !isExistingFolder( p ) )
                {
                    createDirectories( p );
                }
            }

            if( Path::isPathAbsolute( path ) )
            {
                std::fstream fs;
                fs.open( StringUtil::str( path ), std::fstream::out | std::fstream::trunc );

                fs << contents;

                fs.close();
            }
            else
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                auto filePath = Path::lexically_normal( projectPath, path );

                std::fstream fs;
                fs.open( StringUtil::str( filePath ), std::fstream::out | std::fstream::trunc );

                fs << contents;

                fs.close();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto FileSystem::getBase64String( SmartPtr<IStream> &pStream ) -> String
    {
        std::string tempData;
        tempData.reserve( 4096 );

        char buf[4096];
        size_t bytes_read = pStream->read( buf, sizeof( buf ) );
        while( bytes_read > 0 )
        {
            tempData.append( buf, bytes_read );
            bytes_read = pStream->read( buf, sizeof( buf ) );
        }

        return StringUtil::encodeBase64( (u8 *)tempData.c_str(), tempData.length() );
    }

    auto FileSystem::getBase64String( std::ifstream &is ) -> String
    {
        std::string tempData;
        tempData.reserve( 4096 );

        char buf[4096];
        while( is.read( buf, sizeof( buf ) ).gcount() > 0 )
        {
            tempData.append( buf, static_cast<size_t>( is.gcount() ) );
        }

        return StringUtil::encodeBase64( (u8 *)tempData.c_str(), tempData.length() );
    }

    auto FileSystem::getBytesString( SmartPtr<IStream> &pStream ) -> String
    {
        constexpr int bufferSize = 4096;

        std::string tempData;
        tempData.reserve( bufferSize );

        char buf[bufferSize];
        size_t bytes_read = pStream->read( buf, sizeof( buf ) );
        while( bytes_read > 0 )
        {
            tempData.append( buf, bytes_read );
            bytes_read = pStream->read( buf, sizeof( buf ) );
        }

        return tempData.c_str();
    }

    auto FileSystem::getBytesString( std::ifstream &is ) -> String
    {
        constexpr int bufferSize = 4096;

        std::string tempData;
        tempData.reserve( bufferSize );

        char buf[bufferSize];
        while( is.read( buf, sizeof( buf ) ).gcount() > 0 )
        {
            tempData.append( buf, static_cast<size_t>( is.gcount() ) );
        }

        return tempData.c_str();
    }

    void FileSystem::copyFolder( const String &srcPath, const String &dstPath )
    {
        Path::copyFolder( srcPath, dstPath );
    }

    void FileSystem::copyFile( const String &srcPath, const String &dstPath )
    {
        Path::copyFile( srcPath, dstPath );
    }

    void FileSystem::deleteFile( const String &filePath )
    {
        if( Path::isPathAbsolute( filePath ) )
        {
            Path::deleteFile( filePath );
        }
        else
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto absolutePath = Path::getAbsolutePath( projectPath, filePath );

            Path::deleteFile( absolutePath );
        }
    }

    auto FileSystem::getFilePath( const String &path ) -> String
    {
        return Path::getFilePath( path );
    }

    auto FileSystem::getFileName( const String &path ) -> String
    {
        return Path::getFileName( path );
    }

    auto FileSystem::getTypeFromTypeName( const String &typeName ) const -> IFileSystem::ArchiveType
    {
        if( typeName == "FileSystem" )
        {
            return ArchiveType::FileSystem;
        }
        if( typeName == "OBFUSZIP" )
        {
            return ArchiveType::ObfuscatedZip;
        }

        return ArchiveType::Unknown;
    }

    auto FileSystem::getFileHash( const String &pFilePath ) -> String
    {
        try
        {
            String filePath;

            if( Path::isPathAbsolute( pFilePath ) )
            {
                filePath = pFilePath;
            }
            else
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                filePath = Path::lexically_normal( projectPath, pFilePath );
            }

            std::ifstream is( filePath.c_str(), std::ios::in | std::ios::binary );
            if( !is.is_open() )
            {
                return {};
            }

#if WP_USE_BOOST
            boost::crc_32_type crc;

            constexpr size_t bufferSize = 4096;
            char buffer[bufferSize];

            while( is.read( buffer, bufferSize ).gcount() > 0 )
            {
                crc.process_bytes( buffer, static_cast<size_t>( is.gcount() ) );
            }

            is.close();

            std::ostringstream oss;
            oss << std::hex << std::setfill( '0' ) << std::setw( 8 ) << crc.checksum();
            return oss.str().c_str();
#else
            // Fallback using std::hash for non-boost builds
            std::string content;
            content.reserve( 4096 );

            char buffer[4096];
            while( is.read( buffer, sizeof( buffer ) ).gcount() > 0 )
            {
                content.append( buffer, static_cast<size_t>( is.gcount() ) );
            }

            is.close();

            std::size_t hash = std::hash<std::string>{}( content );

            std::ostringstream oss;
            oss << std::hex << std::setfill( '0' ) << std::setw( 16 ) << hash;
            return oss.str().c_str();
#endif
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    auto FileSystem::getFolders( const String &path, bool recursive ) -> Array<String>
    {
        return Path::getFolders( path, recursive );
    }

    auto FileSystem::getFiles( const String &path, bool partialPathMatch ) -> Array<String>
    {
        Array<String> results;

        try
        {
            for( auto &file : m_files )
            {
                if( !file.isDirectory )
                {
                    if( StringUtil::isEqual( file.path.c_str(), path.c_str() ) ||
                        ( partialPathMatch && file.path.find( path.c_str() ) != String::npos ) )
                    {
                        results.push_back( file.filePath.c_str() );
                    }
                }
            }

            for( auto &archive : m_folderArchives )
            {
                if( archive )
                {
                    if( auto fileList = archive->getFileList() )
                    {
                        auto files = fileList->getFiles();
                        for( auto &file : files )
                        {
                            if( !file.isDirectory )
                            {
                                if( StringUtil::isEqual( file.path.c_str(), path.c_str() ) ||
                                    ( partialPathMatch &&
                                      file.path.find( path.c_str() ) != String::npos ) )
                                {
                                    results.push_back( file.filePath.c_str() );
                                }
                            }
                        }
                    }
                }
            }

            for( auto archive : m_fileArchives )
            {
                if( archive )
                {
                    auto fileList = archive->getFileList();
                    auto files = fileList->getFiles();
                    for( auto &file : files )
                    {
                        if( !file.isDirectory )
                        {
                            if( StringUtil::isEqual( file.path.c_str(), path.c_str() ) ||
                                ( partialPathMatch && file.path.find( path.c_str() ) != String::npos ) )
                            {
                                results.push_back( file.filePath.c_str() );
                            }
                        }
                    }
                }
            }
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return results;
    }

    auto FileSystem::getFiles() const -> Array<FileInfo>
    {
        return m_files.snapshot();
    }

    void FileSystem::setFiles( const Array<FileInfo> &files )
    {
        m_files = { files.begin(), files.end() };
    }

    void FileSystem::addFolderArchive( SmartPtr<IArchive> archive )
    {
        m_folderArchives.push_back( archive );
    }

    void FileSystem::removeFolderArchive( SmartPtr<IArchive> archive )
    {
        m_folderArchives.erase( std::remove( m_folderArchives.begin(), m_folderArchives.end(), archive ),
                                m_folderArchives.end() );
    }

    void FileSystem::addFileArchive( SmartPtr<IArchive> archive )
    {
        m_fileArchives.push_back( archive );
    }

    void FileSystem::removeFileArchive( SmartPtr<IArchive> archive )
    {
        m_fileArchives.erase( std::remove( m_fileArchives.begin(), m_fileArchives.end(), archive ),
                              m_fileArchives.end() );
    }

    auto FileSystem::getFilesAsAbsolutePaths( const String &path, bool recursive ) -> Array<String>
    {
#if WP_USE_BOOST
        Array<String> files;

        try
        {
            std::filesystem::path dir = path.c_str();
            std::filesystem::directory_iterator it( dir, _wpFsEc() ), end;

            for( auto &entry : boost::make_iterator_range( it, end ) )
            {
                if( !is_directory( entry ) )
                {
                    auto str = entry.path().u8string();
                    files.push_back( str.c_str() );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return { files.begin(), files.end() };
#else
        return Array<String>();
#endif
    }

    auto FileSystem::getFolderListing( const String &path ) -> SmartPtr<IFolderExplorer>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto listing = factoryManager->make_ptr<FolderListing>();
            WP_ASSERT( listing );

            String cleanPath;

            if( Path::isPathAbsolute( path ) )
            {
                cleanPath = StringUtil::cleanupPath( path );
                listing->setFolderName( cleanPath );
            }
            else
            {
                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                cleanPath = Path::lexically_normal( projectPath, path );
                listing->setFolderName( cleanPath );
            }

            auto folders = Path::getFolders( cleanPath );
            for( auto &folder : folders )
            {
                auto directoryListing = getFolderListing( folder );
                listing->addSubFolder( directoryListing );
            }

            auto files = Path::getFiles( cleanPath );
            listing->setFiles( files );

            return listing;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto FileSystem::getFolderListing( const String &path, const String &ext )
        -> SmartPtr<IFolderExplorer>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto listing = factoryManager->make_ptr<FolderListing>();
            WP_ASSERT( listing );

            listing->setFolderName( path );

            auto folders = Path::getFolders( path );
            for( auto &folder : folders )
            {
                auto directoryListing = getFolderListing( folder );
                listing->addSubFolder( directoryListing );
            }

            auto files = Path::getFiles( path );
            listing->setFiles( files );

            return listing;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void FileSystem::addFile( const FileInfo &file )
    {
        m_files.push_back( file );
    }

    void FileSystem::addFiles( const Array<FileInfo> &newFiles )
    {
        m_files = { newFiles.begin(), newFiles.end() };
    }

    void FileSystem::removeFile( const FileInfo &file )
    {
        auto currentFiles = getFiles();
        auto it = std::find( currentFiles.begin(), currentFiles.end(), file );
        if( it != currentFiles.end() )
        {
            currentFiles.erase( it );
        }

        setFiles( currentFiles );
    }

    void FileSystem::removeFiles( const Array<FileInfo> &files )
    {
        for( auto file : files )
        {
            removeFile( file );
        }
    }

    auto FileSystem::findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath ) const -> bool
    {
        auto files = m_files.snapshot();
        for( auto &file : files )
        {
            if( file.fileId == id )
            {
                fileInfo = file;
                return true;
            }
        }

        auto folderArchives = m_folderArchives.snapshot();
        for( auto &a : folderArchives )
        {
            if( a )
            {
                if( a->findFileInfo( id, fileInfo, ignorePath ) )
                {
                    return true;
                }
            }
        }

        auto fileArchives = m_fileArchives.snapshot();
        for( auto &a : fileArchives )
        {
            if( a )
            {
                if( a->findFileInfo( id, fileInfo, ignorePath ) )
                {
                    return true;
                }
            }
        }

        return false;
    }

    auto FileSystem::findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath ) const
        -> bool
    {
        if( Path::isPathAbsolute( filePath ) )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto projectPath = applicationManager->getProjectPath();

            auto relativePath = Path::getRelativePath( projectPath, filePath );

            for( auto &file : m_files )
            {
                if( StringUtil::isEqual( file.absolutePath.c_str(), filePath.c_str() ) )
                {
                    fileInfo = file;
                    return true;
                }
            }
        }
        else
        {
            for( auto &file : m_files )
            {
                if( StringUtil::isEqual( file.filePath.c_str(), filePath.c_str() ) ||
                    StringUtil::isEqual( file.filePathLowerCase.c_str(), filePath.c_str() ) )
                {
                    fileInfo = file;
                    return true;
                }
            }

            auto fileName = Path::getFileName( filePath );
            for( auto &file : m_files )
            {
                if( StringUtil::isEqual( file.fileName.c_str(), fileName.c_str() ) ||
                    StringUtil::isEqual( file.fileNameLowerCase.c_str(), fileName.c_str() ) )
                {
                    fileInfo = file;
                    return true;
                }
            }
        }

        for( auto &a : m_folderArchives )
        {
            if( a )
            {
                if( a->findFileInfo( filePath, fileInfo, ignorePath ) )
                {
                    return true;
                }
            }
        }

        for( auto &a : m_fileArchives )
        {
            if( a->findFileInfo( filePath, fileInfo, ignorePath ) )
            {
                return true;
            }
        }

        return false;
    }

    auto FileSystem::getSystemFiles() const -> Array<FileInfo>
    {
        return m_files.snapshot();
    }

    void FileSystem::refreshAll( bool async )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();

            if( async )
            {
                auto jobQueue = applicationManager->getJobQueue();

                jobQueue->startJob( [this]() {
                    auto applicationManager = core::IApplicationManager::instance();

                    auto folderArchives = m_folderArchives.snapshot();
                    for( auto archive : folderArchives )
                    {
                        archive->reload( nullptr );
                    }

                    auto parameters = Array<Parameter>();
                    applicationManager->triggerEvent( EventType::IO, IEvent::refreshAll, parameters,
                                                      this, nullptr, nullptr, false,
                                                      Thread::Application_Flag );
                } );
            }
            else
            {
                auto folderArchives = m_folderArchives.snapshot();
                for( auto archive : folderArchives )
                {
                    archive->reload( nullptr );
                }

                applicationManager->triggerEvent( EventType::IO, IEvent::refreshAll, Array<Parameter>(),
                                                  this, nullptr, nullptr, false,
                                                  Thread::Application_Flag );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FileSystem::refreshPath( const String &path, bool async )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto jobQueue = applicationManager->getJobQueue();

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto relativePath = String();
            auto absolutePath = String();

            if( Path::isPathAbsolute( path ) )
            {
                relativePath = Path::getRelativePath( projectPath, path );
                absolutePath = path;
            }
            else
            {
                relativePath = path;
                absolutePath = Path::lexically_normal( projectPath, path );
            }

            if( async )
            {
                jobQueue->startJob( [this, relativePath, absolutePath]() {
                    auto applicationManager = core::IApplicationManager::instance();

                    for( auto archive : m_folderArchives )
                    {
                        auto archivePath = archive->getPath();
                        if( archivePath == relativePath || archivePath == absolutePath )
                        {
                            archive->reload( nullptr );
                        }
                    }

                    auto parameters = Array<Parameter>();
                    parameters.resize( 1 );
                    parameters[0].setStr( relativePath );

                    applicationManager->triggerEvent( EventType::IO, IEvent::refreshPath, parameters,
                                                      this, nullptr, nullptr, false,
                                                      Thread::Application_Flag );
                } );
            }
            else
            {
                for( auto archive : m_folderArchives )
                {
                    auto archivePath = archive->getPath();
                    if( archivePath == path || archivePath == absolutePath )
                    {
                        archive->reload( nullptr );
                    }
                }

                auto parameters = Array<Parameter>();
                parameters.resize( 1 );
                parameters[0].setStr( path );

                applicationManager->triggerEvent( EventType::IO, IEvent::refreshPath, parameters, this,
                                                  nullptr, nullptr, false, Thread::Application_Flag );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    UUID FileSystem::getFileId( const String &filePath ) const
    {
        FileInfo fileInfo;
        if( this->findFileInfo( filePath, fileInfo, false ) )
        {
            return fileInfo.fileId;
        }

        return {};
    }

    void FileSystem::lock()
    {
        // do nothing
    }

    void FileSystem::unlock()
    {
        // do nothing
    }

    auto FileSystem::isValid() const -> bool
    {
        return true;
    }
}  // namespace workphone
