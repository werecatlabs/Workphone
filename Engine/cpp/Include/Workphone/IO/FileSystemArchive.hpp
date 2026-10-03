#ifndef CFileSystemArchive_h__
#define CFileSystemArchive_h__

#include <Workphone/Interface/IO/IArchive.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/IO/FileList.hpp>

namespace workphone
{

    /**
     * \brief Archive implementation that exposes the native file system.
     *
     * \details
     * `FileSystemArchive` implements `IArchive` to provide access to files
     * stored on the host file system. It maintains an optional `FileList`
     * describing the files discovered under the configured path and exposes
     * convenience methods for opening streams, querying file information and
     * performing thread-safe operations on the archive.
     *
     * The class supports case-insensitive lookups and path-ignorant searches
     * depending on the configuration passed to the constructor or individual
     * query calls.
     */
    class WPCore_API FileSystemArchive : public IArchive
    {
    public:
        /**
         * \brief Default constructor.
         *
         * Creates an empty, uninitialized archive. Call `setPath` and/or
         * `setFileList` before using the archive, or use the path-taking
         * constructor to initialize immediately.
         */
        FileSystemArchive();

        /**
         * \brief Construct an archive rooted at `path`.
         *
         * \param path The root directory on the native file system to expose
         *             through this archive.
         * \param ignoreCase If true, file lookups will ignore character case.
         * \param ignorePaths If true, lookups will ignore intermediate
         *                    path components and search by filename only.
         */
        FileSystemArchive( const String &path, bool ignoreCase, bool ignorePaths );

        /**
         * \brief Destructor.
         *
         * Releases any held resources. If streams opened from this archive are
         * still alive they are expected to keep working independently of the
         * archive lifetime.
         */
        ~FileSystemArchive() override;

        /**
         * \copydoc IArchive::load
         *
         * Loads archive metadata from `data`. Typical implementations discover
         * files under the configured path and populate the internal file list.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * \brief Reload archive metadata.
         *
         * Re-scans or refreshes internal state using `data`. This complements
         * `load` and can be used to update the archive after external changes.
         */
        void reload( SmartPtr<ISharedObject> data ) override;

        /**
         * \copydoc IArchive::unload
         *
         * Clears any cached metadata and releases resources associated with
         * this archive.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * \copydoc IArchive::getFileList
         *
         * Returns the `IFileList` instance describing files known to this
         * archive. The returned smart pointer may be null if no file list has
         * been set or discovered.
         */
        SmartPtr<IFileList> getFileList() const override;

        /**
         * \brief Replace the internal file list.
         *
         * This allows callers to provide a precomputed `FileList` instead of
         * relying on on-demand discovery.
         */
        void setFileList( SmartPtr<IFileList> fileList );

        /**
         * \copydoc IArchive::getFiles
         *
         * Returns a copy of the currently known file information entries.
         */
        Array<FileInfo> getFiles() const override;

        /**
         * \copydoc IArchive::getType
         *
         * Returns the archive type identifier for file system archives.
         */
        u8 getType() const override;

        /**
         * \copydoc IArchive::getPassword
         *
         * Returns the password used for protected archives. For file system
         * archives this is typically empty but the property is provided for
         * API compatibility.
         */
        String getPassword() const override;

        /**
         * \copydoc IArchive::setPassword
         *
         * Sets the password used when opening protected resources. No
         * validation is performed by this class - callers are responsible for
         * using the password where required.
         */
        void setPassword( const String &password ) override;

        /**
         * \copydoc IArchive::open
         *
         * \param filePath Path to the file relative to the archive root.
         * \param input If true, open for reading; otherwise open for writing.
         * \param binary If true, open in binary mode; otherwise text mode.
         * \param truncate If true and opening for writing, truncate the file.
         * \param ignorePath If true, ignore directory components when
         *                   searching for the file.
         * \param ignoreCase If true, perform a case-insensitive lookup.
         */
        SmartPtr<IStream> open( const String &filePath, bool input = true, bool binary = false,
                                bool truncate = false, bool ignorePath = false,
                                bool ignoreCase = false ) override;

        /**
         * \copydoc IArchive::exists
         *
         * Checks whether `filePath` refers to a file known to this archive.
         */
        bool exists( const String &filePath, bool ignorePath = false,
                     bool ignoreCase = false ) const override;

        /**
         * \copydoc IArchive::isReadOnly
         *
         * For a file system archive this typically returns false unless the
         * underlying path is not writable by the process.
         */
        bool isReadOnly() const override;

        /**
         * \copydoc IArchive::getPath
         *
         * Returns the configured root path for this archive.
         */
        String getPath() const override;

        /**
         * \copydoc IArchive::setPath
         *
         * Sets the root path that this archive will expose. Changing the
         * path may invalidate cached `FileList` data.
         */
        void setPath( const String &path ) override;

        /**
         * \copydoc IArchive::findFileInfo
         *
         * Finds file metadata by UUID. Returns true and fills `fileInfo` on
         * success.
         */
        bool findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath = false,
                           bool ignoreCase = false ) const override;

        /**
         * \copydoc IArchive::findFileInfo
         *
         * Finds file metadata by path. If `ignorePath` is true the search
         * will match filenames regardless of directory components.
         */
        bool findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath = false,
                           bool ignoreCase = false ) const override;

        /**
         * \copydoc IArchive::lock
         *
         * Acquire the archive's mutex to perform thread-safe operations.
         */
        void lock() override;

        /**
         * \copydoc IArchive::try_lock
         *
         * Attempt to acquire the archive lock without blocking. Returns true
         * if the lock was obtained.
         */
        bool try_lock() override;

        /**
         * \copydoc IArchive::unlock
         *
         * Release the archive lock previously acquired with `lock` or
         * `try_lock`.
         */
        void unlock() override;

        WP_CLASS_REGISTER_DECL;

    private:
        void addFile( const FileInfo &file );

        /// The password.
        AtomicObject<FixedString<128>> m_password;

        /// The file system path.
        AtomicObject<FixedString<WP_MAX_PATH>> m_path;

        /// The file list.
        AtomicSmartPtr<FileList> m_fileList;
    };
}  // namespace workphone

#endif  // CFileSystemArchive_h__
