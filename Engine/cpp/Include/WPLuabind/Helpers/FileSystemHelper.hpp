#ifndef FileSystemHelper_h__
#define FileSystemHelper_h__

#include <Workphone/Interface/IO/IFileSystem.hpp>

namespace workphone
{
    class FileSystemHelper
    {
    public:
        static Array<String> _getFileNamesInFolder( IFileSystem *system, const char *folderName );

        static Array<String> _getFileNamesWithExt( IFileSystem *system, const char *extension );
    };
} // namespace workphone

#endif // FileSystemHelper_h__
