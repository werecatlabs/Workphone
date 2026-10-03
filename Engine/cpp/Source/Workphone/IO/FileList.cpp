#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/FileList.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Memory/PointerUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, FileList, IFileList );

    const String FileList::emptyFileListEntry;

    FileList::FileList() = default;

    FileList::FileList( const String &path ) : m_path( path )
    {
        m_path = StringUtil::cleanupPath( path );
    }

    FileList::~FileList() = default;

    void FileList::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_files.reserve( 1024 );
        setLoadingState( LoadingState::Loaded );
    }

    void FileList::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        m_files.clear();
        setLoadingState( LoadingState::Unloaded );
    }

    auto FileList::getNumFiles() const -> u32
    {
        return static_cast<u32>( m_files.size() );
    }

    void FileList::sort()
    {
        ScopedLock lock( &m_files );
        std::sort( m_files.begin(), m_files.end() );
    }

    auto FileList::getFileName( u32 index ) const -> String
    {
        if( index >= m_files.size() )
        {
            return emptyFileListEntry;
        }

        return m_files[index].fileName;
    }

    //! Gets the full name of a file in the list, String included, based on an index.
    auto FileList::getFullFileName( u32 index ) const -> String
    {
        if( index >= m_files.size() )
        {
            return emptyFileListEntry;
        }

        auto &filePath = m_files[index].filePath;
        return String( filePath.c_str(), filePath.length() );
    }

    auto FileList::getFileSize( u32 index ) const -> u32
    {
        auto size = m_files.size();

        if( index < size )
        {
            return static_cast<u32>( index < size ? m_files[index].uncompressedSize : 0 );
        }

        return 0;
    }

    auto FileList::getFileOffset( u32 index ) const -> u32
    {
        auto size = m_files.size();

        if( index < size )
        {
            return ( index < size ? m_files[index].offset : 0 );
        }

        return 0;
    }

    auto FileList::findFile( const String &filename, bool isDirectory ) const -> s32
    {
        ScopedLock lock( &m_files );
        for( auto &file : m_files )
        {
            if( file.isDirectory == isDirectory &&
                ( StringUtil::isEqual( file.fileName, filename ) ||
                  StringUtil::isEqual( file.fileNameLowerCase, filename ) ) )
            {
                return static_cast<s32>( &file - &m_files[0] );
            }
        }

        return -1;
    }

    auto FileList::getPath() const -> String
    {
        return m_path.load();
    }

    auto FileList::getFiles() -> ConcurrentArray<FileInfo> &
    {
        return m_files;
    }

    auto FileList::getFiles() const -> const ConcurrentArray<FileInfo> &
    {
        return m_files;
    }

    void FileList::setFiles( const Array<FileInfo> &files )
    {
        ScopedLock lock( &m_files );
        m_files = { files.begin(), files.end() };
    }

    void FileList::setPath( const String &path )
    {
        m_path = path;
    }

    void FileList::addFile( const FileInfo &file )
    {
        ScopedLock lock( &m_files );
        m_files.push_back( file );
    }

    void FileList::removeFile( const FileInfo &file )
    {
        ScopedLock lock( &m_files );
        m_files.erase( std::remove_if( m_files.begin(), m_files.end(),
                                       [&file]( const FileInfo &f ) { return f == file; } ),
                       m_files.end() );
    }

    auto FileList::findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath, bool ignoreCase ) const
        -> bool
    {
        ScopedLock lock( &m_files );

        if( m_files.empty() )
        {
            return false;
        }

        for( auto &file : m_files )
        {
            if( file.fileId == id )
            {
                fileInfo = file;
                return true;
            }
        }

        const auto &filePath = fileInfo.filePath;
        if( filePath.empty() )
        {
            return false;
        }

        String filePathLower;
        String fileName;
        String fileNameLower;
        bool isAbsolutePath = Path::isPathAbsolute( filePath );

        if( ignoreCase )
        {
            filePathLower = StringUtil::make_lower( filePath );
        }

        if( ignorePath )
        {
            fileName = Path::getFileName( filePath );
            if( ignoreCase )
            {
                fileNameLower = StringUtil::make_lower( fileName );
            }
        }

        for( auto &file : m_files )
        {
            if( ignorePath )
            {
                if( ignoreCase )
                {
                    if( StringUtil::isEqual( file.fileNameLowerCase, fileNameLower ) )
                    {
                        fileInfo = file;
                        return true;
                    }
                }
                else
                {
                    if( StringUtil::isEqual( file.fileName, fileName ) )
                    {
                        fileInfo = file;
                        return true;
                    }
                }
            }
            else
            {
                if( ignoreCase )
                {
                    if( StringUtil::isEqual( file.filePathLowerCase, filePathLower ) )
                    {
                        fileInfo = file;
                        return true;
                    }
                }
                else
                {
                    if( StringUtil::isEqual( file.filePath, filePath ) )
                    {
                        fileInfo = file;
                        return true;
                    }
                }

                if( isAbsolutePath && StringUtil::isEqual( file.absolutePath, filePath ) )
                {
                    fileInfo = file;
                    return true;
                }
            }
        }

        return false;
    }

    auto FileList::findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath,
                                 bool ignoreCase ) const -> bool
    {
        ScopedLock lock( &m_files );

        if( m_files.empty() || filePath.empty() )
        {
            return false;
        }

        auto filePathHash = StringUtil::getUUID( filePath );

        String filePathLower;
        String fileName;
        String fileNameLower;
        bool isAbsolutePath = Path::isPathAbsolute( filePath );

        if( ignoreCase )
        {
            filePathLower = StringUtil::make_lower( filePath );
        }

        if( ignorePath )
        {
            fileName = Path::getFileName( filePath );
            if( ignoreCase )
            {
                fileNameLower = StringUtil::make_lower( fileName );
            }
        }

        auto files = m_files.snapshot();
        for( auto &file : files )
        {
            if( file.fileId == filePathHash )
            {
                fileInfo = file;
                return true;
            }

            if( ignorePath )
            {
                if( ignoreCase )
                {
                    if( StringUtil::isEqual( file.fileNameLowerCase, fileNameLower ) )
                    {
                        fileInfo = file;
                        return true;
                    }
                }
                else
                {
                    if( StringUtil::isEqual( file.fileName, fileName ) )
                    {
                        fileInfo = file;
                        return true;
                    }
                }
            }
            else
            {
                if( ignoreCase )
                {
                    if( StringUtil::isEqual( file.filePathLowerCase, filePathLower ) )
                    {
                        fileInfo = file;
                        return true;
                    }
                }
                else
                {
                    if( StringUtil::isEqual( file.filePath, filePath ) )
                    {
                        fileInfo = file;
                        return true;
                    }
                }

                if( isAbsolutePath && StringUtil::isEqual( file.absolutePath, filePath ) )
                {
                    fileInfo = file;
                    return true;
                }
            }
        }

        return false;
    }

    auto FileList::exists( const String &filePath, bool ignorePath, bool ignoreCase ) const -> bool
    {
        ScopedLock lock( &m_files );

        if( ignorePath )
        {
            auto fileName = Path::getFileName( filePath );
            auto fileNameLower = StringUtil::make_lower( fileName );
            for( auto &file : m_files )
            {
                if( ignoreCase )
                {
                    if( StringUtil::isEqual( file.fileNameLowerCase, fileNameLower ) )
                    {
                        return true;
                    }
                }
                else
                {
                    if( StringUtil::isEqual( file.fileName, fileName ) )
                    {
                        return true;
                    }
                }
            }
        }
        else
        {
            auto filePathLower = StringUtil::make_lower( filePath );
            for( auto &file : m_files )
            {
                if( ignoreCase )
                {
                    if( StringUtil::isEqual( file.filePathLowerCase, filePathLower ) )
                    {
                        return true;
                    }
                }
                else
                {
                    if( StringUtil::isEqual( file.filePath, filePath ) )
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

}  // namespace workphone
