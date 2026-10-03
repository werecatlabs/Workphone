#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/IO/FolderListing.hpp>

namespace workphone
{

    FolderListing::FolderListing() = default;

    FolderListing::~FolderListing() = default;

    String FolderListing::getFolderName() const
    {
        return m_folderName;
    }

    void FolderListing::setFolderName( const String &folderName )
    {
        m_folderName = folderName;
    }

    const Array<String> &FolderListing::getFiles() const
    {
        return m_files;
    }

    Array<String> &FolderListing::getFiles()
    {
        return m_files;
    }

    void FolderListing::setFiles( const Array<String> &files )
    {
        m_files = files;
    }

    Array<SmartPtr<IFolderExplorer>> FolderListing::getSubFolders() const
    {
        return m_subFolders;
    }

    void FolderListing::setSubFolders( const Array<SmartPtr<IFolderExplorer>> &subFolders )
    {
        m_subFolders = subFolders;
    }

    void FolderListing::addSubFolder( SmartPtr<IFolderExplorer> subFolder )
    {
        m_subFolders.push_back( subFolder );
    }

}  // namespace workphone
