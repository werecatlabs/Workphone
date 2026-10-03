#ifndef WPOgreArchive_h__
#define WPOgreArchive_h__

#include "OgreArchive.h"

namespace workphone
{
    /**
     * @brief Adapter implementing Ogre::Archive for the project's virtual/engine resource layer.
     *
     * This class exposes an archive-like interface that Ogre expects while delegating
     * to the engine's resource/file system. It implements the subset of operations
     * Ogre uses to enumerate, query and open resources.
     *
     * @note Implementations of the methods must return Ogre-managed smart pointers
     *       (e.g. `Ogre::DataStreamPtr`, `Ogre::StringVectorPtr`, `Ogre::FileInfoListPtr`)
     *       as required by the Ogre API.
     *
     * @see Ogre::Archive
     */
    class WPOgreArchive : public Ogre::Archive
    {
    public:
        /**
         * @brief Construct an `WPOgreArchive`.
         *
         * @param name Human / engine visible name for the archive instance. Typically
         *        used by Ogre to identify the archive in the resource system.
         * @param archType A short string describing the archive type (for example
         *        "FileSystem", "Zip", or a project-specific type). Used by Ogre when
         *        reporting or working with multiple archive types.
         */
        WPOgreArchive( const Ogre::String &name, const Ogre::String &archType );

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup in derived or aggregated resource-management code.
         */
        ~WPOgreArchive() override;

        /**
         * @brief Load the archive.
         *
         * Called by Ogre when the archive should prepare any internal state required
         * for subsequent operations. This may be a no-op for some implementations.
         *
         * @note Should be paired with `unload()` by the caller.
         */
        void load() override;

        /**
         * @brief Unload the archive.
         *
         * Release any resources / caches acquired in `load()`. After `unload()` is
         * called the archive may be loaded again later via `load()`.
         */
        void unload() override;

        /**
         * @brief Open a data stream for `filename` in read-only (default) or read/write mode.
         *
         * This const overload allows callers with a const reference to the archive to
         * open streams. Behavioural details (for example whether write access is
         * permitted) depend on the concrete implementation.
         *
         * @param filename Path of the file inside the archive to open. Path semantics
         *        (slashes, absolute vs relative) follow the engine's archive conventions.
         * @param readOnly When true (default) open for read-only access. When false
         *        the returned stream may allow writing if the backend supports it.
         * @return `Ogre::DataStreamPtr` pointing to the opened stream. Returns an
         *         empty/null pointer when the file does not exist or cannot be opened.
         */
        Ogre::DataStreamPtr open( const Ogre::String &filename, bool readOnly = true ) const;

        /**
         * @brief Open a data stream for `filename` in read-only (default) or read/write mode.
         *
         * Non-const overload provided for callers that require a mutable `Archive`.
         *
         * @param filename Path of the file inside the archive to open.
         * @param readOnly When true (default) open for read-only access.
         * @return `Ogre::DataStreamPtr` to the opened stream or null on failure.
         */
        Ogre::DataStreamPtr open( const Ogre::String &filename, bool readOnly = true ) override;

        /**
         * @brief List names of entries inside the archive.
         *
         * @param recursive If true list entries in subdirectories recursively.
         * @param dirs If true include directory entries in the returned list;
         *        otherwise only file entries are returned.
         * @return `Ogre::StringVectorPtr` containing the matched entry names.
         */
        Ogre::StringVectorPtr list( bool recursive = true, bool dirs = false ) override;

        /**
         * @brief List file information entries inside the archive.
         *
         * Similar to `list()` but returns richer information (`Ogre::FileInfo`) such
         * as file sizes and timestamps.
         *
         * @param recursive If true list entries recursively.
         * @param dirs If true include directories in the returned list.
         * @return `Ogre::FileInfoListPtr` containing file info entries.
         */
        Ogre::FileInfoListPtr listFileInfo( bool recursive = true, bool dirs = false ) override;

        /**
         * @brief Find entries matching a pattern.
         *
         * The `pattern` parameter typically uses glob-style matching (e.g. `*.png`).
         * Exact supported pattern syntax depends on the concrete implementation.
         *
         * @param pattern Pattern to match against entry names.
         * @param recursive If true search subdirectories recursively.
         * @param dirs If true include directories in the matches.
         * @return `Ogre::StringVectorPtr` with paths that matched `pattern`.
         */
        Ogre::StringVectorPtr find( const Ogre::String &pattern, bool recursive = true,
                                    bool dirs = false ) override;

        /**
         * @brief Test whether `filename` exists in the archive.
         *
         * @param filename File path to test.
         * @return true if the entry exists, false otherwise.
         */
        bool exists( const Ogre::String &filename ) override;

        /**
         * @brief Find `Ogre::FileInfo` entries matching `pattern`.
         *
         * Non-const overload: same behaviour as the const overload.
         *
         * @param pattern Pattern to match.
         * @param recursive If true include subdirectories.
         * @param dirs If true include directories in the results.
         * @return `Ogre::FileInfoListPtr` with matching file info entries.
         */
        Ogre::FileInfoListPtr findFileInfo( const Ogre::String &pattern, bool recursive = true,
                                            bool dirs = false ) override;

        /**
         * @brief Return last modification time for `filename`.
         *
         * @param filename File path inside the archive.
         * @return Last modification timestamp as `time_t` (epoch). If the time is
         *         unknown this function may return 0; behaviour is implementation-dependent.
         */
        time_t getModifiedTime( const Ogre::String &filename ) override;

        /**
         * @brief Whether file name comparisons for this archive are case sensitive.
         *
         * This flag indicates whether APIs such as `exists()` or `open()` should treat
         * `Foo.txt` and `foo.txt` as the same entry. Typical filesystems on Windows are
         * case-insensitive while Unix-like systems are case-sensitive.
         *
         * @return true if the archive treats names as case sensitive, false otherwise.
         */
        bool isCaseSensitive() const override;

        void lock();
        void lock_shared();

        void unlock();
        void unlock_shared();
    };
}  // namespace workphone
#endif  // WPOgreArchive_h__
