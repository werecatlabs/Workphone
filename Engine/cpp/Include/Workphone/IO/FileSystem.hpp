#ifndef _WP_FileSystem_H_
#define _WP_FileSystem_H_

#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/Map.hpp>
#include <Workphone/Core/FileInfo.hpp>

namespace FW
{
    class FileWatcher;
}

namespace workphone
{

    class FileListener;

    /**
     * @class FileSystem
     * @brief Concrete implementation of the IFileSystem interface.
     *
     * The FileSystem class provides a centralized API for managing files,
     * folders and archives used by the engine. It enumerates and caches
     * files, provides utility functions for reading/writing files and
     * exposes folder/archive registration and lookup helpers.
     *
     * Thread-safety:
     * - Internal containers that can be accessed concurrently are guarded
     *   by ConcurrentArray members. External synchronization may still be
     *   required for compound operations.
     *
     * Usage example:
     * - Register folders and archives using @c addFolder and @c addFileArchive.
     * - Query files via @c getFiles or read contents with @c readAllText.
     */
    class WPCore_API FileSystem : public IFileSystem
    {
    public:
        /// List of file extensions treated as text files (e.g. "txt", "xml").
        static const Array<String> textExtensions;

        /// List of file extensions treated as binary files (e.g. "png", "zip").
        static const Array<String> binaryExtensions;

        /** @brief Constructs a new FileSystem instance. */
        FileSystem();

        /** @brief Destroys the FileSystem instance. */
        ~FileSystem() override;

        /** @copydoc ISharedObject::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::reload */
        void reload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Creates and opens a native file dialog for user file selection.
         * @return SmartPtr to an INativeFileDialog instance. May be null if the
         *         dialog cannot be created on the current platform.
         */
        SmartPtr<INativeFileDialog> openFileDialog() override;

        /** @copydoc IFileSystem::open */
        SmartPtr<IStream> open( const String &filePath ) override;

        /**
         * @copydoc IFileSystem::open
         * @param input Open as input (read) when true, output (write) when false.
         * @param binary Open stream in binary mode when true.
         * @param truncate Truncate existing file when opening for write.
         * @param ignorePath When true, search for the file across registered archives and folders.
         * @param ignoreCase When true, perform case-insensitive matching.
         */
        SmartPtr<IStream> open( const String &filePath, bool input, bool binary, bool truncate,
                                bool ignorePath = false, bool ignoreCase = false ) override;

        /** @copydoc IFileSystem::addFileArchive */
        bool addFileArchive( const String &filename, bool ignoreCase = true, bool ignorePath = true,
                             ArchiveType archiveType = ArchiveType::FileSystem,
                             const String &password = StringUtil::EmptyString ) override;

        /**
         * @copydoc IFileSystem::addFolder
         * @param folderPath Path to the folder to register.
         * @param recursive When true, adds subfolders recursively.
         */
        void addFolder( const String &folderPath, bool recursive = false ) override;

        /**
         * @brief Adds a folder listing object to the file system.
         * @param folderListing Smart pointer to an IFolderExplorer that enumerates folder contents.
         */
        void addFolder( SmartPtr<IFolderExplorer> folderListing );

        /** @copydoc IFileSystem::addArchive */
        void addArchive( const String &filePath ) override;

        /** @copydoc IFileSystem::addArchive */
        void addArchive( const String &filePath, const String &typeName ) override;

        /** @copydoc IFileSystem::addArchive */
        void addArchive( const String &filePath, ArchiveType type ) override;

        /**
         * @brief Returns the number of registered file archives.
         * @return Number of file archives currently registered.
         */
        u32 getFileArchiveCount() const override;

        /**
         * @brief Removes the file archive at the specified index.
         * @param index Index of the archive to remove.
         * @return True if removal succeeded, false otherwise.
         */
        bool removeFileArchive( u32 index ) override;

        /**
         * @brief Removes a file archive by its filename.
         * @param filename Name of the archive to remove.
         * @return True if removal succeeded, false otherwise.
         */
        bool removeFileArchive( const String &filename ) override;

        /**
         * @brief Moves a file archive from sourceIndex by a relative offset.
         * @param sourceIndex Index of the archive to move.
         * @param relative Relative offset to move the archive by (positive or negative).
         * @return True if the move succeeded, false otherwise.
         */
        bool moveFileArchive( u32 sourceIndex, s32 relative ) override;

