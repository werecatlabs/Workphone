#ifndef IArchive_h__
#define IArchive_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/FileInfo.hpp>

namespace workphone
{

    /**
     * @class IArchive
     * @brief Abstract interface representing a file archive.
     *
     * IArchive provides a uniform interface to inspect and open files contained
     * in an archive implementation (for example: a folder on disk, a zip file,
     * a virtual package, etc.). Implementations expose file discovery, file
     * metadata retrieval and stream-based access to archived file contents.
     *
     * Implementations should document any thread-safety, lifetime and ownership
     * guarantees for returned objects (for example SmartPtr<IStream>).
     */
    class WPCore_API IArchive : public ISharedObject
    {
    public:
        IArchive();

        /** @brief Virtual destructor. */
        ~IArchive() override;

        /**
         * @brief Open a file inside the archive and return a stream to it.
         *
         * Requests a stream for the file identified by @p filePath. The returned
         * stream should provide read and/or write access based on the @p input
         * and @p truncate parameters and follow the archive's semantics for
         * binary/text mode.
         *
         * @param filePath Path of the file inside the archive. The interpretation
         *        of the path (case sensitivity, separators, root) depends on the
         *        archive implementation.
         * @param input If true, open for reading; if false, open for writing.
         * @param binary If true, open in binary mode; otherwise treat as text.
         * @param truncate If true and opening for writing, truncate the file to
         *        zero length on open. Semantics when opening for read-only
         *        archive implementations are implementation-defined.
         * @param ignorePath If true the archive may ignore directory components
         *        of @p filePath and resolve by filename only (implementation-specific).
         * @return SmartPtr<IStream> A managed pointer to the opened stream. Returns
         *         null (empty SmartPtr) if the file could not be opened.
         */
        virtual SmartPtr<IStream> open( const String &filePath, bool input = true, bool binary = false,
                                        bool truncate = false, bool ignorePath = false,
                                        bool ignoreCase = false ) = 0;

        /**
         * @brief Check whether a file exists in the archive.
         *
         * @param filePath The path of the file to check.
         * @param ignorePath If true, the archive may perform filename-only checks.
         * @param ignoreCase If true, perform a case-insensitive existence check
         *        when the archive's underlying storage supports it.
         * @return bool True if the file exists in the archive; otherwise false.
         */
        virtual bool exists( const String &filePath, bool ignorePath = false,
                             bool ignoreCase = false ) const = 0;

        /**
         * @brief Get a file list helper for this archive.
         *
         * Returns an object that can be used to enumerate or query the file list
         * more efficiently than repeatedly calling @c exists or @c getFiles.
         * The returned object may be null if the archive does not support a
         * separate file-listing helper.
         *
         * @return SmartPtr<IFileList> Managed pointer to the file list helper or
         *         null if not available.
         */
        virtual SmartPtr<IFileList> getFileList() const = 0;

        /**
         * @brief Retrieve metadata for all files contained in the archive.
         *
         * The returned array contains FileInfo entries describing files (and/or
         * directories) found in the archive. The exact contents, ordering and
         * whether directories are included is archive-specific.
         *
         * @return Array<FileInfo> Array of FileInfo structures describing the archive's contents.
         */
        virtual Array<FileInfo> getFiles() const = 0;

        /**
         * @brief Get the archive type identifier.
         *
         * The type is an implementation-defined 8-bit identifier describing the
         * archive backing (for example: folder, zip, virtual package). Consumers
         * should not rely on specific numeric values unless documented by the
         * concrete archive implementation.
         *
         * @return u8 Archive type identifier.
         */
        virtual u8 getType() const = 0;

        /**
         * @brief Query whether the archive is read-only.
         *
         * If true, attempts to open files for writing or to modify contents should
         * fail. If false, the archive supports updates (subject to other
         * implementation-specific constraints such as passwords or locks).
         *
         * @return bool True when the archive is read-only.
         */
        virtual bool isReadOnly() const = 0;

        /**
         * @brief Get the optional password associated with the archive.
         *
         * Some archive implementations support encrypted contents and may expose
         * an associated password. This accessor returns the password currently
         * configured for use with this archive instance (may be empty).
         *
         * @return String Password string or empty string if none set.
         */
        virtual String getPassword() const = 0;

        /**
         * @brief Set an optional password for the archive.
         *
         * Use this to supply credentials required to access encrypted archive
         * entries. Behaviour when setting a password on archives that do not
         * support encryption is implementation-defined.
         *
         * @param password Password to use for encrypted entries. An empty string
         *        clears any previously set password.
         */
        virtual void setPassword( const String &password ) = 0;

        /**
         * @brief Get the archive path.
         *
         * For a folder-based archive this will typically be the folder path on disk;
         * for other archive types it may be the file path or a logical identifier.
         *
         * @return String The archive path or identifier.
         */
        virtual String getPath() const = 0;

        /**
         * @brief Set the archive path or identifier.
         * This sets the underlying resource path for the archive instance.
         * @param path Path or identifier to associate with this archive.
         */
        virtual void setPath( const String &path ) = 0;

        /**
         * @brief Find FileInfo by UUID.
         *
         * Searches the archive for a file record matching @p id and, if found,
         * writes the metadata into @p fileInfo.
         *
         * @param id The unique identifier of the file to find.
         * @param fileInfo Output parameter that receives the file metadata if found.
         * @param ignorePath If true, path components may be ignored when matching.
         * @param ignoreCase If true, perform a case-insensitive search when matching.
         * @return bool True if a matching FileInfo was found and written to @p fileInfo.
         */
        virtual bool findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath = false,
                                   bool ignoreCase = false ) const = 0;

        /**
         * @brief Find FileInfo by file path.
         *
         * Searches the archive for a file record matching @p filePath and, if found,
         * writes the metadata into @p fileInfo.
         *
         * @param filePath The path of the file to find inside the archive.
         * @param fileInfo Output parameter that receives the file metadata if found.
         * @param ignorePath If true, path components may be ignored when matching.
         * @param ignoreCase If true, perform a case-insensitive search when matching.
         * @return bool True if a matching FileInfo was found and written to @p fileInfo.
         */
        virtual bool findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath = false,
                                   bool ignoreCase = false ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IArchive_h__
