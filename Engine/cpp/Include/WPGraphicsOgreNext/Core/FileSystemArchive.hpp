#ifndef FileSystemArchive_h__
#define FileSystemArchive_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <OgreArchive.h>

namespace workphone
{
    namespace render
    {

        class FileSystemArchive : public Ogre::Archive
        {
        public:
            FileSystemArchive( const String &name );
            ~FileSystemArchive();

            bool isCaseSensitive() const override;

            void load() override;

            void unload() override;

            Ogre::DataStreamPtr open( const Ogre::String &filename, bool readOnly = true ) override;

            Ogre::StringVectorPtr list( bool recursive = true, bool dirs = false ) const;

            Ogre::StringVectorPtr list( bool recursive = true, bool dirs = false ) override;

            Ogre::FileInfoListPtr listFileInfo( bool recursive = true, bool dirs = false ) const;

            Ogre::FileInfoListPtr listFileInfo( bool recursive = true, bool dirs = false ) override;

            Ogre::StringVectorPtr find( const Ogre::String &pattern, bool recursive = true,
                                        bool dirs = false ) override;

            bool exists( const Ogre::String &filename ) override;

            time_t getModifiedTime( const String &filename );

            Ogre::FileInfoListPtr findFileInfo( const Ogre::String &pattern, bool recursive = true,
                                                bool dirs = false ) override;
        };

    }  // namespace render
}  // namespace workphone

#endif  // FileSystemArchive_h__