        /**
         * @brief Retrieves a file archive by index.
         * @param index Index of the archive to retrieve.
         * @return SmartPtr to the IArchive or null if index is out of range.
         */
        SmartPtr<IArchive> getFileArchive( u32 index ) override;

        /**
         * @brief Resolves a filename to an absolute path.
         * @param filename Relative or archive-qualified filename.
         * @return Absolute filesystem path to the file. Returns an empty string if not found.
         */
        String getAbsolutePath( const String &filename ) const override;

        /**
         * @brief Returns the directory portion of a filename.
         * @param filename Path or file name.
         * @return Directory component (without trailing separator) or empty string if none.
         */
        String getFileDir( const String &filename ) const override;

        /**
         * @brief Checks whether a file exists.
         * @param filePath File path to test.
         * @param ignorePath If true, search registered archives and folders as well.
         * @param ignoreCase When true, perform case-insensitive matching.
         * @return True if the file exists, false otherwise.
         */
        bool isExistingFile( const String &filePath, bool ignorePath = false,
                             bool ignoreCase = true ) const override;

        /**
         * @brief Checks whether a file exists, optionally relative to a base path.
         * @param path Base search path.
         * @param filePath File path or name to search for.
         * @param ignorePath If true, search registered archives and folders as well.
         * @param ignoreCase When true, perform case-insensitive matching.
         * @return True if the file exists, false otherwise.
         */
        bool isExistingFile( const String &path, const String &filePath, bool ignorePath = false,
                             bool ignoreCase = true ) const override;

        /**
         * @brief Tests whether a folder exists at the specified path.
         * @param path Folder path to test.
         * @return True if the folder exists, false otherwise.
         */
        bool isExistingFolder( const String &path ) const override;

        /**
         * @brief Creates directories for the provided path, including intermediate directories.
         * @param path Directory path to create.
         */
        void createDirectories( const String &path ) override;

        /**
         * @brief Deletes files that match within the given path.
         * @param path Path containing files to delete.
         */
        void deleteFilesFromPath( const String &path ) override;

        /**
         * @brief Returns file information for files with the specified extension.
         * @param extension Extension to match (e.g. ".txt" or "txt").
         * @return Array of FileInfo matching the extension.
         */
        Array<FileInfo> getFilesWithExtension( const String &extension ) const override;

        /**
         * @brief Returns file information for files with the specified extension inside a path.
         * @param path Path to search.
         * @param extension Extension to match.
         * @return Array of FileInfo matching the extension in the specified path.
         */
        Array<FileInfo> getFilesWithExtension( const String &path,
                                               const String &extension ) const override;

        /**
         * @brief Returns file names that match the given extension from the registered files.
         * @param extension Extension to match.
         * @return Array of matching filenames.
         */
        Array<String> getFileNamesWithExtension( const String &extension ) const override;

        /**
         * @brief Appends file names with the specified extension to the provided output array.
         * @param extension File extension to match.
         * @param fileNames Output array that will be populated with matching names.
         */
        void getFileNamesWithExtension( const String &extension,
                                        Array<String> &fileNames ) const override;

        /**
         * @brief Populates the given array with filenames found directly in the specified folder.
         * @param path Folder path to enumerate.
         * @param fileNames Output array to receive filenames (non-recursive).
         */
        void getFileNamesInFolder( const String &path, Array<String> &fileNames ) override;

        /**
         * @brief Enumerates immediate subfolders of the given path.
         * @param path Folder path to inspect.
         * @param folderNames Output array populated with subfolder names.
         */
        void getSubFolders( const String &path, Array<String> &folderNames ) override;

        /**
         * @brief Tests whether the specified path refers to a folder.
         * @param path Path to test.
         * @return True if path is a folder, false otherwise.
         */
        bool isFolder( const String &path ) override;

        /**
         * @brief Sets the process working directory.
         * @param directory New working directory path.
         * @return True on success, false on failure.
         */
        bool setWorkingDirectory( const String &directory ) override;

        /**
         * @brief Returns the current process working directory.
         * @return Absolute path of the working directory.
         */
        String getWorkingDirectory() override;

        /**
         * @brief Reads the entire contents of a file into a byte array.
         * @param path Path to the file.
         * @return Array of bytes containing file contents. Empty array on failure.
         */
        Array<u8> readAllBytes( const String &path ) override;

        /**
         * @brief Reads the entire contents of a file as a text string.
         * @param path Path to the file.
         * @return File contents as String. Empty string on failure.
         */
        String readAllText( const String &path ) override;

