#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/SystemBind.hpp"
#include "WPLuabind/SmartPtrConverter.hpp"
#include <Workphone/Workphone.hpp>
#include <luabind/luabind.hpp>

namespace workphone
{
    namespace
    {
        SmartPtr<IStream> archiveOpen( IArchive *archive, const String &filePath )
        {
            return archive->open( filePath );
        }

        bool archiveExists( IArchive *archive, const String &filePath )
        {
            return archive->exists( filePath );
        }

        FileInfo archiveFindFileInfo( IArchive *archive, const String &filePath, bool ignorePath,
                                      bool ignoreCase )
        {
            FileInfo fileInfo;
            archive->findFileInfo( filePath, fileInfo, ignorePath, ignoreCase );
            return fileInfo;
        }

        FileInfo fileListFindFileInfo( IFileList *fileList, const String &filePath, bool ignorePath,
                                       bool ignoreCase )
        {
            FileInfo fileInfo;
            fileList->findFileInfo( filePath, fileInfo, ignorePath, ignoreCase );
            return fileInfo;
        }

        Array<String> folderExplorerGetFiles( IFolderExplorer *folderExplorer )
        {
            return folderExplorer->getFiles();
        }

        String streamGetLine( IStream *stream )
        {
            return stream->getLine();
        }

        size_Num streamSkipLine( IStream *stream )
        {
            return stream->skipLine();
        }
    } // namespace

    void bindIO( lua_State *L )
    {
        using namespace luabind;

        module( L )[class_<IArchive, ISharedObject, SmartPtr<IArchive>>( "IArchive" )
                        .def( "open", archiveOpen )
                        .def( "openFull", &IArchive::open )
                        .def( "exists", archiveExists )
                        .def( "existsFull", &IArchive::exists )
                        .def( "getFileList", &IArchive::getFileList )
                        .def( "getFiles", &IArchive::getFiles )
                        .def( "getType", &IArchive::getType )
                        .def( "isReadOnly", &IArchive::isReadOnly )
                        .def( "getPassword", &IArchive::getPassword )
                        .def( "setPassword", &IArchive::setPassword )
                        .def( "getPath", &IArchive::getPath )
                        .def( "setPath", &IArchive::setPath )
                        .def( "findFileInfo", archiveFindFileInfo )
                        .scope[def( "typeInfo", IArchive::typeInfo )]];

        module( L )[class_<IFileList, ISharedObject, SmartPtr<IFileList>>( "IFileList" )
                        .def( "getNumFiles", &IFileList::getNumFiles )
                        .def( "getFileName", &IFileList::getFileName )
                        .def( "getFullFileName", &IFileList::getFullFileName )
                        .def( "getFileSize", &IFileList::getFileSize )
                        .def( "getFileOffset", &IFileList::getFileOffset )
                        .def( "findFile", &IFileList::findFile )
                        .def( "setPath", &IFileList::setPath )
                        .def( "getPath", &IFileList::getPath )
                        .def( "sort", &IFileList::sort )
                        .def( "setFiles", &IFileList::setFiles )
                        .def( "addFile", &IFileList::addFile )
                        .def( "removeFile", &IFileList::removeFile )
                        .def( "findFileInfo", fileListFindFileInfo )
                        .def( "exists", &IFileList::exists )
                        .scope[def( "typeInfo", IFileList::typeInfo )]];

        module( L )[class_<IFileListener, ISharedObject, SmartPtr<IFileListener>>( "IFileListener" )
                        .def( "fileAction", &IFileListener::fileAction )
                        .scope[def( "typeInfo", IFileListener::typeInfo )]];

        module(
            L )[class_<IFolderExplorer, ISharedObject, SmartPtr<IFolderExplorer>>( "IFolderExplorer" )
                    .def( "getFolderName", &IFolderExplorer::getFolderName )
                    .def( "setFolderName", &IFolderExplorer::setFolderName )
                    .def( "getFiles", folderExplorerGetFiles )
                    .def( "setFiles", &IFolderExplorer::setFiles )
                    .def( "getSubFolders", &IFolderExplorer::getSubFolders )
                    .def( "setSubFolders", &IFolderExplorer::setSubFolders )
                    .def( "addSubFolder", &IFolderExplorer::addSubFolder )
                    .scope[def( "typeInfo", IFolderExplorer::typeInfo )]];

        module(
            L )[class_<IStream, ISharedObject, SmartPtr<IStream>>( "IStream" )
                    .def( "isOpen", &IStream::isOpen )
                    .def( "close", &IStream::close )
                    .def( "eof", &IStream::eof )
                    .def( "getLine", streamGetLine )
                    .def( "getLineTrimmed", &IStream::getLine )
                    .def( "seek", &IStream::seek )
                    .def( "size", &IStream::size )
                    .def( "tell", &IStream::tell )
                    .def( "getFileName", &IStream::getFileName )
                    .def( "getAsString", &IStream::getAsString )
                    .def( "setFreeMemory", &IStream::setFreeMemory )
                    .def( "skipLine", streamSkipLine )
                    .def( "skipLineWithDelimiter", &IStream::skipLine )
                    .def( "skip", &IStream::skip )
                    .def( "isReadable", &IStream::isReadable )
                    .def( "isWriteable", &IStream::isWriteable )
                    .def( "getFileInfo", &IStream::getFileInfo )
                    .def( "setFileInfo", &IStream::setFileInfo )
                    .enum_(
                        "AccessMode" )[value( "Read", static_cast<int>( IStream::AccessMode::Read ) ),
                                       value( "Write", static_cast<int>( IStream::AccessMode::Write ) )]
                    .scope[def( "typeInfo", IStream::typeInfo )]];
    }
} // namespace workphone
