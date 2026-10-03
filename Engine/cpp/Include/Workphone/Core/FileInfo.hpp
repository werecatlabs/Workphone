#ifndef __FileInfo_h__
#define __FileInfo_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/UUID.hpp>

namespace workphone
{

    /**
     * @brief Represents metadata and properties of a file or directory in the filesystem or archive.
     *
     * This struct encapsulates information such as file paths, names, sizes, unique identifiers, and
     * archive-related data. It is used to describe both files and directories, supporting operations
     * like comparison and sorting.
     */
    struct WPCore_API FileInfo
    {
        /**
         * @brief The full file path, including directories and filename, as originally provided.
         */
        FixedString<WP_MAX_PATH> filePath;

        /**
         * @brief The full file path in lower case, useful for case-insensitive comparisons.
         */
        FixedString<WP_MAX_PATH> filePathLowerCase;

        /**
         * @brief The directory path, separated by '/', always ending with '/'.
         *
         * Example: "folder/subfolder/"
         */
        FixedString<WP_MAX_PATH> path;

        /**
         * @brief The absolute directory path, separated by '/', always ending with '/'.
         *
         * Example: "/home/user/folder/subfolder/"
         */
        FixedString<WP_MAX_PATH> absolutePath;

        /**
         * @brief The base filename (without path), including extension if present.
         *
         * Example: "file.txt"
         */
        FixedString<WP_MAX_FILENAME> fileName;

        /**
         * @brief The base filename in lower case, useful for case-insensitive operations.
         */
        FixedString<WP_MAX_FILENAME> fileNameLowerCase;

        /**
         * @brief The size of the file after compression, in bytes.
         *
         * If the file is not compressed, this may be equal to uncompressedSize.
         */
        size_t compressedSize = 0;

        /**
         * @brief The original size of the file before compression, in bytes.
         */
        size_t uncompressedSize = 0;

        /**
         * @brief Unique identifier for the file.
         */
        UUID fileId;

        /**
         * @brief Unique identifier for the archive containing the file, if applicable.
         *
         * If the file is not in an archive, this may be zero.
         */
        hash64 archiveId = 0;

        /**
         * @brief Offset of the file's data within the archive, in bytes.
         *
         * If the file is not in an archive, this may be zero.
         */
        u32 offset = 0;

        /**
         * @brief Indicates whether this FileInfo represents a directory (true) or a file (false).
         */
        bool isDirectory = false;

        /**
         * @brief Equality operator for FileInfo.
         * @param other The FileInfo instance to compare with.
         * @return True if all relevant fields are equal, false otherwise.
         */
        bool operator==( const struct FileInfo &other ) const;

        /**
         * @brief Less-than operator for FileInfo, for sorting purposes.
         * @param other The FileInfo instance to compare with.
         * @return True if this instance is considered less than the other, false otherwise.
         */
        bool operator<( const struct FileInfo &other ) const;
    };
}  // namespace workphone

#endif  // __FileInfo_h__