        /**
         * @brief Writes raw bytes to a file.
         * @param path Destination file path.
         * @param bytes Pointer to bytes to write.
         * @param size Number of bytes to write.
         */
        void writeAllBytes( const String &path, u8 *bytes, u32 size ) override;

        /**
         * @brief Writes a byte array to a file.
         * @param path Destination file path.
         * @param bytes Array of bytes to write.
         */
        void writeAllBytes( const String &path, Array<u8> bytes ) override;

        /**
         * @brief Writes a text string to a file using the engine string encoding.
         * @param path Destination file path.
         * @param contents Text to write.
         */
        void writeAllText( const String &path, const String &contents ) override;

        /**
         * @brief Encodes the contents of a stream into a base64 string.
         * @param pStream Input stream to encode.
         * @return Base64-encoded representation of stream data.
         */
        String getBase64String( SmartPtr<IStream> &pStream ) override;

        /**
         * @brief Encodes the contents of an std::ifstream into a base64 string.
         * @param is Input std::ifstream.
         * @return Base64-encoded representation of stream data.
         */
        String getBase64String( std::ifstream &is );

        /**
         * @brief Returns a printable hex or byte representation of the stream contents.
         * @param pStream Input stream to convert.
         * @return String representation of bytes in the stream.
         */
        String getBytesString( SmartPtr<IStream> &pStream ) override;

        /**
         * @brief Returns a printable hex or byte representation of an std::ifstream contents.
         * @param is Input std::ifstream.
         * @return String representation of bytes in the stream.
         */
        String getBytesString( std::ifstream &is );

        /**
         * @brief Recursively copies all files and subfolders from srcPath to dstPath.
         * @param srcPath Source folder.
         * @param dstPath Destination folder.
         */
        void copyFolder( const String &srcPath, const String &dstPath ) override;

        /**
         * @brief Copies a single file.
         * @param srcPath Source file path.
         * @param dstPath Destination file path.
         */
        void copyFile( const String &srcPath, const String &dstPath ) override;

        /**
         * @brief Deletes a single file from disk/archives.
         * @param filePath Path to the file to delete.
         */
        void deleteFile( const String &filePath ) override;

        /**
         * @brief Extracts the path component from a combined path.
         * @param path Combined file/path.
         * @return Directory path component.
         */
        String getFilePath( const String &path ) override;

        /**
         * @brief Extracts the filename component from a combined path.
         * @param path Combined file/path.
         * @return Filename component.
         */
        String getFileName( const String &path ) override;

        /**
         * @brief Converts a textual archive type name to the ArchiveType enum.
         * @param typeName Archive type name (e.g. "zip", "filesystem").
         * @return ArchiveType corresponding to the name. If unknown, returns ArchiveType::FileSystem.
         */
        ArchiveType getTypeFromTypeName( const String &typeName ) const;

        /**
         * @brief Computes a hash (e.g. MD5/SHA) of the specified file.
         * @param pFilePath Path to the file to hash.
         * @return Hexadecimal string containing the computed hash. Empty on failure.
         */
        String getFileHash( const String &pFilePath ) override;

        /**
         * @brief Returns a list of folder names under the specified path.
         * @param path Root path to search.
         * @param recursive When true, includes nested subfolders.
         * @return Array of folder paths.
         */
        Array<String> getFolders( const String &path, bool recursive = false ) override;

        /**
         * @brief Lists files in the specified path.
         * @param path Path to search.
         * @param partialPathMatch When true, includes files that partially match the provided path.
         * @return Array of file paths (relative to registered roots).
         */
        Array<String> getFiles( const String &path, bool partialPathMatch = false ) override;

        /**
         * @brief Returns file paths as absolute filesystem paths.
         * @param path Path to search.
         * @param recursive When true, enumerates subfolders recursively.
         * @return Array of absolute file paths.
         */
        Array<String> getFilesAsAbsolutePaths( const String &path, bool recursive = false ) override;

        /**
         * @brief Returns file names with the specified extension from the provided path.
         * @param path Path to search.
         * @param extension Extension to match.
         * @param recursive When true, searches recursively.
         * @return Array of matching file names.
         */
        Array<String> getFileNamesWithExtension( const String &path, const String &extension,
                                                 bool recursive = false ) override;

        /**
         * @brief Returns a folder listing helper for the specified path.
         * @param path Path to list.
         * @return SmartPtr to an IFolderExplorer, or null if listing cannot be created.
         */
        SmartPtr<IFolderExplorer> getFolderListing( const String &path ) override;

