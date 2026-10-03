#ifndef IFolderListing_h__
#define IFolderListing_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /**
     * An interface for a folder listing.
     */
    class WPCore_API IFolderExplorer : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IFolderExplorer() override;

        /** Gets the folder name. */
        virtual String getFolderName() const = 0;

        /** Sets the folder name. */
        virtual void setFolderName( const String &folderName ) = 0;

        /** Gets the list of files in this folder. */
        virtual Array<String> &getFiles() = 0;

        /** Gets the list of files in this folder. */
        virtual const Array<String> &getFiles() const = 0;

        /** Sets the list of files in this folder. */
        virtual void setFiles( const Array<String> &files ) = 0;

        /** Gets the list of sub-folders in this folder. */
        virtual Array<SmartPtr<IFolderExplorer>> getSubFolders() const = 0;

        /** Sets the list of sub-folders in this folder. */
        virtual void setSubFolders( const Array<SmartPtr<IFolderExplorer>> &subFolders ) = 0;

        /** Adds a sub-folder to the list of sub-folders in this folder. */
        virtual void addSubFolder( SmartPtr<IFolderExplorer> subFolder ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IFolderListing_h__
