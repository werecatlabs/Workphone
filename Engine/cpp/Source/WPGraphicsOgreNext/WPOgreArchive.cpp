#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/WPOgreArchive.hpp>
#include <WPGraphicsOgreNext/WPOgreDataStream.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{

    WPOgreArchive::WPOgreArchive( const Ogre::String &name, const Ogre::String &archType ) :
        Ogre::Archive( name, archType )
    {
    }

    WPOgreArchive::~WPOgreArchive() = default;

    void WPOgreArchive::load()
    {
    }

    void WPOgreArchive::unload()
    {
    }

    auto WPOgreArchive::open( const Ogre::String &filename, bool readOnly /*= true */ ) const
        -> Ogre::DataStreamPtr
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto fileSystem = applicationManager->getFileSystemPtr();

        auto isBinary = true;
        auto fileExt = Path::getFileExtension( filename.c_str() );
        if( fileExt == ".compositor" || fileExt == ".material" || fileExt == ".program" ||
            fileExt == ".fontdef" || fileExt == ".glsl" || fileExt == ".hlsl" )
        {
            isBinary = false;
        }

        if( fileExt == ".ttf" )
        {
            isBinary = true;
        }

        auto path = getName();
        auto filePath = path + "/" + filename;

        auto stream = fileSystem->open( filename.c_str(), true, isBinary, false, false );
        if( !stream )
        {
            stream = fileSystem->open( filePath.c_str(), true, isBinary, false, false );
        }

        if( stream )
        {
            return Ogre::DataStreamPtr( new WPOgreDataStream( stream ) );
        }

        return {};
    }

    auto WPOgreArchive::open( const Ogre::String &filename, bool readOnly ) -> Ogre::DataStreamPtr
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto isBinary = true;
        auto fileExt = Path::getFileExtension( filename.c_str() );
        if( fileExt == ".compositor" || fileExt == ".material" || fileExt == ".program" ||
            fileExt == ".fontdef" || fileExt == ".glsl" || fileExt == ".hlsl" )
        {
            isBinary = false;
        }

        if( fileExt == ".ttf" )
        {
            isBinary = true;
        }

        auto path = getName();

        auto filePath = String();

        if( !Path::isPathAbsolute( filename.c_str() ) )
        {
            if( !StringUtil::contains( filename.c_str(), path.c_str() ) )
            {
                filePath = Path::lexically_normal( path.c_str(), filename.c_str() );
            }
            else
            {
                filePath = filename;
            }
        }
        else
        {
            filePath = filename;
        }

        filePath = StringUtil::cleanupPath( filePath );

        auto stream = fileSystem->open( filePath, true, isBinary, false, false, false );
        if( !stream )
        {
            stream = fileSystem->open( filePath, true, isBinary, false, false, true );
        }

        if( !stream )
        {
            if( !Path::isPathAbsolute( filePath ) )
            {
                auto mediaPath = applicationManager->getMediaPath();
                auto absolutePath = Path::lexically_normal( mediaPath, filePath );
                stream = fileSystem->open( absolutePath, true, isBinary, false, false, true );
            }
            else
            {
                stream = fileSystem->open( filePath, true, isBinary, false, false, true );
            }
        }

        if( stream )
        {
            return Ogre::DataStreamPtr( new WPOgreDataStream( stream ) );
        }

        return {};
    }

    auto WPOgreArchive::list( bool recursive /*= true*/, bool dirs /*= false */ )
        -> Ogre::StringVectorPtr
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto path = getName();
        auto files = fileSystem->getFilesAsAbsolutePaths( path.c_str(), false );

        auto pStrVec = Ogre::StringVectorPtr( new Ogre::StringVector() );
        pStrVec->reserve( files.size() );
        for( auto &file : files )
        {
            pStrVec->push_back( StringUtil::str( file ) );
        }

        return pStrVec;
    }

    auto WPOgreArchive::listFileInfo( bool recursive /*= true*/, bool dirs /*= false */ )
        -> Ogre::FileInfoListPtr
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto path = getName();
        auto files = fileSystem->getFilesAsAbsolutePaths( path.c_str(), recursive );

        auto fileInfoList = new Ogre::FileInfoList();
        fileInfoList->reserve( files.size() );

        for( const auto &file : files )
        {
            // Check if it's a directory
            bool isDirectory = fileSystem->isExistingFolder( file );

            // Filter based on dirs parameter
            if( dirs != isDirectory )
            {
                continue;
            }

            // Skip if not recursive and file is in a subdirectory
            if( !recursive )
            {
                auto relativePath = file;
                if( StringUtil::contains( file, path.c_str() ) )
                {
                    relativePath = file.substr( path.length() );
                    if( !relativePath.empty() && ( relativePath[0] == '/' || relativePath[0] == '\\' ) )
                    {
                        relativePath = relativePath.substr( 1 );
                    }
                }

                // Check if file is in a subdirectory
                if( relativePath.find( '/' ) != String::npos ||
                    relativePath.find( '\\' ) != String::npos )
                {
                    continue;
                }
            }

            // Extract filename and basename
            auto filename = Path::getFileName( file );
            auto basename = filename;

            Ogre::FileInfo info;
            info.archive = this;
            info.filename = Ogre::String( filename.c_str(), filename.length() );
            info.basename = Ogre::String( basename.c_str(), basename.length() );

            auto filepath = Path::getFilePath( file );
            info.path = StringUtil::str( filepath );

            // Get file size
            if( !isDirectory )
            {
#ifdef _WIN32
                struct _stat64i32 tagStat;
                if( _stat( file.c_str(), &tagStat ) == 0 )
                {
                    info.compressedSize = tagStat.st_size;
                    info.uncompressedSize = tagStat.st_size;
                }
                else
                {
                    info.compressedSize = 0;
                    info.uncompressedSize = 0;
                }
#else
                struct stat tagStat;
                if( stat( file.c_str(), &tagStat ) == 0 )
                {
                    info.compressedSize = tagStat.st_size;
                    info.uncompressedSize = tagStat.st_size;
                }
                else
                {
                    info.compressedSize = 0;
                    info.uncompressedSize = 0;
                }
#endif
            }
            else
            {
                // Mark as directory (following Ogre convention)
                info.compressedSize = static_cast<size_t>( -1 );
                info.uncompressedSize = static_cast<size_t>( -1 );
            }

            fileInfoList->push_back( info );
        }

        return Ogre::FileInfoListPtr( fileInfoList );
    }

    auto WPOgreArchive::find( const Ogre::String &pattern, bool recursive /*= true*/,
                              bool dirs /*= false */ ) -> Ogre::StringVectorPtr
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto path = getName();
        auto files = fileSystem->getFilesAsAbsolutePaths( path.c_str(), recursive );

        auto stringVector = new Ogre::StringVector();
        stringVector->reserve( files.size() );

        // Determine if pattern contains directory separator for full path matching
        bool fullMatch = ( pattern.find( '/' ) != Ogre::String::npos ) ||
                         ( pattern.find( '\\' ) != Ogre::String::npos );
        bool wildCard = pattern.find( '*' ) != Ogre::String::npos;

        for( const auto &file : files )
        {
            // Check if it's a directory
            bool isDirectory = fileSystem->isExistingFolder( file );

            // Filter based on dirs parameter
            if( dirs != isDirectory )
            {
                continue;
            }

            // Skip if not recursive and file is in a subdirectory
            if( !recursive && !fullMatch && !wildCard )
            {
                auto relativePath = file;
                if( StringUtil::contains( file, path.c_str() ) )
                {
                    relativePath = file.substr( path.length() );
                    if( !relativePath.empty() && ( relativePath[0] == '/' || relativePath[0] == '\\' ) )
                    {
                        relativePath = relativePath.substr( 1 );
                    }
                }

                // Check if file is in a subdirectory
                if( relativePath.find( '/' ) != String::npos ||
                    relativePath.find( '\\' ) != String::npos )
                {
                    continue;
                }
            }

            // Extract filename and basename for matching
            auto filename = Path::getFileName( file );
            auto basename = filename;

            // Match pattern against filename or basename
            bool matches = false;
            if( fullMatch )
            {
                // Match against full path
                matches = StringUtil::match( file, pattern.c_str(), !isCaseSensitive() );
            }
            else
            {
                // Match against basename
                matches = StringUtil::match( basename, pattern.c_str(), !isCaseSensitive() );
            }

            if( matches )
            {
                stringVector->push_back( filename.c_str() );
            }
        }

        return Ogre::StringVectorPtr( stringVector );
    }

    auto WPOgreArchive::exists( const Ogre::String &filename ) -> bool
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager->isRunning() )
        {
            return false;
        }

        auto fileSystem = applicationManager->getFileSystem();
        if( !fileSystem )
        {
            return false;
        }

        auto folderName = getName();
        auto filePath = Path::lexically_normal( folderName.c_str(), filename.c_str() );

        auto fileExists = fileSystem->isExistingFile( filePath, false, false );
        if( !fileExists )
        {
            fileExists = fileSystem->isExistingFile( filePath, false, true );
        }

        //WP_ASSERT( fileExists == true );

        return fileExists;
    }

    auto WPOgreArchive::findFileInfo( const Ogre::String &pattern, bool recursive, bool dirs )
        -> Ogre::FileInfoListPtr
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto path = getName();
        auto files = fileSystem->getFilesAsAbsolutePaths( path.c_str(), recursive );

        auto fileInfoList = new Ogre::FileInfoList();
        fileInfoList->reserve( files.size() );

        // Determine if pattern contains directory separator for full path matching
        bool fullMatch = ( pattern.find( '/' ) != Ogre::String::npos ) ||
                         ( pattern.find( '\\' ) != Ogre::String::npos );
        bool wildCard = pattern.find( '*' ) != Ogre::String::npos;

        for( const auto &file : files )
        {
            // Check if it's a directory
            bool isDirectory = fileSystem->isExistingFolder( file );

            // Filter based on dirs parameter
            if( dirs != isDirectory )
            {
                continue;
            }

            // Skip if not recursive and file is in a subdirectory
            if( !recursive && !fullMatch && !wildCard )
            {
                auto relativePath = file;
                if( StringUtil::contains( file, path.c_str() ) )
                {
                    relativePath = file.substr( path.length() );
                    if( !relativePath.empty() && ( relativePath[0] == '/' || relativePath[0] == '\\' ) )
                    {
                        relativePath = relativePath.substr( 1 );
                    }
                }

                // Check if file is in a subdirectory
                if( relativePath.find( '/' ) != String::npos ||
                    relativePath.find( '\\' ) != String::npos )
                {
                    continue;
                }
            }

            // Extract filename and basename for matching
            auto filename = Path::getFileName( file );
            auto basename = filename;

            // Match pattern against filename or basename
            bool matches = false;
            if( fullMatch )
            {
                // Match against full path
                matches = StringUtil::match( file, pattern.c_str(), !isCaseSensitive() );
            }
            else
            {
                // Match against basename
                matches = StringUtil::match( basename, pattern.c_str(), !isCaseSensitive() );
            }

            if( matches )
            {
                Ogre::FileInfo info;
                info.archive = this;
                info.filename = Ogre::String( filename.c_str(), filename.length() );
                info.basename = Ogre::String( basename.c_str(), basename.length() );

                auto filepath = Path::getFilePath( file );
                info.path = StringUtil::str( filepath );

                // Get file size
                if( !isDirectory )
                {
#ifdef _WIN32
                    struct _stat64i32 tagStat;
                    if( _stat( file.c_str(), &tagStat ) == 0 )
                    {
                        info.compressedSize = tagStat.st_size;
                        info.uncompressedSize = tagStat.st_size;
                    }
                    else
                    {
                        info.compressedSize = 0;
                        info.uncompressedSize = 0;
                    }
#else
                    struct stat tagStat;
                    if( stat( file.c_str(), &tagStat ) == 0 )
                    {
                        info.compressedSize = tagStat.st_size;
                        info.uncompressedSize = tagStat.st_size;
                    }
                    else
                    {
                        info.compressedSize = 0;
                        info.uncompressedSize = 0;
                    }
#endif
                }
                else
                {
                    // Mark as directory (following Ogre convention)
                    info.compressedSize = static_cast<size_t>( -1 );
                    info.uncompressedSize = static_cast<size_t>( -1 );
                }

                fileInfoList->push_back( info );
            }
        }

        return Ogre::FileInfoListPtr( fileInfoList );
    }

    auto WPOgreArchive::getModifiedTime( const Ogre::String &filename ) -> time_t
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto folderName = getName();
        auto filePath = Path::lexically_normal( folderName.c_str(), filename.c_str() );

        // Try to get file info using standard stat functions
        // First attempt: check without fallback
#ifdef _WIN32
        struct _stat64i32 tagStat;
        if( _stat( filePath.c_str(), &tagStat ) == 0 )
        {
            return tagStat.st_mtime;
        }
#else
        struct stat tagStat;
        if( stat( filePath.c_str(), &tagStat ) == 0 )
        {
            return tagStat.st_mtime;
        }
#endif

        // If file doesn't exist or couldn't get stats, return 0
        return 0;
    }

    auto WPOgreArchive::isCaseSensitive() const -> bool
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WINRT
        return false;  // Windows filesystems are typically case-insensitive
#else
        return true;  // Unix-like filesystems are typically case-sensitive
#endif
    }

    void WPOgreArchive::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( graphicsSystem )
        {
            graphicsSystem->lock();
        }
    }

    void WPOgreArchive::lock_shared()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( graphicsSystem )
        {
            graphicsSystem->lock_shared();
        }
    }

    void WPOgreArchive::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( graphicsSystem )
        {
            graphicsSystem->unlock();
        }
    }

    void WPOgreArchive::unlock_shared()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( graphicsSystem )
        {
            graphicsSystem->unlock_shared();
        }
    }

}  // namespace workphone
