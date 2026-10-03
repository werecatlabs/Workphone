#ifndef _WP_IStream_h__
#define _WP_IStream_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/FileInfo.hpp>

namespace workphone
{

    /**
     * @brief Interface for reading and writing data streams.
     *
     * The IStream interface provides a unified abstraction for data input and output operations.
     * It supports both binary and text-based data streams, with capabilities for reading, writing,
     * seeking, and querying stream properties. This interface is designed to work with various
     * data sources including files, memory buffers, and network streams.
     *
     * @note All implementations must be thread-safe if used in multi-threaded environments.
     * @note Streams should be properly closed when no longer needed to free associated resources.
     *
     * @see ISharedObject
     * @see FileInfo
     * @see String
     */
    class WPCore_API IStream : public ISharedObject
    {
    public:
        /**
         * @brief Enumeration for specifying the access mode of the stream.
         *
         * Defines the allowed operations that can be performed on the stream.
         * These values can be combined using bitwise OR for streams that support
         * both reading and writing.
         */
        enum class AccessMode
        {
            Read = 1, /**< The stream is open for reading operations. */
            Write = 2 /**< The stream is open for writing operations. */
        };

        IStream();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of stream resources. Derived classes should
         * override this to perform any necessary cleanup operations.
         */
        ~IStream() override;

        /**
         * @brief Checks if the stream is currently open and available for operations.
         *
         * @return @c true if the stream is open and ready for I/O operations, @c false otherwise.
         *
         * @note A stream must be opened before any read or write operations can be performed.
         */
        virtual bool isOpen() const = 0;

        /**
         * @brief Closes the stream and releases associated resources.
         *
         * After calling this method, the stream is no longer available for I/O operations.
         * Any subsequent calls to read, write, or other stream operations will result
         * in undefined behavior or errors.
         *
         * @note It is safe to call this method multiple times on the same stream.
         */
        virtual void close() = 0;

        /**
         * @brief Checks if the stream has reached the end of available data.
         *
         * @return @c true if the stream position is at or beyond the end of available data,
         *         @c false if there is more data available to read.
         *
         * @note This method is primarily useful for read operations. For write-only streams,
         *       this method may always return @c false or have undefined behavior.
         */
        virtual bool eof() const = 0;

        /**
         * @brief Reads a specified number of bytes from the stream.
         *
         * Reads up to @p sizeToRead bytes from the current position in the stream
         * and stores them in the provided buffer. The actual number of bytes read
         * may be less than requested if the end of stream is reached.
         *
         * @param[out] buffer Pointer to the buffer where read bytes will be stored.
         *                    Must be large enough to hold at least @p sizeToRead bytes.
         * @param[in] sizeToRead The maximum number of bytes to read from the stream.
         *
         * @return The actual number of bytes read from the stream.
         *
         * @note The buffer must be valid and writable for the entire operation.
         * @note If the stream is not open or not readable, the behavior is undefined.
         * @note The stream position is advanced by the number of bytes actually read.
         */
        virtual size_Num read( void *buffer, size_Num sizeToRead ) = 0;

        /**
         * @brief Writes a specified number of bytes to the stream.
         *
         * Writes @p count bytes from the provided buffer to the current position
         * in the stream. The actual number of bytes written may be less than
         * requested if the stream cannot accept all the data.
         *
         * @param[in] buf Pointer to the buffer containing the bytes to write.
         *                Must contain at least @p count valid bytes.
         * @param[in] count The number of bytes to write to the stream.
         *
         * @return The actual number of bytes written to the stream.
         *
         * @note This method is only applicable to writable streams.
         * @note If the stream is not open or not writable, the behavior is undefined.
         * @note The stream position is advanced by the number of bytes actually written.
         */
        virtual size_Num write( const void *buf, size_Num count ) = 0;

        /**
         * @brief Reads a single line from the stream.
         *
         * Reads characters from the stream until a delimiter is encountered or
         * the maximum count is reached. The delimiter character is not included
         * in the returned data and is skipped over, so the next read operation
         * will occur after the delimiter.
         *
         * @param[out] buf Pointer to the buffer where the line data will be stored.
         *                 Must be large enough to hold at least @p maxCount + 1 characters
         *                 (including the null terminator).
         * @param[in] maxCount The maximum number of characters to read, excluding
         *                     the null terminator.
         * @param[in] delim The delimiter character(s) to stop reading at.
         *                  Defaults to newline ("\\n").
         *
         * @return The number of characters read, excluding the delimiter and null terminator.
         *
         * @note The buffer will be null-terminated regardless of whether the delimiter
         *       was encountered or the maximum count was reached.
         * @note For consistent results, the stream should be opened in binary mode.
         * @note If the stream is not open or not readable, the behavior is undefined.
         */
        virtual size_Num readLine( c8 *buf, size_Num maxCount, const String &delim = "\n" ) = 0;

        /**
         * @brief Retrieves the next line of data as a String object.
         *
         * This is a convenience method for text streams that reads the next line
         * of data and returns it as a String object. The line is read up to the
         * next newline character, and optionally trimmed of whitespace.
         *
         * @param[in] trimAfter If @c true, the line is trimmed of leading and trailing
         *                      whitespace using String::trim(true, true).
         *
         * @return A String containing the next line of data, or an empty string
         *         if the end of stream is reached.
         *
         * @note For consistent results, the stream should be opened in binary mode.
         * @note This method is primarily intended for text-based streams.
         * @note If the stream is not open or not readable, the behavior is undefined.
         */
        virtual String getLine( bool trimAfter = true ) = 0;

