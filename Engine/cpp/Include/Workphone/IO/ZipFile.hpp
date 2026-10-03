#ifndef ZipFile_h__
#define ZipFile_h__

#include <Workphone/IO/DataStream.hpp>

namespace workphone
{
    /**
     * @brief Represents a file stored inside a ZIP archive and exposes it
     *        through the DataStream interface.
     *
     * This class wraps a ZZIP_FILE handle and provides read/seek/tell
     * semantics compatible with the engine's DataStream API. The
     * ZipFile instance does not take ownership of the parent ZipArchive by
     * default; the archive pointer is stored to allow coordinated
     * operations (for example, to keep the archive alive while the file is
     * in use).
     */
    class WPCore_API ZipFile : public DataStream
    {
    public:
        /**
         * @brief Default-construct an empty ZipFile (not open).
         */
        ZipFile();

        /**
         * @brief Construct a ZipFile wrapping an already-open ZZIP_FILE.
         *
         * @param archive Pointer to the parent ZipArchive (may be nullptr).
         * @param fileName The (virtual) path of the file inside the archive.
         * @param zzipFile The raw ZZIP_FILE handle returned by zziplib.
         * @param uncompressedSize Size of the file after decompression in bytes.
         */
        ZipFile( ZipArchive *archive, const String &fileName, ZZIP_FILE *zzipFile,
                 u32 uncompressedSize );

        /**
         * @brief Destructor. Closes the underlying ZZIP_FILE if still open.
         */
        ~ZipFile() override;

        /**
         * @brief Load object data for this stream (part of ISharedObject).
         *
         * Implementation-specific; typically used by the engine resource
         * management. The parameter is a shared object that may be used to
         * restore state after serialization.
         *
         * @param data Context object used during load.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload object data for this stream (part of ISharedObject).
         *
         * @param data Context object used during unload.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Returns true if the underlying ZZIP_FILE handle is valid and
         *        the file is open for I/O.
         */
        bool isOpen() const override;

        /**
         * @brief Close the underlying ZZIP_FILE. After close() the stream is
         *        no longer readable.
         */
        void close() override;

        /**
         * @brief Return true when the stream position is at end-of-file.
         */
        bool eof( void ) const override;

        /**
         * @brief Read up to `sizeToRead` bytes from the file into `buffer`.
         *
         * @param buffer Destination buffer.
         * @param sizeToRead Number of bytes to attempt to read.
         * @return The number of bytes actually read.
         */
        size_Num read( void *buffer, size_Num sizeToRead ) override;

        /**
         * @brief Writing is not supported for ZIP entries; this returns 0 or
         *        may assert depending on implementation.
         *
         * @param buffer Source buffer.
         * @param sizeToWrite Number of bytes to write.
         * @return Number of bytes written (normally 0 for read-only ZipFile).
         */
        size_Num write( const void *buffer, size_Num sizeToWrite ) override;

        /**
         * @brief Seek to an absolute position inside the uncompressed file.
         *
         * @param finalPos Absolute position from the beginning of the file.
         * @return true on success, false on failure.
         */
        bool seek( size_Num finalPos ) override;

        /**
         * @brief Advance the current position by `count` bytes.
         *
         * This is a convenience over `seek(tell() + count)`.
         *
         * @param count Number of bytes to skip forward.
         */
        void skip( size_Num count ) override;

        /**
         * @brief Return the uncompressed size of this file in bytes.
         */
        size_Num size() const override;

        /**
         * @brief Return the current read position (offset) in bytes.
         */
        size_Num tell() const override;

        /**
         * @brief Control whether the underlying memory should be released when
         *        the stream is closed.
         *
         * @param freeMemory If true, release any internal buffers when
         *        closing; otherwise keep them.
         */
        void setFreeMemory( bool freeMemory ) override;

        /**
         * @brief Get the raw ZZIP_FILE pointer used by this ZipFile.
         *
         * @return Raw pointer to ZZIP_FILE or nullptr if none.
         */
        ZZIP_FILE *getZipFile() const;

        /**
         * @brief Set the raw ZZIP_FILE handle. The caller is responsible for
         *        ensuring the handle is valid.
         */
        void setZipFile( ZZIP_FILE *zipFile );

        /**
         * @brief Get the parent ZipArchive associated with this entry.
         */
        ZipArchive *getArchive() const;

        /**
         * @brief Associate this ZipFile with a parent ZipArchive.
         *
         * @param archive Pointer to the ZipArchive to associate with.
         */
        void setArchive( ZipArchive *archive );

        /**
         * @brief Set the cached uncompressed size for this entry.
         *
         * @param iSize Size in bytes.
         */
        void setSize( size_Num iSize );

        /**
         * @brief Returns true if this stream can be read from.
         */
        bool isReadable() const override;

        /**
         * @brief Returns true if this stream can be written to (usually false
         *        for zip entries).
         */
        bool isWriteable() const override;

        /**
         * @brief Returns true if the ZipFile object is in a usable state
         *        (has a valid underlying file handle).
         */
        bool isValid() const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        ZZIP_FILE *m_zipFile = nullptr;
        ZipArchive *m_archive = nullptr;
    };
}  // namespace workphone

#endif  // ObfuscatedZipFile_h__
