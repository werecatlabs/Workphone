#ifndef CFileList_h__
#define CFileList_h__

#include <Workphone/Interface/IO/IFileList.hpp>
#include <Workphone/Atomics/AtomicObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{

    /** Implementation of a file list */
    class WPCore_API FileList : public IFileList
    {
    public:
        static const String emptyFileListEntry;

        /** Default constructor. */
        FileList();

        //! Constructor
        /** @param path The path of this file archive */
        FileList( const String &path );

        //! Destructor
        ~FileList() override;

        /** @copydoc ISharedObject::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IFileList::sort */
        void sort() override;

        /** @copydoc IFileList::getNumFiles */
        u32 getNumFiles() const override;

        /** @copydoc IFileList::getFileName */
        String getFileName( u32 index ) const override;

        /** @copydoc IFileList::getFullFileName */
        String getFullFileName( u32 index ) const override;

        /** @copydoc IFileList::getFileSize */
        u32 getFileSize( u32 index ) const override;

        /** @copydoc IFileList::getFileOffset */
        u32 getFileOffset( u32 index ) const override;

        /** @copydoc IFileList::findFile */
        s32 findFile( const String &filename, bool isFolder ) const override;

        /** @copydoc IFileList::setPath */
        void setPath( const String &path ) override;

        /** @copydoc IFileList::getPath */
        String getPath() const override;

        /** @copydoc IFileList::getFiles */
        ConcurrentArray<FileInfo> &getFiles() override;

        /** @copydoc IFileList::getFiles */
        const ConcurrentArray<FileInfo> &getFiles() const override;

        /** @copydoc IFileList::setFiles */
        void setFiles( const Array<FileInfo> &files ) override;

        /** @copydoc IFileList::findFileInfo */
        bool findFileInfo( UUID id, FileInfo &fileInfo, bool ignorePath = false,
                           bool ignoreCase = false ) const override;

        /** @copydoc IFileList::findFileInfo */
        bool findFileInfo( const String &filePath, FileInfo &fileInfo, bool ignorePath = false,
                           bool ignoreCase = false ) const override;

        /** @copydoc IFileList::exists */
        bool exists( const String &filePath, bool ignorePath = false,
                     bool ignoreCase = false ) const override;

        /** @copydoc IFileList::removeFilePtr */
        void addFile( const FileInfo &file ) override;

        /** @copydoc IFileList::removeFilePtr */
        void removeFile( const FileInfo &file ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// An array of files in the file system.
        ConcurrentArray<FileInfo> m_files;

        /// Path to the file list
        AtomicObject<FixedString<WP_MAX_PATH>> m_path;
    };
}  // namespace workphone

#endif
