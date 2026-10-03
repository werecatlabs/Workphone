#ifndef __WP_BasePath_h__
#define __WP_BasePath_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{

    /**
     * @brief A utility class for file system path operations and manipulations.
     *
     * This class provides a comprehensive set of static methods for handling file system paths,
     * including path manipulation, file operations, directory management, and path normalization.
     * It is designed to work with different character types through template specialization.
     *
     * @tparam CharT The character type used for file paths (e.g., char, wchar_t).
     *               Must support basic character operations and be compatible with std::basic_string.
     * @tparam Traits Character traits class (default: std::char_traits<CharT>).
     * @tparam Allocator Allocator type for string operations (default: std::allocator<CharT>).
     */
    template <class T>
    class BasePath
    {
    public:
        /**
         * @brief The platform-specific path separator string.
         *
         * This is used for constructing and parsing file paths in a platform-independent manner.
         */
        static WPCore_API const BaseString<T> separatorBaseString;

        /**
         * @brief Extracts the directory portion from a file path.
         *
         * @param filePath The full file path to process.
         * @return string_type The directory path without the filename.
         * @example
         *   getFilePath("C:/folder/file.txt") returns "C:/folder/"
         */
        static BaseString<T> WPCore_API getFilePath( const BaseString<T> &filePath );

        /**
         * @brief Extracts the filename with extension from a file path.
         *
         * @param filePath The full file path to process.
         * @return string_type The filename including its extension.
         * @example
         *   getFileName("C:/folder/file.txt") returns "file.txt"
         */
        static BaseString<T> WPCore_API getFileName( const BaseString<T> &filePath );

        /**
         * @brief Extracts the file extension from a file path.
         *
         * @param filePath The full file path to process.
         * @return string_type The file extension including the dot (e.g., ".txt").
         * @example
         *   getFileExtension("C:/folder/file.txt") returns ".txt"
         */
        static BaseString<T> WPCore_API getFileExtension( const BaseString<T> &filePath );

        /**
         * @brief Retrieves the filename without its extension.
         *
         * @param path The full file path to process.
         * @return string_type The filename without the extension.
         * @example
         *   getFileNameWithoutExtension("C:/folder/file.txt") returns "file"
         */
        static BaseString<T> WPCore_API getFileNameWithoutExtension( const BaseString<T> &path );

        /**
         * @brief Retrieves the full path without the file extension.
         *
         * @param path The full file path to process.
         * @return string_type The path without the file extension.
         * @example
         *   getFilePathWithoutExtension("C:/folder/file.txt") returns "C:/folder/file"
         */
        static BaseString<T> WPCore_API getFilePathWithoutExtension( const BaseString<T> &path );

        /**
         * @brief Sets the current working directory for the application.
         *
         * @param dir The directory path to set as the working directory.
         * @throw std::runtime_error If the directory cannot be set.
         */
        static void WPCore_API setWorkingDirectory( const BaseString<T> &dir );

        /**
         * @brief Gets the current working directory.
         *
         * @return string_type The absolute path of the current working directory.
         */
        static BaseString<T> WPCore_API getWorkingDirectory();

        /**
         * @brief Retrieves a list of folders in a given directory.
         *
         * @param path The directory path to search in.
         * @param recursive If true, includes subdirectories in the search.
         * @return Array<string_type> Array of folder paths found.
         */
        static Array<BaseString<T>> WPCore_API getFolders( const BaseString<T> &path,
                                                           bool recursive = false );

        /**
         * @brief Retrieves all paths (files and folders) in a given directory.
         *
         * @param path The directory path to search in.
         * @param recursive If true, includes subdirectories in the search.
         * @return Array<string_type> Array of all paths found.
         */
        static Array<BaseString<T>> WPCore_API getPaths( const BaseString<T> &path,
                                                         bool recursive = false );

        /**
         * @brief Retrieves all files in a given directory.
         *
         * @param path The directory path to search in.
         * @return Array<string_type> Array of file paths found.
         */
        static Array<BaseString<T>> WPCore_API getFiles( const BaseString<T> &path );

        /**
         * @brief Retrieves all files in a directory as absolute paths.
         *
         * @param path The directory path to search in.
         * @param recursive If true, includes subdirectories in the search.
         * @return Array<string_type> Array of absolute file paths found.
         */
        static Array<BaseString<T>> WPCore_API getFilesAsAbsolutePaths( const BaseString<T> &path,
                                                                        bool recursive = false );

        /**
         * @brief Retrieves files with a specific extension in a directory.
         *
         * @param path The directory path to search in.
         * @param extension The file extension to filter by (e.g., ".txt").
         * @return Array<string_type> Array of matching file paths.
         */
        static Array<BaseString<T>> WPCore_API getFiles( const BaseString<T> &path,
                                                         const BaseString<T> &extension );

        /**
         * @brief Retrieves files with a specific extension as absolute paths.
         *
         * @param path The directory path to search in.
         * @param extension The file extension to filter by (e.g., ".txt").
         * @param recursive If true, includes subdirectories in the search.
         * @return Array<string_type> Array of absolute paths for matching files.
         */
        static Array<BaseString<T>> WPCore_API getFilesAsAbsolutePaths( const BaseString<T> &path,
                                                                        const BaseString<T> &extension,
                                                                        bool recursive = false );

        /**
         * @brief Gets the last component of a path (filename or directory name).
         *
         * @param path The path to process.
         * @return string_type The last component of the path.
         * @example
         *   getLeaf("C:/folder/file.txt") returns "file.txt"
         *   getLeaf("C:/folder/subfolder") returns "subfolder"
         */
        static BaseString<T> WPCore_API getLeaf( const BaseString<T> &path );

        /**
         * @brief Checks if a path refers to a directory.
         *
         * @param path The path to check.
         * @return bool True if the path is a directory, false otherwise.
         */
        static bool WPCore_API isFolder( const BaseString<T> &path );

        /**
         * @brief Checks if a path refers to a file.
         *
         * @param filePath The path to check.
         * @return bool True if the path is a file, false otherwise.
         */
        static bool WPCore_API isFile( const BaseString<T> &filePath );

        /**
         * @brief Checks if a directory exists.
         *
         * @param path The directory path to check.
         * @return bool True if the directory exists, false otherwise.
         */
        static bool WPCore_API isExistingFolder( const BaseString<T> &path );

        /**
         * @brief Checks if a file exists.
         *
         * @param filePath The file path to check.
         * @return bool True if the file exists, false otherwise.
         */
        static bool WPCore_API isExistingFile( const BaseString<T> &filePath );

        /**
         * @brief Gets the folder hierarchy from a file path.
         *
         * @param filePath The full file path to process.
         * @return Array<string_type> Array of folder names in the path hierarchy.
         * @example
         *   getFilePathFolders("C:/folder/subfolder/file.txt") returns ["C:", "folder", "subfolder"]
         */
        static Array<BaseString<T>> WPCore_API getFilePathFolders( const BaseString<T> &filePath );

        /**
         * @brief Compares two files for equality.
         *
         * @param lFilePath Path to the first file.
         * @param rFilePath Path to the second file.
         * @return bool True if the files are identical, false otherwise.
         */
        static bool WPCore_API isFilesEqual( const BaseString<T> &lFilePath,
                                             const BaseString<T> &rFilePath );

        /**
         * @brief Gets the size of a file in bytes.
         *
         * @param filename The path to the file.
         * @return size_t The size of the file in bytes.
         * @throw std::runtime_error If the file cannot be accessed.
         */
        static size_t WPCore_API getFileSize( const BaseString<T> &filename );

        /**
         * @brief Copies a folder and its contents to a new location.
         *
         * @param srcPath The source folder path.
         * @param dstPath The destination folder path.
         * @throw std::runtime_error If the copy operation fails.
         */
        static void WPCore_API copyFolder( const BaseString<T> &srcPath, const BaseString<T> &dstPath );

        /**
         * @brief Copies a file to a new location.
         *
         * @param srcPath The source file path.
         * @param dstPath The destination file path.
         * @throw std::runtime_error If the copy operation fails.
         */
        static void WPCore_API copyFile( const BaseString<T> &srcPath, const BaseString<T> &dstPath );

        /**
         * @brief Creates all directories in the specified path.
         *
         * @param dst The directory path to create.
         * @throw std::runtime_error If directory creation fails.
         */
        static void WPCore_API createDirectories( const BaseString<T> &dst );

        /**
         * @brief Computes the MD5 hash of a file.
         *
         * @param filePath The path to the file.
         * @return BaseString<CharT>ing The MD5 hash as a hexadecimal string.
         * @throw std::runtime_error If the file cannot be read.
         */
        static BaseString<T> WPCore_API getMD5( const BaseString<T> &filePath );

        /**
         * @brief Computes the MD5 hash of a data buffer.
         *
         * @param data Pointer to the data buffer.
         * @param size The size of the data buffer in bytes.
         * @return BaseString<CharT>ing The MD5 hash as a hexadecimal string.
         */
        static BaseString<T> WPCore_API getMD5( void *data, size_t size );

        /**
         * @brief Deletes a folder and all its contents.
         *
         * @param path The folder path to delete.
         * @throw std::runtime_error If deletion fails.
         */
        static void WPCore_API deleteFolder( const BaseString<T> &path );

        /**
         * @brief Deletes a file.
         *
         * @param filePath The file path to delete.
         * @throw std::runtime_error If deletion fails.
         */
        static void WPCore_API deleteFile( const BaseString<T> &filePath );

        /**
         * @brief Deletes all files in a directory.
         *
         * @param path The directory path.
         * @throw std::runtime_error If deletion fails.
         */
        static void WPCore_API deleteFiles( const BaseString<T> &path );

        /**
         * @brief Renames a file or folder.
         *
         * @param pathOld The current path.
         * @param pathNew The new path.
         * @throw std::runtime_error If renaming fails.
         */
        static void WPCore_API rename( const BaseString<T> &pathOld, const BaseString<T> &pathNew );

        /**
         * @brief Converts a path to its absolute form.
         *
         * @param path The path to convert.
         * @return string_type The absolute path.
         */
        static BaseString<T> WPCore_API getAbsolutePath( const BaseString<T> &path );

        /**
         * @brief Converts a path to its absolute form relative to a base path.
         *
         * @param path The path to convert.
         * @param relativePath The base path to use for resolution.
         * @return string_type The absolute path.
         */
        static BaseString<T> WPCore_API getAbsolutePath( const BaseString<T> &path,
                                                         const BaseString<T> &relativePath );

        /**
         * @brief Converts a path to its relative form.
         *
         * @param path The path to convert.
         * @param relativePath The base path to use for resolution.
         * @return string_type The relative path.
         */
        static BaseString<T> WPCore_API getRelativePath( const BaseString<T> &path,
                                                         const BaseString<T> &relativePath );

        /**
         * @brief Normalizes a path by removing redundant elements.
         *
         * @param path The path to normalize.
         * @return string_type The normalized path.
         * @example
         *   lexically_normal("C:/folder/../file.txt") returns "C:/file.txt"
         */
        static BaseString<T> WPCore_API lexically_normal( const BaseString<T> &path );

        /**
         * @brief Normalizes a path relative to another path.
         *
         * @param path The path to normalize.
         * @param relativePath The base path to use for normalization.
         * @return string_type The normalized path.
         */
        static BaseString<T> WPCore_API lexically_normal( const BaseString<T> &path,
                                                          const BaseString<T> &relativePath );

        /**
         * @brief Computes the relative path from one directory to another.
         *
         * @param path The base path.
         * @param relativePath The target path.
         * @return string_type The computed relative path.
         */
        static BaseString<T> WPCore_API lexically_relative( const BaseString<T> &path,
                                                            const BaseString<T> &relativePath );

        /**
         * @brief Computes the shortest relative path between two directories.
         *
         * @param path The base path.
         * @param relativePath The target path.
         * @return string_type The computed proximate path.
         */
        static BaseString<T> WPCore_API lexically_proximate( const BaseString<T> &path,
                                                             const BaseString<T> &relativePath );

        /**
         * @brief Gets the path to the Mac application bundle.
         *
         * @return string_type The path to the Mac application bundle.
         * @note This is only relevant on macOS platforms.
         */
        static BaseString<T> WPCore_API macBundlePath();

        /**
         * @brief Checks if a path is absolute.
         *
         * @param basePath The path to check.
         * @return bool True if the path is absolute, false otherwise.
         */
        static bool WPCore_API isPathAbsolute( const BaseString<T> &basePath );

        /**
         * @brief Checks if a path is relative to another path.
         *
         * @param basePath The base path.
         * @param relativePath The path to check.
         * @return bool True if the path is relative, false otherwise.
         */
        static bool WPCore_API isPathRelative( const BaseString<T> &basePath,
                                               const BaseString<T> &relativePath );

        /**
         * @brief Checks if a path contains a filename.
         *
         * @param path The path to check.
         * @return bool True if the path contains a filename, false otherwise.
         */
        static bool WPCore_API hasFileName( const BaseString<T> &path );

        /**
         * @brief Reads all text from a file.
         *
         * @param path The path to the file.
         * @return string_type The contents of the file.
         * @throw std::runtime_error If the file cannot be read.
         */
        static BaseString<T> WPCore_API readAllText( const BaseString<T> &path );

        /**
         * @brief Writes text to a file.
         *
         * @param path The path to the file.
         * @param contents The text to write.
         * @throw std::runtime_error If the file cannot be written.
         */
        static void WPCore_API writeAllText( const BaseString<T> &path, const BaseString<T> &contents );

        /**
         * @brief Checks if a path ends with a specific string.
         *
         * @param path The path to check.
         * @param ending The string to check for at the end.
         * @return bool True if the path ends with the specified string, false otherwise.
         */
        static bool endsWith( const BaseString<T> &path, const BaseString<T> &ending );

        /**
         * @brief Gets the parent directory of a path.
         *
         * @param path The path to process.
         * @return string_type The parent directory path.
         * @example
         *   getParentPath("C:/folder/file.txt") returns "C:/folder"
         */
        static BaseString<T> WPCore_API getParentPath( const BaseString<T> &path );
    };

    /** @brief BasePath specialization for char (equivalent to old BaseString<CharT>ing-based Path). */
    using Path = BasePath<char>;

    /** @brief BasePath specialization for wchar_t (equivalent to old BaseString<CharT>ingW-based PathW).
     */
    using PathW = BasePath<wchar_t>;

}  // namespace workphone

#endif  // __WP_BasePath_h__
