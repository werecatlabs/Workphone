#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/ObfuscatedZipArchive.hpp>
#include <Workphone/IO/ObfuscatedZipFile.hpp>
#include <Workphone/IO/FileList.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/IO/MemoryFile.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <cstdio>
#include <cstdlib>
#include <zzip/plugin.h>
#include <zzip/zzip.h>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ObfuscatedZipArchive, IArchive );

    const int XOR_VALUE = 13;

    // Change this magic number to a value of your choosing.
    static int xor_value = 0;
    static zzip_plugin_io_handlers xor_handlers = {};
    // Change "OBFUSZIP" to match the file extension of your choosing.
    static zzip_strings_t xor_fileext[] = { ".zip", nullptr };

    _zzip_plugin_io ObfuscatedZip_PluginIo;

    std::map<s32, SmartPtr<IStream>> gObfuscatedZipFiles;

    // Static method that un-obfuscates an obfuscated file.
    static auto xor_read( int fd, void *buf, zzip_size_t len ) -> zzip_ssize_t
    {
#if WP_COMPILER == WP_COMPILER_MSVC
        auto bytes = _read( fd, buf, static_cast<u32>( len ) );
#else
        auto bytes = read( fd, buf, len );
#endif

        auto pch = static_cast<char *>( buf );
        for( size_t i = 0; i < bytes; ++i )
        {
            pch[i] ^= XOR_VALUE;
        }

        return bytes;
    }

    ObfuscatedZipArchive::ObfuscatedZipArchive() = default;

    ObfuscatedZipArchive::ObfuscatedZipArchive( const String &name, bool ignoreCase, bool ignorePaths ) :

        m_path( name.c_str() )
    {
    }

    ObfuscatedZipArchive::~ObfuscatedZipArchive()
    {
        unload( nullptr );
    }

    void ObfuscatedZipArchive::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        if( !xor_value )
        {
            xor_value = 13;
        }

        zzip_init_io( &xor_handlers, 0 );
        xor_handlers.fd.read = &xor_read;

        auto name = getPath();

        m_fileList = workphone::make_ptr<FileList>( name );

        if( !mZzipDir )
        {
            zzip_error_t zzipError;
            // mZzipDir = zzip_dir_open(name.c_str(), &zzipError);
            mZzipDir = zzip_dir_open_ext_io( name.c_str(), &zzipError, xor_fileext, &xor_handlers );
            if( zzipError != ZZIP_NO_ERROR )
            {
                WP_LOG_ERROR( "Error opening archive: " + name );
            }

            // Cache names
            ZZIP_DIRENT zzipEntry;
            while( zzip_dir_read( mZzipDir, &zzipEntry ) )
            {
                FileInfo fileInfo;
                // info.archive = this;

                // Get basename / path
                String fileName;
                String path;

                StringUtil::splitFilename( zzipEntry.d_name, fileName, path );
                path = StringUtil::cleanupPath( path );

                String filePath;
                if( !StringUtil::isNullOrEmpty( path ) )
                {
                    filePath = path + "/" + fileName;
                }
                else
                {
                    filePath = fileName;
                }

                filePath = StringUtil::cleanupPath( filePath );

                fileInfo.fileName = fileName.c_str();
                fileInfo.fileNameLowerCase = StringUtil::make_lower( fileInfo.fileName.c_str() ).c_str();

                fileInfo.filePath = zzipEntry.d_name;
                fileInfo.filePathLowerCase = StringUtil::make_lower( fileInfo.filePath.c_str() ).c_str();

                fileInfo.path = filePath.c_str();

                fileInfo.absolutePath = filePath.c_str();

                // Get sizes
                fileInfo.compressedSize = static_cast<u32>( zzipEntry.d_csize );
                fileInfo.uncompressedSize = static_cast<u32>( zzipEntry.st_size );

                fileInfo.isDirectory = fileName.empty();

                fileInfo.fileId = StringUtil::getUUID( filePath );

                // folder entries
                if( fileInfo.fileName.empty() )
                {
                    fileInfo.fileName = fileInfo.fileName.substr( 0, fileInfo.fileName.length() - 1 );
                    //StringUtil::splitFilename(fileInfo.filename, fileInfo.basename, fileInfo.path);
                    // Set compressed size to -1 for folders; anyway nobody will check
                    // the compressed size of a folder, and if he does, its useless anyway
                    fileInfo.compressedSize = static_cast<u32>( -1 );
                }

                m_fileInfoList.push_back( fileInfo );
            }
        }

        m_fileList->setFiles( m_fileInfoList );

        setLoadingState( LoadingState::Loaded );
    }

    void ObfuscatedZipArchive::reload( SmartPtr<ISharedObject> data )
    {
    }

    void ObfuscatedZipArchive::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        if( mZzipDir )
        {
            zzip_dir_close( mZzipDir );
            mZzipDir = nullptr;
        }

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

    auto ObfuscatedZipArchive::open( const String &filename, bool input, bool binary, bool truncate,
                                     bool ignorePath, bool ignoreCase ) -> SmartPtr<IStream>
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        auto filePath = filename;
        if( ignorePath )
        {
            filePath = Path::getFileName( filePath );
        }

        auto flags = ZZIP_CASELESS;
        if( binary )
        {
            flags |= ZZIP_ONLYZIP;
        }
        else
        {
            flags |= ZZIP_PREFERZIP;
        }

        if( ignorePath )
        {
            flags |= ZZIP_NOPATHS;
        }

        if( ignoreCase )
        {
            flags |= ZZIP_CASEINSENSITIVE;
        }

        auto zzipFile = zzip_file_open( mZzipDir, filePath.c_str(), flags );

        if( !zzipFile )
        {
            auto zerr = zzip_error( mZzipDir );
            auto zzDesc = getZzipErrorDescription( zerr );
            WP_LOG_INFO( String( " - Unable to open file " ) + filename + ", error was '" + zzDesc +
                         "'" );
            return nullptr;
        }

        // Get uncompressed size too
        ZZIP_STAT zstat;
        zzip_dir_stat( mZzipDir, filePath.c_str(), &zstat, ZZIP_CASEINSENSITIVE );

        auto size = static_cast<size_t>( zstat.st_size );
        return workphone::make_ptr<ObfuscatedZipFile>( this, filePath, zzipFile,
                                                       static_cast<u32>( size ) );
    }

    auto ObfuscatedZipArchive::exists( const String &filename, bool ignorePath, bool ignoreCase ) const
        -> bool
    {
        return m_fileList->exists( filename, ignorePath, ignoreCase );
    }

    auto ObfuscatedZipArchive::isReadOnly() const -> bool
    {
        return false;
    }

    auto ObfuscatedZipArchive::getPath() const -> String
    {
        return { m_path.c_str() };
    }

    void ObfuscatedZipArchive::setPath( const String &path )
    {
        m_path = path.c_str();
    }

    auto ObfuscatedZipArchive::findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath,
                                             bool ignoreCase ) const -> bool
    {
        return m_fileList->findFileInfo( filePath, fileInfo, ignorePath, ignoreCase );
    }

    auto ObfuscatedZipArchive::findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath,
                                             bool ignoreCase ) const -> bool
    {
        return m_fileList->findFileInfo( id, fileInfo, ignorePath, ignoreCase );
    }

    auto ObfuscatedZipArchive::getFileList() const -> SmartPtr<IFileList>
    {
        return m_fileList;
    }

    auto ObfuscatedZipArchive::getFiles() const -> Array<FileInfo>
    {
        auto files = m_fileList->getFiles();
        return files.snapshot();
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
