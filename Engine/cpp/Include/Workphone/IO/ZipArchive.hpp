#ifndef ZipArchive_h__
#define ZipArchive_h__

#include <Workphone/Interface/IO/IArchive.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/FileInfo.hpp>

namespace workphone
{
    /**
     * @brief Implements an archive backed by a ZIP container.
     *
     * `ZipArchive` provides read (and limited write) access to files stored inside
     * a ZIP archive. It implements the `IArchive` interface so it can be used
     * interchangeably with other archive implementations (for example a
     * filesystem-based archive).
     *
     * The class maintains an internal handle to the underlying ZIP directory
     * (mZzipDir) and a file list representation (m_fileList) to enumerate
     * entries. Password support is available through `setPassword` and
     * `getPassword` for encrypted ZIP entries. The archive may be configured
     * to ignore path components or case when opening files.
     */
    class WPCore_API ZipArchive : public IArchive
    {
    public:
        ZipArchive();
        ZipArchive( const String &name, bool ignoreCase, bool ignorePaths );
        ~ZipArchive() override;

        /**
         * @copydoc IArchive::load
         *
         * Initialize the archive from the provided shared object. The
         * shared object typically represents an opened file or memory buffer
         * containing the ZIP data.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IArchive::unload
         *
         * Reload the archive data from the provided shared object. This will
         * refresh the internal handle and file list to reflect any changes.
         */
        void reload( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IArchive::unload
         *
         * Release any resources held by the archive. After calling unload the
         * archive becomes unusable until `load` or `reload` is called again.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IArchive::getFileList
         *
         * Returns a file list object that can be used to iterate entries in
         * the ZIP archive. The returned pointer may be null when the archive
         * is not loaded.
         */
        SmartPtr<IFileList> getFileList() const override;

        /**
         * @copydoc IArchive::getFiles
         *
         * Returns a copy of the internal file table as an array of
         * FileInfo structures. Useful for consumers that need file metadata
         * without holding the archive open.
         */
        Array<FileInfo> getFiles() const override;

        /**
         * @copydoc IArchive::getType
         *
         * Returns the archive type identifier. Implementations use this to
         * differentiate between archive kinds (zip, filesystem, etc.).
         */
        u8 getType() const override;

        /**
         * Get the password used to open encrypted entries in the archive.
         * Returns an empty string when no password is set.
         */
        String getPassword() const override;

        /**
         * Set a password that should be used when opening encrypted files
         * inside the ZIP archive.
         */
        void setPassword( const String &password ) override;

        /**
         * Open an entry inside the archive and return an IStream for reading
         * or writing.
         *
         * Parameters mirror the IArchive::open contract. `ignorePath` and
         * `ignoreCase` can be used to control filename matching behavior.
         */
        SmartPtr<IStream> open( const String &filename, bool input = true, bool binary = false,
                                bool truncate = false, bool ignorePath = false,
                                bool ignoreCase = false ) override;

        /**
         * Check whether an entry with `filename` exists inside the archive.
         * When `ignorePath` or `ignoreCase` are true, matching will be
         * performed accordingly.
         */
        bool exists( const String &filename, bool ignorePath = false,
                     bool ignoreCase = false ) const override;

        /**
         * @copydoc IArchive::isReadOnly
         *
         * Returns true when the archive does not support modifying entries.
         */
        bool isReadOnly() const override;

        /**
         * Get the path that identifies the archive (usually the file path to
         * the ZIP container).
         */
        String getPath() const override;

        /**
         * Set the identifying path of the archive. This does not open or
         * close the archive by itself; it only adjusts the stored path string.
         */
        void setPath( const String &path ) override;

        /**
         * Returns whether the archive is configured to ignore internal path
         * components when resolving filenames (only the final name component
         * will be matched).
         */
        bool getIgnorePaths() const;

        /**
         * Configure whether path components inside archive entries should be
         * ignored during lookup operations.
         */
        void setIgnorePaths( bool ignorePaths );

        /**
         * @copydoc IArchive::findFileInfo
         *
         * Find file metadata by UUID. Returns true when an entry matching
         * the id was found and populates `fileInfo` with the metadata.
         */
        bool findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath = false,
                           bool ignoreCase = false ) const override;

        /**
         * @copydoc IArchive::findFileInfo
         *
         * Find file metadata by archive-relative file path. Returns true when
         * the entry exists and populates `fileInfo` with its metadata.
         */
        bool findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath = false,
                           bool ignoreCase = false ) const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * Translate a zziplib error code to a human readable description.
         *
         * @param zzipError Error code returned by zziplib calls.
         * @return Localized description string for logging or diagnostics.
         */
        String getZzipErrorDescription( s32 zzipError );

        /// Handle to the root zip directory returned by zziplib. Null when
        /// the archive is not loaded.
        ZZIP_DIR *m_zzipDir = nullptr;

        /// Cached file list representing entries inside the ZIP container.
        SmartPtr<FileList> m_fileList;

        /// Optional password used to decrypt encrypted ZIP entries.
        FixedString<128> m_password;

        /// The identifying path for the archive (usually the ZIP file path).
        FixedString<WP_MAX_PATH> m_path;

        /// When true, path components inside archive entries are ignored when
        /// matching filenames.
        bool m_ignorePaths = false;
    };
}  // namespace workphone

#endif  // ZipArchive_h__
