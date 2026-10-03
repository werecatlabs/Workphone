#ifndef FileStreamDataStream_h__
#define FileStreamDataStream_h__

#include <Workphone/IO/DataStream.hpp>
#include <istream>

namespace workphone
{
    /**
     * @class FileDataStream
     * @brief A data stream implementation for file-based I/O operations using STL streams.
     *
     * This class provides a wrapper around STL file streams (std::ifstream and std::fstream)
     * to enable file reading and writing through the DataStream interface. It supports both
     * read-only and read-write operations, and can optionally manage the lifetime of the
     * underlying stream object.
     *
     * @note The class can operate in two modes:
     *       - Read-only mode: Using std::ifstream
     *       - Read-write mode: Using std::fstream
     */
    class WPCore_API FileDataStream : public DataStream
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Constructs an empty FileDataStream with no associated file stream.
         */
        FileDataStream();

        /**
         * @brief Construct a read-only stream from an STL input file stream.
         *
         * @param s Pointer to an std::ifstream object to wrap
         * @param freeOnClose If true, the stream will be deleted when close() is called or the object is
         * destroyed
         */
        FileDataStream( std::ifstream *s, bool freeOnClose = true );

        /**
         * @brief Construct a read-write stream from an STL file stream.
         *
         * @param s Pointer to an std::fstream object to wrap
         * @param freeOnClose If true, the stream will be deleted when close() is called or the object is
         * destroyed
         */
        FileDataStream( std::fstream *s, bool freeOnClose = true );

        /**
         * @brief Construct a named read-only stream from an STL input file stream.
         *
         * @param name Descriptive name for the stream
         * @param s Pointer to an std::ifstream object to wrap
         * @param freeOnClose If true, the stream will be deleted when close() is called or the object is
         * destroyed
         */
        FileDataStream( const String &name, std::ifstream *s, bool freeOnClose = true );

        /**
         * @brief Construct a named read-write stream from an STL file stream.
         *
         * @param name Descriptive name for the stream
         * @param s Pointer to an std::fstream object to wrap
         * @param freeOnClose If true, the stream will be deleted when close() is called or the object is
         * destroyed
         */
        FileDataStream( const String &name, std::fstream *s, bool freeOnClose = true );

        /**
         * @brief Construct a named read-only stream with explicit size from an STL input file stream.
         *
         * @param name Descriptive name for the stream
         * @param s Pointer to an std::ifstream object to wrap
         * @param size Size of the stream in bytes
         * @param freeOnClose If true, the stream will be deleted when close() is called or the object is
         * destroyed
         */
        FileDataStream( const String &name, std::ifstream *s, size_Num size, bool freeOnClose = true );

        /**
         * @brief Construct a named read-write stream with explicit size from an STL file stream.
         *
         * @param name Descriptive name for the stream
         * @param s Pointer to an std::fstream object to wrap
         * @param size Size of the stream in bytes
         * @param freeOnClose If true, the stream will be deleted when close() is called or the object is
         * destroyed
         */
        FileDataStream( const String &name, std::fstream *s, size_Num size, bool freeOnClose = true );

        /**
         * @brief Destructor.
         *
         * Closes the stream and frees the underlying file stream if freeOnClose was set to true.
         */
        ~FileDataStream() override;

        /** @copydoc DataStream::read */
        size_Num read( void *buf, size_Num count ) override;

        /** @copydoc DataStream::write */
        size_Num write( const void *buf, size_Num count ) override;

        /** @copydoc DataStream::getLine */
        String getLine( bool trimAfter = true ) override;

        /** @copydoc DataStream::readLine */
        size_Num readLine( c8 *buf, size_Num maxCount, const String &delim = "\n" ) override;

        /** @copydoc DataStream::skip */
        void skip( size_Num count ) override;

        /** @copydoc DataStream::seek */
        bool seek( size_Num finalPos ) override;

        /** @copydoc DataStream::tell */
        size_Num tell() const override;

        /** @copydoc DataStream::eof */
        bool eof() const override;

        /** @copydoc DataStream::close */
        void close() override;

        /** @copydoc IStream::isOpen */
        bool isOpen() const override;

        /** @copydoc IStream::isValid */
        bool isValid() const override;

        /**
         * @brief Get the underlying input stream pointer.
         * @return Pointer to the base std::istream, or nullptr if not set
         */
        std::istream *getInStream() const;

        /**
         * @brief Set the underlying input stream.
         * @param stream Pointer to an std::istream to use for reading operations
         */
        void setInStream( std::istream *stream );

        /**
         * @brief Get the underlying read-only file stream pointer.
         * @return Pointer to the std::ifstream, or nullptr if not in read-only mode
         */
        std::ifstream *getFStreamRO() const;

        /**
         * @brief Set the underlying read-only file stream.
         * @param stream Pointer to an std::ifstream to use for read-only operations
         */
        void setFStreamRO( std::ifstream *stream );

        /**
         * @brief Get the underlying read-write file stream pointer.
         * @return Pointer to the std::fstream, or nullptr if not in read-write mode
         */
        std::fstream *getFStream() const;

        /**
         * @brief Set the underlying read-write file stream.
         * @param stream Pointer to an std::fstream to use for read-write operations
         */
        void setFStream( std::fstream *stream );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Determines the access mode based on the currently set stream.
         *
         * This method analyzes the active stream pointer(s) and sets the appropriate
         * access flags (read-only or read-write) for the DataStream base class.
         */
        void determineAccess();

        /// Pointer to the base input stream used for read operations
        std::istream *m_inStream = nullptr;

        /// Pointer to the read-only file stream (std::ifstream) when in read-only mode
        std::ifstream *m_fstreamRO = nullptr;

        /// Pointer to the read-write file stream (std::fstream) when in read-write mode
        std::fstream *m_fstream = nullptr;

        /// Flag indicating whether the stream should be deleted when closed or destroyed
        bool m_freeOnClose = true;
    };
}  // namespace workphone

#endif  // FileStreamDataStream_h__
