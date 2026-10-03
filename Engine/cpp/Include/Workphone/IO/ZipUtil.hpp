#ifndef __ZipUtil_h__
#define __ZipUtil_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    /**
     * @brief Utility class for working with ZIP archives.
     *
     * The `ZipUtil` class provides a small set of static helper functions for
     * extracting ZIP archives and creating (optionally obfuscated) ZIP files
     * from lists of filesystem paths. Creation functions return an error code
     * (0 on success) while extraction is performed via the void `extractZip`
     * method which will typically throw or log errors through the project's
     * error handling when extraction fails.
     *
     * The class also contains low-level helper wrappers around compression
     * and decompression routines (implemented using `def` and `inf`).
     */
    class WPCore_API ZipUtil
    {
    public:
        /**
         * @brief Extracts a ZIP archive to a destination directory.
         *
         * Extracts the contents of the ZIP file located at `src` into the
         * directory specified by `dst`. The function will recreate directory
         * structure and write files into `dst`.
         *
         * @param src Path to the source ZIP archive.
         * @param dst Path to the destination directory where files will be
         *            extracted.
         */
        static void extractZip( const StringW &src, const StringW &dst );

        /**
         * @brief Create a ZIP archive from a list of files.
         *
         * Adds the specified `paths` to a ZIP archive written to
         * `destinationPath`.
         *
         * @param destinationPath Path where the resulting ZIP file will be
         *                        written.
         * @param paths Array of file paths to include in the archive.
         * @return 0 on success, otherwise an error code.
         */
        static s32 createZipFileFromPath( const StringW &destinationPath, const Array<StringW> &paths );

        /**
         * @brief Create a ZIP archive using an explicit zip path and list of
         * files.
         *
         * Similar to the other `createZipFileFromPath` overload but allows
         * passing an explicit `zipPath` in addition to the destination. The
         * exact semantics of `zipPath` depend on the implementation (for
         * example it might be used as a virtual path inside the archive).
         *
         * @param destinationPath Path where the resulting ZIP file will be
         *                        written.
         * @param zipPath Optional path used when adding files into the
         *                archive (implementation-defined semantics).
         * @param paths Array of file paths to include in the archive.
         * @return 0 on success, otherwise an error code.
         */
        static s32 createZipFileFromPath( const StringW &destinationPath, const StringW &zipPath,
                                          const Array<StringW> &paths );

        /**
         * @brief Create a ZIP archive with control over stored paths.
         *
         * This overload allows specifying a `rootDir` which can be removed
         * from the stored entry names. If `useRelativePaths` is true, file
         * paths will be stored relative to `rootDir` where applicable.
         *
         * @param destinationPath Path where the resulting ZIP file will be
         *                        written.
         * @param paths Array of file paths to include in the archive.
         * @param rootDir Root directory to strip from stored entry names.
         * @param useRelativePaths If true, store paths relative to `rootDir`.
         * @return 0 on success, otherwise an error code.
         */
        static int createZipFileFromPath( const StringW &destinationPath, const Array<StringW> &paths,
                                          const StringW &rootDir, bool useRelativePaths );

        /**
         * @brief Create an obfuscated ZIP archive from a list of files.
         *
         * Produces a ZIP file where file contents or entry names are
         * obfuscated according to the project's obfuscation scheme. The
         * resulting archive is written to `destinationPath`.
         *
         * @param destinationPath Path where the resulting obfuscated ZIP
         *                        file will be written.
         * @param paths Array of file paths to include (and obfuscate) in the
         *              archive.
         * @return 0 on success, otherwise an error code.
         */
        static s32 createObfuscatedZipFileFromPath( const StringW &destinationPath,
                                                    const Array<StringW> &paths );

        /**
         * @brief Create an obfuscated ZIP archive using an explicit zip path.
         *
         * Overload of `createObfuscatedZipFileFromPath` that accepts an
         * additional `zipPath` parameter. See the non-obfuscated overload
         * for parameter semantics.
         *
         * @param destinationPath Path where the resulting obfuscated ZIP
         *                        file will be written.
         * @param zipPath Optional path used when adding files into the
         *                archive (implementation-defined semantics).
         * @param paths Array of file paths to include (and obfuscate) in the
         *              archive.
         * @return 0 on success, otherwise an error code.
         */
        static s32 createObfuscatedZipFileFromPath( const StringW &destinationPath,
                                                    const StringW &zipPath,
                                                    const Array<StringW> &paths );

        /**
         * @brief Create an obfuscated ZIP archive with control over stored
         * paths.
         *
         * Similar to the non-obfuscated overload but applies the project's
         * obfuscation to stored data or entry names. Parameters control how
         * paths are stored inside the archive.
         *
         * @param destinationPath Path where the resulting obfuscated ZIP
         *                        file will be written.
         * @param paths Array of file paths to include (and obfuscate) in the
         *              archive.
         * @param rootDir Root directory to strip from stored entry names.
         * @param useRelativePaths If true, store paths relative to `rootDir`.
         * @return 0 on success, otherwise an error code.
         */
        static int createObfuscatedZipFileFromPath( const StringW &destinationPath,
                                                    const Array<StringW> &paths, const StringW &rootDir,
                                                    bool useRelativePaths );

    protected:
        /**
         * @brief Low-level helper that performs deflate (compression).
         *
         * Wraps the underlying compression routine used by the create helpers.
         * It reads data from `source`, compresses it using the provided
         * `level` and writes compressed data to `dest`.
         *
         * @param source Input FILE* to read uncompressed data from.
         * @param dest Output FILE* to write compressed data to.
         * @param level Compression level (typically 0-9 where higher values
         *              give better compression at the cost of speed).
         * @return 0 on success, otherwise an error code.
         */
        static s32 def( FILE *source, FILE *dest, int level );

        /**
         * @brief Low-level helper that performs inflate (decompression).
         *
         * Reads compressed data from `source`, decompresses it and writes
         * the resulting uncompressed data to `dest`.
         *
         * @param source Input FILE* to read compressed data from.
         * @param dest Output FILE* to write decompressed data to.
         * @return 0 on success, otherwise an error code.
         */
        static s32 inf( FILE *source, FILE *dest );

        /**
         * @brief Compression level used by the create helpers.
         *
         * Valid range is typically 0-9 where a higher number increases
         * compression ratio at the cost of CPU time. The exact default value
         * is implementation-defined.
         */
        static int m_compressionLevel;
    };
}  // namespace workphone

#endif  // ZipUtil_h__
