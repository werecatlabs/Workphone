#ifndef __IFileSystem__
#define __IFileSystem__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Core/FileInfo.hpp>

namespace workphone
{

    /**
     * @brief Interface for a file system that provides file and archive management capabilities.
     *
     * This class defines a comprehensive interface for a file system, providing methods for:
     * - Opening, reading, and writing files
     * - Managing archives (folders, ZIP files, etc.)
     * - File and directory operations (copy, delete, create)
     * - File system queries and searches
     * - File monitoring and event handling
     * - Path manipulation and validation
     *
     * The file system supports multiple archive types and can search through multiple
     * resource paths to locate files. It also provides utilities for file hashing,
     * base64 encoding, and folder exploration.
     *
     * @note This is an abstract interface that must be implemented by concrete file system classes.
     * @see ISharedObject, IStream, IArchive, FileInfo
     */
    class WPCore_API IFileSystem : public ISharedObject
    {
    public:
        /**
         * @brief Enumeration defining the types of archives supported by the file system.
         */
        enum class ArchiveType
        {
            Folder,         ///< A regular folder/directory archive
            FileSystem,     ///< A file system archive (default type)
            Zip,            ///< A standard ZIP archive
            ObfuscatedZip,  ///< An obfuscated/encrypted ZIP archive
            Unknown,        ///< An unknown or unsupported archive type

            Count  ///< Total number of archive types (for iteration)
        };

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of file system resources and attached archives.
         */
        ~IFileSystem() override;

        /**
         * @brief Creates a native file dialog for file selection.
         *
         * @return A smart pointer to the created native file dialog interface.
         * @note The returned dialog can be used to prompt users for file selection.
         */
        virtual SmartPtr<INativeFileDialog> openFileDialog() = 0;

        /**
         * @brief Opens a file for reading, searching through all resource paths.
         *
         * This method searches through all registered archives and resource paths
         * to locate and open the specified file for reading.
         *
         * @param filePath The path of the file to open (can be relative or absolute).
         * @return A smart pointer to the opened file stream, or null if the file cannot be opened.
         * @note The file is opened in read mode by default.
         */
        virtual SmartPtr<IStream> open( const String &filePath ) = 0;

        /**
         * @brief Opens a file with specified access parameters.
         *
         * Opens a file with full control over the access mode, binary/text mode,
         * and search behavior.
         *
         * @param filePath The path of the file to open.
         * @param input If true, opens the file for input (reading); if false, opens for output
         * (writing).
         * @param binary If true, opens the file in binary mode; if false, opens in text mode.
         * @param truncate If true, truncates the file when opening for writing.
         * @param ignorePath If true, only searches by filename, ignoring the path component.
         * @param ignoreCase If true, performs case-insensitive file name matching.
         * @return A smart pointer to the opened file stream, or null if the file cannot be opened.
         * @note When ignorePath is true, the file system will search all archives for a file with the
         * given name.
         */
        virtual SmartPtr<IStream> open( const String &filePath, bool input, bool binary, bool truncate,
                                        bool ignorePath = false, bool ignoreCase = false ) = 0;

        /**
         * @brief Adds an archive to the file system for file searching.
         *
         * Archives are searched in the order they are added. Files in archives added later
         * will override files with the same name in archives added earlier.
         *
         * @param filename The path to the archive file or folder to add.
         * @param ignoreCase If true, file name matching within this archive is case-insensitive.
         * @param ignorePaths If true, files in this archive are accessed by filename only.
         * @param archiveType The type of archive being added.
         * @param password The password for encrypted archives (currently only used for ObfuscatedZip).
         * @return True if the archive was successfully added, false otherwise.
         * @note Adding an archive does not load its contents into memory; files are loaded on-demand.
         */
        virtual bool addFileArchive( const String &filename, bool ignoreCase = true,
                                     bool ignorePaths = true,
                                     ArchiveType archiveType = ArchiveType::FileSystem,
                                     const String &password = StringUtil::EmptyString ) = 0;

        /**
         * @brief Adds a folder to the file system for file searching.
         *
         * The folder and its contents become available for file operations.
         * Files in the folder can be accessed using relative paths from the folder root.
         *
         * @param folderPath The path to the folder to add to the file system.
         * @param recursive If true, includes all subfolders and their contents recursively.
         * @note This is a convenience method that calls addFileArchive with ArchiveType::Folder.
         */
        virtual void addFolder( const String &folderPath, bool recursive = false ) = 0;

