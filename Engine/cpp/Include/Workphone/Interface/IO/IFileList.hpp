#ifndef IFileList_h__
#define IFileList_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief Interface representing a collection of files and folders.
     *
     * IFileList provides an abstract representation of a directory or archive
     * listing. Implementations expose file metadata (names, sizes, offsets, etc.)
     * and provide convenience operations such as searching, sorting and direct
     * access to the underlying file info container.
     *
     * Note: the interface itself does not mandate thread-safety. Implementations
     * that expose a shared ConcurrentArray (via `getFilesPtr`) may provide
     * thread-safe access to the underlying file collection; consult the concrete
     * implementation for its concurrency guarantees.
     */
    class WPCore_API IFileList : public ISharedObject
    {
    public:
        IFileList();

        /** Virtual destructor. Implementations should release any resources. */
        ~IFileList() override;

        /**
         * @brief Get the number of entries (files + directories) in the list.
         *
         * @return The number of entries currently contained in this file list.
         */
        virtual u32 getNumFiles() const = 0;

        /**
         * @brief Get the base name of the file at the given index.
         *
         * This returns only the file's name (basename) and does not include the
         * path. If an error occurs (for example the index is out of range) an
         * empty String is returned.
         *
         * @param index Zero-based index of the entry to query. Must be < getNumFiles().
         * @return The file's base name, or an empty String on error.
         */
        virtual String getFileName( u32 index ) const = 0;

        /**
         * @brief Get the full file name (including path) for the entry at index.
         *
         * The returned string contains the complete path/filename as represented
         * by the file list (implementation-defined separator / archive prefix).
         * If the index is invalid an empty String is returned.
         *
         * @param index Zero-based index of the entry to query. Must be < getNumFiles().
         * @return The full path + filename, or an empty String on error.
         */
        virtual String getFullFileName( u32 index ) const = 0;

        /**
         * @brief Get the size in bytes of the file at the given index.
         *
         * For directories the semantics are implementation-defined (commonly zero).
         *
         * @param index Zero-based index of the entry to query. Must be < getNumFiles().
         * @return The file size in bytes. For invalid index behavior is implementation-defined.
         */
        virtual u32 getFileSize( u32 index ) const = 0;

        /**
         * @brief Get the byte-offset of the file's data (for archive-backed lists).
         *
         * When the file list represents an archive (zip/usd/usdz etc.), this returns
         * the offset to the file data within the archive. For filesystem-backed
         * lists this typically returns 0 or an implementation-defined value.
         *
         * @param index Zero-based index of the entry to query. Must be < getNumFiles().
         * @return The offset in bytes. For invalid index behavior is implementation-defined.
         */
        virtual u32 getFileOffset( u32 index ) const = 0;

        /**
         * @brief Find an entry by name.
         *
         * Performs a name-based search. By default this searches for files; set
         * @p isFolder to true to search for directory entries instead.
         *
         * @param filename Name or path fragment to search for (matching rules depend on implementation).
         * @param isFolder If true, search for a directory entry; otherwise search for a file.
         * @return The zero-based index of the found entry, or -1 if not found.
         */
        virtual s32 findFile( const String &filename, bool isFolder = false ) const = 0;

        /**
         * @brief Set the root path represented by this file list.
         *
         * The path is an abstract concept used by the implementation to resolve
         * `getFullFileName` and for display/search purposes. It may be a real
         * filesystem path or an archive root.
         *
         * @param path The path to assign to this file list.
         */
        virtual void setPath( const String &path ) = 0;

        /**
         * @brief Get the root path assigned to this file list.
         *
         * @return The path string previously set with `setPath`. May be empty.
         */
        virtual String getPath() const = 0;

        /**
         * @brief Sort the entries in the file list.
         *
         * Call this after populating the list if a deterministic order is required.
         * The sort order (e.g. case sensitivity, directories first) is implementation-defined.
         */
        virtual void sort() = 0;

        /**
         * @brief Retrieve a copy of all FileInfo entries.
         *
         * Returns an Array containing FileInfo objects for every entry in the list.
         * Modifying the returned Array does not affect the internal storage of this list.
         *
         * @return An Array<FileInfo> with one element per entry.
         */
        virtual ConcurrentArray<FileInfo> &getFiles() = 0;

        /**
         * @brief Retrieve a copy of all FileInfo entries.
         *
         * Returns an Array containing FileInfo objects for every entry in the list.
         * Modifying the returned Array does not affect the internal storage of this list.
         *
         * @return An Array<FileInfo> with one element per entry.
         */
        virtual const ConcurrentArray<FileInfo> &getFiles() const = 0;

        /**
         * @brief Replace the file list contents with the provided array.
         *
         * The array is copied into the implementation's internal storage (or moved,
         * depending on the concrete type). Call `sort()` afterwards if needed.
         *
         * @param files Array of FileInfo objects to set as the list contents.
         */
        virtual void setFiles( const Array<FileInfo> &files ) = 0;

        /**
         * @brief Append a FileInfo entry to the internal file container.
         *
         * Use this to add a single file/directory entry to the list. Call `sort()`
         * afterwards if you require the list to remain ordered.
         *
         * @param file FileInfo structure describing the entry to add.
         */
        virtual void addFile( const FileInfo &file ) = 0;

        /**
         * @brief Remove a FileInfo entry matching the provided FileInfo.
         *
         * Removal semantics (match by filename, id, path, etc.) are implementation-defined.
         *
         * @param file FileInfo describing the entry to remove.
         */
        virtual void removeFile( const FileInfo &file ) = 0;

        /**
         * @brief Find a FileInfo by its UUID.
         *
         * Searches the list for an entry matching the provided unique identifier.
         *
         * @param id The UUID to search for.
         * @param[out] fileInfo On success, receives the matching FileInfo.
         * @param ignorePath If true, match is performed ignoring the path component.
         * @return true if a matching FileInfo was found and written to @p fileInfo.
         */
        virtual bool findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath = false,
                                   bool ignoreCase = false ) const = 0;

        /**
         * @brief Find a FileInfo by file path.
         *
         * Searches the list for an entry whose path or filename matches @p filePath.
         *
         * @param filePath The file path or name to locate.
         * @param[out] fileInfo On success, receives the matching FileInfo.
         * @param ignorePath If true, match is performed ignoring the path component.
         * @return true if a matching FileInfo was found and written to @p fileInfo.
         */
        virtual bool findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath = false,
                                   bool ignoreCase = false ) const = 0;

        /**
         * @brief Check whether an entry with the specified path exists in the list.
         *
         * The comparison behavior can be adjusted with the flags:
         * - ignorePath: compare only the filename portion if true.
         * - ignoreCase: perform case-insensitive comparison if true.
         *
         * @param filePath Path or filename to check for.
         * @param ignorePath If true, only the basename is compared.
         * @param ignoreCase If true, comparison is case-insensitive.
         * @return true if a matching entry exists; false otherwise.
         */
        virtual bool exists( const String &filePath, bool ignorePath = false,
                             bool ignoreCase = false ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IFileList_h__
