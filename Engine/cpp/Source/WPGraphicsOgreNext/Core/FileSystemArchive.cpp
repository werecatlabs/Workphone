#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Core/FileSystemArchive.hpp>
#include "WPOgreDataStream.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::render
{

    FileSystemArchive::FileSystemArchive( const String &name ) :
        Ogre::Archive( name.c_str(), "filesystem" )
    {
    }

    FileSystemArchive::~FileSystemArchive() = default;

    auto FileSystemArchive::isCaseSensitive() const -> bool
    {
        return false;
    }

    void FileSystemArchive::load()
    {
    }

    void FileSystemArchive::unload()
    {
    }

    auto FileSystemArchive::open( const Ogre::String &name, bool readOnly /*= true */ )
        -> Ogre::DataStreamPtr
    {
        auto isBinary = true;
        auto fileExt = Path::getFileExtension( name.c_str() );
        if( fileExt == ".compositor" || fileExt == ".material" || fileExt == ".program" ||
            fileExt == ".fontdef" || fileExt == ".gsls" || fileExt == ".hlsl" )
        {
            isBinary = false;
        }

        if( fileExt == ".ttf" )
        {
            isBinary = true;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        WP_ASSERT( !StringUtil::isNullOrEmpty( name.c_str() ) );

        auto stream = fileSystem->open( name.c_str(), true, isBinary, false, false );
        if( !stream )
        {
            stream = fileSystem->open( name.c_str(), true, isBinary, false, true );
        }

        if( stream )
        {
            return Ogre::DataStreamPtr( new WPOgreDataStream( stream ) );
        }

        return {};
    }

    auto FileSystemArchive::list( bool recursive /*= true*/, bool dirs /*= false */ ) const
        -> Ogre::StringVectorPtr
    {
        return {};
    }

    auto FileSystemArchive::list( bool recursive /*= true*/, bool dirs /*= false */ )
        -> Ogre::StringVectorPtr
    {
        return {};
    }

    auto FileSystemArchive::listFileInfo( bool recursive /*= true*/, bool dirs /*= false */ ) const
        -> Ogre::FileInfoListPtr
    {
        return {};
    }

    auto FileSystemArchive::listFileInfo( bool recursive /*= true*/, bool dirs /*= false */ )
        -> Ogre::FileInfoListPtr
    {
        return {};
    }

    auto FileSystemArchive::find( const Ogre::String &pattern, bool recursive /*= true*/,
                                  bool dirs /*= false */ ) -> Ogre::StringVectorPtr
    {
        return {};
    }

    auto FileSystemArchive::exists( const Ogre::String &name ) -> bool
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        WP_ASSERT( !StringUtil::isNullOrEmpty( name.c_str() ) );
        auto existing = fileSystem->isExistingFile( name.c_str(), false, false );
        if( !existing )
        {
            existing = fileSystem->isExistingFile( name.c_str(), true, false );
            if( !existing )
            {
                existing = fileSystem->isExistingFile( name.c_str(), true, true );
            }
        }

        return existing;
    }

    auto FileSystemArchive::getModifiedTime( const String &filename ) -> time_t
    {
        return 0;
    }

    auto FileSystemArchive::findFileInfo( const Ogre::String &pattern, bool recursive /*= true*/,
                                          bool dirs /*= false */ ) -> Ogre::FileInfoListPtr
    {
        return {};
    }

}  // namespace workphone::render