        /**
         * @brief Adds an archive file to the file system with default settings.
         *
         * @param filePath The path to the archive file to add.
         * @note Uses default settings: case-insensitive, ignore paths, FileSystem archive type.
         */
        virtual void addArchive( const String &filePath ) = 0;

        /**
         * @brief Adds an archive file to the file system with a specified type name.
         *
         * @param filePath The path to the archive file to add.
         * @param typeName The name identifying the type of archive.
         * @note The typeName is used for archive identification and may affect how the archive is
         * processed.
         */
        virtual void addArchive( const String &filePath, const String &typeName ) = 0;

        /**
         * @brief Adds an archive file to the file system with a specified archive type.
         *
         * @param filePath The path to the archive file to add.
         * @param type The specific type of archive being added.
         * @note Different archive types may have different performance characteristics and capabilities.
         */
        virtual void addArchive( const String &filePath, ArchiveType type ) = 0;

        /**
         * @brief Gets the total number of archives currently attached to the file system.
         *
         * @return The number of archives that have been added and are currently active.
         * @note This count includes all types of archives (folders, ZIP files, etc.).
         */
        virtual u32 getFileArchiveCount() const = 0;

        /**
         * @brief Removes an archive from the file system by index.
         *
         * This will close the archive and free any file handles, but will not close resources
         * that have already been loaded and are now cached (e.g., textures, meshes).
         *
         * @param index The zero-based index of the archive to remove.
         * @return True if the archive was successfully removed, false if the index is invalid.
         * @note Removing an archive does not affect files that have already been loaded into memory.
         */
        virtual bool removeFileArchive( u32 index ) = 0;

        /**
         * @brief Removes an archive from the file system by filename.
         *
         * This will close the archive and free any file handles, but will not close resources
         * that have already been loaded and are now cached (e.g., textures, meshes).
         *
         * @param filename The path of the archive file to remove.
         * @return True if the archive was found and successfully removed, false otherwise.
         * @note The filename must match exactly the path used when the archive was added.
         */
        virtual bool removeFileArchive( const String &filename ) = 0;

        /**
         * @brief Changes the search order of attached archives.
         *
         * Archives with lower indices are searched first when looking for files.
         * This method allows reordering archives to control file resolution priority.
         *
         * @param sourceIndex The index of the archive to move.
         * @param relative The relative change in position (positive moves down, negative moves up).
         * @return True if the archive was successfully moved, false if the index is invalid.
         * @note Moving an archive affects the search order for all subsequent file operations.
         */
        virtual bool moveFileArchive( u32 sourceIndex, s32 relative ) = 0;

        /**
         * @brief Gets the archive at a specific index.
         *
         * @param index The zero-based index of the archive to retrieve.
         * @return A smart pointer to the archive interface, or null if the index is invalid.
         * @note This allows direct access to archive properties and methods.
         */
        virtual SmartPtr<IArchive> getFileArchive( u32 index ) = 0;

        /**
         * @brief Gets the current working directory of the file system.
         *
         * @return The current working directory as a string.
         * @note The working directory is used as a base for relative path resolution.
         */
        virtual String getWorkingDirectory() = 0;

        /**
         * @brief Changes the current working directory of the file system.
         *
         * @param directory The new working directory path (operating system dependent).
         * @return True if the directory was successfully changed, false otherwise.
         * @note The working directory affects how relative paths are resolved.
         */
        virtual bool setWorkingDirectory( const String &directory ) = 0;

        /**
         * @brief Converts a relative path to an absolute path, resolving symbolic links.
         *
         * @param filename The relative file or directory path to convert.
         * @return The absolute path that points to the same file or directory.
         * @note This method resolves symbolic links and provides a unique, canonical path.
         */
        virtual String getAbsolutePath( const String &filename ) const = 0;

        /**
         * @brief Checks if a file exists and can be opened.
         *
         * @param filePath The path of the file to check.
         * @param ignorePath If true, only checks the filename part, ignoring the path.
         * @param ignoreCase If true, performs case-insensitive file name matching.
         * @return True if the file exists and can be opened, false otherwise.
         * @note This method searches through all registered archives and resource paths.
         */
        virtual bool isExistingFile( const String &filePath, bool ignorePath = false,
                                     bool ignoreCase = true ) const = 0;

        /**
         * @brief Checks if a file exists in a specific path and can be opened.
         *
         * @param path The base path to search in.
         * @param filePath The relative file path to check.
         * @param ignorePath If true, only checks the filename part, ignoring the path.
         * @param ignoreCase If true, performs case-insensitive file name matching.
         * @return True if the file exists and can be opened, false otherwise.
         * @note This method searches within the specified path and its archives.
         */
        virtual bool isExistingFile( const String &path, const String &filePath, bool ignorePath = false,
                                     bool ignoreCase = true ) const = 0;

