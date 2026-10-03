#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/FileSelection.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( editor, FileSelection, ISharedObject );

    FileSelection::FileSelection() = default;

    FileSelection::~FileSelection() = default;

    auto FileSelection::getFilePath() const -> String
    {
        return m_filePath;
    }

    void FileSelection::setFilePath( const String &filePath )
    {
        m_filePath = StringUtil::cleanupPath( filePath );
    }

    auto FileSelection::getFileInfo() const -> FileInfo
    {
        return m_fileInfo;
    }

    void FileSelection::setFileInfo( const FileInfo &fileInfo )
    {
        m_fileInfo = fileInfo;
    }

}  // namespace workphone::editor
