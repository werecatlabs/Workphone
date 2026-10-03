#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/FileList.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/IO/MemoryFile.hpp>
#include <Workphone/IO/ZipArchive.hpp>
#include <Workphone/IO/ZipFile.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <cstdio>
#include <cstdlib>
#include <zzip/plugin.h>
#include <zzip/zzip.h>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ZipArchive, IArchive );

    ZipArchive::ZipArchive() = default;

    ZipArchive::ZipArchive( const String &name, bool ignoreCase, bool ignorePaths ) :
        m_path( name.c_str() )
    {
    }

    ZipArchive::~ZipArchive()
    {
        unload( nullptr );
    }

    void ZipArchive::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        Array<FileInfo> fileInfoList;
        fileInfoList.reserve( 32 );

        static zzip_strings_t zipFileExt[] = { ".zip", nullptr };

        auto name = getPath();

        m_fileList = factoryManager->make_ptr<FileList>();
        m_fileList->setPath( name );

        if( !m_zzipDir )
        {
            zzip_error_t zzipError;
            m_zzipDir = zzip_dir_open( name.c_str(), &zzipError );
            if( zzipError != ZZIP_NO_ERROR )
            {
                WP_LOG_ERROR( "Error opening archive: " + name );
            }

            // Cache names
            ZZIP_DIRENT zzipEntry;
            while( zzip_dir_read( m_zzipDir, &zzipEntry ) )
            {
                FileInfo fileInfo;
                // info.archive = this;

                // Get basename / path
                String fileName;
                String path;

                StringUtil::splitFilename( zzipEntry.d_name, fileName, path );
                path = StringUtil::cleanupPath( path );

#ifdef _DEBUG
                if( std::string( zzipEntry.d_name ).find( ".program" ) != std::string::npos )
                {
                    int stop = 0;
                    stop = 0;
                }

                if( fileName.find( "PagedGeometry.program" ) != std::string::npos )
                {
                    int stop = 0;
                    stop = 0;
                }
#endif

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

                fileInfo.filePath = filePath.c_str();
                fileInfo.filePathLowerCase = StringUtil::make_lower( fileInfo.filePath.c_str() ).c_str();

                fileInfo.path = path.c_str();

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

                fileInfoList.push_back( fileInfo );
            }

            m_fileList->setFiles( fileInfoList );
        }

        setLoadingState( LoadingState::Loaded );
    }

    void ZipArchive::reload( SmartPtr<ISharedObject> data )
    {
    }

    void ZipArchive::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        if( m_zzipDir )
        {
            zzip_dir_close( m_zzipDir );
            m_zzipDir = nullptr;
        }

        setLoadingState( LoadingState::Unloaded );
    }

    auto ZipArchive::getType() const -> u8
    {
        return static_cast<u8>( IFileSystem::ArchiveType::Zip );
    }

    auto ZipArchive::getPassword() const -> String
    {
        return m_password;
    }

    void ZipArchive::setPassword( const String &password )
    {
        m_password = password;
    }

    auto ZipArchive::open( const String &filename, bool input, bool binary, bool truncate,
                           bool ignorePath, bool ignoreCase ) -> SmartPtr<IStream>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        FileInfo fileInfo;
        m_fileList->findFileInfo( filename, fileInfo, ignorePath );

        //auto fileIndex = m_fileList->findFile( filename, false );
        //if( fileIndex == -1 )
        //{
        //    return nullptr;
        //}

        auto &filePath = fileInfo.filePath;

        ZZIP_STAT zstat;

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

        auto zzipFile = zzip_file_open( m_zzipDir, filename.c_str(), flags );
        if( !zzipFile )
        {
            zzipFile = zzip_file_open( m_zzipDir, filePath.c_str(), ZZIP_ONLYZIP | ZZIP_CASELESS );
            zzip_dir_stat( m_zzipDir, filePath.c_str(), &zstat, ZZIP_CASEINSENSITIVE );
        }
        else
        {
            zzip_dir_stat( m_zzipDir, filename.c_str(), &zstat, ZZIP_CASEINSENSITIVE );
        }

        if( !zzipFile )
        {
            auto zerr = zzip_error( m_zzipDir );
            auto zzDesc = getZzipErrorDescription( zerr );
            WP_LOG_INFO( String( " - Unable to open file " ) + filename + ", error was '" + zzDesc +
                         "'" );
            return nullptr;
        }

        auto size = static_cast<size_t>( zstat.st_size );
        WP_ASSERT( size < 1e8 );

        auto zipFile = factoryManager->make_ptr<ZipFile>();
        zipFile->setArchive( this );
        zipFile->setFileName( filename );
        zipFile->setZipFile( zzipFile );
        zipFile->setSize( size );
        zipFile->load( nullptr );
        return zipFile;
    }

    auto ZipArchive::exists( const String &filename, bool ignorePath, bool ignoreCase ) const -> bool
    {
        return m_fileList->exists( filename, ignorePath, ignoreCase );
    }

    auto ZipArchive::isReadOnly() const -> bool
    {
        return false;
    }

    auto ZipArchive::getPath() const -> String
    {
        return String( m_path.c_str() );
    }

    void ZipArchive::setPath( const String &path )
    {
        m_path = path.c_str();
    }

    auto ZipArchive::getIgnorePaths() const -> bool
    {
        return m_ignorePaths;
    }

    void ZipArchive::setIgnorePaths( bool ignorePaths )
    {
        m_ignorePaths = ignorePaths;
    }

    auto ZipArchive::findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath,
                                   bool ignoreCase ) const -> bool
    {
        return m_fileList->findFileInfo( filePath, fileInfo, ignorePath, ignoreCase );
    }

    auto ZipArchive::findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath, bool ignoreCase ) const
        -> bool
    {
        return m_fileList->findFileInfo( id, fileInfo, ignorePath, ignoreCase );
    }

    auto ZipArchive::getFileList() const -> SmartPtr<IFileList>
    {
        return m_fileList;
    }

    auto ZipArchive::getFiles() const -> Array<FileInfo>
    {
        auto files = m_fileList->getFiles();
        return files.snapshot();
    }

    /// Utility method to format out zzip errors
    auto ZipArchive::getZzipErrorDescription( s32 zzipError ) -> String
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