        /**
         * @brief Changes the current position in the stream.
         *
         * Moves the stream position to the specified location. The new position
         * is specified as an absolute byte offset from the beginning of the stream.
         *
         * @param[in] finalPos The destination position in the stream, specified as
         *                     a byte offset from the beginning (0-based).
         *
         * @return @c true if the seek operation was successful, @c false otherwise.
         *
         * @note Seeking beyond the end of the stream may be allowed for some stream types
         *       (e.g., file streams), but the behavior is implementation-dependent.
         * @note If the stream is not open, the behavior is undefined.
         * @note For write operations, seeking may truncate the stream at the new position.
         */
        virtual bool seek( size_Num finalPos ) = 0;

        /**
         * @brief Gets the total size of the stream in bytes.
         *
         * @return The total size of the stream in bytes, or 0 if the size cannot
         *         be determined or the stream is not open.
         *
         * @note For some stream types (e.g., network streams), the size may not
         *       be known in advance and this method may return 0.
         * @note The size may change during the lifetime of the stream for writable streams.
         */
        virtual size_Num size() const = 0;

        /**
         * @brief Gets the current position in the stream.
         *
         * @return The current position in the stream as a byte offset from the beginning (0-based).
         *
         * @note If the stream is not open, the behavior is undefined.
         * @note The position is always a non-negative value.
         */
        virtual size_Num tell() const = 0;

        /**
         * @brief Gets the name of the file associated with this stream.
         *
         * @return The file name as a String, or an empty string if the stream
         *         is not associated with a file or the name is not available.
         *
         * @note This method is primarily useful for file-based streams.
         * @note For memory streams or network streams, this may return an empty string
         *       or a descriptive identifier.
         */
        virtual String getFileName() const = 0;

        /**
         * @brief Retrieves the entire stream content as a String.
         *
         * This is a convenience method that reads all available data from the stream
         * and returns it as a String object. The stream position is moved to the end
         * after reading.
         *
         * @return A String containing all the data from the stream, or an empty string
         *         if the stream is empty or not readable.
         *
         * @note This method is primarily intended for text-based streams.
         * @note For large streams, this method may consume significant memory.
         * @note If the stream is not open or not readable, the behavior is undefined.
         * @note The stream position will be at the end after this operation.
         */
        virtual String getAsString() = 0;

        /**
         * @brief Sets whether memory should be freed when the object is destroyed.
         *
         * Controls the memory management behavior of the stream object. When set to
         * @c true, any internal buffers or resources will be automatically freed
         * when the object is destroyed.
         *
         * @param[in] freeMemory If @c true, memory will be freed on destruction;
         *                       if @c false, memory management is left to the caller.
         *
         * @note This setting only affects the stream object itself, not any external
         *       resources it may reference.
         * @note The default behavior is implementation-dependent.
         */
        virtual void setFreeMemory( bool freeMemory ) = 0;

        /**
         * @brief Skips over the next line in the stream.
         *
         * Advances the stream position past the next occurrence of the specified
         * delimiter, effectively skipping the current line of data.
         *
         * @param[in] delim The delimiter character(s) to skip to. Defaults to newline ("\\n").
         *
         * @return The number of characters skipped, including the delimiter.
         *
         * @note If the end of stream is reached before finding the delimiter,
         *       all remaining characters are skipped.
         * @note If the stream is not open or not readable, the behavior is undefined.
         */
        virtual size_Num skipLine( const String &delim = "\n" ) = 0;

        /**
         * @brief Skips a specified number of bytes in the stream.
         *
         * Advances or rewinds the stream position by the specified number of bytes.
         * Positive values advance the position forward, negative values rewind it backward.
         *
         * @param[in] count The number of bytes to skip. Positive values move forward,
         *                  negative values move backward.
         *
         * @note Attempting to skip beyond the beginning of the stream may result
         *       in the position being set to 0.
         * @note Attempting to skip beyond the end of the stream may result in the
         *       position being set to the end of the stream.
         * @note If the stream is not open, the behavior is undefined.
         */
        virtual void skip( size_Num count ) = 0;

        /**
         * @brief Checks if the stream supports read operations.
         *
         * @return @c true if the stream is readable, @c false otherwise.
         *
         * @note A stream may be readable even if it is not currently open.
         * @note This method indicates capability, not current state.
         */
        virtual bool isReadable() const = 0;

        /**
         * @brief Checks if the stream supports write operations.
         *
         * @return @c true if the stream is writable, @c false otherwise.
         *
         * @note A stream may be writable even if it is not currently open.
         * @note This method indicates capability, not current state.
         */
        virtual bool isWriteable() const = 0;

        /**
         * @brief Gets the file information associated with this stream.
         *
         * @return A FileInfo object containing metadata about the file associated
         *         with this stream, or an empty FileInfo if not applicable.
         *
         * @note This method is primarily useful for file-based streams.
         * @see FileInfo
         */
        virtual FileInfo getFileInfo() const = 0;

        /**
         * @brief Sets the file information for this stream.
         *
         * @param[in] fileInfo The FileInfo object containing metadata to associate
         *                     with this stream.
         *
         * @note This method is primarily useful for file-based streams.
         * @note Setting file information does not necessarily change the underlying
         *       file or stream data.
         * @see FileInfo
         */
        virtual void setFileInfo( const FileInfo &fileInfo ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // _WP_IStream_h__
