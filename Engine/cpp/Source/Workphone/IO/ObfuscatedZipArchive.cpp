#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/ObfuscatedZipArchive.hpp>
#include <Workphone/IO/ObfuscatedZipFile.hpp>
#include <Workphone/IO/FileList.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <filesystem>
#include <memory>
#include <zzip/plugin.h>
#include <zzip/zzip.h>

#ifdef WP_PLATFORM_WIN32
#    include <io.h>
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ObfuscatedZipArchive, IArchive );

    namespace
    {
        constexpr unsigned char archiveXor = 13;

#ifdef WP_PLATFORM_WIN32
        int openWideArchive( zzip_char_t *name, int flags, ... )
        {
            try
            {
                const auto path = std::filesystem::u8path( name );
                return _wopen( path.c_str(), flags );
            }
            catch( const std::exception & )
            {
                errno = EINVAL;
                return -1;
            }
        }
#endif

        auto readObfuscatedZip( int fd, void *buffer, zzip_size_t length ) -> zzip_ssize_t
        {
#ifdef WP_PLATFORM_WIN32
            const auto bytes = _read( fd, buffer, static_cast<unsigned int>( length ) );
#else
            const auto bytes = read( fd, buffer, length );
#endif
            auto data = static_cast<unsigned char *>( buffer );
            for( zzip_ssize_t i = 0; i < bytes; ++i )
            {
                data[i] ^= archiveXor;
            }
            return bytes;
        }

        auto archiveIo() -> zzip_plugin_io_handlers *
        {
            static auto handlers = [] {
                zzip_plugin_io_handlers io{};
                zzip_init_io( &io, 0 );
#ifdef WP_PLATFORM_WIN32
                io.fd.open = openWideArchive;
#endif
                io.fd.read = readObfuscatedZip;
                return io;
            }();
            return &handlers;
        }

        auto lowerArchivePath( String path ) -> String
        {
            std::transform( path.begin(), path.end(), path.begin(), []( unsigned char ch ) {
                return static_cast<char>( std::tolower( ch ) );
            } );
            return path;
        }

        auto archiveEntryPath( const String &filename ) -> String
        {
            auto path = StringUtil::cleanupPath( filename );
            if( !Path::isPathAbsolute( path ) )
            {
                return path;
            }

            auto application = core::IApplicationManager::instancePtr();
            if( !application )
            {
                return path;
            }

            auto project = StringUtil::cleanupPath( application->getProjectPath() );
            if( project.empty() )
            {
                return path;
            }
            if( project.back() != '/' )
            {
                project += '/';
            }

            const auto equalPathChar = []( unsigned char left, unsigned char right ) {
#ifdef WP_PLATFORM_WIN32
                return std::tolower( left ) == std::tolower( right );
#else
                return left == right;
#endif
            };
            if( path.size() > project.size() &&
                std::equal( project.begin(), project.end(), path.begin(), equalPathChar ) )
            {
                return path.substr( project.size() );
            }
            return path;
        }
    }  // namespace

    ObfuscatedZipArchive::ObfuscatedZipArchive() = default;

    ObfuscatedZipArchive::ObfuscatedZipArchive( const String &name, bool ignoreCase, bool ignorePaths ) :

        m_path( name.c_str() )
    {
    }

    ObfuscatedZipArchive::~ObfuscatedZipArchive()
    {
        unload( nullptr );
    }

    void ObfuscatedZipArchive::load( SmartPtr<ISharedObject> )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        if( mZzipDir )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );
        const auto name = getPath();
        zzip_error_t error = ZZIP_NO_ERROR;
        static zzip_strings_t extensions[] = { nullptr };
        const auto closeDirectory = []( ZZIP_DIR *directory ) { zzip_dir_close( directory ); };
        std::unique_ptr<ZZIP_DIR, decltype( closeDirectory )> directory(
            zzip_dir_open_ext_io( name.c_str(), &error, extensions, archiveIo() ), closeDirectory );
        if( !directory )
        {
            WP_LOG_ERROR( "Unable to open packed archive " + name + ": " +
                          getZzipErrorDescription( error ) );
            setLoadingState( LoadingState::Unloaded );
            return;
        }

        try
        {
            Array<FileInfo> files;
            ZZIP_DIRENT entry{};
            while( zzip_dir_read( directory.get(), &entry ) )
            {
                const String rawPath( entry.d_name );
                const auto filePath = StringUtil::cleanupPath( rawPath );
                if( filePath.empty() || rawPath.size() >= WP_MAX_PATH || entry.d_csize < 0 ||
                    entry.st_size < 0 )
                {
                    continue;
                }

                FileInfo info;
                const auto fileName = Path::getFileName( filePath ).substr( 0, WP_MAX_FILENAME - 1 );
                info.fileName = fileName.c_str();
                info.fileNameLowerCase = lowerArchivePath( fileName ).c_str();
                // zzip requires the entry's original separators when opening legacy archives.
                info.filePath = rawPath.c_str();
                info.filePathLowerCase = lowerArchivePath( filePath ).c_str();
                info.path = Path::getFilePath( filePath ).c_str();
                info.absolutePath = filePath.c_str();
                info.compressedSize = static_cast<u32>( entry.d_csize );
                info.uncompressedSize = static_cast<u32>( entry.st_size );
                info.isDirectory = rawPath.back() == '/' || rawPath.back() == '\\';
                info.fileId = StringUtil::getUUID( filePath );
                files.push_back( info );
            }

            auto fileList = workphone::make_ptr<FileList>( name );
            fileList->setFiles( files );
            m_fileList = fileList;
            mZzipDir = directory.release();
            setLoadingState( LoadingState::Loaded );
        }
        catch( ... )
        {
            setLoadingState( LoadingState::Unloaded );
            throw;
        }
    }

    void ObfuscatedZipArchive::reload( SmartPtr<ISharedObject> data )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        unload( data );
        load( data );
    }

    void ObfuscatedZipArchive::unload( SmartPtr<ISharedObject> )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        setLoadingState( LoadingState::Unloading );
        if( mZzipDir )
        {
            zzip_dir_close( mZzipDir );
            mZzipDir = nullptr;
        }
        m_fileList = nullptr;
        setLoadingState( LoadingState::Unloaded );
    }

    auto ObfuscatedZipArchive::getType() const -> u8
    {
        return static_cast<u8>( IFileSystem::ArchiveType::ObfuscatedZip );
    }

    auto ObfuscatedZipArchive::getPassword() const -> String
    {
        return m_password;
    }

    void ObfuscatedZipArchive::setPassword( const String &password )
    {
        m_password = password;
    }

    auto ObfuscatedZipArchive::open( const String &filename, bool input, bool, bool,
                                     bool ignorePath, bool ignoreCase ) -> SmartPtr<IStream>
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        FileInfo info;
        if( !mZzipDir || !input || !findFileInfo( filename, info, ignorePath, ignoreCase ) ||
            info.isDirectory )
        {
            return nullptr;
        }

        const auto filePath = info.filePath.c_str();
        auto zzipFile = zzip_file_open( mZzipDir, filePath, ZZIP_ONLYZIP );
        if( !zzipFile )
        {
            auto zerr = zzip_error( mZzipDir );
            auto zzDesc = getZzipErrorDescription( zerr );
            WP_LOG_INFO( String( " - Unable to open file " ) + filename + ", error was '" + zzDesc +
                         "'" );
            return nullptr;
        }

        const auto closeFile = []( ZZIP_FILE *file ) { zzip_file_close( file ); };
        std::unique_ptr<ZZIP_FILE, decltype( closeFile )> file( zzipFile, closeFile );
        auto stream = workphone::make_ptr<ObfuscatedZipFile>( this, info.fileName.c_str(), file.get(),
                                                             static_cast<u32>( info.uncompressedSize ) );
        file.release();
        stream->setFileInfo( info );
        return stream;
    }

    auto ObfuscatedZipArchive::exists( const String &filename, bool ignorePath, bool ignoreCase ) const
        -> bool
    {
        FileInfo info;
        return findFileInfo( filename, info, ignorePath, ignoreCase );
    }

    auto ObfuscatedZipArchive::isReadOnly() const -> bool
    {
        return true;
    }

    auto ObfuscatedZipArchive::getPath() const -> String
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return { m_path.c_str() };
    }

    void ObfuscatedZipArchive::setPath( const String &path )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        m_path = path.c_str();
    }

    auto ObfuscatedZipArchive::findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath,
                                             bool ignoreCase ) const -> bool
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        if( !m_fileList )
        {
            return false;
        }

        const auto path = archiveEntryPath( filePath );
        return m_fileList->findFileInfo( path, fileInfo, false, ignoreCase ) ||
               ( ignorePath && m_fileList->findFileInfo( path, fileInfo, true, ignoreCase ) );
    }

    auto ObfuscatedZipArchive::findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath,
                                             bool ignoreCase ) const -> bool
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        if( !m_fileList )
        {
            return false;
        }
        if( m_fileList->findFileInfo( id, fileInfo, false, false ) )
        {
            return true;
        }
        return findFileInfo( fileInfo.filePath.c_str(), fileInfo, ignorePath, ignoreCase );
    }

    auto ObfuscatedZipArchive::getFileList() const -> SmartPtr<IFileList>
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_fileList;
    }

    auto ObfuscatedZipArchive::getFiles() const -> Array<FileInfo>
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_fileList ? m_fileList->getFiles().snapshot() : Array<FileInfo>{};
    }

    /// Utility method to format out zzip errors
    auto ObfuscatedZipArchive::getZzipErrorDescription( s32 zzipError ) -> String
    {
        String errorMsg;
        switch( zzipError )
        {
        case ZZIP_NO_ERROR:
            break;
        case ZZIP_OUTOFMEM:
            errorMsg = "Out of memory.";
            break;
        case ZZIP_DIR_OPEN:
        case ZZIP_DIR_STAT:
        case ZZIP_DIR_SEEK:
        case ZZIP_DIR_READ:
            errorMsg = "Unable to read zip file.";
            break;
        case ZZIP_UNSUPP_COMPR:
            errorMsg = "Unsupported compression format.";
            break;
        case ZZIP_CORRUPTED:
            errorMsg = "Corrupted archive.";
            break;
        default:
            errorMsg = "Unknown error.";
            break;
        }

        return errorMsg;
    }
}  // namespace workphone