        /**
         * @brief Checks if a folder exists in the file system.
         *
         * @param path The path of the folder to check.
         * @return True if the folder exists, false otherwise.
         * @note This checks both the physical file system and virtual folders in archives.
         */
        virtual bool isExistingFolder( const String &path ) const = 0;

        /**
         * @brief Extracts the directory path from a full file path.
         *
         * @param filename The full path to a file.
         * @return The directory path containing the file.
         * @note This method handles various path separators and formats.
         */
        virtual String getFileDir( const String &filename ) const = 0;

        /**
         * @brief Creates all directories in the specified path that don't exist.
         *
         * Creates the full directory structure, including any parent directories
         * that don't already exist.
         *
         * @param path The directory path to create.
         * @note This method will create intermediate directories as needed.
         */
        virtual void createDirectories( const String &path ) = 0;

        /**
         * @brief Deletes all files in the specified directory path.
         *
         * @param path The path of the directory containing files to delete.
         * @note This only deletes files, not subdirectories. Use with caution.
         */
        virtual void deleteFilesFromPath( const String &path ) = 0;

        /**
         * @brief Gets all files with a specified extension from all archives.
         *
         * @param extension The file extension to search for (e.g., "txt", "png").
         * @return An array containing FileInfo objects for all matching files.
         * @note The extension should not include the leading dot.
         */
        virtual Array<FileInfo> getFilesWithExtension( const String &extension ) const = 0;

        /**
         * @brief Gets all files with a specified extension from a specific path.
         *
         * @param path The base path to search in.
         * @param extension The file extension to search for (e.g., "txt", "png").
         * @return An array containing FileInfo objects for all matching files.
         * @note The extension should not include the leading dot.
         */
        virtual Array<FileInfo> getFilesWithExtension( const String &path,
                                                       const String &extension ) const = 0;

        /**
         * @brief Gets the names of all files with a specified extension from all archives.
         *
         * @param extension The file extension to search for (e.g., "txt", "png").
         * @return An array containing the names of all matching files.
         * @note The extension should not include the leading dot.
         */
        virtual Array<String> getFileNamesWithExtension( const String &extension ) const = 0;

        /**
         * @brief Gets the names of all files with a specified extension from all archives.
         *
         * @param extension The file extension to search for (e.g., "txt", "png").
         * @param fileNames An array that will be filled with the names of matching files.
         * @note The extension should not include the leading dot.
         */
        virtual void getFileNamesWithExtension( const String &extension,
                                                Array<String> &fileNames ) const = 0;

        /**
         * @brief Gets the names of all files in a specified folder.
         *
         * @param path The path of the folder to search.
         * @param fileNames An array that will be filled with the names of files in the folder.
         * @note This method searches both the physical file system and virtual folders in archives.
         */
        virtual void getFileNamesInFolder( const String &path, Array<String> &fileNames ) = 0;

        /**
         * @brief Gets the names of all subfolders in a specified folder.
         *
         * @param path The path of the parent folder.
         * @param folderNames An array that will be filled with the names of subfolders.
         * @note This method searches both the physical file system and virtual folders in archives.
         */
        virtual void getSubFolders( const String &path, Array<String> &folderNames ) = 0;

        /**
         * @brief Determines if the specified path is a folder.
         *
         * @param path The path to check.
         * @return True if the path exists and is a folder, false otherwise.
         * @note This checks both the physical file system and virtual folders in archives.
         */
        virtual bool isFolder( const String &path ) = 0;

        /**
         * @brief Reads all bytes from a file into memory.
         *
         * @param path The path to the file to read.
         * @return An array containing all bytes from the file.
         * @note This method loads the entire file into memory, so use with caution for large files.
         */
        virtual Array<u8> readAllBytes( const String &path ) = 0;

        /**
         * @brief Reads all text from a file as a string.
         *
         * @param path The path to the text file to read.
         * @return A string containing all text from the file.
         * @note This method assumes the file contains text data and handles encoding appropriately.
         */
        virtual String readAllText( const String &path ) = 0;

        /**
         * @brief Writes a byte array to a file.
         *
         * @param path The path to the file to write.
         * @param bytes A pointer to the byte array to write.
         * @param size The number of bytes to write.
         * @note This method will create the file if it doesn't exist, or overwrite it if it does.
         */
        virtual void writeAllBytes( const String &path, u8 *bytes, u32 size ) = 0;

