#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Helpers/FileSystemHelper.hpp"

namespace workphone
{
    Array<String> FileSystemHelper::_getFileNamesInFolder( IFileSystem *system, const char *folderName )
    {
        Array<String> fileNames;
        system->getFileNamesInFolder( folderName, fileNames );
        return fileNames;
    }

    Array<String> FileSystemHelper::_getFileNamesWithExt( IFileSystem *system, const char *extension )
    {
        Array<String> fileNames;
        system->getFileNamesWithExtension( extension, fileNames );
        return fileNames;
    }
} // namespace workphone
