#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/FileInfo.hpp>

namespace workphone
{
    auto FileInfo::operator==( const struct FileInfo &other ) const -> bool
    {
        if( isDirectory != other.isDirectory )
        {
            return false;
        }

        return filePath == other.filePath;
    }

    auto FileInfo::operator<( const struct FileInfo &other ) const -> bool
    {
        if( isDirectory != other.isDirectory )
        {
            return isDirectory;
        }

        return filePath < other.filePath;
    }
}  // namespace workphone
