#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/FileSystemArchive.hpp>
#include <Workphone/IO/FileDataStream.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/IO/FileSystemArchive.hpp>
#include <Workphone/IO/MemoryFile.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <fstream>

namespace workphone
{
    namespace
    {
        bool archivePathCharEqual( char lhs, char rhs, bool ignoreCase )
        {
            if( lhs == '\\' )
            {
                lhs = '/';
            }

            if( rhs == '\\' )
            {
                rhs = '/';
            }

            if( ignoreCase )
            {
                lhs = static_cast<char>( std::tolower( static_cast<unsigned char>( lhs ) ) );
                rhs = static_cast<char>( std::tolower( static_cast<unsigned char>( rhs ) ) );
            }

            return lhs == rhs;
        }

        bool archivePathEqual( const String &storedPath, const char *path, size_t pathLength,
                               bool ignoreCase )
        {
            if( storedPath.size() != pathLength )
            {
                return false;
            }

            for( size_t i = 0; i < pathLength; ++i )
            {
                if( !archivePathCharEqual( storedPath[i], path[i], ignoreCase ) )
                {
                    return false;
                }
            }

            return true;
        }

        bool archiveFileNameEqual( const String &storedFileName, const String &filePath,
                                   bool ignoreCase )
        {
            auto fileNameOffset = filePath.find_last_of( "/\\" );
            fileNameOffset =
                fileNameOffset == String::npos ? 0 : static_cast<size_t>( fileNameOffset + 1 );

            return archivePathEqual( storedFileName, filePath.c_str() + fileNameOffset,
                                     filePath.size() - fileNameOffset, ignoreCase );
        }

        bool archiveIsAbsolutePath( const String &filePath )
        {
            if( filePath.empty() )
            {
                return false;
            }

            if( filePath[0] == '/' || filePath[0] == '\\' )
            {
                return true;
            }

            return filePath.size() > 2 && std::isalpha( static_cast<unsigned char>( filePath[0] ) ) &&
                   filePath[1] == ':' && ( filePath[2] == '/' || filePath[2] == '\\' );
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, FileSystemArchive, IArchive );

    FileSystemArchive::FileSystemArchive() = default;

    FileSystemArchive::FileSystemArchive( const String &path, bool ignoreCase, bool ignorePaths ) :
        m_path( path )
    {
    }

    FileSystemArchive::~FileSystemArchive()
    {
        unload( nullptr );
    }

    void FileSystemArchive::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto workingDirectory = Path::getWorkingDirectory();

            auto path = getPath();
            if( StringUtil::isNullOrEmpty( path ) )
            {
                path = workingDirectory;
            }

            auto fileList = factoryManager->make_ptr<FileList>();
            fileList->setPath( path );
            //WP_ASSERT( fileList->getReferences() == 1 );
            m_fileList = fileList;

            auto files = Path::getFiles( path );

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            for( const auto &file : files )
            {
                FileInfo fileInfo;

                auto filePath = String();
                auto absolutePath = String();

                auto cleanFilePath = StringUtil::cleanupPath( file );

                if( Path::isPathAbsolute( cleanFilePath ) )
                {
                    filePath = Path::lexically_relative( projectPath, cleanFilePath );
                    absolutePath = cleanFilePath;
                }
                else
                {
                    filePath = cleanFilePath;
                    absolutePath = Path::lexically_normal( projectPath, cleanFilePath );
                }

                auto fileName = Path::getFileName( filePath );
                //WP_ASSERT( fileInfo.fileName.capacity() > fileName.size() );
                fileInfo.fileName = fileName.c_str();

                auto fileNameLowerCase = StringUtil::make_lower( fileName );
                //WP_ASSERT( fileInfo.fileNameLowerCase.capacity() > fileNameLowerCase.size() );
                fileInfo.fileNameLowerCase = fileNameLowerCase.c_str();

                //WP_ASSERT( fileInfo.filePath.capacity() > filePath.size() );
                fileInfo.filePath = filePath.c_str();

                auto filePathLowerCase = StringUtil::make_lower( filePath );
                //WP_ASSERT( fileInfo.filePathLowerCase.capacity() > filePathLowerCase.size() );
                fileInfo.filePathLowerCase = filePathLowerCase.c_str();

                //WP_ASSERT( fileInfo.absolutePath.capacity() > path.size() );
                fileInfo.path = path.c_str();

                //WP_ASSERT( fileInfo.path.capacity() > absolutePath.size() );

                if( !Path::isPathAbsolute( absolutePath ) )
                {
                    absolutePath = Path::lexically_normal( workingDirectory, file );
                }

                WP_ASSERT( Path::isPathAbsolute( absolutePath ) );
                fileInfo.absolutePath = absolutePath.c_str();

                fileInfo.isDirectory = Path::isFolder( absolutePath );

                fileInfo.fileId = StringUtil::getUUID( filePath );

                addFile( fileInfo );
            }

            //WP_ASSERT( m_fileList->getReferences() == 1 );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );

            if( auto fileList = getFileList() )
            {
                auto state = fileList->getLoadingState();
                if( state == LoadingState::Loaded || state == LoadingState::Loading ||
                    state == LoadingState::Unloading )
                {
                    fileList->unload( nullptr );
                }
            }

