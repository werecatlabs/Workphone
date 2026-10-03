#ifndef ObfuscatedZipFile_h__
#define ObfuscatedZipFile_h__

#include <Workphone/IO/DataStream.hpp>

namespace workphone
{
    /**
     * @brief Data stream that provides read access to a file stored inside an
     * obfuscated ZIP archive.
     *
     * `ObfuscatedZipFile` implements the `DataStream` interface for a single
     * file entry extracted from an `ObfuscatedZipArchive`. The class wraps the
     * underlying `ZZIP_FILE *` handle and provides read, seek and query
     * operations using the same semantics as other `DataStream` implementations.
     *
     * The object does not duplicate the whole file contents in memory unless
     * an implementation of `DataStream::load` requests it; instead it forwards
     * streaming operations to the `zzip` file API. The life-time relationship
     * with the owning `ObfuscatedZipArchive` is documented on the
     * constructor.
     */
    class WPCore_API ObfuscatedZipFile : public DataStream
    {
    public:
        /**
         * @brief Create an empty, closed `ObfuscatedZipFile` instance.
         *
         * The stream will be invalid until a proper constructor is used to
         * associate it with an archive and file entry.
         */
        ObfuscatedZipFile();

        /**
         * @brief Construct a stream for a single file entry inside an archive.
         *
         * @param archive Pointer to the archive that contains the entry. The
         *        archive pointer is referenced for the duration of the file
         *        object and is not copied; ownership stays with the caller.
         * @param fileName The original filename/path of the entry inside the
         *        archive (for diagnostic/logging purposes).
         * @param zzipFile The underlying `ZZIP_FILE *` handle returned by
         *        the zzip library. This object takes responsibility for using
         *        and closing the handle when the stream is closed or
         *        destroyed.
         * @param uncompressedSize The known uncompressed size of the entry in
         *        bytes. Implementations may use this value for `size()`.
         */
        ObfuscatedZipFile( ObfuscatedZipArchive *archive, const String &fileName, ZZIP_FILE *zzipFile,
                           u32 uncompressedSize );

        /**
         * @brief Destructor. Closes the underlying file handle and releases
         * any resources held by this stream.
         */
        ~ObfuscatedZipFile() override;

        /** @copydoc DataStream::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc DataStream::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        bool isOpen() const override;

        void close() override;

        bool eof( void ) const override;

        size_Num read( void *buffer, size_Num sizeToRead ) override;

        size_Num write( const void *buffer, size_Num sizeToWrite ) override;

        bool seek( size_Num finalPos ) override;

        void skip( size_Num count ) override;

        size_Num size() const override;

        size_Num tell() const override;

        void setFreeMemory( bool freeMemory ) override;

        bool isReadable() const override;

        bool isWriteable() const override;

        bool isValid() const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        ZZIP_FILE *m_zipFile = nullptr;
        ObfuscatedZipArchive *m_archive = nullptr;
    };
}  // namespace workphone

#endif  // ObfuscatedZipFile_h__
