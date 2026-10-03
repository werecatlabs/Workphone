#ifndef WPOgreDataStream_h__
#define WPOgreDataStream_h__

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <OgreDataStream.h>

namespace workphone
{

    /**
     * @brief Adapter that wraps a workphone::IStream and exposes the
     *        Ogre::DataStream interface.
     *
     * This class allows code that expects an Ogre::DataStream to read from
     * a generic workphone::IStream implementation. All operations are
     * forwarded to the underlying stream object.
     */
    class WPOgreDataStream : public Ogre::DataStream
    {
    public:
        /**
         * @brief Default-construct an empty data stream adapter.
         *
         * The resulting instance will not have an underlying stream until
         * `setStream` is called.
         */
        WPOgreDataStream();

        /**
         * @brief Construct adapter from an existing stream.
         * @param stream Smart pointer to the underlying IStream to wrap.
         */
        WPOgreDataStream( SmartPtr<IStream> stream );

        /**
         * @brief Virtual destructor.
         */
        ~WPOgreDataStream() override;

        /**
         * @brief Read up to `count` bytes into `buf` from the underlying stream.
         * @param buf Destination buffer to receive data.
         * @param count Maximum number of bytes to read.
         * @return Number of bytes actually read. 0 indicates end-of-stream.
         */
        size_t read( void *buf, size_t count ) override;

        /**
         * @brief Read a single line from the stream.
         * @param trimAfter If true, trim trailing newline characters from the returned string.
         * @return The line read as an Ogre::String. If end-of-stream is reached an empty string is returned.
         */
        Ogre::String getLine( bool trimAfter = true ) override;

        /**
         * @brief Skip forward (or backward if count is negative) by `count` bytes.
         * @param count Number of bytes to skip.
         */
        void skip( long count ) override;

        /**
         * @brief Seek to an absolute position in the stream.
         * @param pos Absolute position (in bytes) from the beginning of the stream.
         */
        void seek( size_t pos ) override;

        /**
         * @brief Get the current read position within the stream.
         * @return Current position in bytes from the beginning of the stream.
         */
        size_t tell( void ) const override;

        /**
         * @brief Check whether the end of the stream has been reached.
         * @return True if no more data is available; false otherwise.
         */
        bool eof( void ) const override;

        /**
         * @brief Close the underlying stream and release resources.
         */
        void close( void ) override;

        /**
         * @brief Get the wrapped IStream instance.
         * @return Smart pointer to the underlying IStream.
         */
        SmartPtr<IStream> getStream() const;

        /**
         * @brief Replace the wrapped IStream instance.
         * @param stream New smart pointer to the IStream to wrap.
         */
        void setStream( SmartPtr<IStream> stream );

    protected:
        /**
         * @brief Underlying stream that provides the actual I/O operations.
         */
        SmartPtr<IStream> m_stream;
    };

}  // namespace workphone

#endif  // WPOgreDataStream_h__