        /**
         * @brief Writes a byte array to a file.
         *
         * @param path The path to the file to write.
         * @param bytes The array of bytes to write to the file.
         * @note This method will create the file if it doesn't exist, or overwrite it if it does.
         */
        virtual void writeAllBytes( const String &path, Array<u8> bytes ) = 0;

        /**
         * @brief Writes text content to a file.
         *
         * @param path The path to the file to write.
         * @param contents The text string to write to the file.
         * @note This method will create the file if it doesn't exist, or overwrite it if it does.
         */
        virtual void writeAllText( const String &path, const String &contents ) = 0;

        /**
         * @brief Converts the contents of a stream to a base64-encoded string.
         *
         * @param pStream A smart pointer to the stream to convert.
         * @return A base64-encoded string representation of the stream contents.
         * @note This is useful for embedding binary data in text-based formats.
         */
        virtual String getBase64String( SmartPtr<IStream> &pStream ) = 0;

        /**
         * @brief Converts the contents of a stream to a bytes string representation.
         *
         * @param pStream A smart pointer to the stream to convert.
         * @return A string representation of the stream contents as bytes.
         * @note This is useful for debugging or displaying binary data as text.
         */
        virtual String getBytesString( SmartPtr<IStream> &pStream ) = 0;

        /**
         * @brief Copies an entire folder and its contents to a new location.
         *
         * @param srcPath The source folder path to copy from.
         * @param dstPath The destination folder path to copy to.
         * @note This method recursively copies all files and subfolders.
         */
        virtual void copyFolder( const String &srcPath, const String &dstPath ) = 0;

        /**
         * @brief Copies a single file from one location to another.
         *
         * @param srcPath The source file path to copy from.
         * @param dstPath The destination file path to copy to.
         * @note This method will overwrite the destination file if it already exists.
         */
        virtual void copyFile( const String &srcPath, const String &dstPath ) = 0;

        /**
         * @brief Deletes a single file from the file system.
         *
         * @param filePath The path of the file to delete.
         * @note This method permanently deletes the file. Use with caution.
         */
        virtual void deleteFile( const String &filePath ) = 0;

        /**
         * @brief Gets the full path of a file or directory.
         *
         * @param path The path to a file or directory.
         * @return The full, resolved path of the file or directory.
         * @note This method resolves relative paths and symbolic links.
         */
        virtual String getFilePath( const String &path ) = 0;

        /**
         * @brief Extracts the filename from a full file path.
         *
         * @param path The full path to a file.
         * @return The filename (without the directory path).
         * @note This method handles various path separators and formats.
         */
        virtual String getFileName( const String &path ) = 0;

        /**
         * @brief Calculates the MD5 hash of a file.
         *
         * @param pFilePath The path to the file to hash.
         * @return The MD5 hash of the file as a hexadecimal string.
         * @note This method reads the entire file to calculate the hash, so it may be slow for large
         * files.
         */
        virtual String getFileHash( const String &pFilePath ) = 0;

        /**
         * @brief Gets the names of all folders within the specified path.
         *
         * @param path The path to the parent directory.
         * @param recursive If true, searches subdirectories recursively for folders.
         * @return An array of folder names found in the specified path.
         * @note This method searches both the physical file system and virtual folders in archives.
         */
        virtual Array<String> getFolders( const String &path, bool recursive = false ) = 0;

        /**
         * @brief Gets the names of all files within the specified path.
         *
         * @param path The path to the directory containing the files.
         * @param partialPathMatch If true, performs partial path matching for file names.
         * @return An array of file names found in the specified path.
         * @note This method searches both the physical file system and virtual folders in archives.
         */
        virtual Array<String> getFiles( const String &path, bool partialPathMatch = false ) = 0;

        /**
         * @brief Gets the absolute paths of all files within the specified path.
         *
         * @param path The path to the directory containing the files.
         * @param recursive If true, searches subdirectories recursively for files.
         * @return An array of absolute file paths found in the specified path.
         * @note This method returns full, resolved paths for all matching files.
         */
        virtual Array<String> getFilesAsAbsolutePaths( const String &path, bool recursive = false ) = 0;

        /**
         * @brief Gets the names of all files with a specified extension within the specified path.
         *
         * @param path The path to the directory containing the files.
         * @param extension The file extension to filter by (e.g., "txt", "png").
         * @param recursive If true, searches subdirectories recursively for files.
         * @return An array of file names with the specified extension.
         * @note The extension should not include the leading dot.
         */
        virtual Array<String> getFileNamesWithExtension( const String &path, const String &extension,
                                                         bool recursive = false ) = 0;