            setFileList( nullptr );
        }
    }

    void FileSystemArchive::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( data );
            load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FileSystemArchive::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Unloaded ||
                getLoadingState() == LoadingState::Unloading )
            {
                return;
            }

            setLoadingState( LoadingState::Unloading );

            if( auto fileList = getFileList() )
            {
                fileList->getFiles().clear();
                fileList->setPath( String() );

                // Guard against use-after-free: the FileList may have already been
                // destroyed by FactoryManager before this destructor runs.
                // Only call unload when the object is in an actively-alive state.
                // A destroyed pool element has its vtable reset to IObject and its
                // LoadingState set to None (0), which is not Unloaded (5) — so a
                // simple "!= Unloaded" check incorrectly passes for destroyed objects,
                // leading to a null vtable slot call and a crash at 0x0.
                auto state = fileList->getLoadingState();
                if( state == LoadingState::Loaded || state == LoadingState::Loading ||
                    state == LoadingState::Unloading )
                {
                    fileList->unload( nullptr );
                }
            }

            setFileList( nullptr );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto FileSystemArchive::getFileList() const -> SmartPtr<IFileList>
    {
        return m_fileList.load();
    }

    void FileSystemArchive::setFileList( SmartPtr<IFileList> fileList )
    {
        m_fileList = fileList;
    }

    auto FileSystemArchive::getFiles() const -> Array<FileInfo>
    {
        if( auto fileList = getFileList() )
        {
            auto &files = fileList->getFiles();
            return files.snapshot();
        }

        return {};
    }

    auto FileSystemArchive::getType() const -> u8
    {
        return static_cast<u8>( IFileSystem::ArchiveType::Folder );
    }

    auto FileSystemArchive::getPassword() const -> String
    {
        return m_password.load();
    }

    void FileSystemArchive::setPassword( const String &password )
    {
        m_password = password;
    }

    auto FileSystemArchive::open( const String &filePath, bool input, bool binary, bool truncate,
                                  bool ignorePath, bool ignoreCase ) -> SmartPtr<IStream>
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( filePath ) );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto factoryManager = applicationManager->getFactoryManagerPtr();

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            FileInfo fileInfo;
            if( findFileInfo( filePath, fileInfo, ignorePath, ignoreCase ) )
            {
                c8 *mode = nullptr;

                if( input )
                {
                    mode = "rb";
                }
                else
                {
                    mode = "wb";
                }

                auto pFile = fopen( fileInfo.absolutePath.c_str(), mode );
                if( !pFile )
                {
                    WP_LOG_ERROR( "Could not open file: " + fileInfo.absolutePath.str() );
                }

                if( pFile )
                {
                    fseek( pFile, 0, SEEK_END );
                    auto size = ftell( pFile );
                    rewind( pFile );

                    if( size < 0 )
                    {
                        WP_LOG_ERROR( "Could not determine file size: " + fileInfo.absolutePath.str() );
                        fclose( pFile );
                        return nullptr;
                    }

                    auto buffer = new c8[size];
                    auto readCount = fread( buffer, 1, static_cast<size_t>( size ), pFile );

                    fclose( pFile );

                    if( static_cast<long>( readCount ) != size )
                    {
                        WP_LOG_ERROR( "Incomplete read for file: " + fileInfo.absolutePath.str() );
                    }

                    return factoryManager->make_ptr<MemoryFile>( buffer, size,
                                                                 fileInfo.absolutePath.str(), true );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto FileSystemArchive::exists( const String &filePath, bool ignorePath, bool ignoreCase ) const
        -> bool
    {
        if( SafePtr<IFileList> fileList = getFileList() )
        {
            ScopedLoadLock loadLock( fileList.get() );

            auto &files = fileList->getFiles();
            ScopedLock lock( &files );

            if( ignorePath )
            {
                for( auto &file : files )
                {
                    if( archiveFileNameEqual( file.fileName, filePath, ignoreCase ) )
                    {
                        return true;
                    }
                }
            }
            else
            {
                if( archiveIsAbsolutePath( filePath ) )
                {
                    for( auto &file : files )
                    {
                        if( archivePathEqual( file.absolutePath, filePath.c_str(), filePath.size(),
                                              ignoreCase ) )
                        {
                            return true;
                        }
                    }
                }
                else
                {
                    for( auto &file : files )
                    {
                        if( archivePathEqual( file.filePath, filePath.c_str(), filePath.size(),
                                              ignoreCase ) )
                        {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    auto FileSystemArchive::isReadOnly() const -> bool
    {
        return false;
    }

    auto FileSystemArchive::getPath() const -> String
    {
        return m_path.load();
    }

    void FileSystemArchive::setPath( const String &path )
    {
        m_path = StringUtil::cleanupPath( path );
    }

    auto FileSystemArchive::findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath,
                                          bool ignoreCase ) const -> bool
    {
        if( auto fileList = getFileList() )
        {
            return fileList->findFileInfo( id, fileInfo, ignorePath, ignoreCase );
        }

        return false;
    }

    auto FileSystemArchive::findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath,
                                          bool ignoreCase ) const -> bool
    {
        if( auto fileList = getFileList() )
        {
            return fileList->findFileInfo( filePath, fileInfo, ignorePath, ignoreCase );
        }

        return false;
    }

    void FileSystemArchive::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto fileSystem = applicationManager->getFileSystem() )
        {
            fileSystem->lock();
        }
    }

    bool FileSystemArchive::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto fileSystem = applicationManager->getFileSystem() )
        {
            return fileSystem->try_lock();
        }

        return false;
    }

    void FileSystemArchive::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto fileSystem = applicationManager->getFileSystem() )
        {
            fileSystem->unlock();
        }
    }

    void FileSystemArchive::addFile( const FileInfo &file )
    {
        if( auto fileList = getFileList() )
        {
            fileList->addFile( file );
        }
    }

}  // namespace workphone
