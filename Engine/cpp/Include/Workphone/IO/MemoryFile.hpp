#ifndef __CMemoryFile_h__
#define __CMemoryFile_h__

#include <Workphone/IO/DataStream.hpp>

namespace workphone
{
    /**
     * @brief A DataStream implementation that operates on a memory buffer.
     *
     * This class provides read/write/seek operations over an in-memory buffer
     * and implements the DataStream interface. It can either take ownership
     * of the provided buffer (and free it when destroyed) or reference an
     * externally managed buffer without freeing it.
     */
    class WPCore_API MemoryFile : public DataStream
    {
    public:
        /**
         * @brief Construct an empty, closed MemoryFile.
         *
         * The stream created by this constructor has no associated buffer and
         * reports size() == 0 and isOpen() == false until setBuffer() is
         * called.
         */
        MemoryFile();

        /**
         * @brief Create a MemoryFile that wraps an existing buffer.
         *
         * @param memory Pointer to the memory buffer to use.
         * @param len Length of the buffer in bytes.
         * @param fileName Optional logical name for the stream (used for
         *                 debugging or virtual file systems).
         * @param deleteMemoryWhenDropped If true, the buffer will be freed
         *                                 when this object is destroyed.
         */
        MemoryFile( void *memory, long len, const String &fileName, bool deleteMemoryWhenDropped );

        /**
         * @brief Destructor. Releases owned memory if ownership was requested.
         */
        ~MemoryFile() override;

        /**
         * @brief Returns true if the stream has an associated buffer.
         *
         * A MemoryFile is considered open when it has a valid buffer pointer
         * (set either via constructor or setBuffer()).
         */
        bool isOpen() const override;

        /**
         * @brief Close the stream.
         *
         * Closing a MemoryFile clears the buffer pointers and frees the
         * memory if ownership was requested.
         */
        void close() override;

        /**
         * @brief Returns true when the current position is at or beyond the end
         *        of the buffer.
         */
        bool eof( void ) const override;

        /**
         * @brief Read up to sizeToRead bytes into the provided buffer.
         * @return The number of bytes actually read.
         */
        size_Num read( void *buffer, size_Num sizeToRead ) override;

        /**
         * @brief Write data into the memory buffer at the current position.
         * @return The number of bytes actually written (may be less than
         *         requested if the buffer has insufficient remaining space).
         */
        size_Num write( const void *buffer, size_Num sizeToWrite ) override;

        /**
         * @brief Seek to an absolute position inside the buffer.
         * @param finalPos Absolute position from the start of the buffer.
         * @return True if the position was valid and the seek succeeded.
         */
        bool seek( size_Num finalPos ) override;

        /**
         * @brief Return the total size of the underlying buffer in bytes.
         */
        size_Num size() const override;

        /**
         * @brief Get the current read/write position inside the buffer.
         */
        size_Num tell() const override;

        /**
         * @brief Get a const pointer to the buffer interpreted as characters.
         * @return Pointer to the buffer as a C string (may not be null-terminated).
         */
        const c8 *getCharPtr() const;

        /**
         * @brief Get a void pointer to the underlying buffer.
         */
        void *getData() const;

        /**
         * @brief Alias for getData(). Returns the raw buffer pointer.
         */
        void *getBuffer() const;

        /**
         * @brief Replace the current buffer without changing ownership flags.
         *
         * This does not free the previous buffer; caller must manage that.
         */
        void setBuffer( void *buffer );

        /**
         * @brief Whether the MemoryFile will free the buffer when destroyed.
         */
        bool getFreeMemory() const;

        /**
         * @brief Set whether the MemoryFile owns the buffer and should free it.
         */
        void setFreeMemory( bool freeMemory ) override;

        /**
         * @brief Advance the current position to the end of the next line.
         * @param delim Line delimiter to search for (default "\n").
         * @return Number of bytes skipped.
         */
        size_Num skipLine( const String &delim = "\n" ) override;

        /**
         * @brief Move the current position forward by count bytes.
         *
         * If the skip would go past the end of the buffer the position is set
         * to the end.
         */
        void skip( size_Num count ) override;

        WP_CLASS_REGISTER_DECL;

    private:
        u8 *m_buffer = nullptr;
        u8 *m_end = nullptr;
        u8 *m_position = nullptr;
        size_Num m_size = 0;
        bool m_freeMemory = false;
    };
}  // namespace workphone

#endif
