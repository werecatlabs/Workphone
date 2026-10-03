#ifndef ObfuscatedZipReader_h__
#define ObfuscatedZipReader_h__

#include <Workphone/Interface/IO/IArchive.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    /** \brief Archive implementation that reads from an obfuscated/packed ZIP file.
     *
     * This class wraps access to a zip archive using zziplib and provides
     * Workphone-specific behaviors such as optional path- and case-insensitive
     * lookups and support for an archive password. The archive keeps an
     * in-memory index of contained files because zziplib only supports a
     * single directory scan.
     */
    class WPCore_API ObfuscatedZipArchive : public IArchive
    {
    public:
        /** \brief Default constructor.
         *
         * Constructs an empty archive object. The archive will remain closed
         * until \c load is called with a valid data object.
         */
        ObfuscatedZipArchive();

        /** \brief Construct and open an archive.
         *
         * @param name Path to the zip file to open.
         * @param ignoreCase When true, file lookups are case-insensitive.
         * @param ignorePaths When true, file lookups ignore directory paths
         *                    (only the file name is considered).
         */
        ObfuscatedZipArchive( const String &name, bool ignoreCase, bool ignorePaths );

        /** \brief Destructor.
         *
         * Ensures the archive is unloaded and any held resources are released.
         */
        ~ObfuscatedZipArchive() override;

        /** \copydoc IArchive::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** \brief Reload archive data.
         *
         * Reinitializes the archive using the provided data object. This is
         * typically used to update the archive contents without destroying the
         * archive instance.
         */
        void reload( SmartPtr<ISharedObject> data ) override;

        /** \copydoc IArchive::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** \copydoc IArchive::getFileList */
        SmartPtr<IFileList> getFileList() const override;

        /** \copydoc IArchive::getFiles */
        Array<FileInfo> getFiles() const override;

        /** \copydoc IArchive::getType */
        u8 getType() const override;

        /** \brief Get the password used to open encrypted entries.
         *
         * @return The archive password as a string. Empty if none set.
         */
        String getPassword() const override;

        /** \brief Set the password to be used for encrypted entries.
         *
         * @param password Password string to use when opening encrypted files.
         */
        void setPassword( const String &password ) override;

        /** \brief Open a file from the archive.
         *
         * @param filename Name or path of the file inside the archive.
         * @param input If true, opens for reading; otherwise for writing.
         * @param binary When true, opens in binary mode.
         * @param truncate If true and opening for write, truncates existing file.
         * @param ignorePath If true, ignore the stored directory path when
         *                   searching for the file.
         * @param ignoreCase If true, perform case-insensitive lookup for the file.
         * @return A smart pointer to an IStream if the file was found and opened,
         *         or an empty pointer on failure.
         */
        SmartPtr<IStream> open( const String &filename, bool input = true, bool binary = false,
                                bool truncate = false, bool ignorePath = false,
                                bool ignoreCase = false ) override;

        /** \brief Check whether a file exists in the archive.
         *
         * @param filename Name or path to check.
         * @param ignorePath If true, match only on the file name portion.
         * @param ignoreCase If true, perform a case-insensitive match.
         * @return True if the file exists in the archive, false otherwise.
         */
        bool exists( const String &filename, bool ignorePath = false,
                     bool ignoreCase = false ) const override;

        /** \copydoc IArchive::isReadOnly */
        bool isReadOnly() const override;

        /** \brief Get the path to the underlying zip file.
         *
         * @return The path used when the archive was opened.
         */
        String getPath() const override;

        /** \brief Set the path of the underlying archive file.
         *
         * This updates the internal path string; it does not automatically
         * open or reload the archive. Use \c load or \c reload to apply.
         *
         * @param path New archive file path.
         */
        void setPath( const String &path ) override;

        /** \brief Query whether path information is ignored for lookups.
         *
         * When true, lookups compare only file names and ignore directory
         * components.
         *
         * @return True if paths are ignored, false otherwise.
         */
        bool getIgnorePaths() const;

        /** \brief Enable or disable ignoring paths during lookups.
         *
         * @param ignorePaths True to ignore directory paths when searching.
         */
        void setIgnorePaths( bool ignorePaths );

        /** \brief Find file information by identifier.
         *
         * @param id GUID/UUID of the file to find.
         * @param[out] fileInfo Output parameter that receives the file metadata
         *                if the file is found.
         * @param ignorePath If true, match only on file name portion.
         * @return True if the file was found, false otherwise.
         */
        bool findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath = false,
                           bool ignoreCase = false ) const override;

        /** \brief Find file information by path or name.
         *
         * @param filePath Path or name of the file to search for.
         * @param[out] fileInfo Receives file metadata when found.
         * @param ignorePath If true, only the file name portion is considered.
         * @return True when file is found, false otherwise.
         */
        bool findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath = false,
                           bool ignoreCase = false ) const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** \brief Convert a zziplib error code to a human-readable string.
         *
         * @param zzipError Integer error code returned by zziplib functions.
         * @return A descriptive error string suitable for logging or diagnostics.
         */
        String getZzipErrorDescription( s32 zzipError );

        //! Mutex protecting archive state for concurrent access.
        RecursiveMutex m_mutex;

        //! Handle to the opened zziplib directory (root of the archive).
        ZZIP_DIR *mZzipDir = nullptr;

        //! Cached list of files inside the archive. zziplib requires a single
        //! directory scan, so we keep an in-memory index for further queries.
        Array<FileInfo> m_fileInfoList;

        //! FileList wrapper built from m_fileInfoList for API compatibility.
        SmartPtr<FileList> m_fileList;

        //! Password used to open encrypted entries (empty if not set).
        FixedString<128> m_password;

        //! Stored path string for the underlying archive file.
        FixedString<WP_MAX_PATH> m_path;
    };
}  // namespace workphone

#endif  // ObfuscatedZipReader_h__
