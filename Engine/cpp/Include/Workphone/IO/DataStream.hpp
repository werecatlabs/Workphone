#ifndef DataStream_h__
#define DataStream_h__

#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @brief Base class for data streams used by the engine.
     *
     * `DataStream` provides a common interface and helper implementations for
     * reading and writing raw bytes and text from a data source. Concrete
     * subclasses implement the low-level `read` and `close` operations (and
     * optionally override others). The class also stores basic file metadata
     * such as the logical name, size and access mode.
     */
    class WPCore_API DataStream : public IStream
    {
    public:
        /**
         * @brief Create an unnamed data stream.
         *
         * @param accessMode Bitmask of `AccessMode` flags indicating allowed
         *                   operations (default: `AccessMode::Read`).
         */
        DataStream( u16 accessMode = static_cast<u16>( AccessMode::Read ) );

        /**
         * @brief Create a named data stream.
         *
         * The provided `name` is a logical identifier for the stream (for
         * example a resource name or file path) and is not required to match
         * any on-disk file path.
         *
         * @param name Logical name for the stream.
         * @param accessMode Bitmask of `AccessMode` flags indicating allowed
         *                   operations (default: `AccessMode::Read`).
         */
        DataStream( const String &name, u16 accessMode = static_cast<u16>( AccessMode::Read ) );

        /**
         * @brief Virtual destructor.
         */
        ~DataStream() override;

        /**
         * @brief Get the (possibly mutable) file/name associated with the stream.
         *
         * This returns a copy of the stored logical name. Use the const
         * overload below when you have a const reference to a stream.
         *
         * @return The stream's logical name.
         */
        String getFileName();

        /**
         * @brief Set the logical name for the stream.
         *
         * @param fileName New logical name (e.g. resource identifier).
         */
        void setFileName( const String &fileName );

        /**
         * @brief Returns the access mode bitmask for the stream.
         *
         * The value corresponds to the `AccessMode` enum bits and indicates
         * whether the stream is readable, writeable, or both.
         *
         * @return Access mode bitmask.
         */
        u16 getAccessMode() const;

        /**
         * @brief Returns true if the stream was opened with read access.
         *
         * @return true if readable; false otherwise.
         */
        bool isReadable() const override;

        /**
         * @brief Returns true if the stream was opened with write access.
         *
         * @return true if writeable; false otherwise.
         */
        bool isWriteable() const override;

        // Streaming operators
        /**
         * @brief Deserialize a value of type `T` from the stream.
         *
         * This operator should be implemented by concrete stream types or
         * rely on `read` to extract the required number of bytes for `T`.
         */
        template <typename T>
        DataStream &operator>>( T &value );

        /**
         * @brief Read raw bytes from the stream.
         *
         * Must be implemented by subclasses.
         *
         * @param buf Destination buffer.
         * @param count Number of bytes to read.
         * @return Number of bytes actually read.
         */
        size_Num read( void *buf, size_Num count ) override = 0;

        /**
         * @brief Write raw bytes to the stream.
         *
         * The default implementation may forward to a subclass-provided
         * mechanism; override if necessary.
         *
         * @param buf Source buffer.
         * @param count Number of bytes to write.
         * @return Number of bytes actually written.
         */
        size_Num write( const void *buf, size_Num count ) override;

        /**
         * @brief Read a line of text into the provided buffer.
         *
         * Reads up to `maxCount` characters (including terminating null if
         * implemented by subclasses) or until any character from `delim` is
         * encountered.
         *
         * @param buf Destination buffer for the line.
         * @param maxCount Maximum number of characters to read.
         * @param delim Delimiter characters (default: "\n").
         * @return Number of bytes read.
         */
        size_Num readLine( char *buf, size_Num maxCount, const String &delim = "\n" ) override;

        /**
         * @brief Read and return a line as a `String`.
         *
         * If `trimAfter` is true the delimiter(s) are removed from the
         * returned string.
         *
         * @param trimAfter If true, trim the delimiter from the returned line.
         * @return The read line as a `String`.
         */
        String getLine( bool trimAfter = true ) override;

        /**
         * @brief Read the entire remaining stream into a `String`.
         *
         * This is mainly intended for text streams; binary streams should be
         * handled via `read`/`write` and sized buffers.
         *
         * @return Contents of the stream as a `String`.
         */
        String getAsString() override;

        /**
         * @brief Skip a single line in the stream.
         *
         * @warning For predictable results this function should be used when
         * the stream is opened in binary mode; text-mode translations may
         * alter newline representations.
         *
         * @param delim Delimiter characters that terminate the line (default: "\n").
         * @return Number of bytes skipped.
         */
        size_Num skipLine( const String &delim = "\n" ) override;

        /**
         * @brief Return the total size of the stream data when known.
         *
         * Some stream types (e.g. network streams or dynamic producers) may
         * not provide a deterministic size; in that case 0 is returned.
         *
         * @return Size of the stream in bytes, or 0 if unknown.
         */
        size_Num size() const override;

        /**
         * @brief Close the stream and release any associated resources.
         *
         * After calling `close`, further operations on the stream are
         * invalid. Subclasses must implement this method.
         */
        void close() override = 0;

        /**
         * @brief Query whether the stream is currently open.
         *
         * @return True if open; false otherwise.
         */
        bool isOpen() const override;

        /**
         * @brief Get the (const) logical name of the stream.
         *
         * Same as the non-const overload but callable on const instances.
         *
         * @return The stream's logical name.
         */
        String getFileName() const override;

        /**
         * @brief Control whether the stream implementation should free
         *        internally held memory on close.
         *
         * @param freeMemory If true, free internal buffers when closing.
         */
        void setFreeMemory( bool freeMemory ) override;

        /**
         * @brief Retrieve file metadata associated with this stream.
         *
         * This may include size, timestamps or other platform-specific
         * information when available.
         *
         * @return `FileInfo` structure describing the stream.
         */
        FileInfo getFileInfo() const override;

        /**
         * @brief Set file metadata for this stream.
         *
         * @param fileInfo Metadata to associate with the stream.
         */
        void setFileInfo( const FileInfo &fileInfo ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        // File metadata held for the stream (size/timestamps/etc.)
        FileInfo m_fileInfo;

        /// Cached size of the stream's data in bytes (0 if unknown)
        size_Num m_size = 0;

        /// Bitmask representing the allowed access mode (AccessMode enum)
        u16 m_access = 0;
    };
}  // namespace workphone

#endif  // DataStream_h__
