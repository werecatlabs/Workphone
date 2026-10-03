#ifndef OgreMemoryStream_h__
#define OgreMemoryStream_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <OgreDataStream.h>

namespace workphone
{

    class MemoryStream : public Ogre::DataStream
    {
    public:
        MemoryStream( void *pMem, size_t size, bool freeOnClose = false, bool readOnly = false );
        ~MemoryStream();

        /** Get a pointer to the start of the memory block this stream holds. */
        u8 *getPtr( void );

        /** Get a pointer to the current position in the memory block this stream holds. */
        u8 *getCurrentPtr( void );

        /** @copydoc DataStream::read
         */
        size_t read( void *buf, size_t count );

        /** @copydoc DataStream::write
         */
        size_t write( const void *buf, size_t count );

        /** @copydoc DataStream::readLine
         */
        size_t readLine( char *buf, size_t maxCount, const Ogre::String &delim = "\n" );

        /** @copydoc DataStream::skipLine
         */
        size_t skipLine( const Ogre::String &delim = "\n" );

        /** @copydoc DataStream::skip
         */
        void skip( long count );

        /** @copydoc DataStream::seek
         */
        void seek( size_t pos );

        /** @copydoc DataStream::tell
         */
        size_t tell( void ) const;

        /** @copydoc DataStream::eof
         */
        bool eof( void ) const;

        /** @copydoc DataStream::close
         */
        void close( void );

    protected:
        /// Pointer to the start of the data area
        u8 *mData;
        /// Pointer to the current position in the memory
        u8 *mPos;
        /// Pointer to the end of the memory
        u8 *mEnd;
    };

}  // namespace workphone

#endif  // OgreMemoryStream_h__