        /**
         * @brief Gets a folder explorer for the specified path.
         *
         * @param path The path to get the folder explorer for.
         * @return A smart pointer to an IFolderExplorer object for browsing the folder contents.
         * @note The folder explorer provides an interface for iterating through folder contents.
         */
        virtual SmartPtr<IFolderExplorer> getFolderListing( const String &path ) = 0;

        /**
         * @brief Gets a folder explorer for the specified path with extension filtering.
         *
         * @param path The path to get the folder explorer for.
         * @param ext The file extension to filter the results by.
         * @return A smart pointer to an IFolderExplorer object for browsing filtered folder contents.
         * @note The folder explorer will only show files with the specified extension.
         */
        virtual SmartPtr<IFolderExplorer> getFolderListing( const String &path, const String &ext ) = 0;

        /**
         * @brief Adds a file to the file system's internal tracking.
         *
         * @param file The FileInfo object representing the file to add.
         * @note This method updates the file system's internal file registry.
         */
        virtual void addFile( const FileInfo &file ) = 0;

        /**
         * @brief Adds multiple files to the file system's internal tracking.
         *
         * @param files An array of FileInfo objects representing the files to add.
         * @note This method updates the file system's internal file registry.
         */
        virtual void addFiles( const Array<FileInfo> &files ) = 0;

        /**
         * @brief Removes a file from the file system's internal tracking.
         *
         * @param file The FileInfo object representing the file to remove.
         * @note This method updates the file system's internal file registry but does not delete the
         * physical file.
         */
        virtual void removeFile( const FileInfo &file ) = 0;

        /**
         * @brief Removes multiple files from the file system's internal tracking.
         *
         * @param files An array of FileInfo objects representing the files to remove.
         * @note This method updates the file system's internal file registry but does not delete the
         * physical files.
         */
        virtual void removeFiles( const Array<FileInfo> &files ) = 0;

        /**
         * @brief Finds a FileInfo object by file ID.
         *
         * @param id The UUID of the file to search for.
         * @param fileInfo A reference to a FileInfo object to store the results in.
         * @param ignorePath If true, the search will ignore the file path component.
         * @return True if a FileInfo object was found for the specified file ID, false otherwise.
         * @note This method searches through the file system's internal file registry.
         */
        virtual bool findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath = false ) const = 0;

        /**
         * @brief Finds a FileInfo object by file path.
         *
         * @param filePath The path of the file to search for.
         * @param fileInfo A reference to a FileInfo object to store the results in.
         * @param ignorePath If true, the search will ignore the file path component.
         * @return True if a FileInfo object was found for the specified file path, false otherwise.
         * @note This method searches through the file system's internal file registry.
         */
        virtual bool findFileInfo( const String &filePath, FileInfo &fileInfo,
                                   bool ignorePath = false ) const = 0;

        /**
         * @brief Gets a list of system files tracked by the file system.
         *
         * @return An array of FileInfo objects containing information about system files.
         * @note System files are typically core files that are always available.
         */
        virtual Array<FileInfo> getSystemFiles() const = 0;

        /**
         * @brief Updates the file system's internal file registry.
         *
         * This method refreshes the list of files stored in the file system's internal registry,
         * scanning all registered archives and paths for changes.
         *
         * @param async If true, the operation runs asynchronously; if false, it runs synchronously on
         * the caller thread.
         * @note Asynchronous operations may take some time to complete and will not block the calling
         * thread.
         */
        virtual void refreshAll( bool async ) = 0;

        /**
         * @brief Updates the file system's internal file registry for a specific path.
         *
         * This method refreshes the list of files stored in the file system's internal registry
         * for the specified path, scanning for changes in that location.
         *
         * @param path The path of the folder to update.
         * @param async If true, the operation runs asynchronously; if false, it runs synchronously on
         * the caller thread.
         * @note Asynchronous operations may take some time to complete and will not block the calling
         * thread.
         */
        virtual void refreshPath( const String &path, bool async ) = 0;

        /**
         * @brief Gets the file ID (UUID) for a file path.
         *
         * @param filePath The path of the file to get the ID for.
         * @return The UUID of the file, or an invalid UUID if the file is not found.
         * @note This method searches through the file system's internal file registry.
         */
        virtual UUID getFileId( const String &filePath ) const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