        /**
         * @brief Returns a folder listing helper for the specified path filtered by extension.
         * @param path Path to list.
         * @param ext Extension filter.
         * @return SmartPtr to an IFolderExplorer filtered by extension.
         */
        SmartPtr<IFolderExplorer> getFolderListing( const String &path, const String &ext ) override;

        /** @copydoc IFileSystem::addFile */
        void addFile( const FileInfo &file ) override;

        /** @copydoc IFileSystem::addFiles */
        void addFiles( const Array<FileInfo> &files ) override;

        /** @copydoc IFileSystem::removeFile */
        void removeFile( const FileInfo &file ) override;

        /** @copydoc IFileSystem::removeFiles */
        void removeFiles( const Array<FileInfo> &files ) override;

        /**
         * @brief Finds file metadata by UUID.
         * @param id File UUID to search for.
         * @param fileInfo Output parameter that will receive the FileInfo if found.
         * @param ignorePath When true, ignores path differences during the search.
         * @return True if the file info was found, false otherwise.
         */
        bool findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath = false ) const override;

        /**
         * @brief Finds file metadata by path.
         * @param filePath Path or filename to search for.
         * @param fileInfo Output parameter that will receive the FileInfo if found.
         * @param ignorePath When true, ignores path differences during the search.
         * @return True if the file info was found, false otherwise.
         */
        bool findFileInfo( const String &filePath, FileInfo &fileInfo,
                           bool ignorePath = false ) const override;

        /** @copydoc IFileSystem::getSystemFiles */
        Array<FileInfo> getSystemFiles() const override;

        /**
         * @brief Refreshes all registered folders/archives.
         * @param async When true, performs refresh asynchronously where supported.
         */
        void refreshAll( bool async ) override;

        /**
         * @brief Refreshes a specific path (folder/archive).
         * @param path Path to refresh.
         * @param async When true, performs refresh asynchronously where supported.
         */
        void refreshPath( const String &path, bool async ) override;

        /**
         * @brief Returns the UUID associated with a file path if known.
         * @param filePath File path to query.
         * @return UUID corresponding to the file, or an invalid UUID if unknown.
         */
        UUID getFileId( const String &filePath ) const override;

        /** @brief Acquire an internal lock to protect compound operations. */
        void lock() override;

        /** @brief Release the internal lock. */
        void unlock() override;

        /** @copydoc IObject::isValid */
        bool isValid() const override;

        WP_CLASS_REGISTER_DECL;

    private:
        /**
         * @brief Determines if a file resides in the same directory as the given path.
         * @param[in] path Directory path to compare against.
         * @param[in] file File path to test.
         * @return Non-negative index (or other non-negative indicator) if the file is in the same
         * directory, or -1 if the file is not in the specified directory.
         */
        s32 isInSameDirectory( const String &path, const String &file );

        /** @brief Returns a snapshot copy of the internal file list. */
        Array<FileInfo> getFiles() const;

        /** @brief Replaces the internal file list with the provided array. */
        void setFiles( const Array<FileInfo> &files );

        /**
         * @brief Adds an archive to the folder archive list.
         * @param archive Archive to add.
         */
        void addFolderArchive( SmartPtr<IArchive> archive );

        /**
         * @brief Removes an archive from the folder archive list.
         * @param archive Archive to remove.
         */
        void removeFolderArchive( SmartPtr<IArchive> archive );

        /**
         * @brief Adds an archive to the file archive list.
         * @param archive Archive to add.
         */
        void addFileArchive( SmartPtr<IArchive> archive );

        /**
         * @brief Removes an archive from the file archive list.
         * @param archive Archive to remove.
         */
        void removeFileArchive( SmartPtr<IArchive> archive );

        /// Pointer to a platform-specific file watcher used to monitor changes.
        FW::FileWatcher *m_fileWatcher = nullptr;

        /// Listener wrapper for file watch events.
        FileListener *m_fileListener = nullptr;

        /// An array of files cached by the file system.
        ConcurrentArray<FileInfo> m_files;

        /// Registered folder archives (archives mounted as folders).
        ConcurrentArray<SmartPtr<IArchive>> m_folderArchives;

        /// Registered file archives (archives treated as file containers).
        ConcurrentArray<SmartPtr<IArchive>> m_fileArchives;
    };
}  // namespace workphone

#endif
